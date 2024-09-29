#include "drv_rtc.h"

#ifdef RT_USING_RTC
#ifndef RT_USING_SOFT_RTC

#if !defined(BSP_USING_RTC)
#error "Please define at least one BSP_USING_RTC"
/* this driver can be disabled at menuconfig → RT-Thread Components → Device Drivers */
#endif

// #define DRV_DEBUG
#define LOG_TAG "drv.rtc"
#include <drv_log.h>

static rt_rtc_dev_t rtc_dev;

#ifdef RT_USING_ALARM
struct rt_rtc_wkalarm wkalarm;
#endif

static time_t drv_rtc_get_timestamp(void)
{
    struct tm tm_new;
    rtc_time_t rtc_time;

    rtc_get_time(UC_RTC, &rtc_time);
    LOG_D("drv_rtc_get_timestamp: %04d-%02d-%02d %02d:%02d:%02d", rtc_time.year, rtc_time.mon, rtc_time.day,
          rtc_time.hour, rtc_time.min, rtc_time.sec);

    tm_new.tm_sec = rtc_time.sec;
    tm_new.tm_min = rtc_time.min;
    tm_new.tm_hour = rtc_time.hour;
    tm_new.tm_mday = rtc_time.day;
    tm_new.tm_mon = rtc_time.mon - 1;
    tm_new.tm_year = rtc_time.year - 1900;
    tm_new.tm_wday = rtc_time.week - 1;

    LOG_D("get rtc time.");
    return timegm(&tm_new);
}

static rt_err_t drv_rtc_set_timestamp(time_t timestamp)
{
    struct tm tm_new;
    rtc_time_t rtc_time;

    gmtime_r(&timestamp, &tm_new);
    if (tm_new.tm_year < 100)
    {
        LOG_E("set rtc time. tm_year Err!");
        return -RT_ERROR;
    }
    LOG_D("set_rtc_time_stamp: %04d-%02d-%02d %02d:%02d:%02d",
          tm_new.tm_year + 1900, tm_new.tm_mon + 1, tm_new.tm_mday,
          tm_new.tm_hour, tm_new.tm_min, tm_new.tm_sec);

    rtc_time.sec = tm_new.tm_sec;
    rtc_time.min = tm_new.tm_min;
    rtc_time.hour = tm_new.tm_hour;
    rtc_time.day = tm_new.tm_mday;
    rtc_time.mon = tm_new.tm_mon + 1;
    rtc_time.year = tm_new.tm_year + 1900;

    /* struct tm.tm_wday 0 is Sunday */
    if (tm_new.tm_wday == 0)
        rtc_time.week = RTC_WDAY_SUN;
    else
        rtc_time.week = tm_new.tm_wday;

    rtc_set_time(UC_RTC, &rtc_time);

    LOG_D("set rtc time.");

    return RT_EOK;
}

static rt_err_t drv_rtc_config(void)
{
    rtc_time_t rtc_time;

    rtc_get_time(UC_RTC, &rtc_time);
    if ((rtc_time.year > 2099) || (rtc_time.mon > 12) || (rtc_time.day > 31) || (rtc_time.week > 7) || (rtc_time.hour > 23) || (rtc_time.min > 59) || (rtc_time.sec > 59))
    {
        rtc_time.sec = 0;
        rtc_time.min = 0;
        rtc_time.hour = 12;
        rtc_time.day = 1;
        rtc_time.mon = 6;
        rtc_time.year = 2020;
        rtc_time.week = 1; // Monday
        rtc_set_time(UC_RTC, &rtc_time);
        LOG_D("drv_rtc_config set timestamp");
    }

    return RT_EOK;
}

static rt_err_t uc8x88_rtc_init(void)
{
    /* calibrate */
    rtc_calibrate();

    rtc_init(UC_RTC);
    if (drv_rtc_config() != RT_EOK)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t uc8x88_rtc_get_secs(time_t *sec)
{
    *sec = drv_rtc_get_timestamp();
    LOG_D("RTC: get rtc_time %d", *sec);

    return RT_EOK;
}

static rt_err_t uc8x88_rtc_set_secs(time_t *sec)
{
    rt_err_t result = RT_EOK;

    if (drv_rtc_set_timestamp(*sec))
    {
        result = -RT_ERROR;
    }
    LOG_D("RTC: set rtc_time %d", *sec);

#ifdef RT_USING_ALARM
    rt_alarm_update(&rtc_dev.parent, 1);
#endif

    return result;
}

static rt_err_t uc8x88_rtc_get_alarm(struct rt_rtc_wkalarm *alarm)
{
#ifdef RT_USING_ALARM
    *alarm = wkalarm;
    LOG_D("GET_ALARM %d:%d:%d", wkalarm.tm_hour, wkalarm.tm_min, wkalarm.tm_sec);
    return RT_EOK;
#else
    return -RT_ERROR;
#endif
}

static rt_err_t uc8x88_rtc_set_alarm(struct rt_rtc_wkalarm *alarm)
{
#ifdef RT_USING_ALARM
    rtc_alarm_t rtc_time;
    if (alarm != RT_NULL)
    {
        wkalarm = *alarm;
        if (wkalarm.enable)
        {
            rtc_time.year = 2020;
            rtc_time.mon = 1;
            rtc_time.day = 1;
            rtc_time.week = 3;
            rtc_time.hour = wkalarm.tm_hour;
            rtc_time.min = wkalarm.tm_min;
            rtc_time.sec = wkalarm.tm_sec;
            rtc_time.mask = RTC_AM_YEAR | RTC_AM_MON | RTC_AM_DAY | RTC_AM_WEEK;
            /* set alarm time */
            rtc_set_alarm(UC_RTC, &rtc_time);
            /* enable rtc irq */
            rtc_enable_alarm_interrupt(UC_RTC);
        }
    }
    else
    {
        LOG_E("RT_DEVICE_CTRL_RTC_SET_ALARM error!!");
        return -RT_ERROR;
    }

    LOG_D("SET_ALARM %d:%d:%d", alarm->tm_hour, alarm->tm_min, alarm->tm_sec);
    return RT_EOK;
#else
    return -RT_ERROR;
#endif
}

void rtc_handler(void)
{
#ifdef RT_USING_ALARM
    rt_interrupt_enter();
    /* disable rtc irq */
    rtc_disable_alarm_interrupt(UC_RTC);
    /* update alarm */
    rt_alarm_update(&rtc_dev.parent, 1);
    /* clear irq pending */
    rtc_clear_alarm_pending(UC_RTC);
    rt_interrupt_leave();
#endif
}

static const struct rt_rtc_ops _rtc_ops = {
    uc8x88_rtc_init,
    uc8x88_rtc_get_secs,
    uc8x88_rtc_set_secs,
    uc8x88_rtc_get_alarm,
    uc8x88_rtc_set_alarm,
    RT_NULL,
    RT_NULL,
};

int rt_hw_rtc_init(void)
{
    rt_err_t result;
    rtc_dev.ops = &_rtc_ops;
    result = rt_hw_rtc_register(&rtc_dev, "rtc", RT_DEVICE_FLAG_RDWR, RT_NULL);
    if (result != RT_EOK)
    {
        LOG_E("rtc register err code: %d", result);
        return result;
    }
    LOG_D("rtc init success");
    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_rtc_init);

#endif /* RT_USING_SOFT_RTC */
#endif /* BSP_USING_ONCHIP_RTC */
