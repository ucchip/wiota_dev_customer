/*
 *dlt645.h
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

#ifndef _DLT645_H
#define _DLT645_H

#include <rtthread.h>
#include "elec_thread.h"

#ifdef USING_ELEC_DEV


typedef enum
{

    /**********************************************************************/
    DLT645_CTRLCODE_0x11        = 0x11,     //读电能表数据

    DLT645_CTRLCODE_0x91        = 0x91,     //电能表正确应答

    DLT645_CTRLCODE_0xD1        = 0xD1,     //电能表错误应答

    /**********************************************************************/
    DLT645_CTRLCODE_0x13        = 0x13,     //读电能表地址

    DLT645_CTRLCODE_0x93        = 0x93,     //电能表正确应答

    /**********************************************************************/
    DLT645_CTRLCODE_0x14        = 0x14,     //写电能表数据

    DLT645_CTRLCODE_0x94        = 0x94,     //电能表正确应答

    DLT645_CTRLCODE_0xD4        = 0xD4,     //电能表错误应答

    /**********************************************************************/

} e_645_cmd;


#define DLT645_VERSION      0x02    //01H:DL/T645 1997,02H DL/T645 2007,3H-FFH保留

typedef struct
{
    rt_uint8_t addr[6];

    rt_uint8_t ctrlcode;

    rt_uint8_t datalen;

    rt_uint8_t data[ELEC_DEV_TRANS_MAX_LEN]; //数据

} s_dlt645_format;


typedef rt_int32_t (*dlt645_fn)(rt_uint8_t ctrlcode, rt_uint8_t* buf, rt_uint16_t len);



/**
 * @brief       645协议编码
 * @param
                in 645格式数据输入
                out 数据流输出

 * @return      -1:输入数据为空，其他：outlen 输出数据长度

 * @par         创建
 *              lcj 2020-7-2创建
 */
rt_int32_t dlt_645_encode(s_dlt645_format* in, rt_uint8_t* out);

/**
 * @brief       将从串口接收到的数据，组包成完整的645数据帧
 * @param
                outbuf 存储数据的地址
                outlen 输入数据长度
 * @return      0:成功；-1:失败
 * @par         创建
 *              lcj2020-7-2创建
 */
rt_int32_t elec_dev_pack_645_frame(rt_uint8_t* out_buf, rt_uint16_t* out_len);

/**
 * @brief       解析645数据帧
 * @param       parsebuf:待处理的数据
 *              parselen:待处理的数据长度
 * @return      0:成功；其它：失败
 * @par         创建
 *              lcj于2021-5-27创建
 */
rt_int32_t elec_dev_parse_645_frame(rt_uint8_t* parsebuf, rt_uint16_t parselen, rt_uint8_t* outbuf );


#endif
#endif
