#define __UC_SPI_FLASH_C_

//#include "AddrShareMemory.h"
#include "uc_spi_flash.h"
#include "uc_spi.h"

#include <uc_sectdefs.h>
#define __critical __attribute__((section(".crt0")))
#define __critical_64 __attribute__((section(".crt0"), aligned(64)))
#define __critical_128 __attribute__((section(".crt0"), aligned(128)))
#define __critical_512 __attribute__((section(".crt0"), aligned(512)))

#define WAIT_XIP_FREE     while(REG_XIP_CTRL & 0x1)
#define WAIT_FOR_WR_DONE  while(Flash_Read_SR()&0x01)
#define WAIT_SPI_IDLE     while(REG_SPI_STATUS != 1)

#define SPI_START(cmd)  (REG_SPI_STATUS = (1<<(SPI_CSN0+8))|(1<<(cmd))); //start

#ifndef     NULL
#define     NULL         0
#endif

extern uint16_t auto_dummy;
//static uint16_t auto_dummy = 6;

__critical_512 uint32_t Flash_Read_SR()
{
    uint32_t data;
    REG_SPI_CMD = FLASH_CMD_STATUS << 24; //read sr
    REG_SPI_LEN = 0x200008;
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_RD);
    while ((REG_SPI_STATUS & 0xFF0000) == 0);
    data = REG_SPI_FIFO_RX;
    return data;
}

#if 0
__critical uint32_t ReadFlashID()
{
    uint32_t data;
    REG_SPI_CMD = FLASH_CMD_ID << 24;   // set cmd
    REG_SPI_LEN = 0x200008;     // set cmd and data len
    SPI_START(SPI_CMD_RD);
    WAIT_XIP_FREE;
    while ((REG_SPI_STATUS & 0xFF0000) == 0);
    data = REG_SPI_FIFO_RX;
    return data;
}
#endif

__critical void FlashEraseSector(uint32_t nBaseAddr)
{
    WAIT_FOR_WR_DONE;
    //  Flash_Read_SR();
    WAIT_XIP_FREE;
    FlashEnableWr();
    REG_SPI_CMD = FLASH_CMD_ERASE_SECTOR << 24;
    REG_SPI_ADDR = (nBaseAddr << 8);
    REG_SPI_LEN  = 0x1808;
    REG_SPI_DUMMY = 0;
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_RD);
    WAIT_SPI_IDLE;
    WAIT_FOR_WR_DONE;
}

#if 0
__critical void FlashWrite(uint32_t nAddr, const uint8_t* pData, uint16_t usLen)
{
    uint16_t usPage, i, usLenTmp;
    uint32_t nTmp;

    usPage = (usLen & FLASH_PAGE_MASK) > 0 ? (usLen >> FLASH_PAGE_BIT_SHIFT) + 1 : usLen >> FLASH_PAGE_BIT_SHIFT;
    WAIT_FOR_WR_DONE;

    for (i = 0; i < usPage; i++)
    {
        nTmp = i << FLASH_PAGE_BIT_SHIFT;
        usLenTmp = (usLen >= (nTmp + FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : usLen - nTmp);
        FlashEnableWr();
        FlashPageProgram(nAddr + nTmp, pData + nTmp, usLenTmp);
    }
}
#endif

#ifdef FLASH_USE_READ
__critical void FlashRead(uint32_t nAddr, uint8_t* pData, uint16_t usLen)
{
    spi_read_fifo(NULL, 0);
    REG_SPI_DUMMY = 0;
    WAIT_XIP_FREE;
    REG_SPI_CMD = FLASH_CMD_READ << 24; // set cmd
    REG_SPI_ADDR = (nAddr << 8);
    REG_SPI_LEN = 0x1808 | (usLen << 19); // set cmd,addr and data len
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_RD);
    spi_read_fifo((int*) pData, (usLen << 3));
}
#else
__critical void FlashQRead(uint32_t nAddr, uint8_t* pData, uint16_t usLen)
{
    //WAIT_FOR_WR_DONE;
    spi_read_fifo(NULL, 0);
    REG_SPI_DUMMY = auto_dummy;
    WAIT_XIP_FREE;
    REG_SPI_CMD = FLASH_CMD_QREAD;  // set cmd
    REG_SPI_ADDR = (nAddr << 8);
    REG_SPI_LEN = 0x1820 | (usLen << 19); // set cmd,addr and data len
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_QRD);
    spi_read_fifo((int*) pData, (usLen << 3));
}
#endif

uint8_t FlashCrc(const uint8_t* pData, uint16_t usLen)
{
    int16_t i;
    uint8_t ucRes = 0x00;

    for (i = 0; i < usLen; i++)
    { ucRes ^= pData[i]; }

    return ucRes;
}

__critical void FlashPageProgram(uint32_t nAddr, const uint8_t* pData, uint16_t usLen)
{
    WAIT_FOR_WR_DONE;
    WAIT_XIP_FREE;
    spi_write_fifo(NULL, 0);
    WAIT_XIP_FREE;
    REG_SPI_CMD = FLASH_CMD_PAGE_PROGRAM << 24;  //set cmd
    REG_SPI_ADDR = (nAddr << 8);
    /* spi len reg format *
     * bit16: bit15:8  bit7:0
     * DLEN   ADDRLEN  CMDLEN
     */
    REG_SPI_LEN = 0x1808 | (usLen << 19);  //set cmd,addr and data len
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_WR);
    WAIT_XIP_FREE;
    spi_write_fifo((int*)pData, usLen << 3);
    WAIT_SPI_IDLE;
    WAIT_XIP_FREE;
    WAIT_FOR_WR_DONE;
}

__critical void FlashEnableWr(void)
{
    REG_SPI_CMD = FLASH_CMD_ENABLE_WR << 24;    // set cmd
    REG_SPI_LEN = 0x0008;      // set cmd and data len
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_WR);
    while (REG_SPI_STATUS != 1);
}

