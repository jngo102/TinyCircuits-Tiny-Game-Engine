#include "engine_display.h"
#include "engine_display_common.h"


// https://stackoverflow.com/questions/43287103/predefined-macro-to-distinguish-arm-none-eabi-gcc-from-gcc
#ifdef __unix__
    #include "engine_display_sdl.h"
#else
    #include "utility/debug_print.h"
#endif


void engine_display_init(){
    #ifdef __unix__
        engine_display_sdl_init();
    #else
        ENGINE_WARNING_PRINTF("Display init not implemented for this platform");
    #endif
}


void engine_display_send(){
    // Send the screen buffer to the display
    // TODO
    engine_get_active_screen_buffer();
    engine_switch_active_screen_buffer();
}