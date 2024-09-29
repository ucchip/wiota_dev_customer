#include <rtthread.h>

#ifdef APP_EXAMPLE_HWTIMER

#ifndef BSP_USING_HWTIMER
#error "Please enable on-chip peripheral hardware timer config"
#endif

#include <rtdevice.h>
#include "uc_hwtimer_app.h"

#define HWTIMER_DEV_NAME "timer1" /* 定时器名称 */

/* 定时器超时回调函数 */
static rt_err_t timeout_cb(rt_device_t dev, rt_size_t size)
{
    static rt_tick_t old_tick = 0;
    static rt_tick_t new_tick = 0;

    new_tick = rt_tick_get();

    rt_kprintf("this is hwtimer timeout callback fucntion!\n");
    rt_kprintf("tick is: %d!, run time is: %d\n", new_tick, new_tick - old_tick);

    old_tick = new_tick;

    return 0;
}

int hwtimer_app_sample(void)
{
    rt_err_t ret = RT_EOK;

    rt_hwtimerval_t timeout_s;    /* 定时器超时值 */
    rt_device_t hw_dev = RT_NULL; /* 定时器设备句柄 */
    rt_hwtimer_mode_t mode;       /* 定时器模式 */
    rt_uint32_t freq = 1000;      /* 计数频率 */

    rt_kprintf("hwtimer_app_sample\n");

    /* 查找定时器设备 */
    hw_dev = rt_device_find(HWTIMER_DEV_NAME);
    if (hw_dev == RT_NULL)
    {
        rt_kprintf("hwtimer sample run failed! can't find %s device!\n", HWTIMER_DEV_NAME);
        return -RT_ERROR;
    }

    /* 以读写方式打开设备 */
    ret = rt_device_open(hw_dev, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK)
    {
        rt_kprintf("open %s device failed!\n", HWTIMER_DEV_NAME);
        return ret;
    }

    /* 设置超时回调函数 */
    rt_device_set_rx_indicate(hw_dev, timeout_cb);

    /* 设置计数频率(若未设置该项，默认为1Mhz 或 支持的最小计数频率) */
    rt_device_control(hw_dev, HWTIMER_CTRL_FREQ_SET, &freq);
    /* 设置模式为周期性定时器（若未设置，默认是HWTIMER_MODE_ONESHOT）*/
    mode = HWTIMER_MODE_PERIOD;
    ret = rt_device_control(hw_dev, HWTIMER_CTRL_MODE_SET, &mode);
    if (ret != RT_EOK)
    {
        rt_kprintf("set mode failed! ret is :%d\n", ret);
        return ret;
    }

    /* 设置定时器超时值并启动定时器 */
    timeout_s.sec = 5;  /* 秒 */
    timeout_s.usec = 0; /* 微秒 */
    if (rt_device_write(hw_dev, 0, &timeout_s, sizeof(timeout_s)) != sizeof(timeout_s))
    {
        rt_kprintf("set timeout value failed\n");
        return -RT_ERROR;
    }

    /* 延时3500ms */
    rt_thread_mdelay(3500);

    /* 读取定时器当前值 */
    rt_device_read(hw_dev, 0, &timeout_s, sizeof(timeout_s));
    rt_kprintf("Read: Sec = %d, Usec = %d\n", timeout_s.sec, timeout_s.usec);

    return RT_EOK;
}

#endif