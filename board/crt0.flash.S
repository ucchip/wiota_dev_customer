// Copyright 2016 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

 
#define EXCEPTION_STACK_SIZE 96


/* ========================================================= [ entry ] === */
.section .reset  
reset_handler:
  csrw mtvec, x0
  csrci  mstatus, 0x08
  /* set all registers to zero */
  mv  x1, x0
  mv  x2, x1
  mv  x3, x1
  mv  x4, x1
  mv  x5, x1
  mv  x6, x1  
  mv  x7, x1 
  mv  x8, x1  
  mv  x9, x1 
  mv x10, x1
  mv x11, x1
  mv x12, x1
  mv x13, x1
  mv x14, x1
  mv x15, x1
  mv x16, x1
  mv x17, x1
  mv x18, x1
  mv x19, x1
  mv x20, x1
  mv x21, x1
  mv x22, x1
  mv x23, x1
  mv x24, x1
  mv x25, x1
  mv x26, x1
  mv x27, x1
  mv x28, x1
  mv x29, x1
  mv x30, x1
  mv x31, x1

  /* stack initilization */
  la   x2, _stack_start


_start:
  .global _start
  li a0, 0 
  jal init_puf 
  /* clear BSS */
  la x26, _bss_start
  la x27, _bss_end

  bge x26, x27, zero_loop_end

zero_loop:
  sw x0, 0(x26)
  addi x26, x26, 4
  ble x26, x27, zero_loop
zero_loop_end:

  /* Run global initialization functions */
  li a0, 1                      /* set app mode */
  call  set_program_type
  call  boot_noop
  call  boot_strap
  li a0, 1
  call  boot_download
  call  __libc_init_array
  j main_entry
  

.section .crt0, "ax" 

main_entry:
  addi   x10, x0, 0
  /* Baud Rate 156250 
  *clock divider, SYSCLK/156250/16-1
  *5MHZ 1; 50MHZ 19
  * 103.68Mhz clk, 115200 sv model 89
  * 19.6608Mhz clk, VHD model, value 4. VHD
  * 196/2Mhz   VHD model value 84 for 115200
  */
  //addi   x11, x0, 84 //98Mhz, 1152000 for sim
  //addi   x11, x0, 70  //131.072Mhz, 115200 for sim
  //addi   x11, x0, 22 //26M DCXO, just leave it here, not necessary
  //jal  uart_set_cfg 
  
  /* jump to main program entry point (argc = argv = 0) */
  addi x10, x0, 0
  addi x11, x0, 0
  jal x1, entry

  jal  uart_wait_tx_done;

  /* if program exits call exit routine from library */
  jal  x1, exit

/* ================================= [ illegal instruction handler] === */
illegal_insn_handler:
  addi x2, x2, -EXCEPTION_STACK_SIZE
  sw x1, 0x5C(x2)
  jal x1, store_regs
  la x1, end_except
  jal x0, illegal_insn_handler_c

/* Proper way to deal with ecall
 * but debug bridge still has problem
 * and single M mode machine doesn't need
 * though...*/
ecall_insn_handler:
  addi x2, x2, -EXCEPTION_STACK_SIZE
  sw x1, 0x5C(x2)
  jal x1, store_regs
  csrr x10, mepc
  jal x0, ecall_insn_handler_c
  csrw mepc, x10
  jal x0, end_except
  addi x2, x2, EXCEPTION_STACK_SIZE
  mret

default_exception_handler:
  addi x2, x2, -EXCEPTION_STACK_SIZE
  sw x1, 0x5C(x2)
  jal x1, store_regs
  la x1, end_except
  jal x0, default_exception_handler_c
// saves all caller-saved registers (except return address)
store_regs:
  sw  x3, 0x00(x2)  // gp
  sw  x4, 0x04(x2)  // tp
  sw  x5, 0x08(x2)  // t0
  sw  x6, 0x0c(x2)  // t1
  sw  x7, 0x10(x2)  // t2
  sw x10, 0x14(x2)  // a0
  sw x11, 0x18(x2)  // a1
  sw x12, 0x1c(x2)  // a2
  sw x13, 0x20(x2)  // a3
  sw x14, 0x24(x2)  // a4
  sw x15, 0x28(x2)  // a5
  sw x16, 0x2c(x2)  // a6
  sw x17, 0x30(x2)  // a7
  sw x28, 0x34(x2)  // t3
  sw x29, 0x38(x2)  // t4
  sw x30, 0x3c(x2)  // t5
  sw x31, 0x40(x2)  // t6
  csrr x28, 0x7B0
  csrr x29, 0x7B1
  csrr x30, 0x7B2
  sw x28, 0x44(x2)  // lpstart[0]
  sw x29, 0x48(x2)  // lpend[0]
  sw x30, 0x4C(x2)  // lpcount[0]
  csrr x28, 0x7B4
  csrr x29, 0x7B5
  csrr x30, 0x7B6
  sw x28, 0x50(x2)  // lpstart[1]
  sw x29, 0x54(x2)  // lpend[1]
  sw x30, 0x58(x2)  // lpcount[1]
  //csrr x5, mepc
  //sw  x5, 0x60(x2)  //mepc
  jalr x0, x1
  
// load back registers from stack
end_except:
  lw  x5, 0x60(x2)  //mepc
  csrw mepc, x5
  lw x28, 0x50(x2)  // lpstart[1]
  lw x29, 0x54(x2)  // lpend[1]
  lw x30, 0x58(x2)  // lpcount[1]
  csrrw x0, 0x7B4, x28
  csrrw x0, 0x7B5, x29
  csrrw x0, 0x7B6, x30
  lw x28, 0x44(x2)  // lpstart[0]
  lw x29, 0x48(x2)  // lpend[0]
  lw x30, 0x4C(x2)  // lpcount[0]
  csrrw x0, 0x7B0, x28
  csrrw x0, 0x7B1, x29
  csrrw x0, 0x7B2, x30
  lw  x3, 0x00(x2)
  lw  x4, 0x04(x2)
  lw  x5, 0x08(x2)
  lw  x6, 0x0c(x2)
  lw  x7, 0x10(x2)
  lw x10, 0x14(x2)
  lw x11, 0x18(x2)
  lw x12, 0x1c(x2)
  lw x13, 0x20(x2)
  lw x14, 0x24(x2)
  lw x15, 0x28(x2)
  lw x16, 0x2c(x2)
  lw x17, 0x30(x2)
  lw x28, 0x34(x2)
  lw x29, 0x38(x2)
  lw x30, 0x3c(x2)
  lw x31, 0x40(x2)
  lw  x1, 0x5C(x2)
  addi x2, x2, EXCEPTION_STACK_SIZE
  mret

    
/* ================== [ all interrupt handler] =========== */
.weak default_int_handler
default_int_handler:
  ret

ISR_ALL_ASM:
  // save all from thread context
    addi sp, sp, -32 * 4
    sw x5,   5 * 4(sp)
    sw x1,   1 * 4(sp)
    li t0, 0x80
    sw t0,   2 * 4(sp)
    sw x4,   4 * 4(sp)   
    sw x6,   6 * 4(sp)
    sw x7,   7 * 4(sp)
    sw x8,   8 * 4(sp)
    sw x9,   9 * 4(sp)
    sw x10, 10 * 4(sp)
    sw x11, 11 * 4(sp)
    sw x12, 12 * 4(sp)
    sw x13, 13 * 4(sp)
    sw x14, 14 * 4(sp)
    sw x15, 15 * 4(sp)
    sw x16, 16 * 4(sp)
    sw x17, 17 * 4(sp)
    sw x18, 18 * 4(sp)
    sw x19, 19 * 4(sp)
    sw x20, 20 * 4(sp)
    sw x21, 21 * 4(sp)
    sw x22, 22 * 4(sp)
    sw x23, 23 * 4(sp)
    sw x24, 24 * 4(sp)
    sw x25, 25 * 4(sp)
    sw x26, 26 * 4(sp)
    sw x27, 27 * 4(sp)
    sw x28, 28 * 4(sp)
    sw x29, 29 * 4(sp)
    sw x30, 30 * 4(sp)
    sw x31, 31 * 4(sp)

ISR_ALL_ASM_BAK:
    // switch to interrupt stack
    move s0, sp
    la   sp, _sp

    // interrupt handle
    call rt_interrupt_enter
    csrr a0, mcause
    csrr a1, mepc
    mv a2, sp
    call default_int_handler
    call rt_interrupt_leave

    // switch to from thread stack
    move sp, s0

    // need to switch new thread
    la   s0, rt_thread_switch_interrupt_flag
    lw   s2, 0(s0)
    beqz s2, spurious_interrupt_all
    sw   zero, 0(s0)

    csrr a0, mepc
    sw a0,  0 * 4(sp)

    la   s0, rt_interrupt_from_thread
    lw   s1, 0(s0)
    sw   sp, 0(s1)

    la   s0, rt_interrupt_to_thread
    lw   s1, 0(s0)
    lw   sp, 0(s1)

    lw  a0,  0 * 4(sp)
    csrw mepc, a0

spurious_interrupt_all:
    lw x1,   1 * 4(sp)
    
    // Remain in M-mode after mret
    li t0, 0x00001800
    csrs mstatus, t0
    lw t0,   2 * 4(sp)
    csrs mstatus, t0 
    
    lw x4,   4 * 4(sp)
    lw x5,   5 * 4(sp)
    lw x6,   6 * 4(sp)
    lw x7,   7 * 4(sp)
    lw x8,   8 * 4(sp)
    lw x9,   9 * 4(sp)
    lw x10, 10 * 4(sp)
    lw x11, 11 * 4(sp)
    lw x12, 12 * 4(sp)
    lw x13, 13 * 4(sp)
    lw x14, 14 * 4(sp)
    lw x15, 15 * 4(sp)
    lw x16, 16 * 4(sp)
    lw x17, 17 * 4(sp)
    lw x18, 18 * 4(sp)
    lw x19, 19 * 4(sp)
    lw x20, 20 * 4(sp)
    lw x21, 21 * 4(sp)
    lw x22, 22 * 4(sp)
    lw x23, 23 * 4(sp)
    lw x24, 24 * 4(sp)
    lw x25, 25 * 4(sp)
    lw x26, 26 * 4(sp)
    lw x27, 27 * 4(sp)
    lw x28, 28 * 4(sp)
    lw x29, 29 * 4(sp)
    lw x30, 30 * 4(sp)
    lw x31, 31 * 4(sp)

    addi sp, sp, 32 * 4
    mret 

/* ================== [ all interrupt handler end] =========== */

  .global _init
  .global _fini
_init:
_fini:
  # These don't have to do anything since we use init_array/fini_array.
  ret

/* =================================================== [ exceptions ] === */
/* This section has to be down here, since we have to disable rvc for it  */

  .section .vectors, "ax"
  .option norvc;

  // external interrupts are handled by the same callback
  // until compiler supports IRQ routines
  .org 0x00
  j ISR_ALL_ASM //ISR_RTC_ASM
  j ISR_ALL_ASM //CCE_NOTIFY          //CCE
  j ISR_ALL_ASM //ISR_PLS_CNT_CMP_AMS     //2: pulse counter compare
  j ISR_ALL_ASM //ISR_UDMA_ASM            //3: udma
  j ISR_ALL_ASM //ISR_ACC_DEN_ASM            //4: access denied
  j ISR_ALL_ASM //ISR_CNT_32K_CMP_AMS     //5: counter 32k compare 
  .rept 14
   nop                       // unused
  .endr
//.global ISA_TA_CMP_ASM  
  j   ISR_ALL_ASM //ISR_ADC_ASM           // 20: ADC
  j   ISR_ALL_ASM //ISR_DAC_ASM           // 21: DAC
  j   ISR_ALL_ASM //ISR_I2C_ASM 	    // 22: i2c
  j   ISR_ALL_ASM //ISR_UART_ASM 	    // 23: uart[1]
  j   ISR_ALL_ASM //ISR_UART_ASM 	    // 24: uart[0]
  j   ISR_ALL_ASM //ISR_GPIO_ASM 	    // 25: gpio
  j   ISR_ALL_ASM //ISR_SPIM0_ASM     // 26: spim	end of transmission
  j   ISR_ALL_ASM //ISR_SPIM1_ASM     // 27: spim R/T finished
  j   ISR_ALL_ASM //ISR_TA_OVF_ASM    // 28: timer A overflow
  j   ISR_ALL_ASM //ISR_TA_CMP_ASM    // 29: timer A compare 
  j   ISR_ALL_ASM //ISR_TB_OVF_ASM    // 30: timer B overflow
  j   ISR_ALL_ASM //ISR_TB_CMP_ASM    // 31: timer B compare


  // reset vector
  .org 0x80
  j   reset_handler

  // illegal instruction exception
  .org 0x84
  //illegal_insn_handler:
  j   illegal_insn_handler_c

  // ecall handler
  /* pulpino doesn't really need this
   * since it's always in M mode.
   * but we do not want system hang...
   */
  .org 0x88
   //should return mepc + 4;...
   //ecall_insn_handler:
   j   ecall_insn_handler_c
  .org 0x8c
   //LSU_E:
   j default_exception_handler_c
  /* magic word for spi dummy autoadapt */
  .word 0xefbeadde
 
