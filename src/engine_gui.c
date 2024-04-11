#include "engine_gui.h"
#include "input/engine_input_common.h"
#include "input/engine_input_module.h"
#include "nodes/2D/gui_button_2d_node.h"
#include "math/engine_math.h"
#include <float.h>


linked_list engine_guis;
engine_node_base_t *focused_gui_node_base = NULL;


bool gui_focused = false;


linked_list_node *engine_gui_track(engine_node_base_t *obj){
    ENGINE_INFO_PRINTF("Tracking new gui node");
    return linked_list_add_obj(&engine_guis, obj);
}


void engine_gui_untrack(linked_list_node *gui_list_node){
    ENGINE_INFO_PRINTF("Untracking gui node");
    linked_list_del_list_node(&engine_guis, gui_list_node);
}


void engine_gui_clear_all(){
    ENGINE_INFO_PRINTF("Untracking all gui nodes...");
    linked_list_clear(&engine_guis);
}


bool engine_gui_get_focus(){
    return gui_focused;
}


void engine_gui_toggle_focus(){
    gui_focused = !gui_focused;

    // If just focused, loop through all elements and focus
    // the first node if no other node is already focused
    if(gui_focused && engine_guis.start != NULL){
        linked_list_node *current_gui_list_node = engine_guis.start;

        while(current_gui_list_node != NULL){
            engine_node_base_t *gui_node_base = current_gui_list_node->object;
            engine_gui_button_2d_node_class_obj_t *gui_button = gui_node_base->node;
            
            // Don't need to focus any nodes, this one already is focused, end function
            if(gui_button->focused){
                return;
            }

            current_gui_list_node = current_gui_list_node->next;
        }

        // Made it this far, must mean that no gui nodes are focused, focus the first one
        engine_node_base_t *first_gui_node_base = engine_guis.start->object;
        engine_gui_button_2d_node_class_obj_t *first_gui_button = first_gui_node_base->node;
        first_gui_button->focused = true;
        focused_gui_node_base = first_gui_node_base;
    }
}


engine_node_base_t *engine_gui_get_focused(){
    return focused_gui_node_base;
}


bool engine_gui_is_left_check(float angle_radians){
    // 135 to 225
    if(angle_radians >= (3.0f*PI/4.0f) && angle_radians <= (5.0f*PI/4.0f)){
        return true;
    }else{
        return false;
    }
}


bool engine_gui_is_right_check(float angle_radians){
    // 315 to 45
    if((angle_radians >= -PI/4.0f && angle_radians <= PI/4.0f)){
        return true;
    }else{
        return false;
    }
}


bool engine_gui_is_up_check(float angle_radians){
    // 225 to 315
    if(angle_radians <= (3.0f*PI/4.0f) && angle_radians <= -PI/4.0f){
        return true;
    }else{
        return false;
    }
}


bool engine_gui_is_down_check(float angle_radians){
    // 45 to 135
    if(angle_radians >= (PI/4.0f) && angle_radians <= (3.0f*PI/4.0f)){
        return true;
    }else{
        return false;
    }
}


// Given `focused_gui_node_base`, find the next closest
// gui node.
void engine_gui_select_closest(bool (direction_check)(float)){
    engine_gui_button_2d_node_class_obj_t *focused_gui_node = focused_gui_node_base->node;
    vector2_class_obj_t *origin = focused_gui_node->position;

    linked_list_node *current_gui_list_node = engine_guis.start;

    engine_node_base_t *closest_gui_node_base = NULL;
    float shortest_distance = FLT_MAX;

    while(current_gui_list_node != NULL){

        // If we're looping through all the gui nodes and
        // come across the already focused node, skip it
        if(current_gui_list_node->object == focused_gui_node_base){
            current_gui_list_node = current_gui_list_node->next;
            continue;
        }

        engine_node_base_t *gui_node_base = current_gui_list_node->object;

        engine_gui_button_2d_node_class_obj_t *gui_button = gui_node_base->node;
        vector2_class_obj_t *other = gui_button->position;
        float angle_radians = engine_math_angle_between(origin->x.value, origin->y.value, other->x.value, other->y.value);

        if(direction_check(angle_radians)){
            float distance = engine_math_distance_between(origin->x.value, origin->y.value, other->x.value, other->y.value);
            if(distance < shortest_distance){
                shortest_distance = distance;
                closest_gui_node_base = gui_node_base;
            }
        }

        current_gui_list_node = current_gui_list_node->next;
    }

    if(closest_gui_node_base != NULL){
        engine_gui_button_2d_node_class_obj_t *closest_gui_button = closest_gui_node_base->node;
        closest_gui_button->focused = true;

        focused_gui_node->focused = false;
        focused_gui_node_base = closest_gui_node_base;
    }
}


void engine_gui_tick(){
    // Every tick, see if the button to toggle GUI focus was pressed.
    // If the GUI toggle button is 0 that means None was set for the
    // toggle button and that we should not automaticaly switch focus
    uint16_t gui_toggle_button = engine_input_get_gui_toggle_button();
    if(gui_toggle_button != 0 && check_just_pressed(gui_toggle_button)){
        engine_gui_toggle_focus();
    }

    // Only run the GUI selection logic when the
    // gui is focused due to the input module
    if(gui_focused == false){
        return;
    }

    if(check_just_pressed(BUTTON_DPAD_LEFT)){
        engine_gui_select_closest(engine_gui_is_left_check);
    }else if(check_just_pressed(BUTTON_DPAD_RIGHT)){
        engine_gui_select_closest(engine_gui_is_right_check);
    }else if(check_just_pressed(BUTTON_DPAD_UP)){
        engine_gui_select_closest(engine_gui_is_up_check);
    }else if(check_just_pressed(BUTTON_DPAD_DOWN)){
        engine_gui_select_closest(engine_gui_is_down_check);
    }else if(check_just_pressed(BUTTON_A)){

    }
}