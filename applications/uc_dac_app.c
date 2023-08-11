#include <rtthread.h>
#ifdef _DAC_APP_
#include <rtdevice.h>
#include "uc_dac_app.h"

#define DAC_DEVICE_NAME    "dac"
#define DAC_DEV_CHANNEL     1


static rt_device_t dac_dev = NULL;

int dac_app_init(void)
{
    rt_err_t ret = RT_EOK;

    dac_dev = rt_device_find(DAC_DEVICE_NAME);
    if (dac_dev == RT_NULL)
    {
        rt_kprintf("find %s failed!\n", DAC_DEVICE_NAME);
        return RT_EEMPTY;
    }
    
    return ret;
}

int dac_app_set_value(rt_uint32_t value)
{
    rt_err_t ret = RT_EOK;
    
    if (dac_dev == RT_NULL)
    {
        rt_kprintf("find %s failed!\n", DAC_DEVICE_NAME);
        return RT_EEMPTY;
    }
    
    rt_dac_enable(dac_dev, DAC_DEV_CHANNEL);
    rt_dac_write(dac_dev, DAC_DEV_CHANNEL, value);
    rt_dac_disable(dac_dev, DAC_DEV_CHANNEL);
    
    return ret;
}

void dac_app_sample(void)
{
    int ret = 0;
    
    rt_kprintf("dac test demo.\r\n");
    
    ret = dac_app_init();
    if(ret != RT_EOK)
    {
        rt_kprintf("init adc device failed!\n");
        return;
    }
    
    ret = dac_app_set_value(3000);
    if(ret != RT_EOK)
    {
        rt_kprintf("dac test failed!\n");
    }
    else
    {
        rt_kprintf("dac output done!\n");
    }
}

#endif
