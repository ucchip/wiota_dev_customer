// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

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
