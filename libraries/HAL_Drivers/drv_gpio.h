/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author            Notes
 * 2018-11-06     balanceTWK        first version
 * 2020-06-16     thread-liu        add stm32mp1
 * 2020-09-01     thread-liu        add GPIOZ
 * 2020-09-18     geniusgogo        optimization design pin-index algorithm
 */

#ifndef __DRV_GPIO_H__
#define __DRV_GPIO_H__

//#include <drv_common.h>
//#include <board.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GET_PIN(PORTx,PIN) (rt_base_t)(PIN)

int rt_hw_pin_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __DRV_GPIO_H__ */

