/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-17     armink       the first version
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include <rtconfig.h>
#include <board.h>


/* ===================== Flash device Configuration ========================= */
extern const struct fal_flash_dev uc8x88_onchip_flash;

/* flash device table */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &uc8x88_onchip_flash,                                            \
}
/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */
#define FAL_PART_TABLE                                                                      \
{                                                                                           \
    {FAL_PART_MAGIC_WORD,        "bl",     "uc8x88_onchip_flash",         0,   28*1024, 0}, \
    {FAL_PART_MAGIC_WORD,       "app",     "uc8x88_onchip_flash",   28*1024,  272*1024, 0}, \
    {FAL_PART_MAGIC_WORD,   "recover",     "uc8x88_onchip_flash",  300*1024,  112*1024, 0}, \
    {FAL_PART_MAGIC_WORD,  "download",     "uc8x88_onchip_flash",  412*1024,   92*1024, 0}, \
    {FAL_PART_MAGIC_WORD,    "static",     "uc8x88_onchip_flash",  504*1024,    8*1024, 0}, \
}
#endif /* FAL_PART_HAS_TABLE_CFG */

#endif /* _FAL_CFG_H_ */
