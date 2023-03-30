/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date                 Author                Notes
 * 20201-8-17     ucchip-wz          v0.00
 */
//#ifdef AT_USING_SERVER
#include <rtthread.h>
#ifdef UC8288_AT_TEST
#ifdef RT_USING_AT
#include <rtdevice.h>
#include <board.h>
#include "uc_wiota_api.h"
#include "at.h"
#include "ati_prs.h"
#include "uc_string_lib.h"
#include "ps_sys.h"


enum at_test_type
{
    AT_TEST_PRINTF = 0,
    AT_TEST_CMD,            // 1
    AT_TEST_STATISTICS,     // 2
    AT_TEST_SLEEP,          // 3
    AT_TEST_TIME,           // 4
    AT_TEST_ALARM_SET,      // 5
    AT_TEST_ALARM_READ,     // 6
    AT_TEST_AUTO_MCS = 7,   // 7
    AT_TEST_AUTO_TX_POW,    // 8
    AT_TEST_SET_AP_POW,     // 9
    AT_TEST_SET_MCS_LIMIT,  // 10
    AT_TEST_SET_TCXO_DIR,   // 11
    AT_TEST_SET_TCXO_OFFSET,// 12
    AT_TEST_SET_OSC,        // 13
};



#define WIOTA_SEND_TIMEOUT 60000
#define WIOTA_WAIT_DATA_TIMEOUT 10000


extern RfStaticInfo_T g_rf_static_info;
extern at_server_t at_get_server(void);
extern void set_rt_kprintf_switch(unsigned char sw);
extern char *parse (char *b, char *f, ...);


static rt_err_t get_char_timeout_test(rt_tick_t timeout, char * chr)
{
    at_server_t at_server = at_get_server();
    return at_server->get_char(at_server, chr, timeout);
}


unsigned int at_test_str_to_dec(unsigned char* str, int len) {
    unsigned int value = 0;
    unsigned char i = 0;
    unsigned char isNegative = FALSE;

    while ((i < len) && (str[i] < '0' || str[i] > '9')) {
        if (str[i] == '-') {
            isNegative = TRUE;
        }
        i++;
    }

    while ((i < len) && (str[i] >= '0') && (str[i] <= '9')) {
        value = value * 10 + (str[i] - '0');
        i++;
    }

    if (isNegative) {
        value = ~value + 1;
    }

    return value;
}


static at_result_t at_test_setup(const char* args)
{
    int type = 0, length = 0, recv_bytes = 0;
    unsigned char * sendbuffer = RT_NULL;
    unsigned char * psendbuffer;
    unsigned int value = 0;

    args = parse ((char*)(++args),"d,d",  &type, &length);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    rt_kprintf("type %d len %d\n",type,length);

    if (length > 0)
    {
//        recv_bytes = length;
        sendbuffer = (unsigned char *)rt_malloc(length);
        if(sendbuffer == NULL)
        {
            at_server_printfln("SEND FAIL");
            return AT_RESULT_NULL;
        }
        psendbuffer = sendbuffer;
        //at_server_printfln("OK");
        at_server_printf(">");
        while(length > 0)
        {
            if(get_char_timeout_test(rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT), (char*)psendbuffer) != RT_EOK)
            {
                at_server_printfln("SEND FAIL");
                rt_free(sendbuffer);
                return AT_RESULT_NULL;
            }
            length--;
            psendbuffer++;
            recv_bytes++;
        }
        //at_server_printfln("Recv %d bytes", recv_bytes);
    }
    else
    {
        sendbuffer = (unsigned char *)rt_malloc(1024);
        psendbuffer = sendbuffer;
        length = 1024;
//        recv_bytes = 1024;
        //at_server_printfln("OK");
        at_server_printf(">");
        while(length > 0)
        {
//            if(get_char_timeout_test(rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT/3), (char*)psendbuffer) != RT_EOK)
            if(get_char_timeout_test(rt_tick_from_millisecond(10), (char*)psendbuffer) != RT_EOK)
            {
                break;
            }
            length--;
            psendbuffer++;
            recv_bytes++;
        }
//        recv_bytes -= length;
        //at_server_printfln("Recv %d bytes", recv_bytes);
    }

    rt_kprintf("actual len %d\n",recv_bytes);

    switch(type)
    {
        case AT_TEST_PRINTF:
            value = at_test_str_to_dec(sendbuffer,recv_bytes);
            set_rt_kprintf_switch(value & 0x1);
            at_server_printfln("DO SUCC");
            break;

        case AT_TEST_CMD:
           if (recv_bytes > 0)
           {
                extern void uart_tool_set_data(unsigned int * data, unsigned int len);
                extern void uart_tool_handle_msg(void);
                uart_tool_set_data((unsigned int *)sendbuffer, recv_bytes);
                uart_tool_handle_msg();
                at_server_printfln("DO SUCC");
           }
           else
           {
                at_server_printfln("DO ERROR");
           }
           break;

        case AT_TEST_STATISTICS:
            if(UC_OP_SUCC == uc_wiota_send_data(sendbuffer, recv_bytes, WIOTA_SEND_TIMEOUT, RT_NULL))
            {
                at_server_printfln("SEND SUCC");
            }
            else
            {
                at_server_printfln("SEND FAIL");
            }

            break;

        case AT_TEST_SLEEP:
//            l1_lpm_read_rtc_time();
            // l1_lpm_sleep_test();
            break;

        case AT_TEST_TIME:
            // l1_lpm_read_rtc_time();
            break;

        case AT_TEST_ALARM_SET:
            l1_lpm_set_alarm_time(20);
            break;

        case AT_TEST_ALARM_READ:
            rt_alarm_read();
            break;

        case AT_TEST_AUTO_MCS:
            value = at_test_str_to_dec(sendbuffer,recv_bytes);
            state_set_auto_mcs(value & 0x1);
            at_server_printfln("DO SUCC");
            break;

        case AT_TEST_AUTO_TX_POW:
            value = at_test_str_to_dec(sendbuffer,recv_bytes);
            state_set_auto_tx_pow(value & 0x1);
            at_server_printfln("DO SUCC");
            break;

        case AT_TEST_SET_AP_POW:
            value = at_test_str_to_dec(sendbuffer,recv_bytes);
            l1_config_set_apmax_power((u8_t)value);
            at_server_printfln("DO SUCC");
            break;

        case AT_TEST_SET_MCS_LIMIT:
            value = at_test_str_to_dec(sendbuffer,recv_bytes);
            uc_wiota_set_data_rate(UC_RATE_NORMAL,(u16_t)value);
            at_server_printfln("DO SUCC");
            break;

        case AT_TEST_SET_TCXO_DIR:
            value = at_test_str_to_dec(sendbuffer,recv_bytes);
            l1_rf_set_tcxo_dir((u8_t)value);
            at_server_printfln("DO SUCC");
            break;

        case AT_TEST_SET_TCXO_OFFSET:
            value = at_test_str_to_dec(sendbuffer,recv_bytes);
            l1_rf_set_tcxo_offset(value);
            at_server_printfln("DO SUCC");
            break;

        case AT_TEST_SET_OSC:
            value = at_test_str_to_dec(sendbuffer,recv_bytes);
            l1_config_set_is_osc((u8_t)value);
            at_server_printfln("DO SUCC");
            break;

        default:
            at_server_printfln("DO ERROR");
            break;
    }

    if (sendbuffer) {
        rt_free(sendbuffer);
        sendbuffer = RT_NULL;
    }

    return AT_RESULT_OK;
}


static at_result_t at_wiotatest_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_test_loop((unsigned char)mode);

    return AT_RESULT_OK;
}


static at_result_t at_wiotatestlpm_setup(const char *args)
{
    int mode = 0;
    int value = 0;

    args = parse((char *)(++args), "d,d", &mode, &value);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_test_lpm((unsigned char)mode,(unsigned char)value);

    return AT_RESULT_OK;
}


static at_result_t at_wiotabclevel_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_bc_mode((unsigned char)mode);

    return AT_RESULT_OK;
}


static at_result_t at_wiotaflash_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 == mode) {
        at_server_printfln("+FLASH STATUS:0x%x",l1_config_read_flash_sr());
    } else if (1 == mode) {
        l1_config_lock_flash_write();
        // rt_kprintf("flash 1\n");
    } else if (2 == mode) {
        l1_config_unlock_flash_write();
        // rt_kprintf("flash 2\n");
    }

    return AT_RESULT_OK;
}


static at_result_t at_wiotaramp_setup(const char *args)
{
    int value = 0;

    args = parse((char *)(++args), "d", &value);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 <= value && value <= 1023) {
        l1_rf_change_ramp_value(value);
    }

    return AT_RESULT_OK;
}


static at_result_t at_wiotarfcmd_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    if (0 <= mode && mode <= 3) {
        // for rf test, 0:open rx, 1: close rx, 2: open tx, 3:close tx
        l1_rf_handle_rx_raw(mode);
    }

    return AT_RESULT_OK;
}


static at_result_t at_wiotarfmode_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    if (0 <= mode && mode <= 3) {
        g_rf_static_info.tx_mode = mode;
        state_set_tx_mode_for_cce(mode);
    }

    return AT_RESULT_OK;
}


AT_CMD_EXPORT("AT+TEST", "=<type>,<len>", RT_NULL, RT_NULL, at_test_setup, RT_NULL);
// below is for inter test !
AT_CMD_EXPORT("AT+WIOTATEST", "=<mode>", RT_NULL, RT_NULL, at_wiotatest_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTATESTLPM", "=<mode>,<value>", RT_NULL, RT_NULL, at_wiotatestlpm_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTABCLEVEL", "=<mode>", RT_NULL, RT_NULL, at_wiotabclevel_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAFLASH", "=<mode>", RT_NULL, RT_NULL, at_wiotaflash_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASETRAMP", "=<value>", RT_NULL, RT_NULL, at_wiotaramp_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASETRF", "=<mode>", RT_NULL, RT_NULL, at_wiotarfcmd_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTATXMODE", "=<mode>", RT_NULL, RT_NULL, at_wiotarfmode_setup, RT_NULL);

#endif

#endif
