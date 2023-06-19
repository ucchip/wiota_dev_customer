#ifndef __UC_SPI_FLASH_H_
#define __UC_SPI_FLASH_H_

//#include "Type.h"
//#include "AddrShareMemory.h"
#include <uc_pulpino.h>

#ifndef __SPI_FLASH_C_
#define EXT_FLASH       extern
#else
#define EXT_FLASH
#endif

#define FLASH_USE_READ

// addr of spi-flash
#define REG_XIP_CTRL            *((volatile uint32_t *)0x1A10C02C)
#define REG_SPI_STATUS          *((volatile uint32_t *)0x1A10C000)
#define REG_SPI_CMD             *((volatile uint32_t *)0x1A10C008)
#define REG_SPI_ADDR            *((volatile uint32_t *)0x1A10C00C)
#define REG_SPI_LEN             *((volatile uint32_t *)0x1A10C010)
#define REG_SPI_DUMMY           *((volatile uint32_t *)0x1A10C014)
#define REG_SPI_FIFO_TX         *((volatile uint32_t *)0x1A10C018)
#define REG_SPI_FIFO_RX         *((volatile uint32_t *)0x1A10C020)

#define FLASH_PAGE_BIT_SHIFT    (8)
#define FLASH_SECTOR_BIT_SHIFT  (12)
#define FLASH_BLOCK_BIT_SHIFT   (16)

#define FLASH_PAGE_MASK         (0xFF)

#define FLASH_PAGE_SIZE         (256)       // Unit:Byte
#define FLASH_SECTOR_SIZE       (4096)      // Unit:Byte

#define FLASH_PAGE_OF_SECTOR    (16)        // Unit:page
#define FLASH_PAGE_OF_BLOCK     (256)       // Unit:page

#define FLASH_SECTOR_OF_BLOCK   (16)        // Unit:Sector
#define FLASH_SECTOR_OF_CHIP    (128)       // Unit:Sector

#define FLASH_BLOCK_OF_CHIP     (32)        // Unit:Block

typedef enum
{
    FLASH_CMD_ID = 0x9F,
    FLASH_CMD_ERASE_SECTOR = 0x20,
    FLASH_CMD_ERASE_BLOCK = 0xD8,
    FLASH_CMD_STATUS = 0x05,
    FLASH_CMD_ENABLE_WR = 0x06,
    FLASH_CMD_QREAD = 0x11101011,
    FLASH_CMD_READ = 0x03,
    FLASH_CMD_PAGE_PROGRAM = 0x02,
    FLASH_CMD_WRITE_STATUS = 0x01,
    FLASH_CMD_ERASE_SECURITY = 0x44,
    FLASH_CMD_PROGRAM_SECURITY = 0x42,
    FLASH_CMD_READ_SECURITY = 0x48,
} ENUM_FLASH_CMD;

// function
//EXT_FLASH uint32_t ReadFlashID();
EXT_FLASH void FlashEraseSector(uint32_t nBaseAddr);
#if 1
EXT_FLASH void FlashWrite(uint32_t nAddr, const uint8_t* pData, uint16_t usLen);
#endif
#ifdef FLASH_USE_READ
EXT_FLASH void FlashRead(uint32_t nAddr, uint8_t* pData, uint16_t usLen);
#else
EXT_FLASH void FlashQRead(uint32_t nAddr, uint8_t* pData, uint16_t usLen);
#endif
EXT_FLASH uint8_t FlashCrc(const uint8_t* pData, uint16_t usLen);

//#ifdef __SPI_FLASH_C_
void FlashEnableWr(void);
void FlashWaitForWr(void);
uint8_t FlashStatus(void);
void FlashPageProgram(uint32_t nAddr, const uint8_t* pData, uint16_t usLen);
void FlashPageRead(uint32_t nAddr, uint8_t* pData, uint16_t usLen);

uint32_t Flash_Read_SR();
void Flash_Write_SR(uint8_t status);

void FlashEraseSecurity(void);
void FlashWriteSecurity(uint32_t rigister_num, uint32_t nAddr, const uint8_t* pData, uint16_t usLen);
void FlashProgramSecurity(uint32_t nAddr, const uint8_t* pData, uint16_t usLen);
void FlashReadSecurity(uint32_t rigister_num, uint32_t nAddr, uint8_t* pData, uint16_t usLen);

//#endif

#endif
