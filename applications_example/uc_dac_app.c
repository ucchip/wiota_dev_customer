#include <rtthread.h>

#ifdef APP_EXAMPLE_DAC

#ifndef RT_USING_DAC
#error "Please enable rt-thread dac device driver"
#endif

#ifndef BSP_USING_DAC
#error "Please enable on-chip peripheral dac config"
#endif

#include <rtdevice.h>

#define THREAD_STACK_SIZE 512
#define THREAD_PRIORITY 5
#define THREAD_TIMESLICE 5

#define DAC_DEVICE_NAME "dac"
#define DAC_DEV_CHANNEL 1

static rt_dac_device_t dac_dev = RT_NULL;

static int dac_app_set_value(rt_dac_device_t dac_dev, rt_uint32_t value)
{
    rt_dac_enable(dac_dev, DAC_DEV_CHANNEL);
    rt_dac_write(dac_dev, DAC_DEV_CHANNEL, value);
    rt_dac_disable(dac_dev, DAC_DEV_CHANNEL);

    return RT_EOK;
}

static void dac_thread_entry(void *parameter)
{
    rt_uint32_t value = 0;

    while (1)
    {
        rt_kprintf("dac output value = %d\n", value);
        dac_app_set_value(dac_dev, value); // 10bit, 0~1023
        value += 10;
        if (value > 1023)
        {
            value = 0;
        }
        rt_thread_mdelay(100);
    }
}

int dac_app_sample(void)
{
    rt_thread_t thread = RT_NULL;

    rt_kprintf("dac_app_sample\n");

    dac_dev = (rt_dac_device_t)rt_device_find(DAC_DEVICE_NAME);
    if (dac_dev == RT_NULL)
    {
        rt_kprintf("find %s failed!\n", DAC_DEVICE_NAME);
        return -RT_EEMPTY;
    }

    /* 创建 serial 线程 */
    thread = rt_thread_create("dac_app",
                              dac_thread_entry,
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
