#ifndef DOUBLE_LINKED_LIST__H
#define DOUBLE_LINKED_LIST__H


typedef struct s_double_linked_list_pool{
    struct s_double_linked_list_elem *freed_pool;

    int use_count;
} double_linked_list_pool;

typedef struct s_double_linked_list{
    struct s_double_linked_list_elem *head;
    struct s_double_linked_list_elem *tail;

    double_linked_list_pool *pool;
} double_linked_list;

typedef struct s_double_linked_list_elem {
    struct s_double_linked_list_elem *next;
    struct s_double_linked_list_elem *previous;

    int data;
} double_linked_list_elem;

// List creation
double_linked_list_pool *double_linked_list_pool_init();
double_linked_list *double_linked_list_init(double_linked_list_pool *dll_pool);

// Add/remove elements
int double_linked_list_pop_head(double_linked_list *dll);
double_linked_list_elem *double_linked_list_add_queue(double_linked_list *dll, int value);
void double_linked_list_delete_elem(double_linked_list *dll, double_linked_list_elem *elem);

// Printing function
void double_linked_list_print(double_linked_list *dll);
void double_linked_list_reverse_print(double_linked_list *dll);

// Tests
int double_linked_list_is_empty(double_linked_list *dll);

// Internal memory management
double_linked_list_elem *double_linked_list_get_free_elem(double_linked_list *dll);
void double_linked_list_free_elem(double_linked_list *dll, double_linked_list_elem *old);

#endif

