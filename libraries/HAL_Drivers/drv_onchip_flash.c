/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-12-5      SummerGift   first version
 * 2020-03-05     redoc        support stm32f103vg
 *
 */

#include <rtthread.h>
#include <rthw.h>
#include "board.h"

#if 1//def BSP_USING_ON_CHIP_FLASH
#include <string.h>
#include "uc_int.h"
#include "uc_spi_flash.h"
//#include "drv_onchip_flash.h"

//#define DRV_DEBUG
#define LOG_TAG             "drv.flash"
#include <drv_log.h>


#define BSWAP_32(x) \
    ((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >>  8) |         \
     (((x) & 0x0000ff00u) <<  8) | (((x) & 0x000000ffu) << 24))

int onchip_flash_read(uint32_t offset, uint8_t* buf, uint32_t size)
{
    uint32_t addr = 0x00000000 + offset;
    uint32_t index = 0;
    //LOG_D("read: addr (0x%x), size %d\r\n", (void*)addr, size);

    while (index < size)
    {
        uint32_t read_addr = 0;
        uint32_t read_data = 0;
        uint8_t order_len = 0;
        uint8_t addr_offset = 0;
        //rt_base_t level;
        uint8_t* read_buf = RT_NULL;

        read_buf = (uint8_t*)&read_data;

        read_addr = ((addr + index) / 4) * 4;
        if ((addr + index) % 4)
        {
            addr_offset = (addr + index) % 4;
        }

        if ((index + 4 - addr_offset) > size)
        {
            order_len = size - index;
        }
        else
        {
            order_len = 4 - addr_offset;
        }
        //LOG_D("read read_addr=0x%08x, index=%d, size=%d, addr_offset=%d, order_len=%d\r\n",
        //    (uint32_t)read_addr, index, size, addr_offset, order_len);

        //level = rt_hw_interrupt_disable();
        int_disable();

#ifdef FLASH_USE_READ
        FlashRead(read_addr, read_buf, 4);
#else
        FlashQRead(read_addr, read_buf, 4);
#endif

        //rt_hw_interrupt_enable(level);
        int_enable();

        read_data = BSWAP_32(read_data);
        memcpy(&buf[index], &read_buf[addr_offset], order_len);

        index += order_len;
    }

    return size;
}

int onchip_flash_write(uint32_t offset, const uint8_t* buf, uint32_t size)
{
    rt_err_t result      = RT_EOK;
    uint32_t addr = 0x00000000 + offset;
    rt_uint32_t end_addr = addr + size;
    uint32_t index = 0;
    //LOG_D("write: addr (0x%x), size %d\r\n", (void*)addr, size);

    if ((end_addr) > (0x00000000 + (FLASH_SECTOR_SIZE * FLASH_SECTOR_OF_CHIP)))
    {
        LOG_D("write outrange flash size! addr is (0x%p)", (void*)(addr + size));
        return -RT_EINVAL;
    }

    if (size < 1)
    {
        LOG_D("write (size < 1)! addr (0x%x), size %d\r\n", (void*)addr, size);
        //return -RT_EINVAL;
        return size;
    }

    while (index < size)
    {
        uint32_t write_addr = 0;
        uint32_t write_data = 0;
        uint8_t order_len = 0;
        uint8_t addr_offset = 0;
        //rt_base_t level;
        uint8_t* write_buf = RT_NULL;

        write_buf = (uint8_t*)&write_data;

        write_addr = ((addr + index) / 4) * 4;
        if ((addr + index) % 4)
        {
            addr_offset = (addr + index) % 4;
        }

        if ((index + 4 - addr_offset) > size)
        {
            order_len = size - index;
        }
        else
        {
            order_len = 4 - addr_offset;
        }
        //LOG_D("write write_addr=0x%08x, index=%d, size=%d, addr_offset=%d, order_len=%d\r\n",
        //    (uint32_t)write_addr, index, size, addr_offset, order_len);

        if ((addr_offset > 0) || (order_len < 4))
        {
            onchip_flash_read(write_addr, write_buf, 4);
        }
        memcpy(&write_buf[addr_offset], &buf[index], order_len);
        write_data = BSWAP_32(write_data);

        //level = rt_hw_interrupt_disable();
        int_disable();
        FlashEnableWr();
        FlashPageProgram(write_addr, (const uint8_t*)write_buf, 4);
        //rt_hw_interrupt_enable(level);
        int_enable();

#if 1
        if (1)
        {
            uint32_t read_back_data = 0;
            uint8_t* read_back_buf = 0;

            read_back_buf = (uint8_t*)&read_back_data;
            onchip_flash_read(write_addr, read_back_buf, 4);
            read_back_data = BSWAP_32(read_back_data);

            if (memcmp(&write_buf[addr_offset], &read_back_buf[addr_offset], order_len) != 0)
            {
                LOG_D("write readback Err\r\n");
                LOG_D("write_data = 0x%08x\r\n", BSWAP_32(write_data));
                LOG_D("read_back_data = 0x%08x\r\n", BSWAP_32(read_back_data));
                result = -RT_ERROR;
                break;
            }
        }
#endif

        index += order_len;
    }

    if (result != RT_EOK)
    {
        LOG_D("write Err result=%d\r\n", result);
        return result;
    }

    return size;
}

int onchip_flash_erase(uint32_t offset, uint32_t size)
{
    rt_err_t result = RT_EOK;
    uint32_t addr = ((0x00000000 + offset) / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE;
    uint32_t index = 0;
    //LOG_D("erase: addr (0x%x), size %d\r\n", (void*)addr, size);

    for (index = 0; index < size; index += FLASH_SECTOR_SIZE)
    {
        //rt_base_t level;

        //level = rt_hw_interrupt_disable();
        int_disable();
        FlashEnableWr();
        FlashEraseSector(addr + index);
        //rt_hw_interrupt_enable(level);
        int_enable();
    }

    if (result != RT_EOK)
    {
        return result;
    }

    return size;
}

#endif /* BSP_USING_ON_CHIP_FLASH */
