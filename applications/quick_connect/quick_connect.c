#include <rtthread.h>
#ifdef _QUICK_CONNECT_
#include <rtdevice.h>
#include <board.h>
#include <string.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#ifdef RT_USING_AT
#include <at.h>
#include "ati_prs.h"
#endif
#include "uc_string_lib.h"
#include "uc_adda.h"
#include "uc_uart.h"
#include "at_wiota.h"
#include "quick_connect.h"

static rt_mutex_t p_mutex_qc_state = NULL;
static rt_mutex_t p_mutex_qc_run = NULL;

void quick_connect_init(void)
{
    p_mutex_qc_state = rt_mutex_create("qs", RT_IPC_FLAG_FIFO);

    if (p_mutex_qc_state == NULL)
    {
        return;
    }
    p_mutex_qc_run = rt_mutex_create("qr", RT_IPC_FLAG_FIFO);

    if (p_mutex_qc_run == NULL)
    {
        return;
    }
}

static e_qc_state qc_wiota_state = QC_EXIT;

void set_qc_wiota_state(e_qc_state state)
{
    rt_mutex_take(p_mutex_qc_state, RT_WAITING_FOREVER);

    qc_wiota_state = state;

    rt_mutex_release(p_mutex_qc_state);
}

e_qc_state get_qc_wiota_state(void)
{
    return qc_wiota_state;
}

static unsigned char qc_auto_run = 0; //默认不启动自动运行

void set_qc_auto_run(void)
{
    rt_mutex_take(p_mutex_qc_run, RT_WAITING_FOREVER);

    qc_auto_run = 1;

    rt_mutex_release(p_mutex_qc_run);
}

void clr_qc_auto_run(void)
{
    rt_mutex_take(p_mutex_qc_run, RT_WAITING_FOREVER);

    qc_auto_run = 0;

    rt_mutex_release(p_mutex_qc_run);
}

unsigned char get_qc_auto_run(void)
{
    return qc_auto_run;
}

static uc_freq_scan_result_t best_freq = {0};

static void quick_clr_best_freq(void)
{
    best_freq.freq_idx = 0xff;
    best_freq.is_synced = 0;
    best_freq.rssi = -126;
    best_freq.snr = -126;
    best_freq.sub_sys_id = 0;
}

static void quick_set_best_freq(uc_freq_scan_result_t *result)
{
    best_freq.freq_idx = result->freq_idx;
    best_freq.is_synced = result->is_synced;
    best_freq.rssi = result->rssi;
    best_freq.snr = result->snr;
    best_freq.sub_sys_id = result->sub_sys_id;
}

//兼容异步freq类型
static unsigned short quick_get_best_freq(void)
{
    return (unsigned short)best_freq.freq_idx;
}

static signed char quick_get_best_rssi(void)
{
    return best_freq.rssi;
}

//symbol_length：帧配置，取值0,1,2,3代表128,256,512,1024
//dlul_ratio：帧配置，取值0,1代表1:1和1:2
//帧配置，取值0,1,2,3代表1,2,4,8个上行group数量，在symbol_length为0/1/2/3时，group_number最高限制为3/2/1/0
static s_qc_cfg qc_cfg[QC_MODE_MAX] =
    {
        //symbol,   dlul,   group,  msc,                up_pow,     down_pow    ,tips
        {0, 1, 1, UC_MCS_LEVEL_2, (22 + 20), (24 + 20)},
        {1, 0, 0, UC_MCS_LEVEL_3, (22 + 20), (24 + 20)},
        {1, 1, 1, UC_MCS_LEVEL_2, (22 + 20), (24 + 20)},
        {2, 0, 0, UC_MCS_LEVEL_0, (22 + 20), (24 + 20)},
        {3, 0, 0, UC_MCS_LEVEL_0, (22 + 20), (24 + 20)},

};

static e_qc_mode cur_mode = QC_MODE_MID_DIS_HIGH_RATE;

int wiota_quick_connect_start(unsigned short freq, e_qc_mode mode)
{
    int ret = -1;
    sub_system_config_t config;

    if (mode >= QC_MODE_MAX)
    {
        return ret;
    }
    //use this api check freq range,when in api mode
    if (uc_wiota_set_freq_info(freq) == FALSE)
    {
        return ret;
    }

    cur_mode = mode;

    clr_qc_auto_run();

    if (get_qc_wiota_state() != QC_EXIT)
    {
        uc_wiota_exit();
#ifdef RT_USING_AT
        at_wiota_set_state(AT_WIOTA_EXIT);
#endif

        set_qc_wiota_state(QC_EXIT);

        rt_thread_mdelay(100);
    }

    quick_clr_best_freq();

    //配置参数

    uc_wiota_get_system_config(&config);

    config.freq_idx = freq;

    config.symbol_length = qc_cfg[mode].symbol_len;

    config.dlul_ratio = qc_cfg[mode].dlul_ratio;

    config.group_number = qc_cfg[mode].group_num;

    config.ap_tx_power = qc_cfg[mode].down_pow - 20;

    uc_wiota_set_system_config(&config);

    uc_wiota_save_static_info();

    rt_kprintf("\nqs f=%d,m=%d,sl=%d,dr=%d,gn=%d,p=%d\n",
               freq, mode, config.symbol_length, config.dlul_ratio, config.group_number, config.ap_tx_power);

#ifdef RT_USING_AT
    at_server_printfln("+QCSTART:%d,%d,%d,%d,%d,%d,%d",
                       freq, config.symbol_length, config.dlul_ratio, config.group_number, qc_cfg[mode].up_pow - 20, config.ap_tx_power, qc_cfg[mode].mcs);
#endif

    rt_thread_mdelay(50);

    set_qc_auto_run();

    ret = 0;

    return ret;
}

int wiota_quick_connect_stop(void)
{
    clr_qc_auto_run();

    if (get_qc_wiota_state() != QC_EXIT)
    {
        uc_wiota_exit();
#ifdef RT_USING_AT
        at_wiota_set_state(AT_WIOTA_EXIT);
#endif

        set_qc_wiota_state(QC_EXIT);

        rt_thread_mdelay(100);
    }

    return 0;
}

static uc_op_result_e quick_freq_scan_op(void)
{
    uc_recv_back_t result;
    uc_freq_scan_result_t *freqlist = NULL;
    unsigned int freq_result_num = 0;
    unsigned int i = 0;
    sub_system_config_t config;
    unsigned char freq[1] = {0};

    uc_wiota_get_system_config(&config);

    result.result = UC_OP_FAIL;

    freq[0] = config.freq_idx;

    if (freq[0] > 200)
    {
        freq[0] = 100;
    }

    uc_wiota_scan_freq(freq, 1, 0, QC_SCAN_TIMEOUT, NULL, &result);

    if (result.result == UC_OP_SUCC)
    {
        freqlist = (uc_freq_scan_result_t *)result.data;

        freq_result_num = result.data_len / sizeof(uc_freq_scan_result_t);

        for (i = 0; i < freq_result_num; i++)
        {
            if (freqlist->is_synced == 1)
            {

                if ((freqlist->rssi >= quick_get_best_rssi()) && (freqlist->rssi != -126)) //比上一次能同步的信号更好
                {
                    quick_set_best_freq(freqlist);
                }
            }

#ifdef RT_USING_AT
            if (get_qc_auto_run() == 1)
            {
                at_server_printfln("+QCCONN:%d,%d,%d", freqlist->freq_idx, freqlist->rssi, freqlist->is_synced);
            }
#endif

            freqlist++;
        }

        rt_free(result.data);
    }

    return result.result;
}
//内部调用
static uc_op_result_e quick_scan(void)
{
    uc_op_result_e ret = UC_OP_FAIL;

    if (get_qc_wiota_state() != QC_EXIT)
    {
        uc_wiota_exit();
#ifdef RT_USING_AT
        at_wiota_set_state(AT_WIOTA_EXIT);
#endif

        set_qc_wiota_state(QC_EXIT);
    }

    uc_wiota_init();

    set_qc_wiota_state(QC_INIT);

    quick_clr_best_freq();

    uc_wiota_run();

#ifdef RT_USING_AT
    at_wiota_set_state(AT_WIOTA_RUN);
#endif

    set_qc_wiota_state(QC_RUN);

    ret = quick_freq_scan_op();

    uc_wiota_exit();

#ifdef RT_USING_AT
    at_wiota_set_state(AT_WIOTA_EXIT);
#endif

    set_qc_wiota_state(QC_EXIT);

    return ret;
}

static int quick_open_wiota(void)
{
    quick_scan();

    if (get_qc_wiota_state() != QC_EXIT)
    {
        uc_wiota_exit();
#ifdef RT_USING_AT
        at_wiota_set_state(AT_WIOTA_EXIT);
#endif

        set_qc_wiota_state(QC_EXIT);
    }

    rt_thread_mdelay(200);

    if (quick_get_best_freq() != 0xff)
    {

        uc_wiota_init();

        set_qc_wiota_state(QC_INIT);

        uc_wiota_set_freq_info(quick_get_best_freq());

        uc_wiota_run();

#ifdef RT_USING_AT
        uc_wiota_register_recv_data_callback(wiota_recv_callback, UC_CALLBACK_NORAMAL_MSG);
        uc_wiota_register_recv_data_callback(wiota_recv_callback, UC_CALLBACK_STATE_INFO);
        at_wiota_set_state(AT_WIOTA_RUN);
#endif

        set_qc_wiota_state(QC_RUN);

        uc_wiota_connect();
    }

    return 0;
}

static void quick_connect_task(void *argument)
{
    uc_wiota_status_e wiota_state = UC_STATUS_NULL;
    int connect_flag = 0;
    unsigned int cnt = 0;

    quick_connect_init();

    while (1)
    {
        cnt++;

        if (cnt % 10 == 0)
        {
            wiota_state = uc_wiota_get_state();

            if ((wiota_state != UC_STATUS_SYNC) && (get_qc_auto_run() == 1))
            {
                quick_open_wiota();
            }

            if (wiota_state == UC_STATUS_SYNC)
            {
                if (connect_flag == 0)
                {
                    connect_flag = 1;

                    if (get_qc_auto_run() == 1)
                    {

                        uc_wiota_set_cur_power(qc_cfg[cur_mode].up_pow - 20);

                        uc_wiota_set_data_rate(UC_RATE_NORMAL, qc_cfg[cur_mode].mcs);

#ifdef RT_USING_AT
                        at_server_printfln("+QCRESULT:%d,%d SUCC", best_freq.freq_idx, best_freq.rssi);
#endif
                    }
                }
            }
            else
            {
                connect_flag = 0;
            }

            cnt++; //防止重入
        }

        rt_thread_mdelay(100);
    }
}

int quick_connect_task_init(void)
{
    rt_thread_t tid = NULL;

    tid = rt_thread_create("qc",
                           quick_connect_task,
                           NULL,
                           1024,
                           RT_THREAD_PRIORITY_MAX - 1,
                           5);
    if (tid != NULL)
    {
        rt_thread_startup(tid);
    }
    return 0;
}

#endif