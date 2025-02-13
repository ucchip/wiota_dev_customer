#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* Automatically generated file; DO NOT EDIT. */
/* RT-Thread Configuration */

/* RT-Thread Kernel */

#define RT_NAME_MAX 8
#define RT_ALIGN_SIZE 4
#define RT_THREAD_PRIORITY_32
#define RT_THREAD_PRIORITY_MAX 32
#define RT_TICK_PER_SECOND 1000
#define RT_USING_OVERFLOW_CHECK
#define RT_USING_HOOK
#define RT_HOOK_USING_FUNC_PTR
#define RT_USING_IDLE_HOOK
#define RT_IDLE_HOOK_LIST_SIZE 4
#define IDLE_THREAD_STACK_SIZE 512
#define RT_USING_TIMER_SOFT
#define RT_TIMER_THREAD_PRIO 4
#define RT_TIMER_THREAD_STACK_SIZE 512

/* kservice optimization */

#define RT_DEBUG

/* Inter-Thread communication */

#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_EVENT
#define RT_USING_MESSAGEQUEUE

/* Memory Management */

#define RT_USING_SMALL_MEM
#define RT_USING_SMALL_MEM_AS_HEAP
#define RT_USING_HEAP

/* Kernel Device Object */

#define RT_USING_DEVICE
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE 256
#define RT_CONSOLE_DEVICE_NAME "uart1"
#define RT_VER_NUM 0x40101

/* RT-Thread Components */

#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE 1224
#define RT_MAIN_THREAD_PRIORITY 6

/* Device Drivers */

#define RT_USING_DEVICE_IPC
#define RT_USING_SERIAL
#define RT_USING_SERIAL_V1
#define RT_SERIAL_RB_BUFSZ 256
#define RT_USING_PIN
#define RT_USING_ADC
#define RT_USING_DAC
#define RT_USING_RTC
#define RT_USING_ALARM
#define RT_USING_WDT

/* Using USB */


/* C/C++ and POSIX layer */

#define RT_LIBC_DEFAULT_TIMEZONE 8

/* POSIX (Portable Operating System Interface) layer */


/* Interprocess Communication (IPC) */


/* Socket is in the 'Network' category */


/* Network */

#define RT_USING_AT
#define AT_USING_SERVER
#define AT_SERVER_DEVICE "uart0"
#define AT_SERVER_RECV_BUFF_LEN 256
#define AT_CMD_END_MARK_CRLF
#define AT_CMD_MAX_LEN 128
#define AT_SW_VERSION_NUM 0x10301

/* Utilities */


/* wiota PS */

#define UC8288_MODULE
#define _QUICK_CONNECT_
#define _FPGA_
#define _RT_THREAD_
#define _ALLOW_TRACE_PRITF_TEST_
#define UC8288_FACTORY
#define _L1_FACTORY_FUNC_
#define _CLK_GATING_
#define _LPM_PAGING_
#define WIOTA_NEED_LICENSE
#define AT_WIOTA_GATEWAY_API
#define AT_WIOTA_GATEWAY

/* Hardware Drivers Config */

#define SOC_UC8288

/* Onboard Peripheral Drivers */

#define BSP_USING_SERIAL

/* On-chip Peripheral Drivers */

#define BSP_USING_GPIO
#define BSP_USING_UART
#define BSP_USING_UART0
#define BSP_USING_UART1
#define BSP_USING_ADC
#define BSP_USING_DAC
#define BSP_USING_RTC
#define BSP_USING_WDT

/* Application Example Config */


#endif
