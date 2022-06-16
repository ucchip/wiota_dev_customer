#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include <board.h>

#include <string.h>
#include "cJSON.h"
#include "app_manager_logic.h"
#include "app_manager_cfg.h"
#include "custom_manager.h"
#include "custom_protocol.h"
#include "custom_pair.h"
#include "light_ctrl.h"
#include "switch_ctrl.h"
#include "custom_data.h"
#include "manager_module.h"


int custom_parse_pair_info_cmd(unsigned char *data, unsigned int data_len, pair_info_t *get_pair_info, unsigned int pair_info_max_count, unsigned char *get_pair_info_count)
{
    int result = 0;
    cJSON *root = NULL;
    cJSON *array = NULL;
    cJSON *item = NULL;
    unsigned int array_count = 0;

    root = cJSON_Parse((const char *)data);
    if (root == NULL)
    {
        result = -1;
        goto __end;
    }

    array = cJSON_GetObjectItem(root, "device_info");
    if (array == NULL)
    {
        result = -1;
        goto __end;
    }
    else if (array->type != cJSON_Array)
    {
        result = -2;
        goto __end;
    }
    array_count = cJSON_GetArraySize(array);
    rt_kprintf("custom_parse_pair_info_cmd array_count = %d \r\n", array_count);
    if ((array_count == 0) || (array_count > pair_info_max_count))
    {
        result = -3;
        goto __end;
    }

    for (unsigned int index = 0; index < array_count; index++)
    {
        cJSON *array_item = cJSON_GetArrayItem(array, index);
        if ((array_item != NULL) && (array_item->type == cJSON_Object))
        {
            item = cJSON_GetObjectItem(array_item, "dest_address");
            if (item == NULL)
            {
                result = -1;
                goto __end;
            }
            else if (item->type != cJSON_Number)
            {
                result = -2;
                goto __end;
            }
            get_pair_info[index].address = item->valuedouble;
    		rt_kprintf("custom_parse_pair_info_cmd address = 0x%08x\r\n", get_pair_info[index].address);

            item = cJSON_GetObjectItem(array_item, "type");
            if (item == NULL)
            {
                result = -1;
                goto __end;
            }
            else if (item->type != cJSON_String)
            {
                result = -2;
                goto __end;
            }
            if (memcmp(item->valuestring, "light", strlen("light")) == 0)
            {
                get_pair_info[index].dev_type = DEV_TYPE_LIGHT;
            }
            else if (memcmp(item->valuestring, "switch", strlen("switch")) == 0)
            {
                get_pair_info[index].dev_type = DEV_TYPE_SWITCH;
            }
            else
            {
                get_pair_info[index].dev_type = 0;
            }
    		rt_kprintf("custom_parse_pair_info_cmd dev_type = %d\r\n", get_pair_info[index].dev_type);

            item = cJSON_GetObjectItem(array_item, "index");
            if (item != NULL)
            {
                if (item->type != cJSON_Number)
                {
                    result = -2;
                    goto __end;
                }
                get_pair_info[index].index = item->valueint - 1;
            }
            else
            {
                get_pair_info[index].index = 0xff;
            }
    		rt_kprintf("custom_parse_pair_info_cmd index = %d\r\n", get_pair_info[index].index);
        }
        else
        {
            result = -1;
            goto __end;
        }
    }
    if (get_pair_info_count != NULL)
    {
        *get_pair_info_count = array_count;
    }

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return result;
}

unsigned char *custom_create_light_property_data(void)
{
    unsigned char *json_data = NULL;
    cJSON *root = NULL;

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        goto __end;
    }
    cJSON_AddItemToObject(root, "type", cJSON_CreateString("light"));

    json_data = (unsigned char *)cJSON_Print((const cJSON *)root);

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return json_data;
}

void custom_delete_light_property_data(unsigned char *data)
{
    rt_free(data);
}

unsigned char *custom_create_light_state_data(e_light_state *state)
{
    unsigned char *json_data = NULL;
    cJSON *root = NULL;

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        goto __end;
    }
    cJSON_AddItemToObject(root, "type", cJSON_CreateString("light"));
    if (*state == LIGHT_ON)
    {
        cJSON_AddItemToObject(root, "state", cJSON_CreateString("on"));
    }
    else
    {
        cJSON_AddItemToObject(root, "state", cJSON_CreateString("off"));
    }

    json_data = (unsigned char *)cJSON_Print((const cJSON *)root);

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return json_data;
}

void custom_delete_light_state_data(unsigned char *data)
{
    rt_free(data);
}

unsigned char *custom_create_light_pair_data(void)
{
    unsigned char *json_data = NULL;
    cJSON *root = NULL;

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        goto __end;
    }
    cJSON_AddItemToObject(root, "type", cJSON_CreateString("light"));

    json_data = (unsigned char *)cJSON_Print((const cJSON *)root);

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return json_data;
}

void custom_delete_light_pair_data(unsigned char *data)
{
    rt_free(data);
}

int custom_parse_light_ctrl_cmd(unsigned char *data, unsigned int data_len, unsigned char *get_type, unsigned char *get_index, unsigned char *get_ctrl_action)
{
    int result = 0;
    cJSON *root = NULL;
    cJSON *item = NULL;

    root = cJSON_Parse((const char *)data);
    if (root == NULL)
    {
        result = -1;
        goto __end;
    }

    item = cJSON_GetObjectItem(root, "type");
    if (item == NULL)
    {
        result = -2;
        goto __end;
    }
    else if (item->type != cJSON_String)
    {
        result = -3;
        goto __end;
    }
    if (get_type != NULL)
    {
        if (memcmp(item->valuestring, "light", strlen("light")) == 0)
        {
            *get_type = DEV_TYPE_LIGHT;
        }
        else if (memcmp(item->valuestring, "switch", strlen("switch")) == 0)
        {
            *get_type = DEV_TYPE_SWITCH;
        }
        else if (memcmp(item->valuestring, "ap", strlen("ap")) == 0)
        {
            *get_type = DEV_TYPE_AP;
        }
        else if (memcmp(item->valuestring, "server", strlen("server")) == 0)
        {
            *get_type = DEV_TYPE_SERVER;
        }
        else
        {
            *get_type = 0;
        }
    }

    item = cJSON_GetObjectItem(root, "index");
    if (item != NULL)
    {
        if (item->type != cJSON_Number)
        {
            result = -3;
            goto __end;
        }
        if (get_index != NULL)
        {
            *get_index = item->valueint - 1;
        }
    }

    unsigned char ctrl_action = 0;
    item = cJSON_GetObjectItem(root, "control");
    if (item == NULL)
    {
        result = -2;
        goto __end;
    }
    else if (item->type != cJSON_String)
    {
        result = -3;
        goto __end;
    }
    else if (strcmp(item->valuestring, "reverse") == 0)
    {
        ctrl_action = CTRL_LIGHT_REVERSE;
    }
    else if (strcmp(item->valuestring, "on") == 0)
    {
        ctrl_action = CTRL_LIGHT_ON;
    }
    else if (strcmp(item->valuestring, "off") == 0)
    {
        ctrl_action = CTRL_LIGHT_OFF;
    }
    else
    {
        result = -4;
        goto __end;
    }
    if (get_ctrl_action != NULL)
    {
        *get_ctrl_action = ctrl_action;
    }

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return result;
}

void custom_light_recv_data_process(t_recv_data_info *recv_data_info, unsigned char *data, unsigned int data_len)
{
    switch (recv_data_info->cmd_type)
    {
    case APP_CMD_PROPERTY_REPORT:
        if (recv_data_info->is_response == 0)
        {
            if (recv_data_info->need_response == 1)
            {    
                t_send_data_info respond_data_info;
                unsigned char *app_data = NULL;

                respond_data_info.auto_src_addr = 1;
                respond_data_info.dest_addr_type = recv_data_info->src_addr_type;
                respond_data_info.need_response = 0;
                respond_data_info.is_response = 1;
                respond_data_info.compress_flag = 1;
                if (recv_data_info->is_packet_num)
                {
                    respond_data_info.packet_num_type = 2;
                    respond_data_info.user_packet_num = recv_data_info->packet_num;
                }
                else
                {
                    respond_data_info.packet_num_type = 0;
                    respond_data_info.user_packet_num = 0;
                }
                respond_data_info.src_addr = 0;
                respond_data_info.dest_addr = recv_data_info->src_addr;
                respond_data_info.cmd_type = recv_data_info->cmd_type;

                app_data = custom_create_light_property_data();
                if (app_data != NULL)
                {
                    (void)manager_send_wiota_data(&respond_data_info, app_data, strlen((const char *)app_data), NULL, NULL);
                    custom_delete_light_property_data(app_data);
                }
            }
        }
        break;

    case APP_CMD_IOTE_STATE_REPORT:
        if (recv_data_info->is_response == 0)
        {
            if (recv_data_info->need_response == 1)
            {    
                t_send_data_info respond_data_info;
                unsigned char *app_data = NULL;
                e_light_state state;

                respond_data_info.auto_src_addr = 1;
                respond_data_info.dest_addr_type = recv_data_info->src_addr_type;
                respond_data_info.need_response = 0;
                respond_data_info.is_response = 1;
                respond_data_info.compress_flag = 1;
                if (recv_data_info->is_packet_num)
                {
                    respond_data_info.packet_num_type = 2;
                    respond_data_info.user_packet_num = recv_data_info->packet_num;
                }
                else
                {
                    respond_data_info.packet_num_type = 0;
                    respond_data_info.user_packet_num = 0;
                }
                respond_data_info.src_addr = 0;
                respond_data_info.dest_addr = recv_data_info->src_addr;
                respond_data_info.cmd_type = recv_data_info->cmd_type;

                state = light_ctrl_get_state();
                app_data = custom_create_light_state_data(&state);
                if (app_data != NULL)
                {
                    (void)manager_send_wiota_data(&respond_data_info, app_data, strlen((const char *)app_data), NULL, NULL);
                    custom_delete_light_state_data(app_data);
                }
            }
        }
        break;

    case APP_CMD_IOTE_PAIR_REQUEST:
        if (recv_data_info->is_response == 1)
        {
            //todo
        }
        break;

    case APP_CMD_NET_PAIR_RESPONSE:
        if (recv_data_info->is_response == 0)
        {
            pair_info_t *get_pair_info = NULL;
            unsigned char get_pair_info_count;

            get_pair_info = rt_malloc(8 * sizeof(pair_info_t));
            if (get_pair_info != NULL)
            {
                int parse_result = custom_parse_pair_info_cmd(data, data_len, get_pair_info, 8, &get_pair_info_count);
                rt_kprintf("custom_parse_pair_info_cmd parse_result = %d\r\n", parse_result);
                if (parse_result == 0)
                {
                    unsigned char index = 0;
                    unsigned int self_userid = manager_get_userid();
                    for (index = 0; index < get_pair_info_count; index++)
                    {
                        if ((get_pair_info[index].address == self_userid)
                            && (get_pair_info[index].dev_type == DEV_TYPE_LIGHT))
                        {
                            break;
                        }
                    }
                    if (index < get_pair_info_count)
                    {
                        custom_set_light_pair_info(get_pair_info, get_pair_info_count);
                    }
                }
                rt_free(get_pair_info);
            }
            if (recv_data_info->need_response == 1)
            {
                t_send_data_info respond_data_info;
                respond_data_info.auto_src_addr = 1;
                respond_data_info.dest_addr_type = recv_data_info->src_addr_type;
                respond_data_info.need_response = 0;
                respond_data_info.is_response = 1;
                respond_data_info.compress_flag = 0;
                if (recv_data_info->is_packet_num)
                {
                    respond_data_info.packet_num_type = 2;
                    respond_data_info.user_packet_num = recv_data_info->packet_num;
                }
                else
                {
                    respond_data_info.packet_num_type = 0;
                    respond_data_info.user_packet_num = 0;
                }
                respond_data_info.src_addr = 0;
                respond_data_info.dest_addr = recv_data_info->src_addr;
                respond_data_info.cmd_type = recv_data_info->cmd_type;
                (void)manager_send_wiota_data(&respond_data_info, NULL, 0, NULL, NULL);
            }
        }
        break;

    case APP_CMD_IOTE_PAIR_REQUEST_CANCEL:
        if (recv_data_info->is_response == 1)
        {
            //todo
        }
        break;

    case APP_CMD_NET_PAIR_CANCEL:
        if (recv_data_info->is_response == 0)
        {
            pair_info_t *get_pair_info = NULL;
            unsigned char get_pair_info_count;

            get_pair_info = rt_malloc(8 * sizeof(pair_info_t));
            if (get_pair_info != NULL)
            {
                int parse_result = custom_parse_pair_info_cmd(data, data_len, get_pair_info, 8, &get_pair_info_count);
                if (parse_result == 0)
                {
                    unsigned char index = 0;
                    unsigned int self_userid = manager_get_userid();
                    for (index = 0; index < get_pair_info_count; index++)
                    {
                        if ((get_pair_info[index].address == self_userid)
                            && (get_pair_info[index].dev_type == DEV_TYPE_LIGHT))
                        {
                            break;
                        }
                    }
                    if (index < get_pair_info_count)
                    {
                        custom_clear_light_pair_info();
                    }
                }
                rt_free(get_pair_info);
            }
            if (recv_data_info->need_response == 1)
            {
                t_send_data_info respond_data_info;
                respond_data_info.auto_src_addr = 1;
                respond_data_info.dest_addr_type = recv_data_info->src_addr_type;
                respond_data_info.need_response = 0;
                respond_data_info.is_response = 1;
                respond_data_info.compress_flag = 0;
                if (recv_data_info->is_packet_num)
                {
                    respond_data_info.packet_num_type = 2;
                    respond_data_info.user_packet_num = recv_data_info->packet_num;
                }
                else
                {
                    respond_data_info.packet_num_type = 0;
                    respond_data_info.user_packet_num = 0;
                }
                respond_data_info.src_addr = 0;
                respond_data_info.dest_addr = recv_data_info->src_addr;
                respond_data_info.cmd_type = recv_data_info->cmd_type;
                (void)manager_send_wiota_data(&respond_data_info, NULL, 0, NULL, NULL);
            }
        }
        break;

    case APP_CMD_NET_CTRL_IOTE:
        if (recv_data_info->is_response == 0)
        {
            pair_info_t ctrl_src_info;
            unsigned char ctrl_action = 0;
            unsigned char check_pass = 0;

            ctrl_src_info.address = recv_data_info->src_addr;
            int parse_result = custom_parse_light_ctrl_cmd(data, data_len, &ctrl_src_info.dev_type, &ctrl_src_info.index, &ctrl_action);
            rt_kprintf("custom_parse_light_ctrl_cmd parse_result = %d\r\n", parse_result);
            if (parse_result == 0)
            {
                rt_kprintf("dev_type = %d\r\n", ctrl_src_info.dev_type);
                rt_kprintf("index = %d\r\n", ctrl_src_info.index);
                rt_kprintf("ctrl_action = %d\r\n", ctrl_action);
                //if ((recv_data_info->src_addr_type == ADDR_TYPE_SERVER) && (ctrl_src_info.dev_type == DEV_TYPE_SERVER))
                if (ctrl_src_info.dev_type == DEV_TYPE_SERVER)
                {
                    check_pass = 1;
                }
                else if (custom_check_light_pair_info(&ctrl_src_info) == 0)
                {
                    check_pass = 1;
                }
                if (check_pass)
                {
                    light_ctrl_output(ctrl_action);
                    manager_sendqueue_custom(MANAGER_LOGIC_INDENTIFICATION, CUSTOM_LOGIC_REPORT_DEVSTATE, NULL);
                }
            }
            if (recv_data_info->need_response == 1)
            {
                t_send_data_info respond_data_info;
                respond_data_info.auto_src_addr = 1;
                respond_data_info.dest_addr_type = recv_data_info->src_addr_type;
                respond_data_info.need_response = 0;
                respond_data_info.is_response = 1;
                respond_data_info.compress_flag = 0;
                if (recv_data_info->is_packet_num)
                {
                    respond_data_info.packet_num_type = 2;
                    respond_data_info.user_packet_num = recv_data_info->packet_num;
                }
                else
                {
                    respond_data_info.packet_num_type = 0;
                    respond_data_info.user_packet_num = 0;
                }
                respond_data_info.src_addr = 0;
                respond_data_info.dest_addr = recv_data_info->src_addr;
                respond_data_info.cmd_type = recv_data_info->cmd_type;
                (void)manager_send_wiota_data(&respond_data_info, NULL, 0, NULL, NULL);
            }
        }
        break;

    default:
        break;
    }
}

unsigned char *custom_create_switch_property_data(unsigned char sw_count)
{
    unsigned char *json_data = NULL;
    cJSON *root = NULL;

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        goto __end;
    }
    cJSON_AddItemToObject(root, "type", cJSON_CreateString("switch"));
    cJSON_AddItemToObject(root, "conter", cJSON_CreateNumber(sw_count));

    json_data = (unsigned char *)cJSON_Print((const cJSON *)root);

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return json_data;
}

void custom_delete_switch_property_data(unsigned char *data)
{
    rt_free(data);
}

unsigned char *custom_create_switch_state_data(unsigned char sw_state_mask)
{
    unsigned char *json_data = NULL;
    cJSON *root = NULL;

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        goto __end;
    }
    cJSON_AddItemToObject(root, "type", cJSON_CreateString("switch"));
    cJSON_AddItemToObject(root, "state", cJSON_CreateNumber(sw_state_mask));

    json_data = (unsigned char *)cJSON_Print((const cJSON *)root);

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return json_data;
}

void custom_delete_switch_state_data(unsigned char *data)
{
    rt_free(data);
}

unsigned char *custom_create_switch_pair_data(unsigned char sw_index)
{
    unsigned char *json_data = NULL;
    cJSON *root = NULL;

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        goto __end;
    }
    cJSON_AddItemToObject(root, "type", cJSON_CreateString("switch"));
    cJSON_AddItemToObject(root, "index", cJSON_CreateNumber(sw_index+1));

    json_data = (unsigned char *)cJSON_Print((const cJSON *)root);

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return json_data;
}

void custom_delete_switch_pair_data(unsigned char *data)
{
    rt_free(data);
}

unsigned char *custom_create_switch_ctrl_light_data(unsigned char sw_index, unsigned char ctrl_action)
{
    unsigned char *json_data = NULL;
    cJSON *root = NULL;

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        goto __end;
    }
    cJSON_AddItemToObject(root, "type", cJSON_CreateString("switch"));
    cJSON_AddItemToObject(root, "index", cJSON_CreateNumber(sw_index+1));
    if (ctrl_action == CTRL_LIGHT_REVERSE)
    {
        cJSON_AddItemToObject(root, "control", cJSON_CreateString("reverse"));
    }
    else if (ctrl_action == CTRL_LIGHT_ON)
    {
        cJSON_AddItemToObject(root, "control", cJSON_CreateString("on"));
    }
    else //if (ctrl_action == CTRL_LIGHT_OFF)
    {
        cJSON_AddItemToObject(root, "control", cJSON_CreateString("off"));
    }

    json_data = (unsigned char *)cJSON_Print((const cJSON *)root);

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return json_data;
}

void custom_delete_switch_ctrl_light_data(unsigned char *data)
{
    rt_free(data);
}

void custom_switch_recv_data_process(t_recv_data_info *recv_data_info, unsigned char *data, unsigned int data_len)
{
    switch (recv_data_info->cmd_type)
    {
    case APP_CMD_PROPERTY_REPORT:
        if (recv_data_info->is_response == 0)
        {
            if (recv_data_info->need_response == 1)
            {    
                t_send_data_info respond_data_info;
                unsigned char *app_data = NULL;

                respond_data_info.auto_src_addr = 1;
                respond_data_info.dest_addr_type = recv_data_info->src_addr_type;
                respond_data_info.need_response = 0;
                respond_data_info.is_response = 1;
                respond_data_info.compress_flag = 1;
                if (recv_data_info->is_packet_num)
                {
                    respond_data_info.packet_num_type = 2;
                    respond_data_info.user_packet_num = recv_data_info->packet_num;
                }
                else
                {
                    respond_data_info.packet_num_type = 0;
                    respond_data_info.user_packet_num = 0;
                }
                respond_data_info.src_addr = 0;
                respond_data_info.dest_addr = recv_data_info->src_addr;
                respond_data_info.cmd_type = recv_data_info->cmd_type;

                app_data = custom_create_switch_property_data(switch_get_count());
                if (app_data != NULL)
                {
                    (void)manager_send_wiota_data(&respond_data_info, app_data, strlen((const char *)app_data), NULL, NULL);
                    custom_delete_switch_property_data(app_data);
                }
            }
        }
        break;

    case APP_CMD_IOTE_STATE_REPORT:
        if (recv_data_info->is_response == 0)
        {
            if (recv_data_info->need_response == 1)
            {    
                t_send_data_info respond_data_info;
                unsigned char *app_data = NULL;
                unsigned char state = 0;

                respond_data_info.auto_src_addr = 1;
                respond_data_info.dest_addr_type = recv_data_info->src_addr_type;
                respond_data_info.need_response = 0;
                respond_data_info.is_response = 1;
                respond_data_info.compress_flag = 1;
                if (recv_data_info->is_packet_num)
                {
                    respond_data_info.packet_num_type = 2;
                    respond_data_info.user_packet_num = recv_data_info->packet_num;
                }
                else
                {
                    respond_data_info.packet_num_type = 0;
                    respond_data_info.user_packet_num = 0;
                }
                respond_data_info.src_addr = 0;
                respond_data_info.dest_addr = recv_data_info->src_addr;
                respond_data_info.cmd_type = recv_data_info->cmd_type;

                switch_get_all_state(&state);
                app_data = custom_create_switch_state_data(state);
                if (app_data != NULL)
                {
                    (void)manager_send_wiota_data(&respond_data_info, app_data, strlen((const char *)app_data), NULL, NULL);
                    custom_delete_switch_state_data(app_data);
                }
            }
        }
        break;

    case APP_CMD_IOTE_PAIR_REQUEST:
        if (recv_data_info->is_response == 1)
        {
            //todo
        }
        break;

    case APP_CMD_NET_PAIR_RESPONSE:
        if (recv_data_info->is_response == 0)
        {
            pair_info_t *get_pair_info = NULL;
            unsigned char get_pair_info_count;

            get_pair_info = rt_malloc(8 * sizeof(pair_info_t));
            if (get_pair_info != NULL)
            {
                int parse_result = custom_parse_pair_info_cmd(data, data_len, get_pair_info, 8, &get_pair_info_count);
                rt_kprintf("custom_parse_pair_info_cmd parse_result = %d, get_pair_info_count = %d\r\n", parse_result, get_pair_info_count);
                if (parse_result == 0)
                {
                    unsigned char index = 0;
                    unsigned int self_userid = manager_get_userid();
                    for (index = 0; index < get_pair_info_count; index++)
                    {
                        if ((get_pair_info[index].address == self_userid)
                            && (get_pair_info[index].dev_type == DEV_TYPE_SWITCH)
                            && (get_pair_info[index].index < switch_get_count()))
                        {
                            rt_kprintf("switch pair info index = %d\r\n", get_pair_info[index].index);
                            custom_set_switch_pair_info(get_pair_info[index].index, get_pair_info, get_pair_info_count);
                        }
                    }
                }
                rt_free(get_pair_info);
            }
            if (recv_data_info->need_response == 1)
            {
                t_send_data_info respond_data_info;
                respond_data_info.auto_src_addr = 1;
                respond_data_info.dest_addr_type = recv_data_info->src_addr_type;
                respond_data_info.need_response = 0;
                respond_data_info.is_response = 1;
                respond_data_info.compress_flag = 0;
                if (recv_data_info->is_packet_num)
                {
                    respond_data_info.packet_num_type = 2;
                    respond_data_info.user_packet_num = recv_data_info->packet_num;
                }
                else
                {
                    respond_data_info.packet_num_type = 0;
                    respond_data_info.user_packet_num = 0;
                }
                respond_data_info.src_addr = 0;
                respond_data_info.dest_addr = recv_data_info->src_addr;
                respond_data_info.cmd_type = recv_data_info->cmd_type;
                (void)manager_send_wiota_data(&respond_data_info, NULL, 0, NULL, NULL);
            }
        }
        break;

    case APP_CMD_IOTE_PAIR_REQUEST_CANCEL:
        if (recv_data_info->is_response == 1)
        {
            //todo
        }
        break;

    case APP_CMD_NET_PAIR_CANCEL:
        if (recv_data_info->is_response == 0)
        {
            pair_info_t *get_pair_info = NULL;
            unsigned char get_pair_info_count;

            get_pair_info = rt_malloc(8 * sizeof(pair_info_t));
            if (get_pair_info != NULL)
            {
                int parse_result = custom_parse_pair_info_cmd(data, data_len, get_pair_info, 8, &get_pair_info_count);
                if (parse_result == 0)
                {
                    unsigned char index = 0;
                    unsigned int self_userid = manager_get_userid();
                    for (index = 0; index < get_pair_info_count; index++)
                    {
                        if ((get_pair_info[index].address == self_userid)
                            && (get_pair_info[index].dev_type == DEV_TYPE_SWITCH)
                            && (get_pair_info[index].index < switch_get_count()))
                        {
                            custom_clear_switch_pair_info(get_pair_info[index].index);
                        }
                    }
                }
                rt_free(get_pair_info);
            }
            if (recv_data_info->need_response == 1)
            {
                t_send_data_info respond_data_info;
                respond_data_info.auto_src_addr = 1;
                respond_data_info.dest_addr_type = recv_data_info->src_addr_type;
                respond_data_info.need_response = 0;
                respond_data_info.is_response = 1;
                respond_data_info.compress_flag = 0;
                if (recv_data_info->is_packet_num)
                {
                    respond_data_info.packet_num_type = 2;
                    respond_data_info.user_packet_num = recv_data_info->packet_num;
                }
                else
                {
                    respond_data_info.packet_num_type = 0;
                    respond_data_info.user_packet_num = 0;
                }
                respond_data_info.src_addr = 0;
                respond_data_info.dest_addr = recv_data_info->src_addr;
                respond_data_info.cmd_type = recv_data_info->cmd_type;
                (void)manager_send_wiota_data(&respond_data_info, NULL, 0, NULL, NULL);
            }
        }
        break;

    case APP_CMD_NET_CTRL_IOTE:
        break;

    default:
        break;
    }
}

#endif
