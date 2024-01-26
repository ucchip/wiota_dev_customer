#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#ifdef WIOTA_FREQ_MANAGER_DEMO
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"

#define WIOTA_SCAN_TIMEOUT 40000
#define WIOTA_TEST_SEND_TIMEOUT 60000

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

static void wiota_test_data(void)
{
    uc_stats_info_t stats_info;
    uc_op_result_e send_result;
    char sendbuffer[] = {"123456789"};

    // clean up all wiota record status.
    uc_wiota_reset_stats(UC_STATS_TYPE_ALL);

    while (1)
    {
        // get wiota connect state
        uc_wiota_status_e connect_state = uc_wiota_get_state();

        // get wiota receive send data record.
        uc_wiota_get_all_stats(&stats_info);

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
#if 0
static int wiota_wait_connect(void)
{
    // wait 4s.
    signed char num = 40;

    // number of success are monitored continue.
    unsigned char counter = 0;

    while (num--)
    {
        rt_thread_mdelay(100);

        // sync success
        if (UC_STATUS_SYNC == uc_wiota_get_state())
        {
            // 4 consecutive success detect.
            if (counter > 4)
                break;

            counter++;
        }
        else // sync faild
            counter = 0;
    }

    // return 0 is successfully.
    return (num > 0 ? 0 : 1);
}
#endif

static int wiota_run_test(unsigned char freq)
{
    // init wiota stack.
    uc_wiota_init();

    // set trial freq index
    uc_wiota_set_freq_info(freq);

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

void wiota_scan_freq_demo(void)
{
    unsigned char sync_freq[16] = {0xFF};
    int sync_freq_num = 0;

    // wiota configure the frequency sweep according
    // to the frequency point list.
    sync_freq_num = wiota_scan_freq(sync_freq);
    if (!sync_freq_num)
    {
        rt_kprintf("wiota_scan_freq no freq\n");
        return ;
    }

    // start connect trial from point where the rssi is higest.
    // until all freq scanned have been used.
    for (int n = 0; n < sync_freq_num; n ++)
    {
        wiota_run_test(sync_freq[n]);
    }

    return ;
}

#endif
