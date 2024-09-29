#include <drv_adc.h>

#ifdef RT_USING_ADC

#if !defined(BSP_USING_ADC)
#error "Please define at least one BSP_USING_ADC"
/* this driver can be disabled at menuconfig → RT-Thread Components → Device Drivers */
#endif

// #define DRV_DEBUG
#define LOG_TAG "drv.adc"
#include <drv_log.h>

static struct rt_adc_device uc8x88_adc_device;

static rt_err_t uc8x88_adc_enabled(struct rt_adc_device *device, rt_uint32_t channel, rt_bool_t enabled)
{
    RT_ASSERT(device != RT_NULL);

    if ((channel == 0) || (channel > 9))
    {
        return -RT_ERROR;
    }

    if (enabled)
    {
        switch (channel)
        {
        case ADC_CONFIG_CHANNEL_A:
        {
            ADC_CHANNEL channel_val = ADC_CHANNEL_A;
            adc_power_set(UC_ADDA);
            adc_channel_select(UC_ADDA, channel_val);
            adc_set_sample_rate(UC_ADDA, ADC_SR_360KSPS);
            adc_fifo_clear(UC_ADDA);
            adc_watermark_set(UC_ADDA, 100);
            break;
        }
        case ADC_CONFIG_CHANNEL_B:
        {
            ADC_CHANNEL channel_val = ADC_CHANNEL_B;
            adc_power_set(UC_ADDA);
            adc_channel_select(UC_ADDA, channel_val);
            adc_set_sample_rate(UC_ADDA, ADC_SR_360KSPS);
            adc_fifo_clear(UC_ADDA);
            adc_watermark_set(UC_ADDA, 100);
            break;
        }
        case ADC_CONFIG_CHANNEL_C:
        {
            ADC_CHANNEL channel_val = ADC_CHANNEL_C;
            adc_power_set(UC_ADDA);
            adc_channel_select(UC_ADDA, channel_val);
            adc_set_sample_rate(UC_ADDA, ADC_SR_360KSPS);
            adc_fifo_clear(UC_ADDA);
            adc_watermark_set(UC_ADDA, 100);
            break;
        }
        case ADC_CONFIG_CHANNEL_BAT1:
        {
            /* 采集电池电压 */
            ADC_CHANNEL channel_val = ADC_CHANNEL_BAT;
            adc_power_set(UC_ADDA);
            adc_set_sample_rate(UC_ADDA, ADC_SR_360KSPS);
            adc_watermark_set(UC_ADDA, 128);
            adc_channel_select(UC_ADDA, channel_val);
            adc_vbat_measure_enable(true);
            adc_fifo_clear(UC_ADDA);
            break;
        }
#if 0
            case ADC_CONFIG_CHANNEL_BAT2:
            {
                /* 采集电池GND电压 */
                ADC_CHANNEL channel_val = ADC_CHANNEL_BAT;
                adc_power_set(UC_ADDA);
                adc_channel_select(UC_ADDA, channel_val);
                adc_vbat_measure_enable(false);
                adc_set_sample_rate(UC_ADDA, ADC_SR_45KSPS);
                adc_fifo_clear(UC_ADDA);
                adc_watermark_set(UC_ADDA, 100);
                break;
            }
#endif
        case ADC_CONFIG_CHANNEL_TEMP_A1:
        {
            ADC_CHANNEL channel_val = ADC_CHANNEL_TEMP;
            adc_power_set(UC_ADDA);
            adc_channel_select(UC_ADDA, channel_val);
            adc_temp_sensor_enable(UC_ADDA, true);
            adc_temp_source_sel(UC_ADDA, ADC_TEMP_A1);
            adc_set_sample_rate(UC_ADDA, ADC_SR_360KSPS);
            adc_fifo_clear(UC_ADDA);
            adc_watermark_set(UC_ADDA, 100);
            break;
        }
        case ADC_CONFIG_CHANNEL_TEMP_A2:
        {
            ADC_CHANNEL channel_val = ADC_CHANNEL_TEMP;
            adc_power_set(UC_ADDA);
            adc_channel_select(UC_ADDA, channel_val);
            adc_temp_sensor_enable(UC_ADDA, true);
            adc_temp_source_sel(UC_ADDA, ADC_TEMP_A2);
            adc_set_sample_rate(UC_ADDA, ADC_SR_360KSPS);
            adc_fifo_clear(UC_ADDA);
            adc_watermark_set(UC_ADDA, 100);
            break;
        }
        case ADC_CONFIG_CHANNEL_TEMP_B:
        {
            temp_in_b_config(UC_ADDA);
            // dc_off_control(1);
            // adc_fifo_clear(UC_ADDA);
            break;
        }
#if 0
            case ADC_CONFIG_CHANNEL_TEMP_C:
            {
                ADC_CHANNEL channel_val = ADC_CHANNEL_TEMP;
                adc_power_set(UC_ADDA);
                adc_channel_select(UC_ADDA, channel_val);
                adc_temp_sensor_enable(UC_ADDA, true);
                adc_temp_source_sel(UC_ADDA, ADC_TEMP_C);
                adc_set_sample_rate(UC_ADDA, ADC_SR_45KSPS);
                adc_fifo_clear(UC_ADDA);
                adc_watermark_set(UC_ADDA, 100);
                break;
            }
#endif
        case ADC_CONFIG_CHANNEL_CHIP_TEMP:
        {
            adc_watermark_set(UC_ADDA, 64);
            internal_temp_measure(UC_ADDA);
            adc_fifo_clear(UC_ADDA);
            break;
        }
        default:
            return RT_ERROR;
        }
    }
    else
    {
        switch (channel)
        {
        case ADC_CONFIG_CHANNEL_TEMP_B:
            dc_off_control(0);
            break;
        case ADC_CONFIG_CHANNEL_BAT1:
            adc_vbat_measure_enable(true);
            break;
        default:
            break;
        }
        adc_fifo_clear(UC_ADDA);
    }

    return RT_EOK;
}

static rt_err_t get_adc_value(struct rt_adc_device *device, rt_uint32_t channel, rt_uint32_t *value)
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

static rt_err_t uc8x88_get_adc_value(struct rt_adc_device *device, rt_uint32_t channel, rt_uint32_t *value)
{
    switch (channel)
    {
    case ADC_CONFIG_CHANNEL_A:
    case ADC_CONFIG_CHANNEL_B:
    case ADC_CONFIG_CHANNEL_C:
    {
        get_adc_value(device, channel, value);
        break;
    }
    case ADC_CONFIG_CHANNEL_BAT1:
    {
        *value = adc_battery_voltage(UC_ADDA);
        break;
    }
    case ADC_CONFIG_CHANNEL_TEMP_A1:
    case ADC_CONFIG_CHANNEL_TEMP_A2:
    {
        get_adc_value(device, channel, value);
        break;
    }
    case ADC_CONFIG_CHANNEL_TEMP_B:
    {
        *value = adc_read_temp_inb(UC_ADDA);
        break;
    }
    case ADC_CONFIG_CHANNEL_CHIP_TEMP:
    {
        *value = adc_temperature_read(UC_ADDA);
        break;
    }
    default:
        return RT_ERROR;
    }
    return RT_EOK;
}

static const struct rt_adc_ops uc8x88_adc_ops = {
    .enabled = uc8x88_adc_enabled,
    .convert = uc8x88_get_adc_value,
};

int rt_hw_adc_init(void)
{
    int result = RT_EOK;
    /* save adc name */
    char *name_buf = "adc";

    /* register ADC device */
    if (rt_hw_adc_register(&uc8x88_adc_device, name_buf, &uc8x88_adc_ops, RT_NULL) == RT_EOK)
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
INIT_BOARD_EXPORT(rt_hw_adc_init);

#endif /* BSP_USING_ADC */
