#include <rtthread.h>
#ifdef RT_USING_AT
#ifdef UC8288_MODULE
#include <rtdevice.h>
#include <board.h>
#include <string.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "at.h"
#include "ati_prs.h"
#include "uc_string_lib.h"
#include "uc_adda.h"
#include "uc_gpio.h"
//#include "uc_boot_download.h"
#include "at_wiota.h"
#include "at_wiota_gpio_report.h"

#define WIOTA_NO_DATA       PIN_LOW
#define WIOTA_DATA_ARRIVE   PIN_HIGH
#define WIOTA_DATA_NUM      (5)

#ifndef SYS_TIMER_FLAG_ONE_SHOT
#define SYS_TIMER_FLAG_DEACTIVATED 0x0 /**< timer is deactive */
#define SYS_TIMER_FLAG_ACTIVATED 0x1   /**< timer is active */
#define SYS_TIMER_FLAG_ONE_SHOT 0x0    /**< one shot timer */
#define SYS_TIMER_FLAG_PERIODIC 0x2    /**< periodic timer */
#define SYS_TIMER_FLAG_HARD_TIMER 0x0 /**< hard timer,the timer's callback function will be called in tick isr. */
#define SYS_TIMER_FLAG_SOFT_TIMER 0x4 /**< soft timer,the timer's callback function will be called in timer thread. */
#define SYS_TIMER_CTRL_SET_TIME 0x0     /**< set timer control command */
#define SYS_TIMER_CTRL_GET_TIME 0x1     /**< get timer control command */
#define SYS_TIMER_CTRL_SET_ONESHOT 0x2  /**< change timer to one shot */
#define SYS_TIMER_CTRL_SET_PERIODIC 0x3 /**< change timer to periodic */
#define SYS_TIMER_CTRL_GET_STATE 0x4    /**< get timer run state active or deactive*/
#else
#include "adp_time.h"
#endif

static int wiota_gpio_mode = WIOTA_MODE_OUT_UART;
static int wiota_gpio_state = WIOTA_NO_DATA;

static int s_store_pin = GPIO_PIN_2;
static int s_store_width = 50;
static rt_timer_t wiota_gpio_timer = RT_NULL;
static  uc_recv_back_t wiota_save_data[WIOTA_DATA_NUM];
static int wiota_save_index = -1;

static int s_wake_out_pin = GPIO_PIN_7;
static int s_wake_out_width = 50;
static rt_timer_t s_wake_out_timer = RT_NULL;

////////////////////////////////////////////////////////////////////////////////

static void wiota_gpio_report_state(void)
{
    if (RT_NULL == wiota_gpio_timer)
    {
        return;
    }
    // ctrl timer
    int state = 0;
    rt_timer_control(wiota_gpio_timer, RT_TIMER_CTRL_GET_STATE, &state);
    if (RT_TIMER_FLAG_DEACTIVATED == state)
    {
        s_store_pin = uc_wiota_get_store_pin();
        rt_pin_write(s_store_pin, WIOTA_DATA_ARRIVE);
        wiota_gpio_state = WIOTA_DATA_ARRIVE;
        rt_timer_start(wiota_gpio_timer);
    }
}

static void wiota_gpio_timer_cb(void* ptr)
{
    int *state = (int *)ptr;
    if (RT_NULL != state)
    {
        s_store_pin = uc_wiota_get_store_pin();
        rt_pin_write(s_store_pin, WIOTA_NO_DATA);
        *state = WIOTA_NO_DATA;
    }
    return ;
}

int at_wiota_gpio_report_init(void)
{
    int pin = uc_wiota_get_store_pin();
    int width = uc_wiota_get_store_width();
    if ((pin >= 0) && (pin <= 18))
    {
        s_store_pin = pin;
    }
    if ((width >= 1) && (width <= 255))
    {
        s_store_width = width;
        rt_pin_mode(s_store_pin, PIN_MODE_OUTPUT);
        rt_pin_write(s_store_pin, WIOTA_NO_DATA);
    }    

    wiota_gpio_mode = WIOTA_MODE_OUT_UART;
    wiota_gpio_state = WIOTA_NO_DATA;
    wiota_save_index = -1;
    rt_memset(wiota_save_data, 0, WIOTA_DATA_NUM * sizeof(uc_recv_back_t));

    // create timer
    wiota_gpio_timer = rt_timer_create("gpio_s", wiota_gpio_timer_cb, &wiota_gpio_state, s_store_width, SYS_TIMER_FLAG_ONE_SHOT | SYS_TIMER_FLAG_SOFT_TIMER);
    if (RT_NULL == wiota_gpio_timer)
    {
        // judge time
        return -1;
    }
    // rt_timer_start(wiota_gpio_timer);
    return 0;
}
// INIT_APP_EXPORT(at_wiota_gpio_report_init);

void wiota_data_show(void)
{
    int i = wiota_save_index + 1;
    int max = 0;

    if (-1 == wiota_save_index)
    {
        return;
    }
    else if (i >= WIOTA_DATA_NUM)
    {
        i = 0;
        max = WIOTA_DATA_NUM;
    }
    else
    {
        max = i + WIOTA_DATA_NUM;
    }

    for (; i < max; i++)
    {
        if ((RT_NULL != wiota_save_data[i % WIOTA_DATA_NUM].data) && (wiota_save_data[i % WIOTA_DATA_NUM].data_len > 0))
        {
            at_server_printf("+WIOTARECV,%d,%d,", wiota_save_data[i % WIOTA_DATA_NUM].type, wiota_save_data[i % WIOTA_DATA_NUM].data_len);
            at_send_data(wiota_save_data[i % WIOTA_DATA_NUM].data, wiota_save_data[i % WIOTA_DATA_NUM].data_len);
            at_server_printfln("");
            rt_free(wiota_save_data[i % WIOTA_DATA_NUM].data);
        }
        wiota_save_data[i % WIOTA_DATA_NUM].data = RT_NULL;
        wiota_save_data[i % WIOTA_DATA_NUM].data_len = 0;
    }
    wiota_save_index = -1;
}

static void wake_out_timer_cb(void *ptr)
{
    int pin = *(int *)ptr;
    rt_pin_write(pin, PIN_LOW);
}

int wake_out_pulse_init(void)
{
    int pin = uc_wiota_get_wake_out_pin();
    int width = uc_wiota_get_wake_out_width();
    if ((pin >= 0) && (pin <= 18))
    {
        s_wake_out_pin = pin;
    }
    if ((width >= 1) && (width <= 255))
    {
        s_wake_out_width = width;
    }
    s_wake_out_timer = rt_timer_create("wake_o", wake_out_timer_cb, 
                                       &s_wake_out_pin, 
                                       s_wake_out_width, 
                                       SYS_TIMER_FLAG_ONE_SHOT | SYS_TIMER_FLAG_SOFT_TIMER);
    if (RT_NULL == s_wake_out_timer)
    {
        return -1;
    }
    if (width)
    {
        rt_pin_mode(s_wake_out_pin, PIN_MODE_OUTPUT);
        rt_pin_write(s_wake_out_pin, PIN_HIGH);
        rt_timer_start(s_wake_out_timer);
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

static at_result_t at_pulse_setup(const char *args)
{
    int value = 0;
    int pin = 0;
    at_result_t ret = AT_RESULT_FAILE;

    args = parse((char *)(++args), "d,d", &pin, &value);
    do 
    {
        if ((pin < 0) || (pin > 18))
        {
            break;
        }
        if (0 == value)
        {
            return AT_RESULT_OK;
        }
        if ((value < WIOTA_DATA_NUM) || (value > 255) || (RT_NULL == wiota_gpio_timer))
        {
            break;
        }
        ret = AT_RESULT_OK;
    } while (0);

    if (AT_RESULT_OK == ret)
    {
        rt_pin_mode(s_store_pin, PIN_MODE_INPUT);
        uc_wiota_set_store_pin(pin);
        s_store_pin = pin;
        rt_pin_mode(s_store_pin, PIN_MODE_OUTPUT);

        s_store_width = value;
        uc_wiota_set_store_width(s_store_width);
        rt_timer_control(wiota_gpio_timer, RT_TIMER_CTRL_SET_TIME, &s_store_width);
    }
    return ret;
}

static at_result_t at_pulse_query(void)
{
    at_server_printfln("+PIN=%d", s_store_pin);
    at_server_printfln("+PULSEWIDTH=%d", s_store_width);
    return AT_RESULT_OK;
}

static at_result_t at_store_data_query(void)
{
    wiota_data_show();
    return AT_RESULT_OK;
}

static at_result_t at_mode_setup(const char *args)
{
    int value = 0;

    args = parse((char *)(++args), "d", &value);
    wiota_gpio_mode_set(value);

    return AT_RESULT_OK;
}

static at_result_t at_mode_query(void)
{
    at_server_printfln("+MODE=%d", wiota_gpio_mode_get());

    return AT_RESULT_OK;
}

static at_result_t at_wake_out_pin_setup(const char *args)
{
    int pin = -1;
    int width = -1;
    int state = 0;
    at_result_t ret = AT_RESULT_FAILE;
    
    args = parse((char *)(++args), "d,d", &pin, &width);
    do 
    {
        if ((pin < 0) || (pin > 18))
        {
            break;
        }
        if (!width)
        {
            rt_timer_stop(s_wake_out_timer);
            return AT_RESULT_OK;
        }
        if ((width < 1) || (width > 255))
        {
            break;
        }
        ret = AT_RESULT_OK;
    } while (0);

    if (AT_RESULT_OK == ret)
    {
        if (pin != s_wake_out_pin)
        {
            s_wake_out_pin = pin;
            uc_wiota_set_wake_out_pin(pin);
        }
        if (width != s_wake_out_width)
        {
            s_wake_out_width = width;
            uc_wiota_set_wake_out_width(width);
            rt_timer_control(s_wake_out_timer, RT_TIMER_CTRL_GET_STATE, &state);
            if (RT_TIMER_FLAG_DEACTIVATED == state)
            {
                rt_timer_start(s_wake_out_timer);
            }
        }
    }
    return ret;
}

static at_result_t at_wake_out_pin_query(void)
{
    at_server_printfln("+PIN=%d", s_wake_out_pin);
    at_server_printfln("+WIDTH=%d", s_wake_out_width);
    return AT_RESULT_OK;
}

////////////////////////////////////////////////////////////////////////////////

AT_CMD_EXPORT("AT+MODE", "=<mode>", RT_NULL, at_mode_query, at_mode_setup, RT_NULL);
AT_CMD_EXPORT("AT+STOREDATA", RT_NULL, RT_NULL, at_store_data_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+PULSEWIDTH", "=<pin>,=<width>", RT_NULL, at_pulse_query, at_pulse_setup, RT_NULL);
AT_CMD_EXPORT("AT+WAKEOUTPIN", "=<pin>,<width>", RT_NULL, at_wake_out_pin_query, at_wake_out_pin_setup, RT_NULL);

////////////////////////////////////////////////////////////////////////////////
// ============================ extern functions ============================ //
////////////////////////////////////////////////////////////////////////////////

int wiota_gpio_mode_get(void)
{
    return wiota_gpio_mode;
}

void wiota_gpio_mode_set(int mode)
{
    wiota_gpio_mode = mode & 1;
}

void wiota_data_insert(uc_recv_back_t *data)
{
    if ((RT_NULL == data) || (RT_NULL == data->data)) return;
    if ((wiota_save_index + 1) >= (int)WIOTA_DATA_NUM)
    {
        wiota_save_index = -1;
    }
    wiota_save_index += 1;
    if (RT_NULL != wiota_save_data[wiota_save_index % WIOTA_DATA_NUM].data)
    {
        rt_free(wiota_save_data[wiota_save_index % WIOTA_DATA_NUM].data);
    }
    rt_memcpy(&wiota_save_data[wiota_save_index % WIOTA_DATA_NUM], data, sizeof(uc_recv_back_t));
    wiota_gpio_report_state();
}

////////////////////////////////////////////////////////////////////////////////

#endif  // ~#ifdef UC8288_MODULE
#endif  // ~#ifdef RT_USING_AT
