#ifndef APP_MANAGER_CFG_H_
#define APP_MANAGER_CFG_H_

enum wiota_net_config_index
{
    I_WIOTA_CFG_FREQ_LIST = 0,
    I_WIOTA_CFG_AP_MAX_POWER,
    I_WIOTA_CFG_ID_LEN,
    I_WIOTA_CFG_PN_NUM,
    I_WIOTA_CFG_SYMBOL_LENGTH,
    I_WIOTA_CFG_DLUL_RATIO,
    I_WIOTA_CFG_BT_VALUE,
    I_WIOTA_CFG_GROUP_NUMBER,
    I_WIOTA_CFG_SPECTRUM_IDX,
    I_WIOTA_CFG_SYSTEM_ID,
    I_WIOTA_CFG_SUBSYSTEM_ID,
};

typedef struct
{
    unsigned char freq_list[16];
    char ap_max_pow;
    unsigned char id_len;
    unsigned char pp;
    unsigned char symbol_length;
    unsigned char dlul_ratio;
    unsigned char btvalue;
    unsigned char group_number;
    unsigned char spectrum_idx;
    unsigned int systemid;
    unsigned int subsystemid;
} t_wiota_net_config;

int manager_config_init(void);
unsigned int manager_get_deviceid(void);
void manager_set_deviceid(unsigned int userid);
unsigned int manager_get_wiotaid(void);
void manager_set_wiotaid(unsigned int wiotaid);
unsigned int manager_get_multicast_addr(void);
void manager_set_multicast_addr(unsigned int multicast_addr);
void manager_set_device_type_name(const char *device_type_name);
char *manager_get_device_type_name(void);
void manager_get_hardware_ver(char *hard_version);
void manager_get_software_ver(char *soft_version);
int manager_get_wiota_config(t_wiota_net_config *wiota_config);
int manager_set_wiota_config(unsigned int config_valid_mask, t_wiota_net_config *wiota_config);

#endif
