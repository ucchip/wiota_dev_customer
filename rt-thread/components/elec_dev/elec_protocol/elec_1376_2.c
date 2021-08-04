/*
 *gdw1376_2.c
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
 * qgdw1376.2 2013 version
 */

#include "elec_1376_2.h"
#include "elec_drv.h"

#ifdef USING_ELEC_DEV

/**
 * @brief       gdw1376.2协议解析
 * @param
                in 数据输入
                out 数据输出645格式
                out->data 需指定存储空间
                inlen 输入数据长度
 * @return      0:成功；其它：-1标识头不正确,2校验和不正确,3标识尾不正确
 * @par         创建
 *              lcj2020-7-2创建
 */
static rt_int8_t gdw_1376_2_decode(rt_uint8_t* in, rt_uint16_t inlen, s_1376_2_format* out)
{
    rt_uint16_t i = 0;
    rt_uint16_t pos = 0;
    rt_uint8_t cs = 0;
    rt_uint16_t len = 0;

    pos = 0;

    //最小长度：标识1+长度2+控制数据1+校验1+标识1
    if ((in == NULL) || (inlen < 7) || (in[pos] != 0x68))
    {
        return -1;
    }
    pos += 1;

    out->datalen = in[pos];
    pos += 1;

    out->datalen = (in[pos] << 8) | out->datalen;
    pos += 1;

    //    out->ctrlcode=in[pos];    //控制码，未使用
    pos += 1;

    if (in[pos] & 0x04)
    {
        out->module_flag = 1;
    }
    pos += 1;

    pos += 1;

    pos += 1;

    pos += 3;

    if (out->module_flag == 1)
    {
        rt_memcpy(out->tn_addr, &in[pos], 6);

        //      for(i=0;i<6;i++)
        //      {
        //          out->tn_addr[i]=in[pos+5-i];
        //      }
        pos += 6;

        pos += 6;
    }

    out->afn = in[pos];
    pos += 1;

    out->fn = in[pos];
    pos += 1;

    out->fn = (in[pos] << 8) | out->fn;
    pos += 1;

    len = out->datalen - pos - 2;

    for (i = 0; i < len; i++)
    {
        out->data[i] = in[pos + i];
    }
    pos += len;

    for (i = 3; i < pos; i++)
    {
        cs += in[i];
    }

    if (cs != in[pos])
    {
        return -2;
    }
    pos += 1;

    if (in[pos] != 0x16)
    {
        return -3;
    }
    pos += 1;

    return 0;
}

/**
 * @brief       gdw1376.2协议编码
 * @param
                in 645格式数据输入
                out 数据流输出

 * @return      outlen 输出数据长度

 * @par         创建
 *              lcj 2020-7-2创建
 */
rt_int32_t gdw_1376_2_encode(rt_uint8_t* sys_addr, s_1376_2_format* in, rt_uint8_t* out)
{
    rt_uint16_t i = 0;
    rt_uint16_t pos = 0;
    rt_uint16_t cs = 0;
    rt_uint16_t len_pos = 0;
    static rt_uint8_t frameindex;

    if (in == NULL)
    {
        return -1;
    }

    pos = 0;

    //不添加前导码

    /*
        for(i=0;i<4;i++)
        {
            out[pos]=0xFE;
            pos+=1;
        }
    */

    out[pos] = 0x68;
    pos += 1;

    len_pos = pos;
    pos += 2;

    out[pos] = 0x41;
    pos += 1;

    out[pos] = 0x00 | (in->module_flag << 2);
    pos += 5;

    out[pos] = frameindex;
    frameindex += 1;
    pos += 1;

    if (in->module_flag)
    {

        rt_memcpy(&out[pos], sys_addr, 6);
        pos += 6;

        rt_memcpy(&out[pos], in->tn_addr, 6);
        pos += 6;
    }

    out[pos] = in->afn;
    pos += 1;

    out[pos] = in->fn & 0xff;
    pos += 1;
    out[pos] = (in->fn >> 8) & 0xff;
    pos += 1;

    rt_memcpy(&out[pos], in->data, in->datalen);
    pos += in->datalen;

    for (i = len_pos + 2; i < pos; i++)
    {
        cs += out[i];
    }

    out[pos] = cs & 0xff;
    pos += 1;

    out[pos] = 0x16;
    pos += 1;

    out[len_pos] = pos & 0xff;
    out[len_pos + 1] = (pos >> 8) & 0xff;

    return pos;
}



/**
 * @brief       将从串口接收到的数据，组包成完整的1376.2数据帧
 * @param
                outbuf 存储数据的地址
                outlen 输入数据长度
 * @return      0:成功；-1:失败
 * @par         创建
 *              lcj2020-7-2创建
 */


//wg slave plc module heart frame have precode and other frame have not,need_len init to 1 for compatiable
#define P1376_2_HEAD_LEN   1

rt_int32_t elec_dev_pack_1376_2_frame(rt_uint8_t* out_buf, rt_uint16_t* out_len)
{

    rt_uint16_t    read_len = 0;
    rt_int32_t      ret = -1;
    static rt_uint8_t      parse_step = 0;
    static rt_uint32_t     timoutcnt = 0;
    static rt_uint16_t     need_len = P1376_2_HEAD_LEN;
    static rt_uint16_t     now_len = 0;

    read_len = elec_uart_read(&out_buf[now_len], need_len);

    if (read_len <= need_len)
    {
        now_len += read_len;

        need_len -= read_len;

    }
    else
    {
        parse_step = 0;
        now_len = 0;
        need_len = P1376_2_HEAD_LEN;

    }

    switch (parse_step)
    {
        case 0:

            if (need_len == 0)  //this time read over
            {
                if (out_buf[now_len - 1] == 0x68)
                {
                    need_len = 2;   //read datalen of protocol
                    parse_step = 1;

                }
                else
                {
                    now_len = 0;
                    need_len = P1376_2_HEAD_LEN;


                }
            }

            timoutcnt = 0;

            break;

        case 1:

            if (need_len == 0)
            {
                need_len = need_len + (out_buf[now_len - 1] << 8 | out_buf[now_len - 2]) - now_len; //all_len - now_len =need_len

                parse_step = 2;
            }

            timoutcnt++;

            if (timoutcnt > 10000)
            {
                parse_step = 0;
                now_len = 0;
                need_len = P1376_2_HEAD_LEN;

            }

            break;

        case 2:

            if (need_len == 0)
            {
                *out_len = now_len;
                ret = 0;

                parse_step = 0;
                now_len = 0;
                need_len = P1376_2_HEAD_LEN;

            }

            timoutcnt++;

            if (timoutcnt > 10000)
            {
                parse_step = 0;
                now_len = 0;
                need_len = P1376_2_HEAD_LEN;

            }
            break;


        default:
            break;
    }

    return ret;
}


static rt_int32_t gdw1376_2_precode_detect(rt_uint8_t* in, rt_uint16_t inlen)
{
    rt_int32_t ret = -1;
    rt_uint16_t i = 0;

    for (i = 0; i < inlen; i++)
    {
        if ((in[i] == 0xFE) && (in[i + 1] == 0xFE) && (in[i + 2] == 0xFE) && (in[i + 3] == 0xFE))
        {
            ret = i + 4;

            break;
        }
        if (in[i] == 0x68)
        {
            ret = i;

            break;
        }
    }

    return ret;
}




/**
 * @brief       解析1376.2数据帧
 * @param       parsebuf:待处理的数据
 *              parselen:待处理的数据长度
 * @return      0:成功；其它：失败
 * @par         创建
 *              lcj于2021-5-27创建
 */
rt_int32_t elec_dev_parse_1376_2_frame(rt_uint8_t* parsebuf, rt_uint16_t parselen, rt_uint8_t* outbuf )
{
    rt_int32_t decode_ret = -1;
    rt_int32_t precode_location = 0;
    rt_uint8_t* plcdata = NULL;

    plcdata = (rt_uint8_t*)rt_malloc(ELEC_DEV_TRANS_MAX_LEN);

    if (plcdata != RT_NULL)
    {
        rt_memset(plcdata, 0, ELEC_DEV_TRANS_MAX_LEN);

        precode_location = gdw1376_2_precode_detect(parsebuf, parselen);

        if (precode_location >= 0)
        {
            decode_ret = gdw_1376_2_decode(&parsebuf[precode_location], (parselen - precode_location), (s_1376_2_format*)plcdata);
        }

        if (decode_ret == 0) //645协议
        {
            rt_memcpy(outbuf, plcdata, ELEC_DEV_TRANS_MAX_LEN);

        }

        rt_free(plcdata);

        return 0;

    }
    else
    {
        return -1;
    }

}


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


rt_int32_t elec_1376_2_send(rt_uint8_t* main_addr, rt_uint8_t* slave_addr, rt_uint8_t afn, rt_uint16_t fn, rt_uint8_t* d_area, rt_uint16_t d_len)
{
    s_1376_2_format* send = NULL;
    rt_uint8_t* sendbuf = NULL;
    rt_int32_t ret = -1;
    rt_int32_t encode_len = 0;

    send = (s_1376_2_format*)rt_malloc(ELEC_DEV_TRANS_MAX_LEN);

    if (send != RT_NULL)
    {
        rt_memset(send, 0, ELEC_DEV_TRANS_MAX_LEN);

        if (slave_addr == RT_NULL)
        {
            send->module_flag = 0;
        }
        else
        {
            send->module_flag = 1;

            rt_memcpy(send->tn_addr, slave_addr, 6);
        }

        send->afn = afn;

        send->fn = fn;

        rt_memcpy(send->data, d_area, d_len);

        send->datalen = d_len;

        sendbuf = (rt_uint8_t*)rt_malloc(ELEC_DEV_TRANS_MAX_LEN);

        if (sendbuf != RT_NULL)
        {
            rt_memset(sendbuf, 0, sizeof(ELEC_DEV_TRANS_MAX_LEN));

            encode_len = gdw_1376_2_encode(main_addr, send, sendbuf);

            if (encode_len > 0)
            {
                ret = elec_uart_write(sendbuf, encode_len);
            }

            rt_free(sendbuf);
        }

        rt_free(send);

    }

    return ret;
}


#endif
