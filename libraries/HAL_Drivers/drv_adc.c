
#include <board.h>
#include<rtthread.h>
#include<rtdevice.h>

#ifdef RT_USING_ADC

#include "uc_adda.h"

//#define DRV_DEBUG
#define LOG_TAG             "drv.adc"
#include <drv_log.h>

static struct rt_adc_device uc8088_adc_device;

static rt_err_t uc8088_adc_enabled(struct rt_adc_device* device, rt_uint32_t channel, rt_bool_t enabled)
{
    RT_ASSERT(device != RT_NULL);

    if ((channel == 0) || (channel > 9))
    {
        return -RT_ERROR;
    }

    if (enabled)
    {
        if (channel == 1)
        {
            ADC_CHANNEL channel_val = ADC_CHANNEL_A;
            adc_power_set(UC_ADDA);
            adc_channel_select(UC_ADDA, channel_val);
        }
        else if (channel == 2)
        {
            ADC_CHANNEL channel_val = ADC_CHANNEL_B;
            adc_power_set(UC_ADDA);
            adc_channel_select(UC_ADDA, channel_val);
        }
        else if (channel == 3)
        {
            ADC_CHANNEL channel_val = ADC_CHANNEL_C;
            adc_power_set(UC_ADDA);
            adc_channel_select(UC_ADDA, channel_val);
        }
        else if (channel == 4)
        {
            /* 采集电池电压 */
            ADC_CHANNEL channel_val = ADC_CHANNEL_BAT;
            adc_power_set(UC_ADDA);
            adc_channel_select(UC_ADDA, channel_val);
            adc_vbat_measure_enable(true);
        }
        else if (channel == 5)
        {
            /* 采集电池GND电压 */
            ADC_CHANNEL channel_val = ADC_CHANNEL_BAT;
            adc_power_set(UC_ADDA);
            adc_channel_select(UC_ADDA, channel_val);
            adc_vbat_measure_enable(false);
        }
        else if (channel == 6)
        {
            ADC_CHANNEL channel_val = ADC_CHANNEL_TEMP;
            adc_power_set(UC_ADDA);
            adc_channel_select(UC_ADDA, channel_val);
            adc_temp_sensor_enable(UC_ADDA, true);
            adc_temp_source_sel(UC_ADDA, ADC_TEMP_A1);
        }
        else if (channel == 7)
        {
            ADC_CHANNEL channel_val = ADC_CHANNEL_TEMP;
            adc_power_set(UC_ADDA);
            adc_channel_select(UC_ADDA, channel_val);
            adc_temp_sensor_enable(UC_ADDA, true);
            adc_temp_source_sel(UC_ADDA, ADC_TEMP_A2);
        }
        else if (channel == 8)
        {
            ADC_CHANNEL channel_val = ADC_CHANNEL_TEMP;
            adc_power_set(UC_ADDA);
            adc_channel_select(UC_ADDA, channel_val);
            adc_temp_sensor_enable(UC_ADDA, true);
            adc_temp_source_sel(UC_ADDA, ADC_TEMP_B);
        }
        else if (channel == 9)
        {
            ADC_CHANNEL channel_val = ADC_CHANNEL_TEMP;
            adc_power_set(UC_ADDA);
            adc_channel_select(UC_ADDA, channel_val);
            adc_temp_sensor_enable(UC_ADDA, true);
            adc_temp_source_sel(UC_ADDA, ADC_TEMP_C);
        }
        adc_set_sample_rate(UC_ADDA, ADC_SR_45KSPS);
        adc_fifo_clear(UC_ADDA);
        adc_watermark_set(UC_ADDA, 100);
    }
    else
    {
        adc_fifo_clear(UC_ADDA);
    }

    return RT_EOK;
}

static rt_err_t uc8088_get_adc_value(struct rt_adc_device* device, rt_uint32_t channel, rt_uint32_t* value)
{
    rt_err_t ret_val = RT_EOK;
    uint32_t wait_count = 0;
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(value != RT_NULL);

    /* get ADC value */
    for (uint8_t index = 0; index < 100; index++)
    {
        adc_read(UC_ADDA);
    }
    adc_fifo_clear(UC_ADDA);
    adc_watermark_set(UC_ADDA, 100);
    while (is_adc_fifo_over_watermark(UC_ADDA))
    {
        if (wait_count < 1000)
        {
            wait_count++;
        }
        else
        {
            break;
        }
    }
    if (wait_count < 1000)
    {
        uint32_t adc_val = 0;
        for (uint8_t index = 0; index < 100; index++)
        {
            adc_wait_data_ready(UC_ADDA);
            adc_val += adc_read(UC_ADDA);
        }
        *value = adc_val / 100;
    }
    else
    {
        *value = 0;
        ret_val = RT_ERROR;
    }

    return ret_val;
}

static const struct rt_adc_ops uc8088_adc_ops =
{
    .enabled = uc8088_adc_enabled,
    .convert = uc8088_get_adc_value,
};

static int uc8088_adc_init(void)
{
    int result = RT_EOK;
    /* save adc name */
    char* name_buf = "adc";

    /* register ADC device */
    if (rt_hw_adc_register(&uc8088_adc_device, name_buf, &uc8088_adc_ops, RT_NULL) == RT_EOK)
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
INIT_BOARD_EXPORT(uc8088_adc_init);

#endif /* BSP_USING_ADC */

