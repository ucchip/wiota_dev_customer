#include <rtthread.h>
#ifdef _BUTTON_APP_
#include <rtdevice.h>
#include <rthw.h>
#include "uc_button_app.h"

#define LED_PIN_NUM             2
#define BUTTON_PIN_NUM          16

/* 按键触发全局变量 */
static volatile uint8_t key_pressed;

/* 按键中断处理函数 */
static void key_irq_handler(void *args)
{
    rt_base_t level;
    level = rt_hw_interrupt_disable();
    if (key_pressed == 0)
    {
        /* 设置按键触发 */
        key_pressed = 1;
    }
    rt_hw_interrupt_enable(level);
}

int button_app_init(void)
{
    rt_pin_mode(LED_PIN_NUM, PIN_MODE_OUTPUT);
    rt_pin_write(LED_PIN_NUM, PIN_LOW);

    /* 设置按键中断上升沿触发 */
    rt_pin_mode(BUTTON_PIN_NUM, PIN_MODE_INPUT_PULLUP);
    rt_pin_attach_irq(BUTTON_PIN_NUM, PIN_IRQ_MODE_RISING, key_irq_handler, RT_NULL);

    return RT_EOK;
}

void button_app_sample(void)
{
    rt_err_t ret = RT_EOK;
    
    rt_kprintf("button test demo.\r\n");
    
    ret = button_app_init();
    if(ret != RT_EOK)
    {
        rt_kprintf("button test error.\r\n");
    }
    
    rt_pin_write(LED_PIN_NUM, PIN_LOW);
    
    rt_pin_irq_enable(BUTTON_PIN_NUM, PIN_IRQ_ENABLE);

    while (1)
    {
        if (key_pressed == 1)
        {
            /* 消抖延时 */
            rt_thread_mdelay(20);

            /* 复位触发信号 */
            key_pressed = 0;

            /* 再次判断 */
            if (rt_pin_read(BUTTON_PIN_NUM) == 0)
            {
                /* 未触发，重新循环 */
                continue;
            }

            rt_kprintf("key pressed, change led\r\n");

            if (rt_pin_read(LED_PIN_NUM) == 1)
            {
                rt_pin_write(LED_PIN_NUM, 0);
            }
            else
            {
                rt_pin_write(LED_PIN_NUM, 1);
            }
        }
        rt_thread_delay(50);
    }
}

#endif // _BUTTON_APP_
