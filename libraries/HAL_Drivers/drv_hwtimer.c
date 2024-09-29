#include <drv_i2c.h>

#ifdef RT_USING_HWTIMER

#if !defined(BSP_USING_HWTIMER) || !defined(BSP_USING_HWTIMER1)
#error "Please define at least one BSP_USING_HWTIMER"
/* this driver can be disabled at menuconfig → RT-Thread Components → Device Drivers */
#endif

// #define DRV_DEBUG
#define LOG_TAG "drv.hwtimer"
#include <drv_log.h>

#define DEFAULT_PRESCALER (7)  // (0-7)
#define DEFAULT_FREQ (1000000) // 1Mhz
#define DEFAULT_CNT (BSP_CLOCK_SYSTEM_FREQ_HZ / (((DEFAULT_PRESCALER + 1)) * DEFAULT_FREQ))

enum
{
#ifdef BSP_USING_HWTIMER1
    TIM1_INDEX,
#endif
    TIM_MAX_INDEX
};

#ifndef TIM_DEV_INFO_CONFIG
#define TIM_DEV_INFO_CONFIG {      \
    .maxfreq = 1000000,            \
    .minfreq = 100,                \
    .maxcnt = 300000,              \
    .cntmode = HWTIMER_CNTMODE_UP, \
}
#endif /* TIM_DEV_INFO_CONFIG */

#ifdef BSP_USING_HWTIMER1
#ifndef TIMER1_CONFIG
#define TIMER1_CONFIG {        \
    .name = "timer1",          \
    .timer_handle = UC_TIMER1, \
    .timer_int = TIMER_IT_CMP, \
    .freq_cnt = DEFAULT_CNT,   \
}
#endif /* HWTIMER1_CONFIG */
#endif /* BSP_USING_HWTIMER1 */

struct uc8x88_hwtimer
{
    const char *name;
    TIMER_TYPE *timer_handle;
    TIMER_INT_TYPE timer_int;
    rt_uint32_t freq_cnt;
    rt_hwtimer_mode_t opmode;
    rt_hwtimer_t timer_device;
};

static struct uc8x88_hwtimer uc8x88_hwtimer_obj[] = {
#ifdef BSP_USING_HWTIMER1
    TIMER1_CONFIG,
#endif
};

static void uc8x88_timer_init(rt_hwtimer_t *timer, rt_uint32_t state)
{
    TIMER_TYPE *tim = RT_NULL;
    struct uc8x88_hwtimer *uc_timer = RT_NULL;

    RT_ASSERT(timer != RT_NULL);
    if (state)
    {
        TIMER_CFG_Type cfg;

        tim = (TIMER_TYPE *)timer->parent.user_data;
        uc_timer = rt_container_of(timer, struct uc8x88_hwtimer, timer_device);

        timer_int_disable(tim, uc_timer->timer_int);
        timer_disable(tim);
        timer_int_clear_pending(tim, uc_timer->timer_int);

        /* time init */
        cfg.cnt = 0x00;
        cfg.cmp = 0x00;
        cfg.pre = DEFAULT_PRESCALER;
        timer_init(tim, &cfg);
        timer_int_enable(tim, uc_timer->timer_int);

        LOG_D("%s init success", uc_timer->name);
    }
}

static rt_err_t uc8x88_timer_start(rt_hwtimer_t *timer, rt_uint32_t pr, rt_hwtimer_mode_t opmode)
{
    TIMER_TYPE *tim = RT_NULL;
    struct uc8x88_hwtimer *uc_timer = RT_NULL;

    RT_ASSERT(timer != RT_NULL);

    tim = (TIMER_TYPE *)timer->parent.user_data;
    uc_timer = rt_container_of(timer, struct uc8x88_hwtimer, timer_device);

    uc_timer->opmode = opmode;

    /* set tim cnt */
    timer_set_count(tim, 0);
    /* set tim compare */
    timer_set_compare_value(tim, (pr * uc_timer->freq_cnt));

    LOG_D("uc8x88_timer_start pr = %d, freq_cnt = %d", pr, uc_timer->freq_cnt);

    /* start timer */
    timer_enable(tim);

    return RT_EOK;
}

static void uc8x88_timer_stop(rt_hwtimer_t *timer)
{
    TIMER_TYPE *tim = RT_NULL;

    RT_ASSERT(timer != RT_NULL);
    tim = (TIMER_TYPE *)timer->parent.user_data;

    /* stop timer */
    timer_disable(tim);

    /* set tim cnt */
    timer_set_count(tim, 0);

    LOG_D("uc8x88_timer_stop");
}

static rt_uint32_t uc8x88_timer_counter_get(rt_hwtimer_t *timer)
{
    TIMER_TYPE *tim = RT_NULL;
    struct uc8x88_hwtimer *uc_timer = RT_NULL;

    RT_ASSERT(timer != RT_NULL);

    tim = (TIMER_TYPE *)timer->parent.user_data;
    uc_timer = rt_container_of(timer, struct uc8x88_hwtimer, timer_device);

    LOG_D("uc8x88_timer_counter_get: real count = %d, freq_cnt = %d", timer_get_count(tim), uc_timer->freq_cnt);

    return (timer_get_count(tim) / uc_timer->freq_cnt);
}

static rt_err_t uc8x88_timer_ctrl(rt_hwtimer_t *timer, rt_uint32_t cmd, void *arg)
{
    rt_err_t result = RT_EOK;
    struct uc8x88_hwtimer *uc_timer = RT_NULL;

    RT_ASSERT(timer != RT_NULL);
    RT_ASSERT(arg != RT_NULL);

    uc_timer = rt_container_of(timer, struct uc8x88_hwtimer, timer_device);

    switch (cmd)
    {
    case HWTIMER_CTRL_FREQ_SET:
    {
        rt_uint32_t freq;

        /* set timer frequence */
        freq = *((rt_uint32_t *)arg);

        uc_timer->freq_cnt = (BSP_CLOCK_SYSTEM_FREQ_HZ / ((DEFAULT_PRESCALER + 1) * freq));

        LOG_D("uc8x88_timer_ctrl freq = %d, freq_cnt = %d", freq, uc_timer->freq_cnt);
    }
    break;
    default:
    {
        result = -RT_EINVAL;
    }
    break;
    }

    return result;
}

static const struct rt_hwtimer_info _info = TIM_DEV_INFO_CONFIG;

static const struct rt_hwtimer_ops _ops = {
    .init = uc8x88_timer_init,
    .start = uc8x88_timer_start,
    .stop = uc8x88_timer_stop,
    .count_get = uc8x88_timer_counter_get,
    .control = uc8x88_timer_ctrl,
};

#ifdef BSP_USING_HWTIMER1
void timer1_compare_handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    // timer_int_clear_pending(UC_TIMER1, TIMER_IT_CMP);

    rt_device_hwtimer_isr(&uc8x88_hwtimer_obj[TIM1_INDEX].timer_device);

    timer_set_count(UC_TIMER1, 0);

    /* leave interrupt */
    rt_interrupt_leave();
}
#endif

static int rt_hw_hwtimer_init(void)
{
    int i = 0;
    int result = RT_EOK;

    for (i = 0; i < sizeof(uc8x88_hwtimer_obj) / sizeof(uc8x88_hwtimer_obj[0]); i++)
    {
        uc8x88_hwtimer_obj[i].timer_device.info = &_info;
        uc8x88_hwtimer_obj[i].timer_device.ops = &_ops;
        if (rt_device_hwtimer_register(&uc8x88_hwtimer_obj[i].timer_device, uc8x88_hwtimer_obj[i].name, uc8x88_hwtimer_obj[i].timer_handle) == RT_EOK)
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
INIT_BOARD_EXPORT(rt_hw_hwtimer_init);

#endif /* RT_USING_HWTIMER */
