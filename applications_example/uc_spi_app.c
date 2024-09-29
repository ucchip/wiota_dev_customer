#include <rtthread.h>

#ifdef APP_EXAMPLE_SPI

#ifndef RT_USING_SPI
#error "Please enable rt-thread spi device driver"
#endif

#ifndef BSP_USING_SPI
#error "Please enable on-chip peripheral spi config"
#endif

#include <rtdevice.h>
#include <drv_spi.h>
#include "uc_spi_app.h"

/*FLASH常用命令*/
#define FLASH_WriteEnable 0x06
#define FLASH_WriteDisable 0x04
#define FLASH_ReadStatusReg 0x05
#define FLASH_WriteStatusReg 0x01
#define FLASH_ReadData 0x03
#define FLASH_FastReadData 0x0B
#define FLASH_FastReadDual 0x3B
#define FLASH_PageProgram 0x02
#define FLASH_BlockErase 0xD8
#define FLASH_SectorErase 0x20
#define FLASH_ChipErase 0xC7
#define FLASH_PowerDown 0xB9
#define FLASH_ReleasePowerDown 0xAB
#define FLASH_DeviceID 0xAB
#define FLASH_ManufactDeviceID 0x90
#define FLASH_JedecDeviceID 0x9F

#ifdef BSP_USING_SOFT_SPI_PIN_GROUP
#define SPI_BUS_NAME "sspi0"
#define SPI_DEVICE_NAME "sspi00"
#else
#define SPI_BUS_NAME "spi0"
#define SPI_DEVICE_NAME "spi00"
#endif

#define SPI_BUS_CLOCK (1 * 1000 * 1000)
#define SPI_CS_PIN (GPIO_PIN_16)
#define SPI_EN_PIN (GPIO_PIN_4)

static struct rt_spi_device *spi_dev;

static int spi_flash_init(void)
{
    rt_pin_mode(SPI_EN_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(SPI_EN_PIN, PIN_HIGH);

    return RT_EOK;
}

static int spi_flash_write_enable(void)
{
    rt_err_t ret;
    rt_uint8_t flash_write_cmd = FLASH_WriteEnable;

    ret = rt_spi_send(spi_dev, &flash_write_cmd, 1);
    if (ret == 0)
        return -RT_ERROR;

    return RT_EOK;
}

static int spi_flash_write_disable(void)
{
    rt_err_t ret;
    rt_uint8_t flash_write_cmd = FLASH_WriteDisable;

    ret = rt_spi_send(spi_dev, &flash_write_cmd, 1);
    if (ret == 0)
        return -RT_ERROR;

    return RT_EOK;
}

static int spi_flash_read_status(rt_uint8_t *status)
{
    rt_err_t ret;
    rt_uint8_t flash_read_cmd = FLASH_ReadStatusReg;

    ret = rt_spi_send_then_recv(spi_dev, &flash_read_cmd, 1, status, 1);

    return ret;
}

static int spi_flash_wait_busy(rt_uint32_t timeout)
{
    rt_uint8_t status = 0;
    rt_uint8_t busy_bit = 0x01;

    do
    {
        if (spi_flash_read_status(&status) != RT_EOK)
        {
            return -RT_ERROR;
        }

        busy_bit = status & 0x01;

        if (timeout == 0)
        {
            return -RT_ETIMEOUT;
        }
        rt_thread_mdelay(1);
        timeout -= 1;
    } while (busy_bit);

    return RT_EOK;
}

static int spi_flash_read_id(rt_uint32_t *id)
{
    rt_err_t ret;
    rt_uint8_t flash_id_cmd = FLASH_JedecDeviceID;
    rt_uint8_t flash_id[3] = {0};

    ret = rt_spi_send_then_recv(spi_dev, &flash_id_cmd, 1, flash_id, 3);

    *id = (flash_id[0] << 16) | (flash_id[1] << 8) | (flash_id[2]);

    ret = spi_flash_wait_busy(1000);

    return ret;
}

static int spi_flash_sector_erase(rt_uint32_t addr)
{
    rt_err_t ret;
    rt_uint8_t flash_erase_cmd = FLASH_SectorErase;
    rt_uint8_t flash_addr[4] = {0};

    flash_addr[0] = flash_erase_cmd;
    flash_addr[1] = (addr >> 16) & 0xff;
    flash_addr[2] = (addr >> 8) & 0xff;
    flash_addr[3] = addr & 0xff;

    ret = spi_flash_write_enable();
    if (ret != RT_EOK)
    {
        return ret;
    }

    ret = rt_spi_send(spi_dev, flash_addr, 4);
    if (ret == 0)
    {
        return -RT_ERROR;
    }

    ret = spi_flash_wait_busy(10000);

    return ret;
}

static int spi_flash_block_erase(rt_uint32_t addr)
{
    rt_err_t ret;
    rt_uint8_t flash_erase_cmd = FLASH_BlockErase;
    rt_uint8_t flash_addr[4] = {0};

    flash_addr[0] = flash_erase_cmd;
    flash_addr[1] = (addr >> 16) & 0xff;
    flash_addr[2] = (addr >> 8) & 0xff;
    flash_addr[3] = addr & 0xff;

    ret = spi_flash_write_enable();
    if (ret != RT_EOK)
    {
        return ret;
    }

    ret = rt_spi_send(spi_dev, flash_addr, 4);
    if (ret == 0)
    {
        return -RT_ERROR;
    }

    ret = spi_flash_wait_busy(10000);

    return ret;
}

static int spi_flash_chip_erase(void)
{
    rt_err_t ret;
    rt_uint8_t flash_erase_cmd = FLASH_ChipErase;

    ret = spi_flash_write_enable();
    if (ret != RT_EOK)
    {
        rt_kprintf("chip erase write enable failed!\n");
        return ret;
    }

    ret = rt_spi_send(spi_dev, &flash_erase_cmd, 1);
    if (ret == 0)
    {
        rt_kprintf("chip erase send failed!\n");
        return -RT_ERROR;
    }

    ret = spi_flash_wait_busy(40000);

    return ret;
}

static int spi_flash_page_write(rt_uint32_t addr, const rt_uint8_t *data, rt_uint16_t len)
{
    rt_err_t ret;
    rt_uint8_t flash_write_cmd = FLASH_PageProgram;
    rt_uint8_t flash_addr[4] = {0};

    if (len > 256)
    {
        rt_kprintf("page size is 256 bytes, but len is larger than 256, write %d bytes.\n", len);
        return -RT_ERROR;
    }

    flash_addr[0] = flash_write_cmd;
    flash_addr[1] = (addr >> 16) & 0xff;
    flash_addr[2] = (addr >> 8) & 0xff;
    flash_addr[3] = addr & 0xff;

    ret = spi_flash_write_enable();
    if (ret != RT_EOK)
    {
        return ret;
    }

    ret = rt_spi_send_then_send(spi_dev, flash_addr, 4, data, len);
    if (ret != RT_EOK)
    {
        return ret;
    }

    ret = spi_flash_wait_busy(5000);

    return ret;
}

static int spi_flash_write(rt_uint32_t addr, const rt_uint8_t *data, rt_uint32_t len)
{
    rt_err_t ret;
    rt_uint32_t write_addr = addr;
    rt_uint16_t write_len = 0;

    while (len)
    {
        if (len > 256)
            write_len = 256;
        else
            write_len = len;

        ret = spi_flash_page_write(write_addr, data, write_len);
        if (ret != RT_EOK)
        {
            return ret;
        }

        len -= write_len;
        data += write_len;
        write_addr += write_len;
    }

    return RT_EOK;
}

static int spi_flash_read(rt_uint32_t addr, rt_uint8_t *data, rt_uint32_t len)
{
    rt_err_t ret;
    rt_uint8_t flash_read_cmd = FLASH_ReadData;
    rt_uint8_t flash_addr[4] = {0};

    flash_addr[0] = flash_read_cmd;
    flash_addr[1] = (addr >> 16) & 0xff;
    flash_addr[2] = (addr >> 8) & 0xff;
    flash_addr[3] = addr & 0xff;

    ret = rt_spi_send_then_recv(spi_dev, flash_addr, 4, data, len);

    return ret;
}

static int spi_flash_write_read_test(void)
{
    rt_uint32_t addr = 0x00000000;
    rt_uint32_t flash_id = 0;
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

    rt_kprintf("read flash id start...\n");
    if (spi_flash_read_id(&flash_id) != RT_EOK)
    {
        rt_kprintf("read flash ID failed!\n");
        goto __err_exit;
    }
    rt_kprintf("flash id: 0x%06x\n", flash_id);

    rt_kprintf("%08x sector erase start...\n", addr);
    if (spi_flash_sector_erase(addr) != RT_EOK)
    {
        rt_kprintf("sector erase failed!\n");
        goto __err_exit;
    }

    rt_kprintf("%08x write start...\n", addr);
    if (spi_flash_write(addr, write_data, test_data_len) != RT_EOK)
    {
        rt_kprintf("write flash failed!\n");
        goto __err_exit;
    }

    rt_kprintf("%08x read start...\n", addr);
    if (spi_flash_read(addr, read_data, test_data_len) != RT_EOK)
    {
        rt_kprintf("read flash failed!\n");
        goto __err_exit;
    }

    rt_kprintf("write data: 0x%02x, read data: 0x%02x\n", write_value, read_data[test_data_len - 1]);
    if (rt_memcmp(write_data, read_data, test_data_len) != 0)
    {
        goto __err_exit;
    }

    return RT_EOK;

__err_exit:
    rt_free(write_data);
    rt_free(read_data);
    return -RT_ERROR;
}

int spi_app_sample(void)
{
    rt_err_t ret = RT_EOK;
    struct rt_spi_configuration cfg;

    rt_kprintf("spi_app_sample\n");

    rt_hw_spi_device_attach(SPI_BUS_NAME, SPI_DEVICE_NAME, SPI_CS_PIN);

    spi_dev = (struct rt_spi_device *)rt_device_find(SPI_DEVICE_NAME);
    if (spi_dev == RT_NULL)
    {
        rt_kprintf("spi device:%s not found!\n", SPI_DEVICE_NAME);
        return -RT_ERROR;
    }

    cfg.data_width = 8;
    cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
    cfg.max_hz = SPI_BUS_CLOCK;

    ret = rt_spi_configure(spi_dev, &cfg);
    if (ret != RT_EOK)
    {
        rt_kprintf("spi configure failed!\n");
        return -RT_ERROR;
    }

    spi_flash_init();
    rt_thread_mdelay(100);

    if (spi_flash_write_read_test() != RT_EOK)
    {
        rt_kprintf("write/read flash test failed!\n");
        return -RT_ERROR;
    }
    else
    {
        rt_kprintf("write/read flash test success!\n");
    }

    return RT_EOK;
}

#endif // APP_EXAMPLE_SPI
