/*
 *elec_api.h
 *
 * Copyright (c) 2017-2021, ucchip
 *
 * Change Logs:
 *
 * Date           Author       Notes
 *
 * 2021-04-19     lcj       the first version
 *
 * code_format:utf-8
 *
 * dlt645 2007 version
 */

#ifndef _ELEC_API_H
#define _ELEC_API_H

#include <rtthread.h>

#ifdef USING_ELEC_DEV

typedef enum
{
    ELEC_DEV_METER,

    ELEC_DEV_CCO,

    ELEC_DEV_CTRLER,

} elec_dev_type;



#define FN_MAX_NUM      15    //645的控制命令个数

typedef rt_int32_t (*exec_recv_func)(rt_uint8_t* inbuf);

typedef rt_int32_t (*exec_app)(void);

typedef rt_int32_t (*pack_func)(rt_uint8_t* buffer, rt_uint16_t* len);

typedef rt_int32_t (*parse_func)(rt_uint8_t* parsebuf, rt_uint16_t parselen, rt_uint8_t* outbuf );


typedef struct
{
    rt_uint8_t  type; //设备类型
    pack_func   pack_frame;   //设备组帧功能
    parse_func  parse_frame;  //设备解析功能
    exec_app    initapp;
    exec_recv_func  recvapp;
    exec_app  taskapp;

} s_dev_reg;


void elec_dev_thread(void* parameter);


#endif
#endif
