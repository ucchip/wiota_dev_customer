#ifndef _STACK_TRACE_H_
#define _STACK_TRACE_H_



#define ISR_NB_START         22
#define ISR_NB_END           31

#define ISR_NAME_LENGTH_MAX  16
#define ISR_CNT_MAX          16

#define DATA_ROM_END         0x300000
#define SP_BUTTOM_ADDR       0x33ec00
#define ISR_STACK_SIZE       0x64

void stack_trace(void);

#endif
