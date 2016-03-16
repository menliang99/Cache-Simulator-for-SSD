#ifndef HASHTABLE_LINKED__H
#define HASHTABLE_LINKED__H

#include "double_linked_list.h"

typedef struct {
    double_linked_list_pool *dll_pool;
} hashtable_linked;

hashtable_linked *hashtable_linked_init();

#endif
