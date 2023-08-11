/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-04-01     armink       first version
 * 2018-04-04     chenyong     add base commands
 */
#include <rtthread.h>
#ifdef UC8288_MODULE

#include <at.h>
#include <stdlib.h>
#include <string.h>
#include <uc_pulpino.h>
#include <rtdevice.h>
#include "ati_prs.h"

//#include "uc_boot_download.h"

#ifdef AT_USING_SERVER

#define AT_ECHO_MODE_CLOSE             0
#define AT_ECHO_MODE_OPEN              1


#define REG_WATCHDOG_EN    (WATCHDOG_BASE_ADDR + 0x00)
#define REG_WATCHDOG_INITVAL (WATCHDOG_BASE_ADDR + 0x04)
#define REG_WATCHDOG_FEED (WATCHDOG_BASE_ADDR + 0x08)
#define reg_xip_ctrl   ((volatile uint32_t *) 0x1a10c02c)
#define WAIT_XIP_FREE    while((*reg_xip_ctrl)&0x1)

extern at_server_t at_get_server(void);
extern void uc_wiota_set_at_baud_rate(unsigned int baud_rate);

static at_result_t at_exec(void)
{
    return AT_RESULT_OK;
}
/*
static at_result_t atz_exec(void)
{
    at_server_printfln("OK");

    at_port_factory_reset();

    return AT_RESULT_NULL;
}
*/
#define AT_WDT_DEVICE_NAME    "wdt"

//static int watchdog_reset(void)
//{
//    rt_err_t ret = RT_EOK;
//    rt_uint32_t timeout = 1;
//    rt_device_t at_wdg_dev = rt_device_find(AT_WDT_DEVICE_NAME);
//    if (!at_wdg_dev)
//    {
//        rt_kprintf("find %s failed!\n", AT_WDT_DEVICE_NAME);
//        return 1;
//    }
//
//    ret = rt_device_control(at_wdg_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
//    if (ret != RT_EOK)
//    {
//        rt_kprintf("set %s timeout failed!\n", AT_WDT_DEVICE_NAME);
//        return 2;
//    }
//
//    if (rt_device_control(at_wdg_dev, RT_DEVICE_CTRL_WDT_START, RT_NULL) != RT_EOK)
//    {
//        rt_kprintf("start %s failed!\n", AT_WDT_DEVICE_NAME);
//        return 3;
//    }
//
//    rt_device_control(at_wdg_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, NULL);
//
//    return 0;
//}



void reset_8288(void)
{
	WAIT_XIP_FREE;
	REG(REG_WATCHDOG_INITVAL) = 0xFFFFFFFF - 1;
	REG(REG_WATCHDOG_FEED) |= 0x1;
	REG(REG_WATCHDOG_EN) |= 0x1;
	while(1);
}


static at_result_t at_rst_exec(void)
{
    reset_8288();
    return AT_RESULT_FAILE;
}

static at_result_t ate_setup(const char* args)
{
    int echo_mode = 0;

    args = parse ((char*)(args),"d", &echo_mode);

    if (echo_mode == AT_ECHO_MODE_CLOSE || echo_mode == AT_ECHO_MODE_OPEN)
    {
        at_get_server()->echo_mode = echo_mode;
    }
    else
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_show_cmd_exec(void)
{
    extern void rt_at_server_print_all_cmd(void);

    rt_at_server_print_all_cmd();

    return AT_RESULT_OK;
}

static at_result_t at_uart_query(void)
{
    struct rt_serial_device* serial = (struct rt_serial_device*)at_get_server()->device;

    at_server_printfln("+UART=%d,%d,%d,%d,%d", serial->config.baud_rate, serial->config.data_bits,
                       serial->config.stop_bits, serial->config.parity, 1);

    return AT_RESULT_OK;
}

static at_result_t at_uart_setup(const char* args)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    int baudrate, databits, stopbits, parity, flow_control;

    args = parse ((char*)(++args),"ddddd", &baudrate, &databits, &stopbits, &parity, &flow_control);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    //at_server_printfln("UART baudrate : %d", baudrate);
    //at_server_printfln("UART databits : %d", databits);
    //at_server_printfln("UART stopbits : %d", stopbits);
    //at_server_printfln("UART parity   : %d", parity);
    //at_server_printfln("UART control  : %d", flow_control);

    config.baud_rate = baudrate;
    config.data_bits = databits;
    config.stop_bits = stopbits;
    config.parity = parity;

    if (rt_device_control(at_get_server()->device, RT_DEVICE_CTRL_CONFIG, &config) != RT_EOK)
    {
        return AT_RESULT_FAILE;
    }
    //boot_set_uart0_baud_rate(baudrate);
    uc_wiota_set_at_baud_rate(baudrate);

    return AT_RESULT_OK;
}



AT_CMD_EXPORT("AT", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_exec);
//AT_CMD_EXPORT("ATZ", RT_NULL, RT_NULL, RT_NULL, RT_NULL, atz_exec);
AT_CMD_EXPORT("AT+RST",RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_rst_exec);
AT_CMD_EXPORT("ATE", "<value>", RT_NULL, RT_NULL, ate_setup, RT_NULL);
AT_CMD_EXPORT("AT&L", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_show_cmd_exec);
AT_CMD_EXPORT("AT+UART", "=<baudrate>,<databits>,<stopbits>,<parity>,<flow_control>", RT_NULL, at_uart_query, at_uart_setup, RT_NULL);
#endif /* AT_USING_SERVER */
#endif
