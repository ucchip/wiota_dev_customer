#include <drv_spi.h>

#ifdef RT_USING_SPI
#ifndef RT_USING_SPI_BITOPS

#if !defined(BSP_USING_SPI) || (!defined(BSP_USING_HARD_SPI_PIN_GROUP_1) && !defined(BSP_USING_HARD_SPI_PIN_GROUP_2))
#error "Please define at least one BSP_USING_HARD_SPI_PIN_GROUP"
/* this driver can be disabled at menuconfig → RT-Thread Components → Device Drivers */
#endif

// #define DRV_DEBUG
#define LOG_TAG "drv.spi"
#include <drv_log.h>

struct uc8x88_hw_spi_cs
{
    GPIO_TypeDef *gpiox;
    GPIO_PIN gpio_pin;
};

/* uc8x88 config class */
struct uc8x88_hard_spi_config
{
    const char *bus_name;
    SPI_TypeDef *base;
    rt_uint32_t clock;
};

struct uc8x88_spi
{
    struct uc8x88_hard_spi_config *config;
    struct rt_spi_configuration *cfg;
    struct rt_spi_bus spi_bus;
};

#define HARD_SPI_CONFIG {       \
    .bus_name = "spi0",         \
    .base = UC_SPIM,            \
    .clock = (1 * 1000 * 1000), \
}

static struct uc8x88_hard_spi_config hard_spi_config = HARD_SPI_CONFIG;

static struct uc8x88_spi spi_obj;

static rt_err_t uc8x88_spi_gpio_init(void)
{
    GPIO_PIN sck_pin, mosi_pin, miso_pin;
    GPIO_FUNCTION gpio_func;

#if defined(BSP_USING_HARD_SPI_PIN_GROUP_1)
    sck_pin = GPIO_PIN_0;
    mosi_pin = GPIO_PIN_3;
    miso_pin = GPIO_PIN_2;
    gpio_func = GPIO_FUNC_2;
#elif defined(BSP_USING_HARD_SPI_PIN_GROUP_2)
    sck_pin = GPIO_PIN_8;
    mosi_pin = GPIO_PIN_9;
    miso_pin = GPIO_PIN_10;
    gpio_func = GPIO_FUNC_1;
#else
#error "Please define at least one BSP_USING_HARD_SPI_PIN_GROUP"
#endif

    gpio_set_pin_mux(UC_GPIO_CFG, sck_pin, gpio_func);
    gpio_set_pin_pupd(UC_GPIO_CFG, sck_pin, GPIO_PUPD_UP);
    gpio_set_pin_direction(UC_GPIO, sck_pin, GPIO_DIR_OUT);

    gpio_set_pin_mux(UC_GPIO_CFG, mosi_pin, gpio_func);
    gpio_set_pin_pupd(UC_GPIO_CFG, mosi_pin, GPIO_PUPD_UP);
    gpio_set_pin_direction(UC_GPIO, mosi_pin, GPIO_DIR_OUT);

    gpio_set_pin_mux(UC_GPIO_CFG, miso_pin, gpio_func);
    gpio_set_pin_pupd(UC_GPIO_CFG, miso_pin, GPIO_PUPD_UP);
    gpio_set_pin_direction(UC_GPIO, miso_pin, GPIO_DIR_IN);

    return RT_EOK;
}

static rt_err_t uc8x88_spi_clock_set(struct uc8x88_spi *spi_drv, rt_uint32_t max_hz)
{
    SPIM_CFG_Type SPI_ConfigStruc;
    rt_uint32_t clk_dlv = 255;

    if (max_hz > 0)
    {
        clk_dlv = BSP_CLOCK_SYSTEM_FREQ_HZ / max_hz / 2;
        if (clk_dlv > 0)
        {
            clk_dlv -= 1;
        }
        if (clk_dlv > 255)
        {
            clk_dlv = 255;
        }
    }
    SPI_ConfigStruc.Clk_rate = clk_dlv & 0xff;
    spim_init(spi_drv->config->base, &SPI_ConfigStruc);

    LOG_D("spi clock set to %dHz", max_hz);

    return RT_EOK;
}

static rt_err_t uc8x88_spi_init(struct uc8x88_spi *spi_drv, struct rt_spi_configuration *cfg)
{
    uc8x88_spi_gpio_init();
    uc8x88_spi_clock_set(spi_drv, cfg->max_hz);

    return RT_EOK;
}

static rt_err_t ux8x88_spi_configure(struct rt_spi_device *device, struct rt_spi_configuration *configuration)
{
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(configuration != RT_NULL);

    struct uc8x88_spi *spi_drv = rt_container_of(device->bus, struct uc8x88_spi, spi_bus);
    spi_drv->cfg = configuration;

    return uc8x88_spi_init(spi_drv, configuration);
}

static rt_uint32_t ux8x88_spi_xfer(struct rt_spi_device *device, struct rt_spi_message *message)
{
    rt_size_t message_length, already_send_length;
    rt_uint16_t send_length;
    char *recv_buf;
    char *send_buf;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);
    RT_ASSERT(device->bus->parent.user_data != RT_NULL);
    RT_ASSERT(message != RT_NULL);

    struct uc8x88_spi *spi_drv = rt_container_of(device->bus, struct uc8x88_spi, spi_bus);
    SPI_TypeDef *spi_handle = spi_drv->config->base;
    struct uc8x88_hw_spi_cs *cs = device->parent.user_data;

    if (message->cs_take && !(device->config.mode & RT_SPI_NO_CS))
    {
        if (device->config.mode & RT_SPI_CS_HIGH)
            gpio_set_pin_value(cs->gpiox, cs->gpio_pin, GPIO_VALUE_HIGH);
        else
            gpio_set_pin_value(cs->gpiox, cs->gpio_pin, GPIO_VALUE_LOW);
    }

    LOG_D("%s transfer prepare and start", spi_drv->config->bus_name);
    LOG_D("%s sendbuf: %X, recvbuf: %X, length: %d",
          spi_drv->config->bus_name,
          (uint32_t)message->send_buf,
          (uint32_t)message->recv_buf, message->length);

    message_length = message->length;
    recv_buf = (char *)message->recv_buf;
    send_buf = (char *)message->send_buf;
    while (message_length)
    {
        // 一次最多传输8192-4=8188字节数据
        if (message_length > 8188)
        {
            send_length = 8188;
            message_length = message_length - 8188;
        }
        else
        {
            send_length = message_length;
            message_length = 0;
        }

        /* calculate the start address */
        already_send_length = message->length - send_length - message_length;
        send_buf = (char *)message->send_buf + already_send_length;
        recv_buf = (char *)message->recv_buf + already_send_length;
        /* start once data exchange */
        if (message->send_buf && message->recv_buf)
        {
#ifdef BSP_SPIM_TX_USING_DMA
            Udma_Spim_Tx((uint32_t *)data_buf, data_len / 4);
#else
            spim_transmit_send_recv(spi_handle, send_buf, recv_buf, send_length);
#endif
        }
        else if (message->send_buf)
        {
#ifdef BSP_SPIM_TX_USING_DMA
            Udma_Spim_Tx((uint32_t *)data_buf, data_len / 4);
#else
            spim_transmit_send(spi_handle, send_buf, send_length);
#endif
        }
        else
        {
#ifdef BSP_SPIM_RX_USING_DMA
            Udma_Spim_Rx((uint32_t *)data_buf, data_len / 4);
#else
            spim_transmit_recv(spi_handle, recv_buf, send_length);
#endif
        }
    }

    if (message->cs_release && !(device->config.mode & RT_SPI_NO_CS))
    {
        if (device->config.mode & RT_SPI_CS_HIGH)
            gpio_set_pin_value(cs->gpiox, cs->gpio_pin, GPIO_VALUE_LOW);
        else
            gpio_set_pin_value(cs->gpiox, cs->gpio_pin, GPIO_VALUE_HIGH);
    }

    return message->length;
}

static const struct rt_spi_ops uc8x88_spi_ops = {
    .configure = ux8x88_spi_configure,
    .xfer = ux8x88_spi_xfer,
};

/**
 * Attach the spi device to SPI bus, this function must be used after initialization.
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

int rt_hw_spi_init(void)
{
    rt_err_t result;

    spi_obj.config = &hard_spi_config;
    spi_obj.spi_bus.parent.user_data = &hard_spi_config;

    result = rt_spi_bus_register(&spi_obj.spi_bus, hard_spi_config.bus_name, &uc8x88_spi_ops);
    RT_ASSERT(result == RT_EOK);

    LOG_D("%s bus init done", hard_spi_config.bus_name);

    return result;
}
INIT_BOARD_EXPORT(rt_hw_spi_init);

#endif

#endif /* RT_USING_SPI */
