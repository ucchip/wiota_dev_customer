#include <at.h>
#include <stdlib.h>
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "ati_prs.h"
//#include "uc_boot_uart.h"
//#include "uc_boot_download.h"
#include "uc_watchdog_app.h"
#include "uc_wiota_api.h"
//static at_result_t at_ymodem_exec(void)
//{
//    boot_set_modem(BOOT_SHARE_ENTER_DOWNLOAD);
//
//#ifdef _WATCHDOG_APP_
//    watchdog_app_disable();
//#endif
//
//    at_server_printfln("OK");
//
//    boot_uart_wait_tx_done();
//    rt_hw_interrupt_disable();
//
//    boot_riscv_reboot();
//    return AT_RESULT_NULL;
//}
//
//AT_CMD_EXPORT("AT+YMODEM", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_ymodem_exec);
static  at_result_t at_uboot_version_query(void)
{
	u8_t version[8] = {0};
	get_uboot_version(version);
	at_server_printfln("+UBOOTVERSION:%s", version);
	return AT_RESULT_OK;
}

static at_result_t at_uboot_baudrate_query(void)
{
	int baudrate = 0;
	get_uboot_baud_rate(&baudrate);
	at_server_printfln("+UBOOTBAUDRATE:%d", baudrate);
	return AT_RESULT_OK;
}

static at_result_t at_set_uboot_baudrate(const char *args)
{
	int baudrate = 0;
	args = parse((char *)(++args), "d", &baudrate);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
	set_uboot_baud_rate(baudrate);
	return AT_RESULT_OK;
}

static at_result_t at_uboot_mode_query(void)
{
	u8_t mode = 0;
	get_uboot_mode(&mode);
	at_server_printfln("+UBOOTMODE:%c", mode);
	return AT_RESULT_OK;
}

static at_result_t at_set_uboot_mode(const char *args)
{
	u8_t mode = 0;
	args = parse((char *)(++args), "s",1, &mode);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
	set_uboot_mode(mode);
	rt_hw_interrupt_disable();
	boot_riscv_reboot();
	return AT_RESULT_OK;
}

static at_result_t at_partition_size_query(void)
{
	int bin_size=0,reserved_size = 0 , ota_size = 0;
	get_partition_size(&bin_size,&reserved_size,&ota_size);
	at_server_printfln("+BINSIZE:%d", bin_size);
	at_server_printfln("+RESERVEDSIZE:%d", reserved_size);
	at_server_printfln("+OTASIZE:%d", ota_size);
	return AT_RESULT_OK;
}

static at_result_t at_set_partition_size(const char *args)
{
	int bin_size=0,reserved_size = 0 , ota_size = 0;
	args = parse((char *)(++args), "d,d,d", &bin_size,&reserved_size,&ota_size);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
	set_partition_size(bin_size,reserved_size,ota_size);
	return AT_RESULT_OK;
}

static at_result_t at_uboot_log_query(void)
{
	u8_t uart_flag,log_flag,select_flag;
	get_uboot_log_set(&uart_flag,&log_flag,&select_flag);
	at_server_printfln("+UARTFLAG:%d",uart_flag);
	at_server_printfln("+LOGFLAG:%d",log_flag);
	at_server_printfln("+SELECTFLAG%d:",select_flag);
	return AT_RESULT_OK;
}

static at_result_t at_set_uboot_log(const char *args)
{
	u8_t uart_flag,log_flag,select_flag;
	args = parse((char *)(++args), "d,d,d",&uart_flag,&log_flag,&select_flag);
	if(!args)
	{
		return AT_RESULT_PARSE_FAILE;
	}
	set_uboot_log(uart_flag,log_flag,select_flag);
	return AT_RESULT_OK;
}

AT_CMD_EXPORT("AT+UBOOTVERSION",RT_NULL,RT_NULL,at_uboot_version_query,RT_NULL,RT_NULL);
AT_CMD_EXPORT("AT+UBOOTBAUDRATE","=<baudrate>",RT_NULL,at_uboot_baudrate_query,at_set_uboot_baudrate,RT_NULL);
AT_CMD_EXPORT("AT+UBOOTMODE","=<mode>",RT_NULL,at_uboot_mode_query,at_set_uboot_mode,RT_NULL);
AT_CMD_EXPORT("AT+PARTITIONSIZE","=<bin_size>,<reserved_size>,<ota_size>",RT_NULL,at_partition_size_query,at_set_partition_size,RT_NULL);
AT_CMD_EXPORT("AT+UBOOTLOG","=<uart_flag>,<log_flag>,<select_flag>",RT_NULL,at_uboot_log_query,at_set_uboot_log,RT_NULL);