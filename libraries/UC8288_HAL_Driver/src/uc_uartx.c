/*
 * Copyright (C) 2020 UCCHIP CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of UCCHIP CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "uc_uartx.h"
#include "uc_pulpino.h"

void uartx_init(UART_TYPE* UARTx, UART_CFG_Type* uart_cfg)
{
    uint32_t tem;
    uint32_t integerdivider;

    CHECK_PARAM(PARAM_UART(UARTx));
    CHECK_PARAM(PARAM_UART_PARITYBIT(uart_cfg->Parity));
    CHECK_PARAM(PARAM_UART_DATABIT(uart_cfg->Databits));
    CHECK_PARAM(PARAM_UART_STOPBIT(uart_cfg->Stopbits));
    CHECK_PARAM(PARAM_UART_CLK_RATE(uart_cfg->Baud_rate));
    CHECK_PARAM(PARAM_UART_LEVEL(uart_cfg->level));

    CGREG |= (1 << CGUART); // don't clock gate UART
    /*------------------------- UART LCR reg Configuration-----------------------*/

    tem = uart_cfg->Databits;
    tem += STOPBIT(uart_cfg->Stopbits);
    tem += PARITYBIT(uart_cfg->Parity);
    UARTx->LCR = tem;

    /*--------------- UART baud rate Configuration-----------------------*/
    integerdivider = (SYSTEM_CLK / 16) / uart_cfg->Baud_rate - 1;
    UARTx->LCR |= 0x80;//set enable bit for access DLM, DLL and FCR
    UARTx->DLM = (integerdivider >> 8) & 0xFF;
    UARTx->DLL = integerdivider & 0xFF;

    /*--------------- set trigger level ,enable fifo,reset rx and tx fifo ----------------------*/

    UARTx->FCR = ((uart_cfg->level << 6) + 0x7);
    UARTx->LCR &= ~0x80;//clear enable bit for access DLM, DLL and FCR
}

void uartx_set_interrupt_type(UART_TYPE* UARTx, UART_IT it_type)
{
    CHECK_PARAM(PARAM_UART(UARTx));
    CHECK_PARAM(PARAM_UART_IT(it_type));
    if (it_type == UART_IT_NONE)
    {
        UARTx->IER = 0;
    }
    else
    {
        UARTx->IER = it_type;
    }
}

uint32_t uartx_is_interrupt_pending(UART_TYPE* UARTx)
{
    CHECK_PARAM(PARAM_UART(UARTx));
    return ((UARTx->IIR & 0x01) == 0x00);
}

UART_II uartx_get_interrupt_id(UART_TYPE* UARTx)
{
    CHECK_PARAM(PARAM_UART(UARTx));
    return (UARTx->IIR >> 1) & 0x03;
}

void uartx_sendchar(UART_TYPE* UARTx, uint8_t data) //blocking
{
    CHECK_PARAM(PARAM_UART(UARTx));
    // wait until there is space in the fifo
    while ( (UARTx->LSR & 0x20) == 0);

    UARTx->THR = data;
}

char uartx_getchar(UART_TYPE* UARTx) //blocking
{
    CHECK_PARAM(PARAM_UART(UARTx));
    // wait until rx fifo is not empty
    while (((UARTx->LSR) & 0x1) != 0x1);

    return UARTx->RBR;
}

uint32_t uartx_send(UART_TYPE* UARTx, uint8_t* buf, uint32_t len) //non-blocking
{
    uint32_t i;

    CHECK_PARAM(PARAM_UART(UARTx));
    for (i = 0; i < len; i++)
    {
        if ((UARTx->LSR & 0x20) != 0)
        {
            UARTx->THR = buf[i];
        }
        else
        {
            break;
        }
    }

    return i;//count sent bytes
}

uint32_t uartx_recv(UART_TYPE* UARTx, uint8_t* buf, uint32_t len) //non-blocking
{
    uint32_t i;

    CHECK_PARAM(PARAM_UART(UARTx));
    for (i = 0; i < len; i++)
    {
        if ((UARTx->LSR & 0x1) == 0x1)
        {
            buf[i] = UARTx->RBR;
        }
        else
        {
            break;
        }
    }

    return i;//count received bytes
}

void uartx_wait_tx_done(UART_TYPE* UARTx)
{
    CHECK_PARAM(PARAM_UART(UARTx));

    while (!(UARTx->LSR & TX_DONE_MASK));
}

void uartx_clean_rxfifo(UART_TYPE* UARTx)
{
    CHECK_PARAM(PARAM_UART(UARTx));

    UARTx->LCR |= 0x80;//set enable bit for access DLM, DLL and FCR
    UARTx->FCR |= TX_FIFO_CLEAN_MASK;
    UARTx->LCR &= ~0x80;//clear enable bit for access DLM, DLL and FCR
}

void uartx_clean_txfifo(UART_TYPE* UARTx)
{
    CHECK_PARAM(PARAM_UART(UARTx));

    UARTx->LCR |= 0x80;//set enable bit for access DLM, DLL and FCR
    UARTx->FCR |= TX_FIFO_CLEAN_MASK;
    UARTx->LCR &= ~0x80;//clear enable bit for access DLM, DLL and FCR
}
