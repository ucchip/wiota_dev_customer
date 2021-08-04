#ifndef TRACE_INTERFACE_H_
#define TRACE_INTERFACE_H_
/*==== TYPES ======================================================*/
#include "trace_function.h"
#include "uctypes.h"
#include "trcRecorder.h"
#include "uc_submodule_def.h"

s16_t vsi_o_tstring( u8_t nAgrs,u8_t subModuleID, n8_t *format, ... );

extern unsigned int submodule_active_flag[];
#define IF_TRACE   if(submodule_active_flag[SUB_MODULE_ID >> 5] & (1 << (SUB_MODULE_ID & 0x1f)))
#define IF_TRACE_SUB(submode_id) if(submodule_active_flag[submode_id >> 5] & (1 << (submode_id & 0x1f)))

#define TRACE_ALLOW_PRINTF
#ifdef TRACE_ALLOW_PRINTF
#include <rtthread.h>
#define TRACE_PRINTF rt_kprintf
#else
#define TRACE_PRINTF(...)
#endif
/*
 * Macro for state output
 * */
#define   STATE_EVENT0(submod_id, log_id) \
    { IF_TRACE vsi_o_tstate( submod_id,log_id,0);}
#define	  STATE_EVENT1(submod_id, log_id,a1) \
    { IF_TRACE vsi_o_tstate( submod_id,log_id,1,a1);}//Do we need TRACE_FUNCTION(__FUNCTION__)??
#define	  STATE_EVENT2(submod_id, log_id,a1,a2) \
    IF_TRACE vsi_o_tstate( submod_id,log_id,2,a1,a2);
#define	  STATE_EVENT3(submod_id, log_id,a1,a2,a3) \
    IF_TRACE vsi_o_tstate( submod_id,log_id,3,a1,a2,a3);
#define	  STATE_EVENT4(submod_id, log_id,a1,a2,a3,a4)  \
    IF_TRACE vsi_o_tstate( submod_id,log_id,4,a1,a2,a3,a4);
#define	  STATE_EVENT5(submod_id, log_id,a1,a2,a3,a4,a5) \
    IF_TRACE vsi_o_tstate( submod_id,log_id,5,a1,a2,a3,a4,a5);
#define	  STATE_EVENT6(submod_id, log_id,a1,a2,a3,a4,a5,a6) \
    IF_TRACE vsi_o_tstate( submod_id,log_id,6,a1,a2,a3,a4,a5,a6);
#define	  STATE_EVENT7(submod_id, log_id,a1,a2,a3,a4,a5,a6,a7) \
    IF_TRACE vsi_o_tstate( submod_id,log_id,7,a1,a2,a3,a4,a5,a6,a7);
#define	  STATE_EVENT8(submod_id, log_id,a1,a2,a3,a4,a5,a6,a7,a8) \
    IF_TRACE vsi_o_tstate( submod_id,log_id,8,a1,a2,a3,a4,a5,a6,a7,a8);
#define	  STATE_EVENT9(submod_id, log_id,a1,a2,a3,a4,a5,a6,a7,a8,a9) \
    IF_TRACE vsi_o_tstate( submod_id,log_id,9,a1,a2,a3,a4,a5,a6,a7,a8,a9);
#define	  STATE_EVENT10(submod_id,log_id,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) \
    IF_TRACE vsi_o_tstate( submod_id,log_id,10,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10);
#define	  STATE_EVENT11(submod_id,log_id,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) \
    IF_TRACE vsi_o_tstate( submod_id,log_id,11,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11);
#define	  STATE_EVENT12(submod_id,log_id,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) \
    IF_TRACE vsi_o_tstate( submod_id,log_id,12,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12);



#define GET_FUNCTION_NAME const char *tmp = __FUNCTION__;int len = strlen(tmp) - 1;
#define FUNCTION(data, len, pc) (((pc*4) < len)? *((int *)(data + pc * 4)) : 0 )
#define SHOW_FUNCTION  FUNCTION(tmp,len,0),FUNCTION(tmp,len,1),FUNCTION(tmp,len,2),FUNCTION(tmp,len,3)   
#define	  TRACE_TEST1(a1) \
    IF_TRACE_SUB(0xff) {GET_FUNCTION_NAME vsi_o_tstate( 0xff,1,6,SHOW_FUNCTION, __LINE__, a1);}   
#define	  TRACE_TEST2(a1,a2) \
    IF_TRACE_SUB(0xff) {GET_FUNCTION_NAME vsi_o_tstate( 0xff,2,7,SHOW_FUNCTION, __LINE__, a1,a2);}    
#define	  TRACE_TEST3(a1,a2,a3) \
    IF_TRACE_SUB(0xff)  {GET_FUNCTION_NAME vsi_o_tstate( 0xff,3,8,SHOW_FUNCTION,__LINE__, a1,a2,a3); }   
#define	  TRACE_TEST4(a1,a2,a3,a4)  \
    IF_TRACE_SUB(0xff)  {GET_FUNCTION_NAME vsi_o_tstate( 0xff,4,9,SHOW_FUNCTION,__LINE__, a1,a2,a3,a4); }   
#define	  TRACE_TEST5(a1,a2,a3,a4,a5) \
    IF_TRACE_SUB(0xff)  {GET_FUNCTION_NAME vsi_o_tstate( 0xff,5,10,SHOW_FUNCTION,__LINE__, a1,a2,a3,a4,a5); }   
#define	  TRACE_TEST6(a1,a2,a3,a4,a5,a6) \
    IF_TRACE_SUB(0xff)  {GET_FUNCTION_NAME vsi_o_tstate( 0xff,6, 11,SHOW_FUNCTION,__LINE__, a1,a2,a3,a4,a5,a6); }   
#define	  TRACE_TEST7(a1,a2,a3,a4,a5,a6,a7) \
    IF_TRACE_SUB(0xff)  {GET_FUNCTION_NAME vsi_o_tstate( 0xff,7,12,SHOW_FUNCTION,__LINE__, a1,a2,a3,a4,a5,a6,a7); }   

#define _FPGA_TRACE_TEST_
#ifdef _FPGA_TRACE_TEST_
#define TRACE_EVENT(a)                                IF_TRACE vsi_o_tstring( 0,SUB_MODULE_ID,a); 
#define TRACE_EVENT_P1(f,a1)                          IF_TRACE vsi_o_tstring( 1,SUB_MODULE_ID,f,a1); 
#define TRACE_EVENT_P2(f,a1,a2)                       IF_TRACE vsi_o_tstring( 2,SUB_MODULE_ID,f,a1,a2); 
#define TRACE_EVENT_P3(f,a1,a2,a3)                    IF_TRACE vsi_o_tstring( 3,SUB_MODULE_ID,f,a1,a2,a3); 
#define TRACE_EVENT_P4(f,a1,a2,a3,a4)                 IF_TRACE vsi_o_tstring( 4,SUB_MODULE_ID,f,a1,a2,a3,a4); 
#define TRACE_EVENT_P5(f,a1,a2,a3,a4,a5)              IF_TRACE vsi_o_tstring( 5,SUB_MODULE_ID,f,a1,a2,a3,a4,a5); 
#define TRACE_EVENT_P6(f,a1,a2,a3,a4,a5,a6)           IF_TRACE vsi_o_tstring( 6,SUB_MODULE_ID,f,a1,a2,a3,a4,a5,a6); 
#define TRACE_EVENT_P7(f,a1,a2,a3,a4,a5,a6,a7)        IF_TRACE vsi_o_tstring( 7,SUB_MODULE_ID,f,a1,a2,a3,a4,a5,a6,a7); 
#define TRACE_EVENT_P8(f,a1,a2,a3,a4,a5,a6,a7,a8)     IF_TRACE vsi_o_tstring( 8,SUB_MODULE_ID,f,a1,a2,a3,a4,a5,a6,a7,a8); 
#define TRACE_EVENT_P9(f,a1,a2,a3,a4,a5,a6,a7,a8,a9)  IF_TRACE vsi_o_tstring( 9,SUB_MODULE_ID,f,a1,a2,a3,a4,a5,a6,a7,a8,a9); 
#else

#define TRACE_EVENT         TRACE_PRINTF
#define TRACE_EVENT_P1      TRACE_PRINTF
#define TRACE_EVENT_P2      TRACE_PRINTF
#define TRACE_EVENT_P3      TRACE_PRINTF
#define TRACE_EVENT_P4      TRACE_PRINTF
#define TRACE_EVENT_P5      TRACE_PRINTF
#define TRACE_EVENT_P6      TRACE_PRINTF
#define TRACE_EVENT_P7      TRACE_PRINTF
#define TRACE_EVENT_P8      TRACE_PRINTF
#define TRACE_EVENT_P9      TRACE_PRINTF
#endif



#endif
