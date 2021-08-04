
/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-26     RT-Thread    first version
 */

#include "trace_interface.h"
#include "uc_wiota_api.h"
#include "adp_sys.h"
#include "test_wiota_api.h"
#include "uc_string_lib.h"

extern void cce_check_fetch();

void test_send_callback(UcSendBack_P sendResult)
{
    TRACE_PRINTF("send result %d\n",sendResult->result);
}

int test_recv_callback(uc_receive_back_p recvData)
{
    TRACE_PRINTF("recv result %d type %d len %d addr 0x%x\n",recvData->result,recvData->type, recvData->data_len,recvData->data);
    
    uc_free(recvData->data);

    return 0;
}

void* testTaskHandle = NULL;

void app_test_main_task(void* pPara) 
{
    u8_t testData[20] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
    u8_t user_id[8] = {TEST_USER_ID};
    u8_t user_id_len = 0;
    t_sub_system_config wita_config =     
   {
        .sub_sys_u = 1,
        .id_len = 4,
        .pn_num = 1,
        .symbol_length = 1,
        .dlul_ratio = 0,
        .btvalue = 1,
        .group_number = 0,
        .systemid = 0x11223344,
        .subsystemid = 0x21456981,
        .na = {0},
    };

    uc_wiota_init();
    
    uc_wiota_set_freq_info(100);
    
    TRACE_PRINTF("test get freq point %d\n", uc_wiota_get_freq_info());
    
    TRACE_PRINTF("set user id %s\n", user_id);
    uc_wiota_set_userid(user_id, 8);

    memset(user_id , 0, 8);
    uc_wiota_get_userid(user_id, &user_id_len);
    TRACE_PRINTF("get user id %s, len = %d\n", user_id, user_id_len);

    uc_wiota_set_system_config(&wita_config);

    uc_wiota_run();
#ifdef _FPGA_    
    cce_check_fetch();
    uc_thread_delay(100);
    TRACE_PRINTF("user_test_main_task start loop\n");
#endif

    uc_wiota_register_recv_data(test_recv_callback);

    while (1)
    {
        TRACE_PRINTF("%s line %d\n", __FUNCTION__, __LINE__);
        uc_wiota_send_data(testData, 20, 20000, test_send_callback);
        uc_thread_delay(20000);
    }
    return;
}

void app_task_init(void) 
{
    uc_thread_create(&testTaskHandle,"test1", app_test_main_task, NULL, 256, 3, 3);

    uc_thread_start(testTaskHandle);
}


