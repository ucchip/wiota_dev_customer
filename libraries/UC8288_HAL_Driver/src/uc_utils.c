#include <uc_utils.h>
#include <uc_pulpino.h>
#include <stdint.h>
#include <uc_string_lib.h>
#include <uc_uart.h>
#include <uc_gpio.h>

// exit loop
void exit (int i)
{
    eoc(i);
    while (1);
}

// signal end of computation
void eoc(int i)
{
}

// sleep for 'iter' iterations. each iteration is approx 10 cycles
void sleep_busy(volatile int iter)
{
    for (int i = 0; i < iter; i++)
    { asm volatile ("nop"); }
}
