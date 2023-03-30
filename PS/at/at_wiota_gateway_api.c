#include <rtthread.h>
#ifdef AT_WIOTA_GATEWAY_API
#ifdef RT_USING_AT
#ifdef UC8288_MODULE
#ifdef AT_WIOTA_GATEWAY
#ifdef _RT_THREAD_
#include <rtdevice.h>
#endif
#include "string.h"
#include "ati_prs.h"
#include "at.h"
#include "uc_wiota_api.h"
#include "uc_wiota_gateway_api.h"


#define AUTH_KEY_LEN            18
#define WIOTA_GATEWAY_WAIT_DATA_TIMEOUT 10000

extern void at_wiota_get_avail_freq_list(unsigned char *output_list, unsigned char list_len);

static void user_recv_data(void *data, unsigned int len, unsigned char data_type)
{
    at_server_printf("+GATEWAYRECV:0x%x, %d, %s\n", data_type, len, data);
}

static void user_get_exception_state(unsigned char exception_type)
{
    at_server_printf("+GATEWAYEXCEPTIONTYPE:0x%x\n", exception_type);
}

static at_result_t at_wiota_gateway_api_init(const char *args)
{
    char auth_key[AUTH_KEY_LEN] = {0xFF};
    unsigned char auth_key_len = 0;
    int init_state = 0;
    unsigned mode = 0;
    unsigned char list[8] = {0xff};

    args = parse((char *)(++args), "d,s", &mode, AUTH_KEY_LEN, auth_key);
    if (!args)
    {
        //rt_kprintf("at_wiota_gateway_api_init error para.\n");
        return AT_RESULT_PARSE_FAILE;
    }

    if (mode != UC_GATEWAY_MODE && mode != UC_TRANSMISSION_MODE)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    auth_key_len = rt_strlen(auth_key);
    auth_key[auth_key_len -2] = '\0';   //  remove '\r\n'
    auth_key[auth_key_len -1] = '\0';

    uc_wiota_gateway_register_user_recv_cb(user_recv_data, user_get_exception_state);
    //uc_wiota_get_freq_list(list);
    #ifdef RT_USING_AT
        at_wiota_get_avail_freq_list(list, APP_CONNECT_FREQ_NUM);
    #endif
    
    
    init_state = uc_wiota_gateway_start(mode, auth_key, list);
    if(UC_GATEWAY_OK == init_state)
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_gateway_api_deinit(void)
{
    int ret = 0;
    
    ret = uc_wiota_gateway_end();
    if(ret)
    {
        return AT_RESULT_OK;
    }

    //rt_kprintf("at_wiota_gateway_api_deinit failed.\n");
    return AT_RESULT_FAILE;
}

extern at_server_t at_get_server(void);

static rt_err_t gateway_get_char_timeout(rt_tick_t timeout, char *chr)
{
    at_server_t at_server = at_get_server();
    return at_server->get_char(at_server, chr, timeout);
}


static at_result_t at_wiota_gateway_api_send_data(const char *args)
{
    unsigned int timeout = 0;
    unsigned int buf_len = 0;
    unsigned int data_len = 0;
    int send_state = 0;
    char send_buf[GATEWAY_SEND_BUF_MAX_LEN] = {0};
    char *psendbuffer = send_buf;

    args = parse((char *)(++args), "d,d", &timeout, &buf_len, GATEWAY_SEND_BUF_MAX_LEN);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    data_len = buf_len;

    at_server_printf(">");

    while (buf_len)
    {
        if (gateway_get_char_timeout(rt_tick_from_millisecond(WIOTA_GATEWAY_WAIT_DATA_TIMEOUT), (char *)psendbuffer) != RT_EOK)
        {
            at_server_printfln("SEND FAIL");
            return AT_RESULT_NULL;
        }
        buf_len--;
        psendbuffer++;
    }

    send_state = uc_wiota_gateway_send_data(send_buf, data_len, timeout);
    if(send_state == 0)
    {
        at_server_printfln("+SEND ERROR");
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_gateway_api_ota_req(void)
{
    int ret = 0;

    ret = uc_wiota_gateway_ota_req();
    if(ret)
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_gateway_api_report_state(void)
{
    int ret = 0;

    ret = uc_wiota_gateway_state_update_info_msg();
    if(ret)
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_gateway_set_ota_req_period(const char *args)
{
    unsigned int ota_req_period = 0;
    int send_state = 0;

    args = parse((char *)(++args), "d", &ota_req_period);

    send_state = uc_wiota_gateway_set_ota_period(ota_req_period);
    if(send_state == 0)
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

AT_CMD_EXPORT("AT+GATEWAYINIT", "=<mode>,<auth_key>", RT_NULL, RT_NULL, at_wiota_gateway_api_init, RT_NULL);
AT_CMD_EXPORT("AT+GATEWAYDEINIT", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_gateway_api_deinit);
AT_CMD_EXPORT("AT+GATEWAYSEND", "=<timeout>,<len>", RT_NULL, RT_NULL, at_wiota_gateway_api_send_data, RT_NULL);
AT_CMD_EXPORT("AT+GATEWAYOTAREQ", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_gateway_api_ota_req);
AT_CMD_EXPORT("AT+GATEWAYREPORTSTATE", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_gateway_api_report_state);
AT_CMD_EXPORT("AT+GATEWAYSETOTAPERIOD", "=<ota_req_period>", RT_NULL, RT_NULL, at_wiota_gateway_set_ota_req_period, RT_NULL);
#endif
#endif
#endif
#endif
