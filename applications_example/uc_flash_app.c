#include <rtthread.h>

#ifdef APP_EXAMPLE_FLASH

#ifndef BSP_USING_ON_CHIP_FLASH
#error "Please enable on-chip peripheral flash config"
#endif

#include <rtdevice.h>
#include "uc_flash_app.h"

#if defined(RT_USING_FAL)

#include <fal.h>

static int on_chip_flash_test(void)
{
    int ret = 0;
    const struct fal_partition *part;
    const char *part_name = "download";
    rt_uint8_t write_value = 0xA5;
    rt_uint32_t test_data_len = 1024;
    rt_uint8_t *write_data = rt_malloc(test_data_len);
    rt_uint8_t *read_data = rt_malloc(test_data_len);

    RT_ASSERT(write_data != RT_NULL);
    RT_ASSERT(read_data != RT_NULL);

    srand(rt_tick_get());
    write_value = (rand() & 0xFF) + 1;

    rt_memset(write_data, write_value, test_data_len);
    rt_memset(read_data, 0, test_data_len);

    fal_init();

    rt_kprintf("fal partition ( %s ) find start...\n", part_name);
    part = fal_partition_find(part_name);
    if (part == RT_NULL)
    {
        rt_kprintf("fal partition ( %s ) find failed!\n", part_name);
        goto __err_exit;
    }

    rt_kprintf("fal partition erase start...\n");
    ret = fal_partition_erase_all(part);
    if (ret < 0)
    {
        rt_kprintf("fal partition erase failed!\n");
        goto __err_exit;
    }
    rt_kprintf("fal partition erase %d success!\n", ret + 1);

    rt_kprintf("fal partition write start...\n");
    ret = fal_partition_write(part, 0, write_data, test_data_len);
    if (ret < 0)
    {
        rt_kprintf("fal partition write failed!\n");
        goto __err_exit;
    }

    rt_kprintf("fal partition read start...\n");
    ret = fal_partition_read(part, 0, read_data, test_data_len);
    if (ret < 0)
    {
        rt_kprintf("fal partition read failed!\n");
        goto __err_exit;
    }

    rt_kprintf("write data: 0x%02x, read data: 0x%02x\n", write_value, read_data[test_data_len - 1]);
    if (rt_memcmp(write_data, read_data, test_data_len) != 0)
    {
        rt_kprintf("write/read flash test failed!\n");
        goto __err_exit;
    }
    else
    {
        rt_kprintf("write/read flash test success!\n");
    }

    return RT_EOK;

__err_exit:
    rt_free(write_data);
    rt_free(read_data);
    return -RT_ERROR;
}

#else

#include <drv_flash.h>

static int on_chip_flash_test(void)
{
    rt_uint32_t addr = (500 * 1024);
    rt_uint8_t write_value = 0xA5;
    rt_uint32_t test_data_len = 1024;
    rt_uint8_t *write_data = rt_malloc(test_data_len);
    rt_uint8_t *read_data = rt_malloc(test_data_len);

    RT_ASSERT(write_data != RT_NULL);
    RT_ASSERT(read_data != RT_NULL);

    srand(rt_tick_get());
    write_value = (rand() & 0xFF) + 1;

    rt_memset(write_data, write_value, test_data_len);
    rt_memset(read_data, 0, test_data_len);

    rt_kprintf("0x%08x flash erase start...\n", addr);
    if (uc8x88_flash_erase(addr, test_data_len) != RT_EOK)
    {
        rt_kprintf("flash erase failed!\n");
        goto __err_exit;
    }

    rt_kprintf("0x%08x write start...\n", addr);
    if (uc8x88_flash_write(addr, write_data, test_data_len) != test_data_len)
    {
        rt_kprintf("write flash failed!\n");
        goto __err_exit;
    }

    rt_kprintf("0x%08x read start...\n", addr);
    if (uc8x88_flash_read(addr, read_data, test_data_len) != test_data_len)
    {
        rt_kprintf("read flash failed!\n");
        goto __err_exit;
    }

    rt_kprintf("write data: 0x%02x, read data: 0x%02x\n", write_value, read_data[test_data_len - 1]);
    if (rt_memcmp(write_data, read_data, test_data_len) != 0)
    {
        rt_kprintf("write/read flash test failed!\n");
        goto __err_exit;
    }
    else
    {
        rt_kprintf("write/read flash test success!\n");
    }

    return RT_EOK;

__err_exit:
    rt_free(write_data);
    rt_free(read_data);
    return -RT_ERROR;
}

#endif

int flash_app_sample(void)
{
    rt_err_t ret = RT_EOK;

    rt_kprintf("flash_app_sample\n");

    ret = on_chip_flash_test();
    if (ret != RT_EOK)
    {
        rt_kprintf("on-chip flash test failed!\n");
        return -RT_ERROR;
    }

    return RT_EOK;
}

#endif