#include <rtthread.h>
#include <rtdevice.h>

#ifdef _FPGA_
#include <board.h>
#include "uc_event.h"
#endif

#ifdef UC8288_MODULE
#include "uc_wiota_static.h"
#include "uc_wiota_api.h"

#ifdef RT_USING_AT
#include "at.h"
#include "at_wiota.h"
#include "at_wiota_gpio_report.h"
#endif

#endif

#ifdef _QUICK_CONNECT_
#include "quick_connect.h"
#endif

#ifdef RT_USING_WDT

rt_device_t wdg_dev = RT_NULL;

static void wiota_app_wdt_idle_hook(void)
{
    rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, RT_NULL);
}

static int wiota_app_wdt_init(void)
{
    int ret = 0;
    rt_uint32_t timeout = 5;
    char *wdt_device_name = "wdt";

    wdg_dev = rt_device_find(wdt_device_name);
    if (!wdg_dev)
    {
        rt_kprintf("find %s failed!\n", wdt_device_name);
        return -RT_ERROR;
    }

    ret = rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
    if (ret != RT_EOK)
    {
        rt_kprintf("set %s timeout failed!\n", wdt_device_name);
        return -RT_ERROR;
    }

    rt_thread_idle_sethook(wiota_app_wdt_idle_hook);

    ret = rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_START, RT_NULL);
    if (ret != RT_EOK)
    {
        rt_kprintf("start %s failed!\n", wdt_device_name);
        return -RT_ERROR;
    }

    return RT_EOK;
}

#endif

int wiota_app_init(void)
{
    int bin_size;
    int reserved_size;
    int ota_size;

    rt_kprintf("wiota_app_init\n");

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

#ifdef RT_USING_WDT
    wiota_app_wdt_init();
#endif

#ifdef UC8288_MODULE
#ifdef RT_USING_AT
    at_server_init();
    at_wiota_manager();
    at_wiota_gpio_report_init();
    wake_out_pulse_init();
    at_wiota_awaken_notice();
#endif
#endif

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
//    at_handle_log_uart(0);
#endif

#ifdef WIOTA_API_TEST
    // app_task_init();
#endif

#ifdef _QUICK_CONNECT_
    quick_connect_task_init();
#endif

    return 0;
}