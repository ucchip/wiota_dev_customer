#include <rtthread.h>

#ifdef APP_EXAMPLE_ADC

#ifndef RT_USING_ADC
#error "Please enable rt-thread adc device driver"
#endif

#ifndef BSP_USING_ADC
#error "Please enable on-chip peripheral adc config"
#endif

#include <rtdevice.h>
#include <drv_adc.h>
#include "uc_adc_app.h"

#define THREAD_STACK_SIZE 512
#define THREAD_PRIORITY 5
#define THREAD_TIMESLICE 5

#define ADC_DEVICE_NAME "adc"
#define ADC_DEV_CHANNEL ADC_CONFIG_CHANNEL_A

static rt_adc_device_t adc_dev = RT_NULL;

static int adc_app_read_value(rt_adc_device_t adc_dev, rt_uint32_t *value)
{
    rt_adc_enable(adc_dev, ADC_DEV_CHANNEL);
    *value = rt_adc_read(adc_dev, ADC_DEV_CHANNEL);
    rt_adc_disable(adc_dev, ADC_DEV_CHANNEL);

    return RT_EOK;
}

static void adc_thread_entry(void *parameter)
{
    rt_uint32_t value = 0, value_old = 0;

    while (1)
    {
        adc_app_read_value(adc_dev, &value);
        if (value_old != value)
        {
            rt_kprintf("adc value = %d, 10bit value = %d\n", value, (value >> 2));
            value_old = value;
        }
        rt_thread_mdelay(100);
    }
}

int adc_app_sample(void)
{
    rt_thread_t thread = RT_NULL;

    rt_kprintf("adc_app_sample\n");

    adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEVICE_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("find %s failed!\n", ADC_DEVICE_NAME);
        return -RT_EEMPTY;
    }

    /* 创建 serial 线程 */
    thread = rt_thread_create("adc_app",
                              adc_thread_entry,
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
