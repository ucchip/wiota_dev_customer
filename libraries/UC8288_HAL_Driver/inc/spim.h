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
 * @brief SPIM library.
 *
 * Provides SPIM helper function like configuring SPI and sending
 * data and commands over SPIM.
 *
 */
#ifndef _SPIM_H_
#define _SPIM_H_

#include <uc_pulpino.h>

#define SPIM_QPI 1
#define SPIM_NO_QPI 0

#define SPIM_CMD_RD 0
#define SPIM_CMD_WR 1
#define SPIM_CMD_QRD 2
#define SPIM_CMD_QWR 3
#define SPIM_CMD_SWRST 4

#define SPIM_CSN0 0
#define SPIM_CSN1 1
#define SPIM_CSN2 2
#define SPIM_CSN3 3

#define SPIM_REG_STATUS (SPIM_BASE_ADDR + 0x00)
#define SPIM_REG_CLKDIV (SPIM_BASE_ADDR + 0x04)
#define SPIM_REG_SPICMD (SPIM_BASE_ADDR + 0x08)
#define SPIM_REG_SPIADR (SPIM_BASE_ADDR + 0x0C)
#define SPIM_REG_SPILEN (SPIM_BASE_ADDR + 0x10)
#define SPIM_REG_SPIDUM (SPIM_BASE_ADDR + 0x14)
#define SPIM_REG_TXFIFO (SPIM_BASE_ADDR + 0x18)
#define SPIM_REG_RXFIFO (SPIM_BASE_ADDR + 0x20)
#define SPIM_REG_INTCFG (SPIM_BASE_ADDR + 0x24)
#define SPIM_REG_INTSTA (SPIM_BASE_ADDR + 0x28)

#define SPIM_STATUS REG(SPIM_REG_STATUS)
#define SPIM_CLKDIV REG(SPIM_REG_CLKDIV)
#define SPIM_SPICMD REG(SPIM_REG_SPICMD)
#define SPIM_SPIADR REG(SPIM_REG_SPIADR)
#define SPIM_SPILEN REG(SPIM_REG_SPILEN)
#define SPIM_SPIDUM REG(SPIM_REG_SPIDUM)
#define SPIM_TXFIFO REG(SPIM_REG_TXFIFO)
#define SPIM_RXFIFO REG(SPIM_REG_RXFIFO)
#define SPIM_INTCFG REG(SPIM_REG_INTCFG)
#define SPIM_INTSTA REG(SPIM_REG_INTSTA)

#define PIN_SSPIM_SIO0 4
#define PIN_SSPIM_SIO1 5
#define PIN_SSPIM_SIO2 6
#define PIN_SSPIM_SIO3 7
#define PIN_SSPIM_CSN 3

#define PIN_MSPIM_SIO0 15
#define PIN_MSPIM_SIO1 14
#define PIN_MSPIM_SIO2 13
#define PIN_MSPIM_SIO3 12
#define PIN_MSPIM_CSN0 16
#define PIN_MSPIM_CSN1 11
#define PIN_MSPIM_CSN2 0
#define PIN_MSPIM_CSN3 1

#define FUNC_SPIM 0

void spim_setup_slave();

void spim_setup_master(int numcs);

void spim_send_data_noaddr(int cmd, char *data, int datalen, int useQpi);

void spim_setup_cmd_addr(int cmd, int cmdlen, int addr, int addrlen);

void spim_set_datalen(int datalen);

void spim_setup_dummy(int dummy_rd, int dummy_wr);

void spim_start_transaction(int trans_type, int csnum);

void spim_write_fifo(int *data, int datalen);

void spim_read_fifo(int *data, int datalen);

int spim_get_status();
#define SPIM_WAIT()                             \
    {                                           \
        while (1 != (spim_get_status() & 0x01)) \
        {                                       \
            ;                                   \
        }                                       \
    }

#endif // _SPIM_H_
