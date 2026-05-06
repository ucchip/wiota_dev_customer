#define __UC_SPI_FLASH_C_

//#include "AddrShareMemory.h"
#include "uc_spi_flash.h"
#include "uc_spi.h"

#include <uc_sectdefs.h>
#define __critical __attribute__((section(".crt0"), noinline))
#define __critical_64 __attribute__((section(".crt0"), aligned(64), noinline))
#define __critical_128 __attribute__((section(".crt0"), aligned(128), noinline))
#define __critical_512 __attribute__((section(".crt0"), aligned(512), noinline))

#define WAIT_XIP_FREE while (REG_XIP_CTRL & 0x1)
#define WAIT_FOR_WR_DONE while (Flash_Read_SR(0) & 0x01)
#define WAIT_SPI_IDLE while (REG_SPI_STATUS != 1)

#define SPI_START(cmd) (REG_SPI_STATUS = (1 << (SPI_CSN0 + 8)) | (1 << (cmd))); //start

#ifndef NULL
#define NULL 0
#endif

#define SPI_DEFAULT_DUMMY (0)

extern uint16_t auto_dummy;
//static uint16_t auto_dummy = 6;

__critical_128 uint32_t Flash_Read_SR(uint8_t is_high)
{
    if (is_high > 1)
    {
        return 0;
    }
    uint32_t data;
    REG_SPI_CMD = (FLASH_CMD_STATUS | 0x30 * is_high) << 24; //read sr
    REG_SPI_LEN = 0x200008;
    REG_SPI_DUMMY = SPI_DEFAULT_DUMMY;
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_RD);
    while ((REG_SPI_STATUS & 0xFF0000) == 0)
        ;
    data = REG_SPI_FIFO_RX;
    return data;
}

__critical_128 uint16_t ReadFlashID(uint8_t flag)
{

    if (flag == 0)
    {
        return 0;
    }
    uint32_t data;
    // REG_SPI_CMD = FLASH_CMD_ID << 24;   // set cmd
    REG_SPI_CMD = FLASH_CMD_DEV_ID << 24; // set cmd
    REG_SPI_LEN = 0x200008;               // set cmd and data len
    REG_SPI_DUMMY = 8;
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_RD);
    WAIT_XIP_FREE;
    while ((REG_SPI_STATUS & 0xFF0000) == 0)
        ;
    data = REG_SPI_FIFO_RX;
    REG_SPI_DUMMY = SPI_DEFAULT_DUMMY;
    return (data & 0xFFFF);
}

__critical_128 void FlashEraseSector(uint32_t nBaseAddr)
{
    WAIT_FOR_WR_DONE;
    WAIT_XIP_FREE;
    FlashEnableWr();
    REG_SPI_CMD = FLASH_CMD_ERASE_SECTOR << 24;
    REG_SPI_ADDR = (nBaseAddr << 8);
    REG_SPI_LEN = 0x1808;
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_WR);
    WAIT_SPI_IDLE;
    WAIT_FOR_WR_DONE;
    FlashDisableWr();
}

__critical_128 void FlashWrite(uint32_t nAddr, const uint8_t *pData, uint16_t usLen)
{
    uint16_t usPage, i, usLenTmp;
    uint32_t nTmp;

    if (usLen == 0)
    {
        return;
    }

    usPage = (usLen & FLASH_PAGE_MASK) > 0 ? (usLen >> FLASH_PAGE_BIT_SHIFT) + 1 : usLen >> FLASH_PAGE_BIT_SHIFT;
    WAIT_FOR_WR_DONE;
    WAIT_XIP_FREE;

    for (i = 0; i < usPage; i++)
    {
        nTmp = i << FLASH_PAGE_BIT_SHIFT;
        usLenTmp = (usLen >= (nTmp + FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : usLen - nTmp);
        FlashEnableWr();
        asm("nop");
        WAIT_XIP_FREE;
        FlashPageProgram(nAddr + nTmp, pData + nTmp, usLenTmp);
    }
    FlashDisableWr();
}

#ifdef FLASH_USE_READ
__critical_128 void FlashRead(uint32_t nAddr, uint8_t *pData, uint16_t usLen)
{
    WAIT_XIP_FREE;
    if (usLen == 0)
    {
        return;
    }
    REG_SPI_CMD = FLASH_CMD_READ << 24; // set cmd
    REG_SPI_ADDR = (nAddr << 8);
    REG_SPI_LEN = 0x1808 | (usLen << 19); // set cmd,addr and data len
    REG_SPI_DUMMY = SPI_DEFAULT_DUMMY;
    spi_prefetch();
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_RD);
    spi_read_fifo((int *)pData, (usLen << 3));
}

#else
__critical_128 void FlashQRead(uint32_t nAddr, uint8_t *pData, uint16_t usLen)
{
    //WAIT_FOR_WR_DONE;
    WAIT_XIP_FREE;
    REG_SPI_DUMMY = auto_dummy;
    REG_SPI_CMD = FLASH_CMD_QREAD; // set cmd
    REG_SPI_ADDR = (nAddr << 8);
    REG_SPI_LEN = 0x1820 | (usLen << 19); // set cmd,addr and data len
    spi_prefetch();
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_QRD);
    spi_read_fifo((int *)pData, (usLen << 3));
    REG_SPI_DUMMY = SPI_DEFAULT_DUMMY;
}
#endif
/*
uint8_t FlashCrc(const uint8_t* pData, uint16_t usLen)
{
    int16_t i;
    uint8_t ucRes = 0x00;

    for (i = 0; i < usLen; i++)
    { ucRes ^= pData[i]; }

    return ucRes;
}
*/

__critical_128 void FlashPageProgram_1(const uint8_t *pData, uint16_t usLen)
{
    WAIT_XIP_FREE;
    spi_prefetch();
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_WR);
    spi_write_fifo((int *)pData, usLen << 3);
    WAIT_SPI_IDLE;
    WAIT_XIP_FREE;
    WAIT_FOR_WR_DONE;
}

__critical_128 void FlashPageProgram(uint32_t nAddr, const uint8_t *pData, uint16_t usLen)
{
    if (usLen == 0)
    {
        return;
    }
    WAIT_FOR_WR_DONE;
    WAIT_XIP_FREE;
    REG_SPI_CMD = FLASH_CMD_PAGE_PROGRAM << 24; //set cmd
    REG_SPI_ADDR = (nAddr << 8);
    /* spi len reg format *
     * bit16: bit15:8  bit7:0
     * DLEN   ADDRLEN  CMDLEN
     */
    REG_SPI_LEN = 0x1808 | (usLen << 19); //set cmd,addr and data len
    REG_SPI_DUMMY = SPI_DEFAULT_DUMMY;
    FlashPageProgram_1(pData, usLen);
}

__critical_128 void FlashEnableWr(void)
{
    REG_SPI_CMD = FLASH_CMD_ENABLE_WR << 24; // set cmd
    REG_SPI_LEN = 0x0008;                    // set cmd and data len
    REG_SPI_DUMMY = SPI_DEFAULT_DUMMY;
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_WR);
    while (REG_SPI_STATUS != 1)
        ;
}
__critical_128 void FlashDisableWr(void)
{
    REG_SPI_CMD = FLASH_CMD_DISABLE_WR << 24; // set cmd
    REG_SPI_LEN = 0x0008;                     // set cmd and data len
    REG_SPI_DUMMY = SPI_DEFAULT_DUMMY;
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_WR);
    while (REG_SPI_STATUS != 1)
        ;
}

// __critical_128 void flash_delay(uint32_t delay_time)
// {
//     int i, j;
//     for (i = 0; i < delay_time; i++)
//     {
//         for (j = 0; j < 40; j++)
//         {
//             asm("nop");
//         }
//     }
// }


__critical_128 void Flash_Write_SR(uint8_t status, uint8_t is_high)
{
    if (is_high > 1)
    {
        return;
    }
    WAIT_FOR_WR_DONE;
    WAIT_XIP_FREE;
    FlashEnableWr();
    // flash_delay(0);
    REG_SPI_CMD = (is_high ? FLASH_CMD_WRITE_STATUS_HIGH : FLASH_CMD_WRITE_STATUS) | status << 16; // set cmd
    REG_SPI_LEN = 0x0010;                                      // set cmd and data len
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_WR);
    WAIT_SPI_IDLE;
    // flash_delay(1000);
    WAIT_FOR_WR_DONE;
    FlashDisableWr();
}

__critical_128 void FlashEraseSecurity(uint8_t mode)
{
    uint32_t nBaseAddr = (mode << 8);

    if (0 == mode)
    {
        return;
    }

    WAIT_FOR_WR_DONE;
    WAIT_XIP_FREE;
    FlashEnableWr();
    REG_SPI_CMD = FLASH_CMD_ERASE_SECURITY << 24;
    REG_SPI_ADDR = (nBaseAddr << 8);
    REG_SPI_LEN = 0x1808;
    REG_SPI_DUMMY = 0;
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_WR);
    WAIT_SPI_IDLE;
    WAIT_FOR_WR_DONE;
    FlashDisableWr();
}

__critical_128 void FlashWriteSecurity(uint32_t rigister_num, uint32_t nAddr, const uint8_t *pData, uint16_t usLen)
{
    if (usLen == 0 || (usLen + nAddr) > 256)
    {
        return;
    }

    nAddr = (rigister_num << 8) | nAddr;

    WAIT_FOR_WR_DONE;
    WAIT_XIP_FREE;

    FlashEnableWr();
    FlashProgramSecurity(nAddr, pData, usLen);
    FlashDisableWr();
}

__critical_128 void FlashProgramSecurity_1(const uint8_t *pData, uint16_t usLen)
{
    WAIT_XIP_FREE;
    spi_prefetch();
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_WR);
    spi_write_fifo((int *)pData, usLen << 3);
    WAIT_SPI_IDLE;
    WAIT_XIP_FREE;
    WAIT_FOR_WR_DONE;
}

__critical_128 void FlashProgramSecurity(uint32_t nAddr, const uint8_t *pData, uint16_t usLen)
{
    WAIT_FOR_WR_DONE;
    WAIT_XIP_FREE;

    REG_SPI_CMD = FLASH_CMD_PROGRAM_SECURITY << 24; //set cmd
    REG_SPI_ADDR = (nAddr << 8);
    /* spi len reg format *
     * bit16: bit15:8  bit7:0
     * DLEN   ADDRLEN  CMDLEN
     */
    REG_SPI_LEN = 0x1808 | (usLen << 19); //set cmd,addr and data len

    FlashProgramSecurity_1(pData, usLen);
}

__critical_128 void FlashReadSecurity_1(uint8_t *pData, uint16_t usLen)
{
    WAIT_XIP_FREE;
    spi_prefetch();
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_RD);
    spi_read_fifo((int *)pData, (usLen << 3));
    REG_SPI_DUMMY = SPI_DEFAULT_DUMMY;
}

__critical_128 void FlashReadSecurity(uint32_t rigister_num, uint32_t nAddr, uint8_t *pData, uint16_t usLen)
{
    if (usLen == 0 || (usLen + nAddr) > 256)
    {
        return;
    }

    nAddr = (rigister_num << 8) | nAddr;
    WAIT_XIP_FREE;
    REG_SPI_DUMMY = 8;
    REG_SPI_CMD = FLASH_CMD_READ_SECURITY << 24; // set cmd
    REG_SPI_ADDR = (nAddr << 8);
    REG_SPI_LEN = 0x1808 | (usLen << 19); // set cmd,addr and data len
    FlashReadSecurity_1(pData, usLen);
}
