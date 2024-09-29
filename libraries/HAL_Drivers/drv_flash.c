#include <drv_flash.h>

#ifdef BSP_USING_ON_CHIP_FLASH

#if defined(RT_USING_FAL)
#include <fal.h>
#endif

// #define DRV_DEBUG
#define LOG_TAG "drv.flash"
#include <drv_log.h>

int uc8x88_flash_read(rt_uint32_t offset, rt_uint8_t *buf, rt_uint32_t size)
{
    rt_uint32_t addr = 0x00000000 + offset;
    rt_base_t level;
    LOG_D("read: addr (0x%x), size %d", (void *)addr, size);

    RT_ASSERT((offset + size) < ROM_SIZE);

    level = rt_hw_interrupt_disable();
    flash_wait_ok();
    FlashRead(0, 0, 0);
    flash_wait_ok();
    FlashRead(addr, buf, size);
    rt_hw_interrupt_enable(level);

    return size;
}

static int uc8x88_flash_page_write(rt_uint32_t addr, const rt_uint8_t *buf, rt_uint32_t size)
{
    rt_uint8_t *write_data = (rt_uint8_t *)buf;
    rt_uint32_t addr_resv;
    rt_uint32_t len_resv;
    rt_uint32_t left_len = 0;
    rt_uint32_t new_addr = 0;

    if (size == 0)
    {
        return -RT_ERROR;
    }

    addr_resv = addr & FLASH_PAGE_MASK; // page size 256 byte
    len_resv = FLASH_PAGE_SIZE - addr_resv;

    if (size <= len_resv)
    {
        FlashWrite(addr, write_data, size);
    }
    else
    {
        FlashWrite(addr, write_data, len_resv);
        left_len = size - len_resv;
        new_addr = addr + len_resv;
        write_data += len_resv;
        while (left_len > FLASH_PAGE_SIZE)
        {
            FlashWrite(new_addr, write_data, FLASH_PAGE_SIZE);
            left_len -= FLASH_PAGE_SIZE;
            new_addr += FLASH_PAGE_SIZE;
            write_data += FLASH_PAGE_SIZE;
        }
        FlashWrite(new_addr, write_data, left_len);
    }

    return RT_EOK;
}

int uc8x88_flash_write(rt_uint32_t offset, const rt_uint8_t *buf, rt_uint32_t size)
{
    rt_uint32_t addr = 0x00000000 + offset;
    rt_base_t level;
    LOG_D("write: addr (0x%x), size %d", (void *)addr, size);

    RT_ASSERT((offset + size) < ROM_SIZE);

    level = rt_hw_interrupt_disable();
    flash_wait_ok();
    uc8x88_flash_page_write(0, 0, 0);
    FlashWrite(0, 0, 0);
    flash_wait_ok();
    uc8x88_flash_page_write(addr, buf, size);
    rt_hw_interrupt_enable(level);

    return size;
}

int uc8x88_flash_erase(rt_uint32_t offset, rt_uint32_t size)
{
    rt_uint32_t addr = ((0x00000000 + offset) / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE;
    rt_uint32_t index = 0;
    rt_base_t level;
    LOG_D("erase: addr (0x%x), size %d", (void *)addr, size);

    RT_ASSERT((offset + size) < ROM_SIZE);

    // disable interrupt
    level = rt_hw_interrupt_disable();
    for (index = 0; index < size; index += FLASH_SECTOR_SIZE)
    {
        flash_wait_ok();
        FlashWrite(0, 0, 0);
        flash_wait_ok();
        FlashEraseSector(addr + index);
    }
    // enable interrupt
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}

#if defined(RT_USING_FAL)

static int fal_flash_read(long offset, rt_rt_uint8_t *buf, size_t size);
static int fal_flash_write(long offset, const rt_rt_uint8_t *buf, size_t size);
static int fal_flash_erase(long offset, size_t size);

const struct fal_flash_dev uc8x88_onchip_flash = {
    .name = "uc8x88_onchip_flash",
    .addr = ROM_START,
    .len = ROM_SIZE,
    .blk_size = FLASH_SECTOR_SIZE,
    .ops = {
        .init = RT_NULL,
        .read = fal_flash_read,
        .write = fal_flash_write,
        .erase = fal_flash_erase,
    },
    .write_gran = 32,
};

static int fal_flash_read(long offset, rt_rt_uint8_t *buf, size_t size)
{
    return uc8x88_flash_read((rt_uint32_t)uc8x88_onchip_flash.addr + offset, buf, size);
}

static int fal_flash_write(long offset, const rt_rt_uint8_t *buf, size_t size)
{
    return uc8x88_flash_write((rt_uint32_t)uc8x88_onchip_flash.addr + offset, buf, size);
}

static int fal_flash_erase(long offset, size_t size)
{
    return uc8x88_flash_erase((rt_uint32_t)uc8x88_onchip_flash.addr + offset, size);
}

#endif

#endif /* BSP_USING_ON_CHIP_FLASH */
