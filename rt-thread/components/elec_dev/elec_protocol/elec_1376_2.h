/*
 *gdw1376_2.h
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


#ifndef _GDW1376_2_H
#define _GDW1376_2_H

#include <rtthread.h>
#include "elec_thread.h"

#ifdef USING_ELEC_DEV

typedef enum
{
    GDW1376_2_AFN_CONFIRM_0x00          = 0x00, //确认/否认

    GDW1376_2_AFN_TRANSFER_0x02         = 0x02, //数据转发

    GDW1376_2_AFN_QUERY_0x03            = 0x03, //查询数据

    GDW1376_2_AFN_LINK_CHK_0x04         = 0x04, //链路检测

    GDW13766_2_AFN_CTRL_CMD_0x05         = 0x05, //控制命令

    GDW1376_2_AFN_ACTIVE_REPROT_0x06    = 0x06, //主动上报

    GDW1376_2_AFN_ROUTER_QUERY_0x10     = 0x10, //路由查询

    GDW1376_2_AFN_TEST_0xF0             = 0xF0, //内部调试

} e_13762_afn;




//376.2总结构体
typedef struct
{
    rt_uint8_t module_flag;    //通信模块标识        0:对集中控制器的载波模块操作，1对单灯的载波模块操作

    rt_uint8_t tn_addr[6];     //从载波模块或者单灯的地址

    rt_uint8_t afn;    //应用功能码

    rt_uint16_t fn;    //数据单元标识,表示移位多少,encode时,移位

    rt_uint16_t datalen;

    rt_uint8_t data[ELEC_DEV_TRANS_MAX_LEN]; //数据单元

} s_1376_2_format;


/**
 * @brief       将从串口接收到的数据，组包成完整的1376.2数据帧
 * @param
                outbuf 存储数据的地址
                outlen 输入数据长度
 * @return      0:成功；-1:失败
 * @par         创建
 *              lcj2020-7-2创建
 */

rt_int32_t elec_dev_pack_1376_2_frame(rt_uint8_t* out_buf, rt_uint16_t* out_len);



/**
 * @brief       解析1376.2数据帧
 * @param       parsebuf:待处理的数据
 *              parselen:待处理的数据长度
 * @return      0:成功；其它：失败
 * @par         创建
 *              lcj于2021-5-27创建
 */
rt_int32_t elec_dev_parse_1376_2_frame(rt_uint8_t* parsebuf, rt_uint16_t parselen, rt_uint8_t* outbuf );


/**
 * @brief       发送1376.2数据帧
 * @param       tnaddr:从模块地址，发给主模块不需要填
 *              afn
                fn
                d_area:数据
                d_len:数据长度
 * @return      0:  发送数据为空或者数据长度小于1
                -1: 失败
                其他：实际发送的数据长度
 * @par         创建
 *              lcj于2021-5-27创建
 */

rt_int32_t elec_1376_2_send(rt_uint8_t* main_addr, rt_uint8_t* slave_addr, rt_uint8_t afn, rt_uint16_t fn, rt_uint8_t* d_area, rt_uint16_t d_len);


#endif
#endif


