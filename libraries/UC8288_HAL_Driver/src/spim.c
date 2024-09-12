// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
#include <spim.h>
#include <uc_gpio.h>
#include <uc_sectdefs.h>
#include <stdint.h>

void spim_setup_master(int numcs)
{
    gpio_set_pin_mux(UC_GPIO_CFG, 0, GPIO_FUNC_2); //SPIM_CLK
    gpio_set_pin_mux(UC_GPIO_CFG, 1, GPIO_FUNC_2); //SPIM_CS
    gpio_set_pin_mux(UC_GPIO_CFG, 2, GPIO_FUNC_2); //SPIM_MISO
    gpio_set_pin_mux(UC_GPIO_CFG, 3, GPIO_FUNC_2); //SPIM_MOSI
    gpio_set_pin_direction(UC_GPIO, 2, GPIO_DIR_IN);
}

void spim_send_data_noaddr(int cmd, char *data, int datalen, int useQpi);

void spim_setup_cmd_addr(int cmd, int cmdlen, int addr, int addrlen)
{
    int cmd_reg;
    cmd_reg = cmd << (32 - cmdlen);
    *(volatile int *)(SPIM_REG_SPICMD) = cmd_reg;
    *(volatile int *)(SPIM_REG_SPIADR) = addr;
    *(volatile int *)(SPIM_REG_SPILEN) = (cmdlen & 0x3F) | ((addrlen << 8) & 0x3F00);
}

void spim_setup_dummy(int dummy_rd, int dummy_wr)
{
    *(volatile int *)(SPIM_REG_SPIDUM) = ((dummy_wr << 16) & 0xFFFF0000) | (dummy_rd & 0xFFFF);
}

void spim_set_datalen(int datalen)
{
    volatile int old_len;
    old_len = *(volatile int *)(SPIM_REG_SPILEN);
    old_len = ((datalen << 16) & 0xFFFF0000) | (old_len & 0xFFFF);
    *(volatile int *)(SPIM_REG_SPILEN) = old_len;
}

void spim_start_transaction(int trans_type, int csnum)
{
    *(volatile int *)(SPIM_REG_STATUS) = ((1 << (csnum + 8)) & 0xF00) | ((1 << trans_type) & 0xFF);
}

int spim_get_status()
{
    volatile int status;
    status = *(volatile int *)(SPIM_REG_STATUS);
    return status;
}

void spim_write_fifo(int *data, int datalen)
{
    volatile int num_words, i;

    num_words = (datalen >> 5) & 0x7FF;

    if ((datalen & 0x1F) != 0)
        num_words++;

    for (i = 0; i < num_words; i++)
    {
        while ((((*(volatile int *)(SPIM_REG_STATUS)) >> 24) & 0xFF) >= 8)
            ;
        *(volatile int *)(SPIM_REG_TXFIFO) = data[i];
    }
}

void spim_read_fifo(int *data, int datalen)
{
    volatile int num_words;
    /* must use register for, i,j*/
    register int i, j;
    num_words = (datalen >> 5) & 0x7FF;

    if ((datalen & 0x1F) != 0)
        num_words++;
    i = 0;
    while (1)
    {
        do
        {
            j = (((*(volatile int *)(SPIM_REG_STATUS)) >> 16) & 0xFF);
        } while (j == 0);
        while (j)
        {
            data[i++] = *(volatile int *)(SPIM_REG_RXFIFO);
            j--;
        };
        //add by xk 20230407 防止由于处理不及时造成死循环
        if (i >= num_words)
            break;
    }
}

/* last function in spi lib, linked to bootstrap code.
 * calling this cause cache to fill 2nd block, so we have
 * 2 blocks of code in cache */
