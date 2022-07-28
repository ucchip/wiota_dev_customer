#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include <board.h>

#include <string.h>
#include "app_manager_cfg.h"
#include "custom_data.h"
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"

static sub_system_config_t g_wiota_config;
static unsigned char g_wiota_freq_list[16];
static unsigned int g_multicast_addr = 0;
static char g_device_type_name[21] = "unknown";

int manager_config_init(void)
{
    unsigned int addr_list[4];
    unsigned char num = 0;

    custom_get_multicast_addr(addr_list, &num);
    if (num)
    {
        g_multicast_addr = addr_list[0];
    }
    else
    {
        g_multicast_addr = 0;
    }

    memset(&g_wiota_config, 0x00, sizeof(g_wiota_config));
    uc_wiota_get_system_config(&g_wiota_config);

    memset(g_wiota_freq_list, 0x00, sizeof(g_wiota_freq_list));
    uc_wiota_get_freq_list(g_wiota_freq_list);

    return 0;
}

unsigned int manager_get_deviceid(void)
{
    unsigned int device_id = 0x0;

    unsigned char dev_type = 0;
    unsigned char count = 0;
    custom_get_devinfo(&dev_type, &count);
    if (dev_type == DEV_TYPE_LIGHT)
    {
        device_id = 0x123456;
    }
    else
    {
        device_id = 0x123459;
    }
    rt_kprintf("manager_get_deviceid = 0x%08X\r\n", device_id);

    return device_id;
}

void manager_set_deviceid(unsigned int userid)
{
    rt_kprintf("manager_set_deviceid = 0x%08X\r\n", userid);
    userid = userid;
}

unsigned int manager_get_wiotaid(void)
{
    unsigned int user_id[2] = {0x0, 0x0};
    unsigned char user_id_len;

    uc_wiota_get_userid(user_id, &user_id_len);
    rt_kprintf("manager_get_wiotaid = 0x%08X\r\n", user_id[0]);

    return user_id[0];
}

void manager_set_wiotaid(unsigned int wiotaid)
{
    rt_kprintf("manager_set_wiotaid start\r\n");
    unsigned int user_id[2] = {0x0, 0x0};

    user_id[0] = wiotaid;
    uc_wiota_set_userid(user_id, 4);
    rt_kprintf("manager_set_wiotaid = 0x%08X\r\n", wiotaid);

    //uc_wiota_save_static_info();
    rt_kprintf("manager_set_wiotaid over\r\n");
}

unsigned int manager_get_multicast_addr(void)
{
    return g_multicast_addr;
    rt_kprintf("manager_get_multicast_addr = 0x%08X\r\n", g_multicast_addr);
}

void manager_set_multicast_addr(unsigned int multicast_addr)
{
    rt_kprintf("manager_set_multicast_addr start\r\n");
    g_multicast_addr = multicast_addr;
    rt_kprintf("manager_set_multicast_addr = 0x%08X\r\n", multicast_addr);
    custom_set_multicast_addr(&multicast_addr, 1);

    uc_wiota_save_static_info();
    rt_kprintf("manager_set_multicast_addr over\r\n");
}

void manager_set_device_type_name(const char *device_type_name)
{
    if (strlen(device_type_name) > 20)
    {
        return;
    }
    memset(g_device_type_name, 0x00, 21);
    strcpy(g_device_type_name, device_type_name);
}

char *manager_get_device_type_name(void)
{
    return g_device_type_name;
}

void manager_get_hardware_ver(char *hard_version)
{
    uc_wiota_get_hardware_ver((unsigned char *)hard_version);
}

void manager_get_software_ver(char *soft_version)
{
    strcpy(soft_version, "NULL");
}

int manager_get_wiota_config(t_wiota_net_config *wiota_config)
{
    memcpy(wiota_config->freq_list, &g_wiota_freq_list, sizeof(g_wiota_freq_list));

    wiota_config->ap_max_pow = g_wiota_config.ap_max_pow;
    wiota_config->id_len = g_wiota_config.id_len;
    wiota_config->pn_num = g_wiota_config.pn_num;
    wiota_config->symbol_length = g_wiota_config.symbol_length;
    wiota_config->dlul_ratio = g_wiota_config.dlul_ratio;
    wiota_config->btvalue = g_wiota_config.btvalue;
    wiota_config->group_number = g_wiota_config.group_number;
    wiota_config->spectrum_idx = g_wiota_config.spectrum_idx;
    wiota_config->systemid = g_wiota_config.systemid;
    wiota_config->subsystemid = g_wiota_config.subsystemid;

    return 0;
}

int manager_set_wiota_config(unsigned int config_valid_mask, t_wiota_net_config *wiota_config)
{
    unsigned char freq_list_update = 0;
    unsigned char wiota_config_update = 0;

    rt_kprintf("manager_set_wiota_config start\r\n");
    if (config_valid_mask & (1 << I_WIOTA_CFG_FREQ_LIST))
    {
        memcpy(g_wiota_freq_list, wiota_config->freq_list, sizeof(g_wiota_freq_list));
        freq_list_update = 1;
    }
    if (config_valid_mask & (1 << I_WIOTA_CFG_AP_MAX_POWER))
    {
        g_wiota_config.ap_max_pow = wiota_config->ap_max_pow;
        wiota_config_update = 1;
    }
    if (config_valid_mask & (1 << I_WIOTA_CFG_ID_LEN))
    {
        g_wiota_config.id_len = wiota_config->id_len;
        wiota_config_update = 1;
    }
    if (config_valid_mask & (1 << I_WIOTA_CFG_PN_NUM))
    {
        g_wiota_config.pn_num = wiota_config->pn_num;
        wiota_config_update = 1;
    }
    if (config_valid_mask & (1 << I_WIOTA_CFG_SYMBOL_LENGTH))
    {
        g_wiota_config.symbol_length = wiota_config->symbol_length;
        wiota_config_update = 1;
    }
    if (config_valid_mask & (1 << I_WIOTA_CFG_DLUL_RATIO))
    {
        g_wiota_config.dlul_ratio = wiota_config->dlul_ratio;
        wiota_config_update = 1;
    }
    if (config_valid_mask & (1 << I_WIOTA_CFG_BT_VALUE))
    {
        g_wiota_config.btvalue = wiota_config->btvalue;
        wiota_config_update = 1;
    }
    if (config_valid_mask & (1 << I_WIOTA_CFG_GROUP_NUMBER))
    {
        g_wiota_config.group_number = wiota_config->group_number;
        wiota_config_update = 1;
    }
    if (config_valid_mask & (1 << I_WIOTA_CFG_SPECTRUM_IDX))
    {
        g_wiota_config.spectrum_idx = wiota_config->spectrum_idx;
        wiota_config_update = 1;
    }
    if (config_valid_mask & (1 << I_WIOTA_CFG_SYSTEM_ID))
    {
        g_wiota_config.systemid = wiota_config->systemid;
        wiota_config_update = 1;
    }
    if (config_valid_mask & (1 << I_WIOTA_CFG_SUBSYSTEM_ID))
    {
        g_wiota_config.subsystemid = wiota_config->subsystemid;
        wiota_config_update = 1;
    }

    if (freq_list_update)
    {
        uc_wiota_set_freq_list(g_wiota_freq_list, sizeof(g_wiota_freq_list));
    }
    if (wiota_config_update)
    {
        uc_wiota_set_system_config(&g_wiota_config);
    }

    uc_wiota_save_static_info();
    rt_kprintf("manager_set_wiota_config over\r\n");

    return 0;
}

#endif
