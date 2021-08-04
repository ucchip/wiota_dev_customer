#include<stdint.h>
#include "uc_puf.h"
#include "uc_spi.h"
#include "uc_string_lib.h"

#define READ_BIT 16384



void load_help_bits()
{
    uint8_t data[2048] = {0};
    uint32_t* help_bit_ram = (uint32_t*)(0x30c000);
    uint32_t addr = 0x1ff700;
//    uint32_t key[4] = {0};

    for (int i = 0; i < 64; i++)
    {
        spi_setup_cmd_addr(FLASH_QRD, 32, (addr + i * 32) << 8, 24);
        spi_set_datalen(256);
        while ((*((volatile uint32_t*)(REG_XIP_CTRL))) & 0x1);

        spi_start_transaction(SPI_CMD_QRD, SPI_CSN0);
        while ((*((volatile uint32_t*)(REG_XIP_CTRL))) & 0x1);

        spi_read_fifo((int*) &data, 256);
        for (int j = 0; j < 8; j++)
        {
            help_bit_ram[j + i * 8] = data[4 * j] << 24   | data[4 * j + 1] << 16 |
                                      data[4 * j + 2] << 8  | data[4 * j + 3];
        }
        memset(data, 0, 256);
    }

    spi_setup_cmd_addr(FLASH_QRD, 32, (addr + 2048) << 8, 24);
    spi_set_datalen(128);
    while ((*((volatile uint32_t*)(REG_XIP_CTRL))) & 0x1);

    spi_start_transaction(SPI_CMD_QRD, SPI_CSN0);
    while ((*((volatile uint32_t*)(REG_XIP_CTRL))) & 0x1);

    spi_read_fifo((int*) &data, 128);
//    for (int j = 0; j < 4; j++)
//    {
//        key[j] = data[4 * j + 3] << 24   | data[4 * j + 2] << 16 |
//                 data[4 * j + 1] << 8  | data[4 * j];
//        //printf("%x\n", key[j]);
//    }
}


#define PULP_CTRL_ADDR (0x10000000 + 0xA100000 + 0x7000)
#define REG_CCE_FETCH  (0x1C >> 2)
#if 1
void init_puf(int gen_puf)
{
    volatile uint32_t* puf_crs = (uint32_t*) 0x1a10a03c;
    volatile uint32_t* cce_wm = (uint32_t*) 0x1a10a038;
    volatile uint32_t* PMC = (uint32_t*) 0x1a10a000;
    uint32_t* ptr = (uint32_t*)( PULP_CTRL_ADDR ) + REG_CCE_FETCH;
    *puf_crs = 0;
    if (gen_puf)
    {
        load_help_bits();
        *cce_wm &= ~0x03;
        *cce_wm |= 0x2;
        *puf_crs = 1;
        *PMC |= 0x8000;
        //    printf("cce fetch enable\n");
        //    printf("cce fetch ctrl=%x\n", ptr);
        *(ptr) =  0x1;
    }
    else
    {
        *cce_wm &= ~0x03;
        *puf_crs = 0x1;
        while (!(*puf_crs & 0xc0000000));
        *puf_crs = 0x0;
        //*PMC |= 0x8000;
        return;
    }
    while (!(*puf_crs & 0x40000000));
    if (*puf_crs & 0x20000000);
    *PMC &= (~0x8000);
    *(ptr) = 0x0;
    *PMC |= 0x8000;

    *puf_crs = 0x0;
    //  printf("init puf end\n");
}
#else
void init_puf(int gen_puf)
{
  volatile uint32_t * puf_csr = (uint32_t*) 0x1a10a03C;
  volatile uint32_t * cce_wm =  (uint32_t*) 0x1a10a038;
  volatile uint32_t * PMC= (uint32_t *) 0x1a10a000; //default 0x30600, bit 15 is cce region reset
  *puf_csr = 0; //reset puf .
  if (gen_puf){
     *cce_wm &= ~0x03; //clean cce workmode;
     *cce_wm |= 0x2; //set to puf mode
     *puf_csr = 1; //release puf module reset
     *PMC |= 0x08000; //release cce_region_reset
     /* TODO code to check puf results*/
  }
  else{
     *puf_csr = 0x1; //just release puf reset
  }
  while (!(*puf_csr&0xC0000000));
}

#endif
