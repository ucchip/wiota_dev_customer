#include <rtthread.h>
#if defined(WIOTA_APP_DEMO) || defined(GATEWAY_MODE_SUPPORT)
#include <rtdevice.h>
#include <rthw.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "uc_spi_flash.h"
#include "tiny_md5.h"

#define BSWAP_32(x)                                         \
    ((((x)&0xff000000u) >> 24) | (((x)&0x00ff0000u) >> 8) | \
     (((x)&0x0000ff00u) << 8) | (((x)&0x000000ffu) << 24))

int uc_wiota_ota_flash_read(unsigned int offset, unsigned char *buf, unsigned int size)
{
    unsigned int addr = 0x00000000 + offset;
    unsigned int index = 0;

    while (index < size)
    {
        unsigned int read_addr = 0;
        unsigned int read_data = 0;
        unsigned char order_len = 0;
        unsigned char addr_offset = 0;
        //rt_base_t level;
        unsigned char *read_buf = RT_NULL;

        read_buf = (unsigned char *)&read_data;

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

        uc_wiota_flash_read(read_buf, read_addr, 4);

        read_data = BSWAP_32(read_data);
        rt_memcpy(&buf[index], &read_buf[addr_offset], order_len);

        index += order_len;
    }

    return size;
}

static int ota_flash_write(unsigned int offset, const unsigned char *buf, unsigned int size)
{
    rt_err_t result = RT_EOK;
    unsigned int addr = 0x00000000 + offset;
    unsigned int end_addr = addr + size;
    unsigned int index = 0;

    if ((end_addr) > (0x00000000 + (FLASH_SECTOR_SIZE * FLASH_PAGE_SIZE)))
    {
        return -RT_EINVAL;
    }

    if (size < 1)
    {
        return -RT_EINVAL;
        //return size;
    }

    while (index < size)
    {
        unsigned int write_addr = 0;
        unsigned int write_data = 0;
        unsigned char order_len = 0;
        unsigned char addr_offset = 0;
        //rt_base_t level;
        unsigned char *write_buf = RT_NULL;

        write_buf = (unsigned char *)&write_data;

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

        if ((addr_offset > 0) || (order_len < 4))
        {
            uc_wiota_ota_flash_read(write_addr, write_buf, 4);
        }
        rt_memcpy(&write_buf[addr_offset], &buf[index], order_len);
        write_data = BSWAP_32(write_data);

        uc_wiota_flash_write(write_buf, write_addr, 4);

        index += order_len;
    }

    if (result != RT_EOK)
    {
        return result;
    }

    return size;
}

void uc_wiota_ota_flash_write(unsigned char *data_buf, unsigned int flash_addr, unsigned int length)
{
    uc_wiota_suspend_connect();
    ota_flash_write(flash_addr, data_buf, length);
    uc_wiota_recover_connect();
}

static int ota_flash_erase(unsigned int offset, unsigned int size)
{
    rt_err_t result = RT_EOK;
    unsigned int addr = ((0x00000000 + offset) / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE;
    unsigned int index = 0;

    for (index = 0; index < size; index += FLASH_SECTOR_SIZE)
    {
        uc_wiota_flash_erase_4K(addr + index);
    }

    if (result != RT_EOK)
    {
        return result;
    }

    return size;
}

void uc_wiota_ota_flash_erase(unsigned int start_addr, unsigned int erase_size)
{
    unsigned int erase_addr = start_addr;
    unsigned int size = 0;

    if (erase_size == 0)
    {
        rt_kprintf("%s line %d para error\n", __FUNCTION__, __LINE__);
        return;
    }

    erase_addr = (erase_addr / 4096) * 4096;

    uc_wiota_suspend_connect();
    if (erase_addr < start_addr)
    {
        ota_flash_erase(erase_addr, 4096);
        erase_addr += 4096;
        size = start_addr - erase_addr;
    }

    while (size < erase_size)
    {
        ota_flash_erase(erase_addr, 4096);
        erase_addr += 4096;
        size += 4096;
    }
    uc_wiota_recover_connect();
}

int uc_wiota_ota_check_flash_data(unsigned int flash_addr, unsigned int flash_size, char *md5)
{
    int data_offset = 0;
    int rc = 0;
    char calc_md5[33] = {0};
    unsigned int buf_len = 512;
    unsigned char *buffer = RT_NULL;
    unsigned char md5_value[16] = {0};
    tiny_md5_context ctx = {0};

    /* allocate memory */
    buffer = rt_malloc(buf_len);
    RT_ASSERT(buffer);

    /* MD5 context setup */
    tiny_md5_starts(&ctx);
    while (data_offset < flash_size)
    {
        unsigned int read_len = buf_len;
        if ((data_offset + read_len) > flash_size)
        {
            read_len = flash_size - data_offset;
        }
        uc_wiota_suspend_connect();

        if (uc_wiota_ota_flash_read(flash_addr + data_offset, buffer, read_len) != read_len)
        {
            uc_wiota_recover_connect();
            rt_free(buffer);
            return 1;
        }
        uc_wiota_recover_connect();
        tiny_md5_update(&ctx, buffer, read_len);
        data_offset += read_len;
    }
    /* MD5 final digest */
    tiny_md5_finish(&ctx, md5_value);

    /* MD5 verification */
    rt_snprintf(calc_md5, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                md5_value[0], md5_value[1], md5_value[2], md5_value[3],
                md5_value[4], md5_value[5], md5_value[6], md5_value[7],
                md5_value[8], md5_value[9], md5_value[10], md5_value[11],
                md5_value[12], md5_value[13], md5_value[14], md5_value[15]);

    rt_kprintf("calc_md5 %s, md5 %s\n", calc_md5, md5);
    if (0 != rt_memcmp(calc_md5, md5, 32))
    {
        rt_kprintf("error calc_md5:%s\n", calc_md5);
        rc = 2;
    }

    if (RT_NULL != buffer)
    {
        rt_free(buffer);
        buffer = RT_NULL;
    }
    return rc;
}

void uc_wiota_ota_jump_program(unsigned int file_size, unsigned char upgrade_type)
{
    uc_wiota_set_download_file_size(file_size);
    if (upgrade_type == 0)
    {
        set_uboot_mode('d');
    }
    else
    {
        set_uboot_mode('b');
    }
    uc_wiota_exit();
    rt_thread_mdelay(2000);
    rt_hw_interrupt_disable();
    rt_hw_cpu_reset();
    // boot_riscv_reboot();
}
#endif