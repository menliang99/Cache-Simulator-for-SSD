#include "ssd_cache.h"
#include "ssd.h"
#include "ssd_timing.h"
#include "ssd_clean.h"
#include "ssd_gang.h"
#include "ssd_utils.h"

#include "../src/disksim_global.h"
#include "modules/ssdmodel_ssd_param.h"
#include "double_linked_list.h"
#include "red_black_tree.h"



ssd_cache_table_entry ssd_cache_table[CACHE_SIZE];
double_linked_list_pool *ssd_cache_dll_pool = NULL;

rb_red_blk_tree *ssd_cache_dirty_rbt = NULL;

double cache_entry_access_time_most_recent;
void increment_usage_count(int cache_index);
void ssd_background_updating(int elem_num, ssd_t *s);

int debug_counter2 = 0;
int p = 0;

int selected_way_no;  
float cache_occupy_number;


int rb_IntComp(const void* a,const void* b) {
    if( *(int*)a > *(int*)b) return(1);
    if( *(int*)a < *(int*)b) return(-1);
    return(0);
}


void ssd_cache_init()
{
    int i;

    for(i=0; i<CACHE_SIZE; i++){
        ssd_cache_table[i].flags = SSD_CACHE_FLAG_INVALID;
        ssd_cache_table[i].last_access_time = 0;
        ssd_cache_table[i].dll_dirty_elem = NULL;
        ssd_cache_table[i].use_counter = 0;
	ssd_cache_table[i].elem_num = -1;
    }

    cache_entry_access_time_most_recent = 0;
    cache_occupy_number = 0.0;
    ssd_cache_dll_pool = double_linked_list_pool_init();
    ssd_cache_dirty_rbt = RBTreeCreate( rb_IntComp, NullFunction, NullFunction, NullFunctionConst, NullFunction );
}




double_linked_list *get_dll_from_rbt( int key ){
    double_linked_list *dll;

    rb_red_blk_node *rbn = RBExactQuery(ssd_cache_dirty_rbt, &key);

    if( rbn == NULL || rbn == ssd_cache_dirty_rbt->nil ){
        int *key_ptr = malloc(sizeof(int));
        *key_ptr = key;

        dll = double_linked_list_init(ssd_cache_dll_pool);
        RBTreeInsert( ssd_cache_dirty_rbt, key_ptr, dll);
    }else
        dll = rbn->info;

    return dll;
}

void ssd_cache_printcacheline(int cl){
    fprintf(stderr, "Cache line %d: addr=%8x, ",
            cl,
            ssd_cache_table[cl].upper_address);

    if( ssd_cache_table[cl].flags == SSD_CACHE_FLAG_INVALID )
        fprintf(stderr, "INVALID");
    if( ssd_cache_table[cl].flags == SSD_CACHE_FLAG_CLEAN )
        fprintf(stderr, "CLEAN");
    if( ssd_cache_table[cl].flags == SSD_CACHE_FLAG_DIRTY )
        fprintf(stderr, "DIRTY");

    fprintf(stderr, ", lat=%f\n",
            ssd_cache_table[cl].last_access_time);
}

/* Function:
 *     check whether it is ok to write the data directly into the cache
 * Input:
 *     blkno_start: the starting address of logical block, which is size of 512 bytes.
 *     count: the number of logical blocks to write. It is always 8 with this version.
 * Return Value:
 *     1: it is ok to write to the cache directly; the global variable, selected_way_no, keeps the way to be written to
 *     0: it is not ok to write to the cache directly; the global variable, selected_way_no, keeps the way to be evicted
 */
int is_write_cacheable(unsigned long blkno, int count){
    unsigned long apn = blkno/8;          //s->params.page_size = 8   
    int upper_address = apn / (CACHE_SIZE / CACHE_WAY);    //indicate which part does the cache map into the SSD
    int cache_offset  = apn % (CACHE_SIZE / CACHE_WAY);  //indicate which line/way does the data access into the cache

    int i;
    // The first step: check whether the address has been cached
    for (i=0; i<CACHE_WAY; i++) {
        int cache_index = cache_offset*CACHE_WAY+i;

        if( ssd_cache_table[cache_index].upper_address == upper_address ) {
            selected_way_no = i;
            return 1;
        }
    }

    // The second step: find the first invalid cache line in this set
    for (i=0; i<CACHE_WAY; i++) {
        int cache_index = cache_offset*CACHE_WAY+i;

        if( ssd_cache_table[cache_index].flags == SSD_CACHE_FLAG_INVALID ) {
            selected_way_no = i;
            return 1;
        }
    }

    // The third step: find the least recently accessed clean cache line in this set
    int have_clean_cache = 0;
    double least_recently_access_time = simtime;

    for (i=0; i<CACHE_WAY; i++) {
        int cache_index = cache_offset*CACHE_WAY+i;

        if( (ssd_cache_table[cache_index].flags == SSD_CACHE_FLAG_CLEAN) )//&&
               // (ssd_cache_table[cache_index].last_access_time<=least_recently_access_time) )   //changed to <=, debugging in 2011,04,14 ??????
	{ 

           if ((ssd_cache_table[cache_index].last_access_time > simtime))
 		fprintf(stderr, "ERROR!!The simulation time is %f. last access time is %f.\n", simtime, ssd_cache_table[cache_index].last_access_time);

            least_recently_access_time = ssd_cache_table[cache_index].last_access_time;
            selected_way_no = i;
            have_clean_cache = 1;
            return 1;
        }
    }

    // The fourth and last step: find the least recently access dirty cache line in this set, failed to write cache directly
    least_recently_access_time = simtime;
    for (i=0; i<CACHE_WAY; i++) {
        int cache_index = cache_offset*CACHE_WAY+i;

        if( ssd_cache_table[cache_index].last_access_time<least_recently_access_time ) {

            least_recently_access_time = ssd_cache_table[cache_index].last_access_time;
            selected_way_no = i;
        }
    }

    return 0;
}

/* Function: write date into cache
 * Input:
 *     blkno_start: the starting address of logical block, which is size of 512 bytes.
 *     count: the number of logical blocks to write. It is always 8 with this version.
 *     the implicit input, selected_way_no: the NO. of way to be written to
 */
void ssd_cache_write(unsigned long blkno, int count, int elem_num)
{
    unsigned long apn = blkno/8;           //s->params.page_size = 8  apn means absolute page number
    int upper_address = apn / (CACHE_SIZE / CACHE_WAY);
    int cache_offset  = apn % (CACHE_SIZE / CACHE_WAY);
    int cache_index = cache_offset * CACHE_WAY + selected_way_no;

    cache_entry_access_time_most_recent = simtime;
    ssd_current_idle_time = simtime;      //  ssd_current_idle_time is intruduced as a global variabile to coherent with the selective cache by menliang
    cache_total_write_count++; 
    cache_write_count ++;

    if( ssd_cache_table[cache_index].flags != SSD_CACHE_FLAG_DIRTY )
    {
        cache_occupy_number++;
        ssd_cache_table[cache_index].flags = SSD_CACHE_FLAG_DIRTY;
        if( ssd_cache_table[cache_index].upper_address != upper_address )
            ssd_cache_table[cache_index].use_counter = 1;
        else
            ssd_cache_table[cache_index].use_counter++;

        double_linked_list *dll = get_dll_from_rbt( ssd_cache_table[cache_index].use_counter );        // Get list from rbt
        ssd_cache_table[cache_index].dll_dirty_elem =  double_linked_list_add_queue( dll, cache_index );   // Add cache line to the tree
    }
    else
    {                                                     
        if( ssd_cache_table[cache_index].upper_address != upper_address )  //Upper_address is not equal but offset equal, Cache block should write back??
         {  
            fprintf(stderr, "This should never happen; the old cache line should have been evicted.");
            exit(1);
         }
        increment_usage_count(cache_index);
    }

    ssd_cache_table[cache_index].upper_address = upper_address;
    ssd_cache_table[cache_index].last_access_time = cache_entry_access_time_most_recent;
    ssd_cache_table[cache_index].elem_num = elem_num;
}

// evict the dirty cache line into the flash   passtively updating the cache
double ssd_cache_clean(unsigned long blkno, int count, int elem_num, ssd_t *s){
    double cost;

    unsigned long apn = blkno/8;
    int cache_offset = apn % (CACHE_SIZE / CACHE_WAY);
    int cache_index = cache_offset * CACHE_WAY + selected_way_no;
    int upper_address = ssd_cache_table[cache_index].upper_address;
                                                                                                                                                                                                                                       
    int use_counter = ssd_cache_table[cache_index].use_counter;

    // Remove from the dirty list.
    ASSERT (ssd_cache_table[cache_index].elem_num == elem_num );

    double_linked_list *dll = get_dll_from_rbt( use_counter );        // Get list from rbt
    //  double_linked_list_print(dll);
    double_linked_list_delete_elem( dll, ssd_cache_table[cache_index].dll_dirty_elem );
     //double_linked_list_print(dll);
    ssd_cache_table[cache_index].dll_dirty_elem = NULL;
    ssd_cache_table[cache_index].use_counter = 0;
    ssd_cache_table[cache_index].last_access_time = simtime;  //added by menliang

    // Switch back the cache line to clean
    ssd_cache_table[cache_index].flags = SSD_CACHE_FLAG_CLEAN;
    cache_occupy_number--;


    cache_total_clean_count++;
    cache_clean_count++;


    unsigned long blkno_evict = (upper_address * (CACHE_SIZE / CACHE_WAY) + cache_offset) * 8;
    elem_num = s->timing_t->choose_element(s->timing_t, blkno_evict);
    ssd_cache_table[cache_index].elem_num = elem_num;


    cache_entry_access_time_most_recent = simtime;
    ssd_current_idle_time = simtime;

    ssd_background_updating(elem_num, s);  //first replenish the ssd background procedure.

    switch(s->params.write_policy) {
        case DISKSIM_SSD_WRITE_POLICY_SIMPLE:

            cost = ssd_write_policy_simple(blkno_evict, 8, elem_num, s);
            break;

        case DISKSIM_SSD_WRITE_POLICY_OSR:   //

            cost = ssd_write_one_active_page(blkno_evict, 8, elem_num, s);  //may cause ssd cleaning operation
            break;

        default:
            fprintf(stderr, "Error: unknown write policy %d in ssd_compute_access_time\n",
                    s->params.write_policy);
            exit(1);
    }
  
    s->elements[elem_num].last_access_time = simtime + cost;  //update element state once it has been read or written.

    return cost;
}

void ssd_compute_cache_access_time_one_active_page(ssd_req **reqs, int total, int elem_num, ssd_t *s){   //called 2181869 times
    // we assume that requests have been broken down into page sized chunks, one page = 8*512=4k bytes.
    double cost;
    unsigned long blkno;
    int count;
    int is_read;

	

    ASSERT( total == 1 );          //everytime get on request???

    blkno = reqs[0]->blk;
    count = reqs[0]->count;
    is_read = reqs[0]->is_read;

    ASSERT( !is_read );
 //   ASSERT( count == 8 ); //To test the number of blocks to write is the multiple of 8   removed by menliang

    if( is_write_cacheable( blkno, count ) ){
        // We are free to write in cache
        ssd_cache_write( blkno, count, elem_num );
        cost = 0.0;
    }else{
 
        cost = ssd_cache_clean(blkno, count, elem_num, s);   //if every block in the aimed cache-set all occupied, write back one line, cost 0.3056ms
        ssd_cache_write(blkno, count, elem_num );

    }

    reqs[0]->acctime = cost;
    reqs[0]->schtime = cost;
    
}

void ssd_background_updating(int elem_num, ssd_t *s)

{
  float updating_wholetime = simtime - s->elements[elem_num].last_access_time;
  double cost = 0.0f;
 // ASSERT(updating_wholetime > 0);

   if(updating_wholetime < 0)
      fprintf(stderr, "Background cleaning is working !!, the updating time is %f \n",  updating_wholetime);

  if (updating_wholetime < 3)
    return;

 if (s->params.cleaning_in_background)
   cost = ssd_clean_element_no_copyback(elem_num, s);

   if(cost > 0.0f )
   fprintf(stderr, "Background cleaning is working !!, the updating time is %f, cost is %f \n",  updating_wholetime, cost);

}

void ssd_cache_updating(ssd_t *s)

{   
    double cost = 0;  

    unsigned long blkno;
    ssd_element *elem;
    double_linked_list *dll;
    double_linked_list_elem *set_test;
    int base_key = 1;
    float updating_wholetime = simtime - cache_entry_access_time_most_recent ; 
    rb_red_blk_node *rbn = RBExactQuery(ssd_cache_dirty_rbt, &base_key);
    int i, j;
    j = 0;


    if (updating_wholetime < 2.0)
       return;


    if( rbn == NULL || rbn == ssd_cache_dirty_rbt->nil )
       { 
	return;  //No written block in the cache
       }


    for (i=0; i<s->params.nelements; i ++)
    {
	s->elements[i].cache_updating_time = 0.0;
        ssd_background_updating(i,s);   
        //which should have happened before the cache updating process, here cache up the missing background cleaning procedure      
    }

    dll = rbn->info;   

do  //assume that during cache-updating, the ssd is always busy, no background updating precedure here.
   {
     if( double_linked_list_is_empty(dll))  // That set list is empty, try to find a non empty one
	  { 
            while( double_linked_list_is_empty( dll ) )
	       {
                   rbn = TreeSuccessor( ssd_cache_dirty_rbt, rbn);  
                           if( rbn == NULL || rbn == ssd_cache_dirty_rbt->nil )
		                { 
                              // fprintf(stderr, "Cache is empty! The recently updated time is %f, the updated cache page number is %d.\n", total_cost, j);
				   for (i=0; i<s->params.nelements; i ++)
        			    {
	 			      s->elements[i].last_access_time = cache_entry_access_time_most_recent + s->elements[i].cache_updating_time;
        			     }
				  return;
				}
		       dll = rbn->info;
		}
	   }   
 
        int entryno = double_linked_list_pop_head( dll );
	ASSERT(entryno != -1);
	ASSERT(ssd_cache_table[entryno].flags == SSD_CACHE_FLAG_DIRTY);

	blkno = (ssd_cache_table[entryno].upper_address * (CACHE_SIZE/CACHE_WAY) + entryno/CACHE_WAY) * 8;
	int elem_num = s->timing_t->choose_element(s->timing_t, blkno);
        ASSERT (elem_num == ssd_cache_table[entryno].elem_num);


        elem = &s->elements[elem_num];

       switch(s->params.write_policy) 
	{
            case DISKSIM_SSD_WRITE_POLICY_SIMPLE:
                cost = ssd_write_policy_simple(blkno, 8, elem_num, s);
                break;

            case DISKSIM_SSD_WRITE_POLICY_OSR:
                cost = ssd_write_one_active_page(blkno, 8, elem_num, s);
                break;

            default:
                fprintf(stderr, "Error: unknown write policy %d in ssd_compute_access_time\n",
                        s->params.write_policy);
                exit(1);
          }
        elem->cache_updating_time = elem->cache_updating_time + cost;
        ssd_cache_table[entryno].flags = SSD_CACHE_FLAG_CLEAN;
        cache_occupy_number--;
        ssd_cache_table[entryno].dll_dirty_elem = NULL;
	ssd_cache_table[entryno].use_counter = 0;
        j ++;

     // fprintf(stderr, "the Evicted blkno is %d, the corresponding elem_num is %d, cost is %f, the update time is %f \n", blkno, elem_num, cost, elem->cache_updating_time );
  }while (elem->cache_updating_time < (updating_wholetime -2));

     for (i=0; i<s->params.nelements; i ++)
        {
	  s->elements[i].last_access_time = simtime;
        }


    if (j > 500)             
    {       
     // fprintf(stderr, "The simulation time is %f. the time of interval is %f,the updated line No is %d.\n ", simtime, updating_wholetime,j);
     }

  }


void ssd_cache_updating_complete(ioreq_event *curr)

{    
    ssd_t *currdisk;
    int elem_num;
    int i;
    double total_cost = 0;

    currdisk = getssd (curr->devno);
    elem_num = curr->ssd_elem_num;

   if(cache_entry_access_time_most_recent == 0)  //The first time access
      {  

        for (i=0; i< currdisk->params.nelements; i ++)
        {
	  currdisk->elements[i].last_access_time = simtime;
        }
         return;
       }

   // ASSERT(currdisk->elements[elem_num].media_busy == FALSE);
    ssd_cache_updating(currdisk);

}


void increment_usage_count(int cache_index){
    int use_counter = ssd_cache_table[cache_index].use_counter;

    if( ssd_cache_table[cache_index].flags == SSD_CACHE_FLAG_DIRTY ){
        double_linked_list *dll = get_dll_from_rbt( use_counter );        // Get list from rbt
        double_linked_list_delete_elem( dll, ssd_cache_table[cache_index].dll_dirty_elem );

        dll = get_dll_from_rbt( use_counter + 1);       // Get the new queue
        ssd_cache_table[cache_index].dll_dirty_elem =
        double_linked_list_add_queue( dll, cache_index );  // Add it again at the end
	ssd_cache_table[cache_index].use_counter++;  //added by menliang
    }
}




// check whether the data to be read is in the cache
// blkno: the starting address of logical block, which is size of 512 bytes.
// count: the number of logical blocks to write. It is always 8 with this version.
int inside_cache_read(unsigned long blkno, int count){
    unsigned long apn = blkno/8;
    int upper_address = apn / (CACHE_SIZE / CACHE_WAY);
    int cache_offset  = apn % (CACHE_SIZE / CACHE_WAY);

    int i;
    for (i=0; i<CACHE_WAY; i++) {
        int cache_index = cache_offset*CACHE_WAY+i;

        if( (ssd_cache_table[cache_index].flags != SSD_CACHE_FLAG_INVALID) &&
                (ssd_cache_table[cache_index].upper_address == upper_address) ) {

            cache_entry_access_time_most_recent = simtime;
            ssd_current_idle_time = simtime;
            increment_usage_count(cache_index);
            cache_total_read_count++;
            cache_read_count++;
            ssd_cache_table[cache_index].last_access_time = cache_entry_access_time_most_recent;
            return 1;
        }
    }

    return 0;

}

void ssd_compute_access_time_one_active_page_read(ssd_req **reqs, int total, int elem_num, ssd_t *s){   //total runtime 1278388
    // we assume that requests have been broken down into page sized chunks
    double cost;
    unsigned long blkno;
    int count;
    int is_read;

    // we can serve only one request at a time
    ASSERT(total == 1);

    blkno = reqs[0]->blk;
    count = reqs[0]->count;
    is_read = reqs[0]->is_read;

   // ASSERT( count == 8 );

    if (is_read) {
        switch(s->params.write_policy) {
            case DISKSIM_SSD_WRITE_POLICY_SIMPLE:
            case DISKSIM_SSD_WRITE_POLICY_OSR:
                if (inside_cache_read(blkno, count)) {      //data are in the cache
                    cost = 0;                        
                } else {                                  //data are not in the cache
                  ssd_background_updating(elem_num, s);  //first replenish the ssd background procedure.
                  cost = ssd_read_policy_simple(count, s);
                  s->elements[elem_num].last_access_time = simtime + cost;  //update element state once it has been read or written.
                }
                break;

            default:
                fprintf(stderr, "Error: unknown write policy %d in ssd_compute_access_time\n",
                        s->params.write_policy);
                exit(1);
        }
    } else { //WARNING: not used anymore, see cache
        fprintf(stderr, "Error: READ conflict in ssd_cache.c \n");
                exit(1);
    }

    reqs[0]->acctime = cost;
    reqs[0]->schtime = cost;
}






