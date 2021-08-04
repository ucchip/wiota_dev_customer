#ifndef _PUF_H_
#define _PUF_H_

#define reg_xip_ctrl   ((volatile uint32_t *) 0x1a10c02c)
#define reg_spi_cmd    ((volatile uint32_t *) 0x1a10c008)
#define reg_spi_addr   ((volatile uint32_t *) 0x1a10c00c)
#define reg_spi_len    ((volatile uint32_t *) 0x1a10c010)
#define reg_spi_status ((volatile uint32_t *) 0x1a10c000)
#define FLASH_RD         0x03000000
#define FLASH_QRD        0x11101011
#define REG_XIP_CTRL    0x1A10C02C
#define REG_PMC         0x1A104200
#define WAIT_XIP_FREE    while((*reg_xip_ctrl)&0x1)
#define SPI_START(cmd)  (*reg_spi_status = (1<<(0+8))|(1<<(cmd))); //start

void init_puf(int gen_puf);
void load_help_bits();
#endif
