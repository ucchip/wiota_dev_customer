#ifndef     _ADP_QUEUE_H_
#define     _ADP_QUEUE_H_
#include   <stdint.h>

#ifdef _RT_THREAD_
#define UC_SYSTEM_IPC_FLAG_FIFO                0x00            /**< FIFOed IPC. @ref IPC. */
#define UC_SYSTEM_IPC_FLAG_PRIO                0x01            /**< PRIOed IPC. @ref IPC. */

#define UC_QUEUE_WAITING_FOREVER              -1              /**< Block forever until get resource. */
#define UC_QUEUE_WAITING_NO                   0               /**< Non-block. */

#define QUEUE_EOK                          0              
#define QUEUE_ERROR                        1            
#define QUEUE_ETIMEOUT                     2           
#define QUEUE_EFULL                        3            
#define QUEUE_EEMPTY                       4             
#define QUEUE_ENOMEM                       5          
#define QUEUE_ENOSYS                       6           
#define QUEUE_EBUSY                        7             
#define QUEUE_EIO                          8              
#define QUEUE_EINTR                        9              
#define QUEUE_EINVAL                       10          
#endif

void * uc_create_queue(const char *name,  unsigned int   msg_size, unsigned int    max_msgs, unsigned char  flag);
int uc_recv_queue(void *queue, void **buf, unsigned int size, signed int timeout);
int uc_send_queue(void *queue, void *buf, unsigned int size, signed int timeout);
int uc_dele_queue(void *queue);

#endif
