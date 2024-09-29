#include <drv_dac.h>

#ifdef RT_USING_DAC

#if !defined(BSP_USING_DAC)
#error "Please define at least one BSP_USING_DAC"
/* this driver can be disabled at menuconfig → RT-Thread Components → Device Drivers */
#endif

// #define DRV_DEBUG
#define LOG_TAG "drv.dac"
#include <drv_log.h>

static struct rt_dac_device uc8x88_dac_device;

static rt_err_t uc8x88_dac_enabled(struct rt_dac_device *device, rt_uint32_t channel)
{
    RT_ASSERT(device != RT_NULL);

    if (channel == 0)
    {
        dac_power_set(UC_ADDA);
        dac_clkdiv_set(UC_ADDA, 60);
    }
    else if (channel == 1)
    {
        auxdac_init(UC_ADDA);
        dac_power_set(UC_ADDA);
        dac_clkdiv_set(UC_ADDA, 60);
    }

    return RT_EOK;
}

static rt_err_t uc8x88_dac_disabled(struct rt_dac_device *device, rt_uint32_t channel)
{
    RT_ASSERT(device != RT_NULL);

    return RT_EOK;
}

static rt_err_t uc8x88_set_dac_value(struct rt_dac_device *device, rt_uint32_t channel, rt_uint32_t *value)
{
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(value != RT_NULL);

    /* set DAC value */
    if (channel == 0)
    {
        dac_write(UC_ADDA, *value);
    }
    else if (channel == 1)
    {
        auxdac_level_set(UC_ADDA, *value);
    }

    return RT_EOK;
}

static const struct rt_dac_ops uc8x88_dac_ops = {
    .disabled = uc8x88_dac_disabled,
    .enabled = uc8x88_dac_enabled,
    .convert = uc8x88_set_dac_value,
};

int rt_hw_dac_init(void)
{
    int result = RT_EOK;
    /* save adc name */
    char *name_buf = "dac";

    /* register DAC device */
    if (rt_hw_dac_register(&uc8x88_dac_device, name_buf, &uc8x88_dac_ops, RT_NULL) == RT_EOK)
    {
        LOG_D("%s register success", name_buf);
    }
    else
    {
        LOG_E("%s register failed", name_buf);
        result = -RT_ERROR;
    }

    return result;
}
INIT_BOARD_EXPORT(rt_hw_dac_init);

#endif /* BSP_USING_ADC */
