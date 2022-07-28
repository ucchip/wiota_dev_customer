#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include <board.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "uc_string_lib.h"
#include "uc_adda.h"
#include "app_manager_cfg.h"

enum app_manager_process
{
    APP_MANAGER_PROCESS_DEFAULT = 0,
    APP_MANAGER_PROCESS_SCAN,
    APP_MANAGER_PROCESS_INIT,
    APP_MANAGER_PROCESS_RUN,
    APP_MANAGER_PROCESS_STRATEGY,
    APP_MANAGER_PROCESS_EXIT,
    APP_MANAGER_PROCESS_END,
};

enum app_manager_report_state
{
    APP_MANAGER_REPORT_FREQ_SUC = 0,
    APP_MANAGER_REPORT_FREQ_FAIL,
    APP_MANAGER_USER_FREQ,
    APP_MANAGER_CONNECT_SUC,
    APP_MANAGER_CONNECT_FAIL,
    APP_MANAGER_EXIT,
};

#define AT_WIOTA_SCAN_TIMEOUT 40000
#define AT_WIOTA_FULL_SCAN_TIMEOUT 500000
#define AT_WIOTA_CONTINE_SCAN_FAIL_MAX 3

typedef struct freq_list_manager_node
{
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
    //rt_thread_t task_handle;
    t_freq_list_manager freq_list;
    t_freq_list_manager *current_freq_node;
    int continue_scan_fail;
    char manager_state;
    char wiota_state;
} t_app_manager_freq;

#define SET_MANAGE_FREQ_STATE(state) g_app_manager_freq.wiota_state = state
#define GET_MANAGE_FREQ_STATE g_app_manager_freq.wiota_state

#define SET_MANAGER_FREQ_PROCESS(state) g_app_manager_freq.manager_state = state
#define GET_MANAGER_FREQ_PROCESS g_app_manager_freq.manager_state

extern void wiota_recv_callback(uc_recv_back_p data);

t_app_manager_freq g_app_manager_freq = {0};

static int at_wiota_get_static_freq(char *list)
{
    int num;
    uc_wiota_get_freq_list((unsigned char *)list);

    for (num = 0; num < 16; num++)
    {
        //rt_kprintf("static freq *(list+%d) %d\n", num, *(list+num));
        if (*(list + num) == 0xFF)
            break;
    }
    return num;
}

static void init_freq_manager_list(void)
{
    rt_memset(&g_app_manager_freq, 0, sizeof(t_app_manager_freq));
    rt_list_init(&g_app_manager_freq.freq_list.node);
}
static void at_wiota_print_freq_list(void)
{
    t_freq_list_manager *temp_node;

    rt_list_for_each_entry(temp_node, &g_app_manager_freq.freq_list.node, node)
    {
        rt_kprintf("address 0x%x freq %d snr %d rssi %d is_synced %d\n",
                   temp_node, temp_node->data.freq, temp_node->data.snr, temp_node->data.rssi, temp_node->data.is_synced);
    }
}

static t_freq_list_manager *manager_wiota_find_node(unsigned char freq)
{
    t_freq_list_manager *temp_node;

    rt_list_for_each_entry(temp_node, &g_app_manager_freq.freq_list.node, node)
    {
        if (temp_node->data.freq == freq)
        {
            return temp_node;
        }
    }
    return RT_NULL;
}

static void at_wiota_add_freq_list(uc_freq_scan_result_p node, u8_t flag)
{
    t_freq_list_manager *temp = manager_wiota_find_node(node->freq_idx);
    if (temp)
    {
        temp->data.snr = (flag ? node->rssi : node->snr);
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
        new_node->data.freq = node->freq_idx;
        new_node->data.is_synced = node->is_synced;
        new_node->data.rssi = node->rssi;
        new_node->data.snr = (flag ? node->rssi : node->snr);
        new_node->data.send_cucess_rate = 0;

        t_freq_list_manager *temp_node;

        rt_list_for_each_entry(temp_node, &g_app_manager_freq.freq_list.node, node)
        {
            if (temp_node->data.rssi < new_node->data.rssi)
            {
                rt_list_insert_before(&temp_node->node, &new_node->node);
                return;
            }
        }

        rt_list_insert_after(g_app_manager_freq.freq_list.node.prev, &new_node->node);
    }
}

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

static void at_wiota_clean_freq_list(void)
{
    rt_list_t *next_node = g_app_manager_freq.freq_list.node.next;
    while (next_node != &g_app_manager_freq.freq_list.node)
    {
        t_freq_list_manager *temp_node = rt_list_entry(next_node, t_freq_list_manager, node);
        rt_free(temp_node);
        temp_node = RT_NULL;
        next_node = next_node->next;
    }
    init_freq_manager_list();
}

static int at_wiota_choose_freq(void)
{
    rt_list_t *head = &(g_app_manager_freq.freq_list.node);
    rt_list_t *tmp = head->next;

    rt_kprintf("%s line %d tmp 0x%x\n", __FUNCTION__, __LINE__, tmp);

    if (tmp == head)
    {
        rt_kprintf("%s line %d list is null\n", __FUNCTION__, __LINE__);
        return 1;
    }
    if (g_app_manager_freq.current_freq_node == RT_NULL)
    {
        g_app_manager_freq.current_freq_node = rt_list_entry(tmp, t_freq_list_manager, node);
    }
    else
    {
        tmp = g_app_manager_freq.current_freq_node->node.next;
        if (tmp == head)
        {
            g_app_manager_freq.current_freq_node = RT_NULL;
            return 2;
        }
        g_app_manager_freq.current_freq_node = rt_list_entry(tmp, t_freq_list_manager, node);
    }

    return 0;
}

static int at_wiota_only_freq(char freq)
{
    uc_freq_scan_result_t result_list = {0};

    result_list.freq_idx = freq;
    at_wiota_add_freq_list(&result_list, 0);
    return 0;
}

static int at_wiota_freq_manager(uc_recv_back_t result, u8_t flag)
{
    int re = 1;

    if (UC_OP_SUCC == result.result || flag)
    {
        uc_freq_scan_result_p freq_list = (uc_freq_scan_result_p)result.data;
        int freq_num = result.data_len / sizeof(uc_freq_scan_result_t);
        int i = 0;

        for (i = 0; i < freq_num; i++)
        {
            rt_kprintf("%s line %d freq_num %d i %d index %d is_synced %d snr %d\n", __FUNCTION__, __LINE__, freq_num, i, freq_list->freq_idx, freq_list->is_synced, freq_list->snr);
            if (freq_list->is_synced || flag)
            {
                at_wiota_add_freq_list(freq_list, flag);
            }
            else
            {
                manager_wiota_remove_freq_list(freq_list);
            }
            re = 0;
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

static int app_manager_scan(void)
{
    uc_recv_back_t result;
    t_app_manager_freq *manager = &g_app_manager_freq;
    u8_t list[16] = {0};
    int list_len = 0;
    int res;

    uc_wiota_init();
    static unsigned char first_setup = 1;
    if (first_setup)
    {
        manager_set_wiotaid(manager_get_deviceid());
        first_setup = 0;
    }

    //uc_wiota_set_dcxo(0x22000);
    uc_wiota_run();
    at_wiota_clean_freq_list();
    //uc_wiota_register_recv_data_callback(wiota_recv_callback,UC_CALLBACK_NORAMAL_MSG);
    //uc_wiota_register_recv_data_callback(wiota_recv_callback,UC_CALLBACK_STATE_INFO);

    rt_kprintf("manager->continue_scan_fail  %d\n", manager->continue_scan_fail);

    list_len = at_wiota_get_static_freq((char *)list);
    rt_kprintf("%s line %d list_len %d\n", __FUNCTION__, __LINE__, list_len);

    switch (list_len)
    {
    case 0:
    {
        uc_wiota_scan_freq(RT_NULL, 0, AT_WIOTA_FULL_SCAN_TIMEOUT, RT_NULL, &result);
        rt_kprintf("%s line %d uc_wiota_scan_freq result %d\n", __FUNCTION__, __LINE__, result.result);
        if (manager->continue_scan_fail < AT_WIOTA_CONTINE_SCAN_FAIL_MAX)
            res = at_wiota_freq_manager(result, 0);
        else
            res = at_wiota_freq_manager(result, 1);
        if (res)
            manager->continue_scan_fail++;
        break;
    }
    case 1:
    {
        res = at_wiota_only_freq(list[0]);
        if (manager->continue_scan_fail > 2 &&
            manager->continue_scan_fail % 2 == 0)
            res = 1; // return fail. enter sleep.
        rt_kprintf("%s line %d res = %d fail counter %d\n", __FUNCTION__, __LINE__, res, manager->continue_scan_fail);
        manager->continue_scan_fail++;

        break;
    }
    default:
    {
        uc_wiota_scan_freq(list, list_len, AT_WIOTA_SCAN_TIMEOUT, RT_NULL, &result);
        rt_kprintf("%s line %d uc_wiota_scan_freq result %d\n", __FUNCTION__, __LINE__, result.result);
        if (manager->continue_scan_fail < AT_WIOTA_CONTINE_SCAN_FAIL_MAX)
            res = at_wiota_freq_manager(result, 0);
        else
            res = at_wiota_freq_manager(result, 1);
        if (res)
            manager->continue_scan_fail++;
        break;
    }
    }
    uc_wiota_exit();
    //at_wiota_set_state(AT_WIOTA_EXIT);

    return res;
}

static int at_wiota_manager_run(void)
{
    signed char num = 20;
    unsigned char counter = 0;
    uc_wiota_run();
    //at_wiota_set_state(AT_WIOTA_RUN);

    uc_wiota_connect();

    while (num--)
    {
        rt_thread_mdelay(100);
        if (UC_STATUS_SYNC == uc_wiota_get_state())
        {
            if (counter > 4)
                break;
            counter++;
        }
        else
            counter = 0;
    }
    return (num > 0 ? 0 : 1);
}

static void at_wiota_manager_startegy(void)
{
    uc_stats_info_t stats_info_ptr;
    uc_wiota_reset_stats(UC_STATS_TYPE_ALL);
    while (1)
    {
        // get state
        UC_WIOTA_STATUS connect_state = uc_wiota_get_state();
        uc_wiota_get_all_stats(&stats_info_ptr);

        if ((stats_info_ptr.ul_sm_succ * 5 < stats_info_ptr.ul_sm_total && stats_info_ptr.ul_sm_total > 20) ||
            UC_STATUS_SYNC_LOST == connect_state || UC_STATUS_ERROR == connect_state)
        {
            rt_kprintf("%s line %d sm_succ %d sm_total %d connect state %d\n",
                       __FUNCTION__, __LINE__, stats_info_ptr.ul_sm_succ, stats_info_ptr.ul_sm_total, connect_state);
            return;
        }
        uc_wiota_reset_stats(UC_STATS_TYPE_ALL);
        rt_thread_mdelay(5000);
    }
}

static void app_manager_report_state(int type)
{
    rt_kprintf("app_manager_report_state type = %d\n", type);
    switch (type)
    {
    case APP_MANAGER_REPORT_FREQ_SUC:
    {
        t_freq_list_manager *tmp;

        rt_list_for_each_entry(tmp, &g_app_manager_freq.freq_list.node, node)
        {
            rt_kprintf("+SCANFFREQ:%d,%d,%d,%d\n", tmp->data.freq, tmp->data.snr, tmp->data.rssi, tmp->data.is_synced);
        }
        break;
    }
    case APP_MANAGER_REPORT_FREQ_FAIL:
    {
        rt_kprintf("+WIOTASCANF:FAIL\n");
        break;
    }
    case APP_MANAGER_USER_FREQ:
    {
        if (g_app_manager_freq.current_freq_node != RT_NULL)
            rt_kprintf("+WIOTAFREQ:%d,%d\n",
                       g_app_manager_freq.current_freq_node->data.freq, g_app_manager_freq.current_freq_node->data.snr);
        break;
    }
    case APP_MANAGER_CONNECT_SUC:
    {
        rt_kprintf("+WIOTA:READY\n");
        break;
    }
    case APP_MANAGER_CONNECT_FAIL:
    {
        rt_kprintf("+WIOTA:CONNECT FAIL\n");
        break;
    }
    case APP_MANAGER_EXIT:
    {
        rt_kprintf("+WIOTA:EXIT\n");
        break;
    }
    }
}

void app_manager_freq(void)
{
    init_freq_manager_list();

    SET_MANAGER_FREQ_PROCESS(APP_MANAGER_PROCESS_SCAN);

    while (APP_MANAGER_PROCESS_STRATEGY != GET_MANAGER_FREQ_PROCESS)
    {
        rt_kprintf("%s line %d state %d\n", __FUNCTION__, __LINE__, GET_MANAGER_FREQ_PROCESS);
        switch (GET_MANAGER_FREQ_PROCESS)
        {
        case APP_MANAGER_PROCESS_SCAN:
        {
            if (app_manager_scan())
            {
                SET_MANAGER_FREQ_PROCESS(APP_MANAGER_PROCESS_SCAN);
                app_manager_report_state(APP_MANAGER_REPORT_FREQ_FAIL);
                rt_thread_mdelay(1000 * g_app_manager_freq.continue_scan_fail);
            }
            else
            {
                SET_MANAGER_FREQ_PROCESS(APP_MANAGER_PROCESS_INIT);
                app_manager_report_state(APP_MANAGER_REPORT_FREQ_SUC);
            }

            break;
        }
        case APP_MANAGER_PROCESS_INIT:
        {
            if (at_wiota_choose_freq())
            {
                SET_MANAGER_FREQ_PROCESS(APP_MANAGER_PROCESS_SCAN);
                break;
            }
            uc_wiota_init();
            app_manager_report_state(APP_MANAGER_USER_FREQ);
            rt_kprintf("%s line %d freq %d\n", __FUNCTION__, __LINE__, g_app_manager_freq.current_freq_node->data.freq);
            uc_wiota_set_freq_info(g_app_manager_freq.current_freq_node->data.freq);
            //at_wiota_set_state(AT_WIOTA_INIT);
            SET_MANAGER_FREQ_PROCESS(APP_MANAGER_PROCESS_RUN);
            break;
        }
        case APP_MANAGER_PROCESS_RUN:
        {
            if (at_wiota_manager_run())
            {
                SET_MANAGER_FREQ_PROCESS(APP_MANAGER_PROCESS_EXIT);
                app_manager_report_state(APP_MANAGER_CONNECT_FAIL);
            }
            else
            {
                SET_MANAGER_FREQ_PROCESS(APP_MANAGER_PROCESS_STRATEGY);
                g_app_manager_freq.continue_scan_fail = 0;
            }
            break;
        }
        case APP_MANAGER_PROCESS_STRATEGY:
        {
            app_manager_report_state(APP_MANAGER_CONNECT_SUC);
            at_wiota_manager_startegy();
            SET_MANAGER_FREQ_PROCESS(APP_MANAGER_PROCESS_EXIT);
            break;
        }

#if 1
        case APP_MANAGER_PROCESS_EXIT:
        {
            uc_wiota_exit();
            app_manager_report_state(APP_MANAGER_PROCESS_EXIT);
            SET_MANAGER_FREQ_PROCESS(APP_MANAGER_PROCESS_INIT);
            break;
        }
#endif
        }
    }
}

void app_manager_exit_wiota(void)
{
    at_wiota_clean_freq_list();
    uc_wiota_exit();
}
#endif