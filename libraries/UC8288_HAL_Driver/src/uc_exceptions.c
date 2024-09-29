#include "uc_string_lib.h"
#include "uc_utils.h"
#include "uc_stack_trace.h"
#include "uc_stack_trace.h"
// use weak attribute here, so we can overwrite this function to provide custom exception handlers, e.g. for tests
//__attribute__((interrupt)) __attribute__((weak))
void default_exception_handler_c(void)
{
   // stack_trace();
    IER = 0x0;
    IPR = 0x0;
    for (;;);
}

// use weak attribute here, so we can overwrite this function to provide custom exception handlers, e.g. for tests
//__attribute__((interrupt)) __attribute__((weak))
void illegal_insn_handler_c(void)
{
    //stack_trace();
    IER = 0x0;
    IPR = 0x0;
    for (;;);
}
// use weak attribute here, so we can overwrite this function to provide custom exception handlers, e.g. for tests
//__attribute__((interrupt)) __attribute__((weak))
unsigned long ecall_insn_handler_c(long epc)
{
    //stack_trace();
    //printf("ecall to TBD\r\n");
    //eturn epc + 4;
    IER = 0x0;
    IPR = 0x0;
    for(;;);
}
