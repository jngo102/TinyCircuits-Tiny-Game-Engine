#include "engine_animation_tween.h"
#include "debug/debug_print.h"
#include "py/objtype.h"
#include "engine_animation_module.h"
#include "nodes/node_base.h"
#include "utility/engine_mp.h"

#include "math/vector2.h"
#include "math/vector3.h"
#include "draw/engine_color.h"
#include <string.h>

#include "../lib/cglm/include/cglm/ease.h"


// Function positions in this array must correlate to enums in `engine_animation_ease_types`
float (*ease[31])(float) = {
    &glm_ease_linear,

    &glm_ease_sine_in,
    &glm_ease_sine_out,
    &glm_ease_sine_inout,

    &glm_ease_quad_in,
    &glm_ease_quad_out,
    &glm_ease_quad_inout,

    &glm_ease_cubic_in,
    &glm_ease_cubic_out,
    &glm_ease_cubic_inout,

    &glm_ease_quart_in,
    &glm_ease_quart_out,
    &glm_ease_quart_inout,

    &glm_ease_quint_in,
    &glm_ease_quint_out,
    &glm_ease_quint_inout,

    &glm_ease_exp_in,
    &glm_ease_exp_out,
    &glm_ease_exp_inout,

    &glm_ease_circ_in,
    &glm_ease_circ_out,
    &glm_ease_circ_inout,

    &glm_ease_back_in,
    &glm_ease_back_out,
    &glm_ease_back_inout,

    &glm_ease_elast_in,
    &glm_ease_elast_out,
    &glm_ease_elast_inout,

    &glm_ease_bounce_out,
    &glm_ease_bounce_in,
    &glm_ease_bounce_inout
};


enum tween_value_types {tween_type_float, tween_type_vec2, tween_type_vec3, tween_type_color};
enum tween_direction {forwards, backwards};


mp_obj_t get_tweening_value(tween_class_obj_t *tween){
    // Always load the attribute since a reference might go stale
    // if the object's attribute is assigned to during tweening
    mp_obj_t tweening_value;
    if(tween->attr == 0){
        tweening_value = tween->object;
    }else{
        tweening_value = engine_mp_load_attr_maybe(tween->object, tween->attr);
    }

    // In case the attr or object is not available anymore, stop tweening
    if(tweening_value == MP_OBJ_NULL || tweening_value == mp_const_none){
        tween->finished = true;
        tween->time = 0.0f;
        tween->loop_type = engine_animation_loop_none;
        return mp_const_none;
    }

    return tweening_value;
}


void set_value_to_end(tween_class_obj_t *tween){
    // Get tweening value and make sure valid
    mp_obj_t tweening_value = get_tweening_value(tween);
    if(tweening_value == mp_const_none){
        return;
    }

    if(tween->tween_type == tween_type_float){
        mp_store_attr(tween->object, tween->attr, mp_obj_new_float(tween->end_0));
    }else if(tween->tween_type == tween_type_vec2){
        vector2_class_obj_t *value = tweening_value;
        value->x.value = tween->end_0;
        value->y.value = tween->end_1;
    }else if(tween->tween_type == tween_type_vec3){
        vector3_class_obj_t *value = tweening_value;
        value->x.value = tween->end_0;
        value->y.value = tween->end_1;
        value->z.value = tween->end_2;
    }else if(tween->tween_type == tween_type_color){
        color_class_obj_t *value = tweening_value;
        value->value = engine_color_from_rgb_float(tween->end_0, tween->end_1, tween->end_2);
    }
}



/* --- doc ---
   NAME: after
   ID: after
   DESC: Function that can be directly set or defined as a method in a class that is called after the tween completes (only called for ONE_SHOT mode)
   PARAM: [type=object] [name=tween] [value=object (the tween object that just finished)]
   RETURN: None
*/
static mp_obj_t tween_class_tick(mp_obj_t self_in, mp_obj_t dt_obj){
    ENGINE_INFO_PRINTF("Tween: tick!");

    tween_class_obj_t *tween = self_in;

    // Depending on what the user did and whats in ->self, get the base back
    if(mp_obj_is_instance_type(((mp_obj_base_t*)self_in)->type)){
        tween = mp_load_attr(self_in, MP_QSTR_base);
    }else{
        tween = self_in;
    }

    if(tween->paused == true){
        return mp_const_none;
    }

    if(tween->finished){
        switch(tween->loop_type){
            case engine_animation_loop_loop:
            {
                tween->finished = false;
                tween->time = 0.0f;
            }
            break;
            case engine_animation_loop_one_shot:
            {
                if(tween->after_called == false && tween->after != mp_const_none){
                    mp_obj_t exec[2];
                    exec[0] = tween->after;
                    exec[1] = tween->self;
                    mp_call_method_n_kw(0, 0, exec);
                    tween->after_called = true;
                }
                return mp_const_none;
            }
            break;
            case engine_animation_loop_ping_pong:
            {
                if(tween->tween_direction == backwards){
                    tween->tween_direction = forwards;
                }else if(tween->tween_direction == forwards){
                    tween->tween_direction = backwards;
                }

                tween->finished = false;
                tween->time = 0.0f;
            }
            break;
            default:
                return mp_const_none;   // By default, if finished (which is true by default) then just stop if no loop type is set, must have been `engine_animation_ease_none`
        }
    }

    // Get dt and add to total running time
    float dt = mp_obj_get_float(dt_obj);
    tween->time += (dt * tween->ping_pong_multiplier * tween->speed);

    // Get tweening value and make sure valid
    mp_obj_t tweening_value = get_tweening_value(tween);
    if(tweening_value == mp_const_none){
        return mp_const_none;
    }

    // If reached end of time, mark as finished
    // and stop. This lets the user catch the
    // `finished` flag for `loop` and `one_shot`
    if(tween->time >= tween->duration){
        tween->finished = true;
        tween->time = 0.0f;

        // Set value exactly equal to the end value
        set_value_to_end(tween);

        return mp_const_none;
    }

    // Figure out where we are in interpolation (percentage)
    float t = 0.0f;

    if(tween->tween_direction == backwards){
        t = 1.0f - (tween->time / tween->duration);
    }else if(tween->tween_direction == forwards){
        t = (tween->time / tween->duration);
    }

    // Ease the factor depending on which ease function was selected
    t = ease[tween->ease_type](t);

    if(tween->tween_type == tween_type_float){
        mp_store_attr(tween->object, tween->attr, mp_obj_new_float(tween->initial_0 + ((tween->end_0 - tween->initial_0) * t)));
    }else if(tween->tween_type == tween_type_vec2){
        vector2_class_obj_t *value = tweening_value;

        float x0 = tween->initial_0;
        float y0 = tween->initial_1;

        float x1 = tween->end_0;
        float y1 = tween->end_1;

        // https://stackoverflow.com/a/51067982
        value->x.value = x0 + t * (x1 - x0);
        value->y.value = y0 + t * (y1 - y0);
    }else if(tween->tween_type == tween_type_vec3){
        vector3_class_obj_t *value = tweening_value;

        float x0 = tween->initial_0;
        float y0 = tween->initial_1;
        float z0 = tween->initial_2;

        float x1 = tween->end_0;
        float y1 = tween->end_1;
        float z1 = tween->end_2;

        // https://stackoverflow.com/a/51067982
        value->x.value = x0 + t * (x1 - x0);
        value->y.value = y0 + t * (y1 - y0);
        value->z.value = z0 + t * (z1 - z0);
    }else if(tween->tween_type == tween_type_color){
        color_class_obj_t *value = tweening_value;

        float r0 = tween->initial_0;
        float g0 = tween->initial_1;
        float b0 = tween->initial_2;

        float r1 = tween->end_0;
        float g1 = tween->end_1;
        float b1 = tween->end_2;

        // https://www.alanzucconi.com/2016/01/06/colour-interpolation/#:~:text=can%20be%20done-,as%20such,-%3A
        // Lame way of interpolating RGB: TODO
        value->value = engine_color_from_rgb_float(r0 + (r1 - r0) * t, g0 + (g1 - g0) * t, b0 + (b1 - b0) * t);
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(tween_class_tick_obj, tween_class_tick);


/* --- doc ---
   NAME: start
   ID: tween_start
   DESC: Starts tweening a value. See https://easings.net/ for plots of the various easing functions
   PARAM: [type=object]      [name=object]         [value=object (the object that has an attribute to be tweened)]
   PARAM: [type=string]      [name=attribute_name] [value=string]
   PARAM: [type=None|object] [name=start]          [value=None|object (None means the current value, otherwise must be the same type as the attribute from `attribute_name`)]
   PARAM: [type=object]      [name=end]            [value=object (must be the same type as the attribute from `attribute_name`)]
   PARAM: [type=int]         [name=duration]       [value=any positive value representing milliseconds]
   PARAM: [type=float]       [name=speed]          [value=any]
   PARAM: [type=enum/int]    [name=loop_type]      [value=LOOP, ONE_SHOT, or PING_PONG]
   PARAM: [type=enum/int]    [name=ease_type]      [value=EASE_LINEAR, EASE_SINE_IN, EASE_SINE_OUT, EASE_SINE_IN_OUT, EASE_QUAD_IN, EASE_QUAD_OUT, EASE_QUAD_IN_OUT, EASE_CUBIC_IN, EASE_CUBIC_OUT, EASE_CUBIC_IN_OUT, EASE_QUART_IN, EASE_QUART_OUT, EASE_QUART_IN_OUT, EASE_QUINT_IN, EASE_QUINT_OUT, EASE_QUINT_IN_OUT, EASE_EXP_IN, EASE_EXP_OUT, EASE_EXP_IN_OUT, EASE_CIRC_IN, EASE_CIRC_OUT, EASE_CIRC_IN_OUT, EASE_BACK_IN, EASE_BACK_OUT, EASE_BACK_IN_OUT, EASE_ELAST_IN, EASE_ELAST_OUT, EASE_ELAST_IN_OUT, EASE_BOUNCE_IN, EASE_BOUNCE_OUT, or EASE_BOUNCE_IN_OUT]
   RETURN: None
*/
mp_obj_t tween_class_start(size_t n_args, const mp_obj_t *args){
    ENGINE_INFO_PRINTF("Tween: start");

    // self, always `tween_class_obj_t` since attr function handles getting base
    tween_class_obj_t *tween = args[0];

    tween->finished = false;
    tween->paused = false;
    tween->time = 0.0f;
    tween->tween_direction = forwards;
    tween->ping_pong_multiplier = 1.0f;
    tween->after_called = false;    // Set this to false every time (don't want to clear `after` entry but only want to call it once)

    // The object with the attribute that will be tweened
    tween->object = args[1];

    const char *attr_name = mp_obj_str_get_str(args[2]);

    // If the string attribute name is empty, error
    if(strlen(attr_name) == 0){
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Tween: ERROR: Trying to tween without an attribute name to lookup!"));
    }

    // Get the qstr for the attribute that should be tweened
    tween->attr = mp_obj_str_get_qstr(args[2]);

    const mp_obj_t start_value = mp_load_attr(tween->object, tween->attr);
    const mp_obj_type_t *value_type = mp_obj_get_type(start_value);
    const mp_obj_t tween_start_value = args[3] == mp_const_none ? start_value : args[3];
    const mp_obj_t tween_end_value = args[4];
    const mp_obj_type_t *start_type = mp_obj_get_type(tween_start_value);
    const mp_obj_type_t *end_type = mp_obj_get_type(tween_end_value);

    if(value_type == &mp_type_float && start_type == &mp_type_float && end_type == &mp_type_float){
        tween->initial_0  = mp_obj_get_float(tween_start_value);
        tween->end_0      = mp_obj_get_float(tween_end_value);
        tween->tween_type = tween_type_float;
    }else if(value_type == &vector2_class_type && start_type == &vector2_class_type && end_type == &vector2_class_type){
        vector2_class_obj_t *start = tween_start_value;
        vector2_class_obj_t *end = tween_end_value;

        tween->initial_0 = start->x.value;
        tween->initial_1 = start->y.value;

        tween->end_0 = end->x.value;
        tween->end_1 = end->y.value;

        tween->tween_type = tween_type_vec2;
    }else if(value_type == &vector3_class_type && start_type == &vector3_class_type && end_type == &vector3_class_type){
        vector3_class_obj_t *start = tween_start_value;
        vector3_class_obj_t *end = tween_end_value;

        tween->initial_0 = start->x.value;
        tween->initial_1 = start->y.value;
        tween->initial_2 = start->z.value;

        tween->end_0 = end->x.value;
        tween->end_1 = end->y.value;
        tween->end_2 = end->z.value;

        tween->tween_type = tween_type_vec3;
    }else if(engine_color_is_instance(start_value) &&
            (engine_color_is_instance(tween_start_value) || mp_obj_is_int(tween_start_value)) &&
            (engine_color_is_instance(tween_end_value) || mp_obj_is_int(tween_end_value))){
        uint16_t start = mp_obj_is_int(tween_start_value) ? mp_obj_get_int(tween_start_value) : ((color_class_obj_t*)tween_start_value)->value;
        uint16_t end = mp_obj_is_int(tween_end_value) ? mp_obj_get_int(tween_end_value) : ((color_class_obj_t*)tween_end_value)->value;

        tween->initial_0 = engine_color_get_r_float(start);
        tween->initial_1 = engine_color_get_g_float(start);
        tween->initial_2 = engine_color_get_b_float(start);

        tween->end_0 = engine_color_get_r_float(end);
        tween->end_1 = engine_color_get_g_float(end);
        tween->end_2 = engine_color_get_b_float(end);

        tween->tween_type = tween_type_color;
    }else{
        ENGINE_PRINTF("ERROR: Got types value: %s, start: %s, end %s:\n",
            mp_obj_get_type_str(start_value), mp_obj_get_type_str(tween_start_value), mp_obj_get_type_str(tween_end_value));
        mp_raise_msg_varg(&mp_type_RuntimeError,
            MP_ERROR_TEXT("Tween: ERROR: Unknown combination of types: value is %s, start is %s, end is %s"),
            mp_obj_get_type_str(start_value), mp_obj_get_type_str(tween_start_value), mp_obj_get_type_str(tween_end_value));
    }

    if(n_args >= 6){
        tween->duration = mp_obj_get_float(args[5]);
    }

    if(n_args >= 7){
        tween->speed = mp_obj_get_float(args[6]);
    }

    if(n_args >= 8){
        tween->loop_type = mp_obj_get_int(args[7]);

        if(tween->loop_type == engine_animation_loop_ping_pong){
            tween->ping_pong_multiplier = 2.0f;
        }
    }

    if(n_args >= 9){
        tween->ease_type = mp_obj_get_int(args[8]);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(tween_class_start_obj, 4, 9, tween_class_start);


/* --- doc ---
   NAME: stop
   ID: stop
   DESC: Stops tweening a value and sets to end value
   RETURN: None
*/
mp_obj_t tween_class_stop(mp_obj_t self_in){
    ENGINE_INFO_PRINTF("Tween: stop");
    tween_class_obj_t *tween = self_in;

    tween->finished = true;
    tween->paused = false;
    tween->loop_type = engine_animation_loop_none;

    set_value_to_end(tween);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(tween_class_stop_obj, tween_class_stop);



/* --- doc ---
   NAME: pause
   ID: pause
   DESC: Pauses tweening a value
   RETURN: None
*/
mp_obj_t tween_class_pause(mp_obj_t self_in){
    ENGINE_INFO_PRINTF("Tween: pause");
    tween_class_obj_t *tween = self_in;

    tween->paused = true;

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(tween_class_pause_obj, tween_class_pause);


/* --- doc ---
   NAME: unpause
   ID: unpause
   DESC: Resumes tweening a value after pause
   RETURN: None
*/
mp_obj_t tween_class_unpause(mp_obj_t self_in){
    ENGINE_INFO_PRINTF("Tween: unpause");
    tween_class_obj_t *tween = self_in;

    tween->paused = false;

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(tween_class_unpause_obj, tween_class_unpause);


/* --- doc ---
   NAME: restart
   ID: restart
   DESC: Restarts tweening a value
   RETURN: None
*/
mp_obj_t tween_class_restart(mp_obj_t self_in){
    ENGINE_INFO_PRINTF("Tween: restart");
    tween_class_obj_t *tween = self_in;

    tween->time = 0.0f;

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(tween_class_restart_obj, tween_class_restart);


mp_obj_t tween_class_del(mp_obj_t self_in){
    ENGINE_INFO_PRINTF("Tween: Deleted");

    tween_class_obj_t *tween = self_in;
    engine_animation_untrack(tween->list_node);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(tween_class_del_obj, tween_class_del);


// Return `true` if handled loading the attr from internal structure, `false` otherwise
bool tween_load_attr(tween_class_obj_t *tween, qstr attribute, mp_obj_t *destination){
    switch(attribute){
        case MP_QSTR___del__:
            destination[0] = MP_OBJ_FROM_PTR(&tween_class_del_obj);
            destination[1] = tween;
            return true;
        break;
        case MP_QSTR_start:
            destination[0] = MP_OBJ_FROM_PTR(&tween_class_start_obj);
            destination[1] = tween;
            return true;
        break;
        case MP_QSTR_stop:
            destination[0] = MP_OBJ_FROM_PTR(&tween_class_stop_obj);
            destination[1] = tween;
            return true;
        break;
        case MP_QSTR_pause:
            destination[0] = MP_OBJ_FROM_PTR(&tween_class_pause_obj);
            destination[1] = tween;
            return true;
        break;
        case MP_QSTR_unpause:
            destination[0] = MP_OBJ_FROM_PTR(&tween_class_unpause_obj);
            destination[1] = tween;
            return true;
        break;
        case MP_QSTR_restart:
            destination[0] = MP_OBJ_FROM_PTR(&tween_class_restart_obj);
            destination[1] = tween;
            return true;
        break;
        case MP_QSTR_tick:
            destination[0] = tween->tick;
            destination[1] = tween->self;   // Pass either native or instance class object depending
            return true;
        break;
        case MP_QSTR_during:
            destination[0] = tween->during;
            destination[1] = tween->self;   // Pass either native or instance class object depending
            return true;
        break;
        case MP_QSTR_after:
            destination[0] = tween->after;
            destination[1] = tween->self;   // Pass either native or instance class object depending
            return true;
        break;
        case MP_QSTR_base:
            destination[0] = tween;
            return true;
        break;
        case MP_QSTR_duration:
            destination[0] = mp_obj_new_float(tween->duration);
            return true;
        break;
        case MP_QSTR_speed:
            destination[0] = mp_obj_new_float(tween->speed);
            return true;
        break;
        case MP_QSTR_loop_type:
            destination[0] = mp_obj_new_int(tween->loop_type);
            return true;
        break;
        case MP_QSTR_ease_type:
            destination[0] = mp_obj_new_int(tween->ease_type);
            return true;
        break;
        case MP_QSTR_finished:
            destination[0] = mp_obj_new_bool(tween->finished);
            return true;
        break;
        default:
            return false; // Fail
    }
}


// Return `true` if handled storing the attr from internal structure, `false` otherwise
bool tween_store_attr(tween_class_obj_t *tween, qstr attribute, mp_obj_t *destination){
    switch(attribute){
        case MP_QSTR_duration:
            tween->duration = mp_obj_get_float(destination[1]);
            return true;
        break;
        case MP_QSTR_speed:
            tween->speed = mp_obj_get_float(destination[1]);
            return true;
        break;
        case MP_QSTR_loop_type:
            tween->loop_type = mp_obj_get_int(destination[1]);
            return true;
        break;
        case MP_QSTR_ease_type:
            tween->ease_type = mp_obj_get_int(destination[1]);
            return true;
        break;
        case MP_QSTR_finished:
            mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Tween: Changing the value of `finished` is not allowed!"));
            return true;
        break;
        case MP_QSTR_tick:
            tween->tick = destination[1];
            return true;
        break;
        case MP_QSTR_during:
            tween->during = destination[1];
            return true;
        break;
        case MP_QSTR_after:
            tween->after = destination[1];
            return true;
        break;
        default:
            return false; // Fail
    }
}


static mp_attr_fun_t tween_class_attr(mp_obj_t self_in, qstr attribute, mp_obj_t *destination){
    ENGINE_INFO_PRINTF("Accessing PhysicsCircle2DNode attr");

    bool is_instance = mp_obj_is_instance_type(((mp_obj_base_t*)self_in)->type);
    tween_class_obj_t *self;

    if(is_instance){
        mp_obj_t dest[2];
        dest[0] = MP_OBJ_NULL; // Indicate we want to load a value
        node_base_use_default_attr_handler(self_in, MP_QSTR_base, dest);
        self = dest[0];
    }else{
        self = self_in;
    }

    // Used for telling if custom load/store functions handled the attr
    bool attr_handled = false;

    if(destination[0] == MP_OBJ_NULL){          // Load
        attr_handled = tween_load_attr(self, attribute, destination);
    }else if(destination[1] != MP_OBJ_NULL){    // Store
        attr_handled = tween_store_attr(self, attribute, destination);

        // If handled, mark as successful store
        if(attr_handled) destination[0] = MP_OBJ_NULL;
    }

    // If this is a Python class instance and the attr was NOT
    // handled by the above, defer the attr to the instance attr
    // handler
    if(is_instance && attr_handled == false){
        node_base_use_default_attr_handler(self_in, attribute, destination);
    }

    return mp_const_none;
}


/* --- doc ---
   NAME: Tween
   ID: Tween
   DESC: Object for interpolating and easing parameters of objects
   ATTR:    [type=function]            [name={ref_link:tween_start}]  [value=function]
   ATTR:    [type=function]            [name={ref_link:stop}]         [value=function]
   ATTR:    [type=function]            [name={ref_link:pause}]        [value=function]
   ATTR:    [type=function]            [name={ref_link:unpause}]      [value=function]
   ATTR:    [type=function]            [name={ref_link:restart}]      [value=function]
   ATTR:    [type=function]            [name={ref_link:tick}]         [value=function]
   ATTR:    [type=function]            [name={ref_link:after}]        [value=function]
   ATTR:    [type=int]                 [name=duration]                [value=any positive value representing milliseconds]
   ATTR:    [type=float]               [name=speed]                   [value=any]
   ATTR:    [type=enum/int]            [name=loop_type]               [value=LOOP, ONE_SHOT, or PING_PONG]
   ATTR:    [type=enum/int]            [name=ease_type]               [value=EASE_LINEAR, EASE_SINE_IN, EASE_SINE_OUT, EASE_SINE_IN_OUT, EASE_QUAD_IN, EASE_QUAD_OUT, EASE_QUAD_IN_OUT, EASE_CUBIC_IN, EASE_CUBIC_OUT, EASE_CUBIC_IN_OUT, EASE_QUART_IN, EASE_QUART_OUT, EASE_QUART_IN_OUT, EASE_QUINT_IN, EASE_QUINT_OUT, EASE_QUINT_IN_OUT, EASE_EXP_IN, EASE_EXP_OUT, EASE_EXP_IN_OUT, EASE_CIRC_IN, EASE_CIRC_OUT, EASE_CIRC_IN_OUT, EASE_BACK_IN, EASE_BACK_OUT, EASE_BACK_IN_OUT, EASE_ELAST_IN, EASE_ELAST_OUT, EASE_ELAST_IN_OUT, EASE_BOUNCE_IN, EASE_BOUNCE_OUT, or EASE_BOUNCE_IN_OUT]
   ATTR:    [type=boolean]             [name=finished]                [value=True or False (read-only)]
   OVRR:    [type=function]            [name={ref_link:tick}]         [value=function]
   OVRR:    [type=function]            [name={ref_link:after}]        [value=function]
*/
mp_obj_t tween_class_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    ENGINE_INFO_PRINTF("New Tween");

    bool inherited = false;

    if(n_args == 1 && mp_obj_is_instance_type(((mp_obj_base_t*)args[0])->type)){
        inherited = true;
    }else{
        inherited = false;
    }

    tween_class_obj_t *self = mp_obj_malloc_with_finaliser(tween_class_obj_t, &tween_class_type);
    self->self = self;
    self->tick = MP_OBJ_FROM_PTR(&tween_class_tick_obj);
    self->list_node = engine_animation_track(self);
    self->base.type = &tween_class_type;

    self->loop_type = engine_animation_loop_none;
    self->ease_type = engine_animation_ease_linear;
    self->duration = 1000.0f;
    self->time = 0.0f;
    self->speed = 1.0f;
    self->finished = true;
    self->paused = true;
    self->object = mp_const_none;
    self->after_called = false;
    self->attr = 0;
    self->after = mp_const_none;
    self->during = mp_const_none;


    if(inherited == true){
        // Get the Python class instance
        mp_obj_t child_class_obj = args[0];

        // Because the instance doesn't have a `node_base` yet, restore the
        // instance type original attr function for now (otherwise get core abort)
        node_base_set_attr_handler_default(child_class_obj);

        // Look for function overrides otherwise use the defaults
        mp_obj_t dest[2];
        mp_load_method_maybe(child_class_obj, MP_QSTR_tick, dest);
        if(dest[0] == MP_OBJ_NULL && dest[1] == MP_OBJ_NULL){   // Did not find method (set to default)
            self->tick = MP_OBJ_FROM_PTR(&tween_class_tick_obj);
        }else{                                                  // Likely found method (could be attribute)
            self->tick = dest[0];
        }

        mp_load_method_maybe(child_class_obj, MP_QSTR_during, dest);
        if(dest[0] == MP_OBJ_NULL && dest[1] == MP_OBJ_NULL){   // Did not find method (set to default)
            self->during = mp_const_none;
        }else{                                                  // Likely found method (could be attribute)
            self->during = dest[0];
        }

        mp_load_method_maybe(child_class_obj, MP_QSTR_after, dest);
        if(dest[0] == MP_OBJ_NULL && dest[1] == MP_OBJ_NULL){   // Did not find method (set to default)
            self->after = mp_const_none;
        }else{                                                  // Likely found method (could be attribute)
            self->after = dest[0];
        }

        // Store one pointer on the instance. Need to be able to get the
        // node base that contains a pointer to the engine specific data we
        // care about
        // mp_store_attr(node_instance, MP_QSTR_node_base, node_base);
        mp_store_attr(child_class_obj, MP_QSTR_base, self);

        // Store default Python class instance attr function
        // and override with custom intercept attr function
        // so that certain callbacks/code can run (see py/objtype.c:mp_obj_instance_attr(...))
        node_base_set_attr_handler(child_class_obj, tween_class_attr);

        // Need a way to access the object node instance instead of the native type for callbacks (tick, draw, collision)
        self->self = child_class_obj;
    }

    return MP_OBJ_FROM_PTR(self);
}


// Class attributes
static const mp_rom_map_elem_t tween_class_locals_dict_table[] = {

};
static MP_DEFINE_CONST_DICT(tween_class_locals_dict, tween_class_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    tween_class_type,
    MP_QSTR_Tween,
    MP_TYPE_FLAG_NONE,

    make_new, tween_class_new,
    attr, tween_class_attr,
    locals_dict, &tween_class_locals_dict
);