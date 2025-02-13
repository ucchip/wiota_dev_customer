#include <drv_usart.h>

#ifdef RT_USING_SERIAL

#if !defined(BSP_USING_UART0) && !defined(BSP_USING_UART1)
#error "Please define at least one BSP_USING_UARTx"
/* this driver can be disabled at menuconfig -> RT-Thread Components -> Device Drivers */
#endif

struct uc8x88_usart
{
    const char *name;
    UART_TYPE *usart_base;
    rt_uint32_t irq_type;
    struct rt_serial_device serial;
};

enum
{
#ifdef BSP_USING_UART0
    UART0_INDEX,
#endif
#ifdef BSP_USING_UART1
    UART1_INDEX,
#endif
    UART_INDEX_MAX,
};

#ifdef BSP_USING_UART0
#ifndef UART0_CONFIG
#define UART0_CONFIG {                          \
    .name = "uart0",                            \
    .usart_base = (UART_TYPE *)UART0_BASE_ADDR, \
    .irq_type = (1 << 23),                      \
}
#endif /* UART1_CONFIG */
#endif

#ifdef BSP_USING_UART1
#ifndef UART1_CONFIG
#define UART1_CONFIG {                          \
    .name = "uart1",                            \
    .usart_base = (UART_TYPE *)UART1_BASE_ADDR, \
    .irq_type = (1 << 24),                      \
}
#endif /* UART1_CONFIG */
#endif

static struct uc8x88_usart usart_config[UART_INDEX_MAX] = {
#ifdef BSP_USING_UART0
    UART0_CONFIG,
#endif
#ifdef BSP_USING_UART1
    UART1_CONFIG,
#endif
};

static rt_err_t uc8x88_usart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    struct uc8x88_usart *usart;
    RT_ASSERT(serial != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    usart = (struct uc8x88_usart *)serial->parent.user_data;
    RT_ASSERT(usart != RT_NULL);

    if (rt_strcmp(usart->name, "uart0") == 0)
    {
        gpio_set_pin_mux((GPIO_CFG_TypeDef *)UC_GPIO_CFG, GPIO_PIN_14, GPIO_FUNC_1);
    }
    else if (rt_strcmp(usart->name, "uart1") == 0)
    {
        gpio_set_pin_mux((GPIO_CFG_TypeDef *)UC_GPIO_CFG, GPIO_PIN_12, GPIO_FUNC_2);
        gpio_set_pin_mux((GPIO_CFG_TypeDef *)UC_GPIO_CFG, GPIO_PIN_13, GPIO_FUNC_2);
    }

    uc_uart_init(usart->usart_base, cfg->baud_rate, cfg->data_bits, cfg->stop_bits + 1, cfg->parity);

    return RT_EOK;
}

static rt_err_t uc8x88_usart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    struct uc8x88_usart *usart;
    RT_ASSERT(serial != RT_NULL);

    usart = (struct uc8x88_usart *)serial->parent.user_data;
    RT_ASSERT(usart != RT_NULL);

    switch (cmd)
    {
    /* disable interrupt */
    case RT_DEVICE_CTRL_CLR_INT:
        /* disable rx irq */
        uc_uart_enable_intrx(usart->usart_base, 0);
        /* disable interrupt */
        IER &= ~(usart->irq_type);
        break;

    /* enable interrupt */
    case RT_DEVICE_CTRL_SET_INT:
        /* enable rx irq */
        uc_uart_enable_intrx(usart->usart_base, 1);
        /* enable interrupt */
        IER |= usart->irq_type;
        break;

    case RT_DEVICE_CTRL_WAIT_TX_DONE:
        /* wait tx done */
        uc_uartx_wait_tx_done(usart->usart_base);
        break;

    default:
        return -RT_ENOSYS;
        break;
    }

    return RT_EOK;
}

static int uc8x88_usart_putc(struct rt_serial_device *serial, char ch)
{
    struct uc8x88_usart *usart;

    RT_ASSERT(serial != RT_NULL);
    usart = (struct uc8x88_usart *)serial->parent.user_data;
    RT_ASSERT(usart != RT_NULL);

    uc_uart_sendchar(usart->usart_base, ch);
    // uc_uartx_wait_tx_done(usart->usart_base);

    return 1;
}

static int uc8x88_usart_getc(struct rt_serial_device *serial)
{
    int ret;
    rt_uint8_t ch = 0;

    struct uc8x88_usart *usart;

    RT_ASSERT(serial != RT_NULL);
    usart = (struct uc8x88_usart *)serial->parent.user_data;
    RT_ASSERT(usart != RT_NULL);

    ret = uc_uart_getchar(usart->usart_base, &ch);
    if (ret == 0)
    {
        return ch;
    }
    else
    {
        return -1;
    }
}

static const struct rt_uart_ops uc8x88_usart_ops = {
    .configure = uc8x88_usart_configure,
    .control = uc8x88_usart_control,
    .putc = uc8x88_usart_putc,
    .getc = uc8x88_usart_getc,
};

static void usart_isr(struct rt_serial_device *serial)
{
    struct uc8x88_usart *usart;

    RT_ASSERT(serial != RT_NULL);

    usart = (struct uc8x88_usart *)serial->parent.user_data;
    RT_ASSERT(usart != RT_NULL);

    if (uc_uart_get_intrxflag(usart->usart_base))
    {
        rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_IND);
    }
}

void uart0_handler(void)
{
#ifdef BSP_USING_UART0
    rt_interrupt_enter();
    usart_isr(&usart_config[UART0_INDEX].serial);
    rt_interrupt_leave();
#endif
}

void uart1_handler(void)
{
#ifdef BSP_USING_UART1
    rt_interrupt_enter();
    usart_isr(&usart_config[UART1_INDEX].serial);
    rt_interrupt_leave();
#endif
}

int rt_hw_usart_init(void)
{
    rt_size_t obj_num;
    int index;

    obj_num = sizeof(usart_config) / sizeof(struct uc8x88_usart);
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    rt_err_t result = 0;

    for (index = 0; index < obj_num; index++)
    {
        usart_config[index].serial.ops = &uc8x88_usart_ops;
        usart_config[index].serial.config = config;

        /* register UART device */
        result = rt_hw_serial_register(&usart_config[index].serial,
                                       usart_config[index].name,
                                       RT_DEVICE_FLAG_STREAM | RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX,
                                       &usart_config[index]);
        RT_ASSERT(result == RT_EOK);
    }

    return result;
}

#endif /* RT_USING_SERIAL */

/******************** end of file *******************/
