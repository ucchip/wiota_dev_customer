
#include <board.h>
#include<rtthread.h>
#include<rtdevice.h>

#ifdef RT_USING_WDT

//#define DRV_DEBUG
#define LOG_TAG             "drv.wdt"
#include <drv_log.h>

#include <uc_watchdog.h>

struct uc8088_wdt_obj
{
    rt_uint16_t is_start;
    rt_uint32_t Reload;
};

#define WDT_MICROSECOND_PER_SECOND   1000

static struct uc8088_wdt_obj uc8088_wdt = {0, 30 * WDT_MICROSECOND_PER_SECOND};
static struct rt_watchdog_ops ops;
static rt_watchdog_t watchdog;

static rt_err_t watchdog_init(rt_watchdog_t* wdt)
{
    return RT_EOK;
}

static rt_err_t watchdog_control(rt_watchdog_t* wdt, int cmd, void* arg)
{
    switch (cmd)
    {
        /* feed the watchdog */
        case RT_DEVICE_CTRL_WDT_KEEPALIVE:
            wdt_feed(UC_WATCHDOG);
            break;

        /* set watchdog timeout */
        case RT_DEVICE_CTRL_WDT_SET_TIMEOUT:
            uc8088_wdt.Reload = (*((rt_uint32_t*)arg)) * WDT_MICROSECOND_PER_SECOND;
            if (uc8088_wdt.Reload == 0)
            {
                LOG_E("wdg set timeout parameter too small");
                return -RT_EINVAL;
            }
            //if(uc8088_wdt.is_start)
            {
                wdt_init(UC_WATCHDOG, uc8088_wdt.Reload);
            }
            break;

        case RT_DEVICE_CTRL_WDT_GET_TIMEOUT:
            (*((rt_uint32_t*)arg)) = uc8088_wdt.Reload / WDT_MICROSECOND_PER_SECOND;
            break;

        case RT_DEVICE_CTRL_WDT_START:
            wdt_init(UC_WATCHDOG, uc8088_wdt.Reload);
            wdt_enable(UC_WATCHDOG);
            wdt_feed(UC_WATCHDOG);
            uc8088_wdt.is_start = 1;
            break;

        default:
            LOG_W("This command is not supported.");
            return -RT_ERROR;
    }
    return RT_EOK;
}

int rt_wdt_init(void)
{
    uc8088_wdt.is_start = 0;
    uc8088_wdt.Reload = 30 * WDT_MICROSECOND_PER_SECOND;

    ops.init = &watchdog_init;
    ops.control = &watchdog_control;
    watchdog.ops = &ops;
    /* register watchdog device */
    if (rt_hw_watchdog_register(&watchdog, "wdt", RT_DEVICE_FLAG_DEACTIVATE, RT_NULL) != RT_EOK)
    {
        LOG_E("wdt device register failed.");
        return -RT_ERROR;
    }
    LOG_D("wdt device register success.");
    return RT_EOK;
}
INIT_BOARD_EXPORT(rt_wdt_init);


#endif /* RT_USING_WDT */
