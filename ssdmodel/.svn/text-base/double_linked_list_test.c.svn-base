#include <stdlib.h>
#include <stdio.h>

#include "double_linked_list.h"

int main(int argc, char **argv){
    int i;

    printf("Testing double linked list.\n");

    double_linked_list_pool *dll_pool = double_linked_list_pool_init();
    double_linked_list *dll = double_linked_list_init(dll_pool);

    double_linked_list_elem *todelete1 = double_linked_list_add_queue( dll, 69);
    double_linked_list_print( dll );
    double_linked_list_reverse_print( dll );
    double_linked_list_delete_elem( dll, todelete1 );
    double_linked_list_print( dll );
    double_linked_list_reverse_print( dll );

    for( i=0; i<20; i++ )
        double_linked_list_add_queue( dll, i );

    double_linked_list_print( dll );
    double_linked_list_reverse_print( dll );

    for( i=0; i<30; i++ )
        if( double_linked_list_is_empty(dll) )
            printf("empty\n");
        else
            printf( "dequeue: %d\n", double_linked_list_pop_head(dll) );

    double_linked_list_print( dll );
    double_linked_list_reverse_print( dll );

    for( i=0; i<20; i++ )
        double_linked_list_add_queue( dll, i );
    for( i=0; i<5; i++ )
        if( double_linked_list_is_empty(dll) )
            printf("empty\n");
        else
            printf( "dequeue: %d\n", double_linked_list_pop_head(dll) );
    double_linked_list_elem *todelete = double_linked_list_add_queue( dll, 69);
    for( i=0; i<20; i++ )
        double_linked_list_add_queue( dll, i );

    double_linked_list_print( dll );
    double_linked_list_reverse_print( dll );
    double_linked_list_delete_elem( dll, todelete );
    double_linked_list_print( dll );
    double_linked_list_reverse_print( dll );

    for( i=0; i<40; i++ )
        if( double_linked_list_is_empty(dll) )
            printf("empty\n");
        else
            printf( "dequeue: %d\n", double_linked_list_pop_head(dll) );
    double_linked_list_print( dll );
    double_linked_list_reverse_print( dll );

    return 0;
}
