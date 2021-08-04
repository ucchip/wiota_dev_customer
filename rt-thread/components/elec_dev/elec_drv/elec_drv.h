/*
 *elec_drv.h
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

#ifndef _ELEC_DRV_H_
#define _ELEC_DRV_H_

#include <rtthread.h>

#ifdef USING_ELEC_DEV


#define PLC_RESET_PIN       GET_PIN(B, 3)

#define PLC_TRIGGER_PIN     GET_PIN(B, 4)

#define PLC_EVENT_PIN     GET_PIN(B, 4)

/**
 * @brief       复位模块
 * @param       无
 * @return      无
 */
void elec_reset(void);

/**
 * @brief       触发此IO口，meter将请求主动上报
 * @param       无
 * @return      无
 */
void elec_eventup_on(void);

/**
 * @brief       串口发送接口
 * @param       buff：待发送的数据
 *              len:待发送的数据长度
 * @return      0:发送数据为空或者数据长度小于1，其他：实际发送的数据长度
 */
rt_int32_t elec_uart_write(void* buff, rt_uint32_t len);

/**
* @brief       串口接收接口
* @param       buff：存放读取的数据
*              len:待读取的数据长度
* @return      0:发送数据为空或者数据长度小于1，其他：实际接收的数据长度
*/
rt_int32_t elec_uart_read(void* buff, rt_uint32_t len);

#endif
#endif
