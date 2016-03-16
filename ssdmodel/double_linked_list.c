#include "double_linked_list.h"

#include <stdlib.h>
#include <stdio.h>

double_linked_list_pool *double_linked_list_pool_init(){
    double_linked_list_pool *dll_pool = malloc( sizeof( double_linked_list_pool) );

    if( dll_pool == NULL ){
        fprintf(stderr, "Unable to create dll pool: Out of memory\n");
        exit(1);
    }

    dll_pool->freed_pool = NULL;
    dll_pool->use_count = 0;

    return dll_pool;
}

double_linked_list *double_linked_list_init(double_linked_list_pool *dll_pool){
    double_linked_list *dll = malloc( sizeof( double_linked_list) );

    if( dll == NULL ){
        fprintf(stderr, "Unable to create dll: Out of memory\n");
        exit(1);
    }

    dll->head = NULL;
    dll->tail = NULL;

    dll_pool->use_count++;
    dll->pool = dll_pool;

    return dll;
}

void double_linked_list_reverse_print(double_linked_list *dll){
    printf("dll_reverse = [");

    double_linked_list_elem *elem = dll->tail;
    while( elem ){
        printf("%d, ", elem->data);
        elem = elem->previous;
    }

    printf("]\n");

    elem = dll->pool->freed_pool;
    int count = 0;
    while( elem ){
        count ++;
        elem = elem->next;
    }

    printf(" free %d\n", count);
}

void double_linked_list_print(double_linked_list *dll){
    printf("dll = [");

    double_linked_list_elem *elem = dll->head;
    while( elem ){
        printf("%d, ", elem->data);
        elem = elem->next;
    }

    printf("]\n");

    elem = dll->pool->freed_pool;
    int count = 0;
    while( elem ){
        count ++;
        elem = elem->next;
    }

    printf(" free %d\n", count);
}

double_linked_list_elem *double_linked_list_add_queue(double_linked_list *dll, int value){
    double_linked_list_elem *new = double_linked_list_get_free_elem(dll);

    new->data = value;

    new->next = NULL;
    new->previous = dll->tail;
    if( dll->tail != NULL )
        dll->tail->next = new;
    if( dll->head == NULL )
        dll->head = new;

    dll->tail = new;

    return new;
}

int double_linked_list_is_empty(double_linked_list *dll){
    return (dll->head == NULL);
}

int double_linked_list_pop_head(double_linked_list *dll){
    int value;

    if( dll->head == NULL ){
        fprintf(stderr, "double_linked_list_pop_head on empty list !\n");
        exit(1);
    }

    double_linked_list_elem *old = dll->head;
    value = old->data;

    dll->head = old->next;

    if( old->next == NULL ){ // It was the last element
        dll->tail = NULL;
    }else{
        dll->head->previous = NULL;
    }

    double_linked_list_free_elem( dll, old );

    return value;
}

void double_linked_list_delete_elem(double_linked_list *dll, double_linked_list_elem *elem){
    if( dll->head == elem )
        dll->head = elem->next;

    if( dll->tail == elem )
        dll->tail = elem->previous;

    if( elem->next != NULL )
        elem->next->previous = elem->previous;

    if( elem->previous != NULL )
        elem->previous->next = elem->next;

    double_linked_list_free_elem( dll, elem );
}

void double_linked_list_free_elem(double_linked_list *dll, double_linked_list_elem *old){
    old->next = dll->pool->freed_pool;
    dll->pool->freed_pool = old;
    old->previous = NULL;
    old->data = -1;
}

double_linked_list_elem *double_linked_list_get_free_elem(double_linked_list *dll){
    double_linked_list_elem *new = NULL;

    if( dll->pool->freed_pool == NULL ){
        new = malloc( sizeof(double_linked_list_elem) );
        new->data = -1;
        new->next = NULL;
        new->previous = NULL;
    }else{
        new = dll->pool->freed_pool;
        dll->pool->freed_pool = new->next;

        new->data = -1;
        new->next = NULL;
        new->previous = NULL;
    }

    return new;
}

