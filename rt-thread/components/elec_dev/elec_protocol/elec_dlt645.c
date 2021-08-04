/*
 *dlt645.c
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

#include "elec_dlt645.h"
#include "elec_drv.h"
#ifdef USING_ELEC_DEV

/**
 * @brief       645协议解析
 * @param
                in 数据输入
                inlen 输入数据长度
                out 数据输出645格式,out->data 需指定存储空间
 * @return      0:成功；其它：-1标识头不正确,2校验和不正确,3标识尾不正确,4解析的数据长度与输入数据长度不符
 * @par         创建
 *              lcj2020-7-2创建
 */
static rt_int32_t dlt_645_decode(rt_uint8_t* in, rt_uint16_t inlen, s_dlt645_format* out)
{
    rt_uint16_t i = 0;
    rt_uint16_t pos = 0;
    rt_uint8_t cs = 0;

    pos = 0;

    //(1+6+1+1+1+2)=12  标识头+地址域+标识+控制码+数据长度+校验和+标识尾
    if ((in == NULL) || (inlen < 12) || (in[pos] != 0x68))
    {
        return -1;
    }
    pos += 1;

    for (i = 0; i < 6; i++)
    {
        out->addr[i] = in[pos + 5 - i];
    }
    pos += 6;

    if (in[pos] != 0x68)
    {
        return -1;
    }
    pos += 1;

    out->ctrlcode = in[pos];
    pos += 1;

    out->datalen = in[pos]; //max len is 0xff
    pos += 1;

    if (out->datalen > ELEC_DEV_TRANS_MAX_LEN)
    {
        return -1;
    }

    for (i = 0; i < out->datalen; i++)
    {
        out->data[i] = in[pos + i] - 0x33;
    }
    pos += out->datalen;

    //计算校验和
    for (i = 0; i < pos; i++)
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

    if (pos != inlen)
    {
        return -4;
    }

    return 0; //成功
}

/**
 * @brief       645协议编码
 * @param
                in 645格式数据输入
                out 数据流输出

 * @return      -1:输入数据为空，其他：outlen 输出数据长度

 * @par         创建
 *              lcj 2020-7-2创建
 */
rt_int32_t dlt_645_encode(s_dlt645_format* in, rt_uint8_t* out)
{
    rt_uint16_t i = 0;
    rt_uint16_t pos = 0;
    rt_uint8_t cs = 0;

    if (in == NULL)
    {
        return -1;
    }

    //wg module answer package need precode
    for (i = 0; i < 4; i++)
    {
        out[i] = 0xFE;
    }

    pos = 4;

    out[pos] = 0x68;
    pos += 1;

    for (i = 0; i < 6; i++)
    {
        out[pos + i] = in->addr[i];
    }
    pos += 6;

    out[pos] = 0x68;
    pos += 1;

    out[pos] = in->ctrlcode;
    pos += 1;

    out[pos] = in->datalen;
    pos += 1;

    for (i = 0; i < in->datalen; i++)
    {
        out[pos + i] = in->data[i] + 0x33;
    }
    pos += in->datalen;

    //计算校验和,排除前导码
    for (i = 4; i < pos; i++)
    {
        cs += out[i];
    }

    out[pos] = cs;
    pos += 1;

    out[pos] = 0x16;
    pos += 1;

    return pos;
}


//wg slave plc module heart frame have precode and other frame have not,need_len init to 1 for compatiable
#define P645_HEAD_LEN 1


/**
 * @brief       将从串口接收到的数据，组包成完整的645数据帧
 * @param
                outbuf 存储数据的地址
                outlen 输入数据长度
 * @return      0:成功；-1:失败
 * @par         创建
 *              lcj2020-7-2创建
 */
rt_int32_t elec_dev_pack_645_frame(rt_uint8_t* out_buf, rt_uint16_t* out_len)
{

    rt_uint16_t read_len = 0;
    rt_int8_t ret = -1;
    static rt_uint8_t parse_step = 0;
    static rt_uint32_t timoutcnt = 0;
    static rt_uint16_t need_len = P645_HEAD_LEN;
    static rt_uint16_t now_len = 0;

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
        need_len = P645_HEAD_LEN;
    }

    switch (parse_step)
    {
        case 0:

            if (need_len == 0) //this time read over
            {
                if (out_buf[now_len - 1] == 0xFE)
                {
                    if (now_len <= 4)
                    {
                        need_len = 1; //continue read 0xfe,after read four 0xfe,begin to read 0x68
                    }
                    else
                    {
                        now_len = 0;
                        need_len = P645_HEAD_LEN; //if 0xfe 0xfe 0xfe 0xfe read over,next char still 0xfe,init this function
                    }
                }
                else if (out_buf[now_len - 1] == 0x68)
                {
                    need_len = 9; //read datalen of protocol
                    parse_step = 1;
                }
                else
                {

                    now_len = 0;
                    need_len = P645_HEAD_LEN;
                }
            }

            timoutcnt = 0;

            break;

        case 1:

            if (need_len == 0)
            {
                need_len += (out_buf[now_len - 1] + 2); //校验+结束符

                parse_step = 2;
            }

            timoutcnt++;

            if (timoutcnt > 10000)
            {
                parse_step = 0;
                now_len = 0;
                need_len = P645_HEAD_LEN;
            }

            break;

        case 2:

            if (need_len == 0)
            {
                *out_len = now_len;
                ret = 0;
                parse_step = 0;
                now_len = 0;
                need_len = P645_HEAD_LEN;
            }

            timoutcnt++;

            if (timoutcnt > 10000)
            {
                parse_step = 0;
                now_len = 0;
                need_len = P645_HEAD_LEN;
            }
            break;

        default:
            break;
    }

    return ret;
}


static rt_int32_t dlt645_precode_detect(rt_uint8_t* in, rt_uint16_t inlen)
{
    rt_int16_t ret = -1;
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
 * @brief       解析645数据帧
 * @param       parsebuf:待处理的数据
 *              parselen:待处理的数据长度
 * @return      0:成功；其它：失败
 * @par         创建
 *              lcj于2021-5-27创建
 */
rt_int32_t elec_dev_parse_645_frame(rt_uint8_t* parsebuf, rt_uint16_t parselen, rt_uint8_t* outbuf )
{
    rt_int32_t decode_ret = -1;
    rt_int32_t precode_location = 0;
    rt_uint8_t* plcdata = NULL;

    plcdata = (rt_uint8_t*)rt_malloc(ELEC_DEV_TRANS_MAX_LEN);

    if (plcdata != RT_NULL)
    {
        rt_memset(plcdata, 0, ELEC_DEV_TRANS_MAX_LEN);

        precode_location = dlt645_precode_detect(parsebuf, parselen);

        if (precode_location >= 0)
        {
            decode_ret = dlt_645_decode(&parsebuf[precode_location], (parselen - precode_location), (s_dlt645_format*)plcdata);
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

#endif
