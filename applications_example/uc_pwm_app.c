#include <rtthread.h>
#ifdef APP_EXAMPLE_PWM

#ifndef RT_USING_PWM
#error "Please enable rt-thread PWM device driver"
#endif

#ifndef BSP_USING_PWM
#error "Please enable on-chip peripheral pwm config"
#endif

#ifndef BSP_USING_PWM0
#error "Please enable on-chip peripheral pwm0 config"
#endif

#include <rtdevice.h>
#include "uc_pwm_app.h"

#define THREAD_STACK_SIZE 512
#define THREAD_PRIORITY 5
#define THREAD_TIMESLICE 5

#define PWM_DEVICE_NAME "pwm0"
#define PWM_DEV_CHANNEL 0 // 此值无用，共有4组pwm，每组只有1个通道

struct rt_device_pwm *pwm_dev = RT_NULL;

static void pwm_thread_entry(void *parameter)
{
    rt_uint32_t value = 0;
    /* 设置PWM周期和脉冲宽度默认值 */
    rt_pwm_set(pwm_dev, PWM_DEV_CHANNEL, 10000, 0);
    /* 使能设备 */
    rt_pwm_enable(pwm_dev, PWM_DEV_CHANNEL);

    while (1)
    {
        rt_thread_mdelay(1000);
        rt_kprintf("pwm out period %d, pulse %d\n", 10000, value);
        rt_pwm_set(pwm_dev, PWM_DEV_CHANNEL, 10000, value);
        value += (10000 / 10);
        if (value > 10000)
        {
            value = 0;
        }
    }
}

int pwm_app_sample(void)
{
    rt_thread_t thread = RT_NULL;

    rt_kprintf("pwm_app_sample\n");

    pwm_dev = (struct rt_device_pwm *)rt_device_find(PWM_DEVICE_NAME);
    if (pwm_dev == RT_NULL)
    {
        rt_kprintf("find %s failed!\n", PWM_DEVICE_NAME);
        return -RT_ERROR;
    }

    /* 创建 serial 线程 */
    thread = rt_thread_create("pwm_app",
                              pwm_thread_entry,
                              RT_NULL,
                              THREAD_STACK_SIZE,
                              THREAD_PRIORITY,
                              THREAD_TIMESLICE);
    /* 创建成功则启动线程 */
    if (RT_NULL == thread)
    {
        return -RT_ERROR;
    }
    else
    {
        rt_thread_startup(thread);
    }

    return RT_EOK;
}

#endif
