#include <rtthread.h>
#ifdef APP_EXAMPLE_RTC

#ifndef RT_USING_RTC
#error "Please enable rt-thread rtc device driver"
#endif

#ifndef BSP_USING_RTC
#error "Please enable on-chip peripheral rtc config"
#endif

#include <rtdevice.h>
#include "uc_rtc_app.h"

#define THREAD_STACK_SIZE 512
#define THREAD_PRIORITY 5
#define THREAD_TIMESLICE 5

#define RTC_DEVICE_NAME "rtc"
static rt_device_t rtc_device = RT_NULL;

#ifdef RT_USING_ALARM

static void user_alarm_callback(rt_alarm_t alarm, time_t timestamp)
{
    rt_kprintf("user alarm callback function. alarm flag: 0x%x, timestamp: %d\n", alarm->flag, timestamp);
}

static void rtc_alarm_app_sample(void)
{
    struct rt_alarm_setup setup;
    struct rt_alarm *alarm = RT_NULL;
    static time_t now;
    struct tm p_tm;

    /* 获取当前时间戳，并把下5秒时间设置为闹钟时间 */
    /* 闹钟第一次响起在当前时间的1分5秒后，之后每隔1分钟响起 */
    now = time(RT_NULL) + 5;
    gmtime_r(&now, &p_tm);

    setup.flag = RT_ALARM_MINUTE;
    setup.wktime.tm_year = p_tm.tm_year;
    setup.wktime.tm_mon = p_tm.tm_mon;
    setup.wktime.tm_mday = p_tm.tm_mday;
    setup.wktime.tm_wday = p_tm.tm_wday;
    setup.wktime.tm_hour = p_tm.tm_hour;
    setup.wktime.tm_min = p_tm.tm_min;
    setup.wktime.tm_sec = p_tm.tm_sec;

    alarm = rt_alarm_create(user_alarm_callback, &setup);
    if (alarm == RT_NULL)
    {
        rt_kprintf("create alarm failed.\n");
    }
    else
    {
        rt_alarm_start(alarm);
    }
}

#endif

static void rtc_thread_entry(void *parameter)
{
    time_t now;
    while (1)
    {
        /* 延时5秒 */
        rt_thread_mdelay(5000);
        /* 获取时间 */
        now = time(RT_NULL);
        rt_kprintf("%s", ctime(&now));
    }
}

int rtc_app_sample(void)
{
    rt_err_t ret = RT_EOK;
    rt_thread_t thread = RT_NULL;

    rt_kprintf("rtc_app_sample\n");

    /*寻找设备*/
    rtc_device = rt_device_find(RTC_DEVICE_NAME);
    if (!rtc_device)
    {
        rt_kprintf("find %s failed!\n", RTC_DEVICE_NAME);
        return -RT_ERROR;
    }

    /*初始化RTC设备*/
    if (rt_device_open(rtc_device, 0) != RT_EOK)
    {
        rt_kprintf("open %s failed!\n", RTC_DEVICE_NAME);
        return -RT_ERROR;
    }

    /* 设置日期 */
    ret = set_date(2020, 1, 1);
    if (ret != RT_EOK)
    {
        rt_kprintf("set RTC date failed\n");
        return ret;
    }

    /* 设置时间 */
    ret = set_time(0, 0, 0);
    if (ret != RT_EOK)
    {
        rt_kprintf("set RTC time failed\n");
        return ret;
    }

#ifdef RT_USING_ALARM
    rtc_alarm_app_sample();
#endif

    /* 创建 serial 线程 */
    thread = rt_thread_create("rtc_app",
                              rtc_thread_entry,
                              RT_NULL,
                              THREAD_STACK_SIZE,
                              THREAD_PRIORITY,
                              THREAD_TIMESLICE);
    /* 创建成功则启动线程 */
    if (RT_NULL == thread)
    {
        return -RT_ERROR;
    }
    else
    {
        rt_thread_startup(thread);
    }

    return RT_EOK;
}

#endif
