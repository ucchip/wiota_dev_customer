#include <rtthread.h>
#ifdef UC_SPIM_SUPPORT
#include <rtdevice.h>
#include "spim.h"

static void spim_mem_mode_set(void)
{
    int data = 31 << 24;

    spim_setup_cmd_addr(0x11, 8, 0, 0);
    spim_set_datalen(1 * 8);
    spim_write_fifo(&data, 8);
    spim_start_transaction(SPIM_CMD_WR, SPIM_CSN0);
    SPIM_WAIT();
}

void uc_spim_init(void)
{
    *(volatile int *)(SPIM_REG_CLKDIV) = 7;
    spim_setup_master(0);
    spim_mem_mode_set();
    rt_kprintf("uc_spim_init suc\n");
}

void uc_spim_read(unsigned int remote_addr, unsigned char *buf, unsigned int buf_len)
{
    if (buf_len == 0)
    {
        return;
    }

    buf_len = (buf_len + 3) & (~3);

    spim_setup_cmd_addr(0x0b, 8, remote_addr, sizeof(unsigned int) * 8);
    spim_set_datalen(buf_len * 8);
    spim_setup_dummy(32, 0);
    spim_start_transaction(SPIM_CMD_RD, SPIM_CSN0);
    spim_read_fifo((int *)buf, buf_len * 8);
    SPIM_WAIT();
}

void uc_spim_write(unsigned int remote_addr, unsigned char *buf, unsigned int buf_len)
{
    if (buf_len == 0)
    {
        return;
    }

    buf_len = (buf_len + 3) & (~3);

    spim_setup_cmd_addr(0x02, 8, remote_addr, sizeof(unsigned int) * 8);
    spim_set_datalen(buf_len * 8);
    spim_setup_dummy(0, 0);
    spim_start_transaction(SPIM_CMD_WR, SPIM_CSN0);
    spim_write_fifo((int *)buf, buf_len * 8);
    SPIM_WAIT();
}

void spim_wr_sample(void)
{
    unsigned int test_mem_addr = 0x00336800;
    unsigned int write_value = 1000;
    unsigned int read_value = 0;

    uc_spim_init();

    uc_spim_write(test_mem_addr, (unsigned char *)&write_value, sizeof(unsigned int));
    uc_spim_read(test_mem_addr, (unsigned char *)&read_value, sizeof(unsigned int));
    if (write_value == read_value)
    {
        rt_kprintf("spim wr test ok\n");
    }
    else
    {
        rt_kprintf("spim wr test fail\n");
    }
}
#endif