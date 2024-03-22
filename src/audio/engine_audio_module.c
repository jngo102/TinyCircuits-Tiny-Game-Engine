#include "py/obj.h"
#include "py/mpthread.h"
#include "engine_audio_module.h"
#include "engine_audio_channel.h"
#include "resources/engine_sound_resource_base.h"
#include "debug/debug_print.h"
#include <stdlib.h>
#include <string.h>
#include "math/engine_math.h"
#include "utility/engine_time.h"



// 4 audio channels. Since audio is large it has to stay in
// flash, but flash is slow to get data from which is a problem
// when sample retrieval time/latency needs to be small to keep
// on track at the relatively high sample rate good audio requires.
// DMA is used to copy data into one of the dual buffer pairs
// each audio channel gets but there's only 12 channels on RP2040
// and one is used for the screen
volatile mp_obj_t channels[CHANNEL_COUNT];

// The master volume that all mixed samples are scaled by (0.0 ~ 1.0)
volatile float master_volume = 1.0f;


#if defined(__unix__)
    // Nothing to do
#elif defined(__arm__)
    #include "pico/stdlib.h"
    #include "hardware/dma.h"
    #include "hardware/structs/xip_ctrl.h"
    #include "hardware/pwm.h"
    #include "hardware/timer.h"
    #include "pico/multicore.h"


    // pg. 542: https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf
    // https://github.com/raspberrypi/pico-examples/blob/master/timer/hello_timer/hello_timer.c#L11-L57
    // https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#rpipdb65a0bdce0635d95877
    // https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#interrupt-numbers
    static struct repeating_timer repeating_audio_timer;

    void __no_inline_not_in_flash_func(engine_audio_handle_buffer)(audio_channel_class_obj_t *channel){
        // When 'buffer_byte_offset = 0' that means the buffer hasn't been filled before, fill it (see that after this function it is immediately incremented)
        // When 'buffer_byte_offset >= channel->buffer_end' that means the index has run out of data, fill it with more
        if(channel->buffers_byte_offsets[channel->reading_buffer_index] == 0 || channel->buffers_byte_offsets[channel->reading_buffer_index] >= channel->buffers_ends[channel->reading_buffer_index]){
            // Reset for the second case above
            channel->buffers_byte_offsets[channel->reading_buffer_index] = 0;

            // Using the sound resource base, fill this channel's
            // buffer with audio data from the source resource
            // channel->buffer_end = channel->source->fill_buffer(channel->source, channel->buffer, channel->source_byte_offset, CHANNEL_BUFFER_SIZE);

            if(channel->source == NULL){
                return;
            }

            uint8_t *current_source_data = channel->source->get_data(channel, CHANNEL_BUFFER_SIZE, &channel->buffers_ends[channel->reading_buffer_index]);

            // memcpy((uint8_t*)channel->buffer, (uint8_t*)current_source_data, channel->buffer_end);

            // https://github.com/raspberrypi/pico-examples/blob/eca13acf57916a0bd5961028314006983894fc84/flash/xip_stream/flash_xip_stream.c#L45-L48
            // while (!(xip_ctrl_hw->stat & XIP_STAT_FIFO_EMPTY))
            //     (void) xip_ctrl_hw->stream_fifo;
            // xip_ctrl_hw->stream_addr = (uint32_t)current_source_data;
            // xip_ctrl_hw->stream_ctr = channel->buffer_end;

            // Just in case we were too quick, wait while previous DMA might still be active
            if(dma_channel_is_busy(channel->dma_channel)){
                ENGINE_WARNING_PRINTF("AudioModule: Waiting on previous DMA transfer to complete, this ideally shouldn't happen");
                dma_channel_wait_for_finish_blocking(channel->dma_channel);
            }

            channel->reading_buffer_index = 1 - channel->reading_buffer_index;
            channel->filling_buffer_index = 1 - channel->filling_buffer_index;

            // https://github.com/raspberrypi/pico-examples/blob/master/flash/xip_stream/flash_xip_stream.c#L58-L70
            dma_channel_configure(
                channel->dma_channel,                                   // Channel to be configured
                &channel->dma_config,                                   // The configuration we just created
                channel->buffers[channel->filling_buffer_index],        // The initial write address
                current_source_data,                                    // The initial read address
                channel->buffers_ends[channel->filling_buffer_index],   // Number of transfers; in this case each is 1 byte
                true                                                    // Start immediately
            );

            // Filled amount will always be equal to or less than to 
            // 0 the 'size' passed to 'fill_buffer'. In the case it was
            // filled with something, increment to the amount filled
            // further. In the case it is filled with nothing, that means
            // the last fill made us reach the end of the source data,
            // figure out if this channel should stop or loop. If loop,
            // run again right away to fill with more data after resetting
            // 'source_byte_offset' 
            if(channel->buffers_ends[channel->filling_buffer_index] > 0){
                channel->source_byte_offset += channel->buffers_ends[channel->filling_buffer_index];
            }else{
                // Gets reset no matter what, whether looping or not
                channel->source_byte_offset = 0;

                // If not looping, disable/remove the source and stop this
                // channel from being played, otherwise, fill with start data
                if(channel->loop == false){
                    audio_channel_stop(channel);
                }else{
                    // Run right away to fill buffer with starting data since looping
                    engine_audio_handle_buffer(channel);
                }
            }

            // Not the best solution but when these are both 1 that means
            // no data has been loaded yet, block until data is loaded into
            // 1 then load the other buffer while not blocking so that it
            // can be switched to next time for reading
            if(channel->reading_buffer_index == channel->filling_buffer_index){
                dma_channel_wait_for_finish_blocking(channel->dma_channel);

                // Flip only the buffer to fill since we're going to fill it now
                channel->filling_buffer_index = 1 - channel->filling_buffer_index;

                // Fill the other buffer right now
                engine_audio_handle_buffer(channel);
            }
        }
    }


    // Samples each channel, adds, normalizes, and sets PWM
    bool __no_inline_not_in_flash_func(repeating_audio_callback)(struct repeating_timer *t){
        float temp_sample = 0;
        float total_sample = 0;
        uint8_t active_channel_count = 0;

        for(uint8_t icx=0; icx<CHANNEL_COUNT; icx++){

            audio_channel_class_obj_t *channel = channels[icx];

            if(channel->busy){
                continue;
            }

            sound_resource_base_class_obj_t *source = channel->source;

            // Go over every channel and check if set to something usable
            if(source != NULL){

                // Another active channel
                active_channel_count += 1;

                // Fill buffer with data whether first time or looping
                engine_audio_handle_buffer(channel);

                uint8_t buffer_index = channel->reading_buffer_index;
                uint16_t buffer_byte_offset = channel->buffers_byte_offsets[buffer_index];

                // Add samples to total
                switch(source->bytes_per_sample){
                    case 1:
                    {
                        temp_sample = (float)channel->buffers[buffer_index][buffer_byte_offset];                // Get 8-bit sample
                        temp_sample = temp_sample / (float)UINT8_MAX;                                           // Scale from 0 ~ 255 to 0.0 ~ 1.0
                        temp_sample = temp_sample * channel->gain;                                              // Scale sample by channel gain
                        temp_sample = (engine_math_clamp(temp_sample, 0.0f, 1.0f) * 512.0f * master_volume);    // Clamp volume scaled sample and scale from 0.0 ~ 1.0 to 0.0 to 512.0 (512 scaled by master_volume)
                    }
                    break;
                    case 2:
                    {   
                        temp_sample = (int16_t)(channel->buffers[buffer_index][buffer_byte_offset+1] << 8) + channel->buffers[buffer_index][buffer_byte_offset];  // Get 16-bit sample
                        temp_sample = temp_sample / (float)UINT16_MAX;                                                                                            // Scale from 0 ~ 65535 to 0.0 ~ 1.0
                        temp_sample = temp_sample * channel->gain;                                                                                                // Scale sample by channel gain
                        temp_sample = (engine_math_clamp(temp_sample, 0.0f, 1.0f) * 512.0f * master_volume);                                                      // Clamp volume scaled sample and scale from 0.0 ~ 1.0 to 0.0 to 512.0 (512 scaled by master_volume)
                    }
                    break;
                    default:
                        ENGINE_ERROR_PRINTF("AudioModule: Audio source with %d bytes per sample is not supported!", source->bytes_per_sample);
                }

                // Add the sample to the total
                total_sample += temp_sample;

                // Make sure to grab the next sample next time
                channel->buffers_byte_offsets[buffer_index] += source->bytes_per_sample;

                // Calculate the current time that we're at in the channel's source
                // t_current =  (1 / sample_rate [1/s]) * (source_byte_offset / bytes_per_sample)
                channel->time = (1.0f / source->sample_rate) * (channel->source_byte_offset / source->bytes_per_sample);
            }
        }
        
        // https://dsp.stackexchange.com/questions/3581/algorithms-to-mix-audio-signals-without-clipping: Viktor was wrong!
        // https://www.google.com/search?q=mixing+n+number+of+channels+audio+stackexchange&rlz=1C1GCEA_enUS850US850&oq=mixing+n+number+of+channels+audio+stackexchange&gs_lcrp=EgZjaHJvbWUyBggAEEUYOTIJCAEQIRgKGKABMgcIAhAhGKsC0gEINjk5OWowajeoAgCwAgA&sourceid=chrome&ie=UTF-8
        // https://electronics.stackexchange.com/questions/193983/mixing-audio-in-a-microprocessor       
        if(active_channel_count > 0){
            pwm_set_gpio_level(23, (uint32_t)(total_sample / active_channel_count));
        }

        return true;
    }
#endif


void engine_audio_setup_playback(){
    #if defined(__unix__)
        // Nothing to do
    #elif defined(__arm__)
        // Setup amplifier but make sure it is disabled while PWM is being setup
        gpio_init(26);
        gpio_set_dir(26, GPIO_OUT);
        gpio_put(26, 0);

        // Setup PWM audio pin, bit-depth, and frequency. Duty cycle
        // is only adjusted PWM parameter as samples are retrievd from
        // channel sources
        uint audio_pwm_pin_slice = pwm_gpio_to_slice_num(23);
        gpio_set_function(23, GPIO_FUNC_PWM);
        pwm_config audio_pwm_pin_config = pwm_get_default_config();
        pwm_config_set_clkdiv_int(&audio_pwm_pin_config, 1);
        pwm_config_set_wrap(&audio_pwm_pin_config, 512);   // 125MHz / 1024 = 122kHz
        pwm_init(audio_pwm_pin_slice, &audio_pwm_pin_config, true);

        // Now allow sound to play by enabling the amplifier
        gpio_put(26, 1);
    #endif
}

size_t stack_size = 4096*2;

void engine_audio_setup(){
    ENGINE_FORCE_PRINTF("EngineAudio: Setting up...");

    // Fill channels array with channels. This has to be done
    // before any callbacks try to access the channels array
    // (otherwise it would be trying to access all NULLs)
    // Setup should be run once per lifetime
    for(uint8_t icx=0; icx<CHANNEL_COUNT; icx++){
        channels[icx] = audio_channel_class_new(&audio_channel_class_type, 0, 0, NULL);
    }

    #if defined(__unix__)
        // Nothing to do
    #elif defined(__arm__)
        // Launch timer setup on other core. All audio playback
        // is done on the other core. This means that access
        // to channels and sources will need to be protected...
        // https://github.com/raspberrypi/pico-examples/blob/master/multicore/hello_multicore/multicore.c
        // multicore_reset_core1();
        // multicore_launch_core1(engine_audio_setup_playback);
        // engine_audio_setup_playback();
        if(add_repeating_timer_us((int64_t)((1.0/11025.0) * 1000000.0), repeating_audio_callback, NULL, &repeating_audio_timer) == false){
            mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("AudioModule: No timer slots available, could not audio callback!"));
        }
    #endif
}


void engine_audio_stop(){
    ENGINE_INFO_PRINTF('EngineAudio: Stopping all channels...');

    for(uint8_t icx=0; icx<CHANNEL_COUNT; icx++){
        // Check that each channel is not NULL since reset
        // can be called before hardware init
        if(channels[icx] != NULL){
            audio_channel_stop(channels[icx]);
        }
    }
}


/*  --- doc ---
    NAME: play
    DESC: Starts playing an audio source on a given channel and looping or not
    PARAM: [type=object]    [name=sound_resource] [value={ref_link:WaveSoundResource}]
    PARAM: [type=int]       [name=channel_index]  [value=0 ~ 3]                                                          
    PARAM: [type=boolean]   [name=loop]           [value=True or False]                                                  
    RETURN: {ref_link:AudioChannel}
*/ 
STATIC mp_obj_t engine_audio_play(mp_obj_t sound_resource_obj, mp_obj_t channel_index_obj, mp_obj_t loop_obj){
    // Should probably make sure this doesn't interfere with DMA or interrupt: TODO
    uint8_t channel_index = mp_obj_get_int(channel_index_obj);
    audio_channel_class_obj_t *channel = channels[channel_index];
    sound_resource_base_class_obj_t *source = sound_resource_obj;

    // Before anything, make sure to stop the channel incase of two `.play(...)` calls in a row
    audio_channel_stop(channel);

    channel->busy = true;

    channel->source = source;
    channel->loop = mp_obj_get_int(loop_obj);
    channel->done = false;

    // Very important to set this link! The source needs access to the channel that
    // is playing it (if one is) so that it can remove itself from the linked channel's
    // source
    source->channel = channel;

    channel->busy = false;

    return MP_OBJ_FROM_PTR(channel);
}
MP_DEFINE_CONST_FUN_OBJ_3(engine_audio_play_obj, engine_audio_play);


/*  --- doc ---
    NAME: set_volume
    DESC: Sets the master volume clamped between 0.0 and 1.0. In the future, this will be persistent and stored/restored using a settings file (TODO)
    PARAM: [type=float] [name=set_volume] [value=0.0 ~ 1.0]
    RETURN: None
*/ 
STATIC mp_obj_t engine_audio_set_volume(mp_obj_t new_volume){
    master_volume = engine_math_clamp(mp_obj_get_float(new_volume), 0.0f, 1.0f);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(engine_audio_set_volume_obj, engine_audio_set_volume);


/*  --- doc ---
    NAME: get_volume
    DESC: Returns the currently set master volume between 0.0 and 1.0
    RETURN: None
*/ 
STATIC mp_obj_t engine_audio_get_volume(){
    return mp_obj_new_float(master_volume);
}
MP_DEFINE_CONST_FUN_OBJ_0(engine_audio_get_volume_obj, engine_audio_get_volume);


/*  --- doc ---
    NAME: engine_audio
    DESC: Module for controlling/playing audio through four channels.
    ATTR: [type=object]     [name={ref_link:AudioChannel}]  [value=function]
    ATTR: [type=function]   [name={ref_link:play}]          [value=function] 
    ATTR: [type=function]   [name={ref_link:set_volume}]    [value=function]
    ATTR: [type=function]   [name={ref_link:get_volume}]    [value=function]
*/ 
STATIC const mp_rom_map_elem_t engine_audio_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_engine_audio) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_AudioChannel), (mp_obj_t)&audio_channel_class_type },
    { MP_OBJ_NEW_QSTR(MP_QSTR_play), (mp_obj_t)&engine_audio_play_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_volume), (mp_obj_t)&engine_audio_set_volume_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_volume), (mp_obj_t)&engine_audio_get_volume_obj },
};

// Module init
STATIC MP_DEFINE_CONST_DICT (mp_module_engine_audio_globals, engine_audio_globals_table);

const mp_obj_module_t engine_audio_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_engine_audio_globals,
};

MP_REGISTER_MODULE(MP_QSTR_engine_audio, engine_audio_user_cmodule);