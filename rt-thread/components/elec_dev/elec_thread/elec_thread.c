/*
 *elec_api.c
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

#include "elec_drv.h"
#include "elec_thread.h"
#include "elec_dlt645.h"
#include "elec_1376_2.h"
#ifdef ELEC_DEV_CCO_MODE
#include "elec_cco_api.h"
#endif
#ifdef ELEC_DEV_METER_MODE
#include "elec_meter_api.h"
#endif
#ifdef ELEC_DEV_CTRLER_MODE
#include "elec_ctrler_api.h"
#endif

#ifdef USING_ELEC_DEV


//模块注册
static void elec_dev_register(s_dev_reg* dev, rt_uint8_t devtype, exec_app initfn, exec_recv_func recvfn, exec_app appfn)
{

    dev->type = devtype;
    dev->initapp = initfn;
    dev->recvapp = recvfn;
    dev->taskapp = appfn;

    if ((devtype == ELEC_DEV_METER) || (devtype == ELEC_DEV_CTRLER))
    {
        dev->pack_frame = elec_dev_pack_645_frame;
        dev->parse_frame = elec_dev_parse_645_frame;
    }

    if (devtype == ELEC_DEV_CCO)
    {
        dev->pack_frame = elec_dev_pack_1376_2_frame;
        dev->parse_frame = elec_dev_parse_1376_2_frame;
    }

}


/**
 * @brief       电力设备接收/处理数据的进程
 * @param       none
 * @return      none
 * @par         创建
 *              lcj2020-7-2创建
 */
void elec_dev_thread(void* parameter)
{
    rt_int32_t ret = -1;
    rt_uint16_t data_len = 0;
    rt_uint8_t recv_buf[ELEC_DEV_TRANS_MAX_LEN] = {0};
    rt_uint8_t data_buf[ELEC_DEV_TRANS_MAX_LEN] = {0};
    s_dev_reg elec_dev;

    rt_memset(&elec_dev, 0, sizeof(s_dev_reg));
    rt_memset(recv_buf, 0, sizeof(recv_buf));

#ifdef ELEC_DEV_METER_MODE
    elec_dev_register(&elec_dev, ELEC_DEV_METER, elec_meter_exec_recv, elec_meter_exec_send);
#endif
#ifdef ELEC_DEV_CTRLER_MODE
    elec_dev_register(&elec_dev, ELEC_DEV_CTRLER, elec_ctrler_exec_recv, elec_ctrler_exec_send);
#endif
#ifdef ELEC_DEV_CCO_MODE
    elec_dev_register(&elec_dev, ELEC_DEV_CCO, elec_cco_init, elec_cco_recv, elec_cco_task);
#endif

    elec_dev.initapp();

    while (1)
    {
        ret = elec_dev.pack_frame(recv_buf, &data_len);

        if (ret == 0)
        {
            ret = elec_dev.parse_frame(recv_buf, data_len, data_buf);

            if (ret == 0)
            {
                elec_dev.recvapp(data_buf);
            }
        }

        elec_dev.taskapp();

        rt_thread_mdelay(1);
    }
}


#endif
