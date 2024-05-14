#include <rtthread.h>
#ifdef RT_USING_AT
#include <rtdevice.h>
#include <board.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "at.h"
#include "ati_prs.h"
#include "uc_string_lib.h"
#include "uc_adda.h"
//#include "uc_boot_download.h"
#include "at_wiota.h"
#include "uc_wiota_gateway_api.h"

enum at_wiota_manager_process
{
    AT_WIOTA_MANAGER_PROCESS_DEFAULT = 0,
    AT_WIOTA_MANAGER_PROCESS_SCAN,
    AT_WIOTA_MANAGER_PROCESS_INIT,
    AT_WIOTA_MANAGER_PROCESS_RUN,
    AT_WIOTA_MANAGER_PROCESS_STRATEGY,
    AT_WIOTA_MANAGER_PROCESS_EXIT,
#ifdef AT_WIOTA_GATEWAY_API
    AT_WIOTA_MANAGER_PROCESS_GW_MODE_EXIT,
#endif
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

#ifdef AT_WIOTA_GATEWAY_API
extern unsigned char uc_wiota_gateway_get_allow_run_flag(void);
extern unsigned int uc_wiota_gateway_get_wiota_id(void);
extern void uc_wiota_gateway_api_handle(void *para);
#endif

typedef struct freq_list_manager_node
{
    unsigned int subsystem_id;
    unsigned char freq;
    signed char snr;
    signed char rssi;
    unsigned char is_synced;
    char send_cucess_rate;
} t_freq_node_manager;

typedef struct freq_list_manager
{
    t_freq_node_manager data;
    rt_list_t node;
} t_freq_list_manager;

typedef struct at_wiota_manager_parament
{
    rt_thread_t task_handle;
    t_freq_list_manager freq_list;
    t_freq_list_manager *current_freq_node;
    int continue_scan_fail;
    char manager_state;
    char wiota_state;
} t_at_wiota_manager;

#define SET_WIOTA_STATE(state) g_wiota_manager.wiota_state = state
#define GET_WIOTA_STATE g_wiota_manager.wiota_state

#define SET_MANAGER_PROCESS(state) g_wiota_manager.manager_state = state
#define GET_MANAGER_PROCESS g_wiota_manager.manager_state

extern dtu_send_t g_dtu_send;

extern void wiota_recv_callback(uc_recv_back_p data);

t_at_wiota_manager g_wiota_manager = {0};

static int at_wiota_get_static_freq(char *list)
{
    int num;
    uc_wiota_get_freq_list((unsigned char *)list);

    for (num = 0; num < 16; num++)
    {
        rt_kprintf("static freq *(list+%d) %d\n", num, *(list+num));
        if (*(list + num) == 0xFF)
            break;
    }
    return num;
}

static int at_wiota_get_subsystem_id_list(unsigned int *list)
{
    int num;
    sub_system_config_t config;
    uc_wiota_get_system_config(&config);

    memcpy(list, config.subsystemid_list, sizeof(config.subsystemid_list));

    for (num = 0; num < sizeof(config.subsystemid_list); num++)
    {
        rt_kprintf("static subsystem_id *(list+%d) %d\n", num, *(list+num));
        if (*(list + num) == (~0) || *(list + num) == 0)
            break;
    }

    if (0 == num)
    {
        *list = config.subsystemid;

         if (*(list + num) != (~0) && *(list + num) != 0)
            num = 1;
    }

    return num;
}

static void at_wiota_init_freq_list(void)
{
    //rt_memset(&g_wiota_manager, 0, sizeof(g_wiota_manager));
    rt_list_init(&g_wiota_manager.freq_list.node);
}
static void at_wiota_print_freq_list(void)
{
    t_freq_list_manager *temp_node;

    rt_list_for_each_entry(temp_node, &g_wiota_manager.freq_list.node, node)
    {
        rt_kprintf("address 0x%x freq %d snr %d rssi %d is_synced %d\n",
                   temp_node, temp_node->data.freq, temp_node->data.snr, temp_node->data.rssi, temp_node->data.is_synced);
    }
}

static t_freq_list_manager *manager_wiota_find_node(unsigned char freq)
{
    t_freq_list_manager *temp_node;

    rt_list_for_each_entry(temp_node, &g_wiota_manager.freq_list.node, node)
    {
        if (temp_node->data.freq == freq)
        {
            return temp_node;
        }
    }
    return RT_NULL;
}

static void at_wiota_add_freq_list(unsigned int subsystem_id, uc_freq_scan_result_p node)
{
    t_freq_list_manager *temp = manager_wiota_find_node(node->freq_idx);
    if (temp)
    {
        temp->data.subsystem_id = subsystem_id;
        temp->data.snr = node->snr;
        temp->data.rssi = node->rssi;
        temp->data.is_synced = node->is_synced;
        temp->data.send_cucess_rate = 0;
    }
    else
    {
        t_freq_list_manager *new_node = rt_malloc(sizeof(t_freq_list_manager));
        if (new_node == RT_NULL)
        {
            rt_kprintf("%s line %d malloc error\n", __FUNCTION__, __LINE__);
            return;
        }
        //rt_kprintf("%s line %d malloc new_node 0x%x id 0x%x freq %d\n", __FUNCTION__, __LINE__, new_node, subsystem_id, node->freq_idx);
        new_node->data.subsystem_id = subsystem_id;
        new_node->data.freq = node->freq_idx;
        new_node->data.is_synced = node->is_synced;
        new_node->data.rssi = node->rssi;
        new_node->data.snr =  node->snr;
        new_node->data.send_cucess_rate = 0;

        t_freq_list_manager *temp_node;

        rt_list_for_each_entry(temp_node, &g_wiota_manager.freq_list.node, node)
        {
            if (temp_node->data.rssi < new_node->data.rssi)
            {
                rt_list_insert_before(&temp_node->node, &new_node->node);
                return;
            }
        }

        rt_list_insert_after(g_wiota_manager.freq_list.node.prev, &new_node->node);
    }
}
#if 0
static void manager_wiota_remove_freq_list(uc_freq_scan_result_p node)
{
    t_freq_list_manager *temp = manager_wiota_find_node(node->freq_idx);
    if (temp)
    {
        rt_list_remove(&temp->node);
        rt_free(temp);
        temp = RT_NULL;
    }
}
#endif
static void at_wiota_clean_freq_list(void)
{
    rt_list_t *next_node = g_wiota_manager.freq_list.node.next;
   while (next_node != &g_wiota_manager.freq_list.node)

     {
        t_freq_list_manager *temp_node = rt_list_entry(next_node, t_freq_list_manager, node);
        //rt_kprintf("at_wiota_clean_freq_list temp_node 0x%x\n", temp_node);
         rt_free(temp_node);
        temp_node = RT_NULL;
        next_node = next_node->next;
     }


    rt_kprintf("at_wiota_clean_freq_list finish\n");
    g_wiota_manager.current_freq_node = RT_NULL;
    at_wiota_init_freq_list();
}

static int at_wiota_choose_freq(void)
{
    t_freq_list_manager *temp_node;
    rt_list_t *the_node = (g_wiota_manager.current_freq_node == RT_NULL)?
        (&g_wiota_manager.freq_list.node) :
        (&g_wiota_manager.current_freq_node->node);

    if(rt_list_isempty(&g_wiota_manager.freq_list.node))
    {
        rt_kprintf("at_wiota_choose_freq freq_list is epty\n");
        return 1;
    }

    rt_list_for_each_entry(temp_node, the_node, node)
    {
        g_wiota_manager.current_freq_node = temp_node;
        break;
    }

    if (g_wiota_manager.current_freq_node == (&g_wiota_manager.freq_list))
    {
        rt_kprintf("at_wiota_choose_freq no entry\n");
        return 2;
    }

    //rt_kprintf("at_wiota_choose_freq current_freq_node 0x%x id 0x%x freq %d\n",
     //   g_wiota_manager.current_freq_node, g_wiota_manager.current_freq_node->data.subsystem_id, g_wiota_manager.current_freq_node->data.freq);

    return 0;
}

static int at_wiota_only_freq(unsigned int subsystem_id, char freq)
{
    uc_freq_scan_result_t result_list = {0};

    result_list.freq_idx = freq;
    at_wiota_add_freq_list(subsystem_id, &result_list);
    return 0;
}

static int at_wiota_freq_manager(unsigned int subsystem_id, uc_recv_back_t result)
{
    int re = 1;

    if (UC_OP_SUCC == result.result)
    {
        uc_freq_scan_result_p freq_list = (uc_freq_scan_result_p)result.data;
        int freq_num = result.data_len / sizeof(uc_freq_scan_result_t);
        int i = 0;

        for (i = 0; i < freq_num; i++)
        {
            // rt_kprintf("%s line %d freq_num %d i %d index %d is_synced %d snr %d\n", __FUNCTION__, __LINE__, freq_num, i, freq_list->freq_idx, freq_list->is_synced, freq_list->snr);
            if (freq_list->is_synced )
            {
                at_wiota_add_freq_list(subsystem_id, freq_list);
                re = 0;
            }
            //else
            //{
            //    manager_wiota_remove_freq_list(freq_list);
            //}
            freq_list++;
        }

        if (!re)
        {
            at_wiota_print_freq_list();
        }

        rt_free(result.data);
    }

    return re;
}

static int at_wiota_manager_scan(void)
{
    sub_system_config_t config;
    uc_recv_back_t result;
    t_at_wiota_manager *manager = &g_wiota_manager;
    unsigned int subsystem_id_list[8] = {0};
    unsigned char freq_list[16] = {0};
    char freq_list_len = 0;
    char subsystem_id_len = 0;
    char strategy = 0;
    int res = 0;

    uc_wiota_init();

#ifdef AT_WIOTA_GATEWAY_API
    unsigned int userid = uc_wiota_gateway_get_dev_address();
    uc_wiota_set_userid( &userid , 4);
#endif

    at_wiota_set_state(AT_WIOTA_INIT);

    uc_wiota_get_system_config(&config);

    //uc_wiota_set_dcxo(0x22000);
    uc_wiota_run();
    uc_wiota_register_recv_data_callback(wiota_recv_callback, UC_CALLBACK_NORAMAL_MSG);

    uc_wiota_register_recv_data_callback(wiota_recv_callback, UC_CALLBACK_STATE_INFO);
    at_wiota_set_state(AT_WIOTA_RUN);

    at_wiota_clean_freq_list();

    freq_list_len = at_wiota_get_static_freq((char *)freq_list);
    subsystem_id_len = at_wiota_get_subsystem_id_list(subsystem_id_list);

    rt_kprintf("scan listLen %d.contScanFail %d subsysid %d\n", freq_list_len, manager->continue_scan_fail, subsystem_id_len);

    strategy = freq_list_len * subsystem_id_len;

    switch (strategy)
    {
    case 0:
    {
        uc_wiota_scan_freq(RT_NULL, 0, 0, AT_WIOTA_FULL_SCAN_TIMEOUT, RT_NULL, &result);
        rt_kprintf("uc_wiota_scan_freq result %d\n", result.result);
        //if (manager->continue_scan_fail < AT_WIOTA_CONTINE_SCAN_FAIL_MAX)
            res = at_wiota_freq_manager(config.subsystemid, result);
        //else
         //   res = at_wiota_freq_manager(result, 1);
        if (res)
            manager->continue_scan_fail++;
        break;
    }
    case 1:
    {
        at_wiota_only_freq(config.subsystemid, freq_list[0]);
        if (manager->continue_scan_fail > 2 &&
            manager->continue_scan_fail % 2 == 0)
            res = 1; // return fail. enter sleep.
        rt_kprintf("res = %d fail counter %d\n",  res, manager->continue_scan_fail);
        manager->continue_scan_fail++;

        break;
    }
    default:
    {
        unsigned char i = 0;
        char flag = 0;
        unsigned int id;

        uc_wiota_get_userid(&id, &i);

        for (i = (id%subsystem_id_len); i < subsystem_id_len + (id % subsystem_id_len); i ++)
        {
            config.subsystemid = subsystem_id_list[i % subsystem_id_len];
            uc_wiota_set_system_config(&config);

            uc_wiota_scan_freq(freq_list, freq_list_len, 0, AT_WIOTA_SCAN_TIMEOUT, RT_NULL, &result);

            rt_kprintf("uc_wiota_scan_freq result %d subsystemid 0x%x timeout %d\n",
                    result.result, config.subsystemid, AT_WIOTA_SCAN_TIMEOUT);

            res = at_wiota_freq_manager(config.subsystemid, result);
            if (res)
                flag ++;
        }

        if (flag)
            manager->continue_scan_fail++;

        res = (flag == subsystem_id_len ? 1: 0);

        break;
    }
    }
    uc_wiota_exit();
    at_wiota_set_state(AT_WIOTA_EXIT);

    return res;
}

static int at_wiota_manager_run(void)
{
    uc_wiota_run();
    uc_wiota_register_recv_data_callback(wiota_recv_callback, UC_CALLBACK_NORAMAL_MSG);
    uc_wiota_register_recv_data_callback(wiota_recv_callback, UC_CALLBACK_STATE_INFO);
    at_wiota_set_state(AT_WIOTA_RUN);
    uc_wiota_connect();
    return uc_wiota_wait_sync(5000);
}

static void at_wiota_manager_startegy(void)
{
    uc_stats_info_t stats_info_ptr;
    uc_wiota_reset_stats(UC_STATS_TYPE_ALL);
    while (1)
    {
        int counter = 0;
        // get state
        uc_wiota_status_e connect_state = uc_wiota_get_state();
        uc_wiota_get_all_stats(&stats_info_ptr);

        if ((stats_info_ptr.ul_sm_succ * 5 < stats_info_ptr.ul_sm_total && stats_info_ptr.ul_sm_total > 20) ||
            UC_STATUS_SYNC_LOST == connect_state || UC_STATUS_ERROR == connect_state
        )
        {
            rt_kprintf("startegy smSucc %d smTotal %d connectState %d\n",
                    stats_info_ptr.ul_sm_succ, stats_info_ptr.ul_sm_total, connect_state);
            return;
        }

        uc_wiota_reset_stats(UC_STATS_TYPE_ALL);
        //rt_thread_mdelay(5000);
        while((counter ++) < 1000)
        {
            if(uc_wiota_gateway_get_allow_run_flag())
            {
                uc_wiota_gateway_api_handle(RT_NULL);
                break;
            }
            else
                rt_thread_mdelay(5);
        }

    }
}

static void at_wiota_set_config(unsigned int subsystem_id)
{
    sub_system_config_t config;
    uc_wiota_get_system_config(&config);
    config.subsystemid = subsystem_id;
    uc_wiota_set_system_config(&config);
}

static void at_wiota_auto_report_state(int type)
{
    //rt_kprintf("at_wiota_auto_report_state type = %d\n", type);
    switch (type)
    {
    case AT_WIOTA_MANAGER_REPORT_FREQ_SUC:
    {
        t_freq_list_manager *tmp;

        rt_list_for_each_entry(tmp, &g_wiota_manager.freq_list.node, node)
        {
            at_server_printfln("+SCANFFREQ:%d,%d,%d,%d\n", tmp->data.freq, tmp->data.snr, tmp->data.rssi, tmp->data.is_synced);
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
        //rt_kprintf("%s line %d current_freq_node 0x%x\n", __FUNCTION__, __LINE__, g_wiota_manager.current_freq_node);
        if (g_wiota_manager.current_freq_node != RT_NULL)
        {
            at_server_printfln("+WIOTAFREQ:0x%x,%d,%d",
                            g_wiota_manager.current_freq_node->data.subsystem_id,
                            g_wiota_manager.current_freq_node->data.freq,
                            g_wiota_manager.current_freq_node->data.snr);
        }
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

static void at_wiota_manager_task(void *parameter)
{
    SET_MANAGER_PROCESS(AT_WIOTA_MANAGER_PROCESS_SCAN);

    while (AT_WIOTA_MANAGER_PROCESS_END != GET_MANAGER_PROCESS)
    {
        rt_kprintf("PROCESS state %d\n", GET_MANAGER_PROCESS);
        switch (GET_MANAGER_PROCESS)
        {
        case AT_WIOTA_MANAGER_PROCESS_SCAN:
        {
            if (at_wiota_manager_scan())
            {
                at_wiota_auto_report_state(AT_WIOTA_MANAGER_REPORT_FREQ_FAIL);
                rt_kprintf("scantf fail.delay %d s\n", g_wiota_manager.continue_scan_fail);
                rt_thread_mdelay(1000 * g_wiota_manager.continue_scan_fail);
            }
            else
            {
                SET_MANAGER_PROCESS(AT_WIOTA_MANAGER_PROCESS_INIT);
                at_wiota_auto_report_state(AT_WIOTA_MANAGER_REPORT_FREQ_SUC);
            }

            break;
        }
        case AT_WIOTA_MANAGER_PROCESS_INIT:
        {
            if (at_wiota_choose_freq())
            {
                SET_MANAGER_PROCESS(AT_WIOTA_MANAGER_PROCESS_SCAN);
                break;
            }

            at_wiota_auto_report_state(AT_WIOTA_MANAGER_USER_FREQ);
            rt_kprintf("%d id 0x%x freq %d\n",
                g_wiota_manager.current_freq_node->data.subsystem_id, g_wiota_manager.current_freq_node->data.freq);

            uc_wiota_init();
#ifdef AT_WIOTA_GATEWAY_API
            unsigned int userid = uc_wiota_gateway_get_dev_address();

            // rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);

            //if (uc_wiota_gateway_get_reboot_flag())
            //{
            //    userid = uc_wiota_gateway_get_wiota_id();
            //}

            uc_wiota_set_userid( &userid , 4);
#endif

            at_wiota_set_config(g_wiota_manager.current_freq_node->data.subsystem_id);
            uc_wiota_set_freq_info(g_wiota_manager.current_freq_node->data.freq);
            at_wiota_set_state(AT_WIOTA_INIT);
            SET_MANAGER_PROCESS(AT_WIOTA_MANAGER_PROCESS_RUN);
            break;
        }
        case AT_WIOTA_MANAGER_PROCESS_RUN:
        {
            if (at_wiota_manager_run())
            {
                SET_MANAGER_PROCESS(AT_WIOTA_MANAGER_PROCESS_EXIT);
                at_wiota_auto_report_state(AT_WIOTA_MANAGER_CONNECT_FAIL);
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
            at_wiota_auto_report_state(AT_WIOTA_MANAGER_CONNECT_SUC);
            at_wiota_manager_startegy();
            SET_MANAGER_PROCESS(AT_WIOTA_MANAGER_PROCESS_EXIT);

            break;
        }
        case AT_WIOTA_MANAGER_PROCESS_EXIT:
        {
            uc_wiota_exit();
            at_wiota_clean_freq_list();
            at_wiota_auto_report_state(AT_WIOTA_MANAGER_PROCESS_EXIT);
            at_wiota_set_state(AT_WIOTA_EXIT);
            SET_MANAGER_PROCESS(AT_WIOTA_MANAGER_PROCESS_INIT);
            break;
        }
        }
    }
}

void at_wiota_get_avail_freq_list(unsigned char *output_list, unsigned char list_len)
{
    t_freq_list_manager *temp_node;
    unsigned char index = 0;
    unsigned char used_freq = uc_wiota_get_freq_info();

    if(g_wiota_manager.current_freq_node != RT_NULL)
    {
        rt_list_for_each_entry(temp_node, &g_wiota_manager.freq_list.node, node)
        {
            if(temp_node->data.freq != used_freq)
            {
                output_list[index++] = temp_node->data.freq;
                if(index >= list_len)
                {
                    break;
                }
            }

        }
    }
}

void at_wiota_manager_suspend(void)
{
    if (RT_NULL != g_wiota_manager.task_handle)
        rt_thread_suspend(g_wiota_manager.task_handle);
}

void at_wiota_manager(void)
{
    if (uc_wiota_get_auto_connect_flag() /*&& GET_MANAGER_PROCESS == AT_WIOTA_MANAGER_PROCESS_DEFAULT*/)
    {
        at_wiota_init_freq_list();
        // rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
        g_wiota_manager.task_handle = rt_thread_create("auMana",
                                                       at_wiota_manager_task,
                                                       RT_NULL,
                                                       1024,
                                                       RT_THREAD_PRIORITY_MAX / 3 - 1,
                                                       3);
        if (RT_NULL == g_wiota_manager.task_handle)
        {
            rt_kprintf("create auMana error\n");
            return;
        }
        rt_thread_startup(g_wiota_manager.task_handle);
    }
}

#endif
