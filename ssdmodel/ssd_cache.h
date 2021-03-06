#ifndef SSD_CACHE_H
#define SSD_CACHE_H

#include "ssd.h"
#include "ssd_timing.h"
#include "ssd_clean.h"
#include "ssd_gang.h"
#include "ssd_utils.h"
#include "modules/ssdmodel_ssd_param.h"
#include "double_linked_list.h"

//#include <pthread.h>
//#include <unistd.h>

#include "ssd_cache_parameters.h"

typedef struct {
	unsigned int upper_address;
	unsigned int flags;
    double last_access_time;
    double_linked_list_elem *dll_dirty_elem;
    int use_counter;
    int elem_num;
} ssd_cache_table_entry;

extern ssd_cache_table_entry ssd_cache_table[CACHE_SIZE];

//pthread_mutex_t lock;
//pthread_cond_t updating;
// pthread_t  thread_ID;
#define SSD_CACHE_FLAG_INVALID 	0
#define SSD_CACHE_FLAG_CLEAN 	1
#define SSD_CACHE_FLAG_DIRTY 	2

//#define CACHE_INVOKE_GAP        2 //in ms

#define CACHE_UPDATING_COST     0.1


//void *thread_function (ioreq_event *curr);
void ssd_cache_init(void);
void ssd_compute_cache_access_time_one_active_page(ssd_req **reqs, int total, int elem_num, ssd_t *s);
void ssd_cache_updating_complete(ioreq_event *curr);
//void ssd_activate_cache_updating(ssd_t *currdisk, int elem_num);
int ssd_invoke_cache_updating(int elem_num, ssd_t *s);
double ssd_cache_read_update (unsigned long blkno, int count,int elem_num, ssd_t *s);
void ssd_compute_access_time_one_active_page_read(ssd_req **reqs, int total, int elem_num, ssd_t *s);


#endif

