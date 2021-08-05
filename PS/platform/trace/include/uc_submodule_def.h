#ifndef _UC_SUB_MODULE_DEF_
#define _UC_SUB_MODULE_DEF_

/********
 * 
 * This file should not be placed ahead of vsi.h
 * 
 * */
 
#ifdef   SUB_MODULE_ID
    #undef   SUB_MODULE_ID
#endif


#ifdef      L1_TEST_SUBMODULE
#define     SUB_MODULE_ID           0x0 
#endif

#ifdef      L1_SUBMODULE
#define     SUB_MODULE_ID           0x1
#endif

#ifdef      APP_SUBMODULE
#define     SUB_MODULE_ID           0x2
#endif

#ifdef      MAC_SUBMODULE
#define     SUB_MODULE_ID           0x3
#endif

#ifdef      TASK_SUBMODULE
#define     SUB_MODULE_ID           0x4
#endif

#ifndef   SUB_MODULE_ID
#define   SUB_MODULE_ID             0xff
#endif


#endif
