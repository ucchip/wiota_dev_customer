#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

// #define WIOTA_SAVE_DATA_DEMO
#ifdef WIOTA_SAVE_DATA_DEMO
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"

#define WIOTA_SCAN_TIMEOUT      (40000)
#define WIOTA_TEST_SEND_TIMEOUT (60000)
#define WIOTA_TEST_WRITE_ADDR   (0x7D000)

static int wiota_get_static_freq(char *list)
{
    int num;
    // read static data 
    uc_wiota_get_freq_list((unsigned char *)list);

    for (num = 0; num < 16; num++)
    {
        rt_kprintf("static freq *(list+%d) %d\n", num, *(list+num));
        if (*(list + num) == 0xFF)
            break;
    }
    return num;
}

static int wiota_scan_freq(unsigned char *sync_freq)
{
    uc_recv_back_t result;
    u8_t freq_list[16] = {0xFF};
    char freq_list_len = 0;
    int sync_freq_num = 0;

    // init wiota stack.
    uc_wiota_init();
    
    // execute wiota protocol stack.
    uc_wiota_run();

    // read the freq index list from flash.
    freq_list_len = wiota_get_static_freq((char *)freq_list);

    // wiota sweep freq according to the default configation.
    uc_wiota_scan_freq(freq_list, freq_list_len, 0, WIOTA_SCAN_TIMEOUT, RT_NULL, &result);

    // frequency sweep successfull.
    if (UC_OP_SUCC == result.result)
    {
        // a list of freq point in result.
        uc_freq_scan_result_p freq_list = (uc_freq_scan_result_p)result.data;
        
        // the number of freq points as a result.
        int freq_num = result.data_len / sizeof(uc_freq_scan_result_t);

        for (int i = 0; i < freq_num; i++)
        {
            // the freq point at which the syncromization is success.
            if (freq_list->is_synced )
            {
                // record frequency point results.
                // result of rssi from high to low.
                sync_freq[sync_freq_num ++] = freq_list->freq_idx;
            }
            freq_list++;
        }
        // resources must be free.
        rt_free(result.data);
    }
    
    // release all resource of the wiota protocol
    uc_wiota_exit();

    return sync_freq_num;
}

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
    if (((addr + data_size) > 0x7E000) || (RT_NULL == data_p) || (data_p <= 0))
    {
        return;
    }
    uc_wiota_suspend_connect();
    uc_wiota_flash_erase_4K(addr);
    uc_wiota_flash_write(data_p, addr, data_size);
    uc_wiota_recover_connect();
}

static void wiota_read_data_from_custom_addr(int addr, unsigned char *data_p, int data_size)
{
    uc_wiota_suspend_connect();
    uc_wiota_flash_read(data_p, addr, data_size);
    uc_wiota_recover_connect();
}

static void wiota_test_data(void)
{
    uc_stats_info_t stats_info;
    UC_OP_RESULT send_result;
    char sendbuffer[] = {"123456789"};

    // clean up all wiota record status.
    uc_wiota_reset_stats(UC_STATS_TYPE_ALL);
    // get user info handle
    uint8_t *user_info_p = uc_wiota_get_user_info();
    
    while (1)
    {
        // get wiota connect state
        UC_WIOTA_STATUS connect_state = uc_wiota_get_state();

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
        // send data fail.
         if (UC_OP_SUCC != send_result)
         {
            rt_kprintf("uc_wiota_send_data send fail %d\n", send_result);
         }
         
        rt_thread_mdelay(5000);
    }
}

void wiota_save_data_demo(void)
{   
    // init wiota stack.
    uc_wiota_init();

    // set trial freq index
    uc_wiota_set_freq_info(93);
    
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

#endif
