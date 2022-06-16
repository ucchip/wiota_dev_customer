#include <rtthread.h>
#ifdef _WATCHDOG_APP_
#include <rtdevice.h>
#include "uc_watchdog_app.h"

#define WDT_DEVICE_NAME    "wdt"

static rt_device_t wdg_dev = NULL;

static void idle_watchdog_hook(void)
{
    rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, NULL);
}

int watchdog_app_init(void)
{
    rt_err_t ret = RT_EOK;
    rt_uint32_t timeout = WACHDOG_KEEP_TIMEOUT;

    wdg_dev = rt_device_find(WDT_DEVICE_NAME);
    if (!wdg_dev)
    {
        rt_kprintf("find %s failed!\n", WDT_DEVICE_NAME);
        return RT_ERROR;
    }

    ret = rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
    if (ret != RT_EOK)
    {
        rt_kprintf("set %s timeout failed!\n", WDT_DEVICE_NAME);
        return RT_ERROR;
    }

    rt_thread_idle_sethook(idle_watchdog_hook);

    return ret;
}

int watchdog_app_disable(void)
{
    rt_err_t ret = RT_EOK;

    ret = rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_STOP, RT_NULL);
    if (ret != RT_EOK)
    {
        rt_kprintf("start %s failed!\n", WDT_DEVICE_NAME);
        return -RT_ERROR;
    }

    return ret;
}


int watchdog_app_enable(void)
{
    rt_err_t ret = RT_EOK;

    ret = rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_START, RT_NULL);
    if (ret != RT_EOK)
    {
        rt_kprintf("start %s failed!\n", WDT_DEVICE_NAME);
        return -RT_ERROR;
    }

    return ret;
}

void watchdog_app_close(void)
{
    watchdog_app_disable();
    rt_thread_idle_delhook(idle_watchdog_hook);
}

#endif
