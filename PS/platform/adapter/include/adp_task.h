#ifndef     _ADP_TASK_H_
#define     _ADP_TASK_H_

typedef void (*TaskFunction_t)( void * );

int uc_thread_create(void ** thread, char *name, void (*entry)(void *parameter), void  *parameter, unsigned int  stack_size, unsigned char   priority, unsigned int  tick);
int uc_thread_start(void * thread);
int uc_thread_del(void * thread);
int uc_thread_delay(signed int time);


#endif
