#include <drv_spi.h>

#ifdef RT_USING_SPI
#ifdef RT_USING_SPI_BITOPS

#if !defined(BSP_USING_SPI) || !defined(BSP_USING_SOFT_SPI_PIN_GROUP)
#error "Please define at least one BSP_USING_SOFT_SPI_PIN_GROUP"
/* this driver can be disabled at menuconfig → RT-Thread Components → Device Drivers */
#endif

#include <spi-bit-ops.h>

// #define DRV_DEBUG
#define LOG_TAG "drv.spi"
#include <drv_log.h>

struct uc8x88_hw_spi_cs
{
    GPIO_TypeDef *gpiox;
    GPIO_PIN gpio_pin;
};

/* uc8x88 config class */
struct uc8x88_soft_spi_config
{
    const char *bus_name;
    rt_uint8_t sck;
    rt_uint8_t mosi;
    rt_uint8_t miso;
};

/* uc8x88 soft spi dirver */
struct uc8x88_spi
{
    struct rt_spi_bit_obj spi;
    struct uc8x88_soft_spi_config *cfg;
};

#define SOFT_SPI_BUS_CONFIG {      \
    .bus_name = "sspi0",           \
    .sck = BSP_SOFT_SPI_SCK_PIN,   \
    .mosi = BSP_SOFT_SPI_MOSI_PIN, \
    .miso = BSP_SOFT_SPI_MISO_PIN, \
}

static const struct uc8x88_soft_spi_config soft_spi_config[] = {
    SOFT_SPI_BUS_CONFIG,
};

static struct uc8x88_spi spi_obj[sizeof(soft_spi_config) / sizeof(soft_spi_config[0])];

static void uc8x88_spi_gpio_init(struct uc8x88_spi *spi)
{
    struct uc8x88_soft_spi_config *cfg = (struct uc8x88_soft_spi_config *)spi->cfg;
    rt_pin_mode(cfg->sck, PIN_MODE_OUTPUT);
    rt_pin_mode(cfg->miso, PIN_MODE_INPUT);
    rt_pin_mode(cfg->mosi, PIN_MODE_OUTPUT);

    rt_pin_write(cfg->miso, PIN_HIGH);
    rt_pin_write(cfg->sck, PIN_HIGH);
    rt_pin_write(cfg->mosi, PIN_HIGH);
}

void uc8x88_tog_sclk(void *data)
{
    struct uc8x88_soft_spi_config *cfg = (struct uc8x88_soft_spi_config *)data;
    if (rt_pin_read(cfg->sck) == PIN_HIGH)
    {
        rt_pin_write(cfg->sck, PIN_LOW);
    }
    else
    {
        rt_pin_write(cfg->sck, PIN_HIGH);
    }
}

void uc8x88_set_sclk(void *data, rt_int32_t state)
{
    struct uc8x88_soft_spi_config *cfg = (struct uc8x88_soft_spi_config *)data;
    if (state)
    {
        rt_pin_write(cfg->sck, PIN_HIGH);
    }
    else
    {
        rt_pin_write(cfg->sck, PIN_LOW);
    }
}

void uc8x88_set_mosi(void *data, rt_int32_t state)
{
    struct uc8x88_soft_spi_config *cfg = (struct uc8x88_soft_spi_config *)data;
    if (state)
    {
        rt_pin_write(cfg->mosi, PIN_HIGH);
    }
    else
    {
        rt_pin_write(cfg->mosi, PIN_LOW);
    }
}

void uc8x88_set_miso(void *data, rt_int32_t state)
{
    struct uc8x88_soft_spi_config *cfg = (struct uc8x88_soft_spi_config *)data;
    if (state)
    {
        rt_pin_write(cfg->miso, PIN_HIGH);
    }
    else
    {
        rt_pin_write(cfg->miso, PIN_LOW);
    }
}

rt_int32_t uc8x88_get_sclk(void *data)
{
    struct uc8x88_soft_spi_config *cfg = (struct uc8x88_soft_spi_config *)data;
    return rt_pin_read(cfg->sck);
}

rt_int32_t uc8x88_get_mosi(void *data)
{
    struct uc8x88_soft_spi_config *cfg = (struct uc8x88_soft_spi_config *)data;
    return rt_pin_read(cfg->mosi);
}

rt_int32_t uc8x88_get_miso(void *data)
{
    struct uc8x88_soft_spi_config *cfg = (struct uc8x88_soft_spi_config *)data;
    return rt_pin_read(cfg->miso);
}

void uc8x88_dir_mosi(void *data, rt_int32_t state)
{
    struct uc8x88_soft_spi_config *cfg = (struct uc8x88_soft_spi_config *)data;
    if (state)
    {
        rt_pin_mode(cfg->mosi, PIN_MODE_INPUT);
    }
    else
    {
        rt_pin_mode(cfg->mosi, PIN_MODE_OUTPUT);
    }
}

void uc8x88_dir_miso(void *data, rt_int32_t state)
{
    struct uc8x88_soft_spi_config *cfg = (struct uc8x88_soft_spi_config *)data;
    if (state)
    {
        rt_pin_mode(cfg->miso, PIN_MODE_INPUT);
    }
    else
    {
        rt_pin_mode(cfg->miso, PIN_MODE_OUTPUT);
    }
}

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

static struct rt_spi_bit_ops uc8x88_soft_spi_ops = {
    .data = RT_NULL,
    .tog_sclk = uc8x88_tog_sclk,
    .set_sclk = uc8x88_set_sclk,
    .set_mosi = uc8x88_set_mosi,
    .set_miso = uc8x88_set_miso,
    .get_sclk = uc8x88_get_sclk,
    .get_mosi = uc8x88_get_mosi,
    .get_miso = uc8x88_get_miso,
    .dir_mosi = uc8x88_dir_mosi,
    .dir_miso = uc8x88_dir_miso,
    .udelay = uc8x88_udelay,
    .delay_us = 1,
};

/**
 * Attach the spi device to soft SPI bus, this function must be used after initialization.
 */
rt_err_t rt_hw_spi_device_attach(const char *bus_name, const char *device_name, GPIO_PIN cs_gpio_pin)
{
    RT_ASSERT(bus_name != RT_NULL);
    RT_ASSERT(device_name != RT_NULL);

    rt_err_t result;
    struct rt_spi_device *spi_device;
    struct uc8x88_hw_spi_cs *cs_pin;

    /* initialize the cs pin && select the slave*/
    gpio_set_pin_mux(UC_GPIO_CFG, cs_gpio_pin, GPIO_FUNC_0);
    gpio_set_pin_pupd(UC_GPIO_CFG, cs_gpio_pin, GPIO_PUPD_UP);
    gpio_set_pin_direction(UC_GPIO, cs_gpio_pin, GPIO_DIR_OUT);
    gpio_set_pin_value(UC_GPIO, cs_gpio_pin, GPIO_VALUE_HIGH);

    /* attach the device to spi bus*/
    spi_device = (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    RT_ASSERT(spi_device != RT_NULL);
    cs_pin = (struct uc8x88_hw_spi_cs *)rt_malloc(sizeof(struct uc8x88_hw_spi_cs));
    RT_ASSERT(cs_pin != RT_NULL);
    cs_pin->gpiox = UC_GPIO;
    cs_pin->gpio_pin = cs_gpio_pin;
    result = rt_spi_bus_attach_device(spi_device, device_name, bus_name, (void *)cs_pin);

    if (result != RT_EOK)
    {
        LOG_E("%s attach to %s faild, %d\n", device_name, bus_name, result);
    }

    RT_ASSERT(result == RT_EOK);

    LOG_D("%s attach to %s done", device_name, bus_name);

    return result;
}

/* Soft SPI initialization function */
int rt_hw_spi_init(void)
{
    rt_size_t obj_num = sizeof(spi_obj) / sizeof(struct uc8x88_spi);
    rt_err_t result;

    for (int i = 0; i < obj_num; i++)
    {
        uc8x88_soft_spi_ops.data = (void *)&soft_spi_config[i];
        spi_obj[i].spi.ops = &uc8x88_soft_spi_ops;
        spi_obj[i].cfg = (void *)&soft_spi_config[i];
        uc8x88_spi_gpio_init(&spi_obj[i]);
        result = rt_spi_bit_add_bus(&spi_obj[i].spi, soft_spi_config[i].bus_name, &uc8x88_soft_spi_ops);
        RT_ASSERT(result == RT_EOK);
    }

    return RT_EOK;
}
INIT_BOARD_EXPORT(rt_hw_spi_init);

#endif

#endif
