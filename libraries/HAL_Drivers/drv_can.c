/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author            Notes
 * 2015-05-14     aubrcool@qq.com   first version
 * 2015-07-06     Bernard           code cleanup and remove RT_CAN_USING_LED;
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "uc_string_lib.h"
#include "uc_can.h"

#ifdef RT_USING_CAN

static struct rt_can_device    can;

static rt_err_t rt_can_configure(struct rt_can_device* can, struct can_configure* cfg)
{
	CAN_InitTypeDef CAN_Init_Structure;
	CAN_FilterInitTypeDef CAN_Filter_Structure;

	CAN_Setup();

	CAN_Init_Structure.CAN_AWUM = DISABLE;
	CAN_Init_Structure.CAN_NART = DISABLE;
	CAN_Init_Structure.CAN_RFLM = DISABLE;
	CAN_Init_Structure.CAN_Mode = CAN_Mode_Normal;
	
	CAN_Init_Structure.CAN_SJW	= CAN_SJW_3tq;
	CAN_Init_Structure.CAN_BS1	= CAN_TS1_12tq;
	CAN_Init_Structure.CAN_BS2	= CAN_TS2_8tq;
	CAN_Init_Structure.CAN_Prescaler = 50;
    
	CAN_Init(UC_CAN_CRTL,&CAN_Init_Structure);	
	
	CAN_Filter_Structure.CAN_FilterNumber = 0;
	CAN_Filter_Structure.CAN_FilterMode	  = CAN_FilterMode_IdMask;
	CAN_Filter_Structure.CAN_FilterScale  = CAN_FilterScale_32bit;
	CAN_Filter_Structure.CAN_FilterIdHigh = 0x1314;
	CAN_Filter_Structure.CAN_FilterIdLow  = 0x1314;
	CAN_Filter_Structure.CAN_FilterActivation = ENABLE;
    
	CAN_FilterInit(UC_CAN_FILTER, UC_CAN_FR,&CAN_Filter_Structure);

	CAN_Reset(UC_CAN_CRTL);
    

    return RT_EOK;
}

static rt_err_t rt_can_init(struct rt_device* dev)
{
   rt_can_configure((struct rt_can_device*)dev, RT_NULL);
   return RT_EOK;
}

static rt_err_t rt_can_open(struct rt_device* dev, rt_uint16_t oflag)
{
    return RT_EOK;
}
static rt_err_t rt_can_close(struct rt_device* dev)
{
    return RT_EOK;
}
static rt_err_t rt_can_control(struct rt_device* dev,
                               int              cmd,
                               void*             args)
{
    return RT_EOK;
}


static rt_size_t rt_can_read(struct rt_device* dev,
                             rt_off_t          pos,
                             void*             buffer,
                             rt_size_t         size)
{
    CanRxMsg RxMessage;

    if(CAN_MessagePending(UC_CAN_CRTL) == 0){
        return 0;
    }
    CAN_Receive(UC_CAN_CRTL,&RxMessage);

    memcpy(buffer, RxMessage.Data, RxMessage.DLC > size ? size: RxMessage.DLC);

    return RxMessage.DLC;	

}

static rt_size_t rt_can_write(struct rt_device* dev,
                              rt_off_t          pos,
                              const void*       buffer,
                              rt_size_t         size)
{
        int i ;
        rt_can_msg_t data = (rt_can_msg_t)buffer;
	CanTxMsg TxMessage;
    
	TxMessage.StdId=0x12;			// \u6807\u51c6\u6807\u8bc6\u7b26 
	TxMessage.ExtId=0x123456;		// \u8bbe\u7f6e\u6269\u5c55\u6807\u793a\u7b26 
	TxMessage.IDE=CAN_Id_Extended; 	// \u6269\u5c55\u5e27
	TxMessage.RTR=CAN_RTR_Data;		// \u6570\u636e\u5e27
	TxMessage.DLC=size ;				// \u8981\u53d1\u9001\u7684\u6570\u636e\u957f\u5ea6

	for(i=0;i<size; i++){
		TxMessage.Data[i]=data->data[i];
	}
	CAN_Transmit(UC_CAN_CRTL, UC_CAN_TX,&TxMessage);  
       return size;
}

#ifdef RT_USING_DEVICE_OPS

const static struct rt_can_ops can_device_ops =
{
    rt_can_configure,
    rt_can_control,
    rt_can_write,
    rt_can_read,
};
#endif
/*
 * can register
 */
 
int rt_hw_can_init(void)
{
    struct rt_device* device;

    device = &(can.parent);

    device->type        = RT_Device_Class_CAN;
    device->rx_indicate = RT_NULL;
    device->tx_complete = RT_NULL;
    
    can.can_rx         = RT_NULL;
    can.can_tx         = RT_NULL;
    #ifdef RT_USING_DEVICE_OPS
    can.ops            = &can_device_ops;
    #else
    device->init        = rt_can_init;
    device->open        = rt_can_open;
    device->close       = rt_can_close;
    
    device->read        = rt_can_read;
    device->write       = rt_can_write;
    device->control     = rt_can_control;
    #endif
    can.status_indicate.ind  = RT_NULL;
    can.status_indicate.args = RT_NULL;

    device->user_data   = NULL;

    /* register a character device */
    return rt_device_register(device, "can1", RT_DEVICE_FLAG_RDWR);
}


INIT_BOARD_EXPORT(rt_hw_can_init);
#endif
