#include <rtthread.h>
#ifdef _RTC_APP_
#include <rtdevice.h>
#include "uc_rtc_app.h"
#include "uc_rtc.h"

#define RTC_DEVICE_NAME    "rtc"
#define RTC_ALARM_SECOND   (5)  // MAX 59

static rt_device_t rtc_dev = NULL;

void rtc_alarm_test(void);

int rtc_app_init(void)
{
    rt_err_t ret = RT_EOK;

    rtc_dev = rt_device_find(RTC_DEVICE_NAME);
    if (rtc_dev == RT_NULL)
    {
        rt_kprintf("find %s failed!\r\n", RTC_DEVICE_NAME);
        return RT_ERROR;
    }

    ret = rt_device_init(rtc_dev);
    if (ret != RT_EOK)
    {
        rt_kprintf("rtc device init failed!\r\n", RTC_DEVICE_NAME);
        return ret;
    }

    return ret;
}


void rtc_app_sample(void)
{
    int ret = 0;
    time_t rtc_time;
    struct tm set_time;

    rt_kprintf("rtc test demo.\r\n");

    ret = rtc_app_init();
    if(ret != RT_EOK)
    {
        rt_kprintf("init rtc error.\r\n");
        return;
    }

    /* make time 2000/1/1 00:00:00 */
    set_time.tm_sec      = 0;    /* Seconds: 0-60 (to accommodate leap seconds) */
    set_time.tm_min      = 0;    /* Minutes: 0-59 */
    set_time.tm_hour     = 0;    /* Hours since midnight: 0-23 */
    set_time.tm_mday     = 1;    /* Day of the month: 1-31 */
    set_time.tm_mon      = 0;    /* Months *since* January: 0-11 */
    set_time.tm_year     = 100;  /* Years since 1900 */
    set_time.tm_wday     = 6;    /* Days since Sunday (0-6), 0 is Sunday */
    set_time.tm_yday     = 0;    /* Days since Jan. 1: 0-365 */
    set_time.tm_isdst    = 0;    /* +1=Daylight Savings Time, 0=No DST, -1=unknown */

    rtc_time = mktime(&set_time);    /* Get the number of seconds after January 1, 1970 */

    rt_kprintf("rtc set time %d-%d-%d %02d:%02d:%02d\r\n",
               (set_time.tm_year + 1900), set_time.tm_mon + 1,
               set_time.tm_mday, set_time.tm_hour, set_time.tm_min,
               set_time.tm_sec);
    
    if(rtc_dev != NULL)
    {
        ret = rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_SET_TIME, &rtc_time);
        if(ret != RT_EOK)
        {
            rt_kprintf("set rtc error, ret = %d\r\n", ret);
            return;
        }
        else
        {
            rt_kprintf("set rtc time ok\r\n");
        }

        /* alarm test */
        rtc_alarm_test();
    }
    else
    {
        rt_kprintf("rtc test error\r\n");
    }
}


void alarm_irq_handler(void *arg)
{
    int ret;
    time_t rtc_time;
    struct tm *now;

    ret = rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &rtc_time);
    if(ret != RT_EOK)
    {
        rt_kprintf("get rtc value error, ret = %d\r\n", ret);
        return;
    }

    /* convert time_t to struct tm */
    now = localtime(&rtc_time);

    rt_kprintf("rtc alarm irq arrived, now time = %d-%d-%d %02d:%02d:%02d\r\n",
               now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour,
               now->tm_min, now->tm_sec);

    /* disable rtc alarm irq */
    // rtc_disable_alarm_interrupt(UC_RTC);
    /* clear irq pending */
    rtc_clear_alarm_pending(UC_RTC);
}

void rtc_alarm_test(void)
{
    int ret = 0;
    rtc_alarm_t rtc_time = {0};
    
    rt_kprintf("alarm test demo\r\n");

    /* enable rtc irq */
    rtc_enable_alarm_interrupt(UC_RTC);
    
    /* set alarm 2000/1/1 00:00:05 */
    rtc_time.year = 2000;
    rtc_time.mon = 1;
    rtc_time.day = 1;
    rtc_time.hour = 0;
    rtc_time.min = 0;
    rtc_time.sec = 0 + RTC_ALARM_SECOND;
    rtc_time.week = 6;
    rtc_time.mask = RTC_AM_YEAR | RTC_AM_MON | RTC_AM_DAY | RTC_AM_WEEK;

    /* set alarm irq */
    rtc_time.call_back = alarm_irq_handler;
    rtc_time.call_back_arg = NULL;
    
    ret = rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_SET_ALARM, &rtc_time);
    if(ret != RT_EOK)
    {
        rt_kprintf("set alarm error, ret = %d\r\n", ret);
        return;
    }
    
    ret = rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_ALARM, &rtc_time);
    if(ret != RT_EOK)
    {
        rt_kprintf("get alarm error, ret = %d\r\n", ret);
    }
    else
    {
        rt_kprintf("set alarm time = %d-%d-%d %d:%d:%d, week = %d.\r\n",
                   rtc_time.year, rtc_time.mon, rtc_time.day, rtc_time.hour,
                   rtc_time.min, rtc_time.sec, rtc_time.week);
    }

}

#endif
