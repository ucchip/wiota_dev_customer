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

#ifndef _UARTX_H_
#define _UARTX_H_

#include "uc_pulpino.h"
#include <stdint.h>

/*------------LSR bit 1 receive data valid---------*/

#define RX_VALID_MASK        0x1
#define RBR_MASK             0xff
#define TX_DONE_MASK         0x40
#define RX_FIFO_CLEAN_MASK   0x02
#define TX_FIFO_CLEAN_MASK   0x04

#define DLAB(x) x<<7        //DLAB bit in LCR reg
#define STOPBIT(x) x<<2     //STOP bit in LCR reg
#define PARITYBIT(x) x<<3   //STOP bit in LCR reg

typedef enum
{
    UART_PARITYBIT_NONE = 0,
    UART_PARITYBIT_ODD  = 1,
    UART_PARITYBIT_EVEN = 3,
} UART_PARITYBIT;

typedef enum
{
    UART_DATABIT_5 = 0,
    UART_DATABIT_6 = 1,
    UART_DATABIT_7 = 2,
    UART_DATABIT_8 = 3
} UART_DATABIT;

typedef enum
{
    UART_STOPBIT_1 = 0,
    UART_STOPBIT_2 = 1,
} UART_STOPBIT;

typedef enum
{
    UART_TL_BYTE_1  = 0,
    UART_TL_BYTE_4  = 1,
    UART_TL_BYTE_8  = 2,
    UART_TL_BYTE_14 = 3
} UART_TRIGGER_LEVEL;

typedef enum
{
    UART_IT_NONE    = 0,
    UART_IT_RX      = (1U << 0),
    UART_IT_TX      = (1U << 1),
    UART_IT_RXLINE  = (1U << 2),
    UART_IT_MODEM   = (1U << 3),
} UART_IT;

typedef enum
{
    UART_II_RLS   = 3, /* Receiver Line Status */
    UART_II_RDA   = 2, /* Received Data Available */
    UART_II_CT    = 6, /* Character Timeout */
    UART_II_THRE  = 1, /* Transmitter Holding Register Empty */
    UART_II_MS    = 0, /* Modem Status */
} UART_II; /*UART interrupt ID*/

typedef struct
{
    uint32_t              Baud_rate;
    UART_PARITYBIT        Parity;      /* parity bit enable */
    UART_DATABIT          Databits;
    UART_STOPBIT          Stopbits;    /* stop  bit */
    UART_TRIGGER_LEVEL    level;
} UART_CFG_Type;


#define PARAM_UART_DATABIT(Databits)    ((Databits==UART_DATABIT_5)||(Databits==UART_DATABIT_6) \
                                         ||(Databits==UART_DATABIT_7)||(Databits==UART_DATABIT_8))

#define PARAM_UART_IT(it_type)    ((it_type==UART_IT_RX)||(it_type==UART_IT_TX) \
                                   ||(it_type==UART_IT_RXLINE)||(it_type==UART_IT_MODEM))

#define PARAM_UART_LEVEL(level)    ((level==UART_TL_BYTE_1)||(level==UART_TL_BYTE_4) \
                                    ||(level==UART_TL_BYTE_8)||(level==UART_TL_BYTE_14))

#define PARAM_UART_PARITYBIT(Parity)    ((Parity==UART_PARITYBIT_NONE)||(Parity==UART_PARITYBIT_ODD)||(Parity==UART_PARITYBIT_EVEN))

#define PARAM_UART_STOPBIT(Stopbits)    ((Stopbits==UART_STOPBIT_2)||(Stopbits==UART_STOPBIT_1))

#define PARAM_UART(SCD)    (SCD==UC_UART)

#define PARAM_UART_CLK_RATE(Clk_rate)    ((Clk_rate<((uint32_t)(configCPU_CLOCK_HZ/2)-1) \
                                           &&(Clk_rate>((uint32_t)(configCPU_CLOCK_HZ/5)-1)))


extern void uartx_init(UART_TYPE* UARTx, UART_CFG_Type* SDC_ConfigStruct);
extern void uartx_set_interrupt_type(UART_TYPE* UARTx, UART_IT it_type);
extern uint32_t uartx_is_interrupt_pending(UART_TYPE* UARTx);
extern UART_II uartx_get_interrupt_id(UART_TYPE* UARTx);
extern void uartx_sendchar(UART_TYPE* UARTx, uint8_t data);
extern char uartx_getchar(UART_TYPE* UARTx);
extern uint32_t uartx_send(UART_TYPE* UARTx, uint8_t* buf, uint32_t len);
extern uint32_t uartx_recv(UART_TYPE* UARTx, uint8_t* buf, uint32_t len);
extern void uartx_wait_tx_done(UART_TYPE* UARTx);
extern void uartx_clean_rxfifo(UART_TYPE* UARTx);
extern void uartx_clean_txfifo(UART_TYPE* UARTx);



#endif