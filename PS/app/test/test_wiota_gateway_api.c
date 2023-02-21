
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
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "uc_wiota_gateway_api.h"
// #include "at.h"
#ifdef _LINUX_
#include <stdlib.h>
#include <string.h>
#else
#include "uc_string_lib.h"
#endif

#ifdef AT_WIOTA_GATEWAY_API
#ifndef AT_WIOTA_GATEWAY

static unsigned int network_state = 0;
static unsigned int *ptr = (unsigned int *)(0x1a107080);

#define AUTO_TEST_APP

#ifdef AUTO_TEST_APP
#define TEST_LED_PIN_NUM            7

static boolean led7_state = RT_FALSE;

static void led7_init(void)
{
    rt_pin_mode(TEST_LED_PIN_NUM, PIN_MODE_OUTPUT);
}

static void led7_off(void)
{
    rt_pin_write(TEST_LED_PIN_NUM, PIN_HIGH);
    led7_state = RT_TRUE;
}

static void led7_on(void)
{
    rt_pin_write(TEST_LED_PIN_NUM, PIN_LOW);
    led7_state = RT_FALSE;
}

static void led7_reversed_state(void)
{
    if(led7_state)
    {
        led7_on();
    }
    else
    {
        led7_off();
    }
}

static void wiota_gateway_api_auto_test_task(void *para)
{
    rt_kprintf("wiota_gateway_api_auto_test_task on.\n");
    led7_init();
    while(1)
    {
        switch(network_state)
        {
        case 0:
            led7_on();
            break;
        case 1:
            led7_reversed_state();
            break;
        case 2:
            led7_off();
            break;
        }
        rt_thread_mdelay(200);
    }
}
#endif

static void user_recv_data(void *data, int len, unsigned char data_type)
{
    rt_kprintf("+user recv:0x%x, %d, %s\n", data_type, len, data);
}

static void user_get_exception_state(unsigned char exception_type)
{
    rt_kprintf("gateway excxption type:0x%x\n", exception_type);
}

static int get_random(void)
{
    int random_num = 0;

    REG(0x1a10a02c) &= ~(1<<4);
    random_num = *ptr;
    if(random_num < 0)
    {
        random_num = (~random_num)+1;
    }
    return random_num;
}

static void wiota_gateway_api_test_task(void *para)
{
    int wiota_gateway_flag = 0;
    unsigned int random_delay_ms = 0;
    unsigned int dev_id = 0;
    unsigned int dev_id_len = 0;
    int dev_id_buf[16] = {0};
    unsigned char list[8] = {0xff};

    uc_wiota_get_userid(&dev_id, &dev_id_len);
    itoa(dev_id, dev_id_buf, 16);
    network_state = 0;
    random_delay_ms = get_random();
    random_delay_ms = get_random();
    random_delay_ms = get_random();
    rt_kprintf("random_delay_ms = %d.\n", random_delay_ms);
    rt_thread_mdelay(random_delay_ms%5000);

    uc_wiota_gateway_register_user_recv_cb(user_recv_data, user_get_exception_state);
    //uc_wiota_get_freq_list(list);
     #ifdef RT_USING_AT
        at_wiota_get_avail_freq_list(list, APP_CONNECT_FREQ_NUM);
    #endif
    
    while(uc_wiota_gateway_start("123456", list) != UC_GATEWAY_OK)
    {
        rt_thread_mdelay(5);
    }

    network_state = 1;
    while(1)
    {
        rt_kprintf("uc_wiota_gateway_api_handle send data.\n");
        wiota_gateway_flag = uc_wiota_gateway_send_data(dev_id_buf, 10, 10000);
        if(wiota_gateway_flag == 0)
        {
            network_state = 2;
            rt_kprintf("wiota_gateway_api_test_task send data failed.\n");
        }
        else
        {
            network_state = 1;
        }

        rt_thread_mdelay(10000);
    }

    uc_wiota_gateway_end();
}

void app_wiota_gateway_api_demo()
{
    rt_thread_t task_handle = RT_NULL;
#ifdef AUTO_TEST_APP
    rt_thread_t auto_task_handle = RT_NULL;
    auto_task_handle = rt_thread_create("auto_test",
                                        wiota_gateway_api_auto_test_task,
                                        RT_NULL,
                                        512,
                                        5,
                                        3);
    if(auto_task_handle != RT_NULL)
    {
        rt_thread_startup(auto_task_handle);
    }
#endif
    rt_kprintf("app_wiota_gateway_api_demo beginning...\n");

    task_handle = rt_thread_create("gateway_test",
                                    wiota_gateway_api_test_task,
                                    RT_NULL,
                                    1024,
                                    5,
                                    3);
    if(task_handle != RT_NULL)
    {
        rt_thread_startup(task_handle);
    }
    else
    {
        rt_kprintf("start app_wiota_gateway_api_demo task failed.\n");
    }
}

#endif
#endif
