#ifndef _CODING_H_
#define _CODING_H_

typedef enum
{
    PRO_RESERVED = 0,
    PRO_COMPRESS_FLAG = 1,
    PRO_SEGMENT_FLAG = 2,
    PRO_RESPONSE_FLAG = 3,
    PRO_NEED_RES = 4,
    PRO_PACKET_NUM = 5,
    PRO_DEST_ADDR = 6,
    PRO_SRC_ADDR = 7,
} header_property_e;

typedef struct
{
    unsigned char reserved : 1;
    unsigned char compress_flag : 1;
    unsigned char segment_flag : 1;
    unsigned char response_flag : 1;
    unsigned char is_need_res : 1;
    unsigned char is_packet_num : 1;
    unsigned char is_dest_addr : 1;
    unsigned char is_src_addr : 1;
} app_ps_property_t;

typedef struct
{
    unsigned int src_addr;
    unsigned int dest_addr;
} app_ps_addr_t;

typedef struct
{
    unsigned short total_num : 15; /* 0 ~ 32767 */
    unsigned short is_end : 1;     /* 0 or 1 */
    unsigned short current_num;    /* 0 ~ 32768 */
} app_ps_segment_info_t;

typedef struct
{
    app_ps_property_t property;
    app_ps_addr_t addr;
    unsigned char packet_num; /* 0 ~ 255 */
    app_ps_segment_info_t segment_info;
    unsigned char cmd_type; /* 0 ~ 255 */
} app_ps_header_t;

#define APP_MAX_CODING_DATA_LEN 512 //(310 - sizeof(app_ps_header_t))
#define APP_MAX_DECODING_DATA_LEN (1024)
#define APP_MAX_FREQ_LIST_NUM 16
#define APP_MAX_IOTE_UPGRADE_NUM 8
#define APP_MAX_IOTE_UPGRADE_STOP_NUM 16
#define APP_MAX_MISSING_DATA_BLOCK_NUM 16
#define APP_CONNECT_FREQ_NUM 8

typedef enum
{
    AUTHENTICATION_REQ = 1,
    AUTHENTICATION_RES = 2,
    AUTHENTICATION_BC_TIME_SLOT = 3,
    VERSION_VERIFY = 10,
    OTA_UPGRADE_REQ = 11,
    OTA_UPGRADE_STOP = 12,
    OTA_UPGRADE_STATE = 13,
    IOTE_MISSING_DATA_REQ = 14,
    IOTE_STATE_UPDATE = 20,
    IOTE_GET_RTC = 21,
    IOTE_USER_DATA = 100,
    IOTE_LOOP_DATA = 101,
    IOTE_VERSION_REQ = 150,
    IOTE_VERSION_RES = 151,
    IOTE_RESPON_STATE = 0xFFFFFFFF,
} app_ps_cmd_e;

typedef struct
{
    int auth_type;                            // 0: identification fixed
    char aut_code[16];                        // key. max len 16.
    unsigned char freq[APP_CONNECT_FREQ_NUM]; //freq list
} app_ps_auth_req_t, *app_ps_auth_req_p;

typedef enum
{
    AUTHENTICATION_SUC = 0,
    AUTHENTICATION_NO_DATA,
    AUTHENTICATION_FAIL,
    AUTHENTICATION_INFO_CHANGE,
    AUTHENTICATION_RECONNECT,
} e_auth_state;

typedef enum
{
    GATEWAY_OTA_END = 0,
    GATEWAY_OTA_UPGRADING = 1,
    GATEWAY_OTA_NOT_UPGRADE = 2,
} gateway_ota_state_e;

typedef struct
{
    //e_auth_state
    unsigned char state;
    unsigned char freq;
    unsigned short time_slot_fn;
} app_connect_res_t, *app_connect_res_p;

typedef struct
{
    app_connect_res_t connect_index; // auth state. eg: e_auth_state
    unsigned int wiota_id;
    unsigned char freq_list[APP_MAX_FREQ_LIST_NUM]; // if 255, means end
} app_ps_auth_res_t, *app_ps_auth_res_p;

typedef struct
{
    char software_version[16];
    char hardware_version[16];
    char device_type[12];
} app_ps_version_verify_t, *app_ps_version_verify_p;

typedef struct
{
    int upgrade_type; // 0: full upgrade, 1: diff upgrade
    int upgrade_range;
    int iote_list[APP_MAX_IOTE_UPGRADE_NUM]; // if 0, means end
    char new_version[16];                    // upgrade to which version
    char old_version[16];                    // which version who need upgrade
    char md5[36];
    int file_size;
    int upgrade_time;
    char device_type[12];
    int data_offset;
    unsigned short data_length;
    unsigned short packet_type; // 0:upgrade packet, 1:miss data packet
    unsigned short mcs_len;
    unsigned short upgrade_num;
    unsigned char data[940]; // do not cbor coding this data ?
} app_ps_ota_upgrade_req_t, *app_ps_ota_upgrade_req_p;

typedef struct
{
    unsigned int iote_list[APP_MAX_IOTE_UPGRADE_STOP_NUM];
} app_ps_ota_upgrade_stop_t, *app_ps_ota_upgrade_stop_p;

typedef struct
{
    int upgrade_type;     // 0: full upgrade, 1: diff upgrade
    char new_version[16]; // upgrade to which version
    char old_version[16]; // which version who need upgrade
    char device_type[12];
    int process_state; // 0: end, 1: is upgrading, 2: not upgraded
} app_ps_ota_upgrade_state_t, *app_ps_ota_upgrade_state_p;

typedef struct
{
    char device_type[12];
    char new_version[16]; // upgrade to which version
    char old_version[16]; // which version who need upgrade
    int upgrade_type;     // 0: full upgrade, 1: diff upgrade
    int miss_data_num;    // number of missing data blocks, not trans this value
    int miss_data_offset[APP_MAX_MISSING_DATA_BLOCK_NUM];
    int miss_data_length[APP_MAX_MISSING_DATA_BLOCK_NUM];
} app_ps_iote_missing_data_req_t, *app_ps_iote_missing_data_req_p;

typedef struct
{
    char device_type[12];
    char cur_version[16];
    unsigned char freq;
    unsigned char rssi;
    signed char snr;
    signed char cur_power;
    signed char min_power;
    signed char max_power;
    unsigned char cur_mcs;
    unsigned char max_mcs;
} app_ps_iote_state_update_t, *app_ps_iote_state_update_p;

typedef struct
{
    unsigned int dev_id;
    unsigned char version[16];
} app_ps_iote_version_t;

typedef struct
{
    unsigned int pof;
    unsigned char auth_period;
    unsigned char resend_times;
    unsigned char mcs;
    unsigned char na;
    unsigned int frame_num;
} app_ps_ts_info_t;

//unsigned char app_packet_num(void);

/*********************************************************************************
 This function is to set property of app ps header

 param:
        in:
            bit : header_property_e, 0 ~ 7
            value : 0 or 1
        out:
            ps_property : header property
 return:NULL.
**********************************************************************************/
void app_set_header_property(header_property_e bit, unsigned char value, app_ps_property_t *ps_property);

/*********************************************************************************
 This function is to coding app data

 param:
        in:
            ps_header : ps header, user fill
            input_data : data segment
            input_data_len : length of data segment
        out:
            output_data : encoded data, need manual release after use
            output_data_len : length of encoded data
 return:
        0: coding suc
        other value: coding failed
**********************************************************************************/
int app_data_coding(app_ps_header_t *ps_header,
                    unsigned char *input_data,
                    unsigned int input_data_len,
                    unsigned char **output_data,
                    unsigned int *output_data_len);

/*********************************************************************************
 This function is to decoding app data

 param:
        in:
            input_data : data segment
            input_data_len : length of data segment
        out:
            output_data : decoded data, need manual release after use
            output_data_len : length of decoded data
            ps_header : decoded ps header
 return:
        0: decoding suc
        other value: decoding failed
**********************************************************************************/
int app_data_decoding(unsigned char *input_data,
                      unsigned int input_data_len,
                      unsigned char **output_data,
                      unsigned int *output_data_len,
                      app_ps_header_t *ps_header);

/*********************************************************************************
 This function is decod data of cbor

 param:
        in:
            input_cmd_type  cmd
        out:
            output_data : decod of data. must free memory.
 return:int.
**********************************************************************************/

//int app_cmd_decoding(app_ps_cmd_e input_cmd_type,
//                     unsigned char *input_data,
//                     unsigned int input_data_len,
//                     unsigned char **output_data);

/*********************************************************************************
 This function is coding data of cbor

 param:
        in:
            input_cmd_type  cmd
        out:
            output_data : coding of data. must free memory.
 return:int.
**********************************************************************************/

//int app_cmd_coding(app_ps_cmd_e input_cmd_type,
//                   unsigned char *input_cmd,
//                   unsigned char **output_data,
//                   unsigned int *output_data_len);

#endif
