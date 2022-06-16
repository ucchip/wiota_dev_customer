#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include <board.h>

#include "switch_ctrl.h"
#include "custom_data.h"
//#include "uc_gpio.h"

#define SWITCH_0_IUTPUT_PIN    17//15
#define SWITCH_1_IUTPUT_PIN    16
#define SWITCH_2_IUTPUT_PIN    15//17

#define SWITCH_0_DOWN_LEVEL     PIN_HIGH
#define SWITCH_1_DOWN_LEVEL     PIN_HIGH
#define SWITCH_2_DOWN_LEVEL     PIN_HIGH

static e_sw_state g_switch_state[SWITCH_COUNT_MAX];
static struct rt_semaphore g_switch_sem[SWITCH_COUNT_MAX];

static void switch_irq_handle(void* args)
{
   if (args != NULL)
   {
       rt_sem_release((rt_sem_t)args);
   }
}

static void switch_update_state(void)
{
    if (rt_pin_read(SWITCH_0_IUTPUT_PIN) == SWITCH_0_DOWN_LEVEL)
    {
        g_switch_state[0] = SW_DOWN;
    }
    else
    {
        g_switch_state[0] = SW_UP;
    }
    if (rt_pin_read(SWITCH_1_IUTPUT_PIN) == SWITCH_1_DOWN_LEVEL)
    {
        g_switch_state[1] = SW_DOWN;
    }
    else
    {
        g_switch_state[1] = SW_UP;
    }
    if (rt_pin_read(SWITCH_2_IUTPUT_PIN) == SWITCH_2_DOWN_LEVEL)
    {
        g_switch_state[2] = SW_DOWN;
    }
    else
    {
        g_switch_state[2] = SW_UP;
    }
}

void switch_ctrl_init(void)
{
    rt_uint32_t irq_mode;

    rt_sem_init(&g_switch_sem[0], "sw_s0", 0, RT_IPC_FLAG_FIFO);
    rt_pin_mode(SWITCH_0_IUTPUT_PIN, PIN_MODE_INPUT);
    if (SWITCH_0_DOWN_LEVEL == PIN_LOW)
    {
        irq_mode = PIN_IRQ_MODE_FALLING;
    }
    else
    {
        irq_mode = PIN_IRQ_MODE_RISING;
    }
    rt_pin_attach_irq(SWITCH_0_IUTPUT_PIN, irq_mode, switch_irq_handle, (void *)&g_switch_sem[0]);
    rt_pin_irq_enable(SWITCH_0_IUTPUT_PIN, RT_TRUE);

    rt_sem_init(&g_switch_sem[1], "sw_s1", 0, RT_IPC_FLAG_FIFO);
    rt_pin_mode(SWITCH_1_IUTPUT_PIN, PIN_MODE_INPUT);
    if (SWITCH_1_DOWN_LEVEL == PIN_LOW)
    {
        irq_mode = PIN_IRQ_MODE_FALLING;
    }
    else
    {
        irq_mode = PIN_IRQ_MODE_RISING;
    }
    rt_pin_attach_irq(SWITCH_1_IUTPUT_PIN, irq_mode, switch_irq_handle, (void *)&g_switch_sem[1]);
    rt_pin_irq_enable(SWITCH_1_IUTPUT_PIN, RT_TRUE);

    rt_sem_init(&g_switch_sem[2], "sw_s2", 0, RT_IPC_FLAG_FIFO);
    rt_pin_mode(SWITCH_2_IUTPUT_PIN, PIN_MODE_INPUT);
    if (SWITCH_2_DOWN_LEVEL == PIN_LOW)
    {
        irq_mode = PIN_IRQ_MODE_FALLING;
    }
    else
    {
        irq_mode = PIN_IRQ_MODE_RISING;
    }
    rt_pin_attach_irq(SWITCH_2_IUTPUT_PIN, irq_mode, switch_irq_handle, (void *)&g_switch_sem[2]);
    rt_pin_irq_enable(SWITCH_2_IUTPUT_PIN, RT_TRUE);

    switch_update_state();
}

unsigned char switch_get_count(void)
{
    unsigned char dev_type = 0;
    unsigned char count = 0;

    custom_get_devinfo(&dev_type, &count);

    return count;
}

int switch_get_state(unsigned char index)
{
    if (index >= switch_get_count())
    {
        return -1;
    }

    switch_update_state();

    return g_switch_state[index];
}

int switch_get_all_state(unsigned char *get_state_mask)
{
    switch_update_state();

    *get_state_mask = 0;
    for (unsigned char index = 0; index < switch_get_count(); index++)
    {
        *get_state_mask |= g_switch_state[index] << index;
    }

    return 0;
}

static unsigned int switch_get_interval_tick(unsigned int last_tick)
{
    unsigned int interval_tick = 0;
    unsigned int cur_tick = rt_tick_get();

    if (cur_tick >= last_tick)
    {
        interval_tick = cur_tick - last_tick;
    }
    else
    {
        interval_tick = 0xffffffff - last_tick + cur_tick + 1;
    }

    return interval_tick;
}

#define SWITCH_DOWN_STATE_HOLD_TIME     10

unsigned char switch_get_down_event_mask(void)
{
    static unsigned char down_start_state = 0;
    static unsigned int down_start_tick[SWITCH_COUNT_MAX];
    unsigned char sw_event_mask = 0;

    for (unsigned int index = 0; index < switch_get_count(); index++)
    {
        if (rt_sem_trytake(&g_switch_sem[index]) == RT_EOK)
        {
            if (switch_get_state(index) == SW_DOWN)
            {
                down_start_state |= (1 << index);
                down_start_tick[index] = rt_tick_get();
            }
        }
        if (down_start_state & (1 << index))
        {
            if (switch_get_interval_tick(down_start_tick[index]) > SWITCH_DOWN_STATE_HOLD_TIME)
            {
                if (switch_get_state(index) == SW_DOWN)
                {
                    sw_event_mask |= (1 << index);
                }
                down_start_state &= ~(1 << index);
            }
        }
    }

    return sw_event_mask;
}

#endif
