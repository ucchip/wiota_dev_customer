#ifndef SMARTCARD_H
#define SMARTCARD_H

#include "uc_pulpino.h"
#include <stdint.h>

#define SUCCESS            (uint8_t)0x0
#define ERROR_TIMEOUT      (uint8_t)0x1
#define ERROR_ATR          (uint8_t)0x2

#define SMARTCARD_REG_RBR ( SMARTCARD_BASE_ADDR + 0x00) // Receiver Buffer Register (Read Only)
#define SMARTCARD_REG_DLL ( SMARTCARD_BASE_ADDR + 0x00) // Divisor Latch (LS)
#define SMARTCARD_REG_THR ( SMARTCARD_BASE_ADDR + 0x00) // Transmitter Holding Register (Write Only)
#define SMARTCARD_REG_DLM ( SMARTCARD_BASE_ADDR + 0x04) // Divisor Latch (MS)
#define SMARTCARD_REG_IER ( SMARTCARD_BASE_ADDR + 0x04) // Interrupt Enable Register
#define SMARTCARD_REG_IIR ( SMARTCARD_BASE_ADDR + 0x08) // Interrupt Identity Register (Read Only)
#define SMARTCARD_REG_FCR ( SMARTCARD_BASE_ADDR + 0x08) // FIFO Control Register (Write Only)
#define SMARTCARD_REG_LCR ( SMARTCARD_BASE_ADDR + 0x0C) // Line Control Register
#define SMARTCARD_REG_MCR ( SMARTCARD_BASE_ADDR + 0x10) // MODEM Control Register
#define SMARTCARD_REG_LSR ( SMARTCARD_BASE_ADDR + 0x14) // Line Status Register
#define SMARTCARD_REG_MSR ( SMARTCARD_BASE_ADDR + 0x18) // MODEM Status Register
#define SMARTCARD_REG_SCR ( SMARTCARD_BASE_ADDR + 0x1C) // Scratch Register
#define SMARTCARD_REG_DIR ( SMARTCARD_BASE_ADDR + 0x28)
#define SMARTCARD_REG_RCR ( SMARTCARD_BASE_ADDR + 0x2C)

#define RBR_SMARTCARD REGP_8(SMARTCARD_REG_RBR)
#define DLL_SMARTCARD REGP_8(SMARTCARD_REG_DLL)
#define THR_SMARTCARD REGP_8(SMARTCARD_REG_THR)
#define DLM_SMARTCARD REGP_8(SMARTCARD_REG_DLM)
#define IER_SMARTCARD REGP_8(SMARTCARD_REG_IER)
#define IIR_SMARTCARD REGP_8(SMARTCARD_REG_IIR)
#define FCR_SMARTCARD REGP_8(SMARTCARD_REG_FCR)
#define LCR_SMARTCARD REGP_8(SMARTCARD_REG_LCR)
#define MCR_SMARTCARD REGP_8(SMARTCARD_REG_MCR)
#define LSR_SMARTCARD REGP_8(SMARTCARD_REG_LSR)
#define MSR_SMARTCARD REGP_8(SMARTCARD_REG_MSR)
#define SCR_SMARTCARD REGP_8(SMARTCARD_REG_SCR)

#define DLAB 1<<7   //DLAB bit in LCR reg
#define ERBFI 1     //ERBFI bit in IER reg
#define ETBEI 1<<1  //ETBEI bit in IER reg
#define PE 1<<2     //PE bit in LSR reg
#define THRE 1<<5   //THRE bit in LSR reg
#define DR 1        //DR bit in LSR reg

#define SMARTCARD_FIFO_DEPTH 64

//SMARTCARD_FIFO_DEPTH but to be compatible with Arduino_libs and also if in future designs it differed
#define SERIAL_RX_BUFFER_SIZE SMARTCARD_FIFO_DEPTH
#define SERIAL_TX_BUFFER_SIZE SMARTCARD_FIFO_DEPTH

void smartcard_set_cfg(int parity, uint16_t clk_counter);
uint8_t smartcard_send(char* str, unsigned int len);
uint8_t smartcard_getchar(char* data, uint32_t timeout);
void smartcard_sendchar(char data);

void smartcard_wait_tx_done(void);
void smartcard_set(void);
void smartcard_reset(void);
void smartcard_clean_fifo(void);

#endif