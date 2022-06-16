#ifndef APP_MANAGER_PROTOCOL_H_
#define APP_MANAGER_PROTOCOL_H_

#include "app_manager_cfg.h"

int manager_parse_multicast_addr_cmd(unsigned char *data, unsigned int data_len, unsigned int *multicast_addr);
int manager_parse_config_cmd(unsigned char *data, unsigned int data_len, unsigned int *config_valid_mask, t_wiota_net_config *wiota_config);
int manager_parse_change_addr_request_cmd(unsigned char *data, unsigned int data_len, unsigned int *new_addr);
unsigned char *manager_create_multicast_addr_request_data(const char *device_type_name);
void manager_delete_multicast_addr_request_data(unsigned char *data);
unsigned char *manager_create_config_data(t_wiota_net_config *wiota_config);
void manager_delete_config_data(unsigned char *data);
void manager_system_recv_data_process(t_recv_data_info *recv_data_info, unsigned char *data, unsigned int data_len);

#endif
