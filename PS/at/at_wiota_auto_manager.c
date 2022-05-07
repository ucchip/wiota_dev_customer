#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "uc_wiota_api.h"
#include "at.h"
#include "ati_prs.h"
#include "uc_string_lib.h"
#include "uc_adda.h"
#include "uc_boot_download.h"
//#include "adp_mem.h"
#include "at_wiota.h"

enum at_wiota_manager_process
{
    AT_WIOTA_MANAGER_PROCESS_DEFAULT = 0,
    AT_WIOTA_MANAGER_PROCESS_SCAN,
    AT_WIOTA_MANAGER_PROCESS_INIT,
    AT_WIOTA_MANAGER_PROCESS_RUN,
    AT_WIOTA_MANAGER_PROCESS_STRATEGY,
    AT_WIOTA_MANAGER_PROCESS_EXIT,
    AT_WIOTA_MANAGER_PROCESS_END,
};

enum at_wiota_manager_report_state
{
    AT_WIOTA_MANAGER_REPORT_FREQ_SUC = 0,
        AT_WIOTA_MANAGER_REPORT_FREQ_FAIL,
    AT_WIOTA_MANAGER_USER_FREQ,
    AT_WIOTA_MANAGER_CONNECT_SUC,
    AT_WIOTA_MANAGER_CONNECT_FAIL,
    AT_WIOTA_MANAGER_EXIT,
};

#define AT_WIOTA_SCAN_TIMEOUT 40000
#define AT_WIOTA_FULL_SCAN_TIMEOUT 500000
#define AT_WIOTA_CONTINE_SCAN_FAIL_MAX 3
typedef struct freq_list_manager_node
{
    unsigned char freq;
    signed char     snr;
    signed char     rssi;
    unsigned char   is_synced;
    char send_cucess_rate;
}t_freq_node_manager;


typedef struct freq_list_manager
{
    t_freq_node_manager node;
    struct freq_list_manager *next;
    struct freq_list_manager *pre;
}t_freq_list_manager;


typedef struct at_wiota_manager_parament
{
    rt_thread_t task_handle;
    t_freq_list_manager freq_list;
    t_freq_list_manager *current_freq_node;
    int  continue_scan_fail;
    char     manager_state;
    char     wiota_state;
}t_at_wiota_manager;

#define SET_WIOTA_STATE(state) g_wiota_manager.wiota_state = state
#define GET_WIOTA_STATE g_wiota_manager.wiota_state

#define SET_MANAGER_PROCESS(state) g_wiota_manager.manager_state = state
#define GET_MANAGER_PROCESS  g_wiota_manager.manager_state 

extern void wiota_recv_callback(uc_recv_back_p data);

t_at_wiota_manager g_wiota_manager = {0};

static int at_wiota_get_static_freq(char *list)
{
    int num;
    uc_wiota_get_freq_list((unsigned char *)list);

    for (num = 0; num < 16; num++)
    {
        //rt_kprintf("static freq *(list+%d) %d\n", num, *(list+num));
        if (*(list+num) == 0xFF)
            break;
    }
    return num;
}

static void at_wiota_init_freq_list(t_freq_list_manager *freq_list)
{
    freq_list->next = freq_list;
    freq_list->pre = freq_list;
}
static void at_wiota_print_freq_list(t_freq_list_manager *list)
{
    t_freq_list_manager *tmp = list->next;
    
    while(tmp != list)
    {
        rt_kprintf("address 0x%x freq %d snr %d rssi %d is_synced %d\n", tmp, tmp->node.freq, tmp->node.snr, tmp->node.rssi, tmp->node.is_synced);
        tmp = tmp->next;
    }
}
static void at_wiota_add_freq_list(t_freq_list_manager *freq_list, t_freq_list_manager *node)
{    
    //t_freq_list_manager *tmp = freq_list->next;
    
    rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);

    if (freq_list->next == freq_list)
    {
        freq_list->next = node;
        freq_list->pre = node;
        node->next = freq_list;
        node->pre = freq_list;
        //rt_kprintf("%s line %d node 0x%x, freq_list->next 0x%x\n", __FUNCTION__, __LINE__, node, freq_list->next);
    }
    else
    {
        node->next = freq_list->next;
        node->pre = freq_list;
        freq_list->next->pre = node;
        freq_list->next = node;
        //rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
    }
}

static void at_wiota_clean_freq_list(t_freq_list_manager *freq_list)
{
    t_freq_list_manager *op_node = freq_list->next;
    while(op_node != freq_list)
    {
        t_freq_list_manager *tmp = op_node;
        op_node = op_node->next;
        rt_free(tmp);
        tmp = RT_NULL;
    }
    freq_list->next = freq_list;
    freq_list->pre = freq_list;
}

static int at_wiota_sort_freq( t_freq_list_manager *list)
{
    t_freq_list_manager *head = list;
    t_freq_list_manager *compare = head->next;

    if (compare == head)
    {
        rt_kprintf("%s line %d list is null\n", __FUNCTION__, __LINE__);
        return 1;
    }

    while(compare != head)
    {
        t_freq_list_manager *tmp = compare->next;
        t_freq_list_manager *get_max = compare;
        
        //rt_kprintf("%s line %d head 0x%x compare 0x%x tmp 0x%x\n", __FUNCTION__, __LINE__, head, compare, tmp);
        while(tmp != head)
        {
            if (get_max->node.snr < tmp->node.snr)
                get_max = tmp;
            tmp = tmp->next;
        }
        // rt_kprintf("%s line %d get_max 0x%x\n", __FUNCTION__, __LINE__, get_max);
        
         //rt_kprintf("%s line %d compare 0x%x\n", __FUNCTION__, __LINE__, compare);
         
        if (compare != get_max && compare->next != get_max)
        {
            t_freq_list_manager *get_max_pre_tmp = get_max->pre ;
            t_freq_list_manager *get_max_next_tmp = get_max->next ;
            
            //rt_kprintf("%s line %d back 0x%x next 0x%x\n", __FUNCTION__, __LINE__, get_max , get_max->next);
            compare->pre->next = get_max;
            compare->next->pre = get_max;
            get_max->next->pre = compare;
            get_max->pre->next = compare;

            get_max->pre = compare->pre;
            get_max->next = compare->next;
            compare->pre = get_max_pre_tmp;
            compare->next =get_max_next_tmp;

            //rt_kprintf("%s line %d get_max 0x%x next 0x%x pre 0x%x\n", __FUNCTION__, __LINE__, get_max , get_max->next, get_max->pre);
            //rt_kprintf("%s line %d compare 0x%x next 0x%x pre 0x%x\n", __FUNCTION__, __LINE__, compare , compare->next, compare->pre);
            compare = get_max->next; 
        }
        else if (compare != get_max )
        {

            compare->pre->next = get_max;
            get_max->next->pre = compare;

            compare->next = get_max->next;
            get_max->pre = compare->pre;
            compare->pre = get_max;
            get_max->next = compare;

            //rt_kprintf("%s line %d get_max 0x%x next 0x%x pre 0x%x\n", __FUNCTION__, __LINE__, get_max , get_max->next, get_max->pre);
            //rt_kprintf("%s line %d compare 0x%x next 0x%x pre 0x%x\n", __FUNCTION__, __LINE__, compare , compare->next, compare->pre);
        }
        else
            compare = compare->next; 
    }
    
    return 0;
}

static int at_wiota_choose_freq( t_at_wiota_manager *manager)
{
    t_freq_list_manager *head = &(manager->freq_list);
    t_freq_list_manager *tmp = head->next;
    
    rt_kprintf("%s line %d tmp 0x%x\n", __FUNCTION__, __LINE__, tmp);

    if (tmp == head)
    {
        rt_kprintf("%s line %d list is null\n", __FUNCTION__, __LINE__);
        return 1;
    }
    if (manager->current_freq_node == RT_NULL)
    {
        manager->current_freq_node = tmp;
    }
    else
    {
        manager->current_freq_node = manager->current_freq_node->next;
        if (manager->current_freq_node == head)
        {
            manager->current_freq_node = RT_NULL;
            return 2;
        }
    }
 
    return 0;
}
#if 0
 void at_wiota_test_list( int count)
{
    t_freq_list_manager freq_list;
    int num = 0;
    
    at_wiota_init_freq_list(&freq_list);
    for(num = 0; num < count; num ++)
    {
        t_freq_list_manager *node = rt_malloc(sizeof(t_freq_list_manager));
        if (node == RT_NULL)
        {
            rt_kprintf("%s line %d malloc is error\n", __FUNCTION__, __LINE__);
            return ;
        }
        rt_kprintf("%s line %d malloc node 0x%x\n", __FUNCTION__, __LINE__, node);
        node->node.snr = 10 - num;
        at_wiota_add_freq_list(&freq_list, node);
    }
    rt_kprintf("%s line %d \n", __FUNCTION__, __LINE__);
    at_wiota_print_freq_list(&freq_list);
    rt_kprintf("%s line %d \n", __FUNCTION__, __LINE__);
    at_wiota_sort_freq(&freq_list);
     at_wiota_print_freq_list(&freq_list);
    rt_kprintf("%s line %d \n", __FUNCTION__, __LINE__);
    at_wiota_clean_freq_list(&freq_list);
    rt_kprintf("%s line %d \n", __FUNCTION__, __LINE__);
    at_wiota_print_freq_list(&freq_list);    
    rt_kprintf("%s line %d \n", __FUNCTION__, __LINE__);
}
#endif

static int at_wiota_only_freq(char freq, t_at_wiota_manager *manager)
{
    t_freq_list_manager *data = rt_malloc(sizeof(t_freq_list_manager));
    if (data == RT_NULL)
    {
        rt_kprintf("%s line %d malloc error\n", __FUNCTION__, __LINE__);
        return 1;
    }
    memset(data, 0, sizeof(t_freq_list_manager));
    data->node.freq = freq;
    at_wiota_add_freq_list(&(manager->freq_list), data);
    return 0;
}

static int at_wiota_freq_manager(uc_recv_back_t result, t_at_wiota_manager *manager, u8_t flag)
{
    int re = 1;
    
    if (UC_OP_SUCC == result.result || flag)
    {
        uc_freq_scan_result_p freqlist = (uc_freq_scan_result_p)result.data;
        int freq_num = result.data_len / sizeof(uc_freq_scan_result_t);
        int i = 0;
        
        for ( i = 0; i < freq_num; i++)
        {
            rt_kprintf("%s line %d freq_num %d i %d index %d is_synced %d snr %d\n", __FUNCTION__, __LINE__, freq_num, i, freqlist->freq_idx,  freqlist->is_synced, freqlist->snr);
            if (freqlist->is_synced || flag)
            {
                t_freq_list_manager *data = rt_malloc(sizeof(t_freq_list_manager));
                if (data == RT_NULL)
                {
                    rt_kprintf("%s line %d malloc error\n", __FUNCTION__, __LINE__);
                    return 1;
                }
                data->node.freq = freqlist->freq_idx;
                data->node.is_synced = freqlist->is_synced;
                data->node.rssi = freqlist->rssi;
                
                if (flag)
                    data->node.snr =  freqlist->rssi;
                else
                    data->node.snr = freqlist->snr;
                
                data->node.send_cucess_rate = 0;
                at_wiota_add_freq_list(&(manager->freq_list), data);
                re = 0;
            }
            freqlist++;
        }
        
        if (!re)
        {
            at_wiota_sort_freq(&(manager->freq_list));
            at_wiota_print_freq_list(&(manager->freq_list));
        }
        
        rt_free(result.data);
    }



    return re;
}


static int at_wiota_manager_scant(t_at_wiota_manager *manager)
{
    uc_recv_back_t result;
    u8_t list[16] = {0};
    int list_len = 0;
    int res;
     
    uc_wiota_init();
    at_wiota_set_state(AT_WIOTA_INIT);
    //uc_wiota_set_dcxo(0x22000);
    uc_wiota_run();    
    uc_wiota_register_recv_data_callback(wiota_recv_callback,UC_CALLBACK_NORAMAL_MSG);
    uc_wiota_register_recv_data_callback(wiota_recv_callback,UC_CALLBACK_STATE_INFO);
    at_wiota_set_state(AT_WIOTA_RUN);

    rt_kprintf("manager->continue_scan_fail  %d\n", manager->continue_scan_fail );

    list_len = at_wiota_get_static_freq((char *)list);
    rt_kprintf("%s line %d list_len %d\n", __FUNCTION__, __LINE__, list_len);

    switch(list_len)
    {
        case 0:
        {
            uc_wiota_scan_freq(RT_NULL, 0, AT_WIOTA_FULL_SCAN_TIMEOUT, RT_NULL, &result);
            rt_kprintf("%s line %d uc_wiota_scan_freq result %d\n", __FUNCTION__, __LINE__, result.result);
            if (manager->continue_scan_fail < AT_WIOTA_CONTINE_SCAN_FAIL_MAX)
                res = at_wiota_freq_manager(result, manager, 0);
            else
                res = at_wiota_freq_manager(result, manager, 1);
            if (res)
                manager->continue_scan_fail++;
            break;
         }
        case 1:
        {
            res = at_wiota_only_freq(list[0], manager);
            if (manager->continue_scan_fail > 2 && \
                manager->continue_scan_fail %2 == 0)
                res = 1;// return fail. enter sleep.
            rt_kprintf("%s line %d res = %d fail counter %d\n", __FUNCTION__, __LINE__, res, manager->continue_scan_fail);
            manager->continue_scan_fail++;
            
            break;
         }
        default:
        {
            uc_wiota_scan_freq(list, list_len, AT_WIOTA_SCAN_TIMEOUT, RT_NULL , &result);
            rt_kprintf("%s line %d uc_wiota_scan_freq result %d\n", __FUNCTION__, __LINE__, result.result);
            if (manager->continue_scan_fail < AT_WIOTA_CONTINE_SCAN_FAIL_MAX)
                res = at_wiota_freq_manager(result, manager, 0);
            else
                res = at_wiota_freq_manager(result, manager, 1);
            if (res)
                manager->continue_scan_fail++;
            break;
         }
    }
    uc_wiota_exit();
    at_wiota_set_state(AT_WIOTA_EXIT);
    
    return res;
}

static int at_wiota_manager_run(void)
{
    signed char num = 20;
    unsigned char counter = 0;
    uc_wiota_run();
    at_wiota_set_state(AT_WIOTA_RUN);
    
    uc_wiota_connect();

    while(num -- )
    {
        rt_thread_mdelay(100);
        if(UC_STATUS_SYNC == uc_wiota_get_state())
         {
            if (counter > 4)
                break;
            counter++;
         }
        else
            counter = 0;
    }
    return (num > 0? 0: 1);
}

static void at_wiota_manager_startegy(void)
{
    uc_stats_info_t stats_info_ptr;    
    uc_wiota_reset_stats(UC_STATS_TYPE_ALL);
    while(1)
    {
        // get state
        UC_WIOTA_STATUS connect_state = uc_wiota_get_state();
        uc_wiota_get_all_stats(&stats_info_ptr);
        
        if ((stats_info_ptr.ul_sm_succ * 3 < stats_info_ptr.ul_sm_total && stats_info_ptr.ul_sm_total > 3) || \
            UC_STATUS_SYNC != connect_state)
        {
            rt_kprintf("%s line %d sm_succ %d sm_total %d connect state %d\n", \
                __FUNCTION__, __LINE__, stats_info_ptr.ul_sm_succ, stats_info_ptr.ul_sm_total, connect_state);
            return ;   
        }
        uc_wiota_reset_stats(UC_STATS_TYPE_ALL);
        rt_thread_mdelay(5000);
    }
    
}

static void at_wiota_auto_report_state(int type, t_at_wiota_manager *manager)
{
    rt_kprintf("at_wiota_auto_report_state type = %d\n", type);
    switch(type)
    {
        case AT_WIOTA_MANAGER_REPORT_FREQ_SUC:
        {
              t_freq_list_manager *head = &(manager->freq_list);
              t_freq_list_manager *tmp = head->next;
    
              while(tmp != head)
             {
                //rt_kprintf("%s line head 0x%x tmp 0x%x\n", __FUNCTION__, __LINE__, head, tmp);
                at_server_printfln("+SCANFFREQ:%d,%d,%d,%d", tmp->node.freq, tmp->node.snr, tmp->node.rssi, tmp->node.is_synced);
                tmp = tmp->next;
             }
             break;
        }
        case AT_WIOTA_MANAGER_REPORT_FREQ_FAIL:
        {
            at_server_printfln("+WIOTASCANF:FAIL");
            break;
        }
        case AT_WIOTA_MANAGER_USER_FREQ:
        {
            if (manager->current_freq_node != RT_NULL)
                at_server_printfln("+WIOTAFREQ:%d,%d", \
                manager->current_freq_node->node.freq, manager->current_freq_node->node.snr);
            break;
        }
        case AT_WIOTA_MANAGER_CONNECT_SUC:
        {
            at_server_printfln("+WIOTA:READY");
            break;
        }
        case AT_WIOTA_MANAGER_CONNECT_FAIL:
        {
            at_server_printfln("+WIOTA:CONNECT FAIL");
            break;
        }
        case AT_WIOTA_MANAGER_EXIT:
        {
            at_server_printfln("+WIOTA:EXIT");
            break;
        }
    }
}


static void at_wiota_manager_task(void* parameter)
{
    SET_MANAGER_PROCESS(AT_WIOTA_MANAGER_PROCESS_SCAN);

    while(AT_WIOTA_MANAGER_PROCESS_END != GET_MANAGER_PROCESS)
    {
        rt_kprintf("%s line %d state %d\n", __FUNCTION__, __LINE__, GET_MANAGER_PROCESS);
        switch(GET_MANAGER_PROCESS)
        {
            case AT_WIOTA_MANAGER_PROCESS_SCAN:
            {
                if (at_wiota_manager_scant(&g_wiota_manager))
                {
                    SET_MANAGER_PROCESS(AT_WIOTA_MANAGER_PROCESS_SCAN);
                    at_wiota_auto_report_state(AT_WIOTA_MANAGER_REPORT_FREQ_FAIL, &g_wiota_manager);
                    rt_thread_mdelay(1000 * g_wiota_manager.continue_scan_fail);
                }
                else
                {
                     SET_MANAGER_PROCESS(AT_WIOTA_MANAGER_PROCESS_INIT);
                      rt_kprintf("%s line head 0x%x tmp 0x%x next node 0x%x\n", __FUNCTION__, __LINE__, &g_wiota_manager.freq_list, g_wiota_manager.freq_list.next);
                     at_wiota_auto_report_state(AT_WIOTA_MANAGER_REPORT_FREQ_SUC, &g_wiota_manager);
                }
                
                break;
            }
            case AT_WIOTA_MANAGER_PROCESS_INIT:
            {
                if (at_wiota_choose_freq(&g_wiota_manager))
                {
                    SET_MANAGER_PROCESS(AT_WIOTA_MANAGER_PROCESS_SCAN);
                    at_wiota_clean_freq_list(&g_wiota_manager.freq_list);
                    break;
                }
                uc_wiota_init();
                at_wiota_auto_report_state(AT_WIOTA_MANAGER_USER_FREQ, &g_wiota_manager);
                rt_kprintf("%s line %d freq %d\n", __FUNCTION__, __LINE__, g_wiota_manager.current_freq_node->node.freq);
                uc_wiota_set_freq_info(g_wiota_manager.current_freq_node->node.freq);
                at_wiota_set_state(AT_WIOTA_INIT);
                SET_MANAGER_PROCESS(AT_WIOTA_MANAGER_PROCESS_RUN);
                break;
            }
            case AT_WIOTA_MANAGER_PROCESS_RUN:
            {
                if (at_wiota_manager_run())
                {
                     SET_MANAGER_PROCESS(AT_WIOTA_MANAGER_PROCESS_EXIT);
                     at_wiota_auto_report_state(AT_WIOTA_MANAGER_CONNECT_FAIL, &g_wiota_manager);
                }
                else
                {
                    SET_MANAGER_PROCESS(AT_WIOTA_MANAGER_PROCESS_STRATEGY);
                    g_wiota_manager.continue_scan_fail = 0;
                }
                break;
            }
            case AT_WIOTA_MANAGER_PROCESS_STRATEGY:
            {
                at_wiota_auto_report_state(AT_WIOTA_MANAGER_CONNECT_SUC, &g_wiota_manager);
                at_wiota_manager_startegy();           
                SET_MANAGER_PROCESS(AT_WIOTA_MANAGER_PROCESS_EXIT);
                break;
            }
            case AT_WIOTA_MANAGER_PROCESS_EXIT:
            {
                 uc_wiota_exit();
                 at_wiota_auto_report_state(AT_WIOTA_MANAGER_PROCESS_EXIT, &g_wiota_manager);
                at_wiota_set_state(AT_WIOTA_EXIT);
                SET_MANAGER_PROCESS(AT_WIOTA_MANAGER_PROCESS_INIT);
                break;
            }
        }
    }
}

void at_wiota_manager(void)
{
    if(uc_wiota_get_auto_connect_flag()/*&& GET_MANAGER_PROCESS == AT_WIOTA_MANAGER_PROCESS_DEFAULT*/)
    {
       at_wiota_init_freq_list(&g_wiota_manager.freq_list);
        rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
        g_wiota_manager.task_handle = rt_thread_create("auMana",\
                              at_wiota_manager_task,\
                              RT_NULL,\
                              1024,\
                              RT_THREAD_PRIORITY_MAX / 3 - 1,\
                              3);
        if (RT_NULL == g_wiota_manager.task_handle)
        {
            rt_kprintf("%s line %d rt_thread_create error\n", __FUNCTION__, __LINE__);
            return ;
        }
        rt_thread_startup(g_wiota_manager.task_handle);
    }
}

