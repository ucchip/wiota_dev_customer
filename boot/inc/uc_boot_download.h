#ifndef __UC_BOOT_DOWNLOAD_H__
#define __UC_BOOT_DOWNLOAD_H__

#include <stdint.h>

#define FLASH_PAGE_LEN 4096
#define MAX_DOWN_FILE ((512/2 - 8/2) * 1024)
//258048 -> 3f000
#define BACK_FLASH_ADDRESS ((512/2 - 8/2) * 1024)
#define ENTERMODEM "+CHOOSEMODEM:D\r\n"
#define SYSTEMSTART "+SYSTEM:START\r\n"

typedef enum
{
    BOOT_SHARE_DEFAULT = 1,
    BOOT_SHARE_ENTER_DOWNLOAD,
    
} BOOT_SHARE_FLAG;

typedef enum
{
    BOOT_SHARE_UART_BAUD_DEFAULT = 1,
    BOOT_SHARE_UART_BAUD_HAVE_SET,
    
} BOOT_SHARE_UART_FLAG;


typedef struct
{
    unsigned int baud_flag;
    unsigned int baud_rate;
    unsigned int flag;
} share_data;

/*
* parment    mean
*     0              no input state
*     1             input current state
*/
void boot_download(int input_flag);
void boot_set_uart0_baud_rate(unsigned int  baud_rate);
void boot_riscv_reboot(void);
void boot_set_modem(unsigned char modem);

#endif
