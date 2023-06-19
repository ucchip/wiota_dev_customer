
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#if defined(RT_USING_SERIAL) && defined(_RS485_APP_)

//#define USE_UART_RINGBUFF

#define RS485_NAME "uart0"
#define RS485_PIN 17
#define RS485_BAUD_RATE 9600
#define RS485_BUFFER_SIZE 4096
#define RS485_PRIORITY 7
#define RS485_TIMESLICE 5
#define RS485_STACK_SIZE 640

#define RS485_TX()    rt_pin_write(RS485_PIN, PIN_HIGH)
#define RS485_RX()    rt_pin_write(RS485_PIN, PIN_LOW)

/* 用于接收消息的信号量 */
static struct rt_semaphore rx_sem;
static rt_device_t serial;

#ifdef USE_UART_RINGBUFF
static struct rt_ringbuffer* uart_ringbuffer = RT_NULL;
#endif

/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
#ifdef USE_UART_RINGBUFF
    uint8_t uart_recv_buf[64];
    uint32_t recv_count = 0;

    if (size == 0)
    {
        return 0;
    }

    while (recv_count < size)
    {
        uint16_t data_len = 0;
        if ((recv_count + 64) < size)
        {
            data_len = 64;
        }
        else
        {
            data_len = size - recv_count;
        }
        data_len = rt_device_read(dev, 0, uart_recv_buf, data_len);
        if (data_len > 0)
        {
            if (uart_ringbuffer != RT_NULL)
            {
                rt_ringbuffer_put(uart_ringbuffer, (const rt_uint8_t*)uart_recv_buf, data_len);
            }
            recv_count += data_len;
        }
        else
        {
            break;
        }
    }
#endif
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    rt_sem_release(&rx_sem);

    return RT_EOK;
}

static void rs485_thread_entry(void *parameter)
{
    char rx_buf[64];

    while (1)
    {
        uint8_t data_len = 0;

        /* 阻塞等待接收信号量，等到信号量后再次读取数据 */
        rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
        rt_memset(rx_buf, 0x00, 64);
#ifdef USE_UART_RINGBUFF
        data_len = rt_ringbuffer_get(uart_ringbuffer, (uint8_t*)rx_buf, 64);
#else
        data_len = rt_device_read(serial, 0, rx_buf, 64);
#endif
        if (data_len > 0)
        {
            // rt_kprintf("[RS485]%s\n", rx_buf);
            RS485_TX();
            rt_device_write(serial, 0, rx_buf, data_len);
            RS485_RX();
        }
    }
}

int rs485_app_sample(void)
{
    rt_err_t ret = RT_EOK;
    char uart_name[RT_NAME_MAX];
    char *str = "hello RT-Thread!\r\n";
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;  /* 初始化配置参数 */

    // rt_kprintf("rs485_test start!!\r\n");
    rt_strncpy(uart_name, RS485_NAME, RT_NAME_MAX);

    rt_pin_mode(RS485_PIN, PIN_MODE_OUTPUT);

    /* 查找系统中的串口设备 */
    serial = rt_device_find(uart_name);
    if (!serial)
    {
        rt_kprintf("find %s failed!\n", uart_name);
        return RT_ERROR;
    }

    /* step2：修改串口配置参数 */
    config.baud_rate = RS485_BAUD_RATE;     // 修改波特率为 9600
    // config.data_bits = DATA_BITS_8;         // 数据位 8
    // config.stop_bits = STOP_BITS_1;         // 停止位 1
    // config.bufsz     = 128;                 // 修改缓冲区 buff size 为 128
    // config.parity    = PARITY_NONE;         // 无奇偶校验位

    /* step3：控制串口设备。通过控制接口传入命令控制字，与控制参数 */
    rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config);

#ifdef USE_UART_RINGBUFF
    uart_ringbuffer = rt_ringbuffer_create(2048);
    if (uart_ringbuffer == RT_NULL)
    {
        rt_kprintf("rt_ringbuffer_create fail!!\r\n");
        return RT_ERROR;
    }
#endif

    /* 初始化信号量 */
    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
    /* 以中断接收及轮询发送模式打开串口设备 */
    rt_device_open(serial, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(serial, uart_input);

    RS485_TX();
    /* 发送字符串 */
    rt_device_write(serial, 0, str, rt_strlen(str));
    RS485_RX();

    /* 创建 serial 线程 */
    rt_thread_t thread = rt_thread_create("serial", 
                                          rs485_thread_entry, 
                                          RT_NULL, 
                                          RS485_STACK_SIZE, 
                                          RS485_PRIORITY, 
                                          RS485_TIMESLICE);
    /* 创建成功则启动线程 */
    if (RT_NULL == thread)
    {
        ret = RT_ERROR;
    }
    else
    {
        rt_thread_startup(thread);
    }

    return ret;
}

#endif

