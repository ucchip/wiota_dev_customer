

#ifndef __BOOT_UART_H__
#define __BOOT_UART_H__


#include "uc_pulpino.h"

#define UC_BOOT_TIMEOUT_S 8750000
#define UART_FIFO_DEPTH 64
#define UART_REG_RBR ( UART_BASE_ADDR + 0x00) // Receiver Buffer Register (Read Only)
#define UART_REG_DLL ( UART_BASE_ADDR + 0x00) // Divisor Latch (LS)
#define UART_REG_THR ( UART_BASE_ADDR + 0x00) // Transmitter Holding Register (Write Only)
#define UART_REG_DLM ( UART_BASE_ADDR + 0x04) // Divisor Latch (MS)
#define UART_REG_IER ( UART_BASE_ADDR + 0x04) // Interrupt Enable Register
#define UART_REG_IIR ( UART_BASE_ADDR + 0x08) // Interrupt Identity Register (Read Only)
#define UART_REG_FCR ( UART_BASE_ADDR + 0x08) // FIFO Control Register (Write Only)
#define UART_REG_LCR ( UART_BASE_ADDR + 0x0C) // Line Control Register
#define UART_REG_MCR ( UART_BASE_ADDR + 0x10) // MODEM Control Register
#define UART_REG_LSR ( UART_BASE_ADDR + 0x14) // Line Status Register
#define UART_REG_MSR ( UART_BASE_ADDR + 0x18) // MODEM Status Register
#define UART_REG_SCR ( UART_BASE_ADDR + 0x1C) // Scratch Register

#define UART1_FIFO_DEPTH 64
#define UART1_REG_RBR ( UART1_BASE_ADDR + 0x00) // Receiver Buffer Register (Read Only)
#define UART1_REG_DLL ( UART1_BASE_ADDR + 0x00) // Divisor Latch (LS)
#define UART1_REG_THR ( UART1_BASE_ADDR + 0x00) // Transmitter Holding Register (Write Only)
#define UART1_REG_DLM ( UART1_BASE_ADDR + 0x04) // Divisor Latch (MS)
#define UART1_REG_IER ( UART1_BASE_ADDR + 0x04) // Interrupt Enable Register
#define UART1_REG_IIR ( UART1_BASE_ADDR + 0x08) // Interrupt Identity Register (Read Only)
#define UART1_REG_FCR ( UART1_BASE_ADDR + 0x08) // FIFO Control Register (Write Only)
#define UART1_REG_LCR ( UART1_BASE_ADDR + 0x0C) // Line Control Register
#define UART1_REG_MCR ( UART1_BASE_ADDR + 0x10) // MODEM Control Register
#define UART1_REG_LSR ( UART1_BASE_ADDR + 0x14) // Line Status Register
#define UART1_REG_MSR ( UART1_BASE_ADDR + 0x18) // MODEM Status Register
#define UART1_REG_SCR ( UART1_BASE_ADDR + 0x1C) // Scratch Register


#define LOG(a) boot_uart_sendstr((const char *)a, strlen(a))

void boot_uart1_set_cfg(int parity, uint16_t clk_counter);
void boot_uart1_sendchar(const char c);
void boot_uart1_wait_tx_done(void);

void boot_uart_set_cfg(int parity, uint16_t clk_counter);

void boot_uart_sendchar(const char c);
void boot_uart_wait_tx_done(void);
void boot_uart_sendstr(const char* str, unsigned int len);
signed char boot_uart_recvchar(char *get_data, int timeout);
int boot_uart_recvstr(char* str, unsigned int len, int timeout);
signed char boot_uart_get(void);

#endif
