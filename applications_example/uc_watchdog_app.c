#include <rtthread.h>
#ifdef APP_EXAMPLE_WATCHDOG

#ifndef RT_USING_WDT
#error "Please enable rt-thread wdt device driver"
#endif

#ifndef BSP_USING_WDT
#error "Please enable on-chip peripheral wdt config"
#endif

#include <rtdevice.h>
#include "uc_watchdog_app.h"

#define WDT_DEVICE_NAME "wdt"

#define WACHDOG_KEEP_TIMEOUT 5

static rt_device_t wdg_dev = RT_NULL;

static void idle_watchdog_hook(void)
{
    rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, RT_NULL);
}

static int watchdog_app_disable(void)
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

static int watchdog_app_enable(void)
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

static void watchdog_app_close(void)
{
    watchdog_app_disable();
    rt_thread_idle_delhook(idle_watchdog_hook);
}

int watchdog_app_sample(void)
{
    rt_err_t ret = RT_EOK;
    rt_uint32_t timeout = WACHDOG_KEEP_TIMEOUT;

    rt_kprintf("watchdog_app_sample\n");

    wdg_dev = rt_device_find(WDT_DEVICE_NAME);
    if (!wdg_dev)
    {
        rt_kprintf("find %s failed!\n", WDT_DEVICE_NAME);
        return -RT_ERROR;
    }

    ret = rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
    if (ret != RT_EOK)
    {
        rt_kprintf("set %s timeout failed!\n", WDT_DEVICE_NAME);
        return -RT_ERROR;
    }

    rt_thread_idle_sethook(idle_watchdog_hook);

    watchdog_app_enable();
    rt_kprintf("enable %s success!\n", WDT_DEVICE_NAME);

    rt_kprintf("current tick is %d\n", rt_tick_get());
    rt_kprintf("delay wdt for %d seconds, the iddle thread will feed the dog twice\n", (WACHDOG_KEEP_TIMEOUT * 2));
    rt_kprintf("current tick is %d, wait for %d seconds\n", rt_tick_get(), (WACHDOG_KEEP_TIMEOUT * 2));
    rt_thread_delay((WACHDOG_KEEP_TIMEOUT * 2) * 1000);
    rt_kprintf("current tick is %d\n", rt_tick_get());
    rt_kprintf("The chip is dead, please wait for the watchdog to restart the chip...\n");
    rt_kprintf("The watchdog will restart the chip in %d seconds...\n", WACHDOG_KEEP_TIMEOUT);
    while (1)
    {
        ;
    }

    return RT_EOK;
}

#endif
