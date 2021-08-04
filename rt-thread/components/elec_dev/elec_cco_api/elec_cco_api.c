/*
 *elec_cco_app.c
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
 * QGDW1376.2 2013 version
 */

#include "elec_thread.h"
#include "elec_cco_api.h"
#include "elec_1376_2.h"
#include "wg_phase.h"

#ifdef USING_ELEC_DEV

rt_uint32_t data_id_merge_big_endian(rt_uint8_t* in)
{
    rt_uint32_t value = 0;

    value = in[0] << 24 | in[1] << 16 | in[2] << 8 | in[3];

    return value;
}


static rt_int32_t elec_cco_recv_confirm(rt_uint16_t Fn, rt_uint8_t* datain, rt_uint16_t datalen)
{
    if (Fn == FN(1))
    {
        rt_kprintf("elec cco mode recv confirm\r\n");
    }

    return 0;
}
static rt_int32_t elec_cco_recv_transfer(rt_uint16_t Fn, rt_uint8_t* datain, rt_uint16_t datalen)
{
    rt_kprintf("transfer=%d\r\n", Fn);

    return 0;
}
static rt_int32_t elec_cco_recv_query(rt_uint16_t Fn, rt_uint8_t* datain, rt_uint16_t datalen)
{
    if (Fn == FN(10))
    {
        plc_detect_flag_set();

        rt_kprintf("elec cco mode recv query\r\n");
    }

    return 0;
}
static rt_int32_t elec_cco_recv_test(rt_uint16_t Fn, rt_uint8_t* datain, rt_uint16_t datalen)
{

    wg_plc_recv_node_info(Fn, datain, datalen);

    rt_kprintf("elec cco mode recv test=%d\r\n", Fn);

    return 0;
}


static s_cco_fn elec_cco_recvfn[FN_MAX_NUM] =
{
    {GDW1376_2_AFN_CONFIRM_0x00,        elec_cco_recv_confirm},

    {GDW1376_2_AFN_TRANSFER_0x02,       elec_cco_recv_transfer},

    {GDW1376_2_AFN_QUERY_0x03,          elec_cco_recv_query},

    {GDW1376_2_AFN_LINK_CHK_0x04,       RT_NULL},

    {GDW13766_2_AFN_CTRL_CMD_0x05,      RT_NULL},

    {GDW1376_2_AFN_ACTIVE_REPROT_0x06,  RT_NULL},

    {GDW1376_2_AFN_ROUTER_QUERY_0x10,   RT_NULL},

    {GDW1376_2_AFN_TEST_0xF0,           elec_cco_recv_test},

};


/**********************************************recv***************************************************/
//具体的功能应用执行接口

rt_int32_t elec_cco_init(void)
{

    wg_node_init();

    return 0;
}


/**********************************************recv***************************************************/


rt_int32_t elec_cco_recv(rt_uint8_t* datain)
{
    s_1376_2_format* cco_data = RT_NULL;
    rt_uint8_t i = 0;

    cco_data = (s_1376_2_format*)datain;

    for (i = 0; i < FN_MAX_NUM; i++)
    {
        if (cco_data->afn == elec_cco_recvfn[i].ctrl_code)
        {
            elec_cco_recvfn[i].pfunc(cco_data->fn, cco_data->data, cco_data->datalen);

            break;

        }
    }

    return 0;
}




/**********************************************task***************************************************/


//具体的功能应用执行接口

rt_int32_t elec_cco_task(void)
{

    wg_plc_mudule_detect();

    wg_plc_mudule_init();

    wg_plc_read_and_load_node_info();

    return 0;
}

#endif
