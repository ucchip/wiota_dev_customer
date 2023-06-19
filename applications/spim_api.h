
#ifndef _UC_SPIM_API_H
#define _UC_SPIM_API_H

#include <stdint.h>
#include <stdbool.h>

#define FLASH_PAGE_8288 (256)
#define FLASH_SECTOR_8288 (4 * 1024)
// Last 4 Byte for bin's timestamp
#define MAX_DOWN_FILE_8288 (512 * 1024 - FLASH_SECTOR_8288)
#define FLASH_8288_LAST_SECTOR_ADDRESS (512 * 1024 - FLASH_SECTOR_8288)
#define FLASH_8288_TIMESTAMP_ADDRESS (512 * 1024 - 4)
#define FLASH_8288_ID (0x000B6013)

#define UC_CPU_ID_REG (0x1A107018)
#define UC8288_ID (0x55438288)

#if 0
/// big endian and little endian convert
#define BSWAP_32(x) \
    ((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >>  8) |         \
     (((x) & 0x0000ff00u) <<  8) | (((x) & 0x000000ffu) << 24))
#define BSWAP_16(x) \
    ((((x) & 0xff00u) >>  8) | (((x) & 0x00ffu) << 8))
#endif

typedef enum _spim_mode
{
    SPIM_MODE_MEM,
    SPIM_MODE_FLASH,
}SPIM_MODE;

/**
 * @brief SPIM access 8288 memory or reg
 */
typedef enum _spim_op_mem
{
    SPIM_OP_MEM_WRITE   = 0x02,
    SPIM_OP_MEM_READ    = 0x0B,
}SPIM_OP_MEM;

typedef enum _spim_endian
{
    SPIM_BIG,
    SPIM_LITTLE
}SPIM_ENDIAN;

typedef enum _boot_init_8288 {
    INIT_8288_FLASH_OK,
    INIT_8288_REG_OK,
    INIT_8288_ERROR = -1
} BOOT_INIT_8288;


struct rt_spi_device* spim_api_init(char *name, int gpio_pin, unsigned int clk);
uint32_t get_cpu_id(struct rt_spi_device *dev);
uint32_t get_flash_id(struct rt_spi_device *dev);
void flash_erase(struct rt_spi_device *dev, int nStartAddr, int nEraseSize);
void flash_page_program(struct rt_spi_device *dev,
                        int nStartAddr,
                        const uint8_t *pData,
                        int nDataSize,
                        int nEndian);
void flash_page_read(struct rt_spi_device *dev,
                     void *dataOut,
                     int nLenOut,
                     int nStartAddr,
                     int nEndian);

uint8_t flash_running(struct rt_spi_device *dev, uint8_t nWrStateBit);
struct rt_spi_message *flash_write(struct rt_spi_device *dev,
                                   uint8_t nCmd,
                                   int nAddr,
                                   int nAddrLen,
                                   void *dataIn,
                                   int nLenIn);
struct rt_spi_message * flash_read(struct rt_spi_device *dev,
                                   void *dataOut, 
                                   int nLenOut, 
                                   uint8_t nCmd, 
                                   int nAddr, 
                                   int nAddrLen);

/**
 * @brief set spim access mode -> memory or flash
 * @param dev       rt_device_find() return handle
 * @param nMode     define by SPIM_MODE
 */
int spim_api_set_mode(struct rt_spi_device *dev, int nMode);
int spim_api_get_mode(struct rt_spi_device *dev);


struct rt_spi_message * spim_api_mem_read(struct rt_spi_device *dev, void *dataOut, int nLenOut, int nAddr);
struct rt_spi_message *spim_api_mem_write(struct rt_spi_device *dev,
                                          int nAddr,
                                          void *dataIn,
                                          int nLenIn);

struct rt_spi_message *spim_api_write(struct rt_spi_device *dev,
                                      uint8_t nCmd,
                                      int nAddr,
                                      int nAddrLen,
                                      void *dataIn,
                                      int nLenIn);

struct rt_spi_message * spim_api_read(struct rt_spi_device *dev,
                                      void *dataOut, 
                                      int nLenOut, 
                                      uint8_t nCmd, 
                                      int nAddr, 
                                      int nAddrLen);

void ByteSwap(uint8_t *pData, int nLen);

#endif
