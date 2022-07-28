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
#include "custom_ota.h"
#include "custom_manager.h"
#include "md5.h"

#define CUSTOM_OTA_DEFAULT_BLOCK_SIZE       512
#define CUSTOM_OTA_FLASH_START_ADDR         (256 * 1024)

#define CUSTOM_OTA_WAIT_DATA_TIMEOUT        (5 * 1000)
#define CUSTOM_OTA_WAIT_PROGRAM_TIMEOUT     (30 * 1000)

#define CUSTOM_OTA_CHECK_VERSION_CYCLE_TIME (1 * 60 * 60 * 1000)

static unsigned char g_custom_ota_upgrade_state = OTA_STATE_IDLE;
static unsigned char g_custom_ota_upgrade_result = OTA_RESULT_INIT;
static t_upgrade_info g_custom_ota_upgrade_info;
static t_upgrade_data_info g_custom_ota_upgrade_data_info;
static rt_timer_t g_custom_ota_timer = NULL;
static unsigned int g_custom_ota_timeout_event = OTA_TET_IDLE;

void custom_ota_request_check_version(void)
{
    t_send_data_info send_data_info;
    unsigned char *app_data = NULL;
    unsigned int get_data_id = 0;
    int result = 0;

    char soft_version[16];
    char hard_version[16];
    char *dev_type = NULL;

    if (g_custom_ota_upgrade_state != OTA_STATE_IDLE)
    {
        return;
    }
    manager_get_software_ver(soft_version);
    manager_get_hardware_ver(hard_version);
    dev_type = manager_get_device_type_name();
    app_data = custom_create_check_version_data(soft_version, hard_version, dev_type);
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
    send_data_info.cmd_type = APP_CMD_OTA_CHECK_VERSION;
    custom_send_lock_take();
    result = manager_send_wiota_data(&send_data_info, app_data, strlen((const char *)app_data), custom_send_data_result_callback, &get_data_id);
    if (result != 0)
    {
        custom_send_lock_release();
    }
    rt_kprintf("custom_ota_request_check_version result = %d, get_data_id = %d\r\n", result, get_data_id);

__end:
    if (app_data != NULL)
    {
        custom_delete_check_version_data(app_data);
    }
}

void custom_ota_request_specified_data(void)
{
    t_send_data_info send_data_info;
    unsigned char *app_data = NULL;
    unsigned int get_data_id = 0;
    int result = 0;

    if (g_custom_ota_upgrade_state != OTA_STATE_DOWNLOAD)
    {
        return;
    }
    if (g_custom_ota_upgrade_data_info.block_recv_mask == NULL)
    {
        return;
    }

    unsigned int data_info_count = 0;
    unsigned int *offset = NULL;
    unsigned int *len = NULL;
    offset = rt_malloc(g_custom_ota_upgrade_data_info.block_count * sizeof(unsigned int));
    len = rt_malloc(g_custom_ota_upgrade_data_info.block_count * sizeof(unsigned int));
    if ((offset == NULL) || (len == NULL))
    {
        rt_kprintf("custom_ota_request_specified_data malloc Err\r\n");
        goto __end;
    }
    for (unsigned int index = 0; index < g_custom_ota_upgrade_data_info.block_count; index++)
    {
        if (g_custom_ota_upgrade_data_info.block_recv_mask[index / 8] & (1 << index % 8))
        {
            offset[data_info_count] = index * g_custom_ota_upgrade_data_info.block_size;
            len[data_info_count] = g_custom_ota_upgrade_data_info.block_size;
            data_info_count++;
        }
    }
    if (data_info_count == 0)
    {
        goto __end;
    }
    app_data = custom_create_ota_request_specified_data(g_custom_ota_upgrade_info.dev_type, 
                g_custom_ota_upgrade_info.old_version, 
                g_custom_ota_upgrade_info.new_version, 
                g_custom_ota_upgrade_info.update_type, 
                data_info_count,
                offset,
                len);
    if (app_data == NULL)
    {
        goto __end;
    }
    //For any service, logic module must response to the request after processing it.
    send_data_info.auto_src_addr = 1;
    send_data_info.dest_addr_type = ADDR_TYPE_GW;
    send_data_info.need_response = 0;
    send_data_info.is_response = 0;
    send_data_info.compress_flag = 1;
    send_data_info.packet_num_type = 0;
    send_data_info.src_addr = 0;
    send_data_info.dest_addr = 0;
    send_data_info.cmd_type = APP_CMD_OTA_GET_DATA;
    custom_send_lock_take();
    result = manager_send_wiota_data(&send_data_info, app_data, strlen((const char *)app_data), custom_send_data_result_callback, &get_data_id);
    if (result != 0)
    {
        custom_send_lock_release();
    }
    rt_kprintf("custom_ota_request_specified_data result = %d, get_data_id = %d\r\n", result, get_data_id);

__end:
    if (app_data != NULL)
    {
        custom_delete_check_version_data(app_data);
    }
    if (offset != NULL)
    {
        rt_free(offset);
    }
    if (len != NULL)
    {
        rt_free(len);
    }
}

static int custom_ota_check_dev_id(unsigned int *dev_list, unsigned int dev_list_count)
{
    unsigned int dev_id = manager_get_deviceid();

    if (dev_list == NULL)
    {
        return -1;
    }

    for (unsigned int index = 0; index < dev_list_count; index++)
    {
        if (dev_list[index] == dev_id)
        {
            return 0;
        }
    }
    rt_kprintf("custom_ota_check_dev_id Err\r\n");

    return -2;
}

static void custom_ota_erase_flash(unsigned int start_addr, unsigned int erase_size)
{
    unsigned int erase_addr = start_addr;
    unsigned int size = 0;

    rt_kprintf("custom_ota_erase_flash start_addr = 0x%08x, erase_size = %d\r\n", start_addr, erase_size);
    if (erase_size == 0)
    {
        return;
    }

    erase_addr = (erase_addr / 4096) * 4096;
    if (erase_addr < start_addr)
    {
        uc_wiota_flash_erase_4K(erase_addr);
        erase_addr += 4096;
        size = start_addr - erase_addr;
    }
    while (size < erase_size)
    {
        uc_wiota_flash_erase_4K(erase_addr);
        erase_addr += 4096;
        size += 4096;
    }
}

static void custom_ota_write_flash(unsigned char *data_buf, unsigned int flash_addr, unsigned int length)
{
    rt_kprintf("custom_ota_write_flash flash_addr = 0x%08x, length = %d\r\n", flash_addr, length);
    uc_wiota_flash_write(data_buf, flash_addr, (unsigned short)length);
}

static int custom_ota_cmp_md5(char *md5, unsigned char *check_code)
{
    unsigned char md5_hex[16];

    if (strlen(md5) != 32)
    {
        return -1;
    }

    memset(md5_hex, 0x00, 16);
    for (unsigned int index = 0; index < 32; index++)
    {
        unsigned char hex_val = 0;
        if ((md5[index] >= '0') && (md5[index] <= '9'))
        {
            hex_val = md5[index] - '0';
        }
        else if ((md5[index] >= 'A') && (md5[index] <= 'F'))
        {
            hex_val = md5[index] - 'A' + 10;
        }
        else if ((md5[index] >= 'a') && (md5[index] <= 'f'))
        {
            hex_val = md5[index] - 'a' + 10;
        }
        else
        {
            return -2;
        }
        if (index % 2)
        {
            md5_hex[index / 2] |= hex_val;
        }
        else
        {
            md5_hex[index / 2] |= hex_val << 4;
        }
    }

    if (memcmp(md5_hex, check_code, 16) != 0)
    {
        rt_kprintf("custom_ota_cmp_md5 Err, md5 = %s\r\n", md5);
        return -3;
    }

    return 0;
}

static int custom_ota_check_flash_data(unsigned int flash_addr, unsigned int flash_size, char *md5)
{
    int result = 0;
    MD5_CTX *md5_context = NULL;
    unsigned char *data_buf = NULL;
    unsigned int data_buf_len = 1024;
    unsigned int data_offset = 0;
    unsigned char check_code[16];

    rt_kprintf("custom_ota_check_flash_data flash_addr = 0x%08x, flash_size = %d, md5 = %s\r\n", flash_addr, flash_size, md5);
    md5_context = rt_malloc(sizeof(MD5_CTX));
    data_buf = rt_malloc(data_buf_len);

    if ((md5_context == NULL) || (data_buf == NULL))
    {
        result = -1;
        goto _end;
    }

    MD5Init(md5_context);
    while (data_offset < flash_size)
    {
        unsigned int read_len = data_buf_len;
        if ((data_offset + read_len) > flash_size)
        {
            read_len = flash_size - data_offset;
        }
        if (uc_wiota_flash_read(data_buf, flash_addr + data_offset, read_len) != UC_OP_SUCC)
        {
            rt_kprintf("custom_ota_check_flash_data uc_wiota_flash_read Err\r\n");
            result = -2;
            goto _end;
        }
        MD5Update(md5_context, data_buf, read_len);
        data_offset += read_len;
    }
    MD5Final(md5_context, check_code);

    if (custom_ota_cmp_md5(md5, check_code) != 0)
    {
        result = -3;
        goto _end;
    }

_end:
    if (md5_context != NULL)
    {
        rt_free(md5_context);
    }
    if (data_buf != NULL)
    {
        rt_free(data_buf);
    }

    return result;
}

static void ota_timer_callback(void *param)
{
    if (param != NULL)
    {
        g_custom_ota_timeout_event = (unsigned int)param;
        rt_kprintf("ota_timeout param = %d\r\n", g_custom_ota_timeout_event);
    }
}

static void custom_ota_start_update_timer(unsigned int timeout_event_type)
{
    rt_tick_t tick_val = 0;
    if (g_custom_ota_timer != NULL)
    {
        rt_timer_stop(g_custom_ota_timer);
        rt_timer_delete(g_custom_ota_timer);
        g_custom_ota_timer = NULL;
    }
    if (timeout_event_type == OTA_TET_WAIT_DATA)
    {
        tick_val = rt_tick_from_millisecond(CUSTOM_OTA_WAIT_DATA_TIMEOUT);
    }
    else if (timeout_event_type == OTA_TET_WAIT_PROGRAM)
    {
        tick_val = rt_tick_from_millisecond(CUSTOM_OTA_WAIT_PROGRAM_TIMEOUT);
    }
    else //if (timeout_event_type == OTA_TET_EXIT)
    {
        tick_val = rt_tick_from_millisecond(CUSTOM_OTA_WAIT_DATA_TIMEOUT);
    }
    g_custom_ota_timer = rt_timer_create("ota_t", ota_timer_callback, (void *)timeout_event_type, tick_val, RT_TIMER_FLAG_ONE_SHOT);
    if (g_custom_ota_timer != NULL)
    {
        rt_timer_start(g_custom_ota_timer);
    }
}

static void custom_ota_stop_timer(void)
{
    if (g_custom_ota_timer != NULL)
    {
        rt_timer_stop(g_custom_ota_timer);
    }
}

static void custom_ota_jump_program(void)
{
    rt_kprintf("custom_ota_jump_program \r\n");
    return;
}

void custom_ota_check_version_process(void)
{
    static unsigned int start_flag = 1;
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
    if ((interval_tick > CUSTOM_OTA_CHECK_VERSION_CYCLE_TIME) || (start_flag == 1))
    {
        rt_kprintf("custom_ota_request_check_version\r\n");
        custom_ota_request_check_version();
        last_tick = cur_tick;
        start_flag = 0;
    }
}

void custom_ota_msg_process(void)
{
    //msg process
    if (g_custom_ota_timeout_event != OTA_TET_IDLE)
    {
        rt_kprintf("g_custom_ota_timeout_event = %d\r\n", g_custom_ota_timeout_event);
        if (g_custom_ota_timeout_event == OTA_TET_WAIT_DATA)
        {
            custom_ota_request_specified_data();
            custom_ota_start_update_timer(OTA_TET_EXIT);
        }
        else if (g_custom_ota_timeout_event == OTA_TET_WAIT_PROGRAM)
        {
            if ((g_custom_ota_upgrade_state == OTA_STATE_DOWNLOAD)
                || (g_custom_ota_upgrade_state == OTA_STATE_PROGRAM))
            {
                g_custom_ota_upgrade_state = OTA_STATE_IDLE;
                g_custom_ota_upgrade_result = OTA_RESULT_FAIL;
                custom_ota_stop_timer();
            }
        }
        else //if (g_custom_ota_timeout_event == OTA_TET_EXIT)
        {
            if ((g_custom_ota_upgrade_state == OTA_STATE_DOWNLOAD)
                || (g_custom_ota_upgrade_state == OTA_STATE_PROGRAM))
            {
                g_custom_ota_upgrade_state = OTA_STATE_IDLE;
                g_custom_ota_upgrade_result = OTA_RESULT_FAIL;
                custom_ota_stop_timer();
            }
        }
        g_custom_ota_timeout_event = OTA_TET_IDLE;
    }
}

void custom_ota_recv_data_process(t_recv_data_info *recv_data_info, unsigned char *data, unsigned int data_len)
{
    rt_kprintf("custom_ota_recv_data_process cmd_type = %d\r\n", recv_data_info->cmd_type);
    rt_kprintf("g_custom_ota_upgrade_state = %d\r\n", g_custom_ota_upgrade_state);
    switch (recv_data_info->cmd_type)
    {
    case APP_CMD_OTA_CHECK_VERSION:
        if (1)
        {
            if (g_custom_ota_upgrade_state == OTA_STATE_IDLE)
            {
                unsigned char state = 0;
                t_upgrade_info *ota_upgrade_info = NULL;
                ota_upgrade_info = rt_malloc(sizeof(t_upgrade_info));
                if (ota_upgrade_info != NULL)
                {
                    memset(ota_upgrade_info, 0x00, sizeof(t_upgrade_info));
                    int parse_result = custom_parse_check_version_cmd(data, data_len, 
                                    &state, 
                                    ota_upgrade_info->new_version, 
                                    ota_upgrade_info->old_version, 
                                    ota_upgrade_info->dev_type, 
                                    &ota_upgrade_info->update_type, 
                                    ota_upgrade_info->file, 
                                    &ota_upgrade_info->size, 
                                    ota_upgrade_info->md5, 
                                    &ota_upgrade_info->access, 
                                    ota_upgrade_info->username, 
                                    ota_upgrade_info->password, 
                                    ota_upgrade_info->path, 
                                    ota_upgrade_info->address, 
                                    ota_upgrade_info->port);
                    rt_kprintf("custom_parse_check_version_cmd parse_result = %d\r\n", parse_result);
                    rt_kprintf("state = %d\r\n", state);
                    rt_kprintf("new_version = %s\r\n", ota_upgrade_info->new_version);
                    rt_kprintf("old_version = %s\r\n", ota_upgrade_info->old_version);
                    rt_kprintf("dev_type = %s\r\n", ota_upgrade_info->dev_type);
                    rt_kprintf("update_type = %d\r\n", ota_upgrade_info->update_type);
                    rt_kprintf("file = %s\r\n", ota_upgrade_info->file);
                    rt_kprintf("size = %d\r\n", ota_upgrade_info->size);
                    rt_kprintf("md5 = %s\r\n", ota_upgrade_info->md5);
                    rt_kprintf("access = %d\r\n", ota_upgrade_info->access);
                    rt_kprintf("username = %s\r\n", ota_upgrade_info->username);
                    rt_kprintf("password = %s\r\n", ota_upgrade_info->password);
                    rt_kprintf("path = %s\r\n", ota_upgrade_info->path);
                    rt_kprintf("address = %s\r\n", ota_upgrade_info->address);
                    rt_kprintf("port = %s\r\n", ota_upgrade_info->port);
                    if (parse_result == 0)
                    {
                        char soft_version[16];
                        manager_get_software_ver(soft_version);
                        if ((state == 1) 
                            && (strcmp(soft_version, ota_upgrade_info->old_version) == 0)
                            && (strcmp(manager_get_device_type_name(), ota_upgrade_info->dev_type) == 0))
                        {
                            memcpy(&g_custom_ota_upgrade_info, ota_upgrade_info, sizeof(t_upgrade_info));
                            if (g_custom_ota_upgrade_data_info.block_recv_mask)
                            {
                                rt_free(g_custom_ota_upgrade_data_info.block_recv_mask);
                                g_custom_ota_upgrade_data_info.block_recv_mask = NULL;
                            }
                            g_custom_ota_upgrade_data_info.block_size = 0;
                            g_custom_ota_upgrade_data_info.block_count = 0;

                            g_custom_ota_upgrade_state = OTA_STATE_DOWNLOAD;
                            g_custom_ota_upgrade_result = OTA_RESULT_INIT;
                            custom_ota_start_update_timer(OTA_TET_WAIT_DATA);
                            rt_kprintf("msg APP_CMD_OTA_CHECK_VERSION goto OTA_STATE_DOWNLOAD\r\n");
                        }
                    }
                    rt_free(ota_upgrade_info);
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

    case APP_CMD_OTA_GET_DATA:
        if (1)
        {
            if ((g_custom_ota_upgrade_state == OTA_STATE_IDLE)
                || (g_custom_ota_upgrade_state == OTA_STATE_DOWNLOAD))
            {
                t_specified_data_info *specified_data_info = NULL;
                specified_data_info = rt_malloc(sizeof(t_specified_data_info));
                if (specified_data_info != NULL)
                {
                    memset(specified_data_info, 0x00, sizeof(t_specified_data_info));
                    int parse_result = custom_parse_ota_respond_specified_data_cmd(data, data_len, 
                                    &specified_data_info->state, 
                                    &specified_data_info->range, 
                                    &specified_data_info->size, 
                                    specified_data_info->md5, 
                                    specified_data_info->dev_list, 
                                    &specified_data_info->dev_list_count, 
                                    specified_data_info->new_version, 
                                    specified_data_info->old_version, 
                                    specified_data_info->dev_type, 
                                    &specified_data_info->offset, 
                                    &specified_data_info->len, 
                                    specified_data_info->ota_data);
                    rt_kprintf("custom_parse_ota_respond_specified_data_cmd parse_result = %d\r\n", parse_result);
                    rt_kprintf("state = %d\r\n", specified_data_info->state);
                    rt_kprintf("range = %d\r\n", specified_data_info->range);
                    rt_kprintf("new_version = %s\r\n", specified_data_info->new_version);
                    rt_kprintf("old_version = %s\r\n", specified_data_info->old_version);
                    rt_kprintf("dev_type = %s\r\n", specified_data_info->dev_type);
                    rt_kprintf("size = %d\r\n", specified_data_info->size);
                    rt_kprintf("md5 = %s\r\n", specified_data_info->md5);
                    if (parse_result == 0)
                    {
                        char soft_version[16];
                        manager_get_software_ver(soft_version);
                        if ((specified_data_info->offset % CUSTOM_OTA_DEFAULT_BLOCK_SIZE == 0)
                            && (strcmp(soft_version, specified_data_info->old_version) == 0)
                            && (strcmp(manager_get_device_type_name(), specified_data_info->dev_type) == 0)
                            && ((specified_data_info->range == 0) || (custom_ota_check_dev_id(specified_data_info->dev_list, specified_data_info->dev_list_count) == 0)))
                        {
                            unsigned char check_ok = 0;
                            if (g_custom_ota_upgrade_state == OTA_STATE_DOWNLOAD)
                            {
                                if ((specified_data_info->state == g_custom_ota_upgrade_info.update_type)
                                    && (specified_data_info->size == g_custom_ota_upgrade_info.size)
                                    && (strcmp(specified_data_info->md5, g_custom_ota_upgrade_info.md5) == 0))
                                {
                                    check_ok = 1;
                                }
                            }
                            else //if (g_custom_ota_upgrade_state == OTA_STATE_IDLE)
                            {
                                memset(&g_custom_ota_upgrade_info, 0x00, sizeof(t_upgrade_info));
                                g_custom_ota_upgrade_info.update_type = specified_data_info->state;
                                g_custom_ota_upgrade_info.size = specified_data_info->size;
                                strcpy(g_custom_ota_upgrade_info.md5, specified_data_info->md5);
                                strcpy(g_custom_ota_upgrade_info.new_version, specified_data_info->new_version);
                                strcpy(g_custom_ota_upgrade_info.old_version, specified_data_info->old_version);
                                strcpy(g_custom_ota_upgrade_info.dev_type, specified_data_info->dev_type);
                                g_custom_ota_upgrade_state = OTA_STATE_DOWNLOAD;
                                g_custom_ota_upgrade_result = OTA_RESULT_INIT;
                                rt_kprintf("msg APP_CMD_OTA_GET_DATA goto OTA_STATE_DOWNLOAD\r\n");

                                check_ok = 1;
                            }
                            if (check_ok)
                            {
                                if (g_custom_ota_upgrade_data_info.block_recv_mask == NULL)
                                {
                                    unsigned int block_recv_mask_size = g_custom_ota_upgrade_info.size / CUSTOM_OTA_DEFAULT_BLOCK_SIZE / 8 + 1;
                                    g_custom_ota_upgrade_data_info.block_recv_mask = rt_malloc(block_recv_mask_size);
                                    if (g_custom_ota_upgrade_data_info.block_recv_mask != NULL)
                                    {
                                        memset(g_custom_ota_upgrade_data_info.block_recv_mask, 0x00, block_recv_mask_size);
                                        g_custom_ota_upgrade_data_info.block_size = CUSTOM_OTA_DEFAULT_BLOCK_SIZE;
                                        g_custom_ota_upgrade_data_info.block_count = g_custom_ota_upgrade_info.size / CUSTOM_OTA_DEFAULT_BLOCK_SIZE;
                                        if (g_custom_ota_upgrade_info.size % CUSTOM_OTA_DEFAULT_BLOCK_SIZE)
                                        {
                                            g_custom_ota_upgrade_data_info.block_count++;
                                        }
                                        custom_ota_erase_flash(CUSTOM_OTA_FLASH_START_ADDR, g_custom_ota_upgrade_info.size);
                                    }
                                }
                                if (g_custom_ota_upgrade_data_info.block_recv_mask != NULL)
                                {
                                    unsigned int offset = specified_data_info->offset / CUSTOM_OTA_DEFAULT_BLOCK_SIZE;
                                    if (~(g_custom_ota_upgrade_data_info.block_recv_mask[offset / 8] & (1 << (offset % 8))))
                                    {
                                        custom_ota_write_flash(specified_data_info->ota_data, CUSTOM_OTA_FLASH_START_ADDR + specified_data_info->offset, specified_data_info->len);
                                        g_custom_ota_upgrade_data_info.block_recv_mask[offset / 8] |= (1 << (offset % 8));
                                    }
                                    custom_ota_start_update_timer(OTA_TET_WAIT_DATA);
                                    for (offset = 0; offset < g_custom_ota_upgrade_data_info.block_count; offset++)
                                    {
                                        if (~(g_custom_ota_upgrade_data_info.block_recv_mask[offset / 8] & (1 << (offset % 8))))
                                        {
                                            break;
                                        }
                                    }
                                    if (offset >= g_custom_ota_upgrade_data_info.block_count)
                                    {
                                        if (custom_ota_check_flash_data(CUSTOM_OTA_FLASH_START_ADDR, g_custom_ota_upgrade_info.size, g_custom_ota_upgrade_info.md5) == 0)
                                        {
                                            custom_ota_start_update_timer(OTA_TET_WAIT_PROGRAM);
                                            g_custom_ota_upgrade_state = OTA_STATE_PROGRAM;
                                            custom_ota_jump_program();
                                        }
                                        else
                                        {
                                            custom_ota_stop_timer();
                                            g_custom_ota_upgrade_state = OTA_STATE_IDLE;
                                            g_custom_ota_upgrade_result = OTA_RESULT_FAIL;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    rt_free(specified_data_info);
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

    case APP_CMD_OTA_STOP:
        if (1)
        {
            rt_kprintf("msg APP_CMD_OTA_STOP\r\n");
            if ((g_custom_ota_upgrade_state == OTA_STATE_DOWNLOAD)
                || (g_custom_ota_upgrade_state == OTA_STATE_PROGRAM))
            {
                g_custom_ota_upgrade_state = OTA_STATE_IDLE;
                g_custom_ota_upgrade_result = OTA_RESULT_STOP;
                custom_ota_stop_timer();
            }
            if (recv_data_info->need_response == 1)
            {
                t_send_data_info respond_data_info;

                manager_respond_data_info_init(&respond_data_info, recv_data_info);
                (void)manager_send_wiota_data(&respond_data_info, NULL, 0, NULL, NULL);
            }
        }
        break;

    case APP_CMD_OTA_STATE:
        if (1)
        {
            if (g_custom_ota_upgrade_state == OTA_STATE_DOWNLOAD)
            {
                t_upgrade_state_info *upgrade_state_info = NULL;
                upgrade_state_info = rt_malloc(sizeof(t_upgrade_state_info));
                if (upgrade_state_info != NULL)
                {
                    memset(upgrade_state_info, 0x00, sizeof(t_upgrade_state_info));
                    int parse_result = custom_parse_ota_upgrade_state_cmd(data, data_len, 
                                    &upgrade_state_info->state, 
                                    upgrade_state_info->new_version, 
                                    upgrade_state_info->old_version, 
                                    upgrade_state_info->dev_type, 
                                    &upgrade_state_info->ota_state);
                    rt_kprintf("custom_parse_ota_upgrade_state_cmd parse_result = %d\r\n", parse_result);
                    rt_kprintf("state = %d\r\n", upgrade_state_info->state);
                    rt_kprintf("new_version = %s\r\n", upgrade_state_info->new_version);
                    rt_kprintf("old_version = %s\r\n", upgrade_state_info->old_version);
                    rt_kprintf("dev_type = %s\r\n", upgrade_state_info->dev_type);
                    rt_kprintf("ota_state = %d\r\n", upgrade_state_info->ota_state);
                    if (parse_result == 0)
                    {
                        if ((upgrade_state_info->state == g_custom_ota_upgrade_info.update_type) 
                            && (strcmp(g_custom_ota_upgrade_info.old_version, upgrade_state_info->old_version) == 0)
                            && (strcmp(g_custom_ota_upgrade_info.new_version, upgrade_state_info->new_version) == 0)
                            && (strcmp(g_custom_ota_upgrade_info.dev_type, upgrade_state_info->dev_type) == 0))
                        {
                            if (upgrade_state_info->ota_state == 1)
                            {
                                custom_ota_start_update_timer(OTA_TET_WAIT_DATA);
                            }
                            else
                            {
                                g_custom_ota_upgrade_state = OTA_STATE_IDLE;
                                g_custom_ota_upgrade_result = OTA_RESULT_FAIL;
                                custom_ota_stop_timer();
                            }
                        }
                    }
                    rt_free(upgrade_state_info);
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
