// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

/**
 * @file
 * @brief SPI library.
 *
 * Provides SPI helper function like configuring SPI and sending
 * data and commands over SPI.
 *
 */
#ifndef _SPIM_H_
#define _SPIM_H_

#include <uc_pulpino.h>
#include "uc_udma.h"

/**/
#define SPIM_QPI    1

/**/
#define SPIM_NO_QPI 0

/**/
#define SPIM_CMD_RD    0

/**/
#define SPIM_CMD_WR    1

/**/
#define SPIM_CMD_QRD   2

/**/
#define SPIM_CMD_QWR   3

/**/
#define SPIM_CMD_SWRST 4

/**/
#define SPIM_CSN0 0

/**/
#define SPIM_CSN1 1

/**/
#define SPIM_CSN2 2

/**/
#define SPIM_CSN3 3

typedef struct
{
    uint32_t Clk_rate;
    uint32_t cs;      /* parity bit enable */
} SPIM_CFG_Type;


#define PARAM_SPIM(spix)    (spix==UC_SPIM)
#define PARAM_SPIM_CS(cs)   ((cs==SPIM_CSN0)||(cs==SPIM_CSN1)||(cs==SPIM_CSN2)||(cs==SPIM_CSN3))
#define PARAM_SPIM_CMD(cmd) ((cmd==SPIM_CMD_RD)||(cmd==SPIM_CMD_WR)||(cmd==SPIM_CMD_QRD)||(cmd==SPIM_CMD_QWR)||(cmd==SPIM_CMD_SWRST))

/**/
#define PIN_SPIM_SIO0 4

/**/
#define PIN_SPIM_SIO1 5

/**/
#define PIN_SPIM_SIO2 6

/**/
#define PIN_SPIM_SIO3 7

/**/
#define PIN_SPIM_CSN  3

/**/
#define PIN_MSPI_SIO0 15

/**/
#define PIN_MSPI_SIO1 14

/**/
#define PIN_MSPI_SIO2 13

/**/
#define PIN_MSPI_SIO3 12

/**/
#define PIN_MSPI_CSN0 16

/**/
#define PIN_MSPI_CSN1 11

/**/
#define PIN_MSPI_CSN2 0

/**/
#define PIN_MSPI_CSN3 1

#define SPILEN_DATA_MASK      0x001F
/**/
#define SPILEN_CLR_MASK      0x003F

/**/
#define SPILEN_ALR_MASK      0x3F00

/**/
#define SPIDUM_TDR_MASK      0xFFFF0000

/**/
#define SPIDUM_RDR_MASK      0x0000FFFF

/**/
#define SPILEN_RLR_MASK      0xFF000000

/**/
#define SPILEN_TLR_MASK      0x00FF0000

/**/
#define SPI_CSN_MASK         0xF00

/**/
#define SPI_TRA_MASK         0xFF

/**/
#define SPILEN_NUM_MASK      0x7FF

/**/
void spim_init(SPI_TypeDef* SPIx, SPIM_CFG_Type* SPI_ConfigStruc);

/**/
void spim_setup_slave();

/**/
void spim_setup_master(int numcs);

/**/
void spim_send_data_noaddr(int cmd, char* data, int datalen, int useQpi);

/**/
void spim_setup_cmd_addr(SPI_TypeDef* SPIx, int cmd, int cmdlen, int addr, int addrlen);

/**/
void spim_setup_dummy(SPI_TypeDef* SPIx, int dummy_rd, int dummy_wr);

/**/
void spim_set_datalen(SPI_TypeDef* SPIx, int datalen);

/**/
void spim_start_transaction(SPI_TypeDef* SPIx, int trans_type, int csnum);

/**/
int spim_get_status(SPI_TypeDef* SPIx);


/**/
void spim_write_fifo(SPI_TypeDef* SPIx, int* data, int datalen);

/**/
void spim_read_fifo(SPI_TypeDef* SPIx, int* data, int datalen) ;

/*useing spi send data with udma*/
void Udma_Spim_Tx(SPI_TypeDef* SPIx, uint32_t* source_addr, uint16_t size);

/*useing spi receive data with udma*/
void Udma_Spim_Rx(SPI_TypeDef* SPIx, uint32_t* dest_addr, uint16_t size);
#endif // _SPI_H_
