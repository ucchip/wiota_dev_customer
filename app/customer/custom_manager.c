#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include <board.h>
#include <string.h>
#include "manager_queue.h"
#include "custom_manager.h"
#include "app_manager_logic.h"
#include "app_manager_cfg.h"
#include "manager_module.h"
#include "uc_wiota_api.h"
#include "app_manager.h"
#include "uc_coding.h"
#include "cJSON.h"
#include "light_ctrl.h"
#include "switch_ctrl.h"
#include "custom_pair.h"
#include "custom_protocol.h"
#include "custom_data.h"
#include "uc_wiota_static.h"

// user peripheral control

static void *custom_queue_handle;
t_costom_manager costom_manger;
//static rt_mutex_t p_custom_send_lock = NULL;

static unsigned char custom_get_device_type(void)
{
    unsigned char dev_type = 0;
    unsigned char count = 0;

    custom_get_devinfo(&dev_type, &count);

    return dev_type;
}

static void custom_send_lock_init(void)
{
    //p_custom_send_lock = rt_mutex_create("cu_sl", RT_IPC_FLAG_FIFO);
}

static void custom_send_lock_take(void)
{
    //if (p_custom_send_lock != NULL)
    //{
    //    rt_mutex_take(p_custom_send_lock, RT_WAITING_FOREVER);
    //}
    manager_send_lock_take();
}

static void custom_send_lock_release(void)
{
    //if (p_custom_send_lock != NULL)
    //{
    //    rt_mutex_release(p_custom_send_lock);
    //}
    manager_send_lock_release();
}

int custom_manager_create_queue(void)
{
    // create wiota app manager queue.
    custom_queue_handle = manager_create_queue("custom_manager", 4, 16, UC_SYSTEM_IPC_FLAG_PRIO);
    if (custom_queue_handle == RT_NULL)
    {
        rt_kprintf("manager_create_queue error\n");
        return 1;
    }

    return 0;
}

/*
*  Other module send queue data to manager_custom module.
*/
int manager_sendqueue_custom(int src_task, int cmd, void *data)
{
    t_app_logic_message *message = rt_malloc(sizeof(t_app_logic_message));
    if (RT_NULL == message)
    {
        rt_kprintf("wiota_recv_wiota_data malloc error\n");
        return 1;
    }
    message->cmd = cmd;
    message->data = data;

    return manager_send_page(custom_queue_handle, src_task, message);
}

static void custom_send_data_result_callback(unsigned int data_id, UC_OP_RESULT send_result)
{
    rt_kprintf("custom_send_data_result_callback, data_id = %d, send_result = %d\r\n", data_id, send_result);

    custom_send_lock_release();
}

void custom_report_light_property(void)
{
    t_send_data_info send_data_info;
    unsigned char *app_data = NULL;
    unsigned int get_data_id = 0;
    int result = 0;

    app_data = custom_create_light_property_data();
    if (app_data == NULL)
    {
        goto __end;
    }
    //For any service, logic module must response to the request after processing it.
    send_data_info.auto_src_addr = 1;
    send_data_info.dest_addr_type = ADDR_TYPE_SERVER;
    send_data_info.need_response = 0;
    send_data_info.is_response = 0;
    send_data_info.compress_flag = 1;
    send_data_info.packet_num_type = 0;
    send_data_info.src_addr = 0;
    send_data_info.dest_addr = 0;
    send_data_info.cmd_type = APP_CMD_PROPERTY_REPORT;
    custom_send_lock_take();
    result = manager_send_wiota_data(&send_data_info, app_data, strlen((const char *)app_data), custom_send_data_result_callback, &get_data_id);
    if (result != 0)
    {
        custom_send_lock_release();
    }
    rt_kprintf("custom_report_light_property result = %d, get_data_id = %d\r\n", result, get_data_id);

__end:
    if (app_data != NULL)
    {
        custom_delete_light_property_data(app_data);
    }
}

void custom_report_light_state(void)
{
    e_light_state state;
    t_send_data_info send_data_info;
    unsigned char *app_data = NULL;
    unsigned int get_data_id = 0;
    int result = 0;

    state = light_ctrl_get_state();
    app_data = custom_create_light_state_data(&state);
    if (app_data == NULL)
    {
        goto __end;
    }
    //For any service, logic module must response to the request after processing it.
    send_data_info.auto_src_addr = 1;
    send_data_info.dest_addr_type = ADDR_TYPE_SERVER;
    send_data_info.need_response = 0;
    send_data_info.is_response = 0;
    send_data_info.compress_flag = 1;
    send_data_info.packet_num_type = 0;
    send_data_info.src_addr = 0;
    send_data_info.dest_addr = 0;
    send_data_info.cmd_type = APP_CMD_IOTE_STATE_REPORT;
    custom_send_lock_take();
    result = manager_send_wiota_data(&send_data_info, app_data, strlen((const char *)app_data), custom_send_data_result_callback, &get_data_id);
    if (result != 0)
    {
        custom_send_lock_release();
    }
    rt_kprintf("custom_report_light_state result = %d, get_data_id = %d\r\n", result, get_data_id);

__end:
    if (app_data != NULL)
    {
        custom_delete_light_state_data(app_data);
    }

    // send dev state request. must wait for respons result.
    //costom_manger.next_process = MANAGER_LOGIC_RESPONSE_DEVSTATE;
}

void custom_request_light_pair(void)
{
    t_send_data_info send_data_info;
    unsigned char *app_data = NULL;
    unsigned int get_data_id = 0;
    int result = 0;

    app_data = custom_create_light_pair_data();
    if (app_data == NULL)
    {
        goto __end;
    }
    //For any service, logic module must response to the request after processing it.
    send_data_info.auto_src_addr = 1;
    send_data_info.dest_addr_type = ADDR_TYPE_SERVER;
    send_data_info.need_response = 0;
    send_data_info.is_response = 0;
    send_data_info.compress_flag = 1;
    send_data_info.packet_num_type = 0;
    send_data_info.src_addr = 0;
    send_data_info.dest_addr = 0;
    send_data_info.cmd_type = APP_CMD_IOTE_PAIR_REQUEST;
    custom_send_lock_take();
    result = manager_send_wiota_data(&send_data_info, app_data, strlen((const char *)app_data), custom_send_data_result_callback, &get_data_id);
    if (result != 0)
    {
        custom_send_lock_release();
    }
    rt_kprintf("custom_request_light_pair result = %d, get_data_id = %d\r\n", result, get_data_id);

__end:
    if (app_data != NULL)
    {
        custom_delete_light_pair_data(app_data);
    }
}

void custom_cancel_light_pair(void)
{
    t_send_data_info send_data_info;
    unsigned char *app_data = NULL;
    unsigned int get_data_id = 0;
    int result = 0;

    app_data = custom_create_light_pair_data();
    if (app_data == NULL)
    {
        goto __end;
    }
    //For any service, logic module must response to the request after processing it.
    send_data_info.auto_src_addr = 1;
    send_data_info.dest_addr_type = ADDR_TYPE_SERVER;
    send_data_info.need_response = 0;
    send_data_info.is_response = 0;
    send_data_info.compress_flag = 1;
    send_data_info.packet_num_type = 0;
    send_data_info.src_addr = 0;
    send_data_info.dest_addr = 0;
    send_data_info.cmd_type = APP_CMD_IOTE_PAIR_REQUEST_CANCEL;
    custom_send_lock_take();
    result = manager_send_wiota_data(&send_data_info, app_data, strlen((const char *)app_data), custom_send_data_result_callback, &get_data_id);
    if (result != 0)
    {
        custom_send_lock_release();
    }
    rt_kprintf("custom_request_light_pair result = %d, get_data_id = %d\r\n", result, get_data_id);

__end:
    if (app_data != NULL)
    {
        custom_delete_light_pair_data(app_data);
    }
}

void custom_report_switch_property(void)
{
    t_send_data_info send_data_info;
    unsigned char *app_data = NULL;
    unsigned int get_data_id = 0;
    int result = 0;

    app_data = custom_create_switch_property_data(switch_get_count());
    if (app_data == NULL)
    {
        goto __end;
    }
    //For any service, logic module must response to the request after processing it.
    send_data_info.auto_src_addr = 1;
    send_data_info.dest_addr_type = ADDR_TYPE_SERVER;
    send_data_info.need_response = 0;
    send_data_info.is_response = 0;
    send_data_info.compress_flag = 1;
    send_data_info.packet_num_type = 0;
    send_data_info.src_addr = 0;
    send_data_info.dest_addr = 0;
    send_data_info.cmd_type = APP_CMD_PROPERTY_REPORT;
    custom_send_lock_take();
    result = manager_send_wiota_data(&send_data_info, app_data, strlen((const char *)app_data), custom_send_data_result_callback, &get_data_id);
    if (result != 0)
    {
        custom_send_lock_release();
    }
    rt_kprintf("custom_report_switch_property result = %d, get_data_id = %d\r\n", result, get_data_id);

__end:
    if (app_data != NULL)
    {
        custom_delete_switch_property_data(app_data);
    }
}

void custom_report_switch_state(void)
{
    unsigned char state = 0;
    t_send_data_info send_data_info;
    unsigned char *app_data = NULL;
    unsigned int get_data_id = 0;
    int result = 0;

    switch_get_all_state(&state);
    app_data = custom_create_switch_state_data(state);
    if (app_data == NULL)
    {
        goto __end;
    }
    //For any service, logic module must response to the request after processing it.
    send_data_info.auto_src_addr = 1;
    send_data_info.dest_addr_type = ADDR_TYPE_SERVER;
    send_data_info.need_response = 0;
    send_data_info.is_response = 0;
    send_data_info.compress_flag = 1;
    send_data_info.packet_num_type = 0;
    send_data_info.src_addr = 0;
    send_data_info.dest_addr = 0;
    send_data_info.cmd_type = APP_CMD_IOTE_STATE_REPORT;
    custom_send_lock_take();
    result = manager_send_wiota_data(&send_data_info, app_data, strlen((const char *)app_data), custom_send_data_result_callback, &get_data_id);
    if (result != 0)
    {
        custom_send_lock_release();
    }
    rt_kprintf("custom_report_switch_state result = %d, get_data_id = %d\r\n", result, get_data_id);

__end:
    if (app_data != NULL)
    {
        custom_delete_switch_state_data(app_data);
    }

    // send dev state request. must wait for respons result.
    //costom_manger.next_process = MANAGER_LOGIC_RESPONSE_DEVSTATE;
}

void custom_request_switch_pair(unsigned char sw_index)
{
    t_send_data_info send_data_info;
    unsigned char *app_data = NULL;
    unsigned int get_data_id = 0;
    int result = 0;

    app_data = custom_create_switch_pair_data(sw_index);
    if (app_data == NULL)
    {
        goto __end;
    }
    //For any service, logic module must response to the request after processing it.
    send_data_info.auto_src_addr = 1;
    send_data_info.dest_addr_type = ADDR_TYPE_SERVER;
    send_data_info.need_response = 0;
    send_data_info.is_response = 0;
    send_data_info.compress_flag = 1;
    send_data_info.packet_num_type = 0;
    send_data_info.src_addr = 0;
    send_data_info.dest_addr = 0;
    send_data_info.cmd_type = APP_CMD_IOTE_PAIR_REQUEST;
    custom_send_lock_take();
    result = manager_send_wiota_data(&send_data_info, app_data, strlen((const char *)app_data), custom_send_data_result_callback, &get_data_id);
    if (result != 0)
    {
        custom_send_lock_release();
    }
    rt_kprintf("custom_request_switch_pair sw_index = %d, result = %d, get_data_id = %d\r\n", sw_index, result, get_data_id);

__end:
    if (app_data != NULL)
    {
        custom_delete_switch_pair_data(app_data);
    }
}

void custom_cancel_switch_pair(unsigned char sw_index)
{
    t_send_data_info send_data_info;
    unsigned char *app_data = NULL;
    unsigned int get_data_id = 0;
    int result = 0;

    app_data = custom_create_switch_pair_data(sw_index);
    if (app_data == NULL)
    {
        goto __end;
    }
    //For any service, logic module must response to the request after processing it.
    send_data_info.auto_src_addr = 1;
    send_data_info.dest_addr_type = ADDR_TYPE_SERVER;
    send_data_info.need_response = 0;
    send_data_info.is_response = 0;
    send_data_info.compress_flag = 1;
    send_data_info.packet_num_type = 0;
    send_data_info.src_addr = 0;
    send_data_info.dest_addr = 0;
    send_data_info.cmd_type = APP_CMD_IOTE_PAIR_REQUEST_CANCEL;
    custom_send_lock_take();
    result = manager_send_wiota_data(&send_data_info, app_data, strlen((const char *)app_data), custom_send_data_result_callback, &get_data_id);
    if (result != 0)
    {
        custom_send_lock_release();
    }
    rt_kprintf("custom_cancel_switch_pair sw_index = %d, result = %d, get_data_id = %d\r\n", sw_index, result, get_data_id);

__end:
    if (app_data != NULL)
    {
        custom_delete_switch_pair_data(app_data);
    }
}

void custom_switch_ctrl_light(unsigned char sw_index, unsigned int light_address, unsigned char ctrl_action)
{
    t_send_data_info send_data_info;
    unsigned char *app_data = NULL;
    unsigned int get_data_id = 0;
    int result = 0;

    app_data = custom_create_switch_ctrl_light_data(sw_index, ctrl_action);
    if (app_data == NULL)
    {
        goto __end;
    }
    //For any service, logic module must response to the request after processing it.
    send_data_info.auto_src_addr = 1;
    send_data_info.dest_addr_type = ADDR_TYPE_USER_DEFINED;
    send_data_info.need_response = 0;
    send_data_info.is_response = 0;
    send_data_info.compress_flag = 1;
    send_data_info.packet_num_type = 1;
    send_data_info.src_addr = 0;
    send_data_info.dest_addr = light_address;
    send_data_info.cmd_type = APP_CMD_NET_CTRL_IOTE;
    custom_send_lock_take();
    result = manager_send_wiota_data(&send_data_info, app_data, strlen((const char *)app_data), custom_send_data_result_callback, &get_data_id);
    if (result != 0)
    {
        custom_send_lock_release();
    }
    rt_kprintf("custom_switch_ctrl_light sw_index = %d, light_address = 0x%08x, ctrl_action = %d, result = %d, get_data_id = %d\r\n", sw_index, light_address, ctrl_action, result, get_data_id);

__end:
    if (app_data != NULL)
    {
        custom_delete_switch_ctrl_light_data(app_data);
    }
}

static void custom_cycle_report_state_process(void)
{
    static unsigned int last_tick = 0;
    unsigned int interval_tick = 0;
    unsigned int cur_tick = rt_tick_get();
    if (cur_tick >= last_tick)
    {
        interval_tick = cur_tick - last_tick;
    }
    else
    {
        interval_tick = 0xffffffff - last_tick + cur_tick + 1;
    }

    if (interval_tick > CUSTOM_REPORT_STATE_CYCLE_TIME)
    {
        if (manager_get_logic_work_state() == 1)
        {
            if (custom_get_device_type() == DEV_TYPE_LIGHT)
            {
                custom_report_light_state();
            }
            else
            {
                custom_report_switch_state();
            }
        }
        last_tick = cur_tick;
    }
}

void custom_manager_task(void *pPara)
{
    rt_kprintf("custom_manager_task enter\n");

    custom_send_lock_init();
#ifdef APP_DEMO_LIGHT
    custom_set_devinfo(DEV_TYPE_LIGHT, 1);
    uc_wiota_save_static_info(0);
#endif
#ifdef APP_DEMO_SWITCH
    custom_set_devinfo(DEV_TYPE_SWITCH, 3);
    uc_wiota_save_static_info(0);
#endif

    uc_wiota_light_func_enable(0); // close ps light for app test
    if (custom_get_device_type() == DEV_TYPE_LIGHT)
    {
        light_ctrl_init();
        custom_light_pair_info_init();
        manager_set_device_type_name("light");
        // setting custom data recv callback function.
        manager_set_custom_data_callback(custom_light_recv_data_process);
    }
    else
    {
        switch_ctrl_init();
        custom_switch_pair_info_init();
        manager_set_device_type_name("switch");
        // setting custom data recv callback function.
        manager_set_custom_data_callback(custom_switch_recv_data_process);
    }

    while (manager_get_logic_work_state() == 0)
    {
        rt_thread_mdelay(100);
    }

    // report current dev state request. must wait for execution results.
    if (custom_get_device_type() == DEV_TYPE_LIGHT)
    {
        custom_report_light_property();
        custom_report_light_state();
        manager_request_multicast_addr();
        //manager_request_config();
        custom_request_light_pair();
    }
    else
    {
        custom_report_switch_property();
        custom_report_switch_state();
        manager_request_multicast_addr();
        //manager_request_config();
        for (unsigned index = 0; index < switch_get_count(); index++)
        {
            custom_request_switch_pair(index);
        }
    }

    while (1)
    {
        t_app_manager_message *page;
        t_app_logic_message *message;

        if (QUEUE_EOK == manager_recv_queue(custom_queue_handle, (void **)&page, 10))
        {
            message = page->message;

            rt_kprintf("custom_manager_task message->cmd %d\n", message->cmd);

            //if (MANAGER_LOGIC_INDENTIFICATION == page->src_task)
            //    custom_manager_process(message);
            switch (message->cmd)
            {
            case CUSTOM_LOGIC_REPORT_DEVSTATE:
            {
                if (custom_get_device_type() == DEV_TYPE_LIGHT)
                {
                    custom_report_light_state();
                }
                else
                {
                    custom_report_switch_state();
                }
                break;
            }
            default:
            {
                break;
            }
            }

            if (RT_NULL != message->data)
                rt_free(message->data);
            rt_free(message);
            rt_free(page);
        }

        custom_cycle_report_state_process();

        if (1)
        {
            static unsigned int last_tick = 0;
            unsigned int interval_tick = 0;
            unsigned int cur_tick = rt_tick_get();
            if (cur_tick >= last_tick)
            {
                interval_tick = cur_tick - last_tick;
            }
            else
            {
                interval_tick = 0xffffffff - last_tick + cur_tick + 1;
            }

            if (interval_tick > 5000)
            {
                rt_uint32_t total = 0;
                rt_uint32_t used = 0;
                rt_uint32_t max_used = 0;
                rt_memory_info(&total, &used, &max_used);
                rt_kprintf("total = %d, used = %d, max_used = %d\r\n", total, used, max_used);
                last_tick = cur_tick;
            }
        }

        if (custom_get_device_type() == DEV_TYPE_SWITCH)
        {
            unsigned char sw_mask = 0;
            unsigned int *get_light_addr = NULL;

            sw_mask = switch_get_down_event_mask();
            if (sw_mask)
            {
                get_light_addr = rt_malloc(8 * sizeof(unsigned int));
                if (get_light_addr != NULL)
                {
                    for (unsigned char index = 0; index < switch_get_count(); index++)
                    {
                        if (sw_mask & (1 << index))
                        {
                            rt_kprintf("switch down index = %d\r\n", index);
                            unsigned int pair_light_count = 0;
                            pair_light_count = custom_get_switch_pair_light_address(index, get_light_addr, 8);
                            for (unsigned int light_index = 0; light_index < pair_light_count; light_index++)
                            {
                                if (manager_get_logic_work_state() == 1)
                                {
                                    custom_switch_ctrl_light(index, get_light_addr[light_index], CTRL_LIGHT_REVERSE);
                                }
                            }
                        }
                    }
                    rt_free(get_light_addr);
                }
            }
        }
    }
}

#endif
