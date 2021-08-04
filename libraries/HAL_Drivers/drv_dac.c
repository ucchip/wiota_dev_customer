
#include <board.h>
#include<rtthread.h>
#include<rtdevice.h>

#ifdef RT_USING_DAC

#include "uc_adda.h"

//#define DRV_DEBUG
#define LOG_TAG             "drv.dac"
#include <drv_log.h>

static struct rt_dac_device uc8088_dac_device;

static rt_err_t uc8088_dac_enabled(struct rt_dac_device* device, rt_uint32_t channel)
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

static rt_err_t uc8088_dac_disabled(struct rt_dac_device* device, rt_uint32_t channel)
{
    RT_ASSERT(device != RT_NULL);

    return RT_EOK;
}

static rt_err_t uc8088_set_dac_value(struct rt_dac_device* device, rt_uint32_t channel, rt_uint32_t* value)
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

static const struct rt_dac_ops uc8088_dac_ops =
{
    .disabled = uc8088_dac_disabled,
    .enabled = uc8088_dac_enabled,
    .convert = uc8088_set_dac_value,
};

static int uc8088_dac_init(void)
{
    int result = RT_EOK;
    /* save adc name */
    char* name_buf = "dac";

    /* register DAC device */
    if (rt_hw_dac_register(&uc8088_dac_device, name_buf, &uc8088_dac_ops, RT_NULL) == RT_EOK)
    {
        LOG_D("%s init success", name_buf);
    }
    else
    {
        LOG_E("%s register failed", name_buf);
        result = -RT_ERROR;
    }

    return result;
}
INIT_BOARD_EXPORT(uc8088_dac_init);

#endif /* BSP_USING_ADC */

