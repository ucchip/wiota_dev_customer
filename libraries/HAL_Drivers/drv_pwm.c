#include "drv_pwm.h"

#ifdef RT_USING_PWM

#if !defined(BSP_USING_PWM) || (!defined(BSP_USING_PWM0) && !defined(BSP_USING_PWM1) && !defined(BSP_USING_PWM2) && !defined(BSP_USING_PWM3))
#error "Please define at least one BSP_USING_PWM"
/* this driver can be disabled at menuconfig → RT-Thread Components → Device Drivers */
#endif

// #define DRV_DEBUG
#define LOG_TAG "drv.pwm"
#include <drv_log.h>

enum
{
#ifdef BSP_USING_PWM0
    PWM0_INDEX,
#endif
#ifdef BSP_USING_PWM1
    PWM1_INDEX,
#endif
#ifdef BSP_USING_PWM2
    PWM2_INDEX,
#endif
#ifdef BSP_USING_PWM3
    PWM3_INDEX,
#endif
    PWM_MAX_INDEX
};

#ifdef BSP_USING_PWM0
#ifndef PWM0_CONFIG
#define PWM0_CONFIG {      \
    .name = "pwm0",        \
    .pwm_handle = UC_PWM0, \
}
#endif /* PWM0_CONFIG */
#endif /* BSP_USING_PWM0 */

#ifdef BSP_USING_PWM1
#ifndef PWM1_CONFIG
#define PWM1_CONFIG {      \
    .name = "pwm1",        \
    .pwm_handle = UC_PWM1, \
}
#endif /* PWM1_CONFIG */
#endif /* BSP_USING_PWM1 */

#ifdef BSP_USING_PWM2
#ifndef PWM2_CONFIG
#define PWM2_CONFIG {      \
    .name = "pwm2",        \
    .pwm_handle = UC_PWM2, \
}
#endif /* PWM2_CONFIG */
#endif /* BSP_USING_PWM2 */

#ifdef BSP_USING_PWM3
#ifndef PWM3_CONFIG
#define PWM3_CONFIG {      \
    .name = "pwm3",        \
    .pwm_handle = UC_PWM3, \
}
#endif /* PWM3_CONFIG */
#endif /* BSP_USING_PWM3 */

struct uc8x88_pwm
{
    char *name;
    PWM_TypeDef *pwm_handle;
    rt_uint32_t duty;
    struct rt_device_pwm pwm_device;
};

static struct uc8x88_pwm uc8x88_pwm_obj[] = {
#ifdef BSP_USING_PWM0
    PWM0_CONFIG,
#endif

#ifdef BSP_USING_PWM1
    PWM1_CONFIG,
#endif

#ifdef BSP_USING_PWM2
    PWM2_CONFIG,
#endif

#ifdef BSP_USING_PWM3
    PWM3_CONFIG,
#endif
};

static rt_err_t uc8x88_pwm_gpio_init(struct uc8x88_pwm *device)
{
    rt_err_t result = RT_EOK;
    PWM_TypeDef *pwm = RT_NULL;

    RT_ASSERT(device != RT_NULL);

    pwm = (PWM_TypeDef *)device->pwm_handle;

    if (pwm == UC_PWM0)
    {
        gpio_set_pin_mux(UC_GPIO_CFG, GPIO_PIN_15, GPIO_FUNC_2);
    }
    else if (pwm == UC_PWM1)
    {
        gpio_set_pin_mux(UC_GPIO_CFG, GPIO_PIN_16, GPIO_FUNC_2);
    }
    else if (pwm == UC_PWM2)
    {
        gpio_set_pin_mux(UC_GPIO_CFG, GPIO_PIN_17, GPIO_FUNC_2);
    }
    else if (pwm == UC_PWM3)
    {
        gpio_set_pin_mux(UC_GPIO_CFG, GPIO_PIN_18, GPIO_FUNC_2);
    }

    return result;
}

static rt_err_t uc8x88_pwm_enable(PWM_TypeDef *hpwm, struct rt_pwm_configuration *configuration, rt_bool_t enable)
{
    if (enable)
    {
        uint32_t pwm_duty = 0;
        for (int i = 0; i < sizeof(uc8x88_pwm_obj) / sizeof(uc8x88_pwm_obj[0]); i++)
        {
            if (uc8x88_pwm_obj[i].pwm_handle == hpwm)
            {
                pwm_duty = uc8x88_pwm_obj[i].duty;
                break;
            }
        }
        pwm_set_duty(hpwm, pwm_duty);
        pwm_enable(hpwm);
    }
    else
    {
        pwm_set_duty(hpwm, 0);
        pwm_disable(hpwm);
    }

    return RT_EOK;
}

static rt_err_t uc8x88_pwm_get(PWM_TypeDef *hpwm, struct rt_pwm_configuration *configuration)
{
    uint32_t pwm_count = 0;
    uint32_t pwm_duty = 0;
    uint32_t ns_ratio = (BSP_CLOCK_SYSTEM_FREQ_HZ / 1000000UL);

    pwm_count = pwm_get_period(hpwm);
    pwm_duty = pwm_get_duty(hpwm);
    configuration->period = pwm_count / ns_ratio;
    configuration->pulse = pwm_duty / ns_ratio;

    return RT_EOK;
}

static rt_err_t uc8x88_pwm_set(PWM_TypeDef *hpwm, struct rt_pwm_configuration *configuration)
{
    uint32_t pwm_count = 0;
    uint32_t pwm_duty = 0;
    uint32_t ns_ratio = (BSP_CLOCK_SYSTEM_FREQ_HZ / 1000000UL);

    pwm_count = configuration->period * ns_ratio;
    pwm_duty = configuration->pulse * ns_ratio;
    pwm_set_period(hpwm, pwm_count);
    pwm_set_duty(hpwm, pwm_duty);

    for (int i = 0; i < sizeof(uc8x88_pwm_obj) / sizeof(uc8x88_pwm_obj[0]); i++)
    {
        if (uc8x88_pwm_obj[i].pwm_handle == hpwm)
        {
            uc8x88_pwm_obj[i].duty = pwm_duty;
            break;
        }
    }

    return RT_EOK;
}

static rt_err_t uc8x88_pwm_control(struct rt_device_pwm *device, int cmd, void *arg)
{
    struct rt_pwm_configuration *configuration = (struct rt_pwm_configuration *)arg;
    PWM_TypeDef *hpwm = (PWM_TypeDef *)device->parent.user_data;

    switch (cmd)
    {
    case PWM_CMD_ENABLE:
        return uc8x88_pwm_enable(hpwm, configuration, RT_TRUE);
    case PWM_CMD_DISABLE:
        return uc8x88_pwm_enable(hpwm, configuration, RT_FALSE);
    case PWM_CMD_SET:
        return uc8x88_pwm_set(hpwm, configuration);
    case PWM_CMD_GET:
        return uc8x88_pwm_get(hpwm, configuration);
    default:
        return RT_EINVAL;
    }
}

static struct rt_pwm_ops drv_ops = {
    uc8x88_pwm_control,
};

int rt_hw_pwm_init(void)
{
    int i = 0;
    int result = RT_EOK;

    for (i = 0; i < sizeof(uc8x88_pwm_obj) / sizeof(uc8x88_pwm_obj[0]); i++)
    {
        /* pwm init */
        if (uc8x88_pwm_gpio_init(&uc8x88_pwm_obj[i]) != RT_EOK)
        {
            LOG_E("%s init failed", uc8x88_pwm_obj[i].name);
            result = -RT_ERROR;
            goto __exit;
        }
        else
        {
            LOG_D("%s init success", uc8x88_pwm_obj[i].name);

            /* register pwm device */
            if (rt_device_pwm_register(&uc8x88_pwm_obj[i].pwm_device, uc8x88_pwm_obj[i].name, &drv_ops, uc8x88_pwm_obj[i].pwm_handle) == RT_EOK)
            {
                LOG_D("%s register success", uc8x88_pwm_obj[i].name);
            }
            else
            {
                LOG_E("%s register failed", uc8x88_pwm_obj[i].name);
                result = -RT_ERROR;
            }
        }
    }

__exit:
    return result;
}
INIT_BOARD_EXPORT(rt_hw_pwm_init);

#endif /* RT_USING_PWM */
