#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdlib.h>
#include "py/obj.h"

#include "debug/debug_print.h"


typedef struct linked_list_node{
    void *object;
    struct linked_list_node *next;
    struct linked_list_node *previous;
} linked_list_node;


typedef struct _linked_list{
    struct linked_list_node *start;
    struct linked_list_node *end;
    uint16_t count;
    bool initialized;
} linked_list;


void linked_list_init(linked_list* list);
linked_list_node *setup_new_node(linked_list *list);
linked_list_node *linked_list_add_obj(linked_list *list, void *obj);
void linked_list_del_list_node(linked_list *list, linked_list_node *node);
void linked_list_clear(linked_list *list);


#endif  // LINKED_LIST_H
