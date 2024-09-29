#include <uc_spi.h>
#include <uc_gpio.h>
#include <uc_uart.h>
#include <uc_utils.h>
#include <uc_pulpino.h>
#include <uc_aon.h>
//#include <uc_aon.h>
#include <uc_sectdefs.h>

//#define SPI_FULL_SPEED
//#define SHIFT_RX_EDGE
#define XTX_FLASH


#define __critical     __attribute__((section(".critical"))
#define __critical_64  __attribute__((section(".critical"), aligned(64)))
#define __critical_128 __attribute__((section(".critical"), aligned(128)))
#define __critical_512 __attribute__((section(".critical"), aligned(512)))

// int flash_read_sr();

#define REG_XIP_CMD     0x1A10C030
#define SEG_TBL_SIZE    20
#define REG_MTR_CFG_0   0x1A107040
#define REG_MTR_CFG_1   0x1A107044
#define REG_MTR_CFG_2   0x1A107048
#define REG_MTR_CFG_3   0x1A10704C
#define REG_MTR_CFG_4   0x1A107050
#define REG_MTR_CFG_5   0x1A107054
#define REG_MTR_CFG_6   0x1A107058
#define REG_MTR_CFG_7   0x1A10705C

extern uint32_t _stbl_end ;
extern uint32_t _sdata, edata;
extern uint32_t _rom_start;

uint16_t auto_dummy = 3;

#define reg_xip_ctrl   ((volatile uint32_t *) 0x1a10c02c)
#define reg_xip_cmd    ((volatile uint32_t *) 0x1a10c030)
#define reg_spi_cmd    ((volatile uint32_t *) 0x1a10c008)
#define reg_spi_addr   ((volatile uint32_t *) 0x1a10c00c)
#define reg_spi_len    ((volatile uint32_t *) 0x1a10c010)
#define reg_spi_status ((volatile uint32_t *) 0x1a10c000)
#define reg_spi_rxfifo ((volatile uint32_t *) 0x1a10c020)
#define reg_spi_txfifo ((volatile uint32_t *) 0x1a10c018)
#define reg_spi_dummy  ((volatile uint32_t *) 0x1a10c014)
#define reg_dcxo_ctrl1 ((volatile uint32_t *) 0x1a10a01c)
#define reg_dcxo_ctrl2 ((volatile uint32_t *) 0x1a10a020)
#define reg_vol        ((volatile uint32_t *) 0x1a10a024)
#define reg_freq_div   ((volatile uint32_t *) 0x1a10a00c)
#define cce_mode_ctrl  ((volatile uint32_t *) 0x1a10a038)
#define paging_exit    ((volatile uint32_t *) 0x3b0c10)
#define pmu_ctrl       ((volatile uint32_t *) 0x1a10a000)


#define SPI_START(cmd)  (*reg_spi_status = (1<<(SPI_CSN0+8))|(1<<(cmd))); //start
#define WAIT_XIP_FREE    while((*reg_xip_ctrl)&0x1)

//cmd shifted if 8 bits
#define FLASH_QRD        0x11101011
#define FLASH_RD         0x03000000
#define FLASH_WE         0x06000000
#define FLASH_WSR        0x01000000
#define FLASH_RSR        0x05000000


/**************************
*
*DO NOT MOVE AROUND THE FUNCTIONS!
*spi_read_fifo() and flash_read_sr() must be in cache()
*otherwise might deadlock or fetch wrong data for instr
***************************
*/

//
//spi len register
//bit 31:16 datalen;
//bit 15:8  addr len;
//bit  7:0  cmd len;
__critical_128 void xip_dummy_detect()
{
    uint32_t data;
    register int i;
    *reg_spi_cmd = FLASH_QRD;  //set cmd
    *reg_spi_len = 0x201820;    //set cmd,add and data len
    *reg_spi_addr = 0x00009000;
    WAIT_XIP_FREE;

    //should add some delay here
#ifdef SPI_FULL_SPEED
    *(reg_xip_ctrl) = 0x20802; //default 0802
#else
    *(volatile int*) (SPI_REG_CLKDIV) = 0x0;
#endif

#ifdef SHIFT_RX_EDGE
    *reg_xip_ctrl |= 0x8000; //change rx edge bit 15;
#endif
    for (i = 2; i < 12; i++)
    {
        *(reg_spi_dummy) = i;
        //        WAIT_XIP_FREE;
        SPI_START(SPI_CMD_QRD);
        //check SPI_RX FIFO
        while (((*(reg_spi_status) >> 16) & 0xFF) == 0);
        data = *(reg_spi_rxfifo);
        if (data == 0xdeadbeef)
        {
            break;
        }
    }
    auto_dummy = i;
}

//__critical_128 uint32_t flash_read_id(){
//  uint32_t data;
//  *reg_spi_cmd = 0x9F000000;  //set cmd
//  *reg_spi_len = 0x200008;      //set cmd and data len
//  SPI_START(SPI_CMD_RD);
//  while(((*(reg_spi_status)>>16)&0xFF)==0);
//  data = *(reg_spi_rxfifo);
//  return data;
//}

__critical_128 void flash_qspi_en()
{
    //uint32_t data;
    //flash we
    // flash_read_sr();//must warm up cache for sr read
    WAIT_XIP_FREE;
    *reg_spi_cmd = FLASH_WE;  //set cmd
    *reg_spi_len = 0x0008;      //set cmd and data len
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_WR);
    while (*reg_spi_status != 1);
    //QSPI EN
    *reg_spi_cmd = FLASH_WSR;
    *reg_spi_len = 0x0008 | (16 << 16);
    *reg_spi_txfifo = 0x00020000;
    SPI_START(SPI_CMD_WR);
    while (*reg_spi_status != 1);
    //cache refill will hang when write in progress
    //poll write in progress
    // while (flash_read_sr() & 0x03);
}


/* this is done in SPI mode for safety */
__critical_128 void fill_mtr()
{
    uint32_t stbl_flash_addr;
    uint32_t stbls[SEG_TBL_SIZE];
    uint32_t* mtr_addr = (uint32_t*) REG_MTR_CFG_0;
    register int count;
    register int i;
    uint8_t* src, *dest;

    /* load mtr registeres using spi  */
    //warmup read fifo, otherwise might deadlock with code fetch
    spi_read_fifo((int*) stbls, 0);
    WAIT_XIP_FREE;
    stbl_flash_addr = (uint32_t)(&_stbl_end) - (SEG_TBL_SIZE << 2);
    *reg_spi_cmd = FLASH_RD;  //set cmd
    *reg_spi_addr = (stbl_flash_addr << 8);
    *reg_spi_len = 0x1808 | ((SEG_TBL_SIZE << 5) << 16); //set cmd,addr and data len
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_RD);
    spi_read_fifo((int*) stbls, (SEG_TBL_SIZE << 5));
    /* we only have 8 mtr regs now */
    for (i = 7; i >= 0; i--)
    {
        *(mtr_addr + i) = stbls[i];
    }
    /* now safe to access RO memory area */
    /* cp init data from RO to RAM */
#ifndef _NO_SDATA_
    count = ((uint32_t)&edata - (uint32_t)&_sdata);
    dest = (uint8_t*) &_sdata;
    src = (uint8_t* ) stbls[19];
    for (i = 0; i < count; i++)
    {
        *(dest++) = *(src++);
    }
#endif
}

/* this is done in SPI mode for safety */
__critical_128 void fill_mtr_qspi()
{
    uint32_t stbl_flash_addr;
    uint32_t stbls[SEG_TBL_SIZE];
    uint32_t* mtr_addr = (uint32_t*) REG_MTR_CFG_0;
    register int count;
    register int i;
    uint8_t* src, *dest;

    /* load mtr registeres using spi  */
    //warmup read fifo, otherwise might deadlock with code fetch
    spi_read_fifo((int*) stbls, 0);
    stbl_flash_addr = (uint32_t)(&_stbl_end) - (uint32_t ) &_rom_start - (SEG_TBL_SIZE << 2);
    *reg_spi_cmd = FLASH_QRD;  //set cmd
    *reg_spi_addr = (stbl_flash_addr << 8);
    *reg_spi_len = 0x1820 | ((SEG_TBL_SIZE << 5) << 16); //set cmd,addr and data len
    WAIT_XIP_FREE;
    SPI_START(SPI_CMD_QRD);
    spi_read_fifo((int*) stbls, (SEG_TBL_SIZE << 5));
    /* we only have 8 mtr regs now */
    for (i = 7; i >= 0; i--)
    {
        *(mtr_addr + i) = stbls[i];
    }
    /* now safe to access RO memory area */
    /* cp init data from RO to RAM */
#ifndef _NO_SDATA_
    count = ((uint32_t)&edata - (uint32_t)&_sdata);
    dest = (uint8_t*) &_sdata;
    src = (uint8_t* ) stbls[19];
    for (i = 0; i < count; i++)
    {
        *(dest++) = *(src++);
    }
#endif
}

__critical_128 void boot_noop()
{
    __asm__ ("nop");
}

__critical_64 void init_parameters()
{
    /* turn off device IRQ */
    IER = 0;
    IPR = 0;
    *reg_spi_dummy = 0;

  // over-clock config

    // *(volatile int*) (0x1a107060) |= 0x1 << 9;

    // *(volatile int*) (0x1a10a020) |= 0x1 << 23;
    // *(volatile int*) (0x1a10a020) |= 0x1 << 22;
    // *(volatile int*) (0x1a10a00c) |= 0x1;
    // *(volatile int*) (0x1a10a024) |= 0x1f << 19;
    // *(volatile int*) (0x1a10a000) |= 0x7<<8;


    *(volatile int*) (SPI_REG_CLKDIV) = 0x1;
}

__critical_128 void xip_switch_mode()
{
    WAIT_XIP_FREE;
#ifdef SPI_FULL_SPEED
    *reg_xip_ctrl = (0x00022006 | auto_dummy << 3);
#else
#ifdef SHIFT_RX_EDGE
    *reg_xip_ctrl = (0x0000A006 | auto_dummy << 3);
#else
    *reg_xip_ctrl = (0x00002006 | auto_dummy << 3);
#endif
#endif
    //this code must not be cache miss!
    /* xtx dspi flash just use 0xBB command to double out */

    //-----------------------------------------------------------------------------
    /*************************
     *   XIP Controller Defs
     * bit 0 RO - sts, 1=busy;
     * bit 1, Enable Flash Fetch. CAUTION! default 1
     * bit 2 SPI mode 0=SPI 1=QSPI
     * bit 3:6 Dummy cycles as in specs, must add extra 2 for modebits.see pdf
     * bit 7 rsv
     * bit 8:13 spi_cmd_len in bits, default 8bits.
     * bit 14: xip tx edge sel, 1 use rising spi edge,  NOT APPLICABLE FOR FULL SPEED MODE
     * bit 15: xip rx edge sel, 1 use falling spi edge, NOT APPLICABLE FOR FULL SPEED MODE
     * bit 16: xip clock pol,   1 use inverted spi clk. NOT APPLICABLE FOR FULL SPEED MODE
     * bit 17: xip full speed mode.
     * bit 18: xip double line spi mode.
     * xip control Enable+QSPI READ+4+2Dummy. 2Extra mode bits as for s256x4.
     * cmd is send as 32bit so on SPI is 8bits. 0xEB is only send out on
     * sdo0,  cmd expand from 8 to 32...
     * so register valule bit 3-6 is 4b'0110
     * 0x00002036, 6 dummy,QSPI, 32bit cmd
     * 0x0000203E, 7 dummy, QSPI, 32bit cmd
     * 0x00002046, 8 dymmy, QSPI, 32bit cmdr
     * 0x00002056, 10 dummy,QSPI, 32bit cmd
     **************************
    */
    //-----------------------------------------------------------------------------

#ifdef XTX_FLASH
    *reg_xip_ctrl = 0x00041002; //16bit cmd, DSPI mode.
    *reg_xip_cmd  = 0x45450000; //0xBB expanded to 16 bit DSPI
#else
    *((volatile uint32_t*)(REG_XIP_CMD)) = 0x11101011;
#endif
    //cache miss his dangerous here...
}

__critical_128 void pmc_init(void)
{
    register int tmp = 0;
    register int pmu_value = *(pmu_ctrl);
    register int vol_value;

    // register unsigned int *wdg_ctr = (unsigned int*)(WATCHDOG_BASE_ADDR);
    // register unsigned int *wdg_wiv = (unsigned int*)(WATCHDOG_BASE_ADDR + 4);
    // register unsigned int *wdg_wfd = (unsigned int*)(WATCHDOG_BASE_ADDR + 8);

    // *wdg_wiv  = 0xFFFFFFFFU - 50 * 32768U / 1000;
    // *wdg_wfd |= 0x01;
    // *wdg_ctr |= 0x01;
    // *wdg_wfd |= 0x01;

    if ((pmu_value >> 27) & 1)
    {
        tmp = (*(volatile int *)(0x3b0028)) | (1 << 5 | 1 << 7 | 1 << 8);
        *(volatile int *)(0x3b0028) = tmp;

        tmp = ((*(volatile int *)(0x3b0000)) & 0xFFFF) | (0x2FD2 << 16);
        *(volatile int *)(0x3b0000) = tmp;

        vol_value = *(reg_vol) & (~(0xF << 15));
        *(reg_vol) = vol_value | (8 << 15);    // set dcdc vol to 2.1v, addr: 0x1a10a024
        *(pmu_ctrl) &= (~(1 << 16 | 1 << 18)); // stop cce pow, stop wut pow
        *(pmu_ctrl) |= (1 << 18);              // enable wut power
    }
}

#ifdef XTX_FLASH
__critical_128 void boot_strap()
{
    WAIT_XIP_FREE;
    init_parameters();
    //flash_read_id();
    fill_mtr();
    flash_qspi_en();
    WAIT_XIP_FREE;
    //8288 dcxo doubler enable; set 96M
    *(reg_dcxo_ctrl2) |= (1 << 16);  // 0x1a10a01c
    *(reg_freq_div) &= (0xFFFFFFF1); // 0x1a10a00c
    // if (0 != *(cce_mode_ctrl))
    // {
    //     *(cce_mode_ctrl) = 0; // reset cce wm, 0x1a10a038
    //     *(paging_exit) = (1); // clear paging, 0x3b0c10
    //     *(paging_exit) = (0);
    //     // *(cce_mode_ctrl) = 8;  // reset cce timer
    //     *(paging_exit) = (1);  // clear paging
    //     *(paging_exit) = (0);
    // }
    *(cce_mode_ctrl) = 0;
    *(paging_exit) |= (1);  // clear paging
    *(cce_mode_ctrl) = 8;
    *(pmu_ctrl) &= ~(1 << 1); // clear sleep, 0x1a10a000

    //should add some delay here
    xip_switch_mode();
    //-----------One Cacheline of Code --------------
    //if (*(reg_dcxo_ctrl2) & 0x10000)
    //{
        //uart_set_cfg(0,55);
        //printf("Doubler Enabled\n");
    //}
    //else
        //uart_set_cfg(0,2);
        //printf("boot strapping done!\r\n");
    //{ return; }
}
#else
__critical_128 void boot_strap()
{
    //this is done in reset handler now;
    WAIT_XIP_FREE;
    if (*reg_spi_cmd == 0x11101011)
    {
        fill_mtr_qspi();
    }
    else
    {
        init_parameters();
        //    flash_read_id();
        fill_mtr();
        /* critical function, enable flash QSPI Mode */
        flash_qspi_en();

        //-----------------------------------------------------------------------------
        /*************************
         *   XIP Controller Defs
         * bit 0 RO - sts, 1=busy;BIT 1, Enable Flash Fetch. CAUTION! default 1
         * bit 2 SPI mode 0=SPI 1=QSPI
         * bit 3:6 Dummy cycles as in specs, must add extra 2 for modebits.see pdf
         * bit 7 rsv
         * bit 8:13 spi_cmd_len in bits, default 8bits.
         * bit 14: xip tx edge sel, 1 use rising spi edge,  NOT APPLICABLE FOR FULL SPEED MODE
         * bit 15: xip rx edge sel, 1 use falling spi edge, NOT APPLICABLE FOR FULL SPEED MODE
         * bit 16: xip clock pol,   1 use inverted spi clk. NOT APPLICABLE FOR FULL SPEED MODE
         * bit 17: xip full speed mode.
         * xip control Enable+QSPI READ+4+2Dummy. 2Extra mode bits as for s256x4.
         * cmd is send as 32bit so on SPI is 8bits. 0xEB is only send out on
         * sdo0,  cmd expand from 8 to 32...
         * so register valule bit 3-6 is 4b'0110
         * 0x00002036, 6 dummy,QSPI, 32bit cmd
         * 0x0000203E, 7 dummy, QSPI, 32bit cmd
         * 0x00002046, 8 dymmy, QSPI, 32bit cmdr
         * 0x00002056, 10 dummy,QSPI, 32bit cmd
         **************************
        */
        //-----------------------------------------------------------------------------
        WAIT_XIP_FREE;
        //8288 dcxo doubler enable;
        *(reg_dcxo_ctrl2) |= (1 << 16);
        //should add some delay here
#ifdef SPI_FULL_SPEED
        *(reg_xip_ctrl) = 0x20802; //default 0802
#endif
        xip_dummy_detect();

        //now change XIP controller to us
        //xip configuration change need to be done without any cachemiss...
        xip_switch_mode();
        //-----------One Cacheline of Code --------------
        //fill_mtr_qspi();
    }
    if (*(reg_dcxo_ctrl2) & 0x10000)
    {
        //  uart_set_cfg(0,51);
        //printf("Doubler Enabled\n");
    }
    else
        //uart_set_cfg(0,27);
        //printf("boot strapping done!\r\n");
    { return; }
}
#endif


// __critical_64 int flash_read_sr()
// {
//     uint32_t data;
//     *reg_spi_cmd = FLASH_RSR; //read sr
//     *reg_spi_len = 0x200008;
//     WAIT_XIP_FREE;
//     SPI_START(SPI_CMD_RD);
//     while (((*(reg_spi_status) >> 16) & 0xFF) == 0);
//     data = *(reg_spi_rxfifo);
//     return data;
// }

