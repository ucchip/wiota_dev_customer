#include <rtconfig.h>
#include "uc_example_app.h"
#include "uc_pin_app.h"
#include "uc_uart_app.h"
#include "uc_i2c_app.h"
#include "uc_spi_app.h"
#include "uc_watchdog_app.h"
#include "uc_pwm_app.h"
#include "uc_rtc_app.h"
#include "uc_adc_app.h"
#include "uc_dac_app.h"
#include "uc_flash_app.h"
#include "uc_hwtimer_app.h"

void uc_peripheral_example(void)
{
#if defined(APP_EXAMPLE_PIN)
    pin_app_sample();
#endif

#if defined(APP_EXAMPLE_UART)
    uart_app_sample();
#endif

#if defined(APP_EXAMPLE_I2C)
    i2c_app_sample();
#endif

#if defined(APP_EXAMPLE_SPI)
    spi_app_sample();
#endif

#if defined(APP_EXAMPLE_WATCHDOG)
    watchdog_app_sample();
#endif

#if defined(APP_EXAMPLE_PWM)
    pwm_app_sample();
#endif

#if defined(APP_EXAMPLE_RTC)
    rtc_app_sample();
#endif

#if defined(APP_EXAMPLE_ADC)
    adc_app_sample();
#endif

#if defined(APP_EXAMPLE_DAC)
    dac_app_sample();
#endif

#if defined(APP_EXAMPLE_FLASH)
    flash_app_sample();
#endif

#if defined(APP_EXAMPLE_HWTIMER)
    hwtimer_app_sample();
#endif
}