#include <stdio.h>
#include <stdint.h>
#include "uc_smartcard.h"
#include "uc_utils.h"
#include "uc_pulpino.h"
//#include "smartcard_init.h"

void smartcard_set_cfg(int parity, uint16_t clk_counter)
{
    uint32_t* pad_mux = (uint32_t*)0x1a107000;
    uint32_t* pad_cfg = (uint32_t*)0x1a107020;

    *(volatile unsigned int*)(pad_mux) = 0xe0;
    pad_cfg += 1;

    *(volatile unsigned int*)(pad_cfg) = 0x0000100;
    *(volatile unsigned int*)(SMARTCARD_REG_LCR) = 0x83; //sets 8N1 and set DLAB to 1
    *(volatile unsigned int*)(SMARTCARD_REG_DLM) = (clk_counter >> 8) & 0xFF;
    *(volatile unsigned int*)(SMARTCARD_REG_DLL) =  clk_counter       & 0xFF;
    *(volatile unsigned int*)(SMARTCARD_REG_FCR) = 0xC7; //enables 16byte FIFO and clear FIFOs
    *(volatile unsigned int*)(SMARTCARD_REG_LCR) = 0x0f; //sets 8N1 and set DLAB to 0

    *(volatile unsigned int*)(SMARTCARD_REG_DIR) = 3;
    *(volatile unsigned int*)(SMARTCARD_REG_IER) = ((*(volatile unsigned int*)(SMARTCARD_REG_IER)) & 0xF0) | 0x02; // set IER (interrupt enable register) on UART
}

void smartcard_set(void)
{
    *(volatile unsigned int*)(SMARTCARD_REG_RCR) = 0x01;
}

void smartcard_reset(void)
{
    *(volatile unsigned int*)(SMARTCARD_REG_RCR) = 0x00;
}

uint8_t smartcard_send(char* str, unsigned int len)
{
    unsigned int i;

    while (len > 0)
    {
        // process this in batches of 16 bytes to actually use the FIFO in the UART
        // wait until there is space in the fifo
        while ( (*(volatile unsigned int*)(SMARTCARD_REG_LSR) & 0x20) == 0);

        for (i = 0; (i < SMARTCARD_FIFO_DEPTH) && (len > 0); i++)
        {
            // load FIFO
            *(volatile unsigned int*)(SMARTCARD_REG_THR) = *str++;
            smartcard_wait_tx_done();
            len--;
        }
    }
    return 1;
}

uint8_t smartcard_getchar(char* data, uint32_t timeout)
{
    char temp = 0;
    uint32_t count = 0;

    do
    {
        count++;
        temp = (*(volatile unsigned int*)(SMARTCARD_REG_LSR));
    } while (((temp & 0x1) != 0x1) && (count < timeout));

    if (count >= timeout)
    {
        return 0;
    }

    *data =  *(volatile int*)SMARTCARD_REG_RBR;
    return 1;
}

void smartcard_sendchar(char data)
{
    // wait until there is space in the fifo
    while ( (*(volatile unsigned int*)(SMARTCARD_REG_LSR) & 0x20) == 0);

    // load FIFO
    *(volatile unsigned int*)(SMARTCARD_REG_THR) = data;
}

void smartcard_wait_tx_done(void)
{
    // wait until there is space in the fifo
    while ( (*(volatile unsigned int*)(SMARTCARD_REG_LSR) & 0x40) == 0);
}

void smartcard_clean_fifo(void)
{
    *(volatile unsigned int*)(SMARTCARD_REG_FCR) = 0xC7; //enables 16byte FIFO and clear FIFOs
}
