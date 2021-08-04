
#include "board.h"
#include<rtthread.h>
#include<rtdevice.h>

#ifdef RT_USING_RTC

#include "uc_rtc.h"

//#define DRV_DEBUG
#define LOG_TAG             "drv.rtc"
#include <drv_log.h>

static struct rt_device rtc;

static time_t get_rtc_timestamp(void)
{
    struct tm tm_new;
    rtc_time_t rtc_time;

    rtc_get_time(UC_RTC, &rtc_time);
    //LOG_D("get_rtc_timestamp: %04d-%02d-%02d %02d:%02d:%02d", rtc_time.year, rtc_time.mon, rtc_time.day,
    //            rtc_time.hour, rtc_time.min, rtc_time.sec);

    tm_new.tm_sec  = rtc_time.sec;
    tm_new.tm_min  = rtc_time.min;
    tm_new.tm_hour = rtc_time.hour;
    tm_new.tm_mday = rtc_time.day;
    tm_new.tm_mon  = rtc_time.mon - 1;
    tm_new.tm_year = rtc_time.year - 1900;
    tm_new.tm_wday  = rtc_time.week - 1;

    LOG_D("get rtc time.");
    return mktime(&tm_new);
}

static rt_err_t set_rtc_time_stamp(time_t time_stamp)
{
    struct tm* p_tm;
    rtc_time_t rtc_time;

    p_tm = localtime(&time_stamp);
    if (p_tm->tm_year < 100)
    {
        LOG_D("set rtc time. tm_year Err!");
        return -RT_ERROR;
    }
    //LOG_D("set_rtc_time_stamp: %04d-%02d-%02d %02d:%02d:%02d", p_tm->tm_year, p_tm->tm_mon, p_tm->tm_mday,
    //            p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);

    LOG_D("set rtc time.");
    rtc_time.sec = p_tm->tm_sec;
    rtc_time.min = p_tm->tm_min;
    rtc_time.hour = p_tm->tm_hour;
    rtc_time.day = p_tm->tm_mday;
    rtc_time.mon = p_tm->tm_mon + 1;
    rtc_time.year = p_tm->tm_year + 1900;
    rtc_time.week = p_tm->tm_wday + 1;
    rtc_set_time(UC_RTC, &rtc_time);

    return RT_EOK;
}

static void rt_rtc_init(void)
{
    rtc_calibrate();
    rtc_init(UC_RTC);
}

static rt_err_t rt_rtc_config(struct rt_device* dev)
{
    rtc_time_t rtc_time;

    rtc_get_time(UC_RTC, &rtc_time);
    if ((rtc_time.year > 2099)
        || (rtc_time.mon > 12)
        || (rtc_time.day > 31)
        || (rtc_time.week > 7)
        || (rtc_time.hour > 23)
        || (rtc_time.min > 59)
        || (rtc_time.sec > 59))
    {
        rtc_time.sec = 0x0;
        rtc_time.min = 0x0;
        rtc_time.hour = 0x0c;
        rtc_time.day = 0x01;
        rtc_time.mon = 0x06;
        rtc_time.year = 0x7e4;
        rtc_time.week = 0x01;
        rtc_set_time(UC_RTC, &rtc_time);
        LOG_D("rt_rtc_config set timestamp");
    }

    return RT_EOK;
}

static rt_err_t rt_rtc_control(rt_device_t dev, int cmd, void* args)
{
    rt_err_t result = RT_EOK;
    RT_ASSERT(dev != RT_NULL);
    switch (cmd)
    {
        case RT_DEVICE_CTRL_RTC_GET_TIME:
            *(time_t*)args = get_rtc_timestamp();
            LOG_D("RTC: get rtc_time %d\n", *(time_t*)args);
            break;

        case RT_DEVICE_CTRL_RTC_SET_TIME:
            if (set_rtc_time_stamp(*(time_t*)args))
            {
                result = -RT_ERROR;
            }
            LOG_D("RTC: set rtc_time %d\n", *(time_t*)args);
            break;
    }

    return result;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops rtc_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    rt_rtc_control
};
#endif

static rt_err_t rt_hw_rtc_register(rt_device_t device, const char* name, rt_uint32_t flag)
{
    RT_ASSERT(device != RT_NULL);

    rt_rtc_init();
    if (rt_rtc_config(device) != RT_EOK)
    {
        return -RT_ERROR;
    }
#ifdef RT_USING_DEVICE_OPS
    device->ops         = &rtc_ops;
#else
    device->init        = RT_NULL;
    device->open        = RT_NULL;
    device->close       = RT_NULL;
    device->read        = RT_NULL;
    device->write       = RT_NULL;
    device->control     = rt_rtc_control;
#endif
    device->type        = RT_Device_Class_RTC;
    device->rx_indicate = RT_NULL;
    device->tx_complete = RT_NULL;
    device->user_data   = RT_NULL;

    /* register a character device */
    return rt_device_register(device, name, flag);
}

int rt_hw_rtc_init(void)
{
    rt_err_t result;
    result = rt_hw_rtc_register(&rtc, "rtc", RT_DEVICE_FLAG_RDWR);
    if (result != RT_EOK)
    {
        LOG_E("rtc register err code: %d", result);
        return result;
    }
    LOG_D("rtc init success");
    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_rtc_init);

#endif /* BSP_USING_ONCHIP_RTC */
