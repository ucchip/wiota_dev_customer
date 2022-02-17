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

#ifdef UC8288_MODULE
#include "at.h"
#else
#include "test_wiota_api.h"
#endif
#ifdef _WATCHDOG_APP_
#include "uc_watchdog_app.h"
#endif

#ifdef _ROMFUNC_    
#include "dll.h"
#endif

extern void uc_wiota_flash_backup_init(void);


//void task_callback(struct rt_thread* from, struct rt_thread* to)
//{
//    rt_kprintf("name = %s, 0x%x\n", from->name, from);
//}
//
//
//void init_statistical_task_info(void)
//{
//    rt_scheduler_sethook(task_callback);
//}


int main(void)
{
#ifdef _ROMFUNC_    
    dll_open();
#endif

    uc_wiota_flash_backup_init();    

//    l1_check_debug();
#ifdef _WATCHDOG_APP_
    if(!watchdog_app_init())
        watchdog_app_enable();
#endif

#ifdef UC8288_MODULE
    at_server_init();
#else
    app_task_init();
#endif
    
//    app_task_init();
    
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

//    init_statistical_task_info();

    return 0;
}


