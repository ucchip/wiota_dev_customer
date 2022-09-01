#ifndef _CUSTOM_PROTOCOL_H_
#define _CUSTOM_PROTOCOL_H_

#include "app_manager_logic.h"
#include "custom_manager.h"
#include "custom_protocol.h"
#include "custom_pair.h"

int custom_parse_pair_info_cmd(unsigned char *data, unsigned int data_len, pair_info_t *get_pair_info, unsigned int pair_info_max_count, unsigned char *get_pair_info_count);
unsigned char *custom_create_check_version_data(char *soft_version, char *hard_version, char *dev_type);
void custom_delete_check_version_data(unsigned char *data);
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
                                   char *port);
unsigned char *custom_create_ota_request_specified_data(char *dev_type,
                                                        char *old_version,
                                                        char *new_version,
                                                        unsigned char update_type,
                                                        unsigned int data_info_count,
                                                        unsigned int *offset,
                                                        unsigned int *len);
void custom_delete_ota_request_specified_data(unsigned char *data);
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
                                                unsigned char *ota_data);
int custom_parse_ota_upgrade_state_cmd(unsigned char *data, unsigned int data_len,
                                       unsigned char *state,
                                       char *new_version,
                                       char *old_version,
                                       char *dev_type,
                                       unsigned char *ota_state);

unsigned char *custom_create_light_property_data(void);
void custom_delete_light_property_data(unsigned char *data);
unsigned char *custom_create_light_state_data(e_light_state *state);
void custom_delete_light_state_data(unsigned char *data);
unsigned char *custom_create_light_pair_data(void);
void custom_delete_light_pair_data(unsigned char *data);
int custom_parse_light_ctrl_cmd(unsigned char *data, unsigned int data_len, unsigned char *get_type, unsigned char *get_index, unsigned char *get_ctrl_action);
void custom_light_recv_data_process(t_recv_data_info *recv_data_info, unsigned char *data, unsigned int data_len);

unsigned char *custom_create_switch_property_data(unsigned char sw_count);
void custom_delete_switch_property_data(unsigned char *data);
unsigned char *custom_create_switch_state_data(unsigned char sw_state_mask);
void custom_delete_switch_state_data(unsigned char *data);
unsigned char *custom_create_switch_pair_data(unsigned char sw_index);
void custom_delete_switch_pair_data(unsigned char *data);
unsigned char *custom_create_switch_ctrl_light_data(unsigned char sw_index, unsigned char ctrl_action);
void custom_delete_switch_ctrl_light_data(unsigned char *data);
void custom_switch_recv_data_process(t_recv_data_info *recv_data_info, unsigned char *data, unsigned int data_len);

#endif
