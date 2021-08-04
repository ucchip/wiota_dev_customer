#ifndef     _ADP_TIME_H_
#define     _ADP_TIME_H_

#if defined(  _FREERTOS_)


#elif defined(_RT_THREAD_)
/**
 * clock & timer macros
 */
#define SYS_TIMER_FLAG_DEACTIVATED       0x0             /**< timer is deactive */
#define SYS_TIMER_FLAG_ACTIVATED         0x1             /**< timer is active */
#define SYS_TIMER_FLAG_ONE_SHOT          0x0             /**< one shot timer */
#define SYS_TIMER_FLAG_PERIODIC          0x2             /**< periodic timer */

#define SYS_TIMER_FLAG_HARD_TIMER        0x0             /**< hard timer,the timer's callback function will be called in tick isr. */
#define SYS_TIMER_FLAG_SOFT_TIMER        0x4             /**< soft timer,the timer's callback function will be called in timer thread. */

#define SYS_TIMER_CTRL_SET_TIME          0x0             /**< set timer control command */
#define SYS_TIMER_CTRL_GET_TIME          0x1             /**< get timer control command */
#define SYS_TIMER_CTRL_SET_ONESHOT       0x2             /**< change timer to one shot */
#define SYS_TIMER_CTRL_SET_PERIODIC      0x3             /**< change timer to periodic */
#define SYS_TIMER_CTRL_GET_STATE         0x4             /**< get timer run state active or deactive*/
#else

#endif




void *uc_timer_create( const char *name, void (*timeout_function)(void *parameter), void *parameter, unsigned int time, unsigned char  flag);
int uc_timer_del(void * timer);
int uc_timer_start(void *timer, unsigned int tiemout /*only freertos*/);
int uc_timer_stop(void *timer);
int uc_timer_control(void *timer, int cmd, void* arg);
//end declearation.


#endif
