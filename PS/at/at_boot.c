#include <at.h>
#include <stdlib.h>
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "ati_prs.h"
#include "uc_boot_uart.h"
#include "uc_boot_download.h"  
#include "uc_watchdog_app.h"

static at_result_t at_ymodem_exec(void)
{
    boot_set_modem(BOOT_SHARE_ENTER_DOWNLOAD);

#ifdef _WATCHDOG_APP_
    watchdog_app_disable();
#endif

    at_server_printfln("OK");

    boot_uart_wait_tx_done();
    rt_hw_interrupt_disable();
    
    boot_riscv_reboot();
    return AT_RESULT_NULL;
}

AT_CMD_EXPORT("AT+YMODEM", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_ymodem_exec);

