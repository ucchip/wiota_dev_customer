
#ifndef     _ADP_SEM_H_
#define     _ADP_SEM_H_

#if defined(_FREERTOS_)

#elif defined( _RT_THREAD_)

#define SYS_SEM_WAITING_FOREVER -1
/**
 * IPC flags and control command definitions
 */
#define ADP_SME_IPC_FLAG_FIFO                0x00            /**< FIFOed IPC. @ref IPC. */
#define ADP_SME_IPC_FLAG_PRIO                0x01            /**< PRIOed IPC. @ref IPC. */

#define ADP_SEM_WAITING_FOREVER      RT_WAITING_FOREVER


/* RT-Thread error code definitions */
#define ADP_EOK                          0               /**< There is no error */
#define ADP_ERROR                        -1               /**< A generic error happens */
#define ADP_ETIMEOUT                     -2               /**< Timed out */
#define ADP_EFULL                        -3               /**< The resource is full */
#define ADP_EEMPTY                       -4               /**< The resource is empty */
#define ADP_ENOMEM                       -5               /**< No memory */
#define ADP_ENOSYS                       -6               /**< No system */
#define ADP_EBUSY                        -7               /**< Busy */
#define ADP_EIO                          -8               /**< IO error */
#define ADP_EINTR                        -9               /**< Interrupted system call */
#define ADP_EINVAL                       -10              /**< Invalid argument */

#else
    
#endif

void * uc_create_sem(char *name, unsigned int value, unsigned char flag);
int uc_sem_del(void *sem);
int uc_wait_sem(void *sem, signed   int  timeout);
int uc_signed_sem(void *sem);

void *uc_create_lock(char *name);
int uc_lock(void *sem, signed   int  timeout);
int uc_unlock(void *sem);

#endif


