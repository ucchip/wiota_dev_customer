
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
#include "uc_wiota_api.h"
#include "test_wiota_api.h"

extern void cce_check_fetch();

void test_send_callback(uc_send_back_p sendResult)
{
    rt_kprintf("send result %d\n",sendResult->result);
}

void test_recv_callback(uc_recv_back_p recvData)
{
    rt_kprintf("recv result %d type %d len %d addr 0x%x\n",
            recvData->result,recvData->type,recvData->data_len,recvData->data);
    if (UC_OP_SUCC == recvData->result) { rt_free(recvData->data); }
    
}


void test_recv_req_callback(uc_recv_back_p recvData)
{
    rt_kprintf("recv req result %d type %d len %d addr 0x%x\n",
            recvData->result,recvData->type,recvData->data_len,recvData->data);
    
    if (UC_OP_SUCC == recvData->result) { rt_free(recvData->data); }
}


void* testTaskHandle = NULL;

void app_test_main_task(void* pPara) 
{
    unsigned int total;
    unsigned int used;
    unsigned int max_used;
    unsigned char testData[20] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
    unsigned int user_id[2] = {0xCDA05471,0x0};
    //unsigned char user_id_len = 0;
    //uc_recv_back_t recv_result_t;
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
    
    // first of all, init wiota
    uc_wiota_init();

    // test! whole config 
    uc_wiota_get_system_config(&wiota_config);
//    rt_kprintf("config show %d %d %d %d %d %d 0x%x 0x%x\n",
//                wiota_config.id_len,wiota_config.pn_num,wiota_config.symbol_length,wiota_config.dlul_ratio,
//                wiota_config.btvalue,wiota_config.group_number,wiota_config.systemid,wiota_config.subsystemid); 
//    wiota_config.pn_num = 2;  // change config then set
//    wiota_config.spectrumIdx = 7;
    uc_wiota_set_system_config(&wiota_config);
    
    // test! set dcxo
//    uc_wiota_set_dcxo(0x1d000);
    uc_wiota_set_dcxo(0x14000);

    // test! freq test
//    uc_wiota_set_freq_info(0);    // 470+0.2*100=490
    uc_wiota_set_freq_info(100);    // 470+0.2*100=490
//    uc_wiota_set_freq_info(150);    // 470+0.2*150=500
//    rt_kprintf("test get freq point %d\n", uc_wiota_get_freq_info());
    
    // test! user id 
//    TRACE_PRINTF("set user id %s\n", user_id);
    uc_wiota_set_userid(user_id,4);
//    memset(user_id,0,4);
//    uc_wiota_get_userid(user_id, &user_id_len);
//    TRACE_PRINTF("get user id %s\n", user_id);
//    TRACE_PRINTF("len = %d\n",user_id_len);

    uc_wiota_set_activetime(3);

    // after config set, run wiota !
    uc_wiota_run();

#ifdef _FPGA_    
    cce_check_fetch();
    rt_thread_mdelay(100);
    rt_kprintf("app test main start loop\n");
#endif

    // sync connect
//    rt_kprintf("wiota state %d\n",uc_wiota_get_state());
    uc_wiota_connect();
    rt_thread_mdelay(1000);
//    rt_kprintf("wiota state %d\n",uc_wiota_get_state());    

    // test! register recv data callback, afer upload, will continue recv ap's data or broadcast
    uc_wiota_register_recv_data(test_recv_callback);

//    unsigned char data[10] = {100,100,100,100,100,100,100,100,100,100};
    //unsigned char data[12] = {90,92,94,96,98,100,102,104,106,108,110,112};
    //unsigned short len = 12;

//    uc_wiota_scan_freq(data, len, 0, test_recv_callback, NULL);

    while (1)
    {
//        rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);

        rt_memory_info(&total,&used,&max_used);
        rt_kprintf("total %d used %d maxused %d \n",total,used,max_used);
        
//        uc_wiota_register_recv_data(test_recv_callback);
        // test! send data periodic
        uc_wiota_send_data(testData, 20, 2000, test_send_callback);
        
        // test! recv data without callback
//        uc_wiota_recv_data(&recv_result_t, 10000, NULL);
//        rt_kprintf("data recv result %d type %d len %d data 0x%x\n",recv_result_t.result,recv_result_t.type,
//                                                                    recv_result_t.data_len,recv_result_t.data);
//        uc_free(recv_result_t.data);
        // test! recv data with callback
//        uc_wiota_recv_data(NULL, 9000, test_recv_req_callback);
        
        
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
      
        rt_thread_mdelay(4000);
    }
    return;
}


int uc_thread_create_test(void ** thread, \
            char *name, void (*entry)(void *parameter), \
            void *parameter, unsigned int  stack_size, \
            unsigned char   priority, \
            unsigned int  tick)
{
    * thread = rt_malloc(sizeof(struct rt_thread));
    void *start_stack = rt_malloc(stack_size * 4);

    if (RT_NULL == start_stack) { return 1; }
    if (RT_EOK != rt_thread_init(* thread, name, entry, parameter, start_stack, stack_size * 4, priority, tick))
    {
        return 2;
    }

    return 0;
}
 
int uc_thread_start_test(void * thread)
{
    return rt_thread_startup((rt_thread_t)thread);
}

void app_task_init(void) 
{
    uc_thread_create_test(&testTaskHandle,"test1", app_test_main_task, NULL, 256, 3, 3);

    uc_thread_start_test(testTaskHandle);
}


