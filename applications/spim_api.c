#include <string.h>
#include <stdint.h>
#include <stdbool.h>
// #include "uc_spi_flash.h"
#include <drivers/spi.h>
#include "spim_api.h"
#include "rtdef.h"
#include "board.h"

// #include "sectdefs.h"

#define SPIM_CMD_RD 0
#define SPIM_CSN0 0

#define WAIT_SPIM_IDLE while (SPIM_STATUS != 1)

/// start
#define SPIM_START(cmd) (SPIM_STATUS = (1 << (SPIM_CSN0 + 8)) | (1 << (cmd)))

#define ALIGN_4B(x) ((x + 3) & (~3))

#define FLASH_PAGE_SIZE (256)
#define FLASH_SECTOR_SIZE (4096)
// except 1 btyte command and 3 bytes address
#define MAX_SPIM_DATA_LEN (28)
// TX FIFO length is 32 bytes
#define MAX_SPIM_WRITE_LEN (32)
// write int array length
#define MAX_SPIM_WRITE_INT (MAX_SPIM_WRITE_LEN >> 2)

typedef enum _flash_cmd
{
    CMD_WR_ENABLE           = 0x06,
    CMD_WR_DISABLE          = 0x04,
    CMD_RD_STATUS           = 0x05,
    CMD_WR_STATUS           = 0x01,
    CMD_READ                = 0x03, ///< no Dummy
    CMD_READ_FAST           = 0x0B, ///< 1B Dummy
    CMD_WR_PAGE             = 0x02, ///< Write 1 page(256B)

    CMD_ERASE_SECTOR_4KB    = 0x20,
    CMD_ERASE_BLOCK_32KB    = 0x52,
    CMD_ERASE_BLOCK_64KB    = 0xD8,
    CMD_ERASE_CHIP          = 0x60, ///< or 0xC7

    CMD_READ_DEV_ID         = 0x90, ///< 3B 3Byte Address
    CMD_READ_ID             = 0x9F, ///< 3B
    CMD_READ_UID            = 0x4B, ///< 4B Dummy 16B

}FLASH_CMD;

typedef enum _flash_write_state
{
    WRITE_STATE_WIP     = 1,
    WRITE_STATE_WEL     = 2,
    WRITE_STATE_BP0     = 4,
    WRITE_STATE_BP1     = 8,
    WRITE_STATE_BP2     = 16,
    WRITE_STATE_S5      = 32,
    WRITE_STATE_LB      = 64,
    WRITE_STATE_SRP0    = 128
} FLASH_WRITE_STATE;

typedef union _spi_cmd_addr
{
    uint32_t u32Val[2];
    uint8_t  u8Val[8];
}spi_cmd_addr;

static spi_cmd_addr s_cmdAddr;
static uint32_t s_nData;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

struct rt_spi_device* spim_api_init(char *name, int gpio_pin, unsigned int clk)
{
    rt_hw_spi_device_attach("spim", "spim0", NULL, gpio_pin);
    struct rt_spi_device* spi_dev = (struct rt_spi_device*)rt_device_find("spim0");
    if (spi_dev == RT_NULL)
    {
        rt_kprintf("can't find %s device!", "spim0");
        return RT_NULL;
    }
    else
    {
        // 配置SPI
        struct rt_spi_configuration cfg;
        cfg.data_width = 8;
        cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
        // 6M can catch by logic analyzer. Maybe can set up to 20M, should be test in future.
        cfg.max_hz = clk;       
        rt_err_t err = rt_spi_configure(spi_dev, &cfg);
        if (err != RT_EOK)
        {
            rt_kprintf("rt_err_t == %d\n", err);
        }
        // demo
        // uint32_t nFlashId = get_flash_id(spi_dev);
        // uint32_t nId = get_cpu_id(spi_dev);
        // rt_kprintf("cpu id = 0x%08X\tflash id = 0x%08X\n", nId, nFlashId);

    }
    return spi_dev;
}

uint32_t get_cpu_id(struct rt_spi_device *dev)
{
    uint32_t nId = -1;
    if (!spim_api_set_mode(dev, SPIM_MODE_MEM))
    {
        spim_api_mem_read(dev, &nId, 4, UC_CPU_ID_REG);
    }
    return nId;
}

uint32_t get_flash_id(struct rt_spi_device *dev)
{
    uint32_t nId = -1;
    if (!spim_api_set_mode(dev, SPIM_MODE_FLASH))
    {
        rt_kprintf("switch flash mode\n");
        flash_read(dev, &nId, 3, CMD_READ_ID, 0, 0);
        nId &= 0x00FFFFFF;
    }
    return nId;
}

void flash_erase(struct rt_spi_device *dev, int nStartAddr, int nEraseSize)
{
    const int STEP_64KB = 64 * 1024;
    const int STEP_32KB = 32 * 1024;
    const int STEP_4KB  =  4 * 1024;
    int nWrCnt = 0;
    int nCmd = 0;
    if (0 == nEraseSize)
    {
        return;
    }
    if (flash_running(dev, WRITE_STATE_WIP | WRITE_STATE_WEL))
    {
        return;
    }
    if (-1 == nEraseSize)
    {
        flash_write(dev, CMD_ERASE_CHIP, 0, 0, RT_NULL, 0);
    }
    else
    {
        nEraseSize = RT_ALIGN(nEraseSize, FLASH_SECTOR_SIZE);
        nStartAddr = ((nStartAddr >> 12) << 12);
        
        while (nWrCnt < nEraseSize)
        {
            if ((nEraseSize - nWrCnt) < STEP_32KB)
            {
                nCmd = CMD_ERASE_SECTOR_4KB;
            }
            else
            {
                if (((nEraseSize - nWrCnt) >= STEP_64KB)
                        && (((nStartAddr + nWrCnt) & (STEP_64KB - 1)) == 0))
                {
                    nCmd = CMD_ERASE_BLOCK_64KB;
                }
                else if (((nStartAddr + nWrCnt) & (STEP_32KB - 1)) == 0)
                {
                    nCmd = CMD_ERASE_BLOCK_32KB;
                }
                else
                {
                    nCmd = CMD_ERASE_SECTOR_4KB;
                }
            }
            flash_write(dev, nCmd, nStartAddr + nWrCnt, 3, RT_NULL, 0);

            switch (nCmd) {
            case CMD_ERASE_BLOCK_64KB:
                nWrCnt += STEP_64KB;
                break;
            case CMD_ERASE_BLOCK_32KB:
                nWrCnt += STEP_32KB;
                break;
            case CMD_ERASE_SECTOR_4KB:
                nWrCnt += STEP_4KB;
                break;
            }
        }
    }
}

void flash_page_program(struct rt_spi_device *dev,
                        int nStartAddr,
                        const uint8_t *pData,
                        int nDataSize,
                        int nEndian)
{
    int i = 0;
    int nWrSize = 0;
    uint8_t *wData = RT_NULL;
    if ((nStartAddr < 0) || (RT_NULL == pData) || (nDataSize <= 0))
    {
        return;
    }
    nWrSize = FLASH_PAGE_SIZE - (nStartAddr & (FLASH_PAGE_SIZE - 1));
    nWrSize = (nWrSize < nDataSize) ? nWrSize : nDataSize;
    wData = (uint8_t *)pData;

    if (SPIM_BIG != nEndian)
    {
        wData = rt_malloc(nDataSize);
        if (RT_NULL == wData)
        {
            return;
        }
        memcpy(wData, pData, nDataSize);
        ByteSwap(wData, nDataSize);
    }
    for (i = 0; i < nDataSize;)
    {
        flash_write(dev,
                    CMD_WR_PAGE,
                    nStartAddr + i,
                    3,
                    wData + i,
                    nWrSize);
        i += nWrSize;
        nWrSize = ((nDataSize - i) >= FLASH_PAGE_SIZE) ?
                    FLASH_PAGE_SIZE : (nDataSize - i);
    }
    if (SPIM_BIG != nEndian)
    {
        rt_free(wData);
    }
}

void flash_page_read(struct rt_spi_device *dev,
                     void *dataOut,
                     int nLenOut,
                     int nStartAddr,
                     int nEndian)
{
    int nRdSize = 0;
    int i = 0;
    if ((nStartAddr < 0) || (RT_NULL == dataOut) || (nLenOut <= 0))
    {
        return;
    }
    if (flash_running(dev, WRITE_STATE_WIP | WRITE_STATE_WEL))
    {
        return;
    }

    nRdSize = FLASH_PAGE_SIZE - (nStartAddr & (FLASH_PAGE_SIZE - 1));
    nRdSize = (nRdSize < nLenOut) ? nRdSize : nLenOut;
    for (i = 0; i < nLenOut;)
    {
        flash_read(dev, dataOut + i, nRdSize, CMD_READ, nStartAddr + i, 3);
        i += nRdSize;
        nRdSize = ((nLenOut - i) >= FLASH_PAGE_SIZE) ?
                    FLASH_PAGE_SIZE : (nLenOut - i);
    }
    if (SPIM_BIG != nEndian)
    {
        ByteSwap(dataOut, nLenOut);
    }
}

struct rt_spi_message *flash_write(struct rt_spi_device *dev,
                                   uint8_t nCmd,
                                   int nAddr,
                                   int nAddrLen,
                                   void *dataIn,
                                   int nLenIn)
{
    struct rt_spi_message *res = RT_NULL;
    if (!flash_running(dev, WRITE_STATE_WIP))
    {
        spim_api_write(dev, CMD_WR_ENABLE, 0, 0, RT_NULL, 0);
        res = spim_api_write(dev, nCmd, nAddr, nAddrLen, dataIn, nLenIn);
        spim_api_write(dev, CMD_WR_DISABLE, 0, 0, RT_NULL, 0);
        if (flash_running(dev, WRITE_STATE_WIP))
        {
            rt_set_errno(RT_EBUSY);
        }
    }
    return res;
}

struct rt_spi_message *flash_read(struct rt_spi_device *dev,
                                  void *dataOut, 
                                  int nLenOut, 
                                  uint8_t nCmd, 
                                  int nAddr, 
                                  int nAddrLen)
{
    struct rt_spi_message *res = spim_api_read(dev, dataOut, nLenOut, nCmd, nAddr, nAddrLen);
    // ByteSwap(dataOut, nLenOut);
    return res;
}

uint8_t flash_running(struct rt_spi_device *dev, uint8_t nWrStateBit)
{
    uint8_t nRet = true;
    int nCnt = 0;
    uint8_t data = 0;
    do {
        spim_api_read(dev, &data, 1, CMD_RD_STATUS, 0, 0);
        nRet = (data & nWrStateBit);
        nCnt++;
    } while(nRet && (nCnt <= 2000));
    return nRet;
}

struct rt_spi_message *spim_api_mem_write(struct rt_spi_device *dev,
                                          int nAddr,
                                          void *dataIn,
                                          int nLenIn)
{
    // ByteSwap(dataIn, nLenIn);
    return spim_api_write(dev, SPIM_OP_MEM_WRITE, nAddr, 4, dataIn, nLenIn);
}

#if 0
struct rt_spi_message *spim_api_mem_read(struct rt_spi_device *dev, void *dataOut, int nLenOut, int nAddr)
{
    RT_ASSERT(RT_NULL != dev);
    if ((RT_NULL == dataOut) || (nLenOut <= 0))
    {
        rt_set_errno(RT_EEMPTY);
        return RT_NULL;
    }

    struct rt_spi_message msgMOSI, msgDummy, msgMISO;
    uint32_t nDummy = 0;
    uint8_t cmdAddr[5] = {0};
    cmdAddr[0] = SPIM_OP_MEM_READ;
    cmdAddr[1] = (nAddr >> 24) & 0xFF;
    cmdAddr[2] = (nAddr >> 16) & 0xFF;
    cmdAddr[3] = (nAddr >>  8) & 0xFF;
    cmdAddr[4] = nAddr & 0xFF;
    
    // send
    msgMOSI.send_buf = cmdAddr;
    msgMOSI.recv_buf = RT_NULL;
    msgMOSI.length = 5;
    msgMOSI.cs_take = 1;
    msgMOSI.cs_release = 0;
    msgMOSI.next = &msgDummy;
    // dummy
    msgDummy.send_buf = RT_NULL;
    msgDummy.recv_buf = &nDummy;
    msgDummy.length = 4;
    msgDummy.cs_take = 0;
    msgDummy.cs_release = 0;
    msgDummy.next = &msgMISO;
    // recv
    msgMISO.send_buf = RT_NULL;
    msgMISO.recv_buf = dataOut;
    msgMISO.length = nLenOut;
    msgMISO.cs_take = 0;
    msgMISO.cs_release = 1;
    msgMISO.next = RT_NULL;

    rt_set_errno(RT_EOK);
    struct rt_spi_message *res = rt_spi_transfer_message(dev, &msgMOSI);
    // ByteSwap(dataOut, nLenOut);
    return res;
}
#endif

struct rt_spi_message *spim_api_mem_read(struct rt_spi_device *dev, void *dataOut, int nLenOut, int nAddr)
{
    RT_ASSERT(RT_NULL != dev);
    if ((RT_NULL == dataOut) || (nLenOut <= 0))
    {
        rt_set_errno(RT_EEMPTY);
        return RT_NULL;
    }

    struct rt_spi_message msgMOSI, msgDummy, msgMISO;
    uint32_t nDummy = 0;
    rt_memset(&s_cmdAddr, 0, sizeof(spi_cmd_addr));

    s_cmdAddr.u32Val[0] = ((SPIM_OP_MEM_READ << 24) | ((nAddr >> 8) & 0x00FFFFFF));
    s_cmdAddr.u8Val[7] = nAddr & 0xFF;
    
    // send
    msgMOSI.send_buf = &s_cmdAddr;
    msgMOSI.recv_buf = RT_NULL;
    msgMOSI.length = 5;
    msgMOSI.cs_take = 1;
    msgMOSI.cs_release = 0;
    msgMOSI.next = &msgDummy;
    // dummy
    msgDummy.send_buf = RT_NULL;
    msgDummy.recv_buf = &nDummy;
    msgDummy.length = 4;
    msgDummy.cs_take = 0;
    msgDummy.cs_release = 0;
    msgDummy.next = &msgMISO;

    // recv
    msgMISO.recv_buf = dataOut;
    if (nLenOut < 4)
    {
        s_nData = 0;
        msgMISO.recv_buf = &s_nData;
    }
    msgMISO.send_buf = RT_NULL;
    msgMISO.length = nLenOut;
    msgMISO.cs_take = 0;
    msgMISO.cs_release = 1;
    msgMISO.next = RT_NULL;

    rt_set_errno(RT_EOK);
    struct rt_spi_message *res = rt_spi_transfer_message(dev, &msgMOSI);
    if (nLenOut < 4)
    {
        rt_memcpy(dataOut, &s_nData, nLenOut);
    }
    return res;
}

int spim_api_set_mode(struct rt_spi_device *dev, int nMode)
{
    int nRet = -1;
    int nCnt = 0;
    // reset
    // spim_api_write(dev, 0xFF, -1, 4, RT_NULL, 0);
    // can not to get the mode, then can not catch the real mode because of the timing.
    // while (((spim_api_get_mode(dev) & 3) != nMode) && (nCnt < 1000))
    {
        spim_api_write(dev, 0xFF, nMode, 4, RT_NULL, 0);
        nCnt++;
    } 
    if (nCnt > 1000)
    {
        rt_kprintf("spim_set_mode() error\n");
        return nRet;
    }
    else
    {
        if (SPIM_MODE_MEM == nMode)
        {
            uint32_t nData = 0x1F;
            spim_api_write(dev, 0x11, 0, 0, &nData, 1);
        }
        if (rt_get_errno() == RT_EOK)
        {
            nRet = 0;
        }
    }
    return nRet;
}

int spim_api_get_mode(struct rt_spi_device *dev)
{
    int nMode = 0;
    spim_api_read(dev, &nMode, 1, 0xFE, 0, 4);
    rt_kprintf("spim_api_get_mode() = 0x%08X\n", nMode);
    return (nMode & 0xFF);
}

struct rt_spi_message *spim_api_write(struct rt_spi_device *dev, uint8_t nCmd, int nAddr, int nAddrLen, void *dataIn, int nLenIn)
{
    RT_ASSERT(RT_NULL != dev);
    struct rt_spi_message msgCmdAddr, msgData;
    rt_memset(&s_cmdAddr, 0, sizeof(spi_cmd_addr));

    switch (nAddrLen)
    {
        /// TODO: must be test case 1 ~ 2
    case 0:
    case 1:
        s_cmdAddr.u8Val[2] = nAddr & 0xFF;
    case 2:
        s_cmdAddr.u8Val[3] = nCmd;
        s_cmdAddr.u8Val[2] = (nAddr >> 8) & 0xFF;
        s_cmdAddr.u8Val[1] = nAddr & 0xFF;
        break;
    case 3:
    {
        s_cmdAddr.u32Val[0] = ((nCmd << 24) | (nAddr & 0x00FFFFFF));
        break;
    }
    case 4:
    {
        s_cmdAddr.u32Val[0] = ((nCmd << 24) | ((nAddr >> 8) & 0x00FFFFFF));
        s_cmdAddr.u8Val[7] = nAddr & 0xFF;
        break;
    }
    }

    // send command
    msgCmdAddr.send_buf = &s_cmdAddr;
    msgCmdAddr.recv_buf = RT_NULL;
    msgCmdAddr.length = 1 + nAddrLen;
    msgCmdAddr.cs_take = 1;
    msgCmdAddr.cs_release = 0;
    msgCmdAddr.next = &msgData;
    // send data
    if ((RT_NULL == dataIn) || (0 == nLenIn))
    {
        msgCmdAddr.cs_release = 1;
        msgCmdAddr.next = RT_NULL;
    }
    else
    {
        msgData.send_buf = dataIn;
        if (nLenIn < 4)
        {
            s_nData = 0;
            rt_memcpy(&s_nData, dataIn, nLenIn);
            ByteSwap((uint8_t *)&s_nData, 4);
            msgData.send_buf = &s_nData;
        }
        msgData.recv_buf = RT_NULL;
        msgData.length = nLenIn;
        msgData.cs_take = 0;
        msgData.cs_release = 1;
        msgData.next = RT_NULL;
    }

    rt_set_errno(RT_EOK);
    struct rt_spi_message *res = rt_spi_transfer_message(dev, &msgCmdAddr);
    
    return res;
}

struct rt_spi_message *spim_api_read(struct rt_spi_device *dev, void *dataOut, int nLenOut, uint8_t nCmd, int nAddr, int nAddrLen)
{
    RT_ASSERT(RT_NULL != dev);
    if ((RT_NULL == dataOut) || (nLenOut <= 0))
    {
        rt_set_errno(RT_EEMPTY);
        return RT_NULL;
    }

    struct rt_spi_message msgMOSI, msgMISO;
    
    rt_memset(&s_cmdAddr, 0, sizeof(spi_cmd_addr));

    switch (nAddrLen)
    {
        /// TODO: must be test case 1 ~ 2
    case 0:
    case 1:
        s_cmdAddr.u8Val[2] = nAddr & 0xFF;
    case 2:
        s_cmdAddr.u8Val[3] = nCmd;
        s_cmdAddr.u8Val[2] = (nAddr >> 8) & 0xFF;
        s_cmdAddr.u8Val[1] = nAddr & 0xFF;
        break;
    case 3:
    {
        s_cmdAddr.u32Val[0] = ((nCmd << 24) | (nAddr & 0x00FFFFFF));
        break;
    }
    case 4:
    {
        s_cmdAddr.u32Val[0] = ((nCmd << 24) | ((nAddr >> 8) & 0x00FFFFFF));
        s_cmdAddr.u8Val[7] = nAddr & 0xFF;
        break;
    }
    }
    // send
    msgMOSI.send_buf = &s_cmdAddr;
    msgMOSI.recv_buf = RT_NULL;
    msgMOSI.length = 1 + nAddrLen;
    msgMOSI.cs_take = 1;
    msgMOSI.cs_release = 0;
    msgMOSI.next = &msgMISO;
    // recv
    msgMISO.recv_buf = dataOut;
    if (nLenOut < 4)
    {
        s_nData = 0;
        msgMISO.recv_buf = &s_nData;
    }
    msgMISO.send_buf = RT_NULL;
    msgMISO.length = nLenOut;
    msgMISO.cs_take = 0;
    msgMISO.cs_release = 1;
    msgMISO.next = RT_NULL;

    rt_set_errno(RT_EOK);
    struct rt_spi_message *res = rt_spi_transfer_message(dev, &msgMOSI);
    if (nLenOut < 4)
    {
        rt_memcpy(dataOut, &s_nData, nLenOut);
    }
    return res;
}

static void EndianSwap(uint8_t *pData, int startIndex, int length)
{
    int cnt = length >> 1; ///< 除以2
    int start = startIndex;
    int end  = startIndex + length - 1;
    uint8_t tmp = 0;
    for (int i = 0; i < cnt; i++)
    {
        tmp            = pData[start+i];
        pData[start+i] = pData[end-i];
        pData[end-i]   = tmp;
    }
}

void ByteSwap(uint8_t *pData, int nLen)
{
    if (nLen <= 1)
    {
        return;
    }
    for (int i = 0; i < nLen; i += 4)
    {
        EndianSwap(pData, i, (nLen < 4) ? nLen : 4);
    }
}
