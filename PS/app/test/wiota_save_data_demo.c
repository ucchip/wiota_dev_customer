#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#ifdef WIOTA_SAVE_DATA_DEMO
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"

#define WIOTA_SCAN_TIMEOUT      (40000)
#define WIOTA_TEST_SEND_TIMEOUT (60000)

/******************************************************************************
*  8288 static data occupies 8k, with a starting address of 504k and an ending address of 512k. 
*  To prevent errors from occurring, the testing address range for this demo is: 
*  the first 4k space of the starting address where static data is stored is used for testing, 
*  which is the starting address of 500k (0x7D000) and the ending address of 504k
******************************************************************************/
#define WIOTA_TEST_WRITE_ADDR   (0x7D000) // 500 * 1024

void wiota_recv_cb(uc_recv_back_p data)
{
    // data successfully received
    if (0 == data->result)
    {
        rt_kprintf("wiota_recv_callback result %d, data:%s\n", data->result, data->data);
        
        // must free data.
        rt_free(data->data);
    }
}

static void wiota_save_data_to_static(unsigned char *user_info_p,
                                      unsigned char *save_data_p, 
                                      int save_data_size)
{
    rt_memcpy(user_info_p, &save_data_p, sizeof(uc_stats_info_t));
    uc_wiota_save_static_info();
}

static void wiota_save_data_to_custom_addr(int addr, unsigned char *data_p, int data_size)
{
    /*
    The set addr=500k=0x7D000, with a maximum data size of 4k. 
    If it is greater than 4k, it will cause other errors. 
    Therefore, when (addr+data_size) is greater than 0x7E000 (504k), it will return.
    */
    if (((addr + data_size) > 0x7E000) || (RT_NULL == data_p) || (data_p <= 0))
    {
        rt_kprintf("data size out of range!!!\n");
        return;
    }

    /*
    When performing a write operation, 
    first suspend synchronization with the AP, 
    then erase the 4k flash and write in flash, 
    and finally recover synchronization with the AP.
    */
    uc_wiota_suspend_connect();
    uc_wiota_flash_erase_4K(addr);
    uc_wiota_flash_write(data_p, addr, data_size);
    uc_wiota_recover_connect();
}

static void wiota_read_data_from_custom_addr(int addr, unsigned char *data_p, int data_size)
{
    /*
    Like write operations, when performing a read operation, 
    first suspend synchronization with the AP, 
    then read the flash, 
    and finally recover synchronization with the AP.
    */
    uc_wiota_suspend_connect();
    uc_wiota_flash_read(data_p, addr, data_size);
    uc_wiota_recover_connect();
}

static void wiota_test_data(void)
{
    uc_stats_info_t stats_info;
    uc_op_result_e send_result;
    char sendbuffer[] = {"123456789"};

    // clean up all wiota record status.
    uc_wiota_reset_stats(UC_STATS_TYPE_ALL);
    // get user info handle
    uint8_t *user_info_p = uc_wiota_get_user_info();
    
    while (1)
    {
        // get wiota connect state
        uc_wiota_status_e connect_state = uc_wiota_get_state();

        // get wiota receive send data record.
        uc_wiota_get_all_stats(&stats_info);
        // save stats to static data area
        wiota_save_data_to_static(user_info_p, &stats_info, sizeof(uc_stats_info_t));

        // save stats to custom address
        wiota_save_data_to_custom_addr(WIOTA_TEST_WRITE_ADDR, &stats_info, sizeof(uc_stats_info_t));
        // read data from given address
        wiota_read_data_from_custom_addr(WIOTA_TEST_WRITE_ADDR, &stats_info, sizeof(uc_stats_info_t));
        
        // connect error or send success less than 20%.
        if ((stats_info.ul_sm_succ * 5 < stats_info.ul_sm_total && stats_info.ul_sm_total > 20) ||
            UC_STATUS_SYNC_LOST == connect_state || UC_STATUS_ERROR == connect_state
        )
        {
            rt_kprintf("%s line %d sm_succ %d sm_total %d connect state %d\n",
                       __FUNCTION__, __LINE__, stats_info.ul_sm_succ, stats_info.ul_sm_total, connect_state);
            
            return;
        }

        // clean up all wiota record status.
        uc_wiota_reset_stats(UC_STATS_TYPE_ALL);

        // send test data.
         send_result = uc_wiota_send_data(sendbuffer, rt_strlen(sendbuffer),  WIOTA_TEST_SEND_TIMEOUT, RT_NULL);
        
         if (UC_OP_SUCC != send_result)
         {
            rt_kprintf("uc_wiota_send_data send fail %d\n", send_result);
         }else
         {
            rt_kprintf("uc_wiota_send_data send success %d\n", send_result);
         }
         
        rt_thread_mdelay(5000);
    }
}

void wiota_save_data_demo(void)
{   
    // init wiota stack.
    uc_wiota_init();

    // set trial freq index
    uc_wiota_set_freq_info(25);
    
    // execute wiota protocol stack.
    uc_wiota_run();

    // register the receive data callback function.
    uc_wiota_register_recv_data_callback(wiota_recv_cb, UC_CALLBACK_NORAMAL_MSG);
    uc_wiota_register_recv_data_callback(wiota_recv_cb, UC_CALLBACK_STATE_INFO);

    // wiota connect ap
    uc_wiota_connect();

    // wait for the connect ap. timeout 4s
    if(uc_wiota_wait_sync(4000))
    {
        // release all resource of the wiota protocol
        uc_wiota_exit();
        return;
    }

    // send data test
    wiota_test_data();

    // release all resource of the wiota protocol
    uc_wiota_exit();
    
}
#endif // WIOTA_SAVE_DATA_DEMO