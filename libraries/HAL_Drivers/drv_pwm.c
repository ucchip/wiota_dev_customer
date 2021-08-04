
#include <board.h>
#include<rtthread.h>
#include<rtdevice.h>

#ifdef RT_USING_PWM
//#include "drv_config.h"

//#define DRV_DEBUG
#define LOG_TAG             "drv.pwm"
#include <drv_log.h>

#include "uc_pwm.h"
#include "uc_gpio.h"

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
};

#ifdef BSP_USING_PWM0
#ifndef PWM0_CONFIG
#define PWM0_CONFIG                             \
    {                                           \
        .pwm_handle     = UC_PWM0,         \
                          .name                    = "pwm0",       \
    }
#endif /* PWM0_CONFIG */
#endif /* BSP_USING_PWM0 */

#ifdef BSP_USING_PWM1
#ifndef PWM1_CONFIG
#define PWM1_CONFIG                             \
    {                                           \
        .pwm_handle     = UC_PWM1,         \
                          .name                    = "pwm1",       \
    }
#endif /* PWM1_CONFIG */
#endif /* BSP_USING_PWM1 */

#ifdef BSP_USING_PWM2
#ifndef PWM2_CONFIG
#define PWM2_CONFIG                             \
    {                                           \
        .pwm_handle     = UC_PWM2,         \
                          .name                    = "pwm2",       \
    }
#endif /* PWM2_CONFIG */
#endif /* BSP_USING_PWM2 */

#ifdef BSP_USING_PWM3
#ifndef PWM3_CONFIG
#define PWM3_CONFIG                             \
    {                                           \
        .pwm_handle     = UC_PWM3,         \
                          .name                    = "pwm3",       \
    }
#endif /* PWM3_CONFIG */
#endif /* BSP_USING_PWM3 */

struct uc8x88_pwm
{
    struct rt_device_pwm pwm_device;
    PWM_TypeDef* pwm_handle;
    rt_uint8_t channel;
    char* name;
};

static struct uc8x88_pwm uc8x88_pwm_obj[] =
{
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

static rt_err_t drv_pwm_control(struct rt_device_pwm* device, int cmd, void* arg);
static struct rt_pwm_ops drv_ops =
{
    drv_pwm_control
};

static rt_err_t drv_pwm_enable(PWM_TypeDef* hpwm, struct rt_pwm_configuration* configuration, rt_bool_t enable)
{
    if (enable)
    {
        pwm_enable(hpwm);
    }
    else
    {
        pwm_disable(hpwm);
    }

    return RT_EOK;
}

static rt_err_t drv_pwm_get(PWM_TypeDef* hpwm, struct rt_pwm_configuration* configuration)
{
    uint32_t pwm_count = 0;
    uint32_t pwm_duty = 0;

    pwm_count = pwm_get_period(hpwm);
    pwm_duty = pwm_get_duty(hpwm);
    configuration->period = pwm_count;
    configuration->pulse = pwm_duty;

    return RT_EOK;
}

static rt_err_t drv_pwm_set(PWM_TypeDef* hpwm, struct rt_pwm_configuration* configuration)
{
    uint32_t pwm_count = 0;
    uint32_t pwm_duty = 0;

    pwm_count = configuration->period;
    pwm_duty = configuration->pulse;
    pwm_set_period(hpwm, pwm_count);
    pwm_set_duty(hpwm, pwm_duty);

    return RT_EOK;
}

static rt_err_t drv_pwm_control(struct rt_device_pwm* device, int cmd, void* arg)
{
    struct rt_pwm_configuration* configuration = (struct rt_pwm_configuration*)arg;
    PWM_TypeDef* hpwm = (PWM_TypeDef*)device->parent.user_data;

    switch (cmd)
    {
        case PWM_CMD_ENABLE:
            return drv_pwm_enable(hpwm, configuration, RT_TRUE);
        case PWM_CMD_DISABLE:
            return drv_pwm_enable(hpwm, configuration, RT_FALSE);
        case PWM_CMD_SET:
            return drv_pwm_set(hpwm, configuration);
        case PWM_CMD_GET:
            return drv_pwm_get(hpwm, configuration);
        default:
            return RT_EINVAL;
    }
}

static rt_err_t uc8x88_hw_pwm_init(struct uc8x88_pwm* device)
{
    rt_err_t result = RT_EOK;
    PWM_TypeDef* pwm = RT_NULL;

    RT_ASSERT(device != RT_NULL);

    pwm = (PWM_TypeDef*)device->pwm_handle;

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

static int uc8x88_pwm_init(void)
{
    int i = 0;
    int result = RT_EOK;

    for (i = 0; i < sizeof(uc8x88_pwm_obj) / sizeof(uc8x88_pwm_obj[0]); i++)
    {
        /* pwm init */
        if (uc8x88_hw_pwm_init(&uc8x88_pwm_obj[i]) != RT_EOK)
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
INIT_DEVICE_EXPORT(uc8x88_pwm_init);
#endif /* RT_USING_PWM */

