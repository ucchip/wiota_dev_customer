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
#include "custom_ota.h"

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
    if (!cJSON_IsArray(array))
    {
        result = -1;
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
        if (cJSON_IsObject(array_item))
        {
            item = cJSON_GetObjectItem(array_item, "dest_address");
            if (!cJSON_IsNumber(item))
            {
                result = -1;
                goto __end;
            }
            get_pair_info[index].address = item->valuedouble;
            rt_kprintf("custom_parse_pair_info_cmd address = 0x%08x\r\n", get_pair_info[index].address);

            item = cJSON_GetObjectItem(array_item, "type");
            if (!cJSON_IsString(item))
            {
                result = -1;
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
                if (!cJSON_IsNumber(item))
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

unsigned char *custom_create_check_version_data(char *soft_version, char *hard_version, char *dev_type)
{
    unsigned char *json_data = NULL;
    cJSON *root = NULL;

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        goto __end;
    }
    cJSON_AddItemToObject(root, "soft_version", cJSON_CreateString(soft_version));
    cJSON_AddItemToObject(root, "hard_version", cJSON_CreateString(hard_version));
    cJSON_AddItemToObject(root, "dev_type", cJSON_CreateString(dev_type));

    json_data = (unsigned char *)cJSON_Print((const cJSON *)root);

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return json_data;
}

void custom_delete_check_version_data(unsigned char *data)
{
    rt_free(data);
}

int custom_parse_check_version_cmd(unsigned char *data, unsigned int data_len, 
                unsigned char *state, 
                char *new_version, 
                char *old_version, 
                char *dev_type, 
                unsigned char *update_type, 
                char *file, 
                unsigned int *size, 
                char *md5, 
                unsigned char *access, 
                char *username, 
                char *password, 
                char *path, 
                char *address, 
                char *port)
{
    int result = 0;
    cJSON *root = NULL;
    cJSON *object = NULL;
    cJSON *item = NULL;

    root = cJSON_Parse((const char *)data);
    if (root == NULL)
    {
        result = -1;
        goto __end;
    }

    item = cJSON_GetObjectItem(root, "state");
    if (!cJSON_IsNumber(item))
    {
        result = -2;
        goto __end;
    }
    if (state != NULL)
    {
        *state = item->valuedouble;
    }

    item = cJSON_GetObjectItem(root, "new_version");
    if (!cJSON_IsString(item))
    {
        result = -2;
        goto __end;
    }
    if (new_version != NULL)
    {
        memcpy(new_version, item->valuestring, strlen(item->valuestring));
    }

    item = cJSON_GetObjectItem(root, "old_version");
    if (!cJSON_IsString(item))
    {
        result = -2;
        goto __end;
    }
    if (old_version != NULL)
    {
        memcpy(old_version, item->valuestring, strlen(item->valuestring));
    }

    item = cJSON_GetObjectItem(root, "dev_type");
    if (!cJSON_IsString(item))
    {
        result = -2;
        goto __end;
    }
    if (dev_type != NULL)
    {
        memcpy(dev_type, item->valuestring, strlen(item->valuestring));
    }

    item = cJSON_GetObjectItem(root, "update_type");
    if (!cJSON_IsNumber(item))
    {
        result = -2;
        goto __end;
    }
    if (update_type != NULL)
    {
        *update_type = item->valuedouble;
    }

    item = cJSON_GetObjectItem(root, "file");
    if (!cJSON_IsString(item))
    {
        result = -2;
        goto __end;
    }
    if (file != NULL)
    {
        memcpy(file, item->valuestring, strlen(item->valuestring));
    }

    item = cJSON_GetObjectItem(root, "size");
    if (!cJSON_IsNumber(item))
    {
        result = -2;
        goto __end;
    }
    if (size != NULL)
    {
        *size = item->valuedouble;
    }

    item = cJSON_GetObjectItem(root, "md5");
    if (!cJSON_IsString(item))
    {
        result = -2;
        goto __end;
    }
    if (md5 != NULL)
    {
        memcpy(md5, item->valuestring, strlen(item->valuestring));
    }

    object = cJSON_GetObjectItem(root, "access_info");
    if (cJSON_IsObject(object))
    {
        item = cJSON_GetObjectItem(object, "access");
        if (!cJSON_IsNumber(item))
        {
            result = -2;
            goto __end;
        }
        if (access != NULL)
        {
            *access = item->valuedouble;
        }

        item = cJSON_GetObjectItem(object, "username");
        if (!cJSON_IsString(item))
        {
            result = -2;
            goto __end;
        }
        if (username != NULL)
        {
            memcpy(username, item->valuestring, strlen(item->valuestring));
        }

        item = cJSON_GetObjectItem(object, "password");
        if (!cJSON_IsString(item))
        {
            result = -2;
            goto __end;
        }
        if (password != NULL)
        {
            memcpy(password, item->valuestring, strlen(item->valuestring));
        }

        item = cJSON_GetObjectItem(object, "path");
        if (!cJSON_IsString(item))
        {
            result = -2;
            goto __end;
        }
        if (path != NULL)
        {
            memcpy(path, item->valuestring, strlen(item->valuestring));
        }

        item = cJSON_GetObjectItem(object, "address");
        if (!cJSON_IsString(item))
        {
            result = -2;
            goto __end;
        }
        if (address != NULL)
        {
            memcpy(address, item->valuestring, strlen(item->valuestring));
        }

        item = cJSON_GetObjectItem(object, "port");
        if (!cJSON_IsString(item))
        {
            result = -2;
            goto __end;
        }
        if (port != NULL)
        {
            memcpy(port, item->valuestring, strlen(item->valuestring));
        }
    }

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return result;
}

unsigned char *custom_create_ota_request_specified_data(char *dev_type, 
                char *old_version, 
                char *new_version, 
                unsigned char update_type, 
                unsigned int data_info_count,
                unsigned int *offset,
                unsigned int *len)
{
    unsigned char *json_data = NULL;
    cJSON *root = NULL;
    cJSON *array = NULL;

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        goto __end;
    }
    cJSON_AddItemToObject(root, "dev_type", cJSON_CreateString(dev_type));
    cJSON_AddItemToObject(root, "old_version", cJSON_CreateString(old_version));
    cJSON_AddItemToObject(root, "new_version", cJSON_CreateString(new_version));
    cJSON_AddItemToObject(root, "update_type", cJSON_CreateNumber(update_type));
    array = cJSON_CreateArray();
    if (array == NULL)
    {
        goto __end;
    }
    for (unsigned int index = 0; index < data_info_count; index++)
    {
        cJSON *object = NULL;
        object = cJSON_CreateObject();
        if (object == NULL)
        {
            goto __end;
        }
        cJSON_AddItemToObject(object, "offset", cJSON_CreateNumber(offset[index]));
        cJSON_AddItemToObject(object, "len", cJSON_CreateNumber(len[index]));
        cJSON_AddItemToArray(array, object);
    }
    cJSON_AddItemToObject(root, "data_info", array);

    json_data = (unsigned char *)cJSON_Print((const cJSON *)root);

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return json_data;
}

void custom_delete_ota_request_specified_data(unsigned char *data)
{
    rt_free(data);
}

int custom_parse_ota_respond_specified_data_cmd(unsigned char *data, unsigned int data_len, 
                unsigned char *state, 
                unsigned char *range, 
                unsigned int *size, 
                char *md5, 
                unsigned int *dev_list, 
                unsigned int *dev_list_count, 
                char *new_version, 
                char *old_version, 
                char *dev_type, 
                unsigned int *offset, 
                unsigned int *len, 
                unsigned char *ota_data)
{
    int result = 0;
    cJSON *root = NULL;
    cJSON *item = NULL;
    cJSON *array = NULL;
    cJSON *object = NULL;
    unsigned int ota_data_len = 0;

    root = cJSON_Parse((const char *)data);
    if (root == NULL)
    {
        result = -1;
        goto __end;
    }

    item = cJSON_GetObjectItem(root, "state");
    if (!cJSON_IsNumber(item))
    {
        result = -2;
        goto __end;
    }
    if (state != NULL)
    {
        *state = item->valuedouble;
    }

    item = cJSON_GetObjectItem(root, "range");
    if (!cJSON_IsNumber(item))
    {
        result = -2;
        goto __end;
    }
    if (range != NULL)
    {
        *range = item->valuedouble;
    }

    item = cJSON_GetObjectItem(root, "size");
    if (!cJSON_IsNumber(item))
    {
        result = -2;
        goto __end;
    }
    if (size != NULL)
    {
        *size = item->valuedouble;
    }

    item = cJSON_GetObjectItem(root, "md5");
    if (!cJSON_IsString(item))
    {
        result = -2;
        goto __end;
    }
    if (md5 != NULL)
    {
        memcpy(md5, item->valuestring, strlen(item->valuestring));
    }

    array = cJSON_GetObjectItem(root, "dev_list");
    if (cJSON_IsArray(array))
    {
        unsigned int index = 0;
        for (index = 0; index < cJSON_GetArraySize(array); index++)
        {
            item = cJSON_GetArrayItem(array, index);
            if (!cJSON_IsNumber(item))
            {
                break;
            }
            if (dev_list != NULL)
            {
                dev_list[index] = item->valuedouble;
            }
        }
        if (dev_list_count != NULL)
        {
            *dev_list_count = index;
        }
    }
    else
    {
        if (dev_list_count != NULL)
        {
            *dev_list_count = 0;
        }
    }

    item = cJSON_GetObjectItem(root, "new_version");
    if (!cJSON_IsString(item))
    {
        result = -2;
        goto __end;
    }
    if (new_version != NULL)
    {
        memcpy(new_version, item->valuestring, strlen(item->valuestring));
    }

    item = cJSON_GetObjectItem(root, "old_version");
    if (!cJSON_IsString(item))
    {
        result = -2;
        goto __end;
    }
    if (old_version != NULL)
    {
        memcpy(old_version, item->valuestring, strlen(item->valuestring));
    }

    item = cJSON_GetObjectItem(root, "dev_type");
    if (!cJSON_IsString(item))
    {
        result = -2;
        goto __end;
    }
    if (dev_type != NULL)
    {
        memcpy(dev_type, item->valuestring, strlen(item->valuestring));
    }

    object = cJSON_GetObjectItem(root, "info");
    if (!cJSON_IsObject(object))
    {
        result = -2;
        goto __end;
    }

    item = cJSON_GetObjectItem(object, "offset");
    if (!cJSON_IsNumber(item))
    {
        result = -2;
        goto __end;
    }
    if (offset != NULL)
    {
        *offset = item->valuedouble;
    }

    item = cJSON_GetObjectItem(object, "len");
    if (!cJSON_IsNumber(item))
    {
        result = -2;
        goto __end;
    }
    if (len != NULL)
    {
        *len = item->valuedouble;
    }
    ota_data_len = item->valuedouble;

#if 0
    item = cJSON_GetObjectItem(object, "data");
    if (!cJSON_IsRaw(item))
    {
        result = -2;
        goto __end;
    }
    if (ota_data != NULL)
    {
        memcpy(ota_data, item->valuestring, strlen(item->valuestring));
    }
#else
    if ((ota_data_len < data_len) && (data[data_len - ota_data_len - 1] == '}'))
    {
        if (ota_data != NULL)
        {
            memcpy(ota_data, &data[data_len - ota_data_len], ota_data_len);
        }
    }
    else
    {
        result = -4;
        goto __end;
    }
#endif

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return result;
}

int custom_parse_ota_upgrade_state_cmd(unsigned char *data, unsigned int data_len, 
                unsigned char *state, 
                char *new_version, 
                char *old_version, 
                char *dev_type, 
                unsigned char *ota_state)
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

    item = cJSON_GetObjectItem(root, "state");
    if (!cJSON_IsNumber(item))
    {
        result = -2;
        goto __end;
    }
    if (state != NULL)
    {
        *state = item->valuedouble;
    }

    item = cJSON_GetObjectItem(root, "new_version");
    if (!cJSON_IsString(item))
    {
        result = -2;
        goto __end;
    }
    if (new_version != NULL)
    {
        memcpy(new_version, item->valuestring, strlen(item->valuestring));
    }

    item = cJSON_GetObjectItem(root, "old_version");
    if (!cJSON_IsString(item))
    {
        result = -2;
        goto __end;
    }
    if (old_version != NULL)
    {
        memcpy(old_version, item->valuestring, strlen(item->valuestring));
    }

    item = cJSON_GetObjectItem(root, "dev_type");
    if (!cJSON_IsString(item))
    {
        result = -2;
        goto __end;
    }
    if (dev_type != NULL)
    {
        memcpy(dev_type, item->valuestring, strlen(item->valuestring));
    }

    item = cJSON_GetObjectItem(root, "ota_state");
    if (!cJSON_IsNumber(item))
    {
        result = -2;
        goto __end;
    }
    if (ota_state != NULL)
    {
        *ota_state = item->valuedouble;
    }

__end:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    return result;
}

#ifdef APP_DEMO_LIGHT
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
    if (!cJSON_IsString(item))
    {
        result = -2;
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
        if (!cJSON_IsNumber(item))
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
    if (!cJSON_IsString(item))
    {
        result = -2;
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

                manager_respond_data_info_init(&respond_data_info, recv_data_info);
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

                manager_respond_data_info_init(&respond_data_info, recv_data_info);
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
                    unsigned int self_userid = manager_get_deviceid();
                    for (index = 0; index < get_pair_info_count; index++)
                    {
                        if ((get_pair_info[index].address == self_userid) && (get_pair_info[index].dev_type == DEV_TYPE_LIGHT))
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

                manager_respond_data_info_init(&respond_data_info, recv_data_info);
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
                    unsigned int self_userid = manager_get_deviceid();
                    for (index = 0; index < get_pair_info_count; index++)
                    {
                        if ((get_pair_info[index].address == self_userid) && (get_pair_info[index].dev_type == DEV_TYPE_LIGHT))
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

                manager_respond_data_info_init(&respond_data_info, recv_data_info);
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

#ifdef APP_DEMO_SWITCH
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
    cJSON_AddItemToObject(root, "index", cJSON_CreateNumber(sw_index + 1));

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
    cJSON_AddItemToObject(root, "index", cJSON_CreateNumber(sw_index + 1));
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

                manager_respond_data_info_init(&respond_data_info, recv_data_info);
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

                manager_respond_data_info_init(&respond_data_info, recv_data_info);
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
                    unsigned int self_userid = manager_get_deviceid();
                    for (index = 0; index < get_pair_info_count; index++)
                    {
                        if ((get_pair_info[index].address == self_userid) && (get_pair_info[index].dev_type == DEV_TYPE_SWITCH) && (get_pair_info[index].index < switch_get_count()))
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

                manager_respond_data_info_init(&respond_data_info, recv_data_info);
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
                    unsigned int self_userid = manager_get_deviceid();
                    for (index = 0; index < get_pair_info_count; index++)
                    {
                        if ((get_pair_info[index].address == self_userid) && (get_pair_info[index].dev_type == DEV_TYPE_SWITCH) && (get_pair_info[index].index < switch_get_count()))
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

                manager_respond_data_info_init(&respond_data_info, recv_data_info);
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

#endif
