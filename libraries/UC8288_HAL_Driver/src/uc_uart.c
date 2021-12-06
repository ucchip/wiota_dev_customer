// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include "uc_uart.h"
#include "uc_utils.h"
#include "uc_pulpino.h"
#include "uc_sectdefs.h"
/**
 * Setup UART. The UART defaults to 8 bit character mode with 1 stop bit.
 *
 * parity       Enable/disable parity mode
 * clk_counter  Clock counter value that is used to derive the UART clock.
 *              It has to be in the range of 1..2^16.
 *              There is a prescaler in place that already divides the SoC
 *              clock by 16.  Since this is a counter, a value of 1 means that
 *              the SoC clock divided by 16*2 = 32 is used. A value of 31 would mean
 *              that we use the SoC clock divided by 16*32 = 512.
 */
__crt0 void uart_set_cfg(int parity, uint16_t clk_counter)
{
    CGREG |= (1 << CGUART); // don't clock gate UART
    *(volatile unsigned int*)(UART_REG_LCR) = 0x83; //sets 8N1 and set DLAB to 1
    *(volatile unsigned int*)(UART_REG_DLM) = (clk_counter >> 8) & 0xFF;
    *(volatile unsigned int*)(UART_REG_DLL) =  clk_counter       & 0xFF;
    *(volatile unsigned int*)(UART_REG_FCR) = 0x27; //enables 16byte FIFO and clear FIFOs
    *(volatile unsigned int*)(UART_REG_LCR) = 0x03; //sets 8N1 and set DLAB to 0

    *(volatile unsigned int*)(UART_REG_IER) = ((*(volatile unsigned int*)(UART_REG_IER)) & 0xF0) | 0x01; // set IER (interrupt enable register) on UART
}

void uart_send(const char* str, unsigned int len)
{
    unsigned int i;

    while (len > 0)
    {
        // process this in batches of 16 bytes to actually use the FIFO in the UART

        // wait until there is space in the fifo
        while ( (*(volatile unsigned int*)(UART_REG_LSR) & 0x20) == 0);

        for (i = 0; (i < UART_FIFO_DEPTH) && (len > 0); i++)
        {
            // load FIFO
            *(volatile unsigned int*)(UART_REG_THR) = *str++;

            len--;
        }
    }
}

char uart_getchar()
{
    while ((*((volatile int*)UART_REG_LSR) & 0x1) != 0x1);

    return *(volatile int*)UART_REG_RBR;
}

void uart_sendchar(const char c)
{
    // wait until there is space in the fifo
    while ( (*(volatile unsigned int*)(UART_REG_LSR) & 0x20) == 0);

    // load FIFO
    *(volatile unsigned int*)(UART_REG_THR) = c;
}

void uart_wait_tx_done(void)
{
    // wait until there is space in the fifo
    while ( (*(volatile unsigned int*)(UART_REG_LSR) & 0x40) == 0);
}

uint8_t uart_get_int_identity()
{
    return *(volatile int*)UART_REG_IIR;
}

/*=============================================== add ==========================================*/
void uc_uart_init(UART_TYPE* uartx, uint32_t baud_rate, uint8_t data_bits, uint8_t stop_bits, uint8_t parity)
{
    uint32_t integerdivider;
    uint32_t line_reg = 0;

    //if (uartx == (UART_TYPE*)UART1_BASE_ADDR)
    //{
    //    set_pin_function(24, 1);
    //    set_pin_function(25, 1);
    //}

    CGREG |= (1 << CGUART); // don't clock gate UART
    //integerdivider = (SYSTEM_CLK/15)/baud_rate - 1;
    integerdivider = (SYSTEM_CLK / 16) / baud_rate -1;
//    integerdivider = 5;

    if ((data_bits >= 5) && (data_bits <= 8))
    {
        line_reg |= data_bits - 5;
    }
    else
    {
        line_reg |= 3;
    }

    if (stop_bits == 2)
    {
        line_reg |= 1 << 2;
    }

    if (parity == 2)
    {
        //Enable parity
        line_reg |= 1 << 3;
        //EVEN
        line_reg |= 1 << 4;
    }
    else if (parity == 1)
    {
        //Enable parity
        line_reg |= 1 << 3;
    }

    uartx->LCR = line_reg | 0x80; //sets 8N1 and set DLAB to 1
    uartx->DLM = (integerdivider >> 8) & 0xFF;
    uartx->DLL =  integerdivider       & 0xFF;
    uartx->FCR = 0x27; //enables 1byte FIFO and clear FIFOs
    uartx->LCR = line_reg; //sets 8N1 and set DLAB to 0

}

char uc_uart_getchar(UART_TYPE* uartx, uint8_t* get_char)
{
    //while((*((volatile int*)UART_REG_LSR) & 0x1) != 0x1);

    //return *(volatile int*)UART_REG_RBR;
    if ((uartx->LSR & 0x1) == 0x1)
    {
        //return uartx->RBR;
        *get_char = uartx->RBR;
        return 0;
    }
    else
    {
        return -1;
    }
}

__crt0 void uc_uart_sendchar(UART_TYPE* uartx, const char c)
{
    // wait until there is space in the fifo
    while ( (uartx->LSR & 0x20) == 0);

    // load FIFO
    uartx->THR = c;
}

uint8_t uc_uart_get_intrxflag(UART_TYPE* uartx)
{
#if 0
    //if ((*((volatile int*)((uint32_t)uartx - UART_BASE_ADDR + UART_REG_IIR)) & 0x5) == 0x5)
    if (uartx->IIR & 0x5)
    {
        return 1;
    }
    else
    {
        return 0;
    }
#else
    if (*((volatile int*)((uint32_t)uartx - UART_BASE_ADDR + UART_REG_IIR)) & 0x5)
    {
        return 1;
    }
    else
    {
        return 0;
    }
#endif
}

void uc_uart_enable_intrx(UART_TYPE* uartx, uint8_t ctrl)
{
#if 0
    if (ctrl)
    {
        uartx->IER = 0x01;
    }
    else
    {
        uartx->IER = 0x00;
    }
#else
    if (ctrl)
    {
        *(volatile unsigned int*)((uint32_t)uartx - UART_BASE_ADDR + UART_REG_IER) = 0x01;
    }
    else
    {
        *(volatile unsigned int*)((uint32_t)uartx - UART_BASE_ADDR + UART_REG_IER) = 0x00;
    }
#endif
}
/*=============================================== end ==========================================*/
