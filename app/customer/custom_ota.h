#ifndef _CUSTOMER_OTA_H_
#define _CUSTOMER_OTA_H_

#include "custom_protocol.h"

enum e_ota_state
{
    OTA_STATE_IDLE = 0,
    OTA_STATE_DOWNLOAD,
    OTA_STATE_PROGRAM,
};

enum e_ota_result
{
    OTA_RESULT_INIT = 0,
    OTA_RESULT_SUCC,
    OTA_RESULT_FAIL,
    OTA_RESULT_STOP,
};

enum e_ota_timeout_event_type
{
    OTA_TET_IDLE = 0,
    OTA_TET_WAIT_DATA,
    OTA_TET_WAIT_PROGRAM,
    OTA_TET_EXIT,
};

typedef struct upgrade_info
{
    char new_version[16];
    char old_version[16];
    char dev_type[12];
    unsigned char update_type;
    char file[64];
    unsigned int size;
    char md5[34];
    unsigned char access;
    char username[16];
    char password[16];
    char path[64];
    char address[64];
    char port[16];
} t_upgrade_info;

typedef struct upgrade_data_info
{
    unsigned int block_size;
    unsigned int block_count;
    unsigned char *block_recv_mask;
} t_upgrade_data_info;

typedef struct specified_data_info
{
    unsigned char state;
    unsigned char range;
    unsigned int size;
    char md5[34];
    unsigned int dev_list[256];
    unsigned int dev_list_count;
    char new_version[16];
    char old_version[16];
    char dev_type[12];
    unsigned int offset;
    unsigned int len;
    unsigned char ota_data[1024];
} t_specified_data_info;

typedef struct upgrade_state_info
{
    unsigned char state;
    char new_version[16];
    char old_version[16];
    char dev_type[12];
    unsigned char ota_state;
} t_upgrade_state_info;

void custom_ota_request_check_version(void);
void custom_ota_request_specified_data(void);
void custom_ota_check_version_process(void);
void custom_ota_msg_process(void);
void custom_ota_recv_data_process(t_recv_data_info *recv_data_info, unsigned char *data, unsigned int data_len);

#endif
