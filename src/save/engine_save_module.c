#include "engine_save_module.h"

#include "py/obj.h"
#include "engine_main.h"
#include "utility/engine_file.h"
#include "debug/debug_print.h"
#include <string.h>


mp_obj_str_t current_location = {
    .base.type = &mp_type_str,
    .hash = 0,
    .data = (byte[256]){},
    .len = 0,
};

// Need a duplicate for renaming
mp_obj_str_t temporary_location = {
    .base.type = &mp_type_str,
    .hash = 0,
    .data = (byte[256]){},
    .len = 0,
};


STATIC mp_obj_t engine_set_location(mp_obj_t location){
    ENGINE_INFO_PRINTF("EngineSave: Setting location");

    if(mp_obj_is_str(location) == false){
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EngineSave: ERROR: location path is not a string"));
    }

    const char *location_str = mp_obj_str_get_data(location, &current_location.len);

    // Cannot be larger than this since `temp-` takes up 5 characters
    if(current_location.len > 250){
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EngineSave: ERROR: location path too long (max length: 250)"));
    }

    // Set these equal to each other, for now
    temporary_location.len = current_location.len;

    // Copy base files name to current and temporary file names
    memcpy(current_location.data, location_str, current_location.len);
    memcpy(temporary_location.data, location_str, current_location.len);

    // Append `-temp` to end of temporary filename and increase length to account for it
    memcpy(temporary_location.data + temporary_location.len, "-temp", 5);
    temporary_location.len += 5;

    // If the file we are going to read from does not exist already, create it
    if(engine_file_exists(&current_location) == false){
        engine_file_open_create_write(0, &current_location);
        engine_file_close(0);
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(engine_set_location_obj, engine_set_location);


void engine_save_start(){
    // Open the the file to read from
    engine_file_open_read(0, &current_location);

    // Open the file to write to (temporary)
    engine_file_open_create_write(1, &temporary_location);
}


void engine_save_end(){
    // Close the reading and writing files
    engine_file_close(0);
    engine_file_close(1);

    // Remove the file we read data from and copied
    engine_file_remove(&current_location);

    // Rename temporary file to what we just deleted
    engine_file_rename(&temporary_location, &current_location);
}


STATIC mp_obj_t engine_save(mp_obj_t save_name, mp_obj_t obj){
    ENGINE_INFO_PRINTF("EngineSave: Saving");

    if(mp_obj_is_str(save_name) == false){
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EngineSave: ERROR: save name is not a string, cannot save!"));
    }

    engine_save_start();

    // Go through current file and copy lines over to
    // temporary file. If at any point a line is read
    // that contains `save_name` serialize `obj` then.
    // If get to the end and `save_name` wasn't found,
    // append the serialization of `obj` to the end of
    // the file

    engine_save_end();

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(engine_save_obj, engine_save);


STATIC mp_obj_t engine_save_module_init(){
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(engine_save_module_init_obj, engine_save_module_init);    


/* --- doc ---
   NAME: engine_save
   ID: engine_save
   DESC: Module for saving and loading data
*/
STATIC const mp_rom_map_elem_t engine_save_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_engine_save) },
    { MP_OBJ_NEW_QSTR(MP_QSTR___init__), (mp_obj_t)&engine_save_module_init_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_save), (mp_obj_t)&engine_save_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_location), (mp_obj_t)&engine_set_location_obj },
};

// Module init
STATIC MP_DEFINE_CONST_DICT(mp_module_engine_save_globals, engine_save_globals_table);

const mp_obj_module_t engine_save_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_engine_save_globals,
};

MP_REGISTER_MODULE(MP_QSTR_engine_save, engine_save_user_cmodule);
