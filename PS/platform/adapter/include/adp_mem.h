#ifndef     _ADP_MEM_H_
#define     _ADP_MEM_H_

#define MEM_CHECK_
#ifdef MEM_CHECK_

#define MALLOC_CHECK_TASK_NUM 5
#define TEST_ADDRESS_MAX   100

struct malloc_test
{
    unsigned int malloc_count ;
    unsigned int malloc_flag ;
    unsigned int test_address[TEST_ADDRESS_MAX] ;
    rt_thread_t test_handle ;
    char name[4];
};

void *set_check_handle(char* name);
void add_mem_list(unsigned int address);
void del_mem_list(unsigned int address);
void show_mem_info(void);

#endif
    
void *uc_malloc(unsigned int size);
void uc_free(void* pv);
int  uc_heap_size(void);


#endif
