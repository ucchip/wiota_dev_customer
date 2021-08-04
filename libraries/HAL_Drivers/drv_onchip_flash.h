/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-12-5      SummerGift   first version
 */

#ifndef __DRV_FLASH_H__
#define __DRV_FLASH_H__

#include <rtthread.h>
#include "rtdevice.h"
#include <rthw.h>

#ifdef __cplusplus
extern "C" {
#endif

int onchip_flash_read(uint32_t offset, uint8_t* buf, uint32_t size);
int onchip_flash_write(uint32_t offset, const uint8_t* buf, uint32_t size);
int onchip_flash_erase(uint32_t offset, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif  /* __DRV_FLASH_H__ */
