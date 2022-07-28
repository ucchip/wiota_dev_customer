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
#ifdef _RT_THREAD_
#include <rtdevice.h>
#endif

#ifdef _FPGA_
#include <board.h>
#include "uc_event.h"
#endif

#ifndef WIOTA_APP_DEMO
#ifdef UC8288_MODULE
#ifdef RT_USING_AT
#include "at.h"
#endif
#else
#include "test_wiota_api.h"
#endif
#else
#include "app_manager.h"
#endif

#ifdef _WATCHDOG_APP_
#include "uc_watchdog_app.h"
#endif

#ifdef _ROMFUNC_
#include "dll.h"
#endif

extern void uc_wiota_static_data_init(void);

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
extern void at_handle_log_uart(int uart_number);
#endif
extern void at_wiota_manager(void);
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

    uc_wiota_static_data_init();

#ifdef _WATCHDOG_APP_
    if (!watchdog_app_init())
        watchdog_app_enable();
#endif

#ifndef WIOTA_APP_DEMO
#ifdef UC8288_MODULE
#ifdef RT_USING_AT
    at_server_init();
    at_wiota_manager();
#endif
#else
    app_task_init();
#endif
#else
    app_manager_enter();
#endif

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
//    at_handle_log_uart(0);
#endif

    //    app_task_init();

    //    uc_wiota_light_func_enable(0);

    //
    //    while(1)
    //    {
    //        unsigned int total;
    //        unsigned int used;
    //        unsigned int max_used;
    //
    //        uc_thread_delay(2000);
    //
    //        rt_memory_info(&total,&used,&max_used);
    //        rt_kprintf("total %d used %d maxused %d\n",total,used,max_used);
    //
    //    }

    //    init_statistical_task_info();

    return 0;
}
