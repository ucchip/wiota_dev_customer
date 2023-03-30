#include <rtthread.h>
#ifdef _SPI_FLASH_APP_
#include <rtdevice.h>
#include "uc_spi_flash_app.h"


#define FLASH_DATA_ADDR         0x1a0000
#define FLASH_DATA_LEN          1024

int spi_flash_app_init(void)
{
    rt_err_t ret = RT_EOK;

    return ret;
}


void spi_flash_app_sample(void)
{
    rt_err_t ret = RT_EOK;
    rt_uint8_t buf[FLASH_DATA_LEN] = {0};
    rt_uint32_t index = 0;

    for(index=0; index<FLASH_DATA_LEN; index++)
    {
        buf[index] = index;
    }

    ret = spi_flash_app_init();
    if(ret != RT_EOK)
    {
        rt_kprintf("spi flash init failed!\n");
        return;
    }

    ret = onchip_flash_erase(FLASH_DATA_ADDR, FLASH_DATA_LEN);
    if(ret != RT_EOK)
    {
        rt_kprintf("spi erase flash failed!\n");
        return;
    }

    ret = onchip_flash_write(FLASH_DATA_ADDR, buf, FLASH_DATA_LEN);
    if(ret != RT_EOK)
    {
        rt_kprintf("spi write flash failed!\n");
        return;
    }

    ret = onchip_flash_read(FLASH_DATA_ADDR, buf, FLASH_DATA_LEN);
    if(ret != RT_EOK)
    {
        rt_kprintf("spi read flash failed!\n");
        return;
    }

    for(index=0; index<FLASH_DATA_LEN; index++)
    {
        if(buf[index] != index)
        {
            rt_kprintf("data error!\n");
            break;
        }
    }
}


#endif
