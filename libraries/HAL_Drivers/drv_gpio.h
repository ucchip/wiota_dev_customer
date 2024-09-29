#ifndef __DRV_GPIO_H__
#define __DRV_GPIO_H__

#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include "board.h"

#define GET_PIN(PORTx, PIN) (rt_base_t)(PIN)

int rt_hw_pin_init(void);

#endif /* __DRV_GPIO_H__ */
