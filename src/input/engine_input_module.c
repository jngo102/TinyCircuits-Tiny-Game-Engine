#include "py/obj.h"
#include "engine_input_common.h"

#ifdef __unix__
    #include "engine_input_sdl.h"
#else
    #include "engine_input_rp3.h"
#endif


void engine_input_setup(){
    #ifdef __unix__
        // Nothing to do
    #else
        engine_input_rp3_setup();
    #endif
}


// Update 'pressed_buttons' (usually called once per game loop)
void engine_input_update_pressed_buttons(){
    #ifdef __unix__
        engine_input_sdl_update_pressed_mask();
    #else
        engine_input_rp3_update_pressed_mask();
    #endif

    // XOR: set `1` if the corresponding bits in the byte are different
    just_changed_buttons = pressed_buttons_last ^ pressed_buttons;

    // Keep track of the buttons states from last time
    pressed_buttons_last = pressed_buttons;
}


/*  --- doc ---
    NAME: check_pressed
    DESC: For checking button presses. OR'ing together values means this returns true when all OR'ed buttons are pressed
    PARAM: [type=int]   [name=button_mask]  [value=single or OR'ed together enum/ints (e.g. 'engine_input.A | engine_input.B')]
    RETURN: None
*/ 
STATIC mp_obj_t engine_input_check_pressed(mp_obj_t button_mask_u16){
    uint16_t button_mask = mp_obj_get_int(button_mask_u16);

    // Check that the bits in the input button mask and the bits
    // in the internal button mask are all exactly on.
    return mp_obj_new_bool((pressed_buttons & button_mask) == button_mask);
}
MP_DEFINE_CONST_FUN_OBJ_1(engine_input_check_pressed_obj, engine_input_check_pressed);


/*  --- doc ---
    NAME: check_just_changed
    DESC: For checking buttons that were either just released or pressed. OR'ing together values means this returns true when all OR'ed buttons just changed
    PARAM: [type=int]   [name=button_mask]  [value=single or OR'ed together enum/ints (e.g. 'engine_input.A | engine_input.B')]
    RETURN: None
*/ 
STATIC mp_obj_t engine_input_check_just_changed(mp_obj_t button_mask_u16){
    uint16_t button_mask = mp_obj_get_int(button_mask_u16);

    return mp_obj_new_bool((just_changed_buttons & button_mask) == button_mask);
}
MP_DEFINE_CONST_FUN_OBJ_1(engine_input_check_just_changed_obj, engine_input_check_just_changed);


/*  --- doc ---
    NAME: engine_input
    DESC: Module for checking button presses
    ATTR: [type=function]   [name={ref_link:check_pressed}]         [value=function] 
    ATTR: [type=function]   [name={ref_link:check_just_changed}]    [value=function] 
    ATTR: [type=enum/int]   [name=A]                                [value=0b0000000000000001]
    ATTR: [type=enum/int]   [name=B]                                [value=0b0000000000000010]
    ATTR: [type=enum/int]   [name=DPAD_UP]                          [value=0b0000000000000100]
    ATTR: [type=enum/int]   [name=DPAD_DOWN]                        [value=0b0000000000001000]
    ATTR: [type=enum/int]   [name=DPAD_LEFT]                        [value=0b0000000000010000]
    ATTR: [type=enum/int]   [name=DPAD_RIGHT]                       [value=0b0000000000100000]
    ATTR: [type=enum/int]   [name=BUMPER_LEFT]                      [value=0b0000000001000000]
    ATTR: [type=enum/int]   [name=BUMPER_RIGHT]                     [value=0b0000000010000000]
    ATTR: [type=enum/int]   [name=MENU]                             [value=0b0000000100000000]
*/ 
STATIC const mp_rom_map_elem_t engine_input_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_engine_input) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_check_pressed), (mp_obj_t)&engine_input_check_pressed_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_check_just_changed), (mp_obj_t)&engine_input_check_just_changed_obj },

    { MP_ROM_QSTR(MP_QSTR_A), MP_ROM_INT(BUTTON_A) },
    { MP_ROM_QSTR(MP_QSTR_B), MP_ROM_INT(BUTTON_B) },
    { MP_ROM_QSTR(MP_QSTR_DPAD_UP), MP_ROM_INT(BUTTON_DPAD_UP) },
    { MP_ROM_QSTR(MP_QSTR_DPAD_DOWN), MP_ROM_INT(BUTTON_DPAD_DOWN) },
    { MP_ROM_QSTR(MP_QSTR_DPAD_LEFT), MP_ROM_INT(BUTTON_DPAD_LEFT) },
    { MP_ROM_QSTR(MP_QSTR_DPAD_RIGHT), MP_ROM_INT(BUTTON_DPAD_RIGHT) },
    { MP_ROM_QSTR(MP_QSTR_BUMPER_LEFT), MP_ROM_INT(BUTTON_BUMPER_LEFT) },
    { MP_ROM_QSTR(MP_QSTR_BUMPER_RIGHT), MP_ROM_INT(BUTTON_BUMPER_RIGHT) },
    { MP_ROM_QSTR(MP_QSTR_MENU), MP_ROM_INT(BUTTON_MENU) },
};

// Module init
STATIC MP_DEFINE_CONST_DICT (mp_module_engine_input_globals, engine_input_globals_table);

const mp_obj_module_t engine_input_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_engine_input_globals,
};

MP_REGISTER_MODULE(MP_QSTR_engine_input, engine_input_user_cmodule);