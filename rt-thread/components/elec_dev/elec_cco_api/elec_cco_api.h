/*
 *elec_meter_app.h
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

#ifndef _ELEC_METER_APP_H_
#define _ELEC_METER_APP_H_

#include <rtthread.h>

#ifdef USING_ELEC_DEV

typedef rt_int32_t (*cco_func)(rt_uint16_t Fn, rt_uint8_t* datain, rt_uint16_t datalen);

typedef struct
{
    rt_uint8_t      ctrl_code;//控制码

    cco_func        pfunc;//执行功能的函数

} s_cco_fn;


//n(1~248)
//f1    0000 0000 0000 0001
//f9    0000 0001 0000 0001
//..
//f17   0000 0010 0000 0001
//f25   0001 0011 0000 0001
#define FN(n)   ((((n-1)/8)<<8)|(1<<((n-1)%8)))

rt_int32_t elec_cco_init(void);


rt_int32_t elec_cco_recv(rt_uint8_t* datain);


rt_int32_t elec_cco_task(void);

#endif
#endif
