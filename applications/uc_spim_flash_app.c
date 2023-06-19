#include <rtthread.h>
#ifdef _SPIM_FLASH_APP_
#include <rtdevice.h>
#include <stdlib.h>
#include <time.h>
#include "uc_gpio.h"
#include "spim_api.h"

#define SPIM_NAME               "spim0"
#define FLASH_CLK               (6 * 1000 * 1000)
#define FLASH_CS_PIN            (GPIO_PIN_13)
#define FLASH_DATA_ADDR         (0x7D000)
#define FLASH_DATA_LEN          (1024)


struct rt_spi_device* spim_dev = RT_NULL;

static int spim_flash_app_init(void)
{
    spim_dev = spim_api_init(SPIM_NAME, FLASH_CS_PIN, FLASH_CLK);
    if (spim_dev)
    {
        return RT_EOK;
    }
    else
    {
        return RT_ERROR;
    }
}

void spim_flash_app_sample(void)
{
    rt_err_t ret = RT_EOK;
    rt_uint8_t *wr_buf = (rt_uint8_t *)rt_calloc(1, FLASH_DATA_LEN);
    rt_uint8_t *rd_buf = (rt_uint8_t *)rt_malloc(FLASH_DATA_LEN);
    rt_int32_t index = 0;

    rt_kprintf("enter spim_flash_app_sample()\r\n");
    srand(time(0));
    for (index = 0; index < FLASH_DATA_LEN; index++)
    {
        wr_buf[index] = rand() & 0xFF;
    }

    do
    {
        ret = spim_flash_app_init();
        if (ret != RT_EOK)
        {
            rt_kprintf("spim flash init failed!\n");
            break;
        }

        flash_erase(spim_dev, FLASH_DATA_ADDR, FLASH_DATA_LEN);
        flash_page_program(spim_dev, FLASH_DATA_ADDR, wr_buf, FLASH_DATA_LEN, SPIM_BIG);
        flash_page_read(spim_dev, rd_buf, FLASH_DATA_LEN, FLASH_DATA_ADDR, SPIM_BIG);

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