#include <rtthread.h>
#ifdef RT_USING_AT
#ifdef UC8288_MODULE
#ifdef UC8288_FACTORY
#ifdef _RT_THREAD_
#include <rtdevice.h>
#endif
#include <board.h>
#include "uc_adda.h"
#include "string.h"
#include "ati_prs.h"
#include "at.h"

#ifdef _L1_FACTORY_FUNC_
#include "uc_wiota_api.h"
#endif

enum factory_can_write_read_type
{
    FACTORY_CAN_WRITE = 0,
    FACTORY_CAN_READ,
};
enum factory_command_type
{
    FACTORY_WIOTA = 0,
    FACTORY_GPIO = 1, // 1
    FACTORY_AD = 3,   // 3
    FACTORY_DA = 4,   // 4
#ifdef UC8288_DRV_TEST
    FACTORY_I2C = 2, // 2
    FACTORY_UART1 = 5,   //5
    FACTORY_PWM = 6,     // 6
    FACTORY_CAN = 7,
#endif
};

#define DAC_DEV_NAME "dac"
#define ADC_DEV_NAME "adc"

static int at_test_ad(unsigned int channel)
{
    rt_adc_device_t adc_dev;
    rt_uint32_t value;

    adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    if (RT_NULL == adc_dev)
    {
        rt_kprintf("ad find %s  fail\n", ADC_DEV_NAME);
        return -1;
    }

    rt_adc_enable(adc_dev, channel);

    value = rt_adc_read(adc_dev, channel);

    rt_adc_disable(adc_dev, channel);

    return value;
}

static int at_test_da(unsigned int channel, unsigned int value)
{
    rt_dac_device_t dac_dev;

    dac_dev = (rt_dac_device_t)rt_device_find(DAC_DEV_NAME);
    if (RT_NULL == dac_dev)
    {
        rt_kprintf("da find fail\n");
        return -1;
    }

    rt_dac_enable(dac_dev, channel);

    rt_dac_write(dac_dev, channel, value);

    //rt_dac_disable(dac_dev, channel);

    return 0;
}

#ifdef UC8288_DRV_TEST

#define AHT10_I2C_BUS_NAME "hw_i2c"
#define UART1_DEV_NMAE "uart1"
#define PWM_DEV_NAME "pwm0"
#define CAN_DEV_NAME "can1"
#define AT24C02_ADDR 0xA0

static rt_err_t write_reg(struct rt_i2c_bus_device *bus, rt_uint8_t reg, rt_uint8_t *data)
{
    rt_uint8_t buf[8];
    struct rt_i2c_msg msgs;
    rt_uint32_t buf_size = 1;

    buf[0] = reg; //cmd
    if (data != RT_NULL)
    {
        buf[1] = data[0];
        buf[2] = data[1];
        buf[3] = data[2];
        buf[4] = data[3];
        buf_size = 5;
    }

    msgs.addr = AT24C02_ADDR;
    msgs.flags = RT_I2C_WR;
    msgs.buf = buf;
    msgs.len = buf_size;

    if (rt_i2c_transfer(bus, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

static rt_err_t read_regs(struct rt_i2c_bus_device *bus, rt_uint8_t len, rt_uint8_t *buf)
{
    struct rt_i2c_msg msgs;

    msgs.addr = AT24C02_ADDR;
    msgs.flags = RT_I2C_RD;
    msgs.buf = buf;
    msgs.len = len;

    if (rt_i2c_transfer(bus, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

static int at_test_i2c(void)
{
    rt_device_t dev;
    unsigned char set_data[4] = {1, 2, 3, 4};
    unsigned char get_data[4] = {0};
    int num = 0;

    dev = rt_device_find(AHT10_I2C_BUS_NAME);
    if (RT_NULL == dev)
    {
        rt_kprintf("rt_device_find i2c fail\n");
        return 1;
    }

    if (RT_EOK != write_reg((struct rt_i2c_bus_device *)dev, 0, set_data))
    {
        rt_kprintf("write_reg i2c fail\n");
        return 2;
    }

    if (RT_EOK != read_regs((struct rt_i2c_bus_device *)dev, 4, get_data))
    {
        rt_kprintf("read_regs i2c fail\n");
        return 3;
    }

    for (num = 0; num < 4; num++)
    {
        if (set_data[num] != get_data[num])
        {
            rt_kprintf("i2c data match fail. num=%d, %d!= %d\n", num, set_data[num], get_data[num]);
            return 4;
        }
    }

    return 0;
}

static int at_factory_test_uart1(void)
{
    static rt_device_t serial;
    unsigned char send_data[4] = {"1234"};
    unsigned char recv_data[4] = {0};

    serial = rt_device_find(UART1_DEV_NMAE);
    if (!serial)
        return 1;

    if (RT_EOK != rt_device_open(serial, RT_DEVICE_OFLAG_RDWR))
    {
        rt_kprintf("uart open fail\n");
        return 2;
    }

    rt_device_write(serial, 0, send_data, sizeof(send_data) / sizeof(unsigned char));

    if (rt_device_read(serial, 0, recv_data, sizeof(recv_data) / sizeof(unsigned char)) < 1)
    {
        rt_kprintf("uart read fail\n");
        return 3;
    }

    rt_device_close(serial);

    return strcmp((const char *)send_data, (const char *)recv_data);
}

static int at_factory_test_pwm(int channel, unsigned int period)
{
    struct rt_device_pwm *pwm_dev;

    pwm_dev = (struct rt_device_pwm *)rt_device_find(PWM_DEV_NAME);
    if (RT_NULL == pwm_dev)
    {
        return 1;
    }

    rt_pwm_set(pwm_dev, channel, period, 0);
    rt_pwm_enable(pwm_dev, channel);
    rt_pwm_disable(pwm_dev, channel);

    return 0;
}
static int at_factory_test_can(int type, void *data)
{
    static rt_device_t can_dev;
    struct rt_can_msg msg = {0};
    //struct rt_can_msg rxmsg = {0};
    rt_err_t res;
    rt_size_t size;

    can_dev = rt_device_find(CAN_DEV_NAME);
    if (RT_NULL == can_dev)
    {
        rt_kprintf("find %s failed!\n", CAN_DEV_NAME);
        return 1;
    }
    res = rt_device_open(can_dev, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    if (res != RT_EOK)
    {
        rt_kprintf("open %s failed!\n", CAN_DEV_NAME);
        return 2;
    }

    if (type == FACTORY_CAN_WRITE)
    {
        msg.id = 0x78;
        msg.ide = RT_CAN_STDID;
        msg.rtr = RT_CAN_DTR;
        msg.len = 8;

        msg.data[0] = 0x00;
        msg.data[1] = 0x11;
        msg.data[2] = 0x22;
        msg.data[3] = 0x33;
        msg.data[4] = 0x44;
        msg.data[5] = 0x55;
        msg.data[6] = 0x66;
        msg.data[7] = 0x77;

        size = rt_device_write(can_dev, 0, &msg, sizeof(msg.data));
        if (size == 0)
        {
            return 2;
        }
    }
    else if (type == FACTORY_CAN_READ)
    {
        int len = 0;
        //rxmsg.hdr = -1;
        rt_device_read(can_dev, 0, data, 8);
        rt_kprintf("recv data:");
        for (len = 0; len < 8; len++)
        {
            rt_kprintf("%x", *((int *)(data + len)));
        }
        rt_kprintf("\n");
    }

    return 0;
}
#endif

static at_result_t at_factory_setup(const char *args)
{
    int type = 0, data = 0, data1 = 0;

    args = parse((char *)(++args), "ddd", &type, &data, &data1);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    rt_kprintf("type = %d,data=%d,data1 = %d\n", type, data, data1);
    switch (type)
    {
    case FACTORY_WIOTA:
    {
#ifdef _L1_FACTORY_FUNC_
        if (!factory_msg_handler(data, data1))
        {
            return AT_RESULT_FAILE;
        }
#endif
        break;
    }
    case FACTORY_GPIO:
    {
        rt_base_t pin = data;
        rt_base_t value = data1 & 0x1;

        rt_pin_mode(pin, PIN_MODE_OUTPUT);
        rt_pin_write(pin, value);
        break;
    }
    case FACTORY_AD:
    {
        unsigned int ch = data;
        int result = at_test_ad(ch);
        if (result < 0)
            return AT_RESULT_NULL;

        switch (ch)
        {
        case ADC_CONFIG_CHANNEL_TEMP_B:
        {
            float val = 0.00;
            val = (float)((float)1.42 / 4.0 + (result - 2048) * (float)1.42 / 2048.0 / 8.0);
            at_server_printfln("+FACTORY=%d,0.%d", type, val * 100.0);
            break;
        }
        default:
        {
            at_server_printfln("+FACTORY=%d,%d", type, result);
            break;
        }
        }

        break;
    }
    case FACTORY_DA:
    {
        unsigned int ch = data;
        unsigned int val = data1;
        if (at_test_da(ch, val) < 0)
            return AT_RESULT_NULL;
        break;
    }

#ifdef UC8288_DRV_TEST
    case FACTORY_I2C:
    {
        if (at_test_i2c())
            return AT_RESULT_FAILE;
        break;
    }

    case FACTORY_UART1:
    {
        if (at_factory_test_uart1())
            return AT_RESULT_NULL;
        break;
    }
    case FACTORY_PWM:
    {
        int channel = data;
        unsigned int period = data1;

        if (at_factory_test_pwm(channel, period))
            return AT_RESULT_NULL;
        break;
    }
    case FACTORY_CAN:
    {
        char recv[8] = {0};
        if (at_factory_test_can(data, recv))
            return AT_RESULT_NULL;

        if (data == FACTORY_CAN_READ)
        {
            at_server_printf("+FACTORY=%d,", type);
            at_send_data(recv, sizeof(recv) / sizeof(recv[0]));
        }

        break;
    }
#endif
    default:
        return AT_RESULT_REPETITIVE_FAILE;
    }

    return AT_RESULT_OK;
}

AT_CMD_EXPORT("AT+FACTORY", "=<type>,<data>,<data1>", RT_NULL, RT_NULL, at_factory_setup, RT_NULL);

#endif
#endif
#endif
