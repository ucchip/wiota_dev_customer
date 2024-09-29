#ifndef __EVENT_H__
#define __EVENT_H__

#include <uc_pulpino.h>
#include "uc_aon.h"

#define REG_IRQ_ENABLE          0x000
#define REG_IRQ_PENDING         0x004
#define REG_IRQ_SET_PENDING     0x008
#define REG_IRQ_CLEAR_PENDING   0x00C

#define REG_EVENT_ENABLE        0x100
#define REG_EVENT_PENDING       0x104
#define REG_EVENT_SET_PENDING   0x108
#define REG_EVENT_CLEAR_PENDING 0x10C

#define REG_SLEEP_CTRL          0x20
#define REG_SLEEP_STATUS        0x24


/* UCCHIP SPECIFIC Registers */

/* TODO ucmoded IRQ registers 0x100 offset  */

/* PMU Registers */
//#define REG_PMU_CTRL            0x200
//#define REG_PMU_DCDC_LDO_CTRL   0x204
//#define REG_PMU_CLKPLL_CTRL     0x208
//#define REG_PMU_CLKPLL_CTRL0    0x20C
//#define REG_PMU_CLKPLL_CTRL1    0x210
//#define REG_PMU_CLKPLL_CTRL2    0x214
//#define REG_PMU_CLKPLL_CTRL3    0x218
//#define REG_PMU_DCXO_CTRL0      0x21C
//#define REG_PMU_DCXO_CTRL1      0x220



/* UCCHIP RTC Registers */
//#define REG_RTC_CTRL            0x300
//#define REG_RTC_TIME0           0x304
//#define REG_RTC_TIME1           0x308
//#define REG_RTC_TSET0           0x30C
//#define REG_RTC_TSET1           0x310
//#define REG_RTC_ASET0           0x314
//#define REG_RTC_ASET1           0x318
//#define REG_RTC_ACTRL           0x31C




// pointer to mem of event unit - PointerEventunit
#define __PE__(a) *(volatile int*) (EVENT_UNIT_BASE_ADDR + a)

// interrupt enable register
#define IER __PE__(REG_IRQ_ENABLE)

// interrupt pending register
#define IPR __PE__(REG_IRQ_PENDING)

// interrupt set pending register
#define ISP __PE__(REG_IRQ_SET_PENDING)

// interrupt clear pending register
#define ICP __PE__(REG_IRQ_CLEAR_PENDING)

// event enable register
#define EER __PE__(REG_EVENT_ENABLE)

// event pending register
#define EPR __PE__(REG_EVENT_PENDING)

// event set pending register
#define ESP __PE__(REG_EVENT_SET_PENDING)

// event clear pending register
#define ECP __PE__(REG_EVENT_CLEAR_PENDING)

// sleep control register
//#define SCR __PE__(REG_SLEEP_CTRL)

// sleep status register
//#define SSR __PE__(REG_SLEEP_STATUS)


//UCCHIP PMU Registers

/* PMU Registers */
//#define PM_CTRL __PE__(REG_PMU_CTRL         )
//#define PM_DLC  __PE__(REG_PMU_DCDC_LDO_CTRL)
//#define PM_CPC  __PE__(REG_PMU_CLKPLL_CTRL  )
//#define PM_CPC0 __PE__(REG_PMU_CLKPLL_CTRL0 )
//#define PM_CPC1 __PE__(REG_PMU_CLKPLL_CTRL1 )
//#define PM_CPC2 __PE__(REG_PMU_CLKPLL_CTRL2 )
//#define PM_CPC3 __PE__(REG_PMU_CLKPLL_CTRL3 )
//#define PM_DXC0 __PE__(REG_PMU_DCXO_CTRL0   )
//#define PM_DXC1 __PE__(REG_PMU_DCXO_CTRL1   )


/* RTC Registers */
//#define RTC_CTRL  __PE__(REG_RTC_CTRL  )
//#define RTC_TIM0  __PE__(REG_RTC_TIME0 )
//#define RTC_TIM1  __PE__(REG_RTC_TIME1 )
//#define RTC_TS0   __PE__(REG_RTC_TSET0 )
//#define RTC_TS1   __PE__(REG_RTC_TSET1 )
//#define RTC_AS0   __PE__(REG_RTC_ASET0 )
//#define RTC_AS1   __PE__(REG_RTC_ASET1 )
//#define RTC_ACTRL __PE__(REG_RTC_ACTRL )


// ISRS
#define GPIO_EVENT              25
#define TIMER_A_OVERFLOW        0x1C
#define TIMER_A_OUTPUT_CMP      0x1D
#define TIMER_B_OVERFLOW        0x1E
#define TIMER_B_OUTPUT_CMP      0x1F

enum
{
    RTC_INT_ID      =   0,  // rtc interrupt
    CCE_INT_ID      =   1,  // 1:cce
    I2C_INT_ID      =   22, // 22: i2c
    UART0_INT_ID    =   23, // 23: uart0
    UART1_INT_ID    =   24, // 24: uart1
    GPIO_INT_ID     =   25, // 25: gpio
    SPIMO_INT_ID    =   26,
    SPIM1_INT_ID    =   27,
    TaOver_INT_ID   =   28,
    TaCmp_INT_ID    =   29,
    TbOver_INT_ID   =   30,
    TbCmp_INT_ID    =   31,
};

#endif
