#ifdef AT_WIOTA_GATEWAY_API
#ifndef __AT_WIOTA_GATEWAY_API_H__
#define __AT_WIOTA_GATEWAY_API_H__
#include "uc_coding.h"

#define GATEWAY_SEND_BUF_MAX_LEN        293

typedef enum {
    UC_GATEWAY_OK = 0,
    UC_GATEWAY_TIMEOUT,
    UC_GATEWAY_NO_KEY,
    UC_CREATE_QUEUE_FAIL,
    UC_CREATE_MISSTIMER_FAIL,
    UC_CREATE_OTATIMER_FAIL,
    UC_CREATE_TASK_FAIL,
    UC_GATEWAY_AUTO_FAIL,
    UC_GATEWAY_OTHER_FAIL,
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
typedef void(*uc_wiota_gateway_exception_report_cb)(unsigned char exception_type);

// user api
extern int uc_wiota_gateway_start(char *auth_key, unsigned char freq_list[APP_CONNECT_FREQ_NUM]);
extern int uc_wiota_gateway_send_data(void *data, unsigned int data_len, unsigned int timeout);
extern int uc_wiota_gateway_state_update_info_msg(void);
extern int uc_wiota_gateway_register_user_recv_cb(uc_wiota_gateway_user_recv_cb user_recv_cb, uc_wiota_gateway_exception_report_cb user_get_exc_report_cb);
extern int uc_wiota_gateway_ota_req(void);
extern int uc_wiota_gateway_set_ota_period(unsigned int p_tick);
extern int uc_wiota_gateway_end(void);
extern uc_gateway_state_t uc_wiota_gateway_get_state(void);

#endif
#endif
