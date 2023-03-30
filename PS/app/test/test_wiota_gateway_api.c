
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

#define LED_ON 0
#define LEN_TW 1
#define LED_OFF 2

static unsigned int network_state = LED_OFF;
static unsigned int gatway_led_state = LED_OFF;
static unsigned int gateway_send_state = LEN_TW;

#define AUTO_TEST_APP

#ifdef AUTO_TEST_APP
#define TEST_LED_PIN_NUM 2
#define TEST_GATEWAY_PIN_NUM 3
#define TEST_GATEWAY_SEND_PIN_NUM 7

static boolean led_send_state = RT_FALSE;
static boolean led7_state = RT_FALSE;
static boolean led3_state = RT_FALSE;

#define GATEWAY_SCAN_TIMEOUT 40000

extern int uc_gateway_get_random(void);

static void led3_init(void)
{
    rt_pin_mode(TEST_GATEWAY_PIN_NUM, PIN_MODE_OUTPUT);
    rt_pin_mode(TEST_GATEWAY_SEND_PIN_NUM, PIN_MODE_OUTPUT);
    rt_pin_write(TEST_GATEWAY_SEND_PIN_NUM, PIN_LOW); // off
}

static void led3_off(void)
{
    rt_pin_write(TEST_GATEWAY_PIN_NUM, PIN_LOW);
    led3_state = RT_TRUE;
}

static void led3_on(void)
{
    rt_pin_write(TEST_GATEWAY_PIN_NUM, PIN_HIGH);
    led3_state = RT_FALSE;
}

static void led3_reversed_state(void)
{
    if (led3_state)
    {
        led3_on();
    }
    else
    {
        led3_off();
    }
}

static void led7_init(void)
{
    rt_pin_mode(TEST_LED_PIN_NUM, PIN_MODE_OUTPUT);
    rt_pin_write(TEST_LED_PIN_NUM, PIN_LOW); // off
}

static void led7_off(void)
{
    rt_pin_write(TEST_LED_PIN_NUM, PIN_LOW);
    led7_state = RT_TRUE;
}

static void led7_on(void)
{
    rt_pin_write(TEST_LED_PIN_NUM, PIN_HIGH);
    led7_state = RT_FALSE;
}

static void led7_reversed_state(void)
{
    if (led7_state)
    {
        led7_on();
    }
    else
    {
        led7_off();
    }
}

static void led_send_off(void)
{
    rt_pin_write(TEST_GATEWAY_SEND_PIN_NUM, PIN_LOW);
    led_send_state = RT_TRUE;
}

static void led_send_on(void)
{
    rt_pin_write(TEST_GATEWAY_SEND_PIN_NUM, PIN_HIGH);
    led_send_state = RT_FALSE;
}

static void led_send_reversed_state(void)
{
    if (led_send_state)
    {
        led_send_on();
    }
    else
    {
        led_send_off();
    }
}

static void wiota_gateway_api_auto_test_task(void *para)
{
    static int rem = 0;
    rt_kprintf("wiota_gateway_api_auto_test_task on.\n");
    led7_init();
    led3_init();

    rem = gateway_send_state;
    while (1)
    {
        switch (network_state)
        {
        case LED_ON:
            led7_on();
            break;
        case LEN_TW:
            led7_reversed_state();
            break;
        case LED_OFF:
            led7_off();
            break;
        }

        switch (gatway_led_state)
        {
        case LED_ON:
            led3_on();
            break;
        case LEN_TW:
            led3_reversed_state();
            break;
        case LED_OFF:
            led3_off();
            break;
        }

        if (rem != gateway_send_state)
        {
            rt_kprintf("rem %d gateway_send_state %d\n", rem, gateway_send_state);
            if (0 == gateway_send_state % 2)
            { // send success
                int cout = 2;
                while ((cout--) > 0)
                {
                    led_send_reversed_state();
                    rt_thread_mdelay(300);
                }
            }
            else
            { // send fail
                int cout = 4;
                while ((cout--) > 0)
                {
                    led_send_reversed_state();
                    rt_thread_mdelay(300);
                }
            }
            led_send_off();
            rem = gateway_send_state;
        }
        else
            rt_thread_mdelay(200);
    }
}
#endif
static void gateway_test_user_recv_data(void *data, unsigned int len, unsigned char data_type)
{
    rt_kprintf("+user recv:0x%x, %d, %s\n", data_type, len, data);
}

static void user_get_exception_state(unsigned char exception_type)
{
    //rt_kprintf("gateway excxption type:0x%x\n", exception_type);
}

static int gateway_get_static_freq(char *list)
{
    int num;
    uc_wiota_get_freq_list((unsigned char *)list);

    for (num = 0; num < 16; num++)
    {
        //rt_kprintf("static freq *(list+%d) %d\n", num, *(list+num));
        if (*(list + num) == 0xFF)
            break;
    }
    return num;
}

static char gateway_test_scantf(void)
{
    unsigned char list[16] = {0xff};
    int list_len;
    uc_recv_back_t recv_result;
    unsigned char freq = 0xFF;
    char rssi;

    uc_wiota_init();
    uc_wiota_run();

    list_len = gateway_get_static_freq((char *)list);
    switch (list_len)
    {
    case 0:
        uc_wiota_exit();
        return freq;
    case 1:
        uc_wiota_exit();
        return list[0];
    }

    uc_wiota_scan_freq(list, list_len, 0, GATEWAY_SCAN_TIMEOUT, RT_NULL, &recv_result);
    if (UC_OP_SUCC == recv_result.result)
    {
        int freq_num = recv_result.data_len / sizeof(uc_freq_scan_result_t);
        uc_freq_scan_result_p freq_info = (uc_freq_scan_result_p)recv_result.data;

        for (int i = 0; i < freq_num; i++)
        {
            if (freq_info->is_synced)
            {
                if (freq == 0xFF)
                {
                    freq = freq_info->freq_idx;
                    rssi = freq_info->rssi;
                }
                else if (rssi < freq_info->rssi)
                {
                    freq = freq_info->freq_idx;
                    rssi = freq_info->rssi;
                }
            }
        }

        rt_free(recv_result.data);
    }
    uc_wiota_exit();
    return freq;
}

static void gateway_send_data_test(void)
{
    char dev_id_buf[16] = {0};
    unsigned char count = 0;
    unsigned int delay_time = 0;
    unsigned char serial[16] = {0};
    uc_stats_info_t stats_info_ptr;
    // int send_counter = 2;

    for (int n = 0; n < 16; n++)
        dev_id_buf[n] = n;

    uc_wiota_get_dev_serial(serial);

    delay_time = ((serial[12] << 8) | serial[13]);

    rt_kprintf("gateway_send_data_test start\n");

    while (1)
    {
        UC_WIOTA_STATUS connect_state = uc_wiota_get_state();
        uc_wiota_get_all_stats(&stats_info_ptr);

        if ((stats_info_ptr.ul_sm_succ * 5 < stats_info_ptr.ul_sm_total && stats_info_ptr.ul_sm_total > 20) ||
            UC_STATUS_SYNC_LOST == connect_state || UC_STATUS_ERROR == connect_state)
        {
            rt_kprintf("%s line %d sm_succ %d sm_total %d connect state %d\n",
                       __FUNCTION__, __LINE__, stats_info_ptr.ul_sm_succ, stats_info_ptr.ul_sm_total, connect_state);
            return;
        }

        count++;
        if (uc_wiota_gateway_send_data(dev_id_buf, 16, 10000) == 0)
        { // fail
            rt_kprintf("uc_wiota_gateway_api_handle send data fail.\n");
            gateway_send_state = ((count << 1) + 1);
        }
        else
        { //success
            rt_kprintf("uc_wiota_gateway_api_handle send data succ:\n");
            for (int num = 0; num < 10; num++)
                rt_kprintf("0x%x ", dev_id_buf[num]);
            rt_kprintf("\n");
            gateway_send_state = (count << 1);
        }

        for (int n = 0; n < 16; n++)
            dev_id_buf[n] += 16;

        rt_kprintf("now delay time %d(0x%x)ms. 0 is default 10s\n", delay_time, delay_time);
        rt_thread_mdelay(delay_time == 0 ? 10000 : delay_time);
    }
}

static int gateway_test_check_connect_state(void)
{
    short count = 300;
    unsigned char flag = 0;
    while (count--)
    {
        if (UC_STATUS_SYNC == uc_wiota_get_state())
        {
            flag++;
            if (flag > 2)
                return 0; // success
        }
        rt_thread_delay(20);
    }
    return 1; // fail
}

static int gateway_test_heap(void)
{
    unsigned int total;
    unsigned int used;
    unsigned int max_used;

    rt_memory_info(&total, &used, &max_used);
    rt_kprintf("-->total %d used %d maxused %d\n", total, used, max_used);
    return 0;
}
extern void at_wiota_get_avail_freq_list(unsigned char *output_list, unsigned char list_len);
static void wiota_gateway_api_test_task(void *para)
{
    //unsigned char list[16] = {0xff};
    unsigned char freq = 0xFF;
    int result = 0;

    //uc_wiota_log_switch(0, 0);

    network_state = LEN_TW;

    while (1)
    {
        //uc_gateway_state_t state;

        network_state = LEN_TW;
        gatway_led_state = LED_OFF;
        gateway_test_heap();
        //mem_test_trace();

        freq = gateway_test_scantf();
        if (0xFF == freq)
        {
            rt_thread_mdelay(1000);
            continue;
        }

        uc_wiota_init();

        rt_kprintf("select freq %d\n", freq);
        uc_wiota_set_freq_info(freq);
        uc_wiota_run();
        uc_wiota_connect();

        // wait connect
        if (gateway_test_check_connect_state())
        {
            uc_wiota_exit();
            continue;
        }

        network_state = LED_ON;
        gatway_led_state = LEN_TW;

        result = uc_wiota_gateway_start(UC_GATEWAY_MODE, "123456", RT_NULL);
        rt_kprintf("%s line %d result %d\n", __FUNCTION__, __LINE__, result);
        switch (result)
        {
        case UC_GATEWAY_OK:
            gatway_led_state = LED_ON;
            break;
        default:
            uc_wiota_gateway_end();
            uc_wiota_exit();
            rt_thread_mdelay(1000);
            gatway_led_state = LED_OFF;
            continue;
        }

        uc_wiota_gateway_register_user_recv_cb(gateway_test_user_recv_data, user_get_exception_state);

        gateway_send_data_test();

        uc_wiota_gateway_end();

        uc_wiota_exit();
        rt_thread_mdelay(500);
        gateway_test_heap();
        //mem_test_trace();
    }
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
    if (auto_task_handle != RT_NULL)
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
    if (task_handle != RT_NULL)
    {
        rt_thread_startup(task_handle);
    }
    else
    {
        rt_kprintf("start app_wiota_gateway_api_demo task failed.\n");
    }
}

#endif
