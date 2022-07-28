#ifndef APP_MANAGER_LOGIC_H_
#define APP_MANAGER_LOGIC_H_

#include "uc_wiota_api.h"

#define MANAGE_LOGIC_RECV_QUEUE_TIMEOUT 50 //10000
#define MANAGE_LOGIC_SERVER_ADDR 0xFFFFFFF1
#define MANAGE_LOGIC_MULTICAST_ADDR_START 0xE0000000
#define MANAGE_LOGIC_MULTICAST_ADDR_END 0xEFFFFFFE
#define MANAGE_LOGIC_BROADCAST_ADDR 0xEFFFFFFF
#define MANAGE_LOGIC_SYSTEM_CMD_TYPE_START 100
#define MANAGE_LOGIC_SYSTEM_CMD_TYPE_END 159
#define SEND_PACKAGE_FRAME_DATA_MAX_LEN 280
#define RECV_PACKAGE_TIMEOUT 3000
#define MANAGER_WIOTA_SEND_TRY_COUNT 3
#define MANAGER_LOGIC_REQUEST_WIOTA_ADDR_CYCLE_TIME 5000

enum app_ps_cmd_type
{
    //property report
    APP_CMD_PROPERTY_REPORT = 1, // report dev property

    //state report
    APP_CMD_AP_STATE_REPORT = 10,
    APP_CMD_IOTE_STATE_REPORT = 11, // report dev state

    //pair
    APP_CMD_IOTE_PAIR_REQUEST = 30,
    APP_CMD_NET_PAIR_RESPONSE = 31,
    APP_CMD_IOTE_PAIR_REQUEST_CANCEL = 32,
    APP_CMD_NET_PAIR_CANCEL = 33,

    //ctrl
    APP_CMD_NET_CTRL_IOTE = 40,

    //ota
    APP_CMD_OTA_CHECK_VERSION = 50,
    APP_CMD_OTA_GET_DATA = 52,
    APP_CMD_OTA_STOP = 54,
    APP_CMD_OTA_STATE = 56,

    //muticast
    APP_CMD_IOTE_MUTICAST_REQUEST = 100,
    APP_CMD_NET_MUTICAST_ASSIGNMENT = 101,

    //config
    APP_CMD_IOTE_REQUEST_NEW_CONFIG = 120,
    APP_CMD_NET_DELIVER_IOTE_CONFIG = 121,

    //reserved addr
    APP_CMD_RESERVED_ADDR_REQUEST = 122,
    APP_CMD_RESERVED_ADDR_RESPONSE = 123,

    //change addr
    APP_CMD_CHANGE_ADDR_REQUEST = 130,
    APP_CMD_CHANGE_ADDR_RESPONSE = 131,
};

enum manager_logic_cmd
{
    MANAGER_LOGIC_DEFAULT = 0,
    MANAGER_LOGIC_RECV_WIOTA_DATA,         // wiota data send to manager queue.
    MANAGER_LOGIC_RECV_WIOTA_STATE,        // wiota state send to manager queue.
    MANAGER_LOGIC_SEND_WIOTA_DATA,         // send wiota data to gateway.
    MANAGER_LOGIC_SEND_WIOTA_PACKAGE_DATA, // send wiota package data to gateway.
    MANAGER_LOGIC_WIOTA_CONNECTED,         // wiota connected.
    MANAGER_LOGIC_RESET_WIOTA,             // reset wiota.
};

enum e_addr_type
{
    ADDR_TYPE_SERVER = 0,
    ADDR_TYPE_GW,
    ADDR_TYPE_USER_DEFINED,
};

enum e_trans_type
{
    TRANS_TYPE_UNICAST = 0,
    TRANS_TYPE_MULTICAST,
    TRANS_TYPE_BROADCAST,
};

typedef struct app_logic_message
{
    int cmd;
    void *data;
} t_app_logic_message;

typedef struct
{
    unsigned char auto_src_addr : 1;  //0: user-defined addr(use src_addr), 1: use self-existent device id
    unsigned char dest_addr_type : 2; //0: server, 1: gw, 2: user-defined addr(use dest_addr)
    unsigned char need_response : 1;
    unsigned char is_response : 1;
    unsigned char compress_flag : 1;
    unsigned char packet_num_type : 2; //0: no packet num, 1: auto packet num, 2: user-defined packet num

    unsigned char user_packet_num; /* 0 ~ 255 */

    unsigned int src_addr;
    unsigned int dest_addr;

    unsigned char cmd_type;
} t_send_data_info;

typedef struct
{
    unsigned char src_addr_type : 2; //0: server, 1: gw, 2: user-defined addr(use src_addr)
    unsigned char trans_type : 2;    //0: unicast, 1: multicast, 2: broadcast
    unsigned char need_response : 1;
    unsigned char is_response : 1;
    unsigned char compress_flag : 1;
    unsigned char is_packet_num : 1;

    unsigned char packet_num; /* 0 ~ 255 */

    unsigned int src_addr;
    unsigned int dest_addr;

    unsigned char cmd_type;
} t_recv_data_info;

typedef void (*f_custom_data_callback)(t_recv_data_info *recv_data_info, unsigned char *data, unsigned int data_len);
typedef void (*f_send_result_cb)(unsigned int data_id, UC_OP_RESULT send_result);

typedef struct app_send_data
{
    unsigned int data_id;
    unsigned char *data;
    unsigned int data_len;
    f_send_result_cb send_result_cb;
} t_app_send_data;

typedef struct app_send_package_data
{
    unsigned int data_id;
    unsigned char frame_count;
    unsigned char **data;
    unsigned int *data_len;
    f_send_result_cb send_result_cb;
} t_app_send_package_data;

typedef struct app_process_manager
{
    int current_process; // flow of bussess logic
} t_app_process_manager;

typedef struct recv_package
{
    t_recv_data_info recv_info;
    unsigned char packet_num;
    unsigned char frame_count;
    unsigned char *frame_recv_mask;
    unsigned char **frame_data_buf;
    unsigned int *frame_data_len;
    unsigned int package_data_len;
    unsigned int last_recv_tick;
    unsigned int recv_timeout;
    rt_list_t list;
} t_recv_package;

int app_manager_create_logic_queue(void);

int manager_sendqueue_logic(int src_task, int cmd, void *data);

void app_manager_logic(void);

unsigned char manager_get_logic_work_state(void);

void manager_set_custom_data_callback(f_custom_data_callback callback);
void manager_set_custom_ota_callback(f_custom_data_callback callback);
void manager_wiota_connected(void);
void manager_reset_wiota(void);
void manager_respond_data_info_init(t_send_data_info *respond_data_info, t_recv_data_info *recv_data_info);
int manager_send_wiota_data(t_send_data_info *send_data_info, unsigned char *data, unsigned int data_len, f_send_result_cb send_result_cb, unsigned int *get_data_id);
void manager_request_multicast_addr(void);
void manager_request_config(void);
void print_hex_data(unsigned char *hex_data, unsigned int hex_data_len);

void manager_send_lock_init(void);
void manager_send_lock_take(void);
void manager_send_lock_release(void);

void manager_recv_package_list_init(void);

#endif
