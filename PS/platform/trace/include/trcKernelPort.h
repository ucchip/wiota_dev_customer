/*******************************************************************************
 * Trace Recorder Library for Tracealyzer v3.3.0
 * Percepio AB, www.percepio.com
 *
 * Terms of Use
 * This file is part of the trace recorder library (RECORDER), which is the 
 * intellectual property of Percepio AB (PERCEPIO) and provided under a
 * license as follows.
 * The RECORDER may be used free of charge for the purpose of recording data
 * intended for analysis in PERCEPIO products. It may not be used or modified
 * for other purposes without explicit permission from PERCEPIO.
 * You may distribute the RECORDER in its original source code form, assuming
 * this text (terms of use, disclaimer, copyright notice) is unchanged. You are
 * allowed to distribute the RECORDER with minor modifications intended for
 * configuration or porting of the RECORDER, e.g., to allow using it on a 
 * specific processor, processor family or with a specific communication
 * interface. Any such modifications should be documented directly below
 * this comment block.  
 *
 * Disclaimer
 * The RECORDER is being delivered to you AS IS and PERCEPIO makes no warranty
 * as to its use or performance. PERCEPIO does not and cannot warrant the 
 * performance or results you may obtain by using the RECORDER or documentation.
 * PERCEPIO make no warranties, express or implied, as to noninfringement of
 * third party rights, merchantability, or fitness for any particular purpose.
 * In no event will PERCEPIO, its technology partners, or distributors be liable
 * to you for any consequential, incidental or special damages, including any
 * lost profits or lost savings, even if a representative of PERCEPIO has been
 * advised of the possibility of such damages, or for any claim by any third
 * party. Some jurisdictions do not allow the exclusion or limitation of
 * incidental, consequential or special damages, or the exclusion of implied
 * warranties or limitations on how long an implied warranty may last, so the
 * above limitations may not apply to you.
 *
 * FreeRTOS-specific definitions needed by the trace recorder
 *
 * <LICENSE INFO>
 *
 * Tabs are used for indent in this file (1 tab = 4 spaces)
 *
 * Copyright Percepio AB, 2017.
 * www.percepio.com
 ******************************************************************************/

#ifndef TRC_KERNEL_PORT_H
#define TRC_KERNEL_PORT_H


#include "trcPortDefines.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define TRC_USE_TRACEALYZER_RECORDER configUSE_TRACE_FACILITY

/*** FreeRTOS version codes **************************************************/
#define FREERTOS_VERSION_NOT_SET				0
#define TRC_FREERTOS_VERSION_7_3				1 /* v7.3 is earliest supported.*/
#define TRC_FREERTOS_VERSION_7_4				2
#define TRC_FREERTOS_VERSION_7_5_OR_7_6			3
#define TRC_FREERTOS_VERSION_8_X				4 /* Any v8.x.x*/
#define TRC_FREERTOS_VERSION_9_0_0				5 
#define TRC_FREERTOS_VERSION_9_0_1				6 
#define TRC_FREERTOS_VERSION_9_0_2				7
#define TRC_FREERTOS_VERSION_10_0_0				8 /* If using FreeRTOS v10.0.0 or later version */

#define TRC_FREERTOS_VERSION_9_X				42 /* Not allowed anymore */

#if (TRC_CFG_FREERTOS_VERSION == TRC_FREERTOS_VERSION_9_X)
/* This setting for TRC_CFG_FREERTOS_VERSION is no longer allowed as v9.0.1 needs special handling. */ 
#error "Please specify your exact FreeRTOS version in trcConfig.h, from the options listed above."
#endif

#if (TRC_CFG_FREERTOS_VERSION >= TRC_FREERTOS_VERSION_10_0_0)
#define prvGetStreamBufferType(x) ((( StreamBuffer_t * )x )->ucFlags & sbFLAGS_IS_MESSAGE_BUFFER)
#else
#define prvGetStreamBufferType(x) 0
#endif

/* Added mainly for our internal testing. This makes it easier to create test applications that 
   runs on multiple FreeRTOS versions. */
#if (TRC_CFG_FREERTOS_VERSION < TRC_FREERTOS_VERSION_8_X)
    /* FreeRTOS v7.0 and later */	
    #define STRING_CAST(x) ( (signed char*) x )
    #define TickType portTickType
#else
    /* FreeRTOS v8.0 and later */
    #define STRING_CAST(x) x
    #define TickType TickType_t
#endif

#if (1)

/*******************************************************************************
 * INCLUDE_xTaskGetCurrentTaskHandle must be set to 1 for tracing to work properly
 ******************************************************************************/
//#undef INCLUDE_xTaskGetCurrentTaskHandle
//#define INCLUDE_xTaskGetCurrentTaskHandle 1

#ifndef TRC_CFG_INCLUDE_EVENT_GROUP_EVENTS
#define TRC_CFG_INCLUDE_EVENT_GROUP_EVENTS 0
#endif

#ifndef TRC_CFG_INCLUDE_PEND_FUNC_CALL_EVENTS
#define TRC_CFG_INCLUDE_PEND_FUNC_CALL_EVENTS 0
#endif

#ifndef TRC_CFG_INCLUDE_TIMER_EVENTS
#define TRC_CFG_INCLUDE_TIMER_EVENTS 0
#endif

/*******************************************************************************
 * vTraceSetQueueName(void* object, const char* name)
 *
 * Parameter object: pointer to the Queue that shall be named
 * Parameter name: the name to set (const string literal)
 *
 * Sets a name for Queue objects for display in Tracealyzer.
 ******************************************************************************/
//void vTraceSetQueueName(void* object, const char* name);

/*******************************************************************************
 * vTraceSetSemaphoreName(void* object, const char* name)
 *
 * Parameter object: pointer to the Semaphore that shall be named
 * Parameter name: the name to set (const string literal)
 *
 * Sets a name for Semaphore objects for display in Tracealyzer.
 ******************************************************************************/
//void vTraceSetSemaphoreName(void* object, const char* name);

/*******************************************************************************
 * vTraceSetMutexName(void* object, const char* name)
 *
 * Parameter object: pointer to the Mutex that shall be named
 * Parameter name: the name to set (const string literal)
 *
 * Sets a name for Semaphore objects for display in Tracealyzer.
 ******************************************************************************/
//void vTraceSetMutexName(void* object, const char* name);

/*******************************************************************************
* vTraceSetEventGroupName(void* object, const char* name)
*
* Parameter object: pointer to the EventGroup that shall be named
* Parameter name: the name to set (const string literal)
*
* Sets a name for EventGroup objects for display in Tracealyzer.
******************************************************************************/
//void vTraceSetEventGroupName(void* object, const char* name);

/*******************************************************************************
* vTraceSetStreamBufferName(void* object, const char* name)
*
* Parameter object: pointer to the StreamBuffer that shall be named
* Parameter name: the name to set (const string literal)
*
* Sets a name for StreamBuffer objects for display in Tracealyzer.
******************************************************************************/
//void vTraceSetStreamBufferName(void* object, const char* name);

/*******************************************************************************
 * vTraceSetMessageBufferName(void* object, const char* name)
 *
 * Parameter object: pointer to the MessageBuffer that shall be named
 * Parameter name: the name to set (const string literal)
 *
 * Sets a name for MessageBuffer objects for display in Tracealyzer.
 ******************************************************************************/
//void vTraceSetMessageBufferName(void* object, const char* name);

/*******************************************************************************
 * Note: Setting names for event groups is difficult to support, this has been 
 * excluded intentionally. This since we don't know if event_groups.c is 
 * included in the build, so referencing it from the recorder may cause errors.
 ******************************************************************************/

/* Gives the currently executing task (wrapper for RTOS-specific function) */
//void* prvTraceGetCurrentTaskHandle(void);

#if (((TRC_CFG_RECORDER_MODE == TRC_RECORDER_MODE_SNAPSHOT) && (TRC_CFG_INCLUDE_ISR_TRACING == 1)) || (TRC_CFG_RECORDER_MODE == TRC_RECORDER_MODE_STREAMING))
/* Tells if the scheduler currently is suspended (task-switches can't occur) */
//unsigned char prvTraceIsSchedulerSuspended(void);

/*******************************************************************************
 * INCLUDE_xTaskGetSchedulerState must be set to 1 for tracing to work properly
 ******************************************************************************/
#undef INCLUDE_xTaskGetSchedulerState
#define INCLUDE_xTaskGetSchedulerState 1

#endif /* (((TRC_CFG_RECORDER_MODE == TRC_RECORDER_MODE_SNAPSHOT) && (TRC_CFG_INCLUDE_ISR_TRACING == 1)) || (TRC_CFG_RECORDER_MODE == TRC_RECORDER_MODE_STREAMING)) */

#define TRACE_KERNEL_VERSION 0x1AA1
//#define TRACE_TICK_RATE_HZ configTICK_RATE_HZ /* Defined in "FreeRTOS.h" */
#define TRACE_CPU_CLOCK_HZ configCPU_CLOCK_HZ /* Defined in "FreeRTOSConfig.h" */
//#define TRACE_GET_CURRENT_TASK() prvTraceGetCurrentTaskHandle()

#define TRACE_GET_OS_TICKS() (uiTraceTickCount) /* Streaming only */

/* If using dynamic allocation of snapshot trace buffer... */
#define TRACE_MALLOC(size) uc_malloc(size) 	

#ifdef configUSE_TIMERS
#if (configUSE_TIMERS == 1)
#undef INCLUDE_xTimerGetTimerDaemonTaskHandle
#define INCLUDE_xTimerGetTimerDaemonTaskHandle 1
#endif /* configUSE_TIMERS == 1*/
#endif /* configUSE_TIMERS */






/******************************************************************************/
/*** Definitions for Snapshot mode ********************************************/
/******************************************************************************/
#if (TRC_CFG_RECORDER_MODE == TRC_RECORDER_MODE_SNAPSHOT)

/*** The object classes *******************************************************/

//#define TRACE_NCLASSES 9
//#define TRACE_CLASS_QUEUE ((traceObjectClass)0)
//#define TRACE_CLASS_SEMAPHORE ((traceObjectClass)1)
//#define TRACE_CLASS_MUTEX ((traceObjectClass)2)
//#define TRACE_CLASS_TASK ((traceObjectClass)3)
//#define TRACE_CLASS_ISR ((traceObjectClass)4)
//#define TRACE_CLASS_TIMER ((traceObjectClass)5)
//#define TRACE_CLASS_EVENTGROUP ((traceObjectClass)6)
//#define TRACE_CLASS_STREAMBUFFER ((traceObjectClass)7)
//#define TRACE_CLASS_MESSAGEBUFFER ((traceObjectClass)8)


/* Flag to tell the context of tracePEND_FUNC_CALL_FROM_ISR */
//extern int uiInEventGroupSetBitsFromISR;

/* Initialization of the object property table */
//void vTraceInitObjectPropertyTable(void);

/* Initialization of the handle mechanism, see e.g, prvTraceGetObjectHandle */
//void vTraceInitObjectHandleStack(void);

/* Returns the "Not enough handles" error message for the specified object class */
//const char* pszTraceGetErrorNotEnoughHandles(traceObjectClass objectclass);

//void* prvTraceGetCurrentTaskHandle(void);


/* peek failed on mutex:		0xE1	*/


/* LAST EVENT (0xE7) */

/****************************
* MACROS TO GET TRACE CLASS *
****************************/
/* Generic versions */


#endif /*#if TRC_CFG_RECORDER_MODE == TRC_RECORDER_MODE_SNAPSHOT */

/******************************************************************************/
/*** Definitions for Streaming mode *******************************************/
/******************************************************************************/
#if 1 //(TRC_CFG_RECORDER_MODE == TRC_RECORDER_MODE_STREAMING)

/*******************************************************************************
* vTraceStoreKernelObjectName
*
* Set the name for a kernel object (defined by its address).
******************************************************************************/			
//void vTraceStoreKernelObjectName(void* object, const char* name); 

/*******************************************************************************
* prvIsNewTCB
*
* Tells if this task is already executing, or if there has been a task-switch.
* Assumed to be called within a trace hook in kernel context.
*******************************************************************************/
//unsigned int  prvIsNewTCB(void* pNewTCB);

//#define TRACE_GET_CURRENT_TASK() prvTraceGetCurrentTaskHandle()

/*************************************************************************/
/* KERNEL SPECIFIC OBJECT CONFIGURATION									 */
/*************************************************************************/

/*******************************************************************************
 * The event codes - should match the offline config file.
 ******************************************************************************/

/*** Event codes for streaming - should match the Tracealyzer config file *****/
#define PSF_EVENT_NULL_EVENT								0x00

#define PSF_EVENT_TRACE_START								0x01
#define PSF_EVENT_TS_CONFIG									0x02
#define PSF_EVENT_OBJ_NAME									0x03
//#define PSF_EVENT_TASK_PRIORITY								0x04
//#define PSF_EVENT_TASK_PRIO_INHERIT							0x05
//#define PSF_EVENT_TASK_PRIO_DISINHERIT						0x06
#define PSF_EVENT_DEFINE_ISR								0x07

//#define PSF_EVENT_TASK_CREATE								0x10
//#define PSF_EVENT_QUEUE_CREATE								0x11
//#define PSF_EVENT_SEMAPHORE_BINARY_CREATE					0x12
//#define PSF_EVENT_MUTEX_CREATE								0x13
//#define PSF_EVENT_TIMER_CREATE								0x14
//#define PSF_EVENT_EVENTGROUP_CREATE							0x15
//#define PSF_EVENT_SEMAPHORE_COUNTING_CREATE					0x16
//#define PSF_EVENT_MUTEX_RECURSIVE_CREATE					0x17
//#define PSF_EVENT_STREAMBUFFER_CREATE						0x18
//#define PSF_EVENT_MESSAGEBUFFER_CREATE						0x19
//
//#define PSF_EVENT_TASK_DELETE								0x20
//#define PSF_EVENT_QUEUE_DELETE								0x21
//#define PSF_EVENT_SEMAPHORE_DELETE							0x22
//#define PSF_EVENT_MUTEX_DELETE								0x23
//#define PSF_EVENT_TIMER_DELETE								0x24
//#define PSF_EVENT_EVENTGROUP_DELETE							0x25
//#define PSF_EVENT_STREAMBUFFER_DELETE						0x28
//#define PSF_EVENT_MESSAGEBUFFER_DELETE						0x29

//#define PSF_EVENT_TASK_READY								0x30
//#define PSF_EVENT_NEW_TIME									0x31
//#define PSF_EVENT_NEW_TIME_SCHEDULER_SUSPENDED				0x32
#define PSF_EVENT_ISR_BEGIN									0x33
#define PSF_EVENT_ISR_RESUME								0x34
//#define PSF_EVENT_TS_BEGIN									0x35
//#define PSF_EVENT_TS_RESUME									0x36
//#define PSF_EVENT_TASK_ACTIVATE								0x37
//
//#define PSF_EVENT_MALLOC									0x38
//#define PSF_EVENT_FREE										0x39

#define PSF_EVENT_LOWPOWER_BEGIN							0x3A
#define PSF_EVENT_LOWPOWER_END								0x3B

#define PSF_EVENT_IFE_NEXT									0x3C
#define PSF_EVENT_IFE_DIRECT								0x3D

//#define PSF_EVENT_TASK_CREATE_FAILED						0x40
//#define PSF_EVENT_QUEUE_CREATE_FAILED						0x41
//#define PSF_EVENT_SEMAPHORE_BINARY_CREATE_FAILED			0x42
//#define PSF_EVENT_MUTEX_CREATE_FAILED						0x43
//#define PSF_EVENT_TIMER_CREATE_FAILED						0x44
//#define PSF_EVENT_EVENTGROUP_CREATE_FAILED					0x45
//#define PSF_EVENT_SEMAPHORE_COUNTING_CREATE_FAILED			0x46
//#define PSF_EVENT_MUTEX_RECURSIVE_CREATE_FAILED				0x47
//#define PSF_EVENT_STREAMBUFFER_CREATE_FAILED				0x49
//#define PSF_EVENT_MESSAGEBUFFER_CREATE_FAILED				0x4A

//#define PSF_EVENT_TIMER_DELETE_FAILED						0x48
//
//#define PSF_EVENT_QUEUE_SEND								0x50
//#define PSF_EVENT_SEMAPHORE_GIVE							0x51
//#define PSF_EVENT_MUTEX_GIVE								0x52
//
//#define PSF_EVENT_QUEUE_SEND_FAILED							0x53
//#define PSF_EVENT_SEMAPHORE_GIVE_FAILED						0x54
//#define PSF_EVENT_MUTEX_GIVE_FAILED							0x55
//
//#define PSF_EVENT_QUEUE_SEND_BLOCK							0x56
//#define PSF_EVENT_SEMAPHORE_GIVE_BLOCK						0x57
//#define PSF_EVENT_MUTEX_GIVE_BLOCK							0x58
//
//#define PSF_EVENT_QUEUE_SEND_FROMISR						0x59
//#define PSF_EVENT_SEMAPHORE_GIVE_FROMISR					0x5A
//
//#define PSF_EVENT_QUEUE_SEND_FROMISR_FAILED					0x5C
//#define PSF_EVENT_SEMAPHORE_GIVE_FROMISR_FAILED				0x5D
//
//#define PSF_EVENT_QUEUE_RECEIVE								0x60
//#define PSF_EVENT_SEMAPHORE_TAKE							0x61
//#define PSF_EVENT_MUTEX_TAKE								0x62
//
//#define PSF_EVENT_QUEUE_RECEIVE_FAILED						0x63
//#define PSF_EVENT_SEMAPHORE_TAKE_FAILED						0x64
//#define PSF_EVENT_MUTEX_TAKE_FAILED							0x65
//
//#define PSF_EVENT_QUEUE_RECEIVE_BLOCK						0x66
//#define PSF_EVENT_SEMAPHORE_TAKE_BLOCK						0x67
//#define PSF_EVENT_MUTEX_TAKE_BLOCK							0x68
//
//#define PSF_EVENT_QUEUE_RECEIVE_FROMISR						0x69
//#define PSF_EVENT_SEMAPHORE_TAKE_FROMISR					0x6A
//
//#define PSF_EVENT_QUEUE_RECEIVE_FROMISR_FAILED				0x6C
//#define PSF_EVENT_SEMAPHORE_TAKE_FROMISR_FAILED				0x6D
//
//#define PSF_EVENT_QUEUE_PEEK								0x70
//#define PSF_EVENT_SEMAPHORE_PEEK							0x71
//#define PSF_EVENT_MUTEX_PEEK								0x72
//
//#define PSF_EVENT_QUEUE_PEEK_FAILED							0x73
//#define PSF_EVENT_SEMAPHORE_PEEK_FAILED						0x74	
//#define PSF_EVENT_MUTEX_PEEK_FAILED							0x75
//
//#define PSF_EVENT_QUEUE_PEEK_BLOCK							0x76
//#define PSF_EVENT_SEMAPHORE_PEEK_BLOCK						0x77
//#define PSF_EVENT_MUTEX_PEEK_BLOCK							0x78

//#define PSF_EVENT_TASK_DELAY_UNTIL							0x79
//#define PSF_EVENT_TASK_DELAY								0x7A
//#define PSF_EVENT_TASK_SUSPEND								0x7B
//#define PSF_EVENT_TASK_RESUME								0x7C
//#define PSF_EVENT_TASK_RESUME_FROMISR						0x7D
//
//#define PSF_EVENT_TIMER_PENDFUNCCALL						0x80
//#define PSF_EVENT_TIMER_PENDFUNCCALL_FROMISR				0x81
//#define PSF_EVENT_TIMER_PENDFUNCCALL_FAILED					0x82
//#define PSF_EVENT_TIMER_PENDFUNCCALL_FROMISR_FAILED			0x83

#define PSF_EVENT_USER_EVENT								0x90

//#define PSF_EVENT_TIMER_START								0xA0
//#define PSF_EVENT_TIMER_RESET								0xA1
//#define PSF_EVENT_TIMER_STOP								0xA2
//#define PSF_EVENT_TIMER_CHANGEPERIOD						0xA3
//#define PSF_EVENT_TIMER_START_FROMISR						0xA4
//#define PSF_EVENT_TIMER_RESET_FROMISR						0xA5
//#define PSF_EVENT_TIMER_STOP_FROMISR						0xA6
//#define PSF_EVENT_TIMER_CHANGEPERIOD_FROMISR				0xA7
//#define PSF_EVENT_TIMER_START_FAILED						0xA8
//#define PSF_EVENT_TIMER_RESET_FAILED						0xA9
//#define PSF_EVENT_TIMER_STOP_FAILED							0xAA
//#define PSF_EVENT_TIMER_CHANGEPERIOD_FAILED					0xAB
//#define PSF_EVENT_TIMER_START_FROMISR_FAILED				0xAC
//#define PSF_EVENT_TIMER_RESET_FROMISR_FAILED				0xAD
//#define PSF_EVENT_TIMER_STOP_FROMISR_FAILED					0xAE
//#define PSF_EVENT_TIMER_CHANGEPERIOD_FROMISR_FAILED			0xAF

//#define PSF_EVENT_EVENTGROUP_SYNC							0xB0
//#define PSF_EVENT_EVENTGROUP_WAITBITS						0xB1
//#define PSF_EVENT_EVENTGROUP_CLEARBITS						0xB2
//#define PSF_EVENT_EVENTGROUP_CLEARBITS_FROMISR				0xB3
//#define PSF_EVENT_EVENTGROUP_SETBITS						0xB4
//#define PSF_EVENT_EVENTGROUP_SETBITS_FROMISR				0xB5
//#define PSF_EVENT_EVENTGROUP_SYNC_BLOCK						0xB6
//#define PSF_EVENT_EVENTGROUP_WAITBITS_BLOCK					0xB7
//#define PSF_EVENT_EVENTGROUP_SYNC_FAILED					0xB8
//#define PSF_EVENT_EVENTGROUP_WAITBITS_FAILED				0xB9

//#define PSF_EVENT_QUEUE_SEND_FRONT							0xC0
//#define PSF_EVENT_QUEUE_SEND_FRONT_FAILED					0xC1
//#define PSF_EVENT_QUEUE_SEND_FRONT_BLOCK					0xC2
//#define PSF_EVENT_QUEUE_SEND_FRONT_FROMISR					0xC3
//#define PSF_EVENT_QUEUE_SEND_FRONT_FROMISR_FAILED			0xC4
//#define PSF_EVENT_MUTEX_GIVE_RECURSIVE						0xC5
//#define PSF_EVENT_MUTEX_GIVE_RECURSIVE_FAILED				0xC6
//#define PSF_EVENT_MUTEX_TAKE_RECURSIVE						0xC7
//#define PSF_EVENT_MUTEX_TAKE_RECURSIVE_FAILED				0xC8

//#define PSF_EVENT_TASK_NOTIFY								0xC9
//#define PSF_EVENT_TASK_NOTIFY_TAKE							0xCA
//#define PSF_EVENT_TASK_NOTIFY_TAKE_BLOCK					0xCB
//#define PSF_EVENT_TASK_NOTIFY_TAKE_FAILED					0xCC
//#define PSF_EVENT_TASK_NOTIFY_WAIT							0xCD
//#define PSF_EVENT_TASK_NOTIFY_WAIT_BLOCK					0xCE
//#define PSF_EVENT_TASK_NOTIFY_WAIT_FAILED					0xCF
//#define PSF_EVENT_TASK_NOTIFY_FROM_ISR						0xD0
//#define PSF_EVENT_TASK_NOTIFY_GIVE_FROM_ISR					0xD1

//#define PSF_EVENT_TIMER_EXPIRED								0xD2
//
//#define PSF_EVENT_STREAMBUFFER_SEND							0xD3
//#define PSF_EVENT_STREAMBUFFER_SEND_BLOCK					0xD4
//#define PSF_EVENT_STREAMBUFFER_SEND_FAILED					0xD5
//#define PSF_EVENT_STREAMBUFFER_RECEIVE						0xD6
//#define PSF_EVENT_STREAMBUFFER_RECEIVE_BLOCK				0xD7
//#define PSF_EVENT_STREAMBUFFER_RECEIVE_FAILED				0xD8
//#define PSF_EVENT_STREAMBUFFER_SEND_FROM_ISR				0xD9
//#define PSF_EVENT_STREAMBUFFER_SEND_FROM_ISR_FAILED			0xDA
//#define PSF_EVENT_STREAMBUFFER_RECEIVE_FROM_ISR				0xDB
//#define PSF_EVENT_STREAMBUFFER_RECEIVE_FROM_ISR_FAILED		0xDC
//#define PSF_EVENT_STREAMBUFFER_RESET						0xDD

//#define PSF_EVENT_MESSAGEBUFFER_SEND						0xDE
//#define PSF_EVENT_MESSAGEBUFFER_SEND_BLOCK					0xDF
//#define PSF_EVENT_MESSAGEBUFFER_SEND_FAILED					0xE0
//#define PSF_EVENT_MESSAGEBUFFER_RECEIVE						0xE1
//#define PSF_EVENT_MESSAGEBUFFER_RECEIVE_BLOCK				0xE2
//#define PSF_EVENT_MESSAGEBUFFER_RECEIVE_FAILED				0xE3
//#define PSF_EVENT_MESSAGEBUFFER_SEND_FROM_ISR				0xE4
//#define PSF_EVENT_MESSAGEBUFFER_SEND_FROM_ISR_FAILED		0xE5
//#define PSF_EVENT_MESSAGEBUFFER_RECEIVE_FROM_ISR			0xE6
//#define PSF_EVENT_MESSAGEBUFFER_RECEIVE_FROM_ISR_FAILED		0xE7
//#define PSF_EVENT_MESSAGEBUFFER_RESET						0xE8
#define PSF_EVENT_USER_EVENT_2								0xF0

//#define PSF_EVENT_USER_TRACE_FUNCTION                       0x0100
//#define PSF_EVENT_USER_TRACE_ERROR                          0x0101

#define	PSF_EVENT_STATE_EVENT                               0xFFF0 //PHY STATE
#define	PSF_EVENT_MSG_EVENT                                 0xFFF1 //protocal msg output
#define	PSF_EVENT_STR_EVENT                                 0xFFF2


#endif /* (TRC_CFG_RECORDER_MODE == TRC_RECORDER_MODE_STREAMING) */

#else 
    
/* When recorder is disabled */
//#define vTraceSetQueueName(object, name)
//#define vTraceSetSemaphoreName(object, name)
//#define vTraceSetMutexName(object, name)
//#define vTraceSetEventGroupName(object, name)
//#define vTraceSetStreamBufferName(object, name)
//#define vTraceSetMessageBufferName(object, name)
    
#endif 

void vTraceEnable(int startOption);

void trace_control(void);

#ifdef __cplusplus
}
#endif

#endif /* TRC_KERNEL_PORT_H */
