#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#ifdef APP_DEMO_LIGHT
#include <rtdevice.h>
#include <board.h>
#include "light_ctrl.h"
//#include "uc_gpio.h"

static e_light_state g_light_state = LIGHT_OFF;

#define LIGHT_OUTPUT_PIN 15
#define LIGHT_ON_LEVEL PIN_HIGH
#define LIGHT_OFF_LEVEL PIN_LOW

#define LIGHT_OUTPUT_ON() rt_pin_write(LIGHT_OUTPUT_PIN, LIGHT_ON_LEVEL)
#define LIGHT_OUTPUT_OFF() rt_pin_write(LIGHT_OUTPUT_PIN, LIGHT_OFF_LEVEL)

void light_ctrl_init(void)
{
    rt_pin_mode(LIGHT_OUTPUT_PIN, PIN_MODE_OUTPUT);

    // default close
    LIGHT_OUTPUT_OFF();
    g_light_state = LIGHT_OFF;
}

void light_ctrl_output(e_light_ctrl ctrl_out)
{
    if (ctrl_out == CTRL_LIGHT_REVERSE)
    {
        if (g_light_state == LIGHT_ON)
        {
            LIGHT_OUTPUT_OFF();
            g_light_state = LIGHT_OFF;
        }
        else
        {
            LIGHT_OUTPUT_ON();
            g_light_state = LIGHT_ON;
        }
    }
    else if (ctrl_out == CTRL_LIGHT_ON)
    {
        LIGHT_OUTPUT_ON();
        g_light_state = LIGHT_ON;
    }
    else
    {
        LIGHT_OUTPUT_OFF();
        g_light_state = LIGHT_OFF;
    }
    rt_kprintf("light_ctrl_output ctrl_out = %d\r\n", ctrl_out);
}

e_light_state light_ctrl_get_state(void)
{
    return g_light_state;
}

#endif
#endif
