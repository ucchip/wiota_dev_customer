#include <rtthread.h>
#ifdef _ADC_APP_
#include <rtdevice.h>
#include "uc_adc_app.h"

#define ADC_DEVICE_NAME    "adc"
#define ADC_DEV_CHANNEL     1


static rt_device_t adc_dev = NULL;

int adc_app_init(void)
{
    rt_err_t ret = RT_EOK;

    adc_dev = rt_device_find(ADC_DEVICE_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("find %s failed!\n", ADC_DEVICE_NAME);
        return RT_EEMPTY;
    }
    
    return ret;
}

int adc_app_read_value(rt_uint32_t *value)
{
    rt_err_t ret = RT_EOK;
    
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("find %s failed!\n", ADC_DEVICE_NAME);
        return RT_EEMPTY;
    }
    
    rt_adc_enable(adc_dev, ADC_DEV_CHANNEL);
    *value = rt_adc_read(adc_dev, ADC_DEV_CHANNEL);
    rt_adc_disable(adc_dev, ADC_DEV_CHANNEL);
    
    return ret;
}

void adc_app_sample(void)
{
    int ret = 0;
    rt_uint32_t value = 0;
    
    rt_kprintf("adc test demo.\r\n");
    
    ret = adc_app_init();
    if(ret != RT_EOK)
    {
        rt_kprintf("init adc device failed!\n");
        return;
    }
    
    while(1)
    {
        ret = adc_app_read_value(&value);
        if(ret == RT_EOK)
        {
            rt_kprintf("adc value = %d.\r\n", value);
        }
        else
        {
            rt_kprintf("read adc failed.\r\n");
        }
    }
}

#endif
