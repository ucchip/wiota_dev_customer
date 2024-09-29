#ifndef __DRV_FLASH_H__
#define __DRV_FLASH_H__

#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include "board.h"

int uc8x88_flash_read(rt_uint32_t offset, rt_uint8_t* buf, rt_uint32_t size);
int uc8x88_flash_write(rt_uint32_t offset, const rt_uint8_t* buf, rt_uint32_t size);
int uc8x88_flash_erase(rt_uint32_t offset, rt_uint32_t size);

#endif
