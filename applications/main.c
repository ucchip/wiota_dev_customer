/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-26     RT-Thread    first version
 */
#include <rtthread.h>
#include "uc_wiota_api.h"
#ifdef _RT_THREAD_
#include <rtdevice.h>
#endif

#ifdef _FPGA_
#include <board.h>
#include "uc_event.h"
#endif

#ifdef UC8288_MODULE
#ifdef RT_USING_AT
#include "at.h"
#endif
#else
#include "test_wiota_api.h"
#endif


#ifdef _WATCHDOG_APP_
#include "uc_watchdog_app.h"
#endif

#ifdef _ROMFUNC_
#include "dll.h"
#endif

#ifdef _ADC_APP_
#include "uc_adc_app.h"
#endif

#ifdef _CAN_APP_
#include "uc_can_app.h"
#endif

#ifdef _DAC_APP_
#include "uc_dac_app.h"
#endif

#ifdef _IIC_APP_
#include "uc_iic_app.h"
#endif

#ifdef _PIN_APP_
#include "uc_pin_app.h"
#endif

#ifdef _BUTTON_APP_
#include "uc_button_app.h"
#endif

#ifdef _PWM_APP_
#include "uc_pwm_app.h"
#endif

#ifdef _RTC_APP_
#include "uc_rtc_app.h"
#endif

#ifdef _SPI_FLASH_APP_
#include "uc_spi_flash_app.h"
#endif

#ifdef _SPIM_FLASH_APP_
#include "uc_spim_flash_app.h"
#endif

#ifdef _UART_APP_
#include "uc_uart_app.h"
#endif

#ifdef _RS485_APP_
#include "uc_rs485_app.h"
#endif

#ifdef UC_SPIM_SUPPORT
#include "uc_spim_api.h"
#endif

extern void uc_wiota_static_data_init(void);
extern int at_wiota_gpio_report_init(void);
extern int wake_out_pulse_init(void);

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
extern void at_handle_log_uart(int uart_number);
#endif
extern void at_wiota_manager(void);
//void task_callback(struct rt_thread* from, struct rt_thread* to)
//{
//    rt_kprintf("name = %s, 0x%x\n", from->name, from);
//}
//
//
//void init_statistical_task_info(void)
//{
//    rt_scheduler_sethook(task_callback);
//}

void app_test(void)
{
#ifdef _ADC_APP_
    adc_app_sample();
#endif

#ifdef _CAN_APP_
    can_app_sample();
#endif

#ifdef _DAC_APP_
    dac_app_sample();
#endif

#ifdef _IIC_APP_
    iic_app_sample();
#endif

#ifdef _PIN_APP_
    pin_app_sample();
#endif

#ifdef _BUTTON_APP_
    button_app_sample();
#endif

#ifdef _PWM_APP_
    pwm_app_sample();
#endif

#ifdef _RTC_APP_
    rtc_app_sample();
#endif

#ifdef _SPI_FLASH_APP_
    spi_flash_app_sample();
#endif

#ifdef _SPIM_FLASH_APP_
    spim_flash_app_sample();
#endif

#ifdef _RS485_APP_
    rs485_app_sample();
#endif

#ifdef _UART_APP_
    uart_app_sample();
#endif

#ifdef UC_SPIM_SUPPORT
    spim_wr_sample();
#endif
}

int main(void)
{
    int bin_size;
    int reserved_size;
    int ota_size;

#ifdef _ROMFUNC_
    dll_open();
#endif

    uc_wiota_static_data_init();

    /*
    compatible with versions earliner than v3.1(SYNC WIOTA V3.1).
    Now rt_thread.bin size 308676.
    Whole flash 512K.Tail has 8K static config data.
    config parament relationship:
        (bin_part + reserved_part + ota_package) == (512K - 8K)
        bin_part max size 311296(rt_thread.bin)
        reserved_part max size 147456.It is not allowed to reduce value.
        ota_package max size 57344
     */
    get_partition_size(&bin_size, &reserved_size, &ota_size);
    if (bin_size < 0x4c000)
    {
        set_partition_size(0x4c000, 0x24000, 0xe000);
    }

#ifdef _WATCHDOG_APP_
    if (!watchdog_app_init())
        watchdog_app_enable();
#endif

    //app_test();

#ifdef UC8288_MODULE
#ifdef RT_USING_AT
    at_server_init();
    at_wiota_manager();
#endif
#endif

    at_wiota_gpio_report_init();
    wake_out_pulse_init();

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
//    at_handle_log_uart(0);
#endif

    //    uc_wiota_light_func_enable(0);

    // unsigned int i;
    // unsigned int value;
    // unsigned int addr = 0x306000;

    // read write 8K
    // for (i=0; i<2048; i++) {
    //     *((unsigned int*)(addr) + i) = 0;
    // }

    while(1)
    {
        unsigned int total;
        unsigned int used;
        unsigned int max_used;


        rt_thread_delay(1000);

        // read write 8K
        // for (i=0; i<2048; i++) {
        //     value = *((unsigned int*)(addr) + i);
        //     value++;
        //     *((unsigned int*)(addr) + i) = value;
        // }

        rt_memory_info(&total,&used,&max_used);
        rt_kprintf("total %d used %d maxused %d\n",total,used,max_used);

    }

    //    init_statistical_task_info();

    return 0;
}
