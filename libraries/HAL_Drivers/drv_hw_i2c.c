/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-8-30     wz   first version
 */

#include <board.h>
#include "drv_soft_i2c.h"
#include "uc_i2c.h"

#ifdef RT_USING_I2C
#ifndef RT_USING_I2C_BITOPS

//#define DRV_DEBUG
#define LOG_TAG              "drv.i2c"
#include <drv_log.h>

typedef struct
{
    I2C_CFG_Type parment;
    I2C_TYPE *base;
} uc_hw_i2c;

struct rt_i2c_bus_device i2c1_bus;
uc_hw_i2c hi2c1 = {
    .base = UC_I2C,
};

static rt_err_t i2c_hw_init(void)
{
    hi2c1.parment.Enable = 0;
    hi2c1.parment.prescaler = 0x63;

    i2c_setup(hi2c1.base, &hi2c1.parment);

    return RT_EOK;
}

static rt_size_t i2c_hw_write(struct rt_i2c_msg *msg)
{
    unsigned int j = 0;
    unsigned int  address = msg->addr & (~0x01); //0->write

    //write
    i2c_send_data(hi2c1.base, address);
    i2c_send_command(hi2c1.base, I2C_START_WRITE);
    i2c_get_ack(hi2c1.base);

    //write higth address
    i2c_send_data(hi2c1.base, msg->buf[0]);
    i2c_send_command(hi2c1.base, I2C_WRITE);
    i2c_get_ack(hi2c1.base);

    //write low address
    i2c_send_data(hi2c1.base, msg->buf[0]);
    i2c_send_command(hi2c1.base, I2C_WRITE);
    i2c_get_ack(hi2c1.base);

    for(j=1; j< msg->len; j++)
    {
        i2c_send_data(hi2c1.base, msg->buf[j]);
        i2c_send_command(hi2c1.base, I2C_WRITE);
        i2c_get_ack(hi2c1.base);
    }
    i2c_send_command(hi2c1.base, I2C_STOP_WRITE);

    while(i2c_busy(hi2c1.base));
    //wait write data over
    do{
        i2c_send_data(hi2c1.base, address);
        i2c_send_command(hi2c1.base, I2C_START_WRITE);
    }while(!i2c_get_ack(hi2c1.base));
    i2c_send_command(hi2c1.base, I2C_STOP);
    while(i2c_busy(hi2c1.base));

    return 0;
}

static rt_size_t i2c_hw_read(struct rt_i2c_msg *msg)
{
    unsigned int  address = msg->addr & (~0x01); //0->write
    unsigned int j = 0;
    i2c_send_data(hi2c1.base, address);
    i2c_send_command(hi2c1.base, I2C_START_WRITE);
    i2c_get_ack(hi2c1.base);

    //write higth address
    i2c_send_data(hi2c1.base, msg->buf[0]);
    i2c_send_command(hi2c1.base, I2C_WRITE);
    i2c_get_ack(hi2c1.base);

    //write low address
    i2c_send_data(hi2c1.base, msg->buf[0]);
    i2c_send_command(hi2c1.base, I2C_WRITE);
    i2c_get_ack(hi2c1.base);

    i2c_send_command(hi2c1.base, I2C_STOP);
    while(i2c_busy(hi2c1.base));

    address = msg->addr | 0x01; //1->read
    i2c_send_data(hi2c1.base, address);
    i2c_send_command(hi2c1.base, I2C_START_WRITE);
    i2c_get_ack(hi2c1.base);

    for(j= 0; j< msg->len; j++)
    {
        i2c_send_command(hi2c1.base, I2C_READ);
        i2c_get_ack(hi2c1.base);
        msg->buf[j] = i2c_get_data(hi2c1.base);
    }

    i2c_send_command(hi2c1.base, I2C_STOP_READ);
    return 0;
}


static rt_size_t i2c_xfer(struct rt_i2c_bus_device *bus, struct rt_i2c_msg msgs[], rt_uint32_t num)
{
    struct rt_i2c_msg *msg;
    rt_int32_t i, ret;

    if (num == 0)
    {
        return 0;
    }

    for(i = 0; i< num; i++)
    {
        msg = &msgs[i];
        if(msg->flags ==  RT_I2C_RD)
        {
            i2c_hw_read(msg);
        }
        else  if(msg->flags ==  RT_I2C_WR)
        {
            i2c_hw_write(msg);
        }
    }
    ret = i;
    return ret;
}

static const struct rt_i2c_bus_device_ops i2c_bus_ops =
{
    i2c_xfer,
    RT_NULL,
    RT_NULL
};

int rt_i2c_hw_init(void)
{
    i2c_hw_init();
    i2c1_bus.ops = &i2c_bus_ops;
    rt_i2c_bus_device_register(&i2c1_bus, "hw_i2c");
    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_i2c_hw_init);
#endif
#endif
