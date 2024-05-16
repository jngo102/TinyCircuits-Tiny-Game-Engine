#include "engine_physics_module.h"

#include "py/obj.h"
#include "engine_main.h"
#include "nodes/node_base.h"
#include "display/engine_display_common.h"
#include "physics/engine_physics.h"
#include "physics/collision_contact_2d.h"


// Could add another method that takes a vector2 to set gravity, just this for now
/* --- doc ---
   NAME: set_gravity
   ID: set_gravity
   DESC: Sets the gravity that all objects are affected by. TODO: lighter objects do not fall slower currently...
   PARAM: [type=float] [name=x] [value=any]
   PARAM: [type=float] [name=y] [value=any]
   RETURN: None
*/
STATIC mp_obj_t engine_physics_set_gravity(mp_obj_t x, mp_obj_t y){
    ENGINE_INFO_PRINTF("EnginePhysics: Setting gravity");
    engine_physics_gravity_x = mp_obj_get_float(x);
    engine_physics_gravity_y = mp_obj_get_float(y);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(engine_physics_set_gravity_obj, engine_physics_set_gravity);


/* --- doc ---
   NAME: get_gravity
   ID: get_gravity
   DESC: Gets the gravity that all objects are affected by
   RETURN: {ref_link:Vector2}
*/
STATIC mp_obj_t engine_physics_get_gravity(){
    ENGINE_INFO_PRINTF("EnginePhysics: Getting gravity");
    return vector2_class_new(&vector2_class_type, 2, 0, (mp_obj_t[]){mp_obj_new_float(engine_physics_gravity_x), mp_obj_new_float(engine_physics_gravity_y)});
}
MP_DEFINE_CONST_FUN_OBJ_0(engine_physics_get_gravity_obj, engine_physics_get_gravity);


STATIC mp_obj_t engine_physics_module_init(){
    engine_main_raise_if_not_initialized();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(engine_physics_module_init_obj, engine_physics_module_init);    


/* --- doc ---
   NAME: engine_physics
   ID: engine_physics
   DESC: Module for controlling physics and for common physics collision shapes
   ATTR: [type=function] [name={ref_link:set_gravity}]                     [value=function]
   ATTR: [type=function] [name={ref_link:get_gravity}]                     [value=function]
*/
STATIC const mp_rom_map_elem_t engine_physics_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_engine_physics) },
    { MP_OBJ_NEW_QSTR(MP_QSTR___init__), (mp_obj_t)&engine_physics_module_init_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_CollisionContact2D), (mp_obj_t)&collision_contact_2d_class_type},
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_gravity), (mp_obj_t)&engine_physics_set_gravity_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_gravity), (mp_obj_t)&engine_physics_get_gravity_obj },
};

// Module init
STATIC MP_DEFINE_CONST_DICT(mp_module_engine_physics_globals, engine_physics_globals_table);

const mp_obj_module_t engine_physics_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_engine_physics_globals,
};

MP_REGISTER_MODULE(MP_QSTR_engine_physics, engine_physics_user_cmodule);
