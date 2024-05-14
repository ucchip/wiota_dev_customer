#ifdef AT_WIOTA_GATEWAY_API
#ifndef __AT_WIOTA_GATEWAY_API_H__
#define __AT_WIOTA_GATEWAY_API_H__
#include "uc_coding.h"
#include "uc_wiota_api.h"

#define GATEWAY_SEND_MAX_LEN        (300)
// #define GATEWAY_SEND_BUF_MAX_LEN    (293)
typedef enum
{
    UC_GW_NO_CONNECT = 0,
    UC_GW_ATTEMPT_CONNECT,
    UC_GW_CONNECT_FAIL,
    UC_GW_CONNECT_SUCESS,
    UC_GW_DISCONNECT,
} uc_gw_connect_e;


typedef enum
{
    UC_GATEWAY_MODE = 0,
    UC_TRANSMISSION_MODE = 2,
} uc_gatway_mode_e;


typedef enum {
    UC_GATEWAY_OK = 0,
    UC_GATEWAY_NO_CONNECT,
    UC_GATEWAY_TIMEOUT,
    UC_GATEWAY_NO_KEY,
    UC_CREATE_QUEUE_FAIL,
    UC_CREATE_MISSTIMER_FAIL, // 5
    UC_CREATE_OTATIMER_FAIL,
    UC_CREATE_TASK_FAIL,
    UC_GATEWAY_AUTO_FAIL,
    UC_GATEWAY_OTHER_FAIL,
    UC_GATEWAY_SEDN_FAIL, // 10
}uc_gateway_start_result;


typedef enum
{
    GATEWAY_DEFAULT = 0,
    GATEWAY_NORMAL = 1,
    GATEWAY_FAILED = 2,
    GATEWAY_END = 3,
    GATEWAY_RECONNECT = 4
} uc_gateway_state_t;

typedef void(*uc_wiota_gateway_user_recv_cb)(void *data, unsigned int len, unsigned char data_type);
typedef void(*uc_wiota_gateway_state_report_cb)(unsigned char exception_type);
typedef void(*uc_wiota_gateway_get_rtc_cb)(unsigned int fmt, time_t time);

// user api
int uc_wiota_gateway_start(uc_gatway_mode_e mode, char *auth_key, unsigned char freq_list[APP_CONNECT_FREQ_NUM]);
int uc_wiota_gateway_send_data(void *data, unsigned int data_len, unsigned int timeout);
int uc_wiota_gateway_state_update_info_msg(void);
int uc_wiota_gateway_register_user_recv_cb(uc_wiota_gateway_user_recv_cb user_recv_cb, uc_wiota_gateway_state_report_cb user_get_state_report_cb);
int uc_wiota_gateway_register_get_rtc_cb(unsigned int fmt, uc_wiota_gateway_get_rtc_cb get_rtc_cb);
int uc_wiota_gateway_get_rtc(unsigned int fmt, uc_wiota_gateway_get_rtc_cb get_rtc);
int uc_wiota_gateway_ota_req(void);
int uc_wiota_gateway_end(void);
uc_gateway_state_t uc_wiota_gateway_get_state(void);
int uc_gateway_get_random(void);
unsigned int uc_wiota_gateway_get_dev_address(void);
unsigned char uc_wiota_gateway_get_time_slot_fn(void);

#endif
#endif
