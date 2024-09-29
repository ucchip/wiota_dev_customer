
#ifndef __BOARD__
#define __BOARD__

#include <stdint.h>
#include <stdlib.h>
#include "uc_pulpino.h"
#include "uc_event.h"
#include "uc_gpio.h"
#include "uc_uart.h"
#include "uc_i2c.h"
#include "uc_spim.h"
#include "uc_timer.h"
#include "uc_pwm.h"
#include "uc_rtc.h"
#include "uc_watchdog.h"
#include "uc_adda.h"
#include "uc_int.h"
#include "uc_spi_flash.h"
#include "uc_can.h"

/*-------------------------- ROM/RAM CONFIG BEGIN --------------------------*/

#define ROM_START   ((uint32_t)0x00000000)
#define ROM_SIZE    (512 * 1024)
#define ROM_END     ((uint32_t)(ROM_START + ROM_SIZE))

#define RAM_START   (0x00308000)
#define RAM_SIZE    (0x7B00)
#define RAM_END     (RAM_START + RAM_SIZE)

/*-------------------------- CLOCK CONFIG BEGIN --------------------------*/

// #define BSP_CLOCK_SOURCE                  ("HSI")
// #define BSP_CLOCK_SOURCE_FREQ_MHZ         ((int32_t)0)
#define BSP_CLOCK_SYSTEM_FREQ_HZ ((int32_t)SYSTEM_CLK)

/*-------------------------- CLOCK CONFIG END --------------------------*/

extern uint32_t _end;
extern uint32_t _heap_end;
#define HEAP_BEGIN &_end
// #define HEAP_END    &_heap_end
// #define HEAP_END    (HEAP_BEGIN + 0x20000)
#define HEAP_END (RAM_END)

void rt_hw_cpu_reset(void);

#endif
