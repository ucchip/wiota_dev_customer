/******************************************************************************
* @file      test_wiota_key_send_data.c
* @brief     Sending information through key control
* @author    ypzhang
* @version   1.0
* @date      2023.11.28
*
* @copyright Copyright (c) 2018 UCchip Technology Co.,Ltd. All rights reserved.
*
******************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <rtthread.h>
#include <rtdevice.h>

#ifdef WIOTA_IOTE_KEY_SEND_DATA_DEMO
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"

#include "uc_string_lib.h"
#include "uc_gpio.h"
#include "uc_pin_app.h"

#define MAX_SEND_BYTE 20

#define LED1_GPIO_NUM            2 //send success
#define LED2_GPIO_NUM            3 //senf dail
#define LED3_GPIO_NUM            17 //recv success

#define IRQ1_KEY_NUM            16

// Receive data counter
uint32_t g_recv_count = 0;
uint32_t g_remember_recv_count = 0;

// Successfully sent data counter
uint32_t g_succ_send_count = 0;
uint32_t g_remember_succ_send_count = 0;

// data send fail counter
uint32_t g_fail_send_count = 0;
uint32_t g_remember_fail_send_count = 0;

uint8_t volatile key_pressed = 0;

static int pin_led_init(void)
{
    rt_err_t ret = RT_EOK;
    rt_pin_mode(LED1_GPIO_NUM, PIN_MODE_OUTPUT);
    rt_pin_mode(LED2_GPIO_NUM, PIN_MODE_OUTPUT);
    rt_pin_mode(LED3_GPIO_NUM, PIN_MODE_OUTPUT);

    return ret;
}

static void wiota_recv_cb(uc_recv_back_p data)
{
    // data successfully received
    if (0 == data->result)
    {
        rt_kprintf("---------recv_data info: ", data->result);
        if(data->type == UC_RECV_SCAN_FREQ || data->type == UC_RECV_SYNC_LOST)
        {
            rt_kprintf("this is statue info, type = %d---------\n", data->type);
            return;
        }

        for(u16_t index = 0; index < data->data_len; index++)
        {
            rt_kprintf("%c", *(data->data+index));
        }
        rt_kprintf(", data_len %d---------\n", data->data_len);
        g_recv_count ++;
        rt_kprintf("send_count = %d, recv_count = %d\n", g_succ_send_count + g_fail_send_count, g_recv_count);
        if(data->data)
        {
            // must free data.
            rt_free(data->data);            
        }               
    }
}

static void key_isr_callback(void *args)
{
    if (key_pressed == 0)
    {
        key_pressed = 1;
    }
}

static int pin_key_init(void)
{
    rt_err_t ret = RT_EOK;

    rt_pin_mode(IRQ1_KEY_NUM, PIN_MODE_INPUT_PULLUP);
    rt_pin_attach_irq(IRQ1_KEY_NUM, PIN_IRQ_MODE_RISING, key_isr_callback, RT_NULL);
    rt_pin_irq_enable(IRQ1_KEY_NUM, PIN_IRQ_ENABLE);

    return ret;
}

static void wiota_test_send_data_task(void* para)
{
    uint16_t index;
    uint16_t data_len;
    uc_stats_info_t stats_info;
    UC_OP_RESULT send_result;
    uint8_t sendbuffer[MAX_SEND_BYTE] ;
    uint32_t user_id = 0xabe44fca;  

    while(1)
    {
        // init wiota stack.
        uc_wiota_init();

        // set trial freq index
        uc_wiota_set_freq_info(25);
        
        // execute wiota protocol stack.
        uc_wiota_run();

        // register the receive data callback function.
        uc_wiota_register_recv_data_callback(wiota_recv_cb, UC_CALLBACK_NORAMAL_MSG);
        uc_wiota_register_recv_data_callback(wiota_recv_cb, UC_CALLBACK_STATE_INFO);

        // set user id
        uc_wiota_set_userid((unsigned int *)&user_id, 3);

        // wiota connect ap
        uc_wiota_connect();

        // wait for the connect ap. timeout 5s
        if (uc_wiota_wait_sync(5000, 2))
        {
            rt_kprintf("connect ap fail !!!!\n");
            continue;
        }

        rt_kprintf("connect ap cuccess !!!!\n");

        // clean up all wiota record status.
        uc_wiota_reset_stats(UC_STATS_TYPE_ALL);

        // key pin init
        pin_key_init();

        key_pressed = 0;
        while (1)
        {  
            if ( key_pressed == 1 )
            {
                // Simple anti shake processing
                rt_thread_mdelay(20);
                key_pressed = 0;
                if (rt_pin_read(IRQ1_KEY_NUM) == 1)
                {
                    rt_kprintf("key pressed !!!\n");
                }
                else
                {
                    rt_kprintf("key not pressed !!!\n");
                    continue;
                }
                
                // get wiota connect state
                UC_WIOTA_STATUS connect_state = uc_wiota_get_state();
                rt_kprintf("curr connect_state =  %d\n", connect_state);
                // get wiota receive send data record.
                uc_wiota_get_all_stats(&stats_info);

                // connect error or send success less than 20%.
                if ((stats_info.ul_sm_succ * 5 < stats_info.ul_sm_total && stats_info.ul_sm_total > 20) ||
                    UC_STATUS_SYNC_LOST == connect_state || UC_STATUS_ERROR == connect_state)
                {
                    rt_kprintf("fail: %s line %d sm_succ %d sm_total %d connect state %d\n",
                            __FUNCTION__, __LINE__, stats_info.ul_sm_succ, stats_info.ul_sm_total, connect_state);
                    break;
                }  

                // clean up all wiota record status.
                uc_wiota_reset_stats(UC_STATS_TYPE_ALL);

                // Random string
                data_len = rand() % MAX_SEND_BYTE;
                data_len = (data_len == 0)? 1 : data_len;
                rt_kprintf("---------send_data info: ");
                for (index = 0; index < data_len; index++){
                    sendbuffer[index] = (char)rand() % 94 + 33;
                    rt_kprintf("%c", sendbuffer[index]);
                }
                rt_kprintf(", data_len = %d--------- \n", data_len);

                // send test data twice.
                send_result = uc_wiota_send_data((unsigned char *)sendbuffer, data_len,  60000, RT_NULL);
                if (send_result == UC_OP_SUCC)
                {
                    g_succ_send_count++;
                    rt_kprintf("uc_wiota_send_data send success\n");
                }
                else
                {
                    g_fail_send_count++;
                    rt_kprintf("uc_wiota_send_data send fail!!!\n");
                    break;;
                }      
            }
            rt_thread_delay(50);
        }
        uc_wiota_exit();
    }
}

static void wiota_async_led_strl_task(void *parameter)
{
    // init GPIO pin
    pin_led_init();

    while (1)
    {
        // recv success
        if (g_remember_recv_count != g_recv_count)
        {
            g_remember_recv_count = g_recv_count;
            rt_pin_write(LED3_GPIO_NUM, PIN_HIGH);
            rt_thread_mdelay(100);
            rt_pin_write(LED3_GPIO_NUM, PIN_LOW);
        }
        
        // send data success
        if (g_remember_succ_send_count != g_succ_send_count)
        {
            g_remember_succ_send_count = g_succ_send_count;
            rt_pin_write(LED1_GPIO_NUM, PIN_HIGH);
            rt_thread_mdelay(100);
            rt_pin_write(LED1_GPIO_NUM, PIN_LOW);
        }

        // send data fail
        if (g_remember_fail_send_count != g_fail_send_count)
        {
            g_remember_fail_send_count = g_fail_send_count;
            rt_pin_write(LED2_GPIO_NUM, PIN_HIGH);
            rt_thread_mdelay(100);
            rt_pin_write(LED2_GPIO_NUM, PIN_LOW);
        }

        rt_thread_mdelay(100);
    }
}

void wiota_iote_key_data_recv_and_send_demo(void)
{
    rt_thread_t testTaskHandle1 = rt_thread_create("send_task", wiota_test_send_data_task, NULL, 1024, 3, 3);
    if (testTaskHandle1 != NULL)
    {
        rt_thread_startup(testTaskHandle1);
    }else{
        uc_wiota_exit();  
        return;
    }

    rt_thread_t testTaskHandle2 = rt_thread_create("led_ctrl_task", wiota_async_led_strl_task, NULL, 1024, 3, 3);
    if (testTaskHandle2 != NULL)
    {
        rt_thread_startup(testTaskHandle2);
    }else{
        uc_wiota_exit();  
        return;
    }

}
#endif //WIOTA_IOTE_KEY_SEND_DATA_DEMO