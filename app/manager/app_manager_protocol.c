#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include <board.h>
#include <string.h>
#include "app_manager_logic.h"
#include "app_manager_protocol.h"
#include "app_manager_cfg.h"
#include "cJSON.h"

int manager_parse_multicast_addr_cmd(unsigned char *data, unsigned int data_len, unsigned int *multicast_addr)
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

    item = cJSON_GetObjectItem(root, "broadcast_address");
    if (cJSON_IsNumber(item))
    {
        *multicast_addr = item->valuedouble;
    }
    else
    {
        result = -2;
    }

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return result;
}

int manager_parse_config_cmd(unsigned char *data, unsigned int data_len, unsigned int *config_valid_mask, t_wiota_net_config *wiota_config)
{
    int result = 0;
    cJSON *root = NULL;
    cJSON *array = NULL;
    cJSON *item = NULL;

    *config_valid_mask = 0;
    root = cJSON_Parse((const char *)data);
    if (root == NULL)
    {
        result = -1;
        goto __end;
    }

    array = cJSON_GetObjectItem(root, "freq_list");
    if (cJSON_IsArray(array))
    {
        int index = 0;
        memset(wiota_config->freq_list, 0xff, sizeof(wiota_config->freq_list));
        for (index = 0; index < sizeof(wiota_config->freq_list); index++)
        {
            item = cJSON_GetArrayItem(array, index);
            if (cJSON_IsNumber(item))
            {
                wiota_config->freq_list[index] = item->valueint;
            }
            else
            {
                break;
            }
        }
        if (index > 0)
        {
            *config_valid_mask |= 1 << I_WIOTA_CFG_FREQ_LIST;
        }
    }

    item = cJSON_GetObjectItem(root, "ap_max_power");
    if (cJSON_IsNumber(item))
    {
        wiota_config->ap_max_pow = item->valueint;
        *config_valid_mask |= 1 << I_WIOTA_CFG_AP_MAX_POWER;
    }

    item = cJSON_GetObjectItem(root, "id_len");
    if (cJSON_IsNumber(item))
    {
        wiota_config->id_len = item->valueint;
        *config_valid_mask |= 1 << I_WIOTA_CFG_ID_LEN;
    }

    item = cJSON_GetObjectItem(root, "pn_num");
    if (cJSON_IsNumber(item))
    {
        wiota_config->pn_num = item->valueint;
        *config_valid_mask |= 1 << I_WIOTA_CFG_PN_NUM;
    }

    item = cJSON_GetObjectItem(root, "symbol_length");
    if (cJSON_IsNumber(item))
    {
        wiota_config->symbol_length = item->valueint;
        *config_valid_mask |= 1 << I_WIOTA_CFG_SYMBOL_LENGTH;
    }

    item = cJSON_GetObjectItem(root, "dlul_ratio");
    if (cJSON_IsNumber(item))
    {
        wiota_config->dlul_ratio = item->valueint;
        *config_valid_mask |= 1 << I_WIOTA_CFG_DLUL_RATIO;
    }

    item = cJSON_GetObjectItem(root, "bt_value");
    if (cJSON_IsNumber(item))
    {
        wiota_config->btvalue = item->valueint;
        *config_valid_mask |= 1 << I_WIOTA_CFG_BT_VALUE;
    }

    item = cJSON_GetObjectItem(root, "group_number");
    if (cJSON_IsNumber(item))
    {
        wiota_config->group_number = item->valueint;
        *config_valid_mask |= 1 << I_WIOTA_CFG_GROUP_NUMBER;
    }

    item = cJSON_GetObjectItem(root, "spectrum_idx");
    if (cJSON_IsNumber(item))
    {
        wiota_config->spectrum_idx = item->valueint;
        *config_valid_mask |= 1 << I_WIOTA_CFG_SPECTRUM_IDX;
    }

    item = cJSON_GetObjectItem(root, "system_id");
    if (cJSON_IsNumber(item))
    {
        wiota_config->systemid = item->valuedouble;
        *config_valid_mask |= 1 << I_WIOTA_CFG_SYSTEM_ID;
    }

    item = cJSON_GetObjectItem(root, "subsystem_id");
    if (cJSON_IsNumber(item))
    {
        wiota_config->subsystemid = item->valuedouble;
        *config_valid_mask |= 1 << I_WIOTA_CFG_SUBSYSTEM_ID;
    }

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return result;
}

int manager_parse_get_wiota_addr_cmd(unsigned char *data, unsigned int data_len, unsigned int *wiota_addr)
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

    item = cJSON_GetObjectItem(root, "new_address");
    if (cJSON_IsNumber(item))
    {
        *wiota_addr = item->valuedouble;
    }
    else
    {
        result = -2;
    }

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return result;
}

unsigned char *manager_create_wiota_addr_request_data(unsigned int device_id)
{
    unsigned char *json_data = NULL;
    cJSON *root = NULL;

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        goto __end;
    }

    cJSON_AddItemToObject(root, "dev_address", cJSON_CreateNumber(device_id));

    json_data = (unsigned char *)cJSON_Print((const cJSON *)root);

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return json_data;
}

void manager_delete_wiota_addr_request_data(unsigned char *data)
{
    rt_free(data);
}

unsigned char *manager_create_multicast_addr_request_data(const char *device_type_name)
{
    unsigned char *json_data = NULL;
    cJSON *root = NULL;

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        goto __end;
    }

    cJSON_AddItemToObject(root, "type", cJSON_CreateString(device_type_name));

    json_data = (unsigned char *)cJSON_Print((const cJSON *)root);

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return json_data;
}

void manager_delete_multicast_addr_request_data(unsigned char *data)
{
    rt_free(data);
}

unsigned char *manager_create_config_data(t_wiota_net_config *wiota_config)
{
    unsigned char *json_data = NULL;
    cJSON *root = NULL;

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        goto __end;
    }

    int freq_list_count = 0;
    for (int index = 0; index < sizeof(wiota_config->freq_list); index++)
    {
        if (wiota_config->freq_list[index] != 0xff)
        {
            freq_list_count++;
        }
    }
    cJSON_AddItemToObject(root, "freq_list", cJSON_CreateIntArray((const int *)wiota_config->freq_list, freq_list_count));
    cJSON_AddItemToObject(root, "ap_max_power", cJSON_CreateNumber(wiota_config->ap_max_pow));
    cJSON_AddItemToObject(root, "id_len", cJSON_CreateNumber(wiota_config->id_len));
    cJSON_AddItemToObject(root, "pn_num", cJSON_CreateNumber(wiota_config->pn_num));
    cJSON_AddItemToObject(root, "symbol_length", cJSON_CreateNumber(wiota_config->symbol_length));
    cJSON_AddItemToObject(root, "dlul_ratio", cJSON_CreateNumber(wiota_config->dlul_ratio));
    cJSON_AddItemToObject(root, "bt_value", cJSON_CreateNumber(wiota_config->btvalue));
    cJSON_AddItemToObject(root, "group_number", cJSON_CreateNumber(wiota_config->group_number));
    cJSON_AddItemToObject(root, "spectrum_idx", cJSON_CreateNumber(wiota_config->spectrum_idx));
    cJSON_AddItemToObject(root, "system_id", cJSON_CreateNumber(wiota_config->systemid));
    cJSON_AddItemToObject(root, "subsystem_id", cJSON_CreateNumber(wiota_config->subsystemid));

    json_data = (unsigned char *)cJSON_Print((const cJSON *)root);

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return json_data;
}

void manager_delete_config_data(unsigned char *data)
{
    rt_free(data);
}

void manager_system_recv_data_process(t_recv_data_info *recv_data_info, unsigned char *data, unsigned int data_len)
{
    switch (recv_data_info->cmd_type)
    {
    case APP_CMD_IOTE_MUTICAST_REQUEST:
        if ((recv_data_info->is_response == 1) && (data_len > 0))
        {
            unsigned int multicast_addr = 0;
            int parse_result = manager_parse_multicast_addr_cmd(data, data_len, &multicast_addr);
            rt_kprintf("manager_parse_multicast_addr_cmd 100 parse_result = %d\r\n", parse_result);
            rt_kprintf("multicast_addr = 0x%08X\r\n", multicast_addr);
            if ((parse_result == 0) && (multicast_addr >= MANAGE_LOGIC_MULTICAST_ADDR_START) && (multicast_addr <= MANAGE_LOGIC_MULTICAST_ADDR_END))
            {
                manager_set_multicast_addr(multicast_addr);
            }
        }
        break;

    case APP_CMD_NET_MUTICAST_ASSIGNMENT:
        if (recv_data_info->is_response == 0)
        {
            unsigned int multicast_addr = 0;
            int parse_result = manager_parse_multicast_addr_cmd(data, data_len, &multicast_addr);
            rt_kprintf("manager_parse_multicast_addr_cmd 101 parse_result = %d\r\n", parse_result);
            rt_kprintf("multicast_addr = 0x%08X\r\n", multicast_addr);
            if ((parse_result == 0) && (multicast_addr >= MANAGE_LOGIC_MULTICAST_ADDR_START) && (multicast_addr <= MANAGE_LOGIC_MULTICAST_ADDR_END))
            {
                manager_set_multicast_addr(multicast_addr);
            }
            if (recv_data_info->need_response == 1)
            {
                t_send_data_info respond_data_info;

                manager_respond_data_info_init(&respond_data_info, recv_data_info);
                (void)manager_send_wiota_data(&respond_data_info, NULL, 0, NULL, NULL);
            }
        }
        break;

    case APP_CMD_IOTE_REQUEST_NEW_CONFIG:
        if ((recv_data_info->is_response == 1) && (data_len > 0))
        {
            t_wiota_net_config *wiota_config = NULL;
            wiota_config = rt_malloc(sizeof(t_wiota_net_config));
            if (wiota_config != NULL)
            {
                unsigned int config_valid_mask = 0;
                int parse_result = manager_parse_config_cmd(data, data_len, &config_valid_mask, wiota_config);
                if ((parse_result == 0) && (config_valid_mask))
                {
                    manager_set_wiota_config(config_valid_mask, wiota_config);
                }
                rt_free(wiota_config);
            }
        }
        break;

    case APP_CMD_NET_DELIVER_IOTE_CONFIG:
        if (recv_data_info->is_response == 0)
        {
            t_wiota_net_config *wiota_config = NULL;
            wiota_config = rt_malloc(sizeof(t_wiota_net_config));
            if (wiota_config != NULL)
            {
                unsigned int config_valid_mask = 0;
                int parse_result = manager_parse_config_cmd(data, data_len, &config_valid_mask, wiota_config);
                if ((parse_result == 0) && (config_valid_mask))
                {
                    manager_set_wiota_config(config_valid_mask, wiota_config);
                }
                rt_free(wiota_config);
            }
            if (recv_data_info->need_response == 1)
            {
                t_send_data_info respond_data_info;

                manager_respond_data_info_init(&respond_data_info, recv_data_info);
                (void)manager_send_wiota_data(&respond_data_info, NULL, 0, NULL, NULL);
            }
        }
        break;

    case APP_CMD_RESERVED_ADDR_REQUEST:
        break;

    case APP_CMD_RESERVED_ADDR_RESPONSE:
        break;

    case APP_CMD_CHANGE_ADDR_REQUEST:
        //if (recv_data_info->is_response == 0)
        {
            unsigned int wiota_addr = 0;

            int parse_result = manager_parse_get_wiota_addr_cmd(data, data_len, &wiota_addr);
            rt_kprintf("manager_parse_get_wiota_addr_cmd 1, parse_result = %d\r\n", parse_result);
            if (parse_result == 0)
            {
                unsigned int old_addr = manager_get_wiotaid();
                rt_kprintf("old_addr = 0x08%x, wiota_addr = 0x08%x\r\n", old_addr, wiota_addr);
                if (old_addr == wiota_addr)
                {
                    manager_wiota_connected();
                }
                else
                {
                    manager_set_wiotaid(wiota_addr);
                    manager_reset_wiota();
                }
            }
            if (recv_data_info->need_response == 1)
            {
                t_send_data_info respond_data_info;

                manager_respond_data_info_init(&respond_data_info, recv_data_info);
                (void)manager_send_wiota_data(&respond_data_info, NULL, 0, NULL, NULL);
            }
        }
        break;

    case APP_CMD_CHANGE_ADDR_RESPONSE:
        if (recv_data_info->is_response == 0)
        {
            unsigned int wiota_addr = 0;

            int parse_result = manager_parse_get_wiota_addr_cmd(data, data_len, &wiota_addr);
            rt_kprintf("manager_parse_get_wiota_addr_cmd 2, parse_result = %d\r\n", parse_result);
            if (parse_result == 0)
            {
                unsigned int old_addr = manager_get_wiotaid();
                rt_kprintf("old_addr = 0x08%x, wiota_addr = 0x08%x\r\n", old_addr, wiota_addr);
                if (old_addr == wiota_addr)
                {
                    manager_wiota_connected();
                }
                else
                {
                    manager_set_wiotaid(wiota_addr);
                    manager_reset_wiota();
                }
            }
            if (recv_data_info->need_response == 1)
            {
                t_send_data_info respond_data_info;

                manager_respond_data_info_init(&respond_data_info, recv_data_info);
                (void)manager_send_wiota_data(&respond_data_info, NULL, 0, NULL, NULL);
            }
        }
        break;

    default:
        break;
    }
}

#endif
