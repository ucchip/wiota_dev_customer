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
#include "at_wiota.h"
#include "uc_wiota_api.h"
#include "uc_wiota_gateway_api.h"
#include "uc_ota_flash.h"
#include "uc_wiota_static.h"

#define AUTH_KEY_LEN 18
#define WIOTA_GATEWAY_WAIT_DATA_TIMEOUT 10000

extern void at_wiota_get_avail_freq_list(unsigned char *output_list, unsigned char list_len);

static void user_recv_data(void *data, unsigned int len, unsigned char data_type)
{
    at_server_printf("+GATEWAYRECV:%d, %d,", data_type, len);
    at_send_data(data, len);
    at_server_printfln("");
}

static void user_get_exception_state(unsigned char exception_type)
{
    const char *exception_string[7] = {
        "GATEWAY_DEFAULT",
        "GATEWAY_NORMAL",
        "GATEWAY_FAILED",
        "GATEWAY_END",
        "GATEWAY_RECONNECT",
        "GATEWAY_OTA_UPGRADE",
        "GATEWAY_OTA_PROGRAMING"};

    at_server_printfln("+GATEWAYSTATE:%s", exception_string[exception_type]);
}

static void extern_mcu_cb(unsigned int start_addr, unsigned int len, char *md5)
{
    at_server_printfln("+EXMCUOTA:0x%x,%u,%s", start_addr, len, md5);
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
        rt_kprintf("gw f para\n");
        return AT_RESULT_PARSE_FAILE;
    }

    if (mode != UC_GATEWAY_MODE && mode != UC_TRANSMISSION_MODE && mode != UC_GATEWAY_TIME_SLOT_MODE)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    auth_key_len = rt_strlen(auth_key);
    auth_key[auth_key_len - 2] = '\0'; //  remove '\r\n'
    auth_key[auth_key_len - 1] = '\0';

    uc_wiota_gateway_register_user_recv_cb(user_recv_data, user_get_exception_state);
    uc_wiota_gateway_register_ex_mcu_cb(extern_mcu_cb);
// uc_wiota_get_freq_list(list);
#ifdef RT_USING_AT
    at_wiota_get_avail_freq_list(list, APP_CONNECT_FREQ_NUM);
#endif

    init_state = uc_wiota_gateway_start(mode, auth_key, list);

    // rt_kprintf("%s line %d init_state %d\n", __FUNCTION__, __LINE__, init_state);
    rt_thread_delay(2);
    if (UC_GATEWAY_OK == init_state)
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_gateway_api_deinit(void)
{
    uc_wiota_gateway_end();

    return AT_RESULT_OK;
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
    // add crc data len
    char send_buf[GATEWAY_SEND_MAX_LEN + 2] = {0};
    char *psendbuffer = send_buf;

    args = parse((char *)(++args), "d,d", &timeout, &buf_len);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (buf_len > GATEWAY_SEND_MAX_LEN)
    {
        rt_kprintf("len %d err\n", buf_len);
        return AT_RESULT_PARSE_FAILE;
    }

    data_len = buf_len;

    at_server_printf(">");

    while (buf_len)
    {
        if (gateway_get_char_timeout(rt_tick_from_millisecond(WIOTA_GATEWAY_WAIT_DATA_TIMEOUT), (char *)psendbuffer) != RT_EOK)
        {
            at_server_printfln("WAIT TIMEOUT");
            return AT_RESULT_NULL;
        }
        buf_len--;
        psendbuffer++;
    }

    send_state = uc_wiota_gateway_send_data(send_buf, data_len, timeout);
    if (send_state == 0)
    {
        at_server_printfln("SEND FAIL");
        return AT_RESULT_FAILE;
    }

    at_server_printfln("SEND SUCC");
    return AT_RESULT_OK;
}

static at_result_t at_wiota_gateway_api_ota_req(void)
{
    int ret = 0;

    ret = uc_wiota_gateway_ota_req();
    if (ret)
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_gateway_ota_state_query(void)
{
    at_server_printfln("+GATEWAYOTASTATE:%d,%d", uc_wiota_gateway_get_ota_state(), uc_wiota_gateway_get_ota_recved_len());

    return AT_RESULT_OK;
}

static at_result_t at_wiota_gateway_api_report_state(void)
{
    int ret = 0;

    ret = uc_wiota_gateway_state_update_info_msg();
    if (ret)
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

static void at_get_rtc_callbcak(unsigned int fmt, time_t time)
{
    if (fmt == 0)
    {
        at_server_printfln("+GATEWAYRTC:%u", time);
    }
    else
    {
        at_server_printfln("+GATEWAYRTC:%s", ctime((const time_t *)&time));
    }
}

static at_result_t at_wiota_gateway_api_get_rtc(const char *args)
{
    unsigned int fmt = 0;

    args = parse((char *)(++args), "d", &fmt);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_gateway_get_rtc(fmt, at_get_rtc_callbcak))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_gateway_verity_setup(const char *args)
{
    unsigned int is_open = 0;

    args = parse((char *)(++args), "d", &is_open);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_gateway_set_verity(is_open);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_gateway_ex_mcu_read(const char *args)
{
    unsigned int read_addr = 0;
    unsigned int read_len = 0;
    unsigned char *buffer = RT_NULL;

    args = parse((char *)(++args), "y,d", &read_addr, &read_len);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (read_len > 512 || uc_wiota_gateway_get_ota_state() == 1)
    {
        return AT_RESULT_FAILE;
    }

    buffer = rt_malloc(read_len);
    RT_ASSERT(buffer);

    uc_wiota_suspend_connect();
    rt_thread_mdelay(uc_wiota_get_frame_len() / 1000 + 2);
    if (uc_wiota_ota_flash_read(read_addr, buffer, read_len) != read_len)
    {
        rt_free(buffer);
        uc_wiota_recover_connect();
        return AT_RESULT_FAILE;
    }
    uc_wiota_recover_connect();

    at_server_printfln("+GATEWAYEXMCU:%d", read_len);
    at_send_data(buffer, read_len);
    at_server_printfln("");

    rt_free(buffer);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_gateway_quick_connect(const char *args)
{
    unsigned int freq = 0;
    unsigned int subsystemid = 0;
    unsigned int is_force_active = 0;
    unsigned int mode = 0;
    sub_system_config_t config = {0};
    uc_gw_info_t gw_info = {0};
    unsigned char list[8] = {0xff};
    int init_state = 0;

    args = parse((char *)(++args), "d,y,d,d", &freq, &subsystemid, &is_force_active, &mode);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (mode != UC_GATEWAY_MODE && mode != UC_TRANSMISSION_MODE && mode != UC_GATEWAY_TIME_SLOT_MODE)
    {
        return AT_RESULT_CHECK_FAILE;
    }

    // init
    uc_wiota_init();
    at_wiota_set_state(AT_WIOTA_INIT);
    // set freq
    if (freq != 0 && freq != uc_wiota_get_freq_info())
    {
        uc_wiota_set_freq_info(freq);
    }
    // set cfg
    uc_wiota_get_system_config(&config);
    if (subsystemid != 0 && subsystemid != config.subsystemid)
    {
        config.subsystemid = subsystemid;
        uc_wiota_set_system_config(&config);
    }
    // run
    uc_wiota_run();
    at_wiota_set_state(AT_WIOTA_RUN);
    // connect
    uc_wiota_connect_quick(is_force_active);
    uc_gateway_set_force_actice(is_force_active);

    uc_wiota_get_gateway_info(&gw_info);
    uc_wiota_set_sm_resend_times(gw_info.resend_times);
    uc_wiota_set_data_rate(0, gw_info.ul_mcs);

    if (0 == is_force_active && 0 != uc_wiota_wait_sync(5000, 0))
    {
        uc_wiota_exit();
        at_wiota_set_state(AT_WIOTA_EXIT);
        return AT_RESULT_FAILE;
    }

    uc_wiota_gateway_register_user_recv_cb(user_recv_data, user_get_exception_state);
    uc_wiota_gateway_register_ex_mcu_cb(extern_mcu_cb);
#ifdef RT_USING_AT
    at_wiota_get_avail_freq_list(list, APP_CONNECT_FREQ_NUM);
#endif

    init_state = uc_wiota_gateway_start(mode, "123456", list);

    rt_thread_delay(2);

    if (UC_GATEWAY_OK != init_state)
    {
        uc_wiota_gateway_end();
        uc_wiota_exit();
        at_wiota_set_state(AT_WIOTA_EXIT);
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_gateway_quick_disconnect(void)
{
    uc_wiota_gateway_end();
    uc_wiota_exit();
    at_wiota_set_state(AT_WIOTA_EXIT);

    return AT_RESULT_OK;
}

// static void uc_gateway_enter_sync_paging(void *para)
// {
//     uc_gateway_enter_sync_paging_by_time_slot();
// }

static at_result_t at_wiota_gateway_send_and_sleep_setup(const char *args)
{
    unsigned int timeout = 0;
    unsigned int buf_len = 0;
    unsigned int is_sleep = 0;
    unsigned int sleep_delay = 0;
    unsigned int data_len = 0;
    int send_state = 0;
    // add crc data len
    char send_buf[GATEWAY_SEND_MAX_LEN + 2] = {0};
    char *psendbuffer = send_buf;
    int result = AT_RESULT_OK;

    args = parse((char *)(++args), "d,d,d,d", &timeout, &buf_len, &is_sleep, &sleep_delay);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (buf_len > GATEWAY_SEND_MAX_LEN)
    {
        return AT_RESULT_CHECK_FAILE;
    }

    data_len = buf_len;

    at_server_printf(">");

    while (buf_len)
    {
        if (gateway_get_char_timeout(rt_tick_from_millisecond(WIOTA_GATEWAY_WAIT_DATA_TIMEOUT), (char *)psendbuffer) != RT_EOK)
        {
            at_server_printfln("WAIT TIMEOUT");
            return AT_RESULT_NULL;
        }
        buf_len--;
        psendbuffer++;
    }

    uc_gateway_set_send_sleep_flag(is_sleep);
    send_state = uc_wiota_gateway_send_data(send_buf, data_len, timeout);
    if (send_state == 0)
    {
        at_server_printfln("SEND FAIL");
        result = AT_RESULT_FAILE;
    }
    else
    {
        at_server_printfln("SEND SUCC");
    }

    if (is_sleep)
    {
        if (sleep_delay > 0)
        {
            rt_thread_mdelay(sleep_delay);
        }
        uc_gateway_enter_sync_paging_by_time_slot();
    }

    return result;
}

AT_CMD_EXPORT("AT+GATEWAYINIT", "=<mode>,<key>", RT_NULL, RT_NULL, at_wiota_gateway_api_init, RT_NULL);
AT_CMD_EXPORT("AT+GATEWAYDEINIT", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_gateway_api_deinit);
AT_CMD_EXPORT("AT+GATEWAYSEND", "=<timeout>,<len>", RT_NULL, RT_NULL, at_wiota_gateway_api_send_data, RT_NULL);
AT_CMD_EXPORT("AT+GATEWAYOTAREQ", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_gateway_api_ota_req);
AT_CMD_EXPORT("AT+GATEWAYOTASTATE", RT_NULL, RT_NULL, at_wiota_gateway_ota_state_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+GATEWAYSTATE", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_gateway_api_report_state);
AT_CMD_EXPORT("AT+GATEWAYRTC", "=<fmt>", RT_NULL, RT_NULL, at_wiota_gateway_api_get_rtc, RT_NULL);
AT_CMD_EXPORT("AT+GATEWAYVERITY", "=<is_open>", RT_NULL, RT_NULL, at_wiota_gateway_verity_setup, RT_NULL);
AT_CMD_EXPORT("AT+GATEWAYEXMCU", "=<addr>,<len>", RT_NULL, RT_NULL, at_wiota_gateway_ex_mcu_read, RT_NULL);
AT_CMD_EXPORT("AT+GATEWAYQC", "=<freq>,<subsysid>,<force_active>,<mode>", RT_NULL, RT_NULL, at_wiota_gateway_quick_connect, RT_NULL);
AT_CMD_EXPORT("AT+GATEWAYQD", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_gateway_quick_disconnect);
AT_CMD_EXPORT("AT+GATEWAYSAS", "=<timeout>,<len>,<is_sleep>,<delay>", RT_NULL, RT_NULL, at_wiota_gateway_send_and_sleep_setup, RT_NULL);

#endif
#endif
#endif
#endif
