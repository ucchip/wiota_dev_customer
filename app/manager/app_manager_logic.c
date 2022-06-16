#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include <board.h>
#include <string.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "manager_queue.h"
#include "app_manager_logic.h"
#include "manager_module.h"
#include "custom_manager.h"
#include "uc_coding.h"
// #include "uc_cbor.h"
#include "app_manager.h"
#include "app_manager_cfg.h"
#include "app_manager_protocol.h"

static void *manager_queue_handle;
t_app_process_manager process_manager;
static app_ps_header_t g_last_recv_head_info;
static f_custom_data_callback g_custom_data_callback = NULL;
static unsigned int g_send_wiota_data_id = 0;
static unsigned char g_send_wiota_auto_packet_num = 0;
static rt_list_t g_recv_package_list_head;
static unsigned char g_manager_logic_work_state = 0;
//static rt_mutex_t p_manager_send_lock = NULL;
static rt_sem_t p_manager_send_lock = NULL;

void manager_send_lock_init(void)
{
    //p_manager_send_lock = rt_mutex_create("cu_sl", RT_IPC_FLAG_FIFO);
    p_manager_send_lock = rt_sem_create("cu_sl", 1, RT_IPC_FLAG_FIFO);
}

void manager_send_lock_take(void)
{
    if (p_manager_send_lock != NULL)
    {
        //rt_mutex_take(p_manager_send_lock, RT_WAITING_FOREVER);
        rt_sem_take(p_manager_send_lock, RT_WAITING_FOREVER);
    }
}

void manager_send_lock_release(void)
{
    if (p_manager_send_lock != NULL)
    {
        //rt_mutex_release(p_manager_send_lock);
        rt_sem_release(p_manager_send_lock);
    }
}

static void manager_send_data_result_callback(unsigned int data_id, UC_OP_RESULT send_result)
{
    rt_kprintf("manager_send_data_result_callback, data_id = %d, send_result = %d\r\n", data_id, send_result);

    manager_send_lock_release();
}

int app_manager_create_logic_queue(void)
{
    // create wiota app manager queue.
    manager_queue_handle = manager_create_queue("app_manager", 4, 16, UC_SYSTEM_IPC_FLAG_PRIO);
    if (manager_queue_handle == RT_NULL)
    {
        rt_kprintf("manager_create_queue error\n");
        return 1;
    }

    return 0;
}

//int app_manager_logic_sendqueue(int src_task, void *data)
//{
//    return manager_send_page(manager_queue_handle, src_task, data);
//}
void print_hex_data(unsigned char *hex_data, unsigned int hex_data_len)
{
    rt_kprintf("############ hex_data_len = %d:", hex_data_len);
    for (uint32_t index = 0; index < hex_data_len; index++)
    {
        if (index % 16 == 0)
        {
            rt_kprintf("\r\n");
        }
        rt_kprintf("%02X ", hex_data[index]);
    }
    rt_kprintf("\r\n############\r\n");
}

/*
*  Other module send queue data to manager_logic module.
*/
int manager_sendqueue_logic(int src_task, int cmd, void *data)
{
    t_app_logic_message *message = rt_malloc(sizeof(t_app_logic_message));
    if (RT_NULL == message)
    {
        rt_kprintf("wiota_recv_wiota_data malloc error\n");
        return 1;
    }
    message->cmd = cmd;
    message->data = data;

    //app_manager_logic_sendqueue(MANAGER_WIOTA_INDENTIFICATION, message);
    return manager_send_page(manager_queue_handle, src_task, message);
}

void manager_set_custom_data_callback(f_custom_data_callback callback)
{
    g_custom_data_callback = callback;
}

int manager_send_wiota_data(t_send_data_info *send_data_info, unsigned char *data, unsigned int data_len, f_send_result_cb send_result_cb, unsigned int *get_data_id)
{
    unsigned char *output_data = NULL;
    unsigned int output_data_len = 0;
    app_ps_header_t ps_head;

    ps_head.property.is_src_addr = 1;
    if (send_data_info->auto_src_addr)
    {
        ps_head.addr.src_addr = manager_get_userid();
    }
    else
    {
        ps_head.addr.src_addr = send_data_info->src_addr;
    }

    if (send_data_info->dest_addr_type == ADDR_TYPE_SERVER)
    {
        ps_head.property.is_dest_addr = 1;
        ps_head.addr.dest_addr = MANAGE_LOGIC_SERVER_ADDR;
    }
    else if (send_data_info->dest_addr_type == ADDR_TYPE_GW)
    {
        ps_head.property.is_dest_addr = 0;
        ps_head.addr.dest_addr = 0;
    }
    else //if (send_data_info->dest_addr_type == ADDR_TYPE_USER_DEFINED)
    {
        ps_head.property.is_dest_addr = 1;
        ps_head.addr.dest_addr = send_data_info->dest_addr;
    }

    ps_head.property.is_need_res = send_data_info->need_response;
    ps_head.property.response_flag = send_data_info->is_response;
    ps_head.property.compress_flag = send_data_info->compress_flag;
    if (send_data_info->packet_num_type == 1)
    {
        ps_head.property.is_packet_num = 1;
        ps_head.packet_num = g_send_wiota_auto_packet_num;
        if (g_send_wiota_auto_packet_num < 255)
        {
            g_send_wiota_auto_packet_num++;
        }
        else
        {
            g_send_wiota_auto_packet_num = 0;
        }
    }
    else if (send_data_info->packet_num_type == 2)
    {
        ps_head.property.is_packet_num = 1;
        ps_head.packet_num = send_data_info->user_packet_num;
    }
    else
    {
        ps_head.property.is_packet_num = 0;
        ps_head.packet_num = 0;
    }
    ps_head.property.segment_flag = 0;
    ps_head.segment_info.total_num = 0;
    ps_head.segment_info.current_num = 0;
    ps_head.cmd_type = send_data_info->cmd_type;
    rt_kprintf("#### app_data:\r\n");
    print_hex_data(data, data_len);

    if (data_len > SEND_PACKAGE_FRAME_DATA_MAX_LEN)
    {
        t_app_send_package_data *send_package_data = NULL;
        unsigned int data_offset = 0;

        send_package_data = rt_malloc(sizeof(t_app_send_package_data));
        if (send_package_data == NULL)
        {
            return -2;
        }
        ps_head.property.segment_flag = 1;
        ps_head.segment_info.total_num = data_len / SEND_PACKAGE_FRAME_DATA_MAX_LEN;
        if (data_len % SEND_PACKAGE_FRAME_DATA_MAX_LEN)
        {
            ps_head.segment_info.total_num++;
        }
        rt_kprintf("ps_head.segment_info.total_num = %d\r\n", ps_head.segment_info.total_num);
        send_package_data->frame_count = ps_head.segment_info.total_num;
        send_package_data->data = rt_malloc(send_package_data->frame_count * sizeof(unsigned char *));
        if (send_package_data->data == NULL)
        {
            rt_free(send_package_data);
            return -2;
        }
        send_package_data->data_len = rt_malloc(send_package_data->frame_count * sizeof(unsigned int));
        if (send_package_data->data_len == NULL)
        {
            rt_free(send_package_data->data);
            rt_free(send_package_data);
            return -2;
        }
        while (data_offset < data_len)
        {
            unsigned int coding_data_len = SEND_PACKAGE_FRAME_DATA_MAX_LEN;
            if ((data_offset + SEND_PACKAGE_FRAME_DATA_MAX_LEN) > data_len)
            {
                coding_data_len = data_len - data_offset;
            }
            ps_head.segment_info.current_num = data_offset / SEND_PACKAGE_FRAME_DATA_MAX_LEN;
            if (0 != app_data_coding(&ps_head, &data[data_offset], coding_data_len, &output_data, &output_data_len))
            {
                rt_kprintf("%s, %d, coding failed\n", __FUNCTION__, __LINE__);
                if (output_data != NULL)
                {
                    rt_free(output_data);
                }
                for (unsigned char index = 0; index < send_package_data->frame_count; index++)
                {
                    if (send_package_data->data[index] != NULL)
                    {
                        rt_free(send_package_data->data[index]);
                    }
                }
                rt_free(send_package_data->data_len);
                rt_free(send_package_data->data);
                rt_free(send_package_data);
                return -1;
            }
            rt_kprintf("#### manager_send_wiota_data index = %d\r\n", data_offset / SEND_PACKAGE_FRAME_DATA_MAX_LEN);
            print_hex_data(output_data, output_data_len);
            send_package_data->data[data_offset / SEND_PACKAGE_FRAME_DATA_MAX_LEN] = output_data;
            send_package_data->data_len[data_offset / SEND_PACKAGE_FRAME_DATA_MAX_LEN] = output_data_len;
            data_offset += coding_data_len;
        }
        g_send_wiota_data_id++;
        if (g_send_wiota_data_id > 0x7fffffff)
        {
            g_send_wiota_data_id = 0;
        }
        send_package_data->data_id = g_send_wiota_data_id;
        send_package_data->send_result_cb = send_result_cb;

        manager_sendqueue_logic(MANAGER_WIOTA_INDENTIFICATION, MANAGER_LOGIC_SEND_WIOTA_PACKAGE_DATA, send_package_data);
    }
    else
    {
        t_app_send_data *send_data = NULL;

        if (0 != app_data_coding(&ps_head, data, data_len, &output_data, &output_data_len))
        {
            rt_kprintf("%s, %d, coding failed\n", __FUNCTION__, __LINE__);
            if (output_data != NULL)
            {
                rt_free(output_data);
            }
            return -1;
        }
        rt_kprintf("#### manager_send_wiota_data \r\n");
        print_hex_data(output_data, output_data_len);

        send_data = rt_malloc(sizeof(t_app_send_data));
        if (send_data == NULL)
        {
            return -2;
        }
        send_data->data = output_data;
        send_data->data_len = output_data_len;
        g_send_wiota_data_id++;
        if (g_send_wiota_data_id > 0x7fffffff)
        {
            g_send_wiota_data_id = 0;
        }
        send_data->data_id = g_send_wiota_data_id;
        send_data->send_result_cb = send_result_cb;

        manager_sendqueue_logic(MANAGER_WIOTA_INDENTIFICATION, MANAGER_LOGIC_SEND_WIOTA_DATA, send_data);
    }

    if (get_data_id != NULL)
    {
        *get_data_id = g_send_wiota_data_id;
    }

    return 0;
}

static void manager_recv_wiota_data(uc_recv_back_p data)
{
    rt_kprintf("@@@ manager_recv_wiota_data len = %d\r\n", data->data_len);
    uc_recv_back_p data_trans = rt_malloc(sizeof(uc_recv_back_t));
    if (data_trans == NULL)
    {
        rt_kprintf("manager_recv_wiota_data rt_malloc Err!\r\n");
    }
    memcpy(data_trans, data, sizeof(uc_recv_back_t));
    int result = manager_sendqueue_logic(MANAGER_WIOTA_INDENTIFICATION, MANAGER_LOGIC_RECV_WIOTA_DATA, data_trans);
    if (result != 0)
    {
        rt_kprintf("@@@ manager_sendqueue_logic result = %d\r\n", result);
    }
}

static void manager_recv_wiota_state(uc_recv_back_p data)
{
    rt_kprintf("@@@ manager_recv_wiota_state len = %d\r\n", data->data_len);
    uc_recv_back_p data_trans = rt_malloc(sizeof(uc_recv_back_t));
    if (data_trans == NULL)
    {
        rt_kprintf("manager_recv_wiota_state rt_malloc Err!\r\n");
    }
    memcpy(data_trans, data, sizeof(uc_recv_back_t));
    int result = manager_sendqueue_logic(MANAGER_WIOTA_INDENTIFICATION, MANAGER_LOGIC_RECV_WIOTA_STATE, data_trans);
    if (result != 0)
    {
        rt_kprintf("@@@ manager_sendqueue_logic result = %d\r\n", result);
    }
}

void manager_recv_package_list_init(void)
{
    rt_list_init(&g_recv_package_list_head);
}

static unsigned int manager_get_interval_tick(unsigned int last_tick)
{
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

    return interval_tick;
}

static void manager_check_and_delete_timeout_recv_package(void)
{
    struct rt_list_node *node = NULL;
    t_recv_package *package_info = NULL;

    rt_list_for_each(node, &g_recv_package_list_head)
    {
        package_info = rt_list_entry(node, t_recv_package, list);
        if (manager_get_interval_tick(package_info->last_recv_tick) > package_info->recv_timeout)
        {
            rt_list_remove(&(package_info->list));
            
            for (unsigned char index = 0; index < package_info->frame_count; index++)
            {
                rt_free(package_info->frame_data_buf[index]);
            } 
            rt_free(package_info->frame_data_len);
            rt_free(package_info->frame_data_buf);
            rt_free(package_info->frame_recv_mask);
            rt_free(package_info);
        }
    }
}

static unsigned char *manager_check_and_get_recv_package_data(t_recv_data_info *recv_data_info, app_ps_header_t *ps_head, unsigned char *data, unsigned int data_len, unsigned int *get_package_len)
{
    struct rt_list_node *node = NULL;
    t_recv_package *package_info = NULL;
    t_recv_package *new_package_info = NULL;
    unsigned char is_match = 0;

    rt_list_for_each(node, &g_recv_package_list_head)
    {
        package_info = rt_list_entry(node, t_recv_package, list);
        if ((memcmp(&(package_info->recv_info), recv_data_info, sizeof(t_recv_data_info)) == 0)
            && (package_info->packet_num == ps_head->packet_num)
            && (package_info->frame_count == ps_head->segment_info.total_num))
        {
            is_match = 1;
            break;
        }
    }

    if (is_match)
    {
        unsigned char offset = ps_head->segment_info.current_num >> 3;
        unsigned char left = ps_head->segment_info.current_num & 0x07;

        if ((ps_head->segment_info.current_num < package_info->frame_count)
            && ((package_info->frame_recv_mask[offset] & (1 << left)) == 0))
        {
            package_info->frame_data_buf[ps_head->segment_info.current_num] = rt_malloc(data_len);
            if (package_info->frame_data_buf[ps_head->segment_info.current_num] != NULL)
            {
                memcpy(package_info->frame_data_buf[ps_head->segment_info.current_num], data, data_len);
                package_info->frame_data_len[ps_head->segment_info.current_num] = data_len;
                package_info->frame_recv_mask[offset] |= (1 << left);
                package_info->package_data_len += data_len;
                //check data over
                unsigned char index = 0;
                for (index = 0; index < package_info->frame_count; index++)
                {
                    offset = index >> 3;
                    left = index & 0x07;
                    if ((package_info->frame_recv_mask[offset] & (1 << left)) == 0)
                    {
                        break;
                    }
                }
                if (index >= package_info->frame_count)
                {
                    rt_list_remove(&(package_info->list));

                    unsigned char *package_data = rt_malloc(package_info->package_data_len);
                    if (package_data != NULL)
                    {
                        unsigned int data_offset = 0;
                        for (index = 0; index < package_info->frame_count; index++)
                        {
                            memcpy(&package_data[data_offset], package_info->frame_data_buf[index], package_info->frame_data_len[index]);
                            data_offset += package_info->frame_data_len[index];
                            rt_free(package_info->frame_data_buf[index]);
                        }
                        rt_free(package_info->frame_data_len);
                        rt_free(package_info->frame_data_buf);
                        rt_free(package_info->frame_recv_mask);
                        rt_free(package_info);
                        *get_package_len = data_offset;

                        return package_data;
                    }
                }
            }
        }
        package_info->last_recv_tick = rt_tick_get();
    }
    else if ((ps_head->segment_info.current_num == 0)
        && (ps_head->segment_info.total_num > 1))
    {
        new_package_info = rt_malloc(sizeof(t_recv_package));
        if (new_package_info != NULL)
        {
            unsigned char num = ps_head->segment_info.total_num >> 3;

            if (ps_head->segment_info.current_num & 0x07)
            {
                num += 1;
            }
            new_package_info->frame_recv_mask = rt_malloc(num * sizeof(unsigned char *));
            new_package_info->frame_data_buf = rt_malloc(ps_head->segment_info.total_num * sizeof(unsigned char *));
            new_package_info->frame_data_len = rt_malloc(ps_head->segment_info.total_num * sizeof(unsigned int));
            if ((new_package_info->frame_recv_mask == NULL)
                || (new_package_info->frame_data_buf == NULL)
                || (new_package_info->frame_data_len == NULL))
            {
                goto __end;
            }
            memset(new_package_info->frame_recv_mask, 0x00, num * sizeof(unsigned char *));
            memset(new_package_info->frame_data_buf, 0x00, ps_head->segment_info.total_num * sizeof(unsigned char *));
            memset(new_package_info->frame_data_len, 0x00, ps_head->segment_info.total_num * sizeof(unsigned int));
            new_package_info->packet_num = ps_head->packet_num;
            new_package_info->frame_count = ps_head->segment_info.total_num;
            memcpy(&new_package_info->recv_info, recv_data_info, sizeof(t_recv_data_info));
            new_package_info->package_data_len = 0;
            new_package_info->recv_timeout = RECV_PACKAGE_TIMEOUT;
            
            new_package_info->frame_data_buf[0] = rt_malloc(data_len);
            if (new_package_info->frame_data_buf[0] != NULL)
            {
                memcpy(new_package_info->frame_data_buf[0], data, data_len);
                new_package_info->frame_data_len[0] = data_len;
                new_package_info->frame_recv_mask[0] |= (1 << 0);
                new_package_info->package_data_len += data_len;
            }
            else
            {
                goto __end;
            }
            new_package_info->last_recv_tick = rt_tick_get();

            rt_list_insert_after(node, &(new_package_info->list));
        }
    }

__end:
    if (new_package_info != NULL)
    {
        if (new_package_info->frame_recv_mask != NULL)
        {
            rt_free(new_package_info->frame_recv_mask);
        }
        if (new_package_info->frame_data_buf != NULL)
        {
            rt_free(new_package_info->frame_data_buf);
        }
        if (new_package_info->frame_data_len != NULL)
        {
            rt_free(new_package_info->frame_data_len);
        }
        rt_free(new_package_info);
    }

    return NULL;
}

static void manager_handle_app_msg(unsigned char wiota_msg_type, app_ps_header_t *ps_head, unsigned char *data, unsigned int data_len)
{
    t_recv_data_info recv_data_info;
    unsigned char *package_data = NULL;
    unsigned int package_len = 0;

    rt_kprintf("manager_handle_app_msg data %p len %d\n", data, data_len);

    if (ps_head->property.is_src_addr == 0)
    {
        recv_data_info.src_addr_type = ADDR_TYPE_GW;
    }
    else if (ps_head->addr.src_addr == MANAGE_LOGIC_SERVER_ADDR)
    {
        recv_data_info.src_addr_type = ADDR_TYPE_SERVER;
    }
    else
    {
        recv_data_info.src_addr_type = ADDR_TYPE_USER_DEFINED;
    }
    if (ps_head->property.is_dest_addr == 0)
    {
        if ((wiota_msg_type == UC_RECV_BC) || (wiota_msg_type == UC_RECV_OTA))
        {
            recv_data_info.trans_type = TRANS_TYPE_BROADCAST;
        }
        else
        {
            recv_data_info.trans_type = TRANS_TYPE_UNICAST;
        }
    }
    else if (ps_head->addr.dest_addr == MANAGE_LOGIC_BROADCAST_ADDR)
    {
        recv_data_info.trans_type = TRANS_TYPE_BROADCAST;
    }
    else if ((ps_head->addr.dest_addr >= MANAGE_LOGIC_MULTICAST_ADDR_START)
            && (ps_head->addr.dest_addr <= MANAGE_LOGIC_MULTICAST_ADDR_END))
    {
        if (ps_head->addr.dest_addr != manager_get_multicast_addr())
        {
            //multicast addr unmatch
            return;
        }
        recv_data_info.trans_type = TRANS_TYPE_MULTICAST;
    }
    else
    {
        if (ps_head->addr.dest_addr != manager_get_userid())
        {
            //unicast device addr unmatch
            return;
        }
        recv_data_info.trans_type = TRANS_TYPE_UNICAST;
    }
    recv_data_info.need_response = ps_head->property.is_need_res;
    recv_data_info.is_response = ps_head->property.response_flag;
    recv_data_info.compress_flag = ps_head->property.compress_flag;
    recv_data_info.is_packet_num = ps_head->property.is_packet_num;
    recv_data_info.packet_num = ps_head->packet_num;
    recv_data_info.src_addr = ps_head->addr.src_addr;
    recv_data_info.dest_addr = ps_head->addr.dest_addr;
    recv_data_info.cmd_type = ps_head->cmd_type;

    // chcek if the repeat msg
    if (ps_head->property.is_packet_num)
    {
        if (memcmp(&g_last_recv_head_info, ps_head, sizeof(g_last_recv_head_info)) != 0)
        {
            memcpy(&g_last_recv_head_info, ps_head, sizeof(g_last_recv_head_info));
        }
        else
        {
            return;
        }
    }

    //chcek segment data
    if (ps_head->property.segment_flag)
    {
        if (ps_head->property.is_packet_num == 0)
        {
            return;
        }
        package_data = manager_check_and_get_recv_package_data(&recv_data_info, ps_head, data, data_len, &package_len);
        if (package_data != NULL)
        {
            data = package_data;
            data_len = package_len;
        }
        else
        {
            return;
        }
    }

    if ((ps_head->cmd_type >= MANAGE_LOGIC_SYSTEM_CMD_TYPE_START)
        && (ps_head->cmd_type <= MANAGE_LOGIC_SYSTEM_CMD_TYPE_END))
    {
        //process system manage data
        manager_system_recv_data_process(&recv_data_info, data, data_len);
    }
    else
    {
        //custom data
        if (g_custom_data_callback != NULL)
        {
            g_custom_data_callback(&recv_data_info, data, data_len);
        }
    }

    if (package_data != NULL)
    {
        rt_free(package_data);
    }

    return;
}

static void app_manager_handle_recv_data(uc_recv_back_p recv_data)
{
    app_ps_header_t ps_head = {0};
    unsigned char *temp_data = RT_NULL;
    unsigned int temp_data_len = 0;

    rt_kprintf("app manager handle recv %d %d\n", recv_data->result, recv_data->type);

    if (UC_OP_SUCC == recv_data->result)
    {

        switch (recv_data->type)
        {
        case UC_RECV_MSG:
        case UC_RECV_BC:
            rt_kprintf("#### app_manager_handle_recv_data: type = %d\r\n", recv_data->type);
            print_hex_data(recv_data->data, recv_data->data_len);
            if (0 != app_data_decoding(recv_data->data, recv_data->data_len, &temp_data, &temp_data_len, &ps_head))
            {
                rt_kprintf("%s, %d decoding failed\n", __FUNCTION__, __LINE__);
                rt_free(recv_data->data);
                return;
            }
            rt_free(recv_data->data);

            rt_kprintf("#### recv_data app_data:\r\n");
            print_hex_data(temp_data, temp_data_len);

            manager_handle_app_msg(recv_data->type, &ps_head, temp_data, temp_data_len);
            if (temp_data)
            {
                rt_free(temp_data);
            }

            break;

        case UC_RECV_OTA:

            break;

        default:
            break;
        }
    }
}

void manager_request_multicast_addr(void)
{
    t_send_data_info send_data_info;
    unsigned int get_data_id = 0;
    unsigned char *app_data = NULL;
    int result = 0;

    app_data = manager_create_multicast_addr_request_data(manager_get_device_type_name());
    if (app_data == NULL)
    {
        goto __end;
    }
    send_data_info.auto_src_addr = 1;
    send_data_info.dest_addr_type = ADDR_TYPE_SERVER;
    send_data_info.need_response = 0;
    send_data_info.is_response = 0;
    send_data_info.compress_flag = 1;
    send_data_info.packet_num_type = 0;
    send_data_info.src_addr = 0;
    send_data_info.dest_addr = 0;
    send_data_info.cmd_type = APP_CMD_IOTE_MUTICAST_REQUEST;
    manager_send_lock_take();
    result = manager_send_wiota_data(&send_data_info, app_data, strlen((const char *)app_data), manager_send_data_result_callback, &get_data_id);
    if (result != 0)
    {
        manager_send_lock_release();
    }
    rt_kprintf("manager_request_multicast_addr result = %d, get_data_id = %d\r\n", result, get_data_id);

__end:
    if (app_data != NULL)
    {
        manager_delete_multicast_addr_request_data(app_data);
    }
}

void manager_request_config(void)
{
    t_send_data_info send_data_info;
    unsigned int get_data_id = 0;
    unsigned char *app_data = NULL;
    t_wiota_net_config *wiota_config = NULL;
    int result = 0;

    wiota_config = rt_malloc(sizeof(t_wiota_net_config));
    if (wiota_config == NULL)
    {
        goto __end;
    }
    manager_get_wiota_config(wiota_config);
    app_data = manager_create_config_data(wiota_config);
    if (app_data == NULL)
    {
        goto __end;
    }
    send_data_info.auto_src_addr = 1;
    send_data_info.dest_addr_type = ADDR_TYPE_SERVER;
    send_data_info.need_response = 0;
    send_data_info.is_response = 0;
    send_data_info.compress_flag = 1;
    send_data_info.packet_num_type = 0;
    send_data_info.src_addr = 0;
    send_data_info.dest_addr = 0;
    send_data_info.cmd_type = APP_CMD_IOTE_REQUEST_NEW_CONFIG;
    manager_send_lock_take();
    result = manager_send_wiota_data(&send_data_info, app_data, strlen((const char *)app_data), manager_send_data_result_callback, &get_data_id);
    if (result != 0)
    {
        manager_send_lock_release();
    }
    rt_kprintf("manager_request_config result = %d, get_data_id = %d\r\n", result, get_data_id);

__end:
    if (wiota_config != NULL)
    {
        rt_free(wiota_config);
    }
    if (app_data != NULL)
    {
        manager_delete_config_data(app_data);
    }
}

unsigned char manager_get_logic_work_state(void)
{
    return g_manager_logic_work_state;
}

void app_manager_logic(void)
{
    static unsigned char first_conn = 1;
    /*
    relationship between page and message:
        typedef struct app_manager_message
        {
            int src_task;
            void *message; -----------------------------> typedef struct app_logic_message
                                                                                         {
                                                                                           int cmd;
                                                                                           void *data;  -------------------------------> diffrent command have different data type.
                                                                                         }t_app_logic_message;
        }t_app_manager_message;
    */

    rt_kprintf("app manager logic\n");

    // set callback function of wiota
    uc_wiota_register_recv_data_callback(manager_recv_wiota_data, UC_CALLBACK_NORAMAL_MSG);
    uc_wiota_register_recv_data_callback(manager_recv_wiota_state, UC_CALLBACK_STATE_INFO);

    process_manager.current_process = MANAGER_LOGIC_DEFAULT;

    g_manager_logic_work_state = 1;
    if (first_conn == 0)
    {
        manager_sendqueue_custom(MANAGER_LOGIC_INDENTIFICATION, CUSTOM_LOGIC_REPORT_DEVSTATE, NULL);
    }
    else
    {
        first_conn = 0;
    }
    while (1)
    {
        t_app_manager_message *page;
        t_app_logic_message *message;
        UC_WIOTA_STATUS wiota_status = 0;

        manager_check_and_delete_timeout_recv_package();

        // check wiota connect state
        wiota_status = uc_wiota_get_state() ;
        if ((wiota_status == UC_STATUS_SYNC_LOST) || (wiota_status == UC_STATUS_ERROR))
        {
            rt_kprintf("uc_wiota_get_state Err!! wiota_status = %d\r\n", wiota_status);
            break;
        }

        if (QUEUE_EOK == manager_recv_queue(manager_queue_handle, (void **)&page, MANAGE_LOGIC_RECV_QUEUE_TIMEOUT))
        {
            message = page->message;

            rt_kprintf("app_manager_logic message->cmd %d\n", message->cmd);

            switch (message->cmd)
            {
            case MANAGER_LOGIC_RECV_WIOTA_DATA:
            {
                // handle recv data
                app_manager_handle_recv_data((uc_recv_back_p)(message->data));
                break;
            }
            case MANAGER_LOGIC_RECV_WIOTA_STATE:
            {

                break;
            }
            case MANAGER_LOGIC_SEND_WIOTA_DATA:
            {
                t_app_send_data *app_send_data = (t_app_send_data *)(message->data);
                if (app_send_data == NULL)
                {
                    rt_kprintf("logic send msg error\n");
                    break;
                }
                // send data to wiota
                if (app_send_data->data != NULL && app_send_data->data_len != 0)
                {
                    UC_OP_RESULT send_result;
                    rt_kprintf("uc_wiota_send_data 1 \r\n");
                    for (unsigned char num = 0; num < MANAGER_WIOTA_SEND_TRY_COUNT; num++)
                    {
                        send_result = uc_wiota_send_data(app_send_data->data, app_send_data->data_len, 10000, NULL);
                        if (send_result == UC_OP_SUCC)
                        {
                            break;
                        }
                    }
                    if (app_send_data->send_result_cb != NULL)
                    {
                        app_send_data->send_result_cb(app_send_data->data_id, send_result);
                    }
                    rt_kprintf("uc_wiota_send_data 1 data_id = %d, send_result = %d\r\n", app_send_data->data_id, send_result);
                }
                else
                {
                    rt_kprintf("logic send data error\n");
                }
                if (app_send_data->data != NULL)
                {
                    rt_free(app_send_data->data);
                }
                break;
            }
            case MANAGER_LOGIC_SEND_WIOTA_PACKAGE_DATA:
            {
                unsigned char index = 0;
                t_app_send_package_data *app_send_package_data = (t_app_send_package_data *)(message->data);
                UC_OP_RESULT send_result = UC_OP_SUCC;

                if (app_send_package_data == NULL)
                {
                    rt_kprintf("logic send msg error\n");
                    break;
                }
                // send data to wiota
                rt_kprintf("uc_wiota_send_data 2 \r\n");
                for (index = 0; index < app_send_package_data->frame_count; index++)
                {
                    for (unsigned char num = 0; num < MANAGER_WIOTA_SEND_TRY_COUNT; num++)
                    {
                        send_result = uc_wiota_send_data(app_send_package_data->data[index], app_send_package_data->data_len[index], 10000, NULL);
                        if (send_result == UC_OP_SUCC)
                        {
                            break;
                        }
                    }
                    if (send_result != UC_OP_SUCC)
                    {
                        break;
                    }
                }
                if (app_send_package_data->send_result_cb != NULL)
                {
                    app_send_package_data->send_result_cb(app_send_package_data->data_id, send_result);
                }
                rt_kprintf("uc_wiota_send_data 2 data_id = %d, send_result = %d\r\n", app_send_package_data->data_id, send_result);
                for (index = 0; index < app_send_package_data->frame_count; index++)
                {
                    rt_free(app_send_package_data->data[index]);
                }
                rt_free(app_send_package_data->data);
                rt_free(app_send_package_data->data_len);
                break;
            }
            default:
            {
                break;
            }
            }

            if (RT_NULL != message->data)
                rt_free(message->data);
            if (RT_NULL != message)
                rt_free(message);
            rt_free(page);  
        }
    }
    g_manager_logic_work_state = 0;
}

#endif
