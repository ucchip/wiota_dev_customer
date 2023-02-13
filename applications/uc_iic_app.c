#include <rtthread.h>
#ifdef _IIC_APP_
#include <rtdevice.h>
#include "uc_iic_app.h"

#define IIC_DEVICE_NAME    "hw_i2c"
#define AT24C02_ADDR 0xA0

static rt_device_t iic_dev = NULL;

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

int iic_app_init(void)
{
    rt_err_t ret = RT_EOK;

    iic_dev = rt_device_find(IIC_DEVICE_NAME);
    if (!iic_dev)
    {
        rt_kprintf("find %s failed!\n", IIC_DEVICE_NAME);
        return RT_ERROR;
    }

    return ret;
}

void iic_app_sample(void)
{
    rt_err_t ret = RT_EOK;
    unsigned char set_data[4] = {1, 2, 3, 4};
    unsigned char get_data[4] = {0};
    
    rt_kprintf("iic test demo.\r\n");
    
    ret = iic_app_init();
    if(ret != RT_EOK)
    {
        rt_kprintf("init iic failed!\n");
        return;
    }
    
    ret = write_reg((struct rt_i2c_bus_device *)iic_dev, 0, set_data);
    if(ret != RT_EOK)
    {
        rt_kprintf("iic write data failed!\n");
        return;
    }
    
    ret = read_regs((struct rt_i2c_bus_device *)dev, 4, get_data);
    if(ret != RT_EOK)
    {
        rt_kprintf("iic write data failed!\n");
        return;
    }
    
    for (rt_uint8_t num = 0; num < 4; num++)
    {
        if (set_data[num] != get_data[num])
        {
            rt_kprintf("i2c data match fail. num=%d, %d!= %d\n", num, set_data[num], get_data[num]);
        }
    }
}

#endif
