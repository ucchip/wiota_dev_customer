#include <rtthread.h>
#ifdef _SPI_FLASH_APP_
#include <rtdevice.h>
#include <stdlib.h>
#include <time.h>
#include "uc_spi_flash_app.h"
#include "drv_onchip_flash.h"

#define FLASH_DATA_ADDR (0x7D000)
#define FLASH_DATA_LEN  (1024)

int spi_flash_app_init(void)
{
    rt_err_t ret = RT_EOK;

    return ret;
}

void spi_flash_app_sample(void)
{
    rt_err_t ret = RT_EOK;
    rt_uint8_t *wr_buf = (rt_uint8_t *)rt_calloc(1, FLASH_DATA_LEN);
    rt_uint8_t *rd_buf = (rt_uint8_t *)rt_malloc(FLASH_DATA_LEN);
    rt_int32_t index = 0;

    rt_kprintf("enter spi_flash_app_sample()\r\n");
    srand(time(0));
    for (index = 0; index < FLASH_DATA_LEN; index++)
    {
        wr_buf[index] = rand() & 0xFF;
    }

    do
    {
        ret = spi_flash_app_init();
        if (ret != RT_EOK)
        {
            rt_kprintf("spi flash init failed!\n");
            break;
        }

        ret = onchip_flash_erase(FLASH_DATA_ADDR, FLASH_DATA_LEN);
        if (ret != RT_EOK)
        {
            rt_kprintf("spi erase flash failed!\n");
            break;
        }

        ret = onchip_flash_write(FLASH_DATA_ADDR, wr_buf, FLASH_DATA_LEN);
        if (ret != FLASH_DATA_LEN)
        {
            rt_kprintf("spi write flash failed!\n");
            break;
        }

        ret = onchip_flash_read(FLASH_DATA_ADDR, rd_buf, FLASH_DATA_LEN);
        if (ret != FLASH_DATA_LEN)
        {
            rt_kprintf("spi read flash failed!\n");
            break;
        }

        if (rt_memcmp(wr_buf, rd_buf, FLASH_DATA_LEN))
        {
            rt_kprintf("data diff!\n");
        }
        else
        {
            rt_kprintf("data same\n");
        }
    } while (0);
    
    rt_free(wr_buf);
    rt_free(rd_buf);
}

#endif
