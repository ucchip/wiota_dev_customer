
#ifndef __BOARD__
#define __BOARD__

#include <stdint.h>
#include "uc_pulpino.h"

#include "drv_onchip_flash.h"
#include "drv_gpio.h"
#include "drv_spi.h"

/*------- uart driver config -------*/
#define BSP_USING_UART0
//#define BSP_USING_UART1

/*------- i2c driver config -------*/
//#define BSP_USING_I2C1
//#define BSP_I2C1_SCL_PIN    GET_PIN(A, 5)//"A5"
//#define BSP_I2C1_SDA_PIN    GET_PIN(A, 6)//"A6"

/*------- spi master driver config -------*/
//#define BSP_USING_SOFTWARE_SPIM
//#define BSP_SPIM_MOSI_PIN    GET_PIN(A, 3)//"A3"
//#define BSP_SPIM_MISO_PIN    GET_PIN(A, 2)//"A2"
//#define BSP_SPIM_SCK_PIN     GET_PIN(A, 0)//"A0"
//#define BSP_SPIM_TX_USING_DMA
//#define BSP_SPIM_RX_USING_DMA

/*------- pwm driver config -------*/
//#define BSP_USING_PWM0
//#define BSP_USING_PWM1
//#define BSP_USING_PWM2
//#define BSP_USING_PWM3

/*------- timer driver config -------*/
//#define BSP_USING_TIM1

/*-------------------------- ROM/RAM CONFIG BEGIN --------------------------*/

#define ROM_START              ((uint32_t)0x00000000)
#define ROM_SIZE               (512 * 1024)
#define ROM_END                ((uint32_t)(ROM_START + ROM_SIZE))

#define RAM_START               (0x00308000)
#define RAM_SIZE               (0x6900)
#define RAM_END                (RAM_START + RAM_SIZE)

/*-------------------------- CLOCK CONFIG BEGIN --------------------------*/

//#define BSP_CLOCK_SOURCE                  ("HSI")
//#define BSP_CLOCK_SOURCE_FREQ_MHZ         ((int32_t)0)
#define BSP_CLOCK_SYSTEM_FREQ_HZ         ((int32_t)SYSTEM_CLK)

/*-------------------------- CLOCK CONFIG END --------------------------*/

extern uint32_t _end;
extern uint32_t _heap_end;
#define HEAP_BEGIN  &_end
//#define HEAP_END    &_heap_end
//#define HEAP_END    (HEAP_BEGIN + 0x20000)
#define HEAP_END    (RAM_END)

void rt_hw_cpu_reset(void);

#endif
