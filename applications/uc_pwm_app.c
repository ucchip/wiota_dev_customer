#include <rtthread.h>
#ifdef _PWM_APP_
#include <rtdevice.h>
#include "uc_pwm_app.h"

#define PWM_DEVICE_NAME    "pwm1"
#define PWM_DEV_CHANNEL     1

struct rt_device_pwm *pwm_dev = RT_NULL;
static struct rt_pwm_configuration pwm_cfg;

int pwm_app_init(int channel, rt_uint32_t period, rt_uint32_t pulse)
{
    rt_err_t ret = RT_EOK;

    pwm_dev = (struct rt_device_pwm *)rt_device_find(PWM_DEVICE_NAME);
    if (pwm_dev == RT_NULL)
    {
        rt_kprintf("find %s failed!\n", PWM_DEVICE_NAME);
        return RT_ERROR;
    }
    
    pwm_cfg.channel = channel;
    pwm_cfg.complementary = RT_TRUE;
    pwm_cfg.period = period;
    pwm_cfg.pulse = pulse;
    
    ret = rt_device_control(pwm_dev, PWM_CMD_SET, &pwm_cfg);
    if(ret != RT_EOK)
    {
        rt_kprintf("enable %s failed!\n", PWM_DEVICE_NAME);
    }

    return ret;
}

int pwm_app_disable(void)
{
    rt_err_t ret = RT_EOK;
    
    if(pwm_dev == RT_NULL)
    {
        return RT_ENOMEM;
    }
    
    ret = rt_device_control(pwm_dev, PWM_CMD_DISABLE, RT_NULL);
    if (ret != RT_EOK)
    {
        rt_kprintf("start %s failed!\n", PWM_DEVICE_NAME);
        return -RT_ERROR;
    }
    
    return ret;
}


int pwm_app_enable(void)
{
    rt_err_t ret = RT_EOK;
    
    if(pwm_dev == RT_NULL)
    {
        return RT_ENOMEM;
    }
    
    ret = rt_device_control(pwm_dev, PWM_CMD_ENABLE, RT_NULL);
    if (ret != RT_EOK)
    {
        rt_kprintf("start %s failed!\n", PWM_DEVICE_NAME);
        return -RT_ERROR;
    }

    return ret;
}

void pwm_app_sample(void)
{
    rt_kprintf("pwm test demo.\r\n");
    
    pwm_app_init(PWM_DEV_CHANNEL, 3000, 300);
    pwm_app_enable();
    
}

#endif
