#include "uc_utils.h"
#include "uc_int.h"
#include "uc_string_lib.h"
#include "uc_uart.h"
#include "uc_event.h"
#include "uc_sectdefs.h"


//defining all interrupt handelrs
//these functions can be redefined by users
// 0: rtc
__attribute__ ((weak))
void ISR_RTC (void) { for (;;); }
// 2: pulse 32k compare
__attribute__ ((weak))
void ISR_PLS_CNT_CMP (void) { for (;;); }
// 3: udma
__attribute__ ((weak))
void ISR_UDMA (void) { for (;;); }
// 5: count 32k compare
__attribute__ ((weak))
void ISR_CNT_32K_CMP(void) { for (;;); }
// 20:  ADC water mark
__attribute__ ((weak))
void ISR_ADC (void) { for (;;); }

// 21: DAC water mark
__attribute__ ((weak))
void ISR_DAC (void) { for (;;); }

// 22: i2c
__attribute__ ((weak))
void ISR_I2C (void) { for (;;); }

// 23: uart0
__attribute__ ((weak))
void ISR_UART0 (void) { for (;;); }

// 24: uart1
__attribute__ ((weak))
void ISR_UART1 (void) { for (;;); }

// 25: gpio
__attribute__ ((weak))
void ISR_GPIO (void) { for (;;); }

// 26: spim end of transmission
__attribute__ ((weak))
void ISR_SPIM0 (void) { for (;;); }

// 27: spim R/T finished
__attribute__ ((weak))
void ISR_SPIM1 (void) { for (;;); }

// 28: timer A overflow
__attribute__ ((weak))
void ISR_TA_OVF (void) { for (;;); }

// 29: timer A compare
__attribute__ ((weak))
void ISR_TA_CMP (void) { for (;;); }

// 30: timer B overflow
__attribute__ ((weak))
void ISR_TB_OVF (void) { for (;;); }

// 31: timer B compare
__attribute__ ((weak))
void ISR_TB_CMP (void) { for (;;); }


/* this is different then global int_enable*/
/* just clear IER to disable all device specific irq*/
__crt0
void irq_disable()
{
    IER = 0x0;
    ICP = 0xFFFFFFFF;
    __asm__ ("csrw mie, x0");
    __asm__ ("csrw mip, x0");
}

__crt0 void enable_event_iqr(int event_id)
{
    IER |= (1 << event_id);
}
__crt0 void disable_event_iqr(int event_id)
{
    IER &= (~(1 << event_id));
}