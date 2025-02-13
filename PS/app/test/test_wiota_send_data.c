/*
 * test_wiota_send_data.c
 *
 *  Created on: 2023.11.02
 *  Author: ypzhang
 */

#include <rtthread.h>
#include <rtdevice.h>

#ifdef WIOTA_IOTE_SEND_DATA_DEMO
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "uc_string_lib.h"
#include "uc_gpio.h"

#include "uc_pin_app.h"

#define MAX_CONN_COUNT 4
#define MAX_SEND_COUNT 4

#define MAX_SEND_BYTE 200

unsigned int recv_count = 0;
unsigned int remenber_recv_count = 0;

#define LED1_GPIO_NUM 2  // send success
#define LED2_GPIO_NUM 3  // senf dail
#define LED3_GPIO_NUM 17 // recv success

typedef struct USER_DATA_INFO
{
    unsigned short data_len;
    unsigned char *data;
} USER_DATA_INFO;

static int pin_app_init(void)
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
        rt_kprintf("-------- wiota_iote_recv_data ----------- \n");
        rt_kprintf("wiota_recv_callback result = %d \r\n", data->result);

        for (unsigned int index = 0; index < data->data_len; index++)
        {
            rt_kprintf("%c", *(data->data + index));
        }
        rt_kprintf(", data_len %d\n", data->data_len);

        // must free data.
        rt_free(data->data);
        recv_count++;
    }
}

static void wiota_test_recv_data_task(void *parameter)
{
    while (1)
    {

        if (remenber_recv_count != recv_count)
        {
            remenber_recv_count = recv_count;
            // recv success
            rt_pin_write(LED3_GPIO_NUM, PIN_HIGH);
            rt_thread_mdelay(1000);
            rt_pin_write(LED3_GPIO_NUM, PIN_LOW);
        }
        rt_thread_mdelay(1000);
    }
}

static void wiota_test_send_data_task(void *parameter)
{
    int i;
    unsigned int data_len;
    rt_err_t ret = RT_EOK;
    uc_stats_info_t stats_info;
    uc_op_result_e send_result;
    char sendbuffer[MAX_SEND_BYTE]; //= {"Hello WIoTa AP"};
    unsigned int user_id = 0xabe44fca;

    while (1)
    {
        // init wiota stack.
        uc_wiota_init();

        // set trial freq index
        uc_wiota_set_freq_info(50);

        // execute wiota protocol stack.
        uc_wiota_run();

        // init GPIO pin
        ret = pin_app_init();
        if (ret != RT_EOK)
        {
            rt_kprintf("pin test error.\r\n");
            continue;
        }

        // register the receive data callback function.
        uc_wiota_register_recv_data_callback(wiota_recv_cb, UC_CALLBACK_NORAMAL_MSG);
        uc_wiota_register_recv_data_callback(wiota_recv_cb, UC_CALLBACK_STATE_INFO);

        // set user id
        uc_wiota_set_userid(&user_id, 4);
        for (i = 0; i < MAX_CONN_COUNT; i++)
        {
            // wiota connect ap
            uc_wiota_connect();

            // wait for the connect ap. timeout 4s
            if (uc_wiota_wait_sync(4000, 2))
            {
                rt_kprintf("connect ap fail, count =  %d\n", i);
                continue;
            }
            break;
        }
        if (i == MAX_CONN_COUNT)
        {
            rt_kprintf("connect ap fail, count =  %d\n", i);
            uc_wiota_exit();
            continue;
        }

        rt_kprintf("connect ap cuccess, count =  %d\n", i);

        // clean up all wiota record status.
        uc_wiota_reset_stats(UC_STATS_TYPE_ALL);

        while (1)
        {

            // get wiota connect state
            uc_wiota_status_e connect_state = uc_wiota_get_state();
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
            data_len = (data_len == 0) ? 1 : data_len;
            rt_kprintf("send_data info: ");
            for (i = 0; i < data_len; i++)
            {
                sendbuffer[i] = (char)rand() % 94 + 33;
                rt_kprintf("%c", sendbuffer[i]);
            }
            rt_kprintf(", data_len = %d \n", data_len);

            // send test data.
            for (i = 0; i < MAX_SEND_COUNT; i++)
            {
                send_result = uc_wiota_send_data((unsigned char *)sendbuffer, data_len, 60000, RT_NULL);
                if (send_result == UC_OP_SUCC)
                {
                    rt_kprintf("uc_wiota_send_data send success, send count %d\n", i);
                    break;
                }
                else if (send_result == UC_OP_TIMEOUT)
                {
                    rt_kprintf("uc_wiota_send_data send timeout!!!, send count %d\n", i);
                }
                else
                {
                    rt_kprintf("uc_wiota_send_data send fail!!!, send count %d\n", i);
                }
            }

            if (!send_result)
            {
                // send success
                rt_pin_write(LED1_GPIO_NUM, PIN_HIGH);
            }
            else
            {
                // send fail
                rt_pin_write(LED2_GPIO_NUM, PIN_HIGH);
            }
            rt_thread_mdelay(1000);

            rt_pin_write(LED1_GPIO_NUM, PIN_LOW);
            rt_pin_write(LED2_GPIO_NUM, PIN_LOW);

            rt_thread_mdelay(4000);

            if (i == MAX_SEND_COUNT)
            {
                rt_kprintf("uc_wiota_send_data send fail!!!, send count %d\n", i);
                break;
            }
        }

        uc_wiota_exit();
    }
}

void wiota_iote_data_recv_and_send_demo(void)
{
    rt_thread_t testTaskHandle2 = rt_thread_create("send_task", wiota_test_send_data_task, NULL, 1024*3, 3, 3);
    if (testTaskHandle2 != NULL)
    {
        rt_thread_startup(testTaskHandle2);
    }
    else
    {
        uc_wiota_exit();
        return;
    }

    rt_thread_t testTaskHandle3 = rt_thread_create("recv_task", wiota_test_recv_data_task, NULL, 1024, 3, 3);
    if (testTaskHandle3 != NULL)
    {
        rt_thread_startup(testTaskHandle3);
    }
    else
    {
        uc_wiota_exit();
        return;
    }
}

#endif // WIOTA_IOTE_SEND_DATA_DEMO