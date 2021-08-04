/*
 *elec_drv.c
 *
 * Copyright (c) 2017-2021, ucchip
 *
 * Change Logs:
 *
 * Date           Author       Notes
 *
 * 2021-04-20     lcj       the first version
 *
 * code_format:utf-8
 *
 */
#include <board.h>
#include "elec_drv.h"

#ifdef USING_ELEC_DEV

static rt_device_t elec_uart_handle = RT_NULL;



/**
 * @brief       复位模块
 * @param       无
 * @return      无
 */
void elec_reset(void)
{
    rt_pin_write(PLC_RESET_PIN, PIN_LOW);

    rt_thread_mdelay(500); //rest off must more than 250ms

    rt_pin_write(PLC_RESET_PIN, PIN_HIGH);
}

/**
 * @brief       触发此IO口，meter将请求主动上报
 * @param       无
 * @return      无
 */
void elec_eventup_on(void)
{
    rt_pin_write(PLC_EVENT_PIN, PIN_HIGH);

    rt_thread_mdelay(50);

    rt_pin_write(PLC_EVENT_PIN, PIN_LOW);
}

/**
 * @brief       初始化控制管脚
 * @param       无
 * @return      0:成功；其它：失败
 */
static void elec_gpio_init(void)
{
    rt_pin_mode(PLC_RESET_PIN, PIN_MODE_OUTPUT);

    rt_pin_mode(PLC_EVENT_PIN, PIN_MODE_OUTPUT);

    rt_pin_write(PLC_RESET_PIN, PIN_HIGH);

    rt_pin_write(PLC_EVENT_PIN, PIN_LOW);
}

/**
 * @brief       串口发送接口
 * @param       buff：待发送的数据
 *              len:待发送的数据长度
 * @return      0:发送数据为空或者数据长度小于1，其他：实际发送的数据长度
 */
rt_int32_t elec_uart_write(void* buff, rt_uint32_t len)
{
    if ((buff == NULL) || (len < 1))
    {
        return 0;
    }
    rt_device_write(elec_uart_handle, 0, buff, len);

    return len;
}

/**
 * @brief       串口接收接口
 * @param       buff：存放读取的数据
 *              len:待读取的数据长度
 * @return      0:发送数据为空或者数据长度小于1，其他：实际接收的数据长度
 */
rt_int32_t elec_uart_read(void* buff, rt_uint32_t len)
{
    if ((buff == NULL) || (len < 1))
    {
        return 0;
    }
    return rt_device_read(elec_uart_handle, 0, buff, len);
}

/**
 * @brief       初始化配置
 * @param       Baud：串口波特率
 * @param       parity：奇偶校验
 * @return      true:成功；false:失败
 */
rt_int32_t elec_dev_init(void)
{
    struct serial_configure port_arg = RT_SERIAL_CONFIG_DEFAULT;
#if UART_USING_MSG_QUEUE
    static char msg_pool[ELEC_DEV_TRANS_MAX_LEN];
#endif
    elec_gpio_init();

    elec_reset();

    if (elec_uart_handle != RT_NULL)
    {
        elec_uart_handle->open_flag &= ~RT_DEVICE_FLAG_DMA_RX;

        rt_device_close(elec_uart_handle);

        elec_uart_handle = RT_NULL;
    }

    elec_uart_handle = rt_device_find(ELEC_DEV_COM_NAME);

    if (elec_uart_handle == RT_NULL)
    {
        rt_kprintf("elec_uart_handle == NULL\r\n");

        return -RT_ERROR;
    }

    port_arg.baud_rate = 2400; //default value

#ifdef ELEC_DEV_BAUD_2400
    port_arg.baud_rate = 2400;
#endif
#ifdef ELEC_DEV_BAUD_4800
    port_arg.baud_rate = 4800;
#endif
#ifdef ELEC_DEV_BAUD_9600
    port_arg.baud_rate = 9600;
#endif
#ifdef ELEC_DEV_BAUD_115200
    port_arg.baud_rate = 115200;
#endif

    port_arg.parity = PARITY_NONE; //default value

#ifdef ELEC_DEV_PARITY_EVEN
    port_arg.parity = PARITY_EVEN;
#endif
#ifdef ELEC_DEV_PARITY_ODD
    port_arg.parity = PARITY_ODD;
#endif
#ifdef ELEC_DEV_PARITY_NONE
    port_arg.parity = PARITY_NONE;
#endif

    if (rt_device_control(elec_uart_handle, RT_DEVICE_CTRL_CONFIG, &port_arg) != RT_EOK)
    {
        rt_kprintf("%s config fail\r\n", ELEC_DEV_COM_NAME);
    }

    //    rt_err_t err_state = rt_device_open(elec_uart_handle, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_DMA_RX);
    rt_err_t err_state = rt_device_open(elec_uart_handle, RT_DEVICE_FLAG_RDWR);

    if (err_state != RT_EOK)
    {
        rt_kprintf("%s open fail rt_err_t = %d\r\n", ELEC_DEV_COM_NAME, err_state);
        return -RT_ERROR;
    }

    return RT_EOK;
}

INIT_COMPONENT_EXPORT(elec_dev_init);

#endif
