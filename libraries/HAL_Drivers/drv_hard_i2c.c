#include <drv_i2c.h>

#ifdef RT_USING_I2C
#ifndef RT_USING_I2C_BITOPS

#if !defined(BSP_USING_I2C) || (!defined(BSP_USING_HARD_I2C_PIN_GROUP_1) && !defined(BSP_USING_HARD_I2C_PIN_GROUP_2))
#error "Please define at least one BSP_USING_HARD_I2C_PIN_GROUP"
/* this driver can be disabled at menuconfig → RT-Thread Components → Device Drivers */
#endif

#ifdef RT_I2C_DEBUG
#define DRV_DEBUG
#endif
#define LOG_TAG "drv.i2c"
#include <drv_log.h>

/* uc8x88 config class */
struct uc8x88_hard_i2c_config
{
    const char *bus_name;
    I2C_TYPE *base;
    rt_uint32_t clock;
};

/* uc8x88 i2c dirver class */
struct uc8x88_i2c
{
    struct rt_i2c_bus_device i2c_bus;
};

#define HARD_I2C_CONFIG { \
    .bus_name = "i2c0",   \
    .base = UC_I2C,       \
    .clock = 100000,      \
}

static const struct uc8x88_hard_i2c_config hard_i2c_config = HARD_I2C_CONFIG;

static struct uc8x88_i2c i2c_obj;

static rt_err_t uc8x88_i2c_gpio_init(void)
{
    GPIO_PIN scl_pin, sda_pin;
    GPIO_FUNCTION gpio_func;

    // GPIO 5 / 6  OR GPIO4 / 14
#if defined(BSP_USING_HARD_I2C_PIN_GROUP_1)
    scl_pin = GPIO_PIN_5;
    sda_pin = GPIO_PIN_6;
    gpio_func = GPIO_FUNC_1;
#elif defined(BSP_USING_HARD_I2C_PIN_GROUP_2)
    scl_pin = GPIO_PIN_4;
    sda_pin = GPIO_PIN_14;
    gpio_func = GPIO_FUNC_2;
#else
#error "Please define at least one BSP_USING_HARD_I2C_PIN_GROUP"
#endif

    gpio_set_pin_mux(UC_GPIO_CFG, scl_pin, gpio_func);
    gpio_set_pin_pupd(UC_GPIO_CFG, scl_pin, GPIO_PUPD_UP);
    gpio_set_pin_mux(UC_GPIO_CFG, sda_pin, gpio_func);
    gpio_set_pin_pupd(UC_GPIO_CFG, sda_pin, GPIO_PUPD_UP);

    return RT_EOK;
}

static rt_err_t uc8x88_i2c_clock_set(struct rt_i2c_bus_device *bus, rt_uint32_t clock)
{
    I2C_CFG_Type config;
    struct uc8x88_hard_i2c_config *cfg = (struct uc8x88_hard_i2c_config *)bus->priv;
    cfg->clock = clock;

    config.Enable = 1;
    config.prescaler = BSP_CLOCK_SYSTEM_FREQ_HZ / (5 * cfg->clock) - 1;

    i2c_setup(cfg->base, &config);

    return RT_EOK;
}

static rt_err_t uc8x88_i2c_init(struct rt_i2c_bus_device *bus)
{
    struct uc8x88_hard_i2c_config *cfg = (struct uc8x88_hard_i2c_config *)bus->priv;

    uc8x88_i2c_gpio_init();
    uc8x88_i2c_clock_set(bus, cfg->clock);

    return RT_EOK;
}

static rt_err_t uc8x88_i2c_bus_control(struct rt_i2c_bus_device *bus, rt_uint32_t cmd, rt_uint32_t args)
{
    rt_err_t ret;
    rt_uint32_t bus_clock;

    RT_ASSERT(bus != RT_NULL);

    switch (cmd)
    {
    case RT_I2C_DEV_CTRL_CLK:
        bus_clock = *(rt_uint32_t *)args;
        ret = uc8x88_i2c_clock_set(bus, bus_clock);
        break;
    default:
        LOG_E("unknown cmd: %d", cmd);
        ret = -RT_EINVAL;
        break;
    }

    return ret;
}

static rt_size_t uc8x88_i2c_write(struct rt_i2c_bus_device *bus, struct rt_i2c_msg *msg)
{
    unsigned int i = 0;
    unsigned int address = msg->addr << 1; // bit0 = 0 ->write
    struct uc8x88_hard_i2c_config *cfg = (struct uc8x88_hard_i2c_config *)bus->priv;

    LOG_D("send write start condition");
    i2c_send_data(cfg->base, address);
    i2c_send_command(cfg->base, I2C_START_WRITE);
    i2c_get_ack(cfg->base);

    LOG_D("write data start");
    for (i = 0; i < msg->len; i++)
    {
        i2c_send_data(cfg->base, msg->buf[i]);
        i2c_send_command(cfg->base, I2C_WRITE);
        i2c_get_ack(cfg->base);
        LOG_D("write byte %d: 0x%x", i, msg->buf[i]);
    }

    LOG_D("send stop condition");
    i2c_send_command(cfg->base, I2C_STOP);
    while (i2c_busy(cfg->base))
    {
        asm("nop");
    }

    LOG_D("write %d byte%s", i, i == 1 ? "" : "s");

    return i;
}

static rt_size_t uc8x88_i2c_read(struct rt_i2c_bus_device *bus, struct rt_i2c_msg *msg)
{
    unsigned int address = (msg->addr << 1) | 0x01; // bit0 = 1 ->read
    unsigned int i = 0;
    struct uc8x88_hard_i2c_config *cfg = (struct uc8x88_hard_i2c_config *)bus->priv;

    LOG_D("send read start condition");
    i2c_send_data(cfg->base, address);
    i2c_send_command(cfg->base, I2C_START_WRITE);
    i2c_get_ack(cfg->base);

    LOG_D("read data start");
    for (i = 0; i < msg->len; i++)
    {
        i2c_send_command(cfg->base, I2C_READ);
        i2c_get_ack(cfg->base);
        msg->buf[i] = i2c_get_data(cfg->base);
        LOG_D("read byte %d: 0x%x", i, msg->buf[i]);
    }

    LOG_D("send read stop condition");
    i2c_send_command(cfg->base, I2C_STOP_READ);
    while (i2c_busy(cfg->base))
    {
        asm("nop");
    }

    LOG_D("read %d byte%s", i, i == 1 ? "" : "s");

    return i;
}

static rt_size_t uc8x88_i2c_master_xfer(struct rt_i2c_bus_device *bus, struct rt_i2c_msg msgs[], rt_uint32_t num)
{
    struct rt_i2c_msg *msg;
    rt_int32_t i, ret;

    if (num == 0)
    {
        return 0;
    }

    for (i = 0; i < num; i++)
    {
        msg = &msgs[i];
        LOG_D("xfer msgs[%d] addr=0x%2x buf=0x%x len= 0x%x flags= 0x%x", i, msg->addr, msg->buf, msg->len, msg->flags);
        if (msg->flags == RT_I2C_RD)
        {
            uc8x88_i2c_read(bus, msg);
        }
        else if (msg->flags == RT_I2C_WR)
        {
            uc8x88_i2c_write(bus, msg);
        }
    }
    ret = i;
    return ret;
}

static const struct rt_i2c_bus_device_ops uc8x88_i2c_ops = {
    .master_xfer = uc8x88_i2c_master_xfer,
    .slave_xfer = RT_NULL,
    .i2c_bus_control = uc8x88_i2c_bus_control,
};

int rt_i2c_hw_init(void)
{
    rt_err_t result = RT_EOK;

    i2c_obj.i2c_bus.ops = &uc8x88_i2c_ops;
    i2c_obj.i2c_bus.priv = (void *)&hard_i2c_config;
    uc8x88_i2c_init(&i2c_obj.i2c_bus);
    result = rt_i2c_bus_device_register(&i2c_obj.i2c_bus, hard_i2c_config.bus_name);
    RT_ASSERT(result == RT_EOK);

    LOG_D("%s bus init done", hard_i2c_config.bus_name);

    return result;
}
INIT_BOARD_EXPORT(rt_i2c_hw_init);

#endif
#endif
