#include <rtthread.h>
#ifdef _CAN_APP_
#include <rtdevice.h>
#include "uc_can_app.h"
#include "drv_can.h"

#define CAN_DEVICE_NAME    "can1"           // can device name
#define CAN_SEND_DATA                       // enable can send data,if use can recevie data,do not define CAN_SEND_DATA


static rt_device_t can_dev = NULL;          // can device handle 

int can_app_init(void)
{
    rt_err_t ret = RT_EOK;
    
    // find can device
    can_dev = rt_device_find(CAN_DEVICE_NAME);
    if (!can_dev)
    {
        rt_kprintf("find %s failed!\n", CAN_DEVICE_NAME);
        return RT_ERROR;
    }
    
    // open can device
    ret = rt_device_open(can_dev, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    if (ret != RT_EOK)
    {
        rt_kprintf("open %s failed!\n", CAN_DEVICE_NAME);
        return RT_ERROR;
    }
    
    return ret;
}


void can_app_sample(void)
{
    int ret = 0;
    struct rt_can_msg msg;
    rt_uint8_t data[8] = {0};
    
    rt_kprintf("can test demo.\r\n");
    
    ret = can_app_init();
    if(ret != RT_EOK)
    {
        rt_kprintf("init can failed!\n");
        return;
    }
    
    if (can_dev == RT_NULL)
    {
        rt_kprintf("find %s failed!\n", CAN_DEVICE_NAME);
        return;
    }
    else
    {
#ifdef CAN_SEND_DATA
        while(1)
        {
            for(rt_uint8_t i=0; i<8; i++)
            {
                // fix frame id
                msg.id = i;
                msg.ide = RT_CAN_STDID;
                msg.rtr = RT_CAN_DTR;
                msg.len = 8;
                
                // fix data
                msg.data[0] = i;
                msg.data[1] = i;
                msg.data[2] = i;
                msg.data[3] = i;
                msg.data[4] = i;
                msg.data[5] = i;
                msg.data[6] = i;
                msg.data[7] = i;
                
                // can send data
                ret = rt_device_write(can_dev, 0, &msg, sizeof(msg.data));
                if(ret != 0)
                {
                    rt_kprintf("can send data success!\n");
                }
                else
                {
                    rt_kprintf("can send data failed!\n");
                }
                
                rt_thread_delay(1000);
            }
        }
#else
        while(1)
        {
            // read can data
            ret = rt_device_read(can_dev, 0, data, 8);
            if(ret != 0)
            {
                rt_kprintf("can recv data = ");
                for(rt_uint8_t i=0; i<8; i++)
                {
                    rt_kprintf("%2x ", data[i]);
                }
                rt_kprintf("\r\n");
            }
            else
            {
                rt_kprintf("...\r\n");
            }
            rt_thread_delay(1000);
        }
#endif
    }
}

#endif
