//#include "uc_pulpino.h"
#include "uc_stack_trace.h"

#include "uc_submodule_def.h"
#include "trace_interface.h"




unsigned int get_mcause(void)
{
    //__asm__("csrr x15, mcause");

    
    unsigned int ret;
    __asm volatile("csrr %0,mcause":"=r"(ret));
    __asm__("csrr x15, mcause");
    return ret;
}

unsigned int get_mepc(void)
{
   // __asm__("csrr x15, mepc");
    
    unsigned int ret;
    __asm volatile("csrr %0,mepc":"=r"(ret));
    __asm__("csrr x15, mepc");
    return ret;
}

unsigned int get_sp(void)
{
     //__asm__("addi a5, sp, 0");
     
    unsigned int ret;
    __asm volatile("addi %0,sp,0":"=r"(ret));
    __asm__("addi a5, sp, 0");
    return ret;
}

#if 0

#define SET_STACK(command, data)   (set_dump_info(command, data) > 0?0:data)
void stack_trace(void)
{
	unsigned int temp, isr_sp;
        n8_t buf[68] = {0};
	
	TRACE_ERR_STATE_LOG(DEBUG_ERROR, START_OF_STACK_TRACE____); // start of stack trace>>>>
	
	temp = get_mcause();
	if (temp & 0x80000000) {
		temp &= 0xFF;
		if ((temp >= ISR_NB_START) || (temp <= ISR_NB_END)) {
                            //Modify by yinkz 20190917, bss section optimize
                            uint8_t isr_name[ISR_CNT_MAX][ISR_NAME_LENGTH_MAX] = {{ISR_I2C_STRING}, 
                            													  {ISR_UART0_STRING}, 
                            													  {ISR_UART1_STRING}, 
                            													  {ISR_GPIO_STRING}, 
                            													  {ISR_SPIM0_STRING}, 
                            													  {ISR_SPIM1_STRING},
                            													  {ISR_TA_OVF_STRING},
                            													  {ISR_TA_CMP_STRING},
                            													  {ISR_TB_OVF_STRING},
                            													  {ISR_TB_CMP_STRING},};
			TRACE_ERROR(DEBUG_ERROR, (n8_t*)&isr_name[temp - ISR_NB_START][0]);
		}
	}
	
	TRACE_ERR_STATE_LOG(DEBUG_ERROR, _CRS_REGISTER__); // <CRS register>:
	SPRINTF(buf, "mcuase:0x%08x mepc:0x%08x", SET_STACK(DUMP_MEPC, get_mcause()), get_mepc());
	TRACE_ERROR(DEBUG_ERROR, buf);
	//get_sp() 16 bytes stack, tack trace 16 bytes stack
	isr_sp = get_sp() + 32;
	
        SPRINTF(buf,"sp_addr:0x%08x", SET_STACK(DUMP_SP_ADDR, isr_sp));
        TRACE_ERROR(DEBUG_ERROR, buf);

	TRACE_ERR_STATE_LOG(DEBUG_ERROR, _RISCV_REGISTER__); // <RISCV register>:
	
	SPRINTF(buf,"ra(x1):0x%08x gp(x3): 0x%08x tp(x4):0x%08x t0(x5): 0x%08x", \
                    SET_STACK(DUMP_TRA, *(volatile unsigned int*) (isr_sp + 0x5c)), \
	           SET_STACK(DUMP_GP, *(volatile unsigned int*) (isr_sp + 0x00)), \
	            SET_STACK(DUMP_TP,  *(volatile unsigned int*) (isr_sp + 0x04)), \
	           SET_STACK(DUMP_T0,    *(volatile unsigned int*) (isr_sp + 0x08)));
         TRACE_ERROR(DEBUG_ERROR, buf);
	SPRINTF(buf,"t1(x6):0x%08x t2(x7):0x%08x a0(x10): 0x%08x a1(x11): 0x%08x", \
                    SET_STACK(DUMP_T1, *(volatile unsigned int*) (isr_sp + 0x0c)), \
                     SET_STACK(DUMP_T2,*(volatile unsigned int*) (isr_sp + 0x10)), \
                    SET_STACK(DUMP_A0, *(volatile unsigned int*) (isr_sp + 0x14)), \
                    SET_STACK(DUMP_A1, *(volatile unsigned int*) (isr_sp + 0x18)));
        TRACE_ERROR(DEBUG_ERROR, buf);
        SPRINTF(buf,"a2(x12):0x%08x a3(x13):0x%08x a4(x14):0x%08x a5(x15):0x%08x", \
                   SET_STACK(DUMP_A2, *(volatile unsigned int*) (isr_sp + 0x1c)), \
	           SET_STACK(DUMP_A3, *(volatile unsigned int*) (isr_sp + 0x20)), \
	           SET_STACK(DUMP_A4,  *(volatile unsigned int*) (isr_sp + 0x24)), \
	           SET_STACK(DUMP_A5,   *(volatile unsigned int*) (isr_sp + 0x28)));
    
        TRACE_ERROR(DEBUG_ERROR, buf);
        SPRINTF(buf,"a6(x16):0x%08x a7(x17):0x%08x t3(x28): 0x%08x t4(x29): 0x%08x", \
                     SET_STACK(DUMP_A6,   *(volatile unsigned int*) (isr_sp + 0x2c)), \
	            SET_STACK(DUMP_A7,  *(volatile unsigned int*) (isr_sp + 0x30)), \
	            SET_STACK(DUMP_T3,  *(volatile unsigned int*) (isr_sp + 0x34)), \
	            SET_STACK(DUMP_T4,  *(volatile unsigned int*) (isr_sp + 0x38)));
        TRACE_ERROR(DEBUG_ERROR, buf);
    
        SPRINTF(buf,"t5(x30):0x%08x t6(x31):0x%08x 0x7B0:0x%08x 0x7B1:0x%08x", \
                     SET_STACK(DUMP_T5, *(volatile unsigned int*) (isr_sp + 0x3c)), \
	            SET_STACK(DUMP_T6, *(volatile unsigned int*) (isr_sp + 0x40)), \
	                  *(volatile unsigned int*) (isr_sp + 0x44), \
	                  *(volatile unsigned int*) (isr_sp + 0x48));
        TRACE_ERROR(DEBUG_ERROR, buf);
        SPRINTF(buf,"0x7B2: 0x%08x 0x7B4:0x%08x 0x7B5:0x%08x 0x7B6:0x%08x",\
                        *(volatile unsigned int*) (isr_sp + 0x4c), \
	                  *(volatile unsigned int*) (isr_sp + 0x50), \
	                  *(volatile unsigned int*) (isr_sp + 0x54), \
	                  *(volatile unsigned int*) (isr_sp + 0x58));
        TRACE_ERROR(DEBUG_ERROR, buf);
        TRACE_ERR_STATE_LOG(DEBUG_ERROR, _FUNCTION_CALL__); // <Function call>:
	
        SPRINTF(buf,"0x%08x<--0x%08x", SET_STACK(DUMP_MEPC,get_mepc()) , SET_STACK(DUMP_SP,*(volatile unsigned int*) (isr_sp + 0x5c)));
        TRACE_ERROR(DEBUG_ERROR, buf);

        TRACE_ERR_STATE_LOG(DEBUG_ERROR, END_OF_STACK_TRACE____); // end of stack trace>>>>
        
        #ifdef  _FPGA_ 
        write_dump_info();
        #endif
}
#else

void stack_trace(void)
{
	unsigned int temp, isr_sp;
	
	TRACE_PRINTF("\r\nstart of stack trace>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n\r\n");
	
	temp = get_mcause();
	if (temp & 0x80000000) {
		temp &= 0xFF;
		if ((temp >= ISR_NB_START) || (temp <= ISR_NB_END)) {
			//TRACE_PRINTF("\t%s!\r\n", &isr_name[temp - ISR_NB_START][0]);
		}
	}
	
	TRACE_PRINTF("\r\n<CRS register>:\r\n\r\n\t");
	TRACE_PRINTF("mcuase:  0x%08x\t\t mepc:    0x%08x\t\t\r\n", get_mcause(), get_mepc());
	
	//get_sp() 16 bytes stack, tack trace 16 bytes stack
	isr_sp = get_sp() + 32;
	
        TRACE_PRINTF("\tsp_addr: 0x%08x\r\n", isr_sp);

	TRACE_PRINTF("\r\n<RISCV register>:\r\n\r\n");
	
	TRACE_PRINTF("\tra(x1):  0x%08x\t\t gp(x3):  0x%08x\t\t tp(x4):  0x%08x\t\t t0(x5):  0x%08x\t\t\r\n", *(volatile unsigned int*) (isr_sp + 0x5c), 
	                  *(volatile unsigned int*) (isr_sp + 0x00), *(volatile unsigned int*) (isr_sp + 0x04), *(volatile unsigned int*) (isr_sp + 0x08));
	TRACE_PRINTF("\tt1(x6):  0x%08x\t\t t2(x7):  0x%08x\t\t a0(x10): 0x%08x\t\t a1(x11): 0x%08x\t\t\r\n", *(volatile unsigned int*) (isr_sp + 0x0c), 
	                  *(volatile unsigned int*) (isr_sp + 0x10), *(volatile unsigned int*) (isr_sp + 0x14), *(volatile unsigned int*) (isr_sp + 0x18));
        TRACE_PRINTF("\ta2(x12): 0x%08x\t\t a3(x13): 0x%08x\t\t a4(x14): 0x%08x\t\t a5(x15): 0x%08x\t\t\r\n", *(volatile unsigned int*) (isr_sp + 0x1c), 
	                  *(volatile unsigned int*) (isr_sp + 0x20), *(volatile unsigned int*) (isr_sp + 0x24), *(volatile unsigned int*) (isr_sp + 0x28));
	TRACE_PRINTF("\ta6(x16): 0x%08x\t\t a7(x17): 0x%08x\t\t t3(x28): 0x%08x\t\t t4(x29): 0x%08x\t\t\r\n", *(volatile unsigned int*) (isr_sp + 0x2c), 
	                  *(volatile unsigned int*) (isr_sp + 0x30), *(volatile unsigned int*) (isr_sp + 0x34), *(volatile unsigned int*) (isr_sp + 0x38));
	TRACE_PRINTF("\tt5(x30): 0x%08x\t\t t6(x31): 0x%08x\t\t 0x7B0:   0x%08x\t\t 0x7B1:   0x%08x\t\t\r\n", *(volatile unsigned int*) (isr_sp + 0x3c), 
	                  *(volatile unsigned int*) (isr_sp + 0x40), *(volatile unsigned int*) (isr_sp + 0x44), *(volatile unsigned int*) (isr_sp + 0x48));
	TRACE_PRINTF("\t0x7B2:   0x%08x\t\t 0x7B4:   0x%08x\t\t 0x7B5:   0x%08x\t\t 0x7B6:   0x%08x\t\t\r\n", *(volatile unsigned int*) (isr_sp + 0x4c), 
	                  *(volatile unsigned int*) (isr_sp + 0x50), *(volatile unsigned int*) (isr_sp + 0x54), *(volatile unsigned int*) (isr_sp + 0x58));

	TRACE_PRINTF("\r\n<Function call>:\r\n\r\n\t");
	
	TRACE_PRINTF("0x%08x<--0x%08x", get_mepc(), *(volatile unsigned int*) (isr_sp + 0x5c));
	


	TRACE_PRINTF("\r\n\r\nend of stack trace>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n\r\n");
}
#endif
