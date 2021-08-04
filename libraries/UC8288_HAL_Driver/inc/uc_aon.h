// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

/**
 * @file
 * @brief Event library for PULPino.
 *
 * Contains event manipulating functions and event related
 * registers.
 *
 * @author Florian Zaruba
 *
 * @version 1.0
 *
 * @date 2/10/2015
 *
 */

#ifndef __AON_H__
#define __AON_H__

#include <uc_pulpino.h>

#define REG_SLEEP_CTRL          0x20
#define REG_SLEEP_STATUS        0x24


/* UCCHIP SPECIFIC Registers */

/* TODO ucmoded IRQ registers 0x100 offset  */

/* PMU Registers */
#define REG_PMU_CTRL            0x000
#define REG_PMU_DCDC_LDO_CTRL   0x004
#define REG_PMU_CLKPLL_CTRL     0x008
#define REG_PMU_CLKPLL_CTRL0    0x00C
#define REG_PMU_CLKPLL_CTRL1    0x010
#define REG_PMU_CLKPLL_CTRL2    0x014
#define REG_PMU_CLKPLL_CTRL3    0x018
#define REG_PMU_DCXO_CTRL0      0x01C
#define REG_PMU_DCXO_CTRL1      0x020



/* UCCHIP RTC Registers */
#define REG_RTC_CTRL            0x100
#define REG_RTC_TIME0           0x104
#define REG_RTC_TIME1           0x108
#define REG_RTC_TSET0           0x10C
#define REG_RTC_TSET1           0x110
#define REG_RTC_ASET0           0x114
#define REG_RTC_ASET1           0x118
#define REG_RTC_ACTRL           0x11C

// pointer to mem of aon - PointerAon
#define __PAE__(a) *(volatile int*) (PMU_BASE_ADDR + a)

// sleep control register
#define SCR __PAE__(REG_SLEEP_CTRL)

// sleep status register
#define SSR __PAE__(REG_SLEEP_STATUS)


//UCCHIP PMU Registers

/* PMU Registers */
#define PM_CTRL __PAE__(REG_PMU_CTRL         )
#define PM_DLC  __PAE__(REG_PMU_DCDC_LDO_CTRL)
#define PM_CPC  __PAE__(REG_PMU_CLKPLL_CTRL  )
#define PM_CPC0 __PAE__(REG_PMU_CLKPLL_CTRL0 )
#define PM_CPC1 __PAE__(REG_PMU_CLKPLL_CTRL1 )
#define PM_CPC2 __PAE__(REG_PMU_CLKPLL_CTRL2 )
#define PM_CPC3 __PAE__(REG_PMU_CLKPLL_CTRL3 )
#define PM_DXC0 __PAE__(REG_PMU_DCXO_CTRL0   )
#define PM_DXC1 __PAE__(REG_PMU_DCXO_CTRL1   )


/* RTC Registers */
//#define RTC_CTRL  __PAE__(REG_RTC_CTRL  )
//#define RTC_TIM0  __PAE__(REG_RTC_TIME0 )
//#define RTC_TIM1  __PAE__(REG_RTC_TIME1 )
//#define RTC_TS0   __PAE__(REG_RTC_TSET0 )
//#define RTC_TS1   __PAE__(REG_RTC_TSET1 )
//#define RTC_AS0   __PAE__(REG_RTC_ASET0 )
//#define RTC_AS1   __PAE__(REG_RTC_ASET1 )
//#define RTC_ACTRL __PAE__(REG_RTC_ACTRL )

#endif
