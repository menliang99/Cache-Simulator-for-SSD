#include "hashtable_linked.h"

#include <stdlib.h>
#include <stdio.h>

hashtable_linked *hashtable_linked_init(){
    hashtable_linked *htl = malloc( sizeof(hashtable_linked) );

    if( htl == NULL ){
        fprintf(stderr, "Unable to allocate memory for htl.\n");
        exit(1);
    }

    htl->dll_pool = double_linked_list_pool_init();

    return htl;
}

