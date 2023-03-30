
//#include <interrupt.h>
#include <rthw.h>

#include <board.h>
//#include <platform.h>
//#include <encoding.h>
//#include <interrupt.h>

#include "uc_pulpino.h"
#include <uc_utils.h>
#include <uc_event.h>
#include "uc_timer.h"
#include "uc_spi_flash.h"

#if 0
extern void use_default_clocks(void);
extern void use_pll(int refsel, int bypass, int r, int f, int q);

#define TICK_COUNT  (2 * RTC_FREQ / RT_TICK_PER_SECOND)

#define MTIME       (*((volatile uint64_t *)(CLINT_CTRL_ADDR + CLINT_MTIME)))
#define MTIMECMP    (*((volatile uint64_t *)(CLINT_CTRL_ADDR + CLINT_MTIMECMP)))

/* system tick interrupt */
void handle_m_time_interrupt()
{
    MTIMECMP = MTIME + TICK_COUNT;
    rt_tick_increase();
}

/* fixed misaligned bug for qemu */
void* __wrap_memset(void* s, int c, size_t n)
{
    return rt_memset(s, c, n);
}

static void rt_hw_clock_init(void)
{
    use_default_clocks();
    use_pll(0, 0, 1, 31, 1);
}

static void rt_hw_timer_init(void)
{
    MTIMECMP = MTIME + TICK_COUNT;

    /*  enable timer interrupt*/
    set_csr(mie, MIP_MTIP);
}
#else
/* fixed misaligned bug for qemu */
void* __wrap_memset(void* s, int c, size_t n)
{
    return rt_memset(s, c, n);
}
#endif

void timer0_compare_handler(void)
{
    //timer_int_clear_pending(UC_TIMER0, TIMER_IT_CMP);

    rt_tick_increase();
    timer_set_count(UC_TIMER0, 0);
}

//#define configCPU_CLOCK_HZ            ( ( unsigned long ) 4860000)
//#define configTICK_RATE_DIV           ( 500 )   //Div=Tick*PreScalar 2500?
#define configCPU_CLOCK_HZ          ( ( unsigned long ) BSP_CLOCK_SYSTEM_FREQ_HZ)
#define configTICK_RATE_DIV         ( RT_TICK_PER_SECOND )

void rt_hw_systick_reinit(uint32_t freq_div)
{
    /* Setup Timer A */
    TIMER_CFG_Type TIMERX_InitStructure;

    timer_int_disable(UC_TIMER0, TIMER_IT_CMP);
    timer_disable(UC_TIMER0);
    timer_int_clear_pending(UC_TIMER0, TIMER_IT_CMP);

    /* set compare value*/
    TIMERX_InitStructure.cmp = configCPU_CLOCK_HZ / (freq_div * 5 * configTICK_RATE_DIV);

    /* Timer0 start timer */
    timer_deinit(UC_TIMER0, &TIMERX_InitStructure);
    timer_enable(UC_TIMER0);

    /* Enable TA IRQ */
    timer_int_enable(UC_TIMER0, TIMER_IT_CMP);
}

static void rt_hw_systick_init(void)
{
    /* Setup Timer A */
    TIMER_CFG_Type TIMERX_InitStructure;

    /* set time count*/
    TIMERX_InitStructure.cnt = 0x0;

    /* set compare value*/
    TIMERX_InitStructure.cmp = configCPU_CLOCK_HZ / (5 * configTICK_RATE_DIV);

    /* set prescaler value*/
    TIMERX_InitStructure.pre = 4;

    /* Timer0 start timer */
    timer_init(UC_TIMER0, &TIMERX_InitStructure);
    timer_enable(UC_TIMER0);

    /* Enable TA IRQ */
    timer_int_enable(UC_TIMER0, TIMER_IT_CMP);
}

void rt_hw_board_init(void)
{
    /* initialize the system clock */
    //rt_hw_clock_init();

    /* initialize hardware interrupt */
    //rt_hw_interrupt_init();

#ifdef RT_USING_HEAP
    rt_memset((void*)HEAP_BEGIN, 0x00, (uint32_t)HEAP_END - (uint32_t)HEAP_BEGIN);
    rt_system_heap_init((void*)HEAP_BEGIN, (void*)HEAP_END);
#endif

    /* initialize timer0 */
    rt_hw_systick_init();

    /* Pin driver initialization is open by default */
#ifdef RT_USING_PIN
    extern int rt_hw_pin_init(void);
    rt_hw_pin_init();
#endif

#ifdef RT_USING_SERIAL
    extern int rt_hw_usart_init(void);
    rt_hw_usart_init();
    //extern int virtual_usart_init(void);
    //virtual_usart_init();
#endif

#ifdef RT_USING_CONSOLE
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

    return;
}

void rt_hw_cpu_reset(void)
{
    volatile uint32_t* pmu_ctrl = (uint32_t*)(PMU_BASE_ADDR + 0x000);
    volatile uint32_t* reg_xip_ctrl = (volatile uint32_t*)0x1a10c02c;

    //WAIT_XIP_FREE
    while ((*reg_xip_ctrl) & 0x1);
    *pmu_ctrl |= 1 << 14;
}

#ifdef RT_USING_FINSH
#include <finsh.h>
static void reboot(uint8_t argc, char** argv)
{
    rt_hw_cpu_reset();
}
FINSH_FUNCTION_EXPORT_ALIAS(reboot, __cmd_reboot, Reboot System);
#endif /* RT_USING_FINSH */


