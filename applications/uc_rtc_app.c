#include <rtthread.h>
#ifdef _RTC_APP_
#include <rtdevice.h>
#include "uc_rtc_app.h"
#include "uc_rtc.h"

#define RTC_DEVICE_NAME    "rtc"

static rt_device_t rtc_dev = NULL;

int rtc_app_init(void)
{
    rt_err_t ret = RT_EOK;

    rtc_dev = rt_device_find(RTC_DEVICE_NAME);
    if (rtc_dev == RT_NULL)
    {
        rt_kprintf("find %s failed!\n", RTC_DEVICE_NAME);
        return RT_ERROR;
    }

    return ret;
}


void rtc_app_sample(void)
{
    int ret = 0;
    time_t rtc_time;
    
    rtc_time = 30;
    rt_kprintf("rtc test demo.\r\n");
    
    ret = rtc_app_init();
    if(ret != RT_EOK)
    {
        rt_kprintf("init rtc error.\r\n");
        return;
    }
    
    if(rtc_dev != NULL)
    {
        ret = rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_SET_TIME, &rtc_time);
        if(ret != RT_EOK)
        {
            rt_kprintf("set rtc error.\r\n");
            return;
        }
        
        while(1)
        {
            rt_thread_mdelay(3000);
            
            ret = rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &rtc_time);
            if(ret != RT_EOK)
            {
                rt_kprintf("set rtc error.\r\n");
                return;
            }
            else
            {
                rt_kprintf("rtc value = %d.\r\n", rtc_time);
            }
        }
    }
    else
    {
        rt_kprintf("rtc test error.\r\n");
    }
}

void alarm_app_sample(void)
{
    int ret = 0;
    rtc_alarm_t rtc_time;
    
    rt_kprintf("alarm test demo.\r\n");
    
    rtc_time.year = 2022;
    rtc_time.mon = 11;
    rtc_time.day = 20;
    rtc_time.hour = 10;
    rtc_time.min = 0;
    rtc_time.sec = 0;
    rtc_time.week = 7;
    rtc_time.mask = RTC_AM_YEAR | RTC_AM_MON | RTC_AM_DAY | RTC_AM_WEEK;
    
    ret = rtc_app_init();
    if(ret != RT_EOK)
    {
        rt_kprintf("init alarm error.\r\n");
        return;
    }
    
    if(rtc_dev != NULL)
    {
        ret = rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_SET_ALARM, &rtc_time);
        if(ret != RT_EOK)
        {
            rt_kprintf("set alarm error.\r\n");
            return;
        }
        
        rt_thread_mdelay(3000);
        
        ret = rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_ALARM, &rtc_time);
        if(ret != RT_EOK)
        {
            rt_kprintf("set alarm error.\r\n");
        }
        else
        {
            rt_kprintf("alarm time = %d-%d-%d %d:%d:%d,week = %d.\r\n", rtc_time.year, rtc_time.mon, rtc_time.day, rtc_time.hour, rtc_time.min, rtc_time.sec, rtc_time.week);
        }
    }
    else
    {
        rt_kprintf("alarm test error.\r\n");
    }
}

#endif
