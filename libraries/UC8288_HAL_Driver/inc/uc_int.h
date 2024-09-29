#ifndef _INT_H_
#define _INT_H_

/* Number of interrupt handlers - really depends on PIC width in OR1200*/
#define MAX_INT_HANDLERS  32

#define int_disable()   asm ( "csrci mstatus, 0x08")
#define int_enable()    asm ( "csrsi mstatus, 0x08")

//declearing all interrupt handelrs
//these functions can be redefined by users


void ISR_RTC (void);
void ISR_PLS_CNT_CMP (void);  // 2: pulse 32k compare
void ISR_UDMA (void); // 3: udma
void ISR_CNT_32K_CMP(void); // 5: count 32k compare
void ISR_I2C (void);    // 22: i2c
void ISR_UART0 (void);  // 23: uart0
void ISR_UART1 (void);  // 24: uart1
void ISR_GPIO (void);   // 25: gpio
void ISR_SPIM0 (void);  // 26: spim end of transmission
void ISR_SPIM1 (void);  // 27: spim R/T finished
void ISR_TA_OVF (void); // 28: timer A overflow
void ISR_TA_CMP (void); // 29: timer A compare
void ISR_TB_OVR (void); // 30: timer B overflow
void ISR_TB_CMP (void); // 31: timer B compare



#endif // _INT_H_
