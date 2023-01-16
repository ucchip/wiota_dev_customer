
/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-26     RT-Thread    first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "test_wiota_api.h"
// #include "at.h"
#ifdef _LINUX_
#include <stdlib.h>
#include <string.h>
#else
#include "uc_string_lib.h"
#include "uc_gpio.h"
#endif

extern void cce_check_fetch();

void *testTaskHandle = NULL;
void *test2TaskHandle = NULL;

const unsigned int USER_ID_LIST[7] = {0x4c00ccdb, 0xc11cc34c, 0x488c558a, 0xabe44fcb, 0x1c1138b8, 0xdba09b6f, 0x9768c6cc};
const unsigned int USER_DCXO_LIST[7] = {0x25000, 0x29000, 0x30000, 0x2d000, 0x14000, 0x30000, 0x2E000};
#define USER_IDX 3
#define TEST_DATA_MAX_SIZE 310

boolean is_scaning_freq = FALSE;
boolean is_need_reset = FALSE;
boolean is_need_scaning_freq = FALSE;
u8_t new_freq_idx = 0;
u32_t test_flash_addr = 0x3F000; //FLASH_OPEN_START_ADDRESS;
u8_t *test_flash_data = NULL;
u16_t test_flash_data_len = 0;

void test_send_callback(uc_send_back_p sendResult)
{
    rt_kprintf("app send result %d\n", sendResult->result);
}

void test_recv_callback(uc_recv_back_p recvData)
{
    u8_t freq_idx;
    u8_t freq_num;
    uc_freq_scan_result_t *freq_list;

    rt_kprintf("app recv result %d type %d len %d addr 0x%x\n",
               recvData->result, recvData->type, recvData->data_len, recvData->data);

    // at_server_printfln("app recv result %d type %d len %d addr 0x%x",
    // recvData->result,recvData->type,recvData->data_len,recvData->data);

    switch (recvData->type)
    {
    case UC_RECV_SCAN_FREQ:
        freq_idx = *(recvData->data);
        new_freq_idx = freq_idx;
        freq_num = recvData->data_len / sizeof(uc_freq_scan_result_t);
        freq_list = (uc_freq_scan_result_t *)(recvData->data);
        rt_kprintf("scan num %d\n", freq_num);
        for (u8_t i = 0; i < freq_num; i++)
        {
            rt_kprintf("freq %d is_sync %d rssi %d snr %d\n", freq_list[i].freq_idx, freq_list[i].is_synced,
                       freq_list[i].rssi, freq_list[i].snr);
        }
        is_scaning_freq = FALSE;
        is_need_reset = TRUE;
        break;

    case UC_RECV_SYNC_LOST:
        rt_kprintf("lost sync, re scan\n");
        is_need_scaning_freq = TRUE;
        break;

    case UC_RECV_MSG:
    case UC_RECV_BC:
    case UC_RECV_OTA:

        //            at_server_printfln("recv type %d result %d",recvData->type,recvData->result);

        break;

    default:
        rt_kprintf("Type ERROR!!!!!!!!!!!\n");
        break;
    }

    if (UC_OP_SUCC == recvData->result && recvData->data != RT_NULL)
    {
        rt_free(recvData->data);
    }
}

void test_recv_req_callback(uc_recv_back_p recvData)
{
    rt_kprintf("app recv req result %d type %d len %d addr 0x%x\n",
               recvData->result, recvData->type, recvData->data_len, recvData->data);

    if (UC_OP_SUCC == recvData->result)
    {
        rt_free(recvData->data);
    }
}

void test_send_data_callback(uc_send_back_p send_back)
{
    rt_kprintf("app send data result %d addr 0x%x\n", send_back->result, send_back->oriPtr);
}

#if 0
void app_test_main_task(void* pPara)
{
    unsigned int total;
    unsigned int used;
    unsigned int max_used;
//    unsigned char testData[20] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
    unsigned char* testData = rt_malloc(TEST_DATA_MAX_SIZE);
    unsigned int i;
    unsigned int user_id[2] = {USER_ID_LIST[USER_IDX],0x0};

//    unsigned char scan_data[10] = {100,100,100,100,100,100,100,100,100,100};
//    unsigned char scan_data[17] = {90,100,105,120,125,130,131,133,134,135,135,135,136,137,138,139,150};
//    unsigned char scan_data[20] = {0,50,70,115,129,132,133,111,136,138,140,105,142,106,147,150,152,107,135,104};
//    unsigned char scan_data[20] = {94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115};
    unsigned char scan_data[20] = {5,15,25,35,45,55,65,75,85,95,105,115,125,135,145,155,165,175,185,195};
////    unsigned char data[64] = {0};
    unsigned short scan_len = 20;
//    unsigned short dfe_test = 85;
//    unsigned short dfe_loop = 0;

    //unsigned char user_id_len = 0;
//    uc_recv_back_t recv_result_t;
    sub_system_config_t wiota_config = {0};

//    {
//        .id_len = 1,
//        .pn_num = 1,
//        .symbol_length = 1,
//        .dlul_ratio = 0,
//        .btvalue = 1,
//        .group_number = 0,
//        .systemid = 0x11223344,
//        .subsystemid = 0x21456981,
//        .na = {0},
//    };

//    l1_lpm_sleep_config_reset();

    for (i = 0; i < TEST_DATA_MAX_SIZE; i++) {
        testData[i] = (i+30) & 0xFF;
    }

    // first of all, init wiota
    uc_wiota_init();

    // test! whole config
    uc_wiota_get_system_config(&wiota_config);
    rt_kprintf("config show %d %d %d %d %d %d 0x%x 0x%x\n",
                wiota_config.id_len,wiota_config.pn_num,wiota_config.symbol_length,wiota_config.dlul_ratio,
                wiota_config.btvalue,wiota_config.group_number,wiota_config.systemid,wiota_config.subsystemid);
//    wiota_config.pn_num = 2;  // change config then set
//    wiota_config.spectrumIdx = 7;
//    uc_wiota_set_system_config(&wiota_config);

    // test! set dcxo
//    uc_wiota_set_dcxo(USER_DCXO_LIST[USER_IDX]);
    uc_wiota_set_dcxo(0x20000);

    // test! freq test
//    uc_wiota_set_freq_info(85);    // 470+0.2*100=490
    uc_wiota_set_freq_info(188);    // 470+0.2*100=490
//    uc_wiota_set_freq_info(150);    // 470+0.2*150=500
//    rt_kprintf("test get freq point %d\n", uc_wiota_get_freq_info());

    // test! user id
//    TRACE_PRINTF("set user id %s\n", user_id);
//    uc_wiota_set_userid(user_id,4);
//    memset(user_id,0,4);
//    uc_wiota_get_userid(user_id, &user_id_len);
//    TRACE_PRINTF("get user id %s\n", user_id);
//    TRACE_PRINTF("len = %d\n",user_id_len);

//    uc_wiota_set_active_time(3);

    // after config set, run wiota !
    uc_wiota_run();

#ifdef _FPGA_
    cce_check_fetch();
//    rt_thread_mdelay(100);
    rt_kprintf("app test main start loop\n");
#endif

    // sync connect
//    rt_kprintf("wiota state %d\n",uc_wiota_get_state());
//    uc_wiota_connect();
//    rt_thread_mdelay(10);
//    rt_kprintf("wiota state %d\n",uc_wiota_get_state());

    // test! register recv data callback, afer upload, will continue recv ap's data or broadcast
    uc_wiota_register_recv_data_callback(test_recv_callback,UC_CALLBACK_NORAMAL_MSG);
    uc_wiota_register_recv_data_callback(test_recv_callback,UC_CALLBACK_STATE_INFO);


//    for (int i = 0; i < 16; i++) {
//        scan_data[i*4] = 125;
//        scan_data[i*4+1] = 130;
//        scan_data[i*4+2] = 135;
//        scan_data[i*4+3] = 140;
//    }

    is_scaning_freq = TRUE;
    uc_wiota_scan_freq(scan_data, scan_len, 0, 0, test_recv_callback, NULL);
//    uc_wiota_scan_freq(scan_data, scan_len, 0, 0, NULL, &recv_result_t);
//    uc_wiota_scan_freq(NULL, 0, 0, 0, NULL, &recv_result_t);
//    if (UC_OP_SUCC == recv_result_t.result) { rt_free(recv_result_t.data); }
//    uc_wiota_scan_freq(NULL, 0, 0, 0, test_recv_callback, NULL);


//    l1_print_data_u32((u32_t*)(0x405000),256);

//    rt_kprintf("dfe test %d \n",l1_read_dfe_counter());

//    rt_thread_mdelay(150);
    rt_thread_mdelay(5000);


    while (0)
    {
        rt_thread_mdelay(3000);

        if (UC_STATUS_SYNC == uc_wiota_get_state()) {
            uc_wiota_send_data(testData, 20, 10000, NULL);
        } else {
            uc_wiota_exit();
            rt_thread_mdelay(3000);
            uc_wiota_init();
//            uc_wiota_set_dcxo(0x1E000);
            uc_wiota_set_freq_info(175);
            uc_wiota_set_userid(user_id,4);
            uc_wiota_run();
            cce_check_fetch();
            uc_wiota_connect();
            rt_thread_mdelay(3000);
        }
    }


    while (0)
    {
        uc_wiota_exit();
        rt_thread_mdelay(3000);
        rt_kprintf("reset test\n");
        rt_thread_mdelay(10);

//        dfe_loop++;
//        if (dfe_loop > 10) {
//            dfe_test += 1;
//            dfe_loop = 0;
//            if (dfe_test > 0xFF) { break; }
//        }
//
//        rt_kprintf("test begin loop %d dfe %d\n",dfe_loop,dfe_test);

        uc_wiota_init();
//        uc_wiota_set_dcxo(USER_DCXO_LIST[USER_IDX]);
        uc_wiota_set_dcxo(0);
        uc_wiota_set_freq_info(135);
        uc_wiota_set_userid(user_id,4);
//        state_csr11_info_init(dfe_test);
        uc_wiota_run();
        cce_check_fetch();
        uc_wiota_connect();
//        rt_thread_mdelay(150);
        rt_thread_mdelay(10000);
        rt_kprintf("test end\n");
    }

    while (0)
    {
        rt_kprintf("test end\n");
        rt_thread_mdelay(100000);
    }

    while (1)
    {
//        rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);

        rt_memory_info(&total,&used,&max_used);
        rt_kprintf("total %d used %d maxused %d \n",total,used,max_used);
//        rt_kprintf("dfe test %d \n",l1_read_dfe_counter());

        if (is_need_reset) {
            uc_wiota_exit();
            uc_wiota_init();
            uc_wiota_set_dcxo(0x20000);
            uc_wiota_set_freq_info(new_freq_idx);
            uc_wiota_run();
            uc_wiota_register_recv_data_callback(test_recv_callback,UC_CALLBACK_NORAMAL_MSG);
            uc_wiota_register_recv_data_callback(test_recv_callback,UC_CALLBACK_STATE_INFO);
            uc_wiota_connect();
            rt_thread_mdelay(1000);
            is_need_reset = FALSE;
        }

        if (is_need_scaning_freq) {
            is_need_scaning_freq = FALSE;
            is_scaning_freq = TRUE;
//            uc_wiota_scan_freq(NULL, 0, 0, 0, test_recv_callback, NULL);
            uc_wiota_scan_freq(scan_data, scan_len, 0, 0, test_recv_callback, NULL);
        }

        if (is_scaning_freq || UC_STATUS_SYNC != uc_wiota_get_state()) {
            rt_thread_mdelay(5000);
            continue;
        }

        // test! recv data without callback
//        uc_wiota_recv_data(&recv_result_t, 10000, NULL);
//        rt_kprintf("data recv result %d type %d len %d data 0x%x\n",recv_result_t.result,recv_result_t.type,
//                                                                    recv_result_t.data_len,recv_result_t.data);
//        uc_free(recv_result_t.data);
        // test! recv data with callback
//        uc_wiota_recv_data(NULL, 9000, test_recv_req_callback);

//        uc_wiota_exit();
//        l1_lpm_set_alarm_test(121);
//        l1_lpm_set_alarm_test(600);
//        rt_thread_mdelay(100);
//        l1_lpm_sleep_test();

        // test! connect and disconnect
//        rt_thread_mdelay(10000);
//        rt_kprintf("wiota state %d\n",uc_wiota_get_state());
//        uc_wiota_disconnect();
//        rt_thread_mdelay(1000);
//        rt_kprintf("wiota state %d\n",uc_wiota_get_state());
//        rt_thread_mdelay(1000);
//        rt_kprintf("wiota state %d\n",uc_wiota_get_state());
//        uc_wiota_connect();
//        rt_thread_mdelay(1000);
//        rt_kprintf("wiota state %d\n",uc_wiota_get_state());

        // test! wiota iote task exit
//        rt_thread_mdelay(3000);
//        uc_wiota_exit();
//        rt_thread_mdelay(3000);
//        uc_wiota_init();
//        // test! freq test
//        uc_wiota_set_dcxo(0xB000);
//        uc_wiota_set_freq_info(100);    // 470+0.2*100=490
//        rt_thread_mdelay(1000);
//        uc_wiota_run();
//        rt_thread_mdelay(1000);
//        uc_wiota_connect();
//        rt_thread_mdelay(1000);

        uc_wiota_send_data(testData, 20, 10000, NULL);

        rt_thread_mdelay(5000);
    }

    rt_free(testData);

    return;
}

#else
//void app_test_main_task(void* pPara)
//{
//    s8_t temp = 0;
//    u8_t subframe = 0;
//
//    uc_wiota_init();
//    uc_wiota_run();
//
//    while (1)
//    {
////        if (UC_STATUS_SYNC == uc_wiota_get_state()) {
//            temp = l1_adc_temperature_read();
//            subframe = l1_get_subframeCounter();
//            rt_kprintf("test temp read %d subf %d\n",temp,subframe);
////        }
//        rt_thread_mdelay(1);
//    }
//    return;
//}

#define TEST_FREQ_SINGLE 100
extern u32_t g_tracking_succ_test_cnt;

// small demo for all auto
void app_test_main_task(void *pPara)
{
    unsigned int total;
    unsigned int used;
    unsigned int max_used;
    unsigned char *testData = rt_malloc(TEST_DATA_MAX_SIZE);
    unsigned char *testData1 = rt_malloc(20);
    unsigned int i;
    unsigned int user_id[2] = {0x6d980e0a, 0x0};
    unsigned char user_id_len = 0;
    unsigned char scan_data[4] = {TEST_FREQ_SINGLE, TEST_FREQ_SINGLE, TEST_FREQ_SINGLE, TEST_FREQ_SINGLE}; //{172,125,160,134}; // {172,174,176,178};
    unsigned short scan_len = 4;
    u8_t app_count = 48;

    //    uc_recv_back_t recv_result_t;
    sub_system_config_t wiota_config = {0};

    for (i = 0; i < TEST_DATA_MAX_SIZE; i++)
    {
        testData[i] = (i + 30) & 0x7F;
    }

    memcpy(testData, " Hello, WIoTa.", 14);

    // first of all, init wiota
    uc_wiota_init();
    //    uc_wiota_set_active_time(5);

    // test! whole config
    uc_wiota_get_system_config(&wiota_config);
    rt_kprintf("config show %d %d %d %d %d %d 0x%x 0x%x\n",
               wiota_config.id_len, wiota_config.pp, wiota_config.symbol_length, wiota_config.dlul_ratio,
               wiota_config.btvalue, wiota_config.group_number, wiota_config.systemid, wiota_config.subsystemid);
    //    wiota_config.pn_num = 2;  // change config then set
    wiota_config.symbol_length = 1; // 256,1024
    wiota_config.ap_max_pow = 22;
    uc_wiota_set_system_config(&wiota_config);

    // test! set dcxo
    //    uc_wiota_set_dcxo(USER_DCXO_LIST[USER_IDX]);

    //gpio_set_pin_value(UC_GPIO,17,GPIO_VALUE_HIGH);

    uc_wiota_set_is_osc(1);

    if (!uc_wiota_get_is_osc())
    {
        uc_wiota_set_dcxo(0x36000);
    }

    // test! freq test
    //    uc_wiota_set_freq_info(85);    // 470+0.2*100=490
    uc_wiota_set_freq_info(TEST_FREQ_SINGLE); // 470+0.2*100=490
                                              //    uc_wiota_set_freq_info(150);    // 470+0.2*150=500
                                              //    rt_kprintf("test get freq point %d\n", uc_wiota_get_freq_info());

    //    uc_wiota_set_userid(user_id,4);

    //    uc_wiota_set_cur_power(21);
    //    uc_wiota_set_max_power(17);

    //    uc_wiota_set_is_gating(TRUE);

    // after config set, run wiota !
    uc_wiota_run();

#ifdef _FPGA_
    cce_check_fetch();
    //    rt_thread_mdelay(100);
    rt_kprintf("app test main start loop\n");
#endif

    //    is_scaning_freq = TRUE;
    //    uc_wiota_scan_freq(scan_data, scan_len, 0, 0, test_recv_callback, NULL);

    is_need_reset = TRUE;
    new_freq_idx = TEST_FREQ_SINGLE;

    //    uc_wiota_set_cur_power(19);
    //    uc_wiota_register_recv_data_callback(test_recv_callback,UC_CALLBACK_NORAMAL_MSG);
    //    uc_wiota_register_recv_data_callback(test_recv_callback,UC_CALLBACK_STATE_INFO);
    //    uc_wiota_connect();

    rt_thread_mdelay(2000);

    uc_wiota_get_userid(user_id, &user_id_len);

    rt_kprintf("app test data addr 0x%x 0x%x\n", testData, testData1);

    while (1)
    {
        rt_memory_info(&total, &used, &max_used);
        rt_kprintf("total %d used %d maxused %d \n", total, used, max_used);
        //        rt_kprintf("dfe test %d \n",l1_read_dfe_counter());

        if (is_need_reset)
        {
            uc_wiota_exit();
            rt_kprintf("total %d used %d maxused %d \n", total, used, max_used);
            rt_thread_mdelay(1000);
            uc_wiota_init();
            //            uc_wiota_set_active_time(5);
            if (!uc_wiota_get_is_osc())
            {
                uc_wiota_set_dcxo(0x36000);
            }
            uc_wiota_set_freq_info(new_freq_idx);
            //            uc_wiota_set_cur_power(21);
            //            uc_wiota_set_max_power(17);
            uc_wiota_run();
            uc_wiota_register_recv_data_callback(test_recv_callback, UC_CALLBACK_NORAMAL_MSG);
            uc_wiota_register_recv_data_callback(test_recv_callback, UC_CALLBACK_STATE_INFO);
            uc_wiota_connect();
            //            uc_wiota_set_data_rate(UC_RATE_NORMAL,UC_MCS_LEVEL_5);
            //            uc_wiota_set_cur_power(21);
            //            uc_wiota_set_max_power(17);
            rt_thread_mdelay(1000);
            is_need_reset = FALSE;
        }

        if (is_need_scaning_freq)
        {
            is_need_scaning_freq = FALSE;
            is_scaning_freq = TRUE;
            // uc_wiota_scan_freq(NULL, 0, 0, 0, test_recv_callback, NULL);
            uc_wiota_scan_freq(scan_data, scan_len, 0, 0, test_recv_callback, NULL);
        }

        if (is_scaning_freq || UC_STATUS_SYNC != uc_wiota_get_state())
        {
            rt_thread_mdelay(5000);
            continue;
        }

        memcpy(&(testData[0]), &app_count, 1);
        app_count++;
        if (app_count > 57)
        {
            app_count = 48;
        }

        //        uc_wiota_send_data(testData, 206, 10000, NULL);
        //        if (g_tracking_succ_test_cnt > 11) {
        //            uc_wiota_send_data(testData, 20, 10000, NULL);
        //        }

        uc_wiota_send_data(testData, 2, 10000, NULL);
        //        uc_wiota_send_data(testData, 2, 10000, test_send_data_callback);
        //        uc_wiota_send_data(testData1, 2, 10000, test_send_data_callback);
        rt_thread_mdelay(4000);
    }

    rt_free(testData);

    return;
}

#endif

void test1_recv_callback(uc_recv_back_p recvData)
{
    rt_kprintf("app recv result %d type %d len %d addr 0x%x\n",
               recvData->result, recvData->type, recvData->data_len, recvData->data);

    switch (recvData->type)
    {
    case UC_RECV_SCAN_FREQ:
        break;

    case UC_RECV_SYNC_LOST:
        rt_kprintf("lost sync, re scan\n");
        break;

    case UC_RECV_MSG:
    case UC_RECV_BC:
        break;

    case UC_RECV_OTA:
        if (UC_OP_SUCC == recvData->result && recvData->data != RT_NULL)
        {

            test_flash_data = recvData->data;
            test_flash_data_len = recvData->data_len;
        }
        break;

    default:
        rt_kprintf("Type ERROR!!!!!!!!!!!\n");
        break;
    }

    if (UC_RECV_OTA != recvData->type && UC_OP_SUCC == recvData->result && recvData->data != RT_NULL)
    {
        rt_free(recvData->data);
    }
}

void app_test1_main_task(void *pPara)
{
    unsigned int total;
    unsigned int used;
    unsigned int max_used;
    sub_system_config_t wiota_config = {0};
    unsigned int user_id[2] = {0x6d980e0a, 0x0};
    unsigned char user_id_len = 0;
    // u32_t* dataPtr = uc_malloc(4096);
    // u16_t i = 0;
    // u32_t dfe[3];

    // memset(dataPtr,0,4096);
    // for (i=0;i<1024;i++) {
    //     dataPtr[i] = i;
    // }

    // while(1)
    {
        rt_thread_delay(2000);

        rt_memory_info(&total, &used, &max_used);
        rt_kprintf("before total %d used %d maxused %d\n", total, used, max_used);

        uc_wiota_get_system_config(&wiota_config);
        rt_kprintf("config show %d %d %d %d %d %d %d 0x%x 0x%x\n",
                   wiota_config.ap_max_pow, wiota_config.id_len, wiota_config.pp, wiota_config.symbol_length, wiota_config.dlul_ratio,
                   wiota_config.btvalue, wiota_config.group_number, wiota_config.systemid, wiota_config.subsystemid);
        // wiota_config.pn_num = 2;  // change config then set
        // wiota_config.symbol_length = 3; // 256,1024
        // wiota_config.ap_max_pow = 14;
        // uc_wiota_set_system_config(&wiota_config);

        uc_wiota_set_userid(user_id, 4);
        uc_wiota_init();
        rt_thread_delay(1000);
        uc_wiota_set_freq_info(115); // 470+0.2*100=490
        uc_wiota_get_userid(user_id, &user_id_len);
        rt_kprintf("userid 0x%x len %d\n", user_id[0], user_id_len);

        // uc_wiota_get_system_config(&wiota_config);
        // rt_kprintf("config show %d %d %d %d %d %d %d 0x%x 0x%x\n",
        //         wiota_config.ap_max_pow,wiota_config.id_len,wiota_config.pn_num,wiota_config.symbol_length,wiota_config.dlul_ratio,
        //         wiota_config.btvalue,wiota_config.group_number,wiota_config.systemid,wiota_config.subsystemid);

        uc_wiota_run();
        rt_thread_delay(2000);
        uc_wiota_register_recv_data_callback(test1_recv_callback, UC_CALLBACK_NORAMAL_MSG);
        uc_wiota_register_recv_data_callback(test1_recv_callback, UC_CALLBACK_STATE_INFO);
        uc_wiota_connect();
        rt_thread_delay(2000);

        // for (u8_t i = 0; i<63; i++) {
        // }3dp

        // uc_wiota_suspend_connect();

        // dfe[0] = l1_read_dfe_counter();
        // uc_wiota_flash_erase_4K(FLASH_OPEN_START_ADDRESS);
        // dfe[1] = l1_read_dfe_counter();
        // uc_wiota_flash_write(dataPtr,FLASH_OPEN_START_ADDRESS,4096);
        // dfe[2] = l1_read_dfe_counter();
        // rt_kprintf("dfe time use erase %d write %d\n",dfe[1]-dfe[0],dfe[2]-dfe[1]);

        // uc_wiota_recover_connect();

        rt_thread_delay(2000);

        rt_memory_info(&total, &used, &max_used);
        rt_kprintf("after total %d used %d maxused %d\n", total, used, max_used);
    }

    while (1)
    {
        rt_thread_delay(2000);
        rt_memory_info(&total, &used, &max_used);
        rt_kprintf("after total %d used %d maxused %d\n", total, used, max_used);
    }

    return;
}

void app_test2_main_task(void *pPara)
{
    unsigned int total;
    unsigned int used;
    unsigned int max_used;

    while (1)
    {
        rt_thread_delay(10);

        if (NULL != test_flash_data)
        {

            if (1024 == test_flash_data_len)
            {

                rt_kprintf("ota write 0\n");

                uc_wiota_suspend_connect();
                rt_kprintf("ota write 1\n");

                if ((test_flash_addr % 0xFFF) == 0)
                {
                    uc_wiota_flash_erase_4K(test_flash_addr);
                }
                rt_kprintf("ota write 2\n");

                uc_wiota_flash_write(test_flash_data, test_flash_addr, 1024);
                rt_kprintf("ota write 3\n");

                rt_kprintf("ota write 4\n");
                uc_wiota_recover_connect();

                rt_kprintf("dfe addr 0x%x\n", test_flash_addr);

                test_flash_addr += 1024;
            }

            rt_free(test_flash_data);
            test_flash_data = NULL;

            rt_memory_info(&total, &used, &max_used);
            rt_kprintf("after total %d used %d maxused %d\n", total, used, max_used);
        }
    }

    return;
}

int uc_thread_create_test(void **thread,
                          char *name, void (*entry)(void *parameter),
                          void *parameter, unsigned int stack_size,
                          unsigned char priority,
                          unsigned int tick)
{
    *thread = rt_malloc(sizeof(struct rt_thread));
    void *start_stack = rt_malloc(stack_size * 4);

    if (RT_NULL == start_stack)
    {
        return 1;
    }
    if (RT_EOK != rt_thread_init(*thread, name, entry, parameter, start_stack, stack_size * 4, priority, tick))
    {
        return 2;
    }

    return 0;
}

#ifdef GPS_TEST_CASE
#define RV_BUF_SIZE 256
static rt_device_t serial;
static struct rt_semaphore rx_sem;
char rv_buf[5][RV_BUF_SIZE];
char rv_buf_in[RV_BUF_SIZE];
unsigned char w_idx = 0;
unsigned char r_idx = 0;

static rt_err_t uart_rx_ind(rt_device_t dev, rt_size_t size)
{
    if (size > 0)
    {
        rt_sem_release(&rx_sem);
    }
    return RT_EOK;
}

static rt_err_t uart_get_char(char *ch, rt_int32_t timeout)
{
    rt_err_t result = RT_EOK;

    while (rt_device_read(serial, 0, ch, 1) == 0)
    {
        result = rt_sem_take(&rx_sem, rt_tick_from_millisecond(timeout));
        if (result != RT_EOK)
        {
            return result;
        }
    }

    return result;
}

static int uc_wiota_read_gps_data_init(void)
{
    // find uart0
    serial = rt_device_find("uart0");
    if (serial == NULL)
    {
        rt_kprintf("find uart0 failed\n");
        return -1;
    }
    rt_kprintf("find uart0 suc\n");

    // init rx_sem
    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);

    // config banud rate -> 115200
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    config.baud_rate = BAUD_RATE_115200;
    rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config);

    // open uart0
    if (RT_EOK != rt_device_open(serial, RT_DEVICE_FLAG_INT_RX))
    {
        rt_kprintf("uart0 open fail\n");
        return -2;
    }
    rt_kprintf("open uart0 suc\n");

    // set uart0 recv callback
    rt_device_set_rx_indicate(serial, uart_rx_ind);
    rt_kprintf("set uart0 rx indicate suc\n");

    return 0;
}

static void uc_wiota_read_gps_data_deinit(void)
{
    // detach rx_sem
    rt_sem_detach(&rx_sem);

    // close uart0
    rt_device_close(serial);
}

static void uc_wiota_read_gps_data_by_serial(void)
{
    char ch;
    static int i = 0;
    unsigned char temp_w;

    while (1)
    {
        // wait recv uart0 data
        if (RT_EOK != uart_get_char(&ch, RT_WAITING_FOREVER))
            continue;

        i = (i >= RV_BUF_SIZE) ? RV_BUF_SIZE : i;

        rv_buf_in[i++] = ch;

        // recv end
        if (rt_strstr(rv_buf_in, "\r\n"))
        {
            rt_kprintf("%s\n", rv_buf_in);
            temp_w = w_idx;
            temp_w++;

            if (temp_w == 5)
            {
                temp_w = 0;
            }

            if (temp_w != r_idx)
            {
                rt_memcpy(&rv_buf[temp_w][0], rv_buf_in, rt_strlen(rv_buf_in));
                w_idx = temp_w;
            }
            rt_memset(rv_buf_in, 0, RV_BUF_SIZE);
            i = 0;
            break;
        }
    }
}

void gps_data_read_task(void *pPara)
{
    // init wiota ps
    uc_wiota_init();

    // set osc
    uc_wiota_set_is_osc(1);

    // set ferq
    uc_wiota_set_freq_info(145);

    // run wiota ps
    uc_wiota_run();

    // connect to ap
    uc_wiota_connect();

    // close wiota ps log
    uc_wiota_log_switch(UC_LOG_UART, 0);

    // init uart0
    uc_wiota_read_gps_data_init();

    while (1)
    {
        uc_wiota_read_gps_data_by_serial();
        rt_thread_delay(1000);
    }

    uc_wiota_read_gps_data_deinit();
}

void gps_data_send_task(void *pPara)
{
    unsigned char temp_idx = 0;

    while (1)
    {
        if (r_idx != w_idx)
        {
            temp_idx = r_idx + 1;
            if (temp_idx == 5)
            {
                temp_idx = 0;
            }

            if (rt_strstr(&rv_buf[temp_idx][0], "$GPGGA"))
            {
                if (0 == uc_wiota_send_data((unsigned char *)&rv_buf[temp_idx][0], rt_strlen(&rv_buf[temp_idx][0]), 1000, RT_NULL))
                {
                    rt_kprintf("send gps data to ap suc\n");
                }
            }
            rt_memset(&rv_buf[temp_idx][0], 0, RV_BUF_SIZE);
            r_idx = temp_idx;
        }
        rt_thread_delay(10);
    }
}
#endif

void app_task_init(void)
{
    uc_thread_create_test(&testTaskHandle, "test1", app_test_main_task, NULL, 256, 3, 3);

    // uc_thread_create_test(&testTaskHandle,"test1", app_test1_main_task, NULL, 256, 3, 3);
    // uc_thread_create_test(&test2TaskHandle,"test2", app_test2_main_task, NULL, 256, 3, 3);
#ifdef GPS_TEST_CASE
    void *gps_read_handle;
    void *gps_send_handle;
    uc_thread_create_test(&gps_read_handle, "read", gps_data_read_task, NULL, 256, 3, 3);
    uc_thread_create_test(&gps_send_handle, "send", gps_data_send_task, NULL, 256, 3, 3);
    rt_thread_startup((rt_thread_t)gps_read_handle);
    rt_thread_startup((rt_thread_t)gps_send_handle);
#endif
    rt_thread_startup((rt_thread_t)testTaskHandle);
    // rt_thread_startup((rt_thread_t)test2TaskHandle);
}
