// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
#include <uc_spi.h>
#include <uc_gpio.h>
#include <uc_sectdefs.h>


/*********************************************************
 * Function name    :void spim_send_data_noaddr(int cmd,
                                                char *data,
                                                int datalen,
                                                int useQpi)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
void spi_send_data_noaddr(int cmd, char* data, int datalen, int useQpi);

/*********************************************************
 * Function name    :void spim_send_data_noaddr(int cmd,
                                                char *data,
                                                int datalen,
                                                int useQpi)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
__reset void spi_setup_cmd_addr(int cmd, int cmdlen, int addr, int addrlen)
{
    int cmd_reg;
    cmd_reg = cmd << (32 - cmdlen);
    *(volatile int*) (SPI_REG_SPICMD) = cmd_reg;
    *(volatile int*) (SPI_REG_SPIADR) = addr;
    *(volatile int*) (SPI_REG_SPILEN) = (cmdlen & 0x3F) | ((addrlen << 8) & 0x3F00);
}


/*********************************************************
 * Function name    :void spim_send_data_noaddr(int cmd,
                                                char *data,
                                                int datalen,
                                                int useQpi)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
__reset void spi_setup_dummy(int dummy_rd, int dummy_wr)
{
    *(volatile int*) (SPI_REG_SPIDUM) = ((dummy_wr << 16) & 0xFFFF0000) | (dummy_rd & 0xFFFF);
}


/*********************************************************
 * Function name    :void spim_send_data_noaddr(int cmd,
                                                char *data,
                                                int datalen,
                                                int useQpi)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
__reset void spi_set_datalen(int datalen)
{
    volatile int old_len;
    old_len = *(volatile int*) (SPI_REG_SPILEN);
    old_len = ((datalen << 16) & 0xFFFF0000) | (old_len & 0xFFFF);
    *(volatile int*) (SPI_REG_SPILEN) = old_len;
}


/*********************************************************
 * Function name    :void spim_send_data_noaddr(int cmd,
                                                char *data,
                                                int datalen,
                                                int useQpi)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
__reset void spi_start_transaction(int trans_type, int csnum)
{
    *(volatile int*) (SPI_REG_STATUS) = ((1 << (csnum + 8)) & 0xF00) | ((1 << trans_type) & 0xFF);
}


/*********************************************************
 * Function name    :void spim_send_data_noaddr(int cmd,
                                                char *data,
                                                int datalen,
                                                int useQpi)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
__reset int spi_get_status()
{
    volatile int status;
    status = *(volatile int*) (SPI_REG_STATUS);
    return status;
}


/*********************************************************
 * Function name    :void spim_send_data_noaddr(int cmd,
                                                char *data,
                                                int datalen,
                                                int useQpi)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
__reset void spi_write_fifo(int* data, int datalen)
{
    volatile int num_words, i;

    num_words = (datalen >> 5) & 0x7FF;

    if ( (datalen & 0x1F) != 0)
    { num_words++; }

    for (i = 0; i < num_words; i++)
    {
        while ((((*(volatile int*) (SPI_REG_STATUS)) >> 24) & 0xFF) >= 8);
        *(volatile int*) (SPI_REG_TXFIFO) = data[i];
    }
}


/*********************************************************
 * Function name    :void spim_send_data_noaddr(int cmd,
                                                char *data,
                                                int datalen,
                                                int useQpi)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
__reset void spi_read_fifo(int* data, int datalen)
{
    volatile int num_words;
    /* must use register for, i,j*/
    register int i, j;
    num_words = (datalen >> 5) & 0x7FF;
    if (datalen == 0)
    { return; }

    if ( (datalen & 0x1F) != 0)
    { num_words++; }
    i = 0;
    while (1)
    {
        do
        {
            j = (((*(volatile int*) (SPI_REG_STATUS)) >> 16) & 0xFF);
        } while (j == 0);
        do
        {
            data[i++] = *(volatile int*) (SPI_REG_RXFIFO);
            j--;
        } while (j);
        if (i >= num_words)
        {
            break;
        }
    }
}

/* last function in spi lib, linked to bootstrap code.
 * calling this cause cache to fill 2nd block, so we have
 * 2 blocks of code in cache */
__reset void spi_no_op()
{
    return;
}

