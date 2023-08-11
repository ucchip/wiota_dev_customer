
#include <rtthread.h>
#include <rthw.h>
#include <board.h>
#include "uc_sectdefs.h"
#ifdef RT_USING_SERIAL

#include <rtdevice.h>

//#include <encoding.h>
//#include <platform.h>
//#include <interrupt.h>
#include <rthw.h>

//#include <platform.h>
//#include <encoding.h>
//#include <interrupt.h>
#include "drv_gpio.h"

#include <uc_utils.h>
#include <uc_event.h>
#include <uc_uart.h>
//#include <uc_uartx.h>
#include "uc_timer.h"
#include "uc_gpio.h"

//#define DRV_DEBUG
#define DBG_TAG              "drv.usart"
#ifdef DRV_DEBUG
#define DBG_LVL               DBG_LOG
#else
#define DBG_LVL               DBG_INFO
#endif /* DRV_DEBUG */
#include <rtdbg.h>

/* uc8088 config class */
struct uc8088_uart_config
{
    const char* name;
    UART_TYPE* Instance;
    rt_uint32_t irq_type;
};

/* uc8088 uart dirver class */
struct uc8088_uart
{
    UART_TYPE* handle;
    //UART_CFG_Type uc_uart_cfg;
    uint32_t baud_rate;
    struct uc8088_uart_config* config;

    struct rt_serial_device serial;
};

#if defined(BSP_USING_UART0)
#ifndef UART0_CONFIG
#define UART0_CONFIG                                                \
    {                                                               \
        .name = "uart0",                                            \
                .Instance = (UART_TYPE *)UART0_BASE_ADDR,                   \
                            .irq_type = (1 << 23),                                      \
    }
#endif /* UART1_CONFIG */
#endif

#if defined(BSP_USING_UART1)
#ifndef UART1_CONFIG
#define UART1_CONFIG                                                \
    {                                                               \
        .name = "uart1",                                            \
                .Instance = (UART_TYPE *)UART1_BASE_ADDR,                   \
                            .irq_type = (1 << 24),                                      \
    }
#endif /* UART1_CONFIG */
#endif

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

static struct uc8088_uart_config uart_config[UART_INDEX_MAX] =
{
#ifdef BSP_USING_UART0
    UART0_CONFIG,
#endif
#ifdef BSP_USING_UART1
    UART1_CONFIG,
#endif
};

static struct uc8088_uart uart_obj[sizeof(uart_config) / sizeof(uart_config[0])] = {0};

static rt_err_t usart_configure(struct rt_serial_device* serial, struct serial_configure* cfg)
{
    struct uc8088_uart* uart;
    RT_ASSERT(serial != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    uart = rt_container_of(serial, struct uc8088_uart, serial);

    if (rt_strcmp(uart->config->name, "uart0") == 0)
    {
        gpio_set_pin_mux((GPIO_CFG_TypeDef*)UC_GPIO_CFG, 14, GPIO_FUNC_1);
    }
    else if (rt_strcmp(uart->config->name, "uart1") == 0)
    {
        gpio_set_pin_mux((GPIO_CFG_TypeDef*)UC_GPIO_CFG, 12, GPIO_FUNC_2);
        gpio_set_pin_mux((GPIO_CFG_TypeDef*)UC_GPIO_CFG, 13, GPIO_FUNC_2);
    }

    uart->handle          = uart->config->Instance;
#if 0
    uart->uc_uart_cfg.Baud_rate     = cfg->baud_rate;
    uart->uc_uart_cfg.Reset     = RESET_LOW;
    uart->uc_uart_cfg.level     = BYTE_1;

    switch (cfg->data_bits)
    {
        case DATA_BITS_5:
            uart->uc_uart_cfg.Databits = DATABIT_5;
            break;
        case DATA_BITS_6:
            uart->uc_uart_cfg.Databits = DATABIT_6;
            break;
        case DATA_BITS_7:
            uart->uc_uart_cfg.Databits = DATABIT_7;
            break;
        case DATA_BITS_8:
            uart->uc_uart_cfg.Databits = DATABIT_8;
            break;
        default:
            uart->uc_uart_cfg.Databits = DATABIT_8;
            break;
    }
    switch (cfg->stop_bits)
    {
        case STOP_BITS_1:
            uart->uc_uart_cfg.Stopbits = STOPBIT_ENABLE;
            break;
        default:
            uart->uc_uart_cfg.Stopbits = STOPBIT_ENABLE;
            break;
    }
    switch (cfg->parity)
    {
        case PARITY_NONE:
            uart->uc_uart_cfg.Parity = PARITYBIT_DISABLE;
            break;
        case PARITY_ODD:
            uart->uc_uart_cfg.Parity = PARITYBIT_DISABLE;
            break;
        case PARITY_EVEN:
            uart->uc_uart_cfg.Parity = PARITYBIT_DISABLE;
            break;
        default:
            uart->uc_uart_cfg.Parity = PARITYBIT_DISABLE;
            break;
    }

    uartx_init(uart->handle, &(uart->uc_uart_cfg));
#endif
    uart->baud_rate = cfg->baud_rate;
    uc_uart_init(uart->handle, cfg->baud_rate, cfg->data_bits, cfg->stop_bits + 1, cfg->parity);

    return RT_EOK;
}

static rt_err_t usart_control(struct rt_serial_device* serial, int cmd, void* arg)
{
    struct uc8088_uart* uart;

    RT_ASSERT(serial != RT_NULL);
    uart = rt_container_of(serial, struct uc8088_uart, serial);

    switch (cmd)
    {
        /* disable interrupt */
        case RT_DEVICE_CTRL_CLR_INT:
            /* disable rx irq */
            uc_uart_enable_intrx(uart->handle, 0);
            /* disable interrupt */
            IER &= ~(uart->config->irq_type);
            break;

        /* enable interrupt */
        case RT_DEVICE_CTRL_SET_INT:
            /* enable rx irq */
            uc_uart_enable_intrx(uart->handle, 1);
            /* enable interrupt */
            IER |= uart->config->irq_type;
            break;
		
		case RT_DEVICE_CTRL_WAIT_TX_DONE:
        	uc_uartx_wait_tx_done(uart->handle);
			break;
    }

    return RT_EOK;
}

__crt0 int usart_putc(struct rt_serial_device* serial, char c)
{
    struct uc8088_uart* uart;
    RT_ASSERT(serial != RT_NULL);

    uart = rt_container_of(serial, struct uc8088_uart, serial);

    uc_uart_sendchar(uart->handle, c);

    return 1;
}

static int usart_getc(struct rt_serial_device* serial)
{
    rt_uint8_t val = -1;
    struct uc8088_uart* uart;
    char ret_val = -1;

    RT_ASSERT(serial != RT_NULL);
    uart = rt_container_of(serial, struct uc8088_uart, serial);

    ret_val = uc_uart_getchar(uart->handle, &val);
    if (ret_val == 0)
    {
        return (rt_uint8_t)val;
    }
    else
    {
        return -1;
    }
}

static struct rt_uart_ops uc8088_uart_ops =
{
    .configure = usart_configure,
    .control = usart_control,
    .putc = usart_putc,
    .getc = usart_getc,
};

void uart0_handler(void)
{
#ifdef BSP_USING_UART0
    if ((uart_obj[UART0_INDEX].serial.serial_rx != NULL)
        && uc_uart_get_intrxflag(uart_obj[UART0_INDEX].handle))
    {
        rt_hw_serial_isr((struct rt_serial_device*)(&uart_obj[UART0_INDEX].serial), RT_SERIAL_EVENT_RX_IND);
    }
#endif
}

void uart1_handler(void)
{
#ifdef BSP_USING_UART1
    if ((uart_obj[UART1_INDEX].serial.serial_rx != NULL)
        && uc_uart_get_intrxflag(uart_obj[UART1_INDEX].handle))
    {
        rt_hw_serial_isr((struct rt_serial_device*)(&uart_obj[UART1_INDEX].serial), RT_SERIAL_EVENT_RX_IND);
    }
#endif
}

int rt_hw_usart_init(void)
{
    rt_size_t obj_num = sizeof(uart_obj) / sizeof(struct uc8088_uart);
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    rt_err_t result = 0;

    for (int i = 0; i < obj_num; i++)
    {
        uart_obj[i].config = &uart_config[i];
        uart_obj[i].serial.ops    = &uc8088_uart_ops;
        uart_obj[i].serial.config = config;
        /* register UART device */
        result = rt_hw_serial_register(&uart_obj[i].serial,
                                       uart_obj[i].config->name,
                                       RT_DEVICE_FLAG_STREAM
                                       | RT_DEVICE_FLAG_RDWR
                                       | RT_DEVICE_FLAG_INT_RX
                                       , NULL);
        //RT_ASSERT(result == RT_EOK);
    }

    return result;
}

#if 0
static rt_err_t virtual_usart_configure(struct rt_serial_device* serial, struct serial_configure* cfg)
{
    return RT_EOK;
}

static rt_err_t virtual_usart_control(struct rt_serial_device* serial, int cmd, void* arg)
{
    return RT_EOK;
}

static int virtual_usart_putc(struct rt_serial_device* serial, char c)
{
    return 0;
}

static int virtual_usart_getc(struct rt_serial_device* serial)
{
    return -1;
}

static struct rt_uart_ops virtual_uart_ops =
{
    .configure = virtual_usart_configure,
    .control = virtual_usart_control,
    .putc = virtual_usart_putc,
    .getc = virtual_usart_getc,
};

static struct rt_serial_device virtual_usart_serial =
{
    .ops = &virtual_uart_ops,
    .config.baud_rate = BAUD_RATE_115200,
    .config.bit_order = BIT_ORDER_LSB,
    .config.data_bits = DATA_BITS_8,
    .config.parity    = PARITY_NONE,
    .config.stop_bits = STOP_BITS_1,
    .config.invert    = NRZ_NORMAL,
    .config.bufsz     = RT_SERIAL_RB_BUFSZ,
};

int virtual_usart_init(void)
{
    rt_hw_serial_register(&virtual_usart_serial,
                          "uartx",
                          RT_DEVICE_FLAG_STREAM
                          | RT_DEVICE_FLAG_RDWR
                          | RT_DEVICE_FLAG_INT_RX
                          , NULL);

    return 0;
}
#endif

#endif
