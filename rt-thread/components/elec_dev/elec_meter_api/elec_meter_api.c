/*
 *elec_meter_app.c
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

#include "elec_thread.h"
#include "elec_dlt645.h"

#ifdef USING_ELEC_DEV

static void function1(void* parameter)
{
}
static void function2(void* parameter)
{
}
static void function3(void* parameter)
{
}
static void function4(void* parameter)
{
}


static s_app_fn elec_meter_fn[FN_MAX_NUM]
{
    {DLT645_CTRLCODE_0x11,  function1},
    {DLT645_CTRLCODE_0x13,  function2},
    {DLT645_CTRLCODE_0x14,  function3},

};
/**
 * @brief       电力设备-电表线程任务
 * @param       parameter：线程参数(基本无用)
 * @return      无
 * @par         创建
 *              lcj于2021-5-27创建
 */

void elec_meter_app_reg(void)
{

}

#endif
