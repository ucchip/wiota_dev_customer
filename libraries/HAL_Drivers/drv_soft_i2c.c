#include "drv_i2c.h"

#ifdef RT_USING_I2C
#ifdef RT_USING_I2C_BITOPS

#if !defined(BSP_USING_I2C) || !defined(BSP_USING_SOFT_I2C_PIN_GROUP)
#error "Please define BSP_USING_SOFT_I2C_PIN_GROUP"
/* this driver can be disabled at menuconfig → RT-Thread Components → Device Drivers */
#endif

#ifdef RT_I2C_DEBUG
#define DRV_DEBUG
#endif
#define LOG_TAG "drv.i2c"
#include <drv_log.h>

/* uc8x88 config class */
struct uc8x88_soft_i2c_config
{
    const char *bus_name;
    rt_uint8_t scl;
    rt_uint8_t sda;
};

/* uc8x88 i2c dirver class */
struct uc8x88_i2c
{
    struct rt_i2c_bit_ops ops;
    struct rt_i2c_bus_device i2c_bus;
};

#define SOFT_I2C_CONFIG {        \
    .bus_name = "i2c0",          \
    .scl = BSP_SOFT_I2C_SCL_PIN, \
    .sda = BSP_SOFT_I2C_SDA_PIN, \
}

static const struct uc8x88_soft_i2c_config soft_i2c_config[] = {
    SOFT_I2C_CONFIG,
};

static struct uc8x88_i2c i2c_obj[sizeof(soft_i2c_config) / sizeof(soft_i2c_config[0])];

/**
 * This function initializes the i2c pin.
 *
 * @param uc8x88 i2c dirver class.
 */
static void uc8x88_i2c_gpio_init(struct uc8x88_i2c *i2c)
{
    struct uc8x88_soft_i2c_config *cfg = (struct uc8x88_soft_i2c_config *)i2c->ops.data;

    rt_pin_mode(cfg->scl, PIN_MODE_OUTPUT_OD);
    rt_pin_mode(cfg->sda, PIN_MODE_OUTPUT_OD);

    rt_pin_write(cfg->scl, PIN_HIGH);
    rt_pin_write(cfg->sda, PIN_HIGH);
}

/**
 * This function sets the sda pin.
 *
 * @param uc8x88 config class.
 * @param The sda pin state.
 */
static void uc8x88_set_sda(void *data, rt_int32_t state)
{
    struct uc8x88_soft_i2c_config *cfg = (struct uc8x88_soft_i2c_config *)data;
    rt_pin_mode(cfg->sda, PIN_MODE_OUTPUT_OD);
    if (state)
    {
        rt_pin_write(cfg->sda, PIN_HIGH);
    }
    else
    {
        rt_pin_write(cfg->sda, PIN_LOW);
    }
}

/**
 * This function sets the scl pin.
 *
 * @param uc8x88 config class.
 * @param The scl pin state.
 */
static void uc8x88_set_scl(void *data, rt_int32_t state)
{
    struct uc8x88_soft_i2c_config *cfg = (struct uc8x88_soft_i2c_config *)data;
    rt_pin_mode(cfg->scl, PIN_MODE_OUTPUT_OD);
    if (state)
    {
        rt_pin_write(cfg->scl, PIN_HIGH);
    }
    else
    {
        rt_pin_write(cfg->scl, PIN_LOW);
    }
}

/**
 * This function gets the sda pin state.
 *
 * @param The sda pin state.
 */
static rt_int32_t uc8x88_get_sda(void *data)
{
    struct uc8x88_soft_i2c_config *cfg = (struct uc8x88_soft_i2c_config *)data;
    rt_pin_mode(cfg->sda, PIN_MODE_INPUT);
    return rt_pin_read(cfg->sda);
}

/**
 * This function gets the scl pin state.
 *
 * @param The scl pin state.
 */
static rt_int32_t uc8x88_get_scl(void *data)
{
    struct uc8x88_soft_i2c_config *cfg = (struct uc8x88_soft_i2c_config *)data;
    rt_pin_mode(cfg->scl, PIN_MODE_INPUT);
    return rt_pin_read(cfg->scl);
}
/**
 * The time delay function.
 *
 * @param microseconds.
 */
static void uc8x88_udelay(rt_uint32_t us)
{
    rt_uint32_t ticks;
    rt_uint32_t told, tnow, tcnt = 0;
    rt_uint32_t Compare_Value = ((rt_uint32_t)BSP_CLOCK_SYSTEM_FREQ_HZ) / (5 * RT_TICK_PER_SECOND);

    ticks = us * Compare_Value / (1000000 / RT_TICK_PER_SECOND);
    told = timer_get_count(UC_TIMER0);
    while (1)
    {
        tnow = timer_get_count(UC_TIMER0);
        if (tnow != told)
        {
            if (tnow > told)
            {
                tcnt += tnow - told;
            }
            else
            {
                tcnt += Compare_Value - told + tnow;
            }
            told = tnow;
            if (tcnt >= ticks)
            {
                break;
            }
        }
    }
}

static const struct rt_i2c_bit_ops uc8x88_bit_ops_default = {
    .data = RT_NULL,
    .set_sda = uc8x88_set_sda,
    .set_scl = uc8x88_set_scl,
    .get_sda = uc8x88_get_sda,
    .get_scl = uc8x88_get_scl,
    .udelay = uc8x88_udelay,
    .delay_us = 100,
    .timeout = 100,
};

/**
 * if i2c is locked, this function will unlock it
 *
 * @param uc8x88 config class
 *
 * @return RT_EOK indicates successful unlock.
 */
static rt_err_t uc8x88_i2c_bus_unlock(const struct uc8x88_soft_i2c_config *cfg)
{
    rt_int32_t i = 0;

    rt_pin_mode(cfg->scl, PIN_MODE_OUTPUT_OD);
    rt_pin_mode(cfg->sda, PIN_MODE_INPUT);
    if (PIN_LOW == rt_pin_read(cfg->sda))
    {
        while (i++ < 9)
        {
            rt_pin_write(cfg->scl, PIN_HIGH);
            uc8x88_udelay(100);
            rt_pin_write(cfg->scl, PIN_LOW);
            uc8x88_udelay(100);
        }
    }
    if (PIN_LOW == rt_pin_read(cfg->sda))
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

/* I2C initialization function */
int rt_hw_i2c_init(void)
{
    rt_size_t obj_num = sizeof(i2c_obj) / sizeof(struct uc8x88_i2c);
    rt_err_t result = RT_EOK;

    for (int i = 0; i < obj_num; i++)
    {
        i2c_obj[i].ops = uc8x88_bit_ops_default;
        i2c_obj[i].ops.data = (void *)&soft_i2c_config[i];
        i2c_obj[i].i2c_bus.priv = &i2c_obj[i].ops;
        uc8x88_i2c_gpio_init(&i2c_obj[i]);
        result = rt_i2c_bit_add_bus(&i2c_obj[i].i2c_bus, soft_i2c_config[i].bus_name);
        RT_ASSERT(result == RT_EOK);
        uc8x88_i2c_bus_unlock(&soft_i2c_config[i]);

        LOG_D("software simulation %s init done, pin scl: %d, pin sda %d",
              soft_i2c_config[i].bus_name,
              soft_i2c_config[i].scl,
              soft_i2c_config[i].sda);
    }

    return result;
}
INIT_BOARD_EXPORT(rt_hw_i2c_init);

#endif
#endif /* RT_USING_I2C */
