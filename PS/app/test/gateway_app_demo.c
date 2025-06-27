
#include <rtthread.h>
#include <rtdevice.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "uc_wiota_gateway_api.h"
#include "gateway_app_demo.h"

// freq
#define INVALID_FREQ 0xFF
#define DEFAULT_FREQ 160
#define MAX_FREQ 200
#define MIN_FREQ 0
#define FREQ_LIST_NUM 16
#define MIN_FREQ_LIST_NUM 1

// subsystem id
#define INVALID_SUBSYSTEM_ID1 0
#define INVALID_SUBSYSTEM_ID2 0xFFFFFFFF
#define DEFAULT_SUBSYSTEM_ID 0x21456981
#define SUBSYSTEM_ID_NUM 8
#define MIN_SUBSYSTEM_ID_NUM 1

// scan timeout max
#define WIOTA_SCAN_TIMEOUT (2 * 60 * 1000)

#define WIOTA_CONNECT_TIMEOUT 5000
#define WIOTA_WAIT_CONNECT_FN 2

// user static data config
typedef struct
{
    unsigned short factory_code;
    unsigned char ps_log_switch;
    unsigned char ex32k_enable;
    unsigned char max_power;
    unsigned char default_power;
    unsigned char mcs;
    unsigned char gw_mode;
    char auth_code[16];
    char device_type[16];
    // and so on
} user_cfg_t;

typedef enum
{
    MSG_TYPE_SCAN_FREQ = 0,
    MSG_TYPE_SYNC_TO_AP = 1,
    MSG_TYPE_RECV_DATA = 2,
    MSG_TYPE_SEND_DATA = 3,
    MSG_TYPE_GW_ABNORMAL = 4,
    MSG_TYPE_MAX = 0xff
} gw_msg_type_e;

typedef struct
{
    int msg_type; // gw_msg_type_e
    int data_len;
    void *data;
} gw_app_msg_t;

typedef enum
{
    SYNC_TO_AP = 0,
    AUTH_FAIL = 1,
    GW_ABNORMAL = 2,
    // other state
    SYNC_STATE_MAX = 0xff // default
} freq_state_e;

typedef struct
{
    rt_list_t node;
    unsigned char freq_idx;
    signed char rssi;
    unsigned char snr;
    unsigned char state; // freq_state_e
    unsigned int subsystemid;
} freq_info_t;

typedef struct
{
    rt_list_t freq_info_list; // node freq_info_t
    freq_info_t last_freq_info;
} sync_info_t;

static user_cfg_t *g_user_cfg = RT_NULL;
static rt_mq_t g_gw_mq = RT_NULL;
static sync_info_t g_sync_info = {0};

static void gateway_user_cfg_set_default(void)
{
    user_cfg_t user_cfg = {
        .factory_code = 0x1001,
        .ps_log_switch = 1,
        .ex32k_enable = 0,
        .max_power = 21,
        .default_power = 0,
        .mcs = 2,
        .gw_mode = UC_GATEWAY_MODE,
        .auth_code = "123456",
        .device_type = "iote",
    };

    rt_memcpy(g_user_cfg, &user_cfg, sizeof(user_cfg_t));

    uc_wiota_save_static_info();

    rt_kprintf("user_cfg_set_default\n");
}

static void gateway_user_cfg_init(void)
{
    g_user_cfg = (user_cfg_t *)uc_wiota_get_user_info();
    RT_ASSERT(g_user_cfg);

    if (g_user_cfg->factory_code != 0x1001)
    {
        gateway_user_cfg_set_default();
    }

    rt_kprintf("-- -user_cfg----\n");
    rt_kprintf("factory_code  0x%x\n", g_user_cfg->factory_code);
    rt_kprintf("ps_log_switch %d\n", g_user_cfg->ps_log_switch);
    rt_kprintf("ex32k_enable  %d\n", g_user_cfg->ex32k_enable);
    rt_kprintf("max_power     %d\n", g_user_cfg->max_power);
    rt_kprintf("default_power %d\n", g_user_cfg->default_power);
    rt_kprintf("mcs           %d\n", g_user_cfg->mcs);
    rt_kprintf("gw_mode       %d\n", g_user_cfg->gw_mode);
    rt_kprintf("auth_code     %s\n", g_user_cfg->auth_code);
    rt_kprintf("device_type   %s\n", g_user_cfg->device_type);
    rt_kprintf("-- -user_cfg----\n");
}

user_cfg_t *gateway_user_cfg_get(void)
{
    return g_user_cfg;
}

static int gateway_send_msg(int cmd, void *data, int data_len)
{
    gw_app_msg_t msg = {0};

    msg.msg_type = cmd;
    msg.data = data;
    msg.data_len = data_len;

    if (RT_EOK != rt_mq_send(g_gw_mq, &msg, sizeof(gw_app_msg_t)))
    {
        rt_kprintf("send queue fail\n");
        return -RT_ERROR;
    }

    return RT_EOK;
}

static void gateway_scan_freq_init(void)
{
    rt_memset(&g_sync_info, 0, sizeof(sync_info_t));
    rt_list_init(&g_sync_info.freq_info_list);
}

static void gateway_send_scan_freq_msg(void)
{
    gateway_send_msg(MSG_TYPE_SCAN_FREQ, RT_NULL, 0);
}

int gateway_app_send_data(user_ul_data_t *ul_data)
{
    if (ul_data == RT_NULL || ul_data->data_len == 0 || ul_data->data == RT_NULL)
    {
        rt_kprintf("para invalid\n");
        return -RT_ERROR;
    }

    user_ul_data_t *user_data = rt_malloc(sizeof(user_ul_data_t));
    if (user_data == RT_NULL)
    {
        return -RT_ENOMEM;
    }
    unsigned char *data = rt_malloc(ul_data->data_len);
    if (data == RT_NULL)
    {
        rt_free(user_data);
        return -RT_ENOMEM;
    }

    user_data->data_len = ul_data->data_len;
    user_data->timeout = ul_data->timeout;
    user_data->callback = ul_data->callback;
    rt_memcpy(data, ul_data->data, ul_data->data_len);
    user_data->data = data;

    int res = gateway_send_msg(MSG_TYPE_SEND_DATA, user_data, 0);
    if (RT_EOK != res)
    {
        rt_free(user_data->data);
        rt_free(user_data);
    }

    return res;
}

static unsigned char gateway_get_freq_list(unsigned char *freq_list)
{
    unsigned char freq_num = 0;
    unsigned char temp_list[FREQ_LIST_NUM] = {0};

    if (freq_list == RT_NULL)
    {
        return 0;
    }

    uc_wiota_get_freq_list(temp_list);
    for (unsigned char i = 0; i < FREQ_LIST_NUM; i++)
    {
        if (temp_list[i] == INVALID_FREQ)
        {
            break;
        }
        if (temp_list[i] <= MAX_FREQ && temp_list[i] >= MIN_FREQ)
        {
            freq_list[i] = temp_list[i];
            freq_num++;
            rt_kprintf("freq_num %d, freq_list[%d] %d\n", freq_num, i, freq_list[i]);
        }
    }

    if (freq_num == 0)
    {
        freq_list[0] = DEFAULT_FREQ;
        freq_num = 1;
    }

    return freq_num;
}

static unsigned char gateway_get_subsystemid_list(unsigned int *subsystemid_list)
{
    unsigned char subsystemid_num = 0;

    uc_wiota_get_subsystemid_list(subsystemid_list);
    for (unsigned char i = 0; i < SUBSYSTEM_ID_NUM; i++)
    {
        if (subsystemid_list[i] == INVALID_SUBSYSTEM_ID1 ||
            subsystemid_list[i] == INVALID_SUBSYSTEM_ID2)
        {
            break;
        }
        subsystemid_num++;
        rt_kprintf("subsystemid_num %d, subsystemid_list[%d] 0x%x\n", subsystemid_num, i, subsystemid_list[i]);
    }

    if (subsystemid_num == 0)
    {
        subsystemid_list[0] = DEFAULT_SUBSYSTEM_ID;
        subsystemid_num = 1;
    }

    return subsystemid_num;
}

void gateway_set_sync_config(unsigned char freq_idx, unsigned int subsystemid)
{
    sub_system_config_t config = {0};

    uc_wiota_get_system_config(&config);
    config.freq_idx = freq_idx;
    config.subsystemid = subsystemid;
    uc_wiota_set_system_config(&config);

    rt_kprintf("-- -wiota_cfg----\n");
    rt_kprintf("id_len        %d\n", config.id_len);
    rt_kprintf("pp            %d\n", config.pp);
    rt_kprintf("symbol_length %d\n", config.symbol_length);
    rt_kprintf("dlul_ratio    %d\n", config.dlul_ratio);
    rt_kprintf("btvalue       %d\n", config.btvalue);
    rt_kprintf("group_number  %d\n", config.group_number);
    rt_kprintf("spectrum_idx  %d\n", config.spectrum_idx);
    rt_kprintf("old_subsys_v  %d\n", config.old_subsys_v);
    rt_kprintf("bitscb        %d\n", config.bitscb);
    rt_kprintf("subsystemid   0x%x\n", config.subsystemid);
    rt_kprintf("freq_idx      %d\n", config.freq_idx);
    rt_kprintf("-- -wiota_cfg----\n");
}

static int gateway_sync_to_ap(void)
{
    uc_wiota_connect_quick(1);
    if (uc_wiota_wait_sync(WIOTA_CONNECT_TIMEOUT, WIOTA_WAIT_CONNECT_FN))
    {
        uc_wiota_disconnect();
        return -RT_ERROR;
    }

    return RT_EOK;
}

static void gateway_clear_freq_info_list(void)
{
    rt_list_t *head_node = &g_sync_info.freq_info_list;
    rt_list_t *next_node = head_node->next;
    freq_info_t *freq_info = RT_NULL;

    while (next_node != head_node)
    {
        freq_info = rt_list_entry(next_node, freq_info_t, node);
        rt_free(freq_info);
        freq_info = RT_NULL;
        next_node = next_node->next;
    }
    gateway_scan_freq_init();
}

static freq_info_t *gateway_find_freq_info_node(unsigned int subsystemid, unsigned char freq_idx)
{
    freq_info_t *freq_info = RT_NULL;
    rt_list_t *head_node = &g_sync_info.freq_info_list;

    rt_list_for_each_entry(freq_info, head_node, node)
    {
        if (freq_info->subsystemid == subsystemid && freq_info->freq_idx == freq_idx)
        {
            return freq_info;
        }
    }

    return RT_NULL;
}

static void gateway_add_to_sync_info(uc_freq_scan_result_t *scan_res)
{
    unsigned char state = SYNC_TO_AP;

    if (scan_res->sub_sys_id == g_sync_info.last_freq_info.subsystemid &&
        scan_res->freq_idx == g_sync_info.last_freq_info.freq_idx)
    {
        state = g_sync_info.last_freq_info.state; // AUTH_FAIL or SYNC_LOST
    }

    freq_info_t *freq_info = gateway_find_freq_info_node(scan_res->sub_sys_id, scan_res->freq_idx);
    if (freq_info == RT_NULL)
    {
        freq_info_t *new_node = (freq_info_t *)rt_malloc(sizeof(freq_info_t));
        RT_ASSERT(new_node);

        new_node->freq_idx = scan_res->freq_idx;
        new_node->rssi = scan_res->rssi;
        new_node->snr = scan_res->snr;
        new_node->freq_idx = scan_res->freq_idx;
        new_node->state = state;
        new_node->subsystemid = scan_res->sub_sys_id;

        freq_info_t *temp_node;
        rt_list_t *head_node = &g_sync_info.freq_info_list;
        rt_list_for_each_entry(temp_node, head_node, node)
        {
            if (temp_node->rssi > new_node->rssi)
            {
                rt_list_insert_after(&temp_node->node, &new_node->node);
                return;
            }
        }

        rt_list_insert_before(&temp_node->node, &new_node->node);
    }
    else
    {
        freq_info->freq_idx = scan_res->freq_idx;
        freq_info->rssi = scan_res->rssi;
        freq_info->snr = scan_res->snr;
        freq_info->state = state;
        freq_info->subsystemid = scan_res->sub_sys_id;
    }
}

static void gateway_del_from_sync_info(uc_freq_scan_result_t *scan_res)
{
    freq_info_t *freq_info = gateway_find_freq_info_node(scan_res->sub_sys_id, scan_res->freq_idx);
    if (freq_info)
    {
        rt_list_remove(&freq_info->node);
        rt_free(freq_info);
        freq_info = RT_NULL;
    }
}

static void gateway_scan_freq(unsigned char *freq_list, unsigned char freq_num, unsigned int *subsystemid_list, unsigned char subsystemid_num)
{
    sub_system_config_t config = {0};
    uc_recv_back_t scan_result = {0};
    uc_freq_scan_result_t *freq_info = RT_NULL;

    uc_wiota_get_system_config(&config);

    for (unsigned char i = 0; i < subsystemid_num; i++)
    {
        config.subsystemid = subsystemid_list[i];
        uc_wiota_set_system_config(&config);

        uc_wiota_scan_freq(freq_list, freq_num, 0, WIOTA_SCAN_TIMEOUT, RT_NULL, &scan_result);
        if (scan_result.result == UC_OP_SUCC)
        {
            freq_info = (uc_freq_scan_result_t *)scan_result.data;

            for (unsigned char j = 0; j < scan_result.data_len / sizeof(uc_freq_scan_result_t); j++)
            {
                if (freq_info[j].is_synced == 1)
                {
                    // at least one
                    gateway_add_to_sync_info(freq_info);
                }
                else
                {
                    gateway_del_from_sync_info(freq_info);
                }
            }
            rt_free(scan_result.data);
        }
    }

    // not even one
}

static void gateway_handle_scan_freq_msg(void)
{
    unsigned char freq_num = 0;
    unsigned char freq_list[FREQ_LIST_NUM] = {0};
    unsigned char subsystemid_num = 0;
    unsigned int subsystemid_list[SUBSYSTEM_ID_NUM] = {0};

    freq_num = gateway_get_freq_list(freq_list);
    RT_ASSERT(freq_num >= MIN_FREQ_LIST_NUM);
    subsystemid_num = gateway_get_subsystemid_list(subsystemid_list);
    RT_ASSERT(subsystemid_num >= MIN_SUBSYSTEM_ID_NUM);

    // scan freq list
    gateway_clear_freq_info_list();
    gateway_scan_freq(freq_list, freq_num, subsystemid_list, subsystemid_num);

    gateway_send_msg(MSG_TYPE_SYNC_TO_AP, RT_NULL, 0);
}

static freq_info_t *gateway_choose_one_freq_info(void)
{
    freq_info_t *freq_info = RT_NULL;
    rt_list_t *head_node = &g_sync_info.freq_info_list;

    // 1.SYNC_TO_AP
    // 2.GW_ABNORMAL
    // 3.AUTH_FAIL

    rt_list_for_each_entry(freq_info, head_node, node)
    {
        if (freq_info->state == SYNC_TO_AP)
        {
            rt_memcpy(&g_sync_info.last_freq_info, freq_info, sizeof(freq_info_t));
            return freq_info;
        }
    }

    rt_list_for_each_entry(freq_info, head_node, node)
    {
        if (freq_info->state == GW_ABNORMAL)
        {
            rt_memcpy(&g_sync_info.last_freq_info, freq_info, sizeof(freq_info_t));
            return freq_info;
        }
    }

    rt_list_for_each_entry(freq_info, head_node, node)
    {
        if (freq_info->state == AUTH_FAIL)
        {
            rt_memcpy(&g_sync_info.last_freq_info, freq_info, sizeof(freq_info_t));
            return freq_info;
        }
    }

    return RT_NULL;
}

static int gateway_handle_sync_to_ap_msg(void)
{
    freq_info_t *freq_info = gateway_choose_one_freq_info();
    if (freq_info)
    {
        gateway_set_sync_config(freq_info->freq_idx, freq_info->subsystemid);
        return gateway_sync_to_ap();
    }
    else
    {
        unsigned char freq_num = 0;
        unsigned char freq_list[FREQ_LIST_NUM] = {0};
        unsigned char subsystemid_num = 0;
        unsigned int subsystemid_list[SUBSYSTEM_ID_NUM] = {0};

        freq_num = gateway_get_freq_list(freq_list);
        RT_ASSERT(freq_num >= MIN_FREQ_LIST_NUM);
        subsystemid_num = gateway_get_subsystemid_list(subsystemid_list);
        RT_ASSERT(subsystemid_num >= MIN_SUBSYSTEM_ID_NUM);

        rt_kprintf("lterate through, sync\n");
        for (int i = 0; i < subsystemid_num; i++)
        {
            for (int j = 0; j < freq_num; j++)
            {
                gateway_set_sync_config(freq_list[j], subsystemid_list[i]);
                if (0 == gateway_sync_to_ap())
                {
                    return RT_EOK;
                }
            }
        }
    }

    return -RT_ERROR;
}

static void gateway_user_data_cb(void *data, unsigned int len, unsigned char data_type)
{
    unsigned char *user_data = rt_malloc(len);
    if (user_data == RT_NULL)
    {
        rt_kprintf("recv data no memory\n");
        return;
    }
    rt_memcpy(user_data, data, len);

    if (RT_EOK != gateway_send_msg(MSG_TYPE_RECV_DATA, user_data, len))
    {
        rt_free(user_data);
    }

    return;
}

static void gateway_state_report_cb(unsigned char exception_type)
{
    const char exception_string[5][24] = {
        "GATEWAY_DEFAULT",
        "GATEWAY_NORMAL",
        "GATEWAY_FAILED",
        "GATEWAY_END",
        "GATEWAY_RECONNECT"};

    rt_kprintf("%s\n", exception_string[exception_type]);

    switch (exception_type) // uc_gateway_state_t
    {
    case GATEWAY_FAILED:
    case GATEWAY_RECONNECT:
        gateway_send_msg(MSG_TYPE_GW_ABNORMAL, RT_NULL, 0);
        break;

    default:
        break;
    }
}
static void gateway_mode_entry(void)
{
    switch (uc_wiota_gateway_start(g_user_cfg->gw_mode, g_user_cfg->auth_code, RT_NULL)) // uc_gateway_start_result
    {
    case UC_GATEWAY_OK:
        uc_wiota_gateway_register_user_recv_cb(gateway_user_data_cb, gateway_state_report_cb);
        rt_kprintf("gw_mode run\n");
        break;

    case UC_GATEWAY_AUTH_FAIL:
        g_sync_info.last_freq_info.state = AUTH_FAIL;
        uc_wiota_gateway_end();
        gateway_send_scan_freq_msg();
        rt_kprintf("auth_fail\n");
        break;

    case UC_GATEWAY_NO_CONNECT:
    case UC_GATEWAY_NO_KEY:
    case UC_CREATE_QUEUE_FAIL:
    case UC_CREATE_MISSTIMER_FAIL:
    case UC_CREATE_TASK_FAIL:
    case UC_GATEWAY_OTHER_FAIL:
        // not exist
        break;

    default:
        break;
    }
}

static void gateway_handle_recv_data_msg(void *data, int data_len)
{
    rt_kprintf("recv_data:%d, %s\n", data_len, data);
    // TODO: data handle
}

static void gateway_app_task_entry(void *para)
{
    gw_app_msg_t msg = {0};
    const char *process[] = {
        "SCAN_FREQ",
        "SYNC_TO_AP",
        "RECV_DATA",
        "SEND_DATA",
        "GW_ABNORMAL"};

    gateway_scan_freq_init();
    gateway_send_scan_freq_msg();

    while (1)
    {
        if (RT_EOK != rt_mq_recv(g_gw_mq, &msg, sizeof(gw_app_msg_t), RT_WAITING_FOREVER))
        {
            continue;
        }

        rt_kprintf("process %s\n", process[msg.msg_type]);
        switch (msg.msg_type)
        {
        case MSG_TYPE_SCAN_FREQ:
            gateway_handle_scan_freq_msg();
            break;

        case MSG_TYPE_SYNC_TO_AP:
            if (RT_EOK == gateway_handle_sync_to_ap_msg())
            {
                gateway_mode_entry();
            }
            else
            {
                rt_kprintf("sync to ap fail\n");
                gateway_send_scan_freq_msg();
            }
            break;

        case MSG_TYPE_RECV_DATA:
            gateway_handle_recv_data_msg(msg.data, msg.data_len);
            break;

        case MSG_TYPE_SEND_DATA: // data send from other threads,such as serial transparent transmission data or sensor-collected data,etc.
        {
            // called gateway_app_send_data
            user_ul_data_t *ul_data = (user_ul_data_t *)msg.data;
            int send_res = uc_wiota_gateway_send_data(ul_data->data, ul_data->data_len, ul_data->timeout);
            if (ul_data->callback)
            {
                ul_data->callback(send_res);
            }
            rt_free(ul_data->data);
            break;
        }

        case MSG_TYPE_GW_ABNORMAL:
            g_sync_info.last_freq_info.state = GW_ABNORMAL;
            uc_wiota_gateway_end();
            gateway_send_scan_freq_msg();
            break;

        default:
            rt_kprintf("msg_type %d err\n", msg.msg_type);
            break;
        }

        if (msg.data) // maybe is null
        {
            rt_free(msg.data);
        }
    }
}

void gateway_app_demo_init(void)
{
    // 1.Applicable to the user's static data area,initalize the user's static data, mainly some WIoTa parameter configurations,etc.
    // 2.set WIoTa configurations based on user static data.
    // 3.create a gateway mode APP thread and a thread message queue.
    //// 3.1 scan freq
    //// 3.2 sync to ap
    //// 3.3 entry gateway mode
    //// 3.4 data recv and send(data send is best handled in a separately created, as the sending process is blocking).
    //// 3.5 exception handling

    gateway_user_cfg_init();

    user_cfg_t *config = gateway_user_cfg_get();

    uc_wiota_log_switch(UC_LOG_UART, config->ps_log_switch);

    uc_wiota_init();
    uc_wiota_run();

    if (config->ex32k_enable)
    {
        uc_wiota_set_outer_32K(1);
    }
    uc_wiota_set_data_rate(UC_RATE_NORMAL, config->mcs);
    uc_wiota_set_max_power(config->max_power);
    uc_wiota_set_cur_power(config->default_power);

#define GATEWAY_APP_QUEUE_DEPTH 16
#define GATEWAY_APP_TASK_STACK_SIZE 1024
#define GATEWAY_APP_TASK_STACK_PRIORITY 5
#define GATEWAY_APP_TASK_TICK 3

    g_gw_mq = rt_mq_create("gw_mq", sizeof(gw_app_msg_t), GATEWAY_APP_QUEUE_DEPTH, RT_IPC_FLAG_PRIO);
    RT_ASSERT(g_gw_mq);

    rt_thread_t gw_app_task = rt_thread_create("gw_app",
                                               gateway_app_task_entry,
                                               RT_NULL,
                                               GATEWAY_APP_TASK_STACK_SIZE,
                                               GATEWAY_APP_TASK_STACK_PRIORITY,
                                               GATEWAY_APP_TASK_TICK);
    RT_ASSERT(gw_app_task);
    rt_thread_startup(gw_app_task);

    rt_kprintf("gateway_app_demo_init suc\n");
}