/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-12-10     zylx         first version
 */

#include <board.h>
#include<rtthread.h>
#include<rtdevice.h>

#ifdef RT_USING_HWTIMER
//#include "drv_config.h"
#include "uc_timer.h"

//#define DRV_DEBUG
#define LOG_TAG             "drv.hwtimer"
#include <drv_log.h>

enum
{
#ifdef BSP_USING_TIM1
    TIM1_INDEX,
#endif
    TIM_INDEX_MAX,
};

struct uc8x88_hwtimer
{
    rt_hwtimer_t time_device;
    TIMER_TYPE* tim_handle;
    char* name;
};

#ifndef TIM_DEV_INFO_CONFIG
#define TIM_DEV_INFO_CONFIG                     \
    {                                           \
        .maxfreq = BSP_CLOCK_SYSTEM_FREQ_HZ,                     \
                   .minfreq = BSP_CLOCK_SYSTEM_FREQ_HZ / 8,                        \
                              .maxcnt  = 0x7FFFFFFF,                      \
                                         .cntmode = HWTIMER_CNTMODE_UP,          \
    }
#endif /* TIM_DEV_INFO_CONFIG */

#ifdef BSP_USING_TIM1
#ifndef TIM1_CONFIG
#define TIM1_CONFIG                                         \
    {                                                       \
        .tim_handle     = UC_TIMER1,                     \
                          .name                    = "timer1",                 \
    }
#endif /* TIM1_CONFIG */
#endif /* BSP_USING_TIM1 */

static struct uc8x88_hwtimer uc8x88_hwtimer_obj[] =
{
#ifdef BSP_USING_TIM1
    TIM1_CONFIG,
#endif
};

static void uc8x88_timer_init(struct rt_hwtimer_device* timer, rt_uint32_t state)
{
    TIMER_TYPE* tim = RT_NULL;
    struct uc8x88_hwtimer* tim_device = RT_NULL;

    RT_ASSERT(timer != RT_NULL);
    if (state)
    {
        TIMER_CFG_Type cfg;

        tim = (TIMER_TYPE*)timer->parent.user_data;
        tim_device = (struct uc8x88_hwtimer*)timer;
        tim_device = tim_device;

        /* time init */
        cfg.cnt = 0x0;
        cfg.cmp = 0x0;
        cfg.pre = 4;
        timer_init(tim, &cfg);
        timer_int_enable(tim, TIMER_IT_CMP);

        LOG_D("%s init success", tim_device->name);
    }
}

static rt_err_t uc8x88_timer_start(rt_hwtimer_t* timer, rt_uint32_t t, rt_hwtimer_mode_t opmode)
{
    rt_err_t result = RT_EOK;
    TIMER_TYPE* tim = RT_NULL;

    RT_ASSERT(timer != RT_NULL);

    tim = (TIMER_TYPE*)timer->parent.user_data;

    /* set tim cnt */
    timer_set_count(tim, 0);
    /* set tim arr */
    timer_set_compare_value(tim, t - 1);
    LOG_D("uc8x88_timer_start t = %d", t);

    if (opmode == HWTIMER_MODE_ONESHOT)
    {
        /* set timer to single mode */
        //tim->Instance->CR1 |= TIM_OPMODE_SINGLE;
    }
    else
    {
        //tim->Instance->CR1 &= (~TIM_OPMODE_SINGLE);
    }

    /* start timer */
    timer_enable(tim);

    return result;
}

static void uc8x88_timer_stop(rt_hwtimer_t* timer)
{
    TIMER_TYPE* tim = RT_NULL;

    RT_ASSERT(timer != RT_NULL);

    tim = (TIMER_TYPE*)timer->parent.user_data;

    /* stop timer */
    timer_disable(tim);

    /* set tim cnt */
    timer_set_count(tim, 0);

    LOG_D("uc8x88_timer_stop");
}

static rt_err_t uc8x88_timer_ctrl(rt_hwtimer_t* timer, rt_uint32_t cmd, void* arg)
{
    TIMER_TYPE* tim = RT_NULL;
    rt_err_t result = RT_EOK;

    RT_ASSERT(timer != RT_NULL);
    RT_ASSERT(arg != RT_NULL);

    tim = (TIMER_TYPE*)timer->parent.user_data;

    switch (cmd)
    {
        case HWTIMER_CTRL_FREQ_SET:
        {
            rt_uint32_t freq;
            rt_uint16_t val;

            /* set timer frequence */
            freq = *((rt_uint32_t*)arg);
            LOG_D("uc8x88_timer_ctrl freq = %d", freq);

            val = BSP_CLOCK_SYSTEM_FREQ_HZ / freq;
            if (val < 1)
            {
                val = 1;
            }
            timer_set_prescaler_value(tim, val - 1);
        }
        break;
        default:
        {
            result = -RT_ENOSYS;
        }
        break;
    }

    return result;
}

static rt_uint32_t uc8x88_timer_counter_get(rt_hwtimer_t* timer)
{
    TIMER_TYPE* tim = RT_NULL;

    RT_ASSERT(timer != RT_NULL);

    tim = (TIMER_TYPE*)timer->parent.user_data;

    return timer_get_count(tim);
}

static const struct rt_hwtimer_info _info = TIM_DEV_INFO_CONFIG;

static const struct rt_hwtimer_ops _ops =
{
    .init = uc8x88_timer_init,
    .start = uc8x88_timer_start,
    .stop = uc8x88_timer_stop,
    .count_get = uc8x88_timer_counter_get,
    .control = uc8x88_timer_ctrl,
};

#ifdef BSP_USING_TIM1
void timer1_compare_handler(void)
{
    //timer_int_clear_pending(UC_TIMER1, TIMER_IT_CMP);

    timer_set_count(uc8x88_hwtimer_obj[TIM1_INDEX].tim_handle, 0);

    rt_device_hwtimer_isr(&uc8x88_hwtimer_obj[TIM1_INDEX].time_device);
}
#endif

static int uc8x88_hwtimer_init(void)
{
    int i = 0;
    int result = RT_EOK;

    for (i = 0; i < sizeof(uc8x88_hwtimer_obj) / sizeof(uc8x88_hwtimer_obj[0]); i++)
    {
        uc8x88_hwtimer_obj[i].time_device.info = &_info;
        uc8x88_hwtimer_obj[i].time_device.ops  = &_ops;
        if (rt_device_hwtimer_register(&uc8x88_hwtimer_obj[i].time_device, uc8x88_hwtimer_obj[i].name, uc8x88_hwtimer_obj[i].tim_handle) == RT_EOK)
        {
            LOG_D("%s register success", uc8x88_hwtimer_obj[i].name);
        }
        else
        {
            LOG_E("%s register failed", uc8x88_hwtimer_obj[i].name);
            result = -RT_ERROR;
        }
    }

    return result;
}
INIT_BOARD_EXPORT(uc8x88_hwtimer_init);

#endif /* RT_USING_HWTIMER */
