#include <rtthread.h>

#ifdef APP_EXAMPLE_PIN

#ifndef RT_USING_PIN
#error "Please enable rt-thread GPIO device driver"
#endif

#ifndef BSP_USING_GPIO
#error "Please enable on-chip peripheral gpio config"
#endif

#include <rtdevice.h>
#include "uc_pin_app.h"

#define THREAD_STACK_SIZE 512
#define THREAD_PRIORITY 5
#define THREAD_TIMESLICE 5

#define LED1_PIN_NUM 2
#define LED2_PIN_NUM 3
#define IRQ1_PIN_NUM 16
#define IRQ2_PIN_NUM 17

static void led2_on(void *args)
{
    rt_kprintf("key1 pressed, LED2 on.\n");
    rt_pin_write(LED2_PIN_NUM, PIN_HIGH);
}

static void led2_off(void *args)
{
    rt_kprintf("key2 pressed, LED2 off.\n");
    rt_pin_write(LED2_PIN_NUM, PIN_LOW);
}

static int pin_app_init(void)
{
    rt_err_t ret = RT_EOK;
    rt_pin_mode(LED1_PIN_NUM, PIN_MODE_OUTPUT);
    rt_pin_mode(LED2_PIN_NUM, PIN_MODE_OUTPUT);

    rt_pin_write(LED1_PIN_NUM, PIN_LOW);
    rt_pin_write(LED2_PIN_NUM, PIN_LOW);

    rt_pin_mode(IRQ1_PIN_NUM, PIN_MODE_INPUT_PULLUP);
    rt_pin_attach_irq(IRQ1_PIN_NUM, PIN_IRQ_MODE_RISING, led2_on, RT_NULL);
    rt_pin_irq_enable(IRQ1_PIN_NUM, PIN_IRQ_ENABLE);

    rt_pin_mode(IRQ2_PIN_NUM, PIN_MODE_INPUT_PULLUP);
    rt_pin_attach_irq(IRQ2_PIN_NUM, PIN_IRQ_MODE_RISING, led2_off, RT_NULL);
    rt_pin_irq_enable(IRQ2_PIN_NUM, PIN_IRQ_ENABLE);

    return ret;
}

static void pin_thread_entry(void *parameter)
{
    while (1)
    {
        rt_pin_write(LED1_PIN_NUM, PIN_HIGH);
        rt_kprintf("LED1 on.\n");
        rt_thread_mdelay(500);
        rt_pin_write(LED1_PIN_NUM, PIN_LOW);
        rt_kprintf("LED1 off.\n");
        rt_thread_mdelay(500);
    }
}

int pin_app_sample(void)
{
    rt_err_t ret = RT_EOK;
    rt_thread_t thread = RT_NULL;

    rt_kprintf("pin_app_sample\n");

    ret = pin_app_init();
    if (ret != RT_EOK)
    {
        rt_kprintf("pin init error\n");
        return -RT_ERROR;
    }

    /* 创建 serial 线程 */
    thread = rt_thread_create("pin_app",
                              pin_thread_entry,
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

#endif // APP_EXAMPLE_PIN
