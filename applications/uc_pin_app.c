#include <rtthread.h>
#ifdef _PIN_APP_
#include <rtdevice.h>
#include "uc_pin_app.h"

#define LED1_PIN_NUM            2
#define LED2_PIN_NUM            3
#define IRQ1_PIN_NUM            7
#define IRQ2_PIN_NUM            16

static void led2_on(void *args)
{
    rt_pin_write(LED2_PIN_NUM, PIN_HIGH);
}

static void led2_off(void *args)
{
    rt_pin_write(LED2_PIN_NUM, PIN_LOW);
}

int pin_app_init(void)
{
    rt_err_t ret = RT_EOK;
    rt_pin_mode(LED1_PIN_NUM, PIN_MODE_OUTPUT);
    rt_pin_mode(LED2_PIN_NUM, PIN_MODE_OUTPUT);
    
    rt_pin_write(LED1_PIN_NUM, PIN_LOW);
    rt_pin_write(LED2_PIN_NUM, PIN_LOW);
    
    rt_pin_mode(IRQ1_PIN_NUM, PIN_MODE_INPUT_PULLDOWN);
    rt_pin_attach_irq(IRQ1_PIN_NUM, PIN_IRQ_MODE_RISING, led2_on, RT_NULL);
    
    rt_pin_mode(IRQ2_PIN_NUM, PIN_MODE_INPUT_PULLDOWN);
    rt_pin_attach_irq(IRQ2_PIN_NUM, PIN_IRQ_MODE_RISING, led2_off, RT_NULL);
    
    return ret;
}

void pin_app_sample(void)
{
    rt_err_t ret = RT_EOK;
    
    rt_kprintf("pin test demo.\r\n");
    
    ret = pin_app_init();
    if(ret != RT_EOK)
    {
        printf("pin test error.\r\n");
    }
    
    rt_pin_write(LED1_PIN_NUM, PIN_HIGH);
    rt_pin_write(LED2_PIN_NUM, PIN_LOW);
    
    rt_pin_irq_enable(IRQ1_PIN_NUM, PIN_IRQ_ENABLE);
    rt_pin_irq_enable(IRQ2_PIN_NUM, PIN_IRQ_ENABLE);
}


#endif
