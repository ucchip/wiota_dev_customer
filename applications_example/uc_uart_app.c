#include <rtthread.h>

#ifdef APP_EXAMPLE_UART

#ifndef RT_USING_SERIAL
#error "Please enable rt-thread serial device driver"
#endif

#ifndef BSP_USING_UART
#error "Please enable on-chip peripheral uart config"
#endif

#ifndef BSP_USING_UART1
#error "Please enable on-chip peripheral uart1 config"
#endif

#include <rtdevice.h>
#include "uc_uart_app.h"

#define THREAD_STACK_SIZE 512
#define THREAD_PRIORITY 5
#define THREAD_TIMESLICE 5

#define UART_NAME "uart1"
#define UART_BAUD_RATE 9600

// #define USE_RS485
#ifdef USE_RS485
 // 485接收超时时间，接收到最后1字节后，等待10ms，若无数据，则认为接收完成
#define RS485_RX_TIMEOUT 10
#define RS485_CTRL_PIN 17
#define RS485_TX() rt_pin_write(RS485_CTRL_PIN, PIN_HIGH)
#define RS485_RX() rt_pin_write(RS485_CTRL_PIN, PIN_LOW)
#endif

// #define USE_UART_RINGBUFF
#ifdef USE_UART_RINGBUFF
#define RINGBUFF_SIZE 512
static struct rt_ringbuffer *uart_ringbuffer = RT_NULL;
#endif

/* 用于接收消息的信号量 */
static struct rt_semaphore rx_sem;
static rt_device_t serial;

static rt_size_t uart_output(rt_device_t dev, const void *buffer, rt_size_t size)
{
    rt_size_t ret = 0;

#ifdef USE_RS485
    RS485_TX();
#endif

    ret = rt_device_write(dev, 0, buffer, size);

#ifdef USE_RS485
    rt_device_control(dev, RT_DEVICE_CTRL_WAIT_TX_DONE, RT_NULL);
    RS485_RX();
#endif

    return ret;
}

/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
#ifdef USE_UART_RINGBUFF

    uint8_t data_len = 0;
    uint8_t uart_recv_buf[64];
    /* 接收到数据后存入 ringbuffer */
    data_len = rt_device_read(dev, 0, uart_recv_buf, size);
    if (data_len > 0)
    {
        rt_ringbuffer_put(uart_ringbuffer, (const rt_uint8_t *)uart_recv_buf, data_len);
    }

#endif
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    rt_sem_release(&rx_sem);

    return RT_EOK;
}

static void uart_thread_entry(void *parameter)
{
    rt_uint8_t rx_buf[64];
    rt_size_t rx_size = 0;
    rt_int32_t rx_timeout = RT_WAITING_FOREVER;
    rt_err_t ret = RT_EOK;

    rt_memset(rx_buf, 0x00, 64);
    while (1)
    {
        /* 阻塞等待接收信号量，等到信号量后再次读取数据 */
        ret = rt_sem_take(&rx_sem, rx_timeout);
#ifdef USE_UART_RINGBUFF
        rx_size += rt_ringbuffer_get(uart_ringbuffer, &rx_buf[rx_size], 64);
#else
        rx_size += rt_device_read(serial, 0, &rx_buf[rx_size], 64);
#endif
        if (rx_size > 64)
        {
            rt_kprintf("uart rx size over 64 bytes!\n");
            rt_memset(rx_buf, 0x00, 64);
            rx_size = 0;
            continue;
        }
        else if (rx_size > 0)
        {
#ifdef USE_RS485
            if (ret == -RT_ETIMEOUT)
            {
                uart_output(serial, rx_buf, rx_size);
                rt_memset(rx_buf, 0x00, 64);
                rx_size = 0;
                rx_timeout = RT_WAITING_FOREVER;
            }
            else
            {
                rx_timeout = RS485_RX_TIMEOUT;
            }
#else
            if (ret == RT_EOK)
            {
                uart_output(serial, rx_buf, rx_size);
                rt_memset(rx_buf, 0x00, 64);
                rx_size = 0;
            }
#endif
        }
    }
}

int uart_app_sample(void)
{
    rt_thread_t thread = RT_NULL;
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT; /* 初始化配置参数 */
    char str[] = "uart app sample\n";

#ifdef USE_RS485
    rt_pin_mode(RS485_CTRL_PIN, PIN_MODE_OUTPUT);
    RS485_RX();
#endif

    /* step1：查找系统中的串口设备 */
    serial = rt_device_find(UART_NAME);
    if (serial == RT_NULL)
    {
        rt_kprintf("find %s failed!\n", UART_NAME);
        return RT_ERROR;
    }

    /* step2：修改串口配置参数 */
    config.baud_rate = UART_BAUD_RATE; // 修改波特率 9600
    // config.data_bits = DATA_BITS_8;    // 数据位 8
    // config.stop_bits = STOP_BITS_1;    // 停止位 1
    // config.bufsz = 128;                // 修改缓冲区 buff size 为 128
    // config.parity = PARITY_NONE;       // 无奇偶校验位

    /* step3：控制串口设备。通过控制接口传入命令控制字，与控制参数 */
    rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config);

#ifdef USE_UART_RINGBUFF
    uart_ringbuffer = rt_ringbuffer_create(RINGBUFF_SIZE);
    if (uart_ringbuffer == RT_NULL)
    {
        rt_kprintf("rt_ringbuffer_create fail!!\n");
        return RT_ERROR;
    }
#endif

    /* 初始化信号量 */
    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
    /* 以中断接收及轮询发送模式打开串口设备 */
    rt_device_open(serial, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(serial, uart_input);

    /* 发送字符串 */
    uart_output(serial, str, rt_strlen(str));

    /* 创建 serial 线程 */
    thread = rt_thread_create("serial",
                              uart_thread_entry,
                              RT_NULL,
                              THREAD_STACK_SIZE,
                              THREAD_PRIORITY,
                              THREAD_TIMESLICE);
    /* 创建成功则启动线程 */
    if (RT_NULL == thread)
    {
        return -RT_ERROR;
    }
    else
    {
        rt_thread_startup(thread);
    }

    return RT_EOK;
}

#endif
