/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-26     RT-Thread    first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "uc_event.h"
#include "trcPortDefines.h"
#include "trace_interface.h"
#include "test_wiota_api.h"

#define TIMESTAMP   (__DATE__ " " __TIME__)
static const char *g_version = TIMESTAMP;


int main(void)
{
    //    l1_config_static_inform_init();    
    //IER = 0x0;
    //IPR = 0x0;

#ifdef _ROMFUNC_    
    dll_open();
#endif

    TRACE_PRINTF("main %s\n", g_version);
    
//    l1_check_debug();

    vTraceEnable(TRC_START);
    rt_thread_idle_sethook(trace_control);
    
    app_task_init();

//
//    while(1)
//    {
//        unsigned int total;
//        unsigned int used;
//        unsigned int max_used;
//

//        
//        uc_thread_delay(10000);
//        
//        rt_memory_info(&total,&used,&max_used);
//        TRACE_PRINTF("total %d used %d maxused %d cCount %d l1Count %d\n",total,used,max_used,g_dfe_before,l1Count);
//                
//    }

    return 0;
}


