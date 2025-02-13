/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date                 Author                Notes
 * 20201-8-17     ucchip-wz          v0.00
 */
#include <rtthread.h>
#ifdef RT_USING_AT
#ifdef UC8288_MODULE
#include <rtdevice.h>
#include <board.h>
#include <string.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "at.h"
#include "ati_prs.h"
#include "uc_string_lib.h"
#include "uc_adda.h"
// #include "uc_boot_download.h"
#include "at_wiota.h"
#include "uc_uart.h"
#include "at_wiota_gpio_report.h"
#ifdef _QUICK_CONNECT_
#include "quick_connect.h"
#endif

const unsigned short symLen_mcs_byte[4][8] = {{7, 9, 52, 66, 80, 0, 0, 0},
                                              {7, 15, 22, 52, 108, 157, 192, 0},
                                              {7, 15, 31, 42, 73, 136, 255, 297},
                                              {7, 15, 31, 63, 108, 220, 451, 619}};

enum at_wiota_lpm
{
    AT_WIOTA_SLEEP = 0,
    AT_WIOTA_PAGING_RX,     // 1, enter paging mode(system is sleep)
    AT_WIOTA_GATING,        // 2
    AT_WIOTA_CLOCK,         // 3
    AT_WIOTA_FREQ_DIV,      // 4
    AT_WIOTA_VOL_MODE,      // 5
    AT_WIOTA_SYNC_PAGING,   // 6, sync paging, need sync ok, then enter
    AT_WIOTA_PAGING_TIMINT, // 7, sync paging, need sync ok, then enter
    AT_WIOTA_EX_WK,         // 8
    AT_WIOTA_PAGING_TX,     // 9
    AT_WIOTA_LPM_MAX,
};

enum at_wiota_log
{
    AT_LOG_CLOSE = 0,
    AT_LOG_OPEN,
    AT_LOG_UART0,
    AT_LOG_UART1,
    AT_LOG_SPI_CLOSE,
    AT_LOG_SPI_OPEN,
};

#define ADC_DEV_NAME "adc"
#define WIOTA_TRANS_END_STRING "EOF"

#define WIOTA_SCAN_FREQ_TIMEOUT 120000
#define WIOTA_SEND_TIMEOUT 60000
#define WIOTA_WAIT_DATA_TIMEOUT 10000
#define WIOTA_TRANS_AUTO_SEND 1000
#define WIOTA_SEND_DATA_MUX_LEN 1024
#define WIOTA_DATA_END 0x1A
#define WIOTA_TRANS_MAX_LEN 310
#define WIOTA_TRANS_END_STRING_MAX 8
#define WIOTA_TRANS_BUFF (WIOTA_TRANS_MAX_LEN + WIOTA_TRANS_END_STRING_MAX + CRC16_LEN + 1)

#define WIOTA_MUST_INIT(state)             \
    if (state != AT_WIOTA_INIT)            \
    {                                      \
        return AT_RESULT_REPETITIVE_FAILE; \
    }

#define WIOTA_MUST_RUN(state)              \
    if (state != AT_WIOTA_RUN)             \
    {                                      \
        return AT_RESULT_REPETITIVE_FAILE; \
    }

#define WIOTA_MUST_ALREADY_INIT(state)                   \
    if (state != AT_WIOTA_INIT && state != AT_WIOTA_RUN) \
    {                                                    \
        return AT_RESULT_REPETITIVE_FAILE;               \
    }

#define WIOTA_CHECK_AUTOMATIC_MANAGER()   \
    if (uc_wiota_get_auto_connect_flag()) \
        return AT_RESULT_REFUSED;

#ifdef WIOTA_TEST_LOOP
enum at_test_mode_data_type
{
    AT_TEST_MODE_RECVDATA = 0,
    AT_TEST_MODE_QUEUE_EXIT,
};

typedef struct at_test_queue_data
{
    enum at_test_mode_data_type type;
    void *data;
    void *paramenter;
} t_at_test_queue_data;

typedef struct at_test_statistical_data
{
    int type;
    int dev;

    int upcurrentrate;
    int upaverate;
    int upminirate;
    int upmaxrate;

    int downcurrentrate;
    int downavgrate;
    int downminirate;
    int downmaxrate;

    int send_fail;
    int recv_fail;
    int max_mcs;
    unsigned char mcs;
    char power;
    unsigned char rssi;
    char snr;
} t_at_test_statistical_data;

typedef struct at_test_data
{
    char type;
    char mode;
    short test_data_len;
    int time;
    int num;
    rt_timer_t test_mode_timer;
    rt_thread_t test_mode_task;
    // rt_thread_t test_data_task;
    rt_mq_t test_queue;
    rt_sem_t test_sem;
    // char tast_state;
    t_at_test_statistical_data statistical;
} at_test_data_t;

typedef enum
{
    AT_TEST_COMMAND_DEFAULT = 0,
    AT_TEST_COMMAND_UP_TEST,
    AT_TEST_COMMAND_DOWN_TEST,
    AT_TEST_COMMAND_LOOP_TEST,
    AT_TEST_COMMAND_DATA_MODE,
    AT_TEST_COMMAND_DATA_DOWN,
    AT_TEST_COMMAND_STOP,
} at_test_communication_command_e;

#define AT_TEST_COMMUNICATION_HEAD_LEN 9
// #define AT_TEST_COMMUNICATION_RESERVED_LEN 4
#define AT_TEST_COMMUNICATION_HEAD "testMode"
typedef struct at_test_communication
{
    char head[AT_TEST_COMMUNICATION_HEAD_LEN];
    char command;
    char timeout;
    char mcs_num;
    short test_len;
    short all_len;
} t_at_test_communication;

#define AT_TEST_COMMUNICATION_DATA_LEN 40
#define AT_TEST_TIMEROUT 200

#define AT_TEST_GET_RATE(TIME, NUM, LEN, CURRENT, AVER, MIN, MAX) \
    {                                                             \
        CURRENT = LEN * 1000 / TIME;                              \
        if (AVER == 0)                                            \
        {                                                         \
            AVER = CURRENT;                                       \
        }                                                         \
        else                                                      \
        {                                                         \
            AVER = (AVER * NUM + CURRENT) / (NUM + 1);            \
        }                                                         \
        if (MIN > CURRENT || MIN == 0)                            \
        {                                                         \
            MIN = CURRENT;                                        \
        }                                                         \
        if (MAX < CURRENT || MAX == 0)                            \
        {                                                         \
            MAX = CURRENT;                                        \
        }                                                         \
    }

#define AT_TEST_CALCUTLATE(RESULT, ALL, BASE)                \
    {                                                        \
        if (0 != ALL)                                        \
        {                                                    \
            float get_result = ((float)BASE) / ((float)ALL); \
            get_result = get_result * 100.0;                 \
            RESULT = (int)get_result;                        \
        }                                                    \
        else                                                 \
        {                                                    \
            RESULT = 0;                                      \
        }                                                    \
    }
#endif

// extern dtu_send_t g_dtu_send;
extern at_server_t at_get_server(void);
extern char *parse(char *b, char *f, ...);
extern void reset_8288(void);
extern void at_wiota_manager_suspend(void);

static at_wiota_state_e wiota_state = AT_WIOTA_DEFAULT;
#ifdef WIOTA_TEST_LOOP
static at_test_data_t g_test_data = {0};
#endif

void at_wiota_set_state(at_wiota_state_e state)
{
    wiota_state = state;
}

at_wiota_state_e at_wiota_get_state(void)
{
    return wiota_state;
}

static rt_err_t get_char_timeout(rt_tick_t timeout, char *chr)
{
    at_server_t at_server = at_get_server();
    return at_server->get_char(at_server, chr, timeout);
}

static at_result_t at_auto_connect_query(void)
{
    at_server_printfln("+AUTOCONNECT=%d", uc_wiota_get_auto_connect_flag());
    return AT_RESULT_OK;
}

static at_result_t at_auto_connect_setup(const char *args)
{
    int value = 0;

    args = parse((char *)(++args), "d", &value);

    if (!args || value < 0 || value > 1)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_auto_connect(value & 0x1);

    at_wiota_manager_suspend();

    if (uc_wiota_execute_state() == MODULE_STATE_RUN)
    {
        uc_wiota_exit();
#ifdef _QUICK_CONNECT_
        clr_qc_auto_run();
#endif
    }
    else
    {
        uc_wiota_save_static_info();
    }

    at_server_printfln("OK");

    reset_8288();

    return AT_RESULT_OK;
}

static at_result_t at_wiota_version_query(void)
{
    unsigned char version[16] = {0};
    unsigned char git_info[36] = {0};
    unsigned char time[36] = {0};
    unsigned int cce_version = 0;

    uc_wiota_get_version(version, git_info, time, &cce_version);

    at_server_printfln("+WIOTAVERSION:%s", version);
    at_server_printfln("+GITINFO:%s", git_info);
    at_server_printfln("+TIME:%s", time);
    at_server_printfln("+CCEVERSION:%x", cce_version);

    return AT_RESULT_OK;
}

static at_result_t at_freq_query(void)
{
    at_server_printfln("+WIOTAFREQ=%d", uc_wiota_get_freq_info());

    return AT_RESULT_OK;
}

static at_result_t at_freq_setup(const char *args)
{
    int freq = 0;

    WIOTA_CHECK_AUTOMATIC_MANAGER();

    WIOTA_MUST_ALREADY_INIT(wiota_state)

    args = parse((char *)(++args), "d", &freq);
    if (!args || freq > 255)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (uc_wiota_set_freq_info(freq))
    {
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_FAILE;
    }
}

static unsigned int nth_power(unsigned int num, unsigned int n)
{
    unsigned int s = 1;

    for (unsigned int i = 0; i < n; i++)
    {
        s *= num;
    }
    return s;
}

static unsigned int convert_string_to_int(unsigned int numLen, const unsigned char *pStart, unsigned int scale)
{
    unsigned char *temp = NULL;
    unsigned int len = 0;
    unsigned int nth = numLen;
    unsigned int numValue = 0;

    temp = (unsigned char *)rt_malloc(numLen);
    if (temp == NULL)
    {
        rt_kprintf("convert malloc f\n");
        return numValue;
    }

    for (len = 0; len < numLen; len++)
    {
        if (pStart[len] >= '0' && pStart[len] <= '9')
        {
            temp[len] = pStart[len] - '0';
        }
        else if (pStart[len] >= 'A' && pStart[len] <= 'F')
        {
            temp[len] = pStart[len] - 'A' + 10;
        }
        else if (pStart[len] >= 'a' && pStart[len] <= 'f')
        {
            temp[len] = pStart[len] - 'a' + 10;
        }
        else
        {
            rt_kprintf("err num 0x%x\n", pStart[len]);
            break;
        }
        numValue += nth_power(scale, nth - 1) * temp[len];
        nth--;
    }
    rt_free(temp);
    temp = NULL;
    return numValue;
}

static unsigned char convert_string_to_freq_array(unsigned char *string, uc_freq_scan_req_dyn_t *array, unsigned char mode, unsigned int freq_num)
{
    unsigned char *pStart = string;
    unsigned char *pEnd = string;
    unsigned int num = 0;
    unsigned int numLen = 0;
    unsigned char isRight = 0;

    while (*pStart != '\0')
    {
        while (*pEnd != '\0')
        {
            if (*pEnd == ',')
            {
                if (num < freq_num)
                {
                    array[num].freq_idx = (unsigned char)convert_string_to_int(numLen, pStart, 10);
                }
                else if (num < freq_num * 2)
                {
                    array[num - freq_num].sub_sys_id = convert_string_to_int(numLen, pStart, 16);
                }
                pEnd++;
                pStart = pEnd;
                numLen = 0;
                num++;
            }
            numLen++;
            pEnd++;
        }

        if (num < freq_num)
        {
            array[num].freq_idx = (unsigned char)convert_string_to_int(numLen, pStart, 10);
        }
        else if (num < freq_num * 2)
        {
            array[num - freq_num].sub_sys_id = convert_string_to_int(numLen, pStart, 16);
        }
        num++;
        pStart = pEnd;
    }

    if (mode)
    {
        if (num == freq_num * 2)
        {
            isRight = TRUE;
        }
    }
    else
    {
        if (num == freq_num)
        {
            isRight = TRUE;
        }
    }

    // rt_kprintf("convert %d %d %d %d\n", mode, freq_num, num, isRight);

    return isRight;
}

static at_result_t at_scan_freq_setup(const char *args)
{
    unsigned int freqNum = 0;
    unsigned int timeout = 0;
    unsigned char *freqString = RT_NULL;
    unsigned char *tempFreq = RT_NULL;
    uc_recv_back_t result;
    unsigned char *freqArry = NULL;
    unsigned int dataLen = 0;
    unsigned int strLen = 0;
    unsigned int mode = 0;
    uc_freq_scan_req_dyn_p freqScanReq_ptr = NULL;
    unsigned char isRight = FALSE;

    WIOTA_CHECK_AUTOMATIC_MANAGER();

    if (wiota_state != AT_WIOTA_RUN)
    {
        return AT_RESULT_REPETITIVE_FAILE;
    }

    args = parse((char *)(++args), "d,d,d,d", &timeout, &mode, &dataLen, &freqNum);
    if (!args || (0 != mode && 1 != mode))
    {
        return AT_RESULT_PARSE_FAILE;
    }
    // strLen = dataLen;

    if (freqNum > 0)
    {
        // calc max len, freq "200," is 4bytes, subsysid "abcd1234," is 9bytes
        // freqString = (unsigned char *)rt_malloc(freqNum * (4 + 9 * mode) + 1);
        freqString = (unsigned char *)rt_malloc(freqNum * (4 + 9 * 1) + 1);
        if (RT_NULL == freqString)
        {
            at_server_printfln("MEM FAIL");
            return AT_RESULT_NULL;
        }
        tempFreq = freqString;
        at_server_printfln("OK");
        at_server_printf(">");
        while (1)
        {
            if (get_char_timeout(rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT), (char *)tempFreq) != RT_EOK)
            {
                rt_free(freqString);
                at_server_printfln("GET FAIL");
                return AT_RESULT_NULL;
            }
            // dataLen--;
            strLen++;
            if (strLen > 1 && *tempFreq == '\n' && *(tempFreq - 1) == '\r')
            {
                break;
            }
            tempFreq++;
        }

        freqScanReq_ptr = rt_malloc(freqNum * sizeof(uc_freq_scan_req_dyn_t));
        if (freqScanReq_ptr == NULL)
        {
            rt_free(freqString);
            at_server_printfln("MEM FAIL");
            return AT_RESULT_NULL;
        }
        rt_memset(freqScanReq_ptr, 0, freqNum * sizeof(uc_freq_scan_req_dyn_t));

        freqString[strLen - 2] = '\0'; // add string end flag

        isRight = convert_string_to_freq_array(freqString, freqScanReq_ptr, (unsigned char)mode, freqNum);
        if (!isRight)
        {
            rt_free(freqString);
            rt_free(freqScanReq_ptr);
            at_server_printfln("CONVERT FAIL");
            return AT_RESULT_NULL;
        }
        rt_free(freqString);

        if (0 == mode)
        {
            freqArry = (unsigned char *)rt_malloc(freqNum * sizeof(unsigned char));
            if (freqArry == NULL)
            {
                rt_free(freqScanReq_ptr);
                at_server_printfln("MEM FAIL");
                return AT_RESULT_NULL;
            }

            for (unsigned int i = 0; i < freqNum; i++)
            {
                freqArry[i] = freqScanReq_ptr[i].freq_idx;
            }
            rt_free(freqScanReq_ptr);
            uc_wiota_scan_freq((unsigned char *)freqArry, freqNum * sizeof(uc_freq_scan_req_t),
                               (unsigned char)mode, timeout, RT_NULL, &result);
            rt_free(freqArry);
        }
        else
        {
            uc_wiota_scan_freq((unsigned char *)freqScanReq_ptr, freqNum * sizeof(uc_freq_scan_req_dyn_t),
                               (unsigned char)mode, timeout, RT_NULL, &result);
            rt_free(freqScanReq_ptr);
        }
    }
    else
    {
        // uc_wiota_scan_freq(RT_NULL, 0, WIOTA_SCAN_FREQ_TIMEOUT, RT_NULL, &result);
        uc_wiota_scan_freq(RT_NULL, 0, 0, 0, RT_NULL, &result); // scan all wait for ever
    }

    if (UC_OP_SUCC == result.result)
    {
        uc_freq_scan_result_p freqlinst = (uc_freq_scan_result_p)result.data;
        int freq_num = result.data_len / sizeof(uc_freq_scan_result_t);

        at_server_printfln("+WIOTASCANFREQ:");

        for (int i = 0; i < freq_num; i++)
        {
            at_server_printfln("%d,%d,%d,%d,0x%x", freqlinst->freq_idx, freqlinst->rssi, freqlinst->snr, freqlinst->is_synced, freqlinst->sub_sys_id);
            freqlinst++;
        }

        rt_free(result.data);
        return AT_RESULT_OK;
    }
    else if (UC_OP_TIMEOUT == result.result)
    {
        at_server_printfln("SCAN TOUT");
    }

    return AT_RESULT_NULL;
}

static at_result_t at_dcxo_query(void)
{
    at_server_printfln("+WIOTADCXO=0x%x", uc_wiota_get_dcxo());

    return AT_RESULT_OK;
}

static at_result_t at_dcxo_setup(const char *args)
{
    int dcxo = 0;

    WIOTA_CHECK_AUTOMATIC_MANAGER();

    WIOTA_MUST_INIT(wiota_state)

    args = parse((char *)(++args), "y", &dcxo);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    // rt_kprintf("dcxo=0x%x\n", dcxo);

    if (uc_wiota_set_dcxo(dcxo))
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_PARSE_FAILE;
}

static at_result_t at_userid_query(void)
{
    unsigned int id[2] = {0};
    unsigned char len = 0;

    uc_wiota_get_userid(&(id[0]), &len);
    at_server_printfln("+WIOTAUSERID=0x%x", id[0]);

    return AT_RESULT_OK;
}
static at_result_t at_dev_addr_setup(const char *args)
{
    unsigned int dev_addr;

    args = parse((char *)(++args), "y", &dev_addr);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_dev_addr(dev_addr);

    return AT_RESULT_OK;
}

static at_result_t at_dev_addr_query(void)
{
    unsigned char serial[16] = {0};
    unsigned int *dev_addr = (unsigned int *)(&serial[4]);

    uc_wiota_get_dev_serial(serial);

    at_server_printfln("+WIOTADEVADDRESS=0x%x", *dev_addr);

    return AT_RESULT_OK;
}

static at_result_t at_userid_setup(const char *args)
{
    unsigned int userid[2] = {0};

    WIOTA_CHECK_AUTOMATIC_MANAGER();

    WIOTA_MUST_ALREADY_INIT(wiota_state)

    args = parse((char *)(++args), "y", &userid[0]);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    // rt_kprintf("userid:%x\n", userid[0]);

    if (uc_wiota_set_userid(userid, 4))
    {
        return AT_RESULT_PARSE_FAILE;
    }

    return AT_RESULT_OK;
}

static rt_uint32_t at_temp_query(void)
{
    rt_device_t adc_dev;

    adc_dev = rt_device_find(ADC_DEV_NAME);
    if (RT_NULL == adc_dev)
    {
        rt_kprintf("ad find %s fail\n", ADC_DEV_NAME);
        return 0;
    }

    rt_adc_enable((rt_adc_device_t)adc_dev, ADC_CONFIG_CHANNEL_CHIP_TEMP);
    return rt_adc_read((rt_adc_device_t)adc_dev, ADC_CONFIG_CHANNEL_CHIP_TEMP);
}

static at_result_t at_radio_query(void)
{
    rt_uint32_t temp = 0;
    radio_info_t radio;

    if (AT_WIOTA_RUN != wiota_state)
    {
        rt_kprintf("wiota state err %d\n", wiota_state);
        return AT_RESULT_FAILE;
    }

    temp = at_temp_query();

    uc_wiota_get_radio_info(&radio);
    // temp,rssi,ber,snr,cur_power,max_pow,cur_mcs,max_mcs
    at_server_printfln("+WIOTARADIO=%d,-%d,%d,%d,%d,%d,%d,%d,%d,%d",
                       temp, radio.rssi, radio.ramp_type, radio.snr, radio.cur_power,
                       radio.min_power, radio.max_power, radio.cur_mcs, radio.max_mcs, radio.frac_offset);

    return AT_RESULT_OK;
}

static at_result_t at_system_config_query(void)
{
    sub_system_config_t config;
    uc_wiota_get_system_config(&config);

    at_server_printfln("+WIOTACONFIG=%d,%d,%d,%d,%d,%d,%d,%d,%d,0x%x",
                       config.ap_tx_power, config.id_len, config.symbol_length, config.dlul_ratio,
                       config.btvalue, config.group_number, config.spectrum_idx, config.old_subsys_v, config.bitscb,
                       config.subsystemid);

    return AT_RESULT_OK;
}

static at_result_t at_system_config_setup(const char *args)
{
    sub_system_config_t config = {0};
    unsigned int temp[9];

    WIOTA_CHECK_AUTOMATIC_MANAGER();

    WIOTA_MUST_ALREADY_INIT(wiota_state)

    uc_wiota_get_system_config(&config);

    args = parse((char *)(++args), "d,d,d,d,d,d,d,d,d,y",
                 &temp[0], &temp[1], &temp[2],
                 &temp[3], &temp[4], &temp[5],
                 &temp[6], &temp[7], &temp[8],
                 &config.subsystemid);

    config.ap_tx_power = (char)(temp[0] - 20);
    config.id_len = (unsigned char)temp[1];
    config.symbol_length = (unsigned char)temp[2];
    config.dlul_ratio = (unsigned char)temp[3];
    config.btvalue = (unsigned char)temp[4];
    config.group_number = (unsigned char)temp[5];
    config.spectrum_idx = (unsigned char)temp[6];
    config.old_subsys_v = (unsigned char)temp[7];
    config.bitscb = (unsigned char)temp[8];

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    // default config
    config.pp = 1;

    // rt_kprintf("id_len=%d,symbol_len=%d,dlul=%d,bt=%d,group_num=%d,ap_tx_power=%d,spec_idx=%d,systemid=0x%x,subsystemid=0x%x\n",
    //            config.id_len, config.symbol_length, config.dlul_ratio,
    //            config.btvalue, config.group_number, config.ap_tx_power,
    //            config.spectrum_idx, config.systemid, config.subsystemid);

    if (uc_wiota_set_system_config(&config))
    {
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_FAILE;
    }
}

static at_result_t at_wiota_init_exec(void)
{
    WIOTA_CHECK_AUTOMATIC_MANAGER();

    if (wiota_state == AT_WIOTA_DEFAULT || wiota_state == AT_WIOTA_EXIT)
    {
        uc_wiota_init();
        wiota_state = AT_WIOTA_INIT;
        return AT_RESULT_OK;
    }

    return AT_RESULT_REPETITIVE_FAILE;
}
#ifdef WIOTA_TEST_LOOP
static unsigned char at_test_mode_wiota_recv_fun(uc_recv_back_p recv_data)
{
    unsigned int send_data_address = 0;
    t_at_test_queue_data *queue_data = RT_NULL;
    uc_recv_back_p copy_recv_data = RT_NULL;
    rt_err_t re;

    t_at_test_communication *test_mode_data = (t_at_test_communication *)(recv_data->data);
    if (g_test_data.type != AT_TEST_COMMAND_DEFAULT && g_test_data.type != AT_TEST_COMMAND_LOOP_TEST)
    {
        if (0 == recv_data->result)
            rt_free(recv_data->data);
        return 1;
    }

    if (!(recv_data->data_len >= sizeof(t_at_test_communication) &&
          0 == strcmp(test_mode_data->head, AT_TEST_COMMUNICATION_HEAD)))
        return 0;

    if (!g_test_data.time)
    {
        if (0 == recv_data->result)
            rt_free(recv_data->data);
        return 1;
    }

    copy_recv_data = rt_malloc(sizeof(uc_recv_back_t));
    queue_data = rt_malloc(sizeof(t_at_test_queue_data));

    rt_memcpy(copy_recv_data, recv_data, sizeof(uc_recv_back_t));

    queue_data->type = AT_TEST_MODE_RECVDATA;
    queue_data->data = copy_recv_data;

    send_data_address = (unsigned int)queue_data;

    re = rt_mq_send(g_test_data.test_queue, &send_data_address, 4);
    // at_send_queue(g_test_data.test_queue, data,  2000);
    if (RT_EOK != re)
    {
        rt_kprintf("rt_mq_send error %d\n", re);
        rt_free(copy_recv_data);
        rt_free(queue_data);

        if (0 == recv_data->result)
            rt_free(recv_data->data);
    }

    return 1;
}
#endif
void wiota_recv_callback(uc_recv_back_p data)
{
    unsigned int fn = 0;
    // rt_kprintf("wiota_recv_callback result %d\n", data->result);

    if (UC_OP_SUCC != data->result)
    {
        return;
    }
    if (WIOTA_MODE_OUT_UART == wiota_gpio_mode_get())
    {
#ifdef WIOTA_TEST_LOOP
        if (/*g_test_data.time > 0 && */ at_test_mode_wiota_recv_fun(data))
        {
            return;
        }
#endif

        // if (g_dtu_send->flag && (!g_dtu_send->at_show))
        // {
        //     at_send_data(data->data, data->data_len);
        //     rt_free(data->data);
        //     return;
        // }

        if (data->type < UC_RECV_SCAN_FREQ)
        {
#ifdef WIOTA_TEST_LOOP
            if (AT_TEST_COMMAND_DATA_DOWN == g_test_data.type)
            {
                rt_free(data->data);
                return;
            }
#endif
            at_server_printf("+WIOTARECV,%d,%d,", data->type, data->data_len);
            at_send_data(data->data, data->data_len);
            at_server_printfln("");
        }
        else
        {
            switch (data->type)
            {
            case UC_RECV_SYNC_LOST:
                at_server_printfln("+WIOTASYNC,LOST");
                break;
            case UC_RECV_IDLE_PAGING:
                at_server_printfln("+WIOTAPGIDLE,RECV");
                break;
            case UC_RECV_PG_TX_DONE:
                at_server_printfln("+WIOTAPGTX,DONE");
                break;
            case UC_RECV_FN_CHANGE:
                if (uc_wiota_get_is_report_fn())
                {
                    rt_memcpy(&fn, data->data, 4);
                    at_server_printfln("+WIOTAFN,%d", fn);
                }
                break;
            case UC_RECV_PHY_ERROR:
                at_server_printfln("+WIOTAPHY,ERROR");
                break;

            default:
                break;
            }
        }

        rt_free(data->data);

    } // ~if (WIOTA_MODE_OUT_UART == wiota_gpio_mode_get())
    else
    {
        wiota_data_insert(data);
    }
}

static at_result_t at_wiota_cfun_setup(const char *args)
{
    int state = 0;

    WIOTA_CHECK_AUTOMATIC_MANAGER();

    args = parse((char *)(++args), "d", &state);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    // rt_kprintf("state = %d\n", state);

    if (1 == state && wiota_state == AT_WIOTA_INIT)
    {
        uc_wiota_run();
        uc_wiota_register_recv_data_callback(wiota_recv_callback, UC_CALLBACK_NORAMAL_MSG);
        uc_wiota_register_recv_data_callback(wiota_recv_callback, UC_CALLBACK_STATE_INFO);
        wiota_state = AT_WIOTA_RUN;
    }
    else if (0 == state && wiota_state == AT_WIOTA_RUN)
    {
        uc_wiota_exit();
#ifdef _QUICK_CONNECT_
        clr_qc_auto_run();
#endif

        wiota_state = AT_WIOTA_EXIT;
    }
    else
        return AT_RESULT_REPETITIVE_FAILE;

    return AT_RESULT_OK;
}

static at_result_t at_connect_query(void)
{
    at_server_printfln("+WIOTACONNECT=%d,%d", uc_wiota_get_state(), uc_wiota_get_active_time());

    return AT_RESULT_OK;
}

static at_result_t at_connect_setup(const char *args)
{
    int state = 0, timeout = 0;

    WIOTA_CHECK_AUTOMATIC_MANAGER();

    args = parse((char *)(++args), "d,d", &state, &timeout);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    // rt_kprintf("state = %d, timeout=%d\n", state, timeout);

    if (wiota_state != AT_WIOTA_RUN)
        return AT_RESULT_REPETITIVE_FAILE;

    // rt_kprintf("state = %d, timeout=%d\n", state, timeout);

    if (timeout)
        uc_wiota_set_active_time((unsigned int)timeout);

    if (1 == state)
    {
        uc_wiota_connect();
    }
    else if (2 == state)
    {
#ifdef _LPM_PAGING_
        uc_wiota_connect_quick(0);
#endif
    }
    else if (3 == state)
    {
#ifdef _LPM_PAGING_
        uc_wiota_connect_quick(1);
#endif
    }
    else
    {
        uc_wiota_disconnect();
    }

    return AT_RESULT_OK;
}

at_result_t at_wiotasend_handle_data(int timeout, int length, unsigned char is_wait, uc_send callback, unsigned int fn)
{
    unsigned char *sendbuffer = NULL;
    unsigned char *psendbuffer;
    int send_result = 0;

    if (is_wait)
    {
        timeout = WIOTA_SEND_TIMEOUT;
        length = WIOTA_SEND_DATA_MUX_LEN;
    }

    sendbuffer = (unsigned char *)rt_malloc(length + CRC16_LEN); // reserve CRC16_LEN for low mac
    if (sendbuffer == NULL || length < 2)
    {
        rt_free(sendbuffer);
        at_server_printfln("SEND FAIL");
        return AT_RESULT_NULL;
    }
    psendbuffer = sendbuffer;
    // at_server_printfln("SUCC");
    at_server_printf(">");

    while (length)
    {
        if (get_char_timeout(rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT), (char *)psendbuffer) != RT_EOK)
        {
            at_server_printfln("SEND FAIL");
            rt_free(sendbuffer);
            return AT_RESULT_NULL;
        }
        length--;
        if (is_wait && WIOTA_DATA_END == *psendbuffer)
        {
            break;
        }
        psendbuffer++;
    }
    if ((psendbuffer - sendbuffer) > 0)
    {
        if (0 == fn)
        {
            send_result = uc_wiota_send_data(sendbuffer, psendbuffer - sendbuffer,
                                             timeout > 0 ? timeout : WIOTA_SEND_TIMEOUT, callback);
        }
        else
        {
            send_result = uc_wiota_send_data_by_fn(sendbuffer, psendbuffer - sendbuffer,
                                                   timeout > 0 ? timeout : WIOTA_SEND_TIMEOUT, fn);
        }
        if (NULL == callback)
        {
            rt_free(sendbuffer);
            sendbuffer = NULL;
            if (UC_OP_SUCC == send_result)
            {
                at_server_printfln("SEND SUCC");
                return AT_RESULT_OK;
            }
            else if (UC_OP_TIMEOUT == send_result)
            {
                at_server_printfln("SEND TOUT");
                return AT_RESULT_NULL;
            }
            else
            {
                at_server_printfln("SEND FAIL");
                return AT_RESULT_NULL;
            }
        }
        else
        {
            return AT_RESULT_OK;
        }
    }
    else
    {
        rt_free(sendbuffer);
        at_server_printfln("SEND FAIL");
        return AT_RESULT_NULL;
    }
}

static at_result_t at_wiotasend_exec(void)
{
    if (AT_WIOTA_RUN != wiota_state)
    {
        return AT_RESULT_FAILE;
    }

    return at_wiotasend_handle_data(0, 0, TRUE, RT_NULL, 0);
}

static at_result_t at_wiotasend_setup(const char *args)
{
    int length = 0, timeout = 0;

    if (AT_WIOTA_RUN != wiota_state)
    {
        return AT_RESULT_FAILE;
    }

    args = parse((char *)(++args), "d,d", &timeout, &length);
    if (!args || length <= 0)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    return at_wiotasend_handle_data(timeout, length, FALSE, RT_NULL, 0);
}

void at_send_callback(uc_send_back_p sendResult)
{
    if (UC_OP_SUCC == sendResult->result)
    {
        rt_free(sendResult->oriPtr);
        at_server_printfln("SEND SUCC");
    }
    else if (UC_OP_TIMEOUT == sendResult->result)
    {
        at_server_printfln("SEND TOUT");
    }
    else
    {
        rt_free(sendResult->oriPtr);
        at_server_printfln("SEND FAIL");
    }
}

static at_result_t at_wiotasend_noblock_exec(void)
{
    if (AT_WIOTA_RUN != wiota_state)
    {
        return AT_RESULT_FAILE;
    }

    return at_wiotasend_handle_data(0, 0, TRUE, at_send_callback, 0);
}

static at_result_t at_wiotasend_noblock_setup(const char *args)
{
    int length = 0, timeout = 0;

    if (AT_WIOTA_RUN != wiota_state)
    {
        return AT_RESULT_FAILE;
    }

    args = parse((char *)(++args), "d,d", &timeout, &length);
    if (!args || length <= 0)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    return at_wiotasend_handle_data(timeout, length, FALSE, at_send_callback, 0);
}

static at_result_t at_wiotasend_by_fn_setup(const char *args)
{
    int length = 0, timeout = 0;
    unsigned int fn = 0;

    if (AT_WIOTA_RUN != wiota_state)
    {
        return AT_RESULT_FAILE;
    }

    args = parse((char *)(++args), "d,d,d", &timeout, &length, &fn);
    rt_kprintf("send by fn %u\n", fn);
    if (!args || length <= 0)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    return at_wiotasend_handle_data(timeout, length, FALSE, RT_NULL, fn);
}

static at_result_t at_wiotatrans_process(unsigned short timeout, char *strEnd)
{
    unsigned char *pBuff = RT_NULL;
    int result = 0;
    int send_result = 0;
    timeout = (timeout == 0) ? WIOTA_SEND_TIMEOUT : timeout;
    if ((RT_NULL == strEnd) || ('\0' == strEnd[0]))
    {
        strEnd = WIOTA_TRANS_END_STRING;
    }
    unsigned char nLenEnd = strlen(strEnd);
    //    unsigned char nStrEndCount = 0;
    int16_t nSeekRx = 0;
    char nRun = 1;
    //    char nCatchEnd = 0;
    char nSendFlag = 0;

    pBuff = (unsigned char *)rt_malloc(WIOTA_TRANS_BUFF);
    if (pBuff == RT_NULL)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    rt_memset(pBuff, 0, WIOTA_TRANS_BUFF);
    at_server_printfln("\r\nEnter transmission mode >");

    while (nRun)
    {
        get_char_timeout(rt_tick_from_millisecond(-1), (char *)&pBuff[nSeekRx]);
        ++nSeekRx;
        if ((nSeekRx > 2) && ('\n' == pBuff[nSeekRx - 1]) && ('\r' == pBuff[nSeekRx - 2]))
        {
            nSendFlag = 1;
            nSeekRx -= 2;
            if ((nSeekRx >= nLenEnd) && pBuff[nSeekRx - 1] == strEnd[nLenEnd - 1])
            {
                int i = 0;
                for (i = 0; i < nLenEnd; ++i)
                {
                    if (pBuff[nSeekRx - nLenEnd + i] != strEnd[i])
                    {
                        break;
                    }
                }
                if (i >= nLenEnd)
                {
                    nSeekRx -= nLenEnd;
                    nRun = 0;
                }
            }
        }

        if ((nSeekRx > (WIOTA_TRANS_MAX_LEN + nLenEnd + 2)) || (nSendFlag && (nSeekRx > WIOTA_TRANS_MAX_LEN)))
        {
            at_server_printfln("\r\nThe message's length can not over 310 characters.");
            do
            {
                // discard any characters after the end string
                result = get_char_timeout(rt_tick_from_millisecond(200), (char *)&pBuff[0]);
            } while (RT_EOK == result);
            nSendFlag = 0;
            nSeekRx = 0;
            nRun = 1;
            rt_memset(pBuff, 0, WIOTA_TRANS_BUFF);
            continue;
        }

        if (nSendFlag)
        {
            nSeekRx = (nSeekRx > WIOTA_TRANS_MAX_LEN) ? WIOTA_TRANS_MAX_LEN : nSeekRx;
            if (nSeekRx > 0)
            {
                send_result = uc_wiota_send_data(pBuff, nSeekRx, timeout, RT_NULL);
                if (UC_OP_SUCC == send_result)
                {
                    at_server_printfln("SEND SUCC");
                }
                else if (UC_OP_TIMEOUT == send_result)
                {
                    at_server_printfln("SEND TOUT");
                }
                else
                {
                    at_server_printfln("SEND FAIL");
                }
            }
            nSeekRx = 0;
            nSendFlag = 0;
            rt_memset(pBuff, 0, WIOTA_TRANS_BUFF);
        }
    }

    do
    {
        // discard any characters after the end string
        result = get_char_timeout(rt_tick_from_millisecond(200), (char *)&pBuff[0]);
    } while (RT_EOK == result);

    at_server_printfln("\r\nLeave transmission mode");
    if (RT_NULL != pBuff)
    {
        rt_free(pBuff);
        pBuff = RT_NULL;
    }
    return AT_RESULT_OK;
}

static at_result_t at_wiotatrans_setup(const char *args)
{
    if (AT_WIOTA_RUN != wiota_state)
    {
        return AT_RESULT_FAILE;
    }
    int timeout = 0;
    char strEnd[WIOTA_TRANS_END_STRING_MAX + 1] = {0};

    args = parse((char *)(++args), "d,s", &timeout, (signed long)8, strEnd);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    int i = 0;
    for (i = 0; i < WIOTA_TRANS_END_STRING_MAX; i++)
    {
        if (('\r' == strEnd[i]) || ('\n' == strEnd[i]))
        {
            strEnd[i] = '\0';
            break;
        }
    }

    if (i <= 0)
    {
        strcpy(strEnd, WIOTA_TRANS_END_STRING);
    }
    return at_wiotatrans_process(timeout & 0xFFFF, strEnd);
}

static at_result_t at_wiotatrans_exec(void)
{
    if (AT_WIOTA_RUN != wiota_state)
    {
        return AT_RESULT_FAILE;
    }
    return at_wiotatrans_process(0, RT_NULL);
}
#if 0
void dtu_send_process(void)
{
    int16_t nSeekRx = 0;
    unsigned char buff[WIOTA_TRANS_BUFF] = {0};
    rt_err_t result;

    int i = 0;
    for (i = 0; i < WIOTA_TRANS_END_STRING_MAX; i++)
    {
        if (('\0' == g_dtu_send->exit_flag[i]) ||
            ('\r' == g_dtu_send->exit_flag[i]) ||
            ('\n' == g_dtu_send->exit_flag[i]))
        {
            g_dtu_send->exit_flag[i] = '\0';
            break;
        }
    }
    g_dtu_send->flag_len = i;
    if (g_dtu_send->flag_len <= 0)
    {
        strcpy(g_dtu_send->exit_flag, WIOTA_TRANS_END_STRING);
        g_dtu_send->flag_len = strlen(WIOTA_TRANS_END_STRING);
    }
    g_dtu_send->timeout = g_dtu_send->timeout ? g_dtu_send->timeout : 5000;
    g_dtu_send->wait = g_dtu_send->wait ? g_dtu_send->wait : 200;

    while (g_dtu_send->flag)
    {
        result = get_char_timeout(rt_tick_from_millisecond(g_dtu_send->wait), (char *)&buff[nSeekRx]);
        if (RT_EOK == result)
        {
            nSeekRx++;
            if ((nSeekRx >= g_dtu_send->flag_len) && (buff[nSeekRx - 1] == g_dtu_send->exit_flag[g_dtu_send->flag_len - 1]))
            {

                int i = 0;
                for (i = 0; i < g_dtu_send->flag_len; ++i)
                {
                    if (buff[nSeekRx - g_dtu_send->flag_len + i] != g_dtu_send->exit_flag[i])
                    {
                        break;
                    }
                }
                if (i >= g_dtu_send->flag_len)
                {
                    nSeekRx -= g_dtu_send->flag_len;
                    g_dtu_send->flag = 0;
                }
            }
            if (g_dtu_send->flag && (nSeekRx > (WIOTA_TRANS_MAX_LEN + g_dtu_send->flag_len)))
            {
                // too long to send
                result = RT_ETIMEOUT;
            }
        }
        if ((nSeekRx > 0) && ((RT_EOK != result) || (0 == g_dtu_send->flag)))
        {
            // timeout to send
            if (nSeekRx > WIOTA_TRANS_MAX_LEN)
            {
                nSeekRx = WIOTA_TRANS_MAX_LEN;
                do
                {
                    // discard any characters after the end string
                    char ch;
                    result = get_char_timeout(rt_tick_from_millisecond(100), &ch);
                } while (RT_EOK == result);
            }

            if ((AT_WIOTA_RUN == wiota_state) &&
                (UC_OP_SUCC == uc_wiota_send_data(buff, nSeekRx, g_dtu_send->timeout, RT_NULL)))
            {
                if (g_dtu_send->at_show)
                {
                    at_server_printfln("SEND:%4d.", nSeekRx);
                }
            }
            else
            {
                at_server_printfln("SEND FAIL");
            }
            nSeekRx = 0;
            rt_memset(buff, 0, WIOTA_TRANS_BUFF);
        }
    }
    do
    {
        // discard any characters after the end string
        result = get_char_timeout(rt_tick_from_millisecond(100), (char *)&buff[0]);
    } while (RT_EOK == result);
    if (0 == g_dtu_send->flag)
    {
        at_server_printfln("OK");
    }
}

static at_result_t at_wiota_dtu_send_setup(const char *args)
{
    if ((AT_WIOTA_RUN != wiota_state) || (RT_NULL == g_dtu_send))
    {
        return AT_RESULT_FAILE;
    }
    rt_memset(g_dtu_send->exit_flag, 0, WIOTA_TRANS_END_STRING_MAX);
    int timeout = 0;
    int wait = 0;
    args = parse((char *)(++args), "d,d,s", &timeout, &wait, (signed long)WIOTA_TRANS_END_STRING_MAX, g_dtu_send->exit_flag);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    g_dtu_send->timeout = timeout & 0xFFFF;
    g_dtu_send->wait = wait & 0xFFFF;
    g_dtu_send->flag = 1;
    return AT_RESULT_OK;
}

static at_result_t at_wiota_dtu_send_exec(void)
{
    if (AT_WIOTA_RUN != wiota_state)
    {
        return AT_RESULT_FAILE;
    }
    g_dtu_send->flag = 1;
    return AT_RESULT_OK;
}
#endif
static at_result_t at_wiotarecv_setup(const char *args)
{
    unsigned short timeout = 0;
    uc_recv_back_t result;

    if (AT_WIOTA_DEFAULT == wiota_state)
    {
        return AT_RESULT_FAILE;
    }

    args = parse((char *)(++args), "d", &timeout);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (timeout < 1)
        timeout = WIOTA_WAIT_DATA_TIMEOUT;

    // rt_kprintf("timeout = %d\n", timeout);

    uc_wiota_recv_data(&result, timeout, RT_NULL);
    if (!result.result)
    {
        if (result.type < UC_RECV_SCAN_FREQ)
        {
            at_server_printf("+WIOTARECV,%d,%d,", result.type, result.data_len);
        }
        else if (result.type == UC_RECV_SYNC_LOST)
        {
            at_server_printf("+WIOTASYNC,LOST");
        }
        at_send_data(result.data, result.data_len);
        at_server_printfln("");
        rt_free(result.data);
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_FAILE;
    }
}

static at_result_t at_wiota_recv_exec(void)
{
    uc_recv_back_t result;

    if (AT_WIOTA_DEFAULT == wiota_state)
    {
        return AT_RESULT_FAILE;
    }

    uc_wiota_recv_data(&result, WIOTA_WAIT_DATA_TIMEOUT, RT_NULL);
    if (!result.result)
    {
        if (result.type < UC_RECV_SCAN_FREQ)
        {
            at_server_printf("+WIOTARECV,%d,%d,", result.type, result.data_len);
        }
        else if (result.type == UC_RECV_SYNC_LOST)
        {
            at_server_printf("+WIOTASYNC,LOST");
        }
        at_send_data(result.data, result.data_len);
        at_server_printfln("");
        rt_free(result.data);
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_FAILE;
    }
}

static at_result_t at_wiotalpm_setup(const char *args)
{
    int mode = 0, value = 0, value2 = 0;
    unsigned char is_ok = TRUE;

    // WIOTA_CHECK_AUTOMATIC_MANAGER();

    args = parse((char *)(++args), "d,d,d", &mode, &value, &value2);

    switch (mode)
    {
    case AT_WIOTA_SLEEP:
    {
        at_server_printfln("OK");
        uart_wait_tx_done();
        uc_wiota_sleep_enter((unsigned char)value, (unsigned char)value2);
        break;
    }
#ifdef _LPM_PAGING_
    case AT_WIOTA_PAGING_RX:
    {
        at_server_printfln("OK");
        uart_wait_tx_done();
        uc_wiota_paging_rx_enter((unsigned char)value, (unsigned int)value2);
        break;
    }
#endif
    case AT_WIOTA_CLOCK:
    {
        uc_wiota_set_alarm_time((unsigned int)value);
        break;
    }
#ifdef _CLK_GATING_
    case AT_WIOTA_GATING:
    {
        uc_wiota_set_is_gating((unsigned char)value, (unsigned char)value2);
        break;
    }
#endif // _CLK_GATING_
    case AT_WIOTA_FREQ_DIV:
    {
        uc_wiota_set_freq_div((unsigned char)value);
        break;
    }
    case AT_WIOTA_VOL_MODE:
    {
        uc_wiota_set_vol_mode((unsigned char)value);
        break;
    }
#ifdef _LPM_PAGING_
    case AT_WIOTA_SYNC_PAGING:
    {
        WIOTA_MUST_RUN(wiota_state)
        at_server_printfln("OK");
        uart_wait_tx_done();
        is_ok = uc_wiota_sync_paging_enter(0, 0, (unsigned int)value, (unsigned int)value2);
        break;
    }
    case AT_WIOTA_PAGING_TIMINT:
    {
        WIOTA_MUST_RUN(wiota_state)
        at_server_printfln("OK");
        uart_wait_tx_done();
        is_ok = uc_wiota_sync_paging_enter(1, (unsigned int)value, 0, (unsigned int)value2);
        break;
    }
#endif
    case AT_WIOTA_EX_WK:
    {
        uc_wiota_set_is_ex_wk((unsigned char)value);
        break;
    }
#ifdef _LPM_PAGING_
    case AT_WIOTA_PAGING_TX:
    {
        uc_wiota_paging_tx_start();
        break;
    }
#endif
    default:
        return AT_RESULT_FAILE;
    }
    if (is_ok)
    {
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_FAILE;
    }
}

static at_result_t at_wiotarate_setup(const char *args)
{
    int rate_mode = 0xFF;
    int rate_value = 0xFF;

    args = parse((char *)(++args), "d,d", &rate_mode, &rate_value);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    at_server_printfln("+WIOTARATE: %d, %d", (unsigned char)rate_mode, (unsigned short)rate_value);
    if(uc_wiota_set_data_rate((unsigned char)rate_mode, (unsigned short)rate_value))
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_PARSE_FAILE;
}

static at_result_t at_wiotapow_setup(const char *args)
{
    int mode = 0;
    int power = 0x7F;

    WIOTA_CHECK_AUTOMATIC_MANAGER();

    args = parse((char *)(++args), "d,d", &mode, &power);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    // at can't parse minus value for now
    if (mode == 0)
    {
        uc_wiota_set_cur_power((signed char)(power - 20));
    }
    else if (mode == 1)
    {
        uc_wiota_set_max_power((signed char)(power - 20));
    }
    else if (mode == 2)
    {
        if (power > 1)
        {
            return AT_RESULT_PARSE_FAILE;
        }
        uc_wiota_set_ramp_type((signed char)(power & 0x1));
    }
    else
    {
        return AT_RESULT_PARSE_FAILE;
    }

    return AT_RESULT_OK;
}

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)

void at_handle_log_uart(int uart_number)
{
    rt_device_t device = NULL;
    //    rt_device_t old_device = NULL;
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT; /*init default parment*/

    device = rt_device_find(AT_SERVER_DEVICE);

    if (device)
    {
        rt_device_close(device);
    }

    if (0 == uart_number)
    {
        config.baud_rate = BAUD_RATE_460800;
        rt_console_set_device(AT_SERVER_DEVICE);
        // boot_set_uart0_baud_rate(BAUD_RATE_460800);
    }
    else if (1 == uart_number)
    {
        config.baud_rate = BAUD_RATE_115200;
        rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
        // boot_set_uart0_baud_rate(BAUD_RATE_115200);
    }

    if (device)
    {
        rt_device_control(device, RT_DEVICE_CTRL_CONFIG, &config);
        rt_device_open(device, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    }
}

#endif

static at_result_t at_wiotalog_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    switch (mode)
    {
    case AT_LOG_CLOSE:
    case AT_LOG_OPEN:
        uc_wiota_log_switch(UC_LOG_UART, mode - AT_LOG_CLOSE);
        break;

    case AT_LOG_UART0:
    case AT_LOG_UART1:
#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
        at_handle_log_uart(mode - AT_LOG_UART0);
#endif
        break;

    case AT_LOG_SPI_CLOSE:
    case AT_LOG_SPI_OPEN:
        uc_wiota_log_switch(UC_LOG_SPI, mode - AT_LOG_SPI_CLOSE);
        break;

    default:
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiotastats_query(void)
{
    uc_stats_info_t local_stats_t = {0};

    uc_wiota_get_all_stats(&local_stats_t);

    at_server_printfln("+WIOTASTATS=0,%d,%d,%d,%d,%d,%d,%d", local_stats_t.rach_fail, local_stats_t.active_fail, local_stats_t.ul_succ,
                       local_stats_t.dl_fail, local_stats_t.dl_succ, local_stats_t.bc_fail, local_stats_t.bc_succ);

    return AT_RESULT_OK;
}

static at_result_t at_wiotastats_setup(const char *args)
{
    int mode = 0;
    int type = 0;
    unsigned int back_stats;

    args = parse((char *)(++args), "d,d", &mode, &type);
    if (!args || type >= UC_STATS_TYPE_MAX)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (UC_STATS_READ == mode)
    {
        if (UC_STATS_TYPE_ALL == type)
        {
            at_wiotastats_query();
        }
        else
        {
            back_stats = uc_wiota_get_stats((unsigned char)type);
            at_server_printfln("+WIOTASTATS=%d,%d", type, back_stats);
        }
    }
    else if (UC_STATS_WRITE == mode)
    {
        uc_wiota_reset_stats((unsigned char)type);
    }
    else
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiotacrc_query(void)
{
    at_server_printfln("+WIOTACRC=%d", uc_wiota_get_crc());

    return AT_RESULT_OK;
}

static at_result_t at_wiotacrc_setup(const char *args)
{
    int crc_limit = 0;

    args = parse((char *)(++args), "d", &crc_limit);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_crc((unsigned short)crc_limit);

    return AT_RESULT_OK;
}

#ifdef WIOTA_TEST_LOOP
static void at_test_get_wiota_info(t_at_test_statistical_data *info, int i_time)
{
    uc_throughput_info_t throughput_info;
    uc_stats_info_t stats_info_ptr;
    radio_info_t radio;

    uc_wiota_get_throughput(&throughput_info);
    uc_wiota_reset_throughput(UC_STATS_TYPE_ALL);
    // rt_kprintf("throughput_info.ul_succ_data_len = %d\n", throughput_info.ul_succ_data_len);
    AT_TEST_GET_RATE(i_time, g_test_data.num, throughput_info.ul_succ_data_len,
                     info->upcurrentrate, info->upaverate, info->upminirate, info->upmaxrate);
    // rt_kprintf("upcurren %d  upave %d min %d max %d\n", info->upcurrentrate, info->upaverate, info->upminirate, info->upmaxrate);

    // rt_kprintf("throughput_info.dl_succ_data_len = %d\n", throughput_info.dl_succ_data_len);
    AT_TEST_GET_RATE(i_time, g_test_data.num, throughput_info.dl_succ_data_len,
                     info->downcurrentrate, info->downavgrate, info->downminirate, info->downmaxrate);
    g_test_data.num++;

    uc_wiota_get_all_stats(&stats_info_ptr);
    //    uc_wiota_reset_stats(UC_STATS_TYPE_ALL);

    // rt_kprintf("rach_fail=%d active_fail=%d ul_succ=%d\n", stats_info_ptr.rach_fail, stats_info_ptr.active_fail, stats_info_ptr.ul_succ);
    AT_TEST_CALCUTLATE(info->send_fail,
                       stats_info_ptr.rach_fail + stats_info_ptr.active_fail + stats_info_ptr.ul_succ,
                       stats_info_ptr.rach_fail + stats_info_ptr.active_fail);

    // rt_kprintf("dl_fail=%d dl_succ=%d\n", stats_info_ptr.dl_fail, stats_info_ptr.dl_succ);
    AT_TEST_CALCUTLATE(info->recv_fail,
                       stats_info_ptr.dl_fail + stats_info_ptr.dl_succ,
                       stats_info_ptr.dl_fail);

    uc_wiota_get_radio_info(&radio);
    info->max_mcs = radio.max_mcs;
    info->mcs = radio.cur_mcs;
    info->power = radio.cur_power;
    info->rssi = radio.rssi;
    info->snr = radio.snr;
}

static int at_test_mcs_rate(int mcs)
{
    sub_system_config_t config;
    uc_wiota_get_system_config(&config);
    int frameLen = uc_wiota_get_frame_len();
    int result = 8 * symLen_mcs_byte[config.symbol_length][mcs] * 1000 / frameLen;
    return result;
}

static void at_test_report_to_uart(void)
{
    at_test_get_wiota_info(&g_test_data.statistical, g_test_data.time);
    int rate_mcs = at_test_mcs_rate(g_test_data.statistical.max_mcs);
    //    unsigned int total;
    //    unsigned int used;
    //    unsigned int max_used;
    unsigned int dl_fail = uc_wiota_get_stats(UC_STATS_DL_FAIL);
    unsigned int dl_succ = uc_wiota_get_stats(UC_STATS_DL_SUCC);
    unsigned int dl_rato = 0;
    //    uc_stats_info_t local_stats_t = {0};

    //    uc_wiota_get_all_stats(&local_stats_t);
    //    dl_fail = local_stats_t.dl_fail;
    //    dl_succ = local_stats_t.dl_succ;
    if (0 != (dl_succ + dl_fail))
    {
        dl_rato = dl_fail * 100 / (dl_succ + dl_fail);
    }
    uc_wiota_reset_stats(UC_STATS_TYPE_ALL);

    //    rt_memory_info(&total,&used,&max_used);
    switch (g_test_data.type)
    {
    case AT_TEST_COMMAND_UP_TEST:
        at_server_printfln("+UP: %dbps %dbps %d rssi -%d snr %d",
                           g_test_data.statistical.upcurrentrate / 1000 * 8, rate_mcs,
                           g_test_data.statistical.mcs, g_test_data.statistical.rssi, g_test_data.statistical.snr);
        break;
    case AT_TEST_COMMAND_DOWN_TEST:
        at_server_printfln("+DOWN: %dbps %d rssi -%d snr %d",
                           g_test_data.statistical.downcurrentrate / 1000 * 8,
                           g_test_data.statistical.mcs, g_test_data.statistical.rssi, g_test_data.statistical.snr);
        break;
    case AT_TEST_COMMAND_LOOP_TEST:
        at_server_printfln("+LOOP: %dbps %dbps %dbps %d rssi -%d snr %d",
                           g_test_data.statistical.upcurrentrate / 1000 * 8,
                           g_test_data.statistical.downcurrentrate / 1000 * 8, rate_mcs,
                           g_test_data.statistical.mcs, g_test_data.statistical.rssi, g_test_data.statistical.snr);
        break;
    case AT_TEST_COMMAND_DATA_MODE:
        at_server_printfln("+DATA: %dbps %dbps %d rssi -%d snr %d",
                           g_test_data.statistical.downcurrentrate / 1000 * 8, rate_mcs,
                           g_test_data.statistical.mcs, g_test_data.statistical.rssi, g_test_data.statistical.snr);
        break;
    case AT_TEST_COMMAND_DATA_DOWN:
        at_server_printfln("+DATADOWN: %dbps %d rssi -%d snr %d dl succ %d fail %d rato %d%",
                           g_test_data.statistical.downcurrentrate / 1000 * 8,
                           g_test_data.statistical.mcs, g_test_data.statistical.rssi, g_test_data.statistical.snr,
                           dl_succ, dl_fail, dl_rato);
        break;
    default:
        break;
    }
}

static void at_test_mode_time_fun(void *parameter)
{
    at_test_report_to_uart();
}

static void at_test_mode_task_fun(void *parameter)
{
    at_test_data_t *test_data = &g_test_data;
    t_at_test_queue_data *recv_queue_data = RT_NULL;
    unsigned int queue_data_on = 0;
    int send_flag = 0;
    t_at_test_communication *communication = RT_NULL;
    int data_test_flag = 1;

    test_data->test_data_len = sizeof(t_at_test_communication);

    communication = rt_malloc(sizeof(t_at_test_communication) + CRC16_LEN);
    if (RT_NULL == communication)
    {
        rt_kprintf("test_mode malloc err\n");
        return;
    }
    rt_memset(communication, 0, sizeof(t_at_test_communication));
    rt_memcpy(communication->head, AT_TEST_COMMUNICATION_HEAD, strlen(AT_TEST_COMMUNICATION_HEAD));
    communication->command = AT_TEST_COMMAND_DEFAULT;
    communication->timeout = 0;
    communication->all_len = sizeof(t_at_test_communication);
    // test_data->test_mode_timer = RT_NULL;
    test_data->type = AT_TEST_COMMAND_DEFAULT;
    int time_start_flag = 1;
    while (1)
    {
        // recv queue data. wait start test
        if (RT_EOK == rt_mq_recv(test_data->test_queue, &queue_data_on, 4, 100)) // RT_WAITING_NO
        {
            recv_queue_data = (t_at_test_queue_data *)queue_data_on;
            rt_kprintf("type %d\n", recv_queue_data->type);

            switch ((int)recv_queue_data->type)
            {
            case AT_TEST_MODE_RECVDATA:
            {
                uc_recv_back_p recv_data = recv_queue_data->data;
                t_at_test_communication *communication_data = (t_at_test_communication *)(recv_data->data);
                // pasre data
                test_data->time = communication_data->timeout;
                test_data->type = communication_data->command;
                test_data->test_data_len = communication_data->test_len;

                if (communication->all_len != communication_data->all_len || communication->command != communication_data->command)
                {
                    // re malloc
                    if (AT_TEST_COMMAND_DATA_MODE != test_data->type)
                    {
                        rt_free(communication);
                        communication = rt_malloc(communication_data->all_len + CRC16_LEN);
                        rt_memset(communication, 0, communication_data->all_len);
                        rt_memcpy(communication, communication_data, communication_data->all_len);
                        if (RT_NULL == communication)
                        {
                            rt_kprintf("malloc f\n");
                            return;
                        }
                    }
                }

                if (RT_NULL != test_data->test_mode_timer && AT_TEST_COMMAND_DATA_MODE != test_data->type && time_start_flag)
                {
                    // if (test_data->test_mode_timer)
                    //  {
                    int timeout = test_data->time * 1000;
                    rt_timer_control(test_data->test_mode_timer, RT_TIMER_CTRL_SET_TIME, &timeout);
                    rt_timer_start(test_data->test_mode_timer);
                    time_start_flag = 0;
                    // }
                }

                // loop data to ap
                if (AT_TEST_COMMAND_LOOP_TEST == test_data->type)
                {
                    send_flag = 0;
                }

                // free data
                rt_free(communication_data);
                rt_free(recv_data);
                rt_free(recv_queue_data);

                if (AT_TEST_COMMAND_DATA_MODE == test_data->type && 1 == data_test_flag)
                {
                    uc_wiota_set_data_rate(UC_RATE_NORMAL, communication_data->mcs_num);
                    uc_wiota_test_loop(1);
                    data_test_flag = 0;
                }
                break;
            }
            case AT_TEST_MODE_QUEUE_EXIT:
            {
                if (AT_TEST_COMMAND_DATA_MODE == g_test_data.type)
                {
                    uc_wiota_set_data_rate(UC_RATE_NORMAL, UC_MCS_AUTO);
                    uc_wiota_test_loop(0);
                }
                else
                {
                    communication->command = AT_TEST_COMMAND_STOP; // also stop cmd
                    uc_wiota_send_data((unsigned char *)communication, communication->all_len, 20000, RT_NULL);
                }

                rt_free(recv_queue_data);
                rt_free(communication);
                rt_sem_release(test_data->test_sem);
                return;
            }
            }
        }

        if (test_data->type == AT_TEST_COMMAND_UP_TEST || send_flag == 0)
        {
            uc_op_result_e res;

            if (test_data->type == AT_TEST_COMMAND_UP_TEST)
            {
                unsigned char *send_test = rt_malloc(test_data->test_data_len + CRC16_LEN);

                uc_wiota_algo_srand(uc_wiota_get_curr_rf_cnt());
                for (int i = 0; i < test_data->test_data_len; i++)
                {
                    *((unsigned char *)send_test + i) = uc_wiota_algo_rand() & 0xFF;
                }
                res = uc_wiota_send_data(send_test, test_data->test_data_len, 20000, RT_NULL);
                rt_free(send_test);
            }
            else
            {
                res = uc_wiota_send_data((unsigned char *)communication, communication->all_len, 20000, RT_NULL);
            }

            if (res == UC_OP_SUCC)
            {
                send_flag = 1;
            }
        }
    }
}

static at_result_t at_test_mode_start_setup(const char *args)
{
    int mode = 0;
    int time = 0;

    args = parse((char *)(++args), "d,d", &mode, &time);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (wiota_state != AT_WIOTA_RUN)
        return AT_RESULT_REPETITIVE_FAILE;

    if (g_test_data.time > 0)
    {
        rt_kprintf("repeat use\n");
        return AT_RESULT_PARSE_FAILE;
    }

    g_test_data.mode = mode;
    g_test_data.time = 1;

    // create timer
    if (g_test_data.test_mode_timer == NULL)
    {
        g_test_data.test_mode_timer = rt_timer_create("teMode",
                                                      at_test_mode_time_fun,
                                                      RT_NULL,
                                                      g_test_data.time * 1000,
                                                      RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);

        if (RT_NULL == g_test_data.test_mode_timer)
        {
            rt_kprintf("timer_create err\n");
            return AT_RESULT_PARSE_FAILE;
        }
    }

    if (mode == 0)
    {
        g_test_data.type = AT_TEST_COMMAND_DATA_DOWN;

        /* fix bug237: watchdog timeout when throughput test */
        g_test_data.time = (time == 0) ? 3 : time;

        int timeout = g_test_data.time * 1000;
        rt_timer_control(g_test_data.test_mode_timer, RT_TIMER_CTRL_SET_TIME, &timeout);
        rt_timer_start(g_test_data.test_mode_timer);
        uc_wiota_test_loop(2);
    }
    else if (mode == 1)
    {
        // create queue
        g_test_data.test_queue = rt_mq_create("teMode", 4, 8, RT_IPC_FLAG_PRIO);
        if (RT_NULL == g_test_data.test_queue)
        {
            rt_kprintf("create_queue err\n");
            return AT_RESULT_PARSE_FAILE;
        }

        // create sem
        g_test_data.test_sem = rt_sem_create("teMode", 0, RT_IPC_FLAG_PRIO);
        if (RT_NULL == g_test_data.test_sem)
        {
            rt_kprintf("sem_create err\n");
            return AT_RESULT_PARSE_FAILE;
        }

        // create task
        g_test_data.test_mode_task = rt_thread_create("teMode",
                                                      at_test_mode_task_fun,
                                                      RT_NULL,
                                                      2048,
                                                      RT_THREAD_PRIORITY_MAX / 3 - 1,
                                                      3);
        if (RT_NULL == g_test_data.test_mode_task)
        {
            rt_kprintf("task_create err\n");
            return AT_RESULT_PARSE_FAILE;
        }
        rt_thread_startup(g_test_data.test_mode_task);
    }
    else
    {
        return AT_RESULT_PARSE_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_test_mode_stop_exec(void)
{
    if (g_test_data.mode == 0)
    {
        uc_wiota_test_loop(0);

        if (RT_NULL != g_test_data.test_mode_timer)
        {
            rt_timer_delete(g_test_data.test_mode_timer);
        }

        rt_memset(&g_test_data, 0, sizeof(g_test_data));
    }
    else
    {
        unsigned int send_data_address;

        if (g_test_data.time < 1)
        {
            rt_kprintf("no run\n");
            return AT_RESULT_PARSE_FAILE;
        }

        g_test_data.time = 0;

        t_at_test_queue_data *data = rt_malloc(sizeof(t_at_test_queue_data));

        if (RT_NULL == data)
        {
            rt_kprintf("malloc err\n");
            return AT_RESULT_PARSE_FAILE;
        }
        data->type = AT_TEST_MODE_QUEUE_EXIT;
        send_data_address = (unsigned int)data;

        rt_err_t res = rt_mq_send_wait(g_test_data.test_queue, &send_data_address, 4, 1000);
        if (0 != res)
        {
            rt_kprintf("mq_send_wait err\n");
            rt_free(data);
            return AT_RESULT_PARSE_FAILE;
        }

        // wait  RT_WAITING_FOREVER
        if (RT_EOK != rt_sem_take(g_test_data.test_sem, RT_WAITING_FOREVER))
        {
            rt_kprintf("sem_take err\n");
            return AT_RESULT_PARSE_FAILE;
        }

        if (RT_NULL != g_test_data.test_mode_timer)
        {
            rt_timer_stop(g_test_data.test_mode_timer);
            rt_timer_delete(g_test_data.test_mode_timer);
        }

        rt_mq_delete(g_test_data.test_queue);
        rt_sem_delete(g_test_data.test_sem);
        rt_thread_delete(g_test_data.test_mode_task);
        rt_memset(&g_test_data, 0, sizeof(g_test_data));
    }
    return AT_RESULT_OK;
}
#endif

static at_result_t at_wiotaosc_query(void)
{
    at_server_printfln("+WIOTAOSC=%d", uc_wiota_get_is_osc());

    return AT_RESULT_OK;
}

static at_result_t at_wiotaosc_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_is_osc((unsigned char)mode);

    return AT_RESULT_OK;
}
#ifdef _LIGHT_PILOT_
static at_result_t at_wiotalight_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_light_func_enable((unsigned char)mode);

    return AT_RESULT_OK;
}
#endif
static at_result_t at_wiota_save_static_exec(void)
{
    WIOTA_CHECK_AUTOMATIC_MANAGER();

    uc_wiota_save_static_info();

    return AT_RESULT_OK;
}

static at_result_t at_wiota_save_mode_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_exit_save_static((unsigned char)mode);

    return AT_RESULT_OK;
}

static at_result_t at_wiotabitscb_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (uc_wiota_set_bitscb((unsigned char)mode))
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_PARSE_FAILE;
}

static at_result_t at_wiotamultcast_setup(const char *args)
{
    unsigned int multcastList[3];

    args = parse((char *)(++args), "y,y,y", &(multcastList[0]), &(multcastList[1]), &(multcastList[2]));

    WIOTA_MUST_ALREADY_INIT(wiota_state)

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_multcast_id_list(multcastList);

    return AT_RESULT_OK;
}
#ifdef _LPM_PAGING_
static at_result_t at_paging_tx_config_query(void)
{
    uc_lpm_tx_cfg_t config;
    uc_wiota_get_paging_tx_cfg(&config);
    at_server_printfln("+WIOTAPAGINGTX=%d,%d,%d,%d,%d,%d",
                       config.freq, config.spectrum_idx, config.bandwidth,
                       config.symbol_length, config.awaken_id, config.send_time);

    return AT_RESULT_OK;
}

static at_result_t at_paging_tx_config_setup(const char *args)
{
    uc_lpm_tx_cfg_t config = {0};
    unsigned int temp[6];

    WIOTA_MUST_ALREADY_INIT(wiota_state)

    args = parse((char *)(++args), "d,d,d,d,d,d",
                 &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5]);

    if (!args || temp[0] > 255)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_get_paging_tx_cfg(&config);

    config.freq = (unsigned char)temp[0];
    config.spectrum_idx = (unsigned char)temp[1];
    config.bandwidth = (unsigned char)temp[2];
    config.symbol_length = (unsigned char)temp[3];
    config.awaken_id = (unsigned short)temp[4];
    config.send_time = (unsigned int)temp[5];

    if (uc_wiota_set_paging_tx_cfg(&config))
    {
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_PARSE_FAILE;
    }
}

static at_result_t at_paging_rx_config_query(void)
{
    uc_lpm_rx_cfg_t config;
    uc_wiota_get_paging_rx_cfg(&config);
    at_server_printfln("+WIOTAPAGINGRX=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                       config.freq, config.spectrum_idx, config.bandwidth,
                       config.symbol_length, config.awaken_id, config.detect_period,
                       config.lpm_nlen, config.lpm_utimes, config.threshold,
                       config.extra_flag, config.extra_period);

    return AT_RESULT_OK;
}

static at_result_t at_paging_rx_config_setup(const char *args)
{
    uc_lpm_rx_cfg_t config = {0};
    unsigned int temp[11];
    // WIOTA_MUST_ALREADY_INIT(wiota_state)

    args = parse((char *)(++args), "d,d,d,d,d,d,d,d,d,d,d",
                 &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5], &temp[6], &temp[7], &temp[8], &temp[9], &temp[10]);

    if (!args || temp[0] > 255)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_get_paging_rx_cfg(&config);

    config.freq = (unsigned char)temp[0];
    config.spectrum_idx = (unsigned char)temp[1];
    config.bandwidth = (unsigned char)temp[2];
    config.symbol_length = (unsigned char)temp[3];
    config.awaken_id = (unsigned short)temp[4];
    config.detect_period = (unsigned int)temp[5];
    config.lpm_nlen = (unsigned char)temp[6];
    config.lpm_utimes = (unsigned char)temp[7];
    config.threshold = (unsigned short)temp[8];
    config.extra_flag = (unsigned short)temp[9];
    config.extra_period = (unsigned int)temp[10];

    if (uc_wiota_set_paging_rx_cfg(&config))
    {
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_PARSE_FAILE;
    }
}

static at_result_t at_paging_rx_config_another_query(void)
{
    uc_lpm_rx_cfg_t config;
    uc_wiota_get_paging_rx_cfg(&config);
    at_server_printfln("+WIOTAPAGINGRXANO=%d,%d",
                       config.period_multiple, config.awaken_id_another);

    return AT_RESULT_OK;
}

static at_result_t at_paging_rx_config_another_setup(const char *args)
{
    uc_lpm_rx_cfg_t config = {0};
    unsigned int temp[3];

    // WIOTA_MUST_ALREADY_INIT(wiota_state)

    args = parse((char *)(++args), "d,d", &temp[0], &temp[1]);

    if (!args || temp[0] > 255)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_get_paging_rx_cfg(&config);

    config.period_multiple = (unsigned char)temp[0];
    config.awaken_id_another = (unsigned short)temp[1];

    if (uc_wiota_set_paging_rx_cfg(&config))
    {
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_PARSE_FAILE;
    }
}

static at_result_t at_paging_config_mode_query(void)
{
    uc_lpm_rx_cfg_t config_rx;
    uc_lpm_tx_cfg_t config_tx;
    uc_wiota_get_paging_rx_cfg(&config_rx);
    uc_wiota_get_paging_tx_cfg(&config_tx);
    at_server_printfln("+WIOTAPAGINGMODE=%d,%d", config_rx.mode, config_tx.mode);
    return AT_RESULT_OK;
}

static at_result_t at_paging_config_mode_setup(const char *args)
{
    uc_lpm_rx_cfg_t config_rx = {0};
    uc_lpm_tx_cfg_t config_tx = {0};
    unsigned int rx_mode, tx_mode;

    // WIOTA_MUST_ALREADY_INIT(wiota_state)

    args = parse((char *)(++args), "d,d", &rx_mode, &tx_mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_get_paging_rx_cfg(&config_rx);
    uc_wiota_get_paging_tx_cfg(&config_tx);

    config_rx.mode = rx_mode;
    config_tx.mode = tx_mode;

    if (uc_wiota_set_paging_rx_cfg(&config_rx) && uc_wiota_set_paging_tx_cfg(&config_tx))
    {
        return AT_RESULT_OK;
    }
    return AT_RESULT_PARSE_FAILE;
}
#endif // _LPM_PAGING_
static at_result_t at_wiotatxmode_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    if (uc_wiota_set_tx_mode((unsigned char)mode))
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_CMD_ERR;
}

static at_result_t at_wiota_hard_switch_state_exec(void)
{
    uc_wiota_hard_switch_to_active();
    return AT_RESULT_OK;
}

static at_result_t at_memory_query(void)
{
    unsigned int total = 0;
    unsigned int used = 0;
    unsigned int max_used = 0;
#ifndef RT_USING_MEMHEAP_AS_HEAP
#if defined(RT_USING_HEAP) && defined(RT_USING_SMALL_MEM)
    rt_memory_info(&total, &used, &max_used);
    // rt_kprintf("total %d used %d maxused %d\n", total, used, max_used);
#endif
#endif

    at_server_printfln("+MEMORY=%d,%d,%d", total, used, max_used);

    return AT_RESULT_OK;
}

static at_result_t at_wiota32k_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_outer_32K((unsigned char)mode);

    return AT_RESULT_OK;
}

static at_result_t at_adjust_result_query(const char *args)
{
    int mode = 0;
    signed char temp;
    unsigned char dir;
    unsigned int offset;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (uc_wiota_get_adjust_result(mode, &temp, &dir, &offset))
    {
        at_server_printfln("+ADJUST=%d,%d,%u", temp, dir, offset);
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_PARSE_FAILE;
    }
}
#ifdef _LPM_PAGING_
static at_result_t at_wiotawaken_query(void)
{
    unsigned char awaken_cause = 0;
    unsigned char is_cs_awawen = 0;
    unsigned char pg_awaken_cause = 0;
    unsigned char detect_idx = 0;
    unsigned int detected_times = 0;

    awaken_cause = uc_wiota_get_awakened_cause(&is_cs_awawen);

    if (AWAKENED_CAUSE_PAGING == awaken_cause)
    {
        pg_awaken_cause = uc_wiota_get_paging_awaken_cause(&detected_times, &detect_idx);
    }

    at_server_printfln("+WIOTAWAKEN:%d,%d,%d,%u,%d", awaken_cause, is_cs_awawen, pg_awaken_cause, detected_times, detect_idx);

    return AT_RESULT_OK;
}
#endif // _LPM_PAGING_
static at_result_t at_wiota_resend_setup(const char *args)
{
    int times = 0;

    args = parse((char *)(++args), "d", &times);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_sm_resend_times((unsigned char)times);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_usetemp_setup(const char *args)
{
    int flag = 0;

    args = parse((char *)(++args), "d", &flag);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (uc_wiota_set_is_use_temp((unsigned char)flag))
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_PARSE_FAILE;
}

static at_result_t at_wiota_usetemp_query(void)
{
    at_server_printfln("+IUTEMP=%d", uc_wiota_get_is_use_temp());
    return AT_RESULT_OK;
}

static at_result_t at_module_id_query(void)
{
    unsigned char module_id[19] = {0};
    unsigned char valid;

    valid = uc_wiota_get_module_id(module_id);

    at_server_printfln("+MODULEID=%s,%d", module_id, valid);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_test_subframe_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (mode >= 0 && mode <= 2)
    {
        uc_wiota_set_subframe_test((unsigned char)mode);
    }
    else
    {
        at_server_printfln("+WIOTASUBFTEST=%u,%u", uc_wiota_get_subframe_test(0), uc_wiota_get_subframe_test(1));
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_gps_query(void)
{
    uc_gps_time_t gps_info = {0};
    uc_wiota_get_gps_info(&gps_info);
    // <isValid>,<gps_s>,<gps_us>,<rf_us>,<rf_curr>
    at_server_printfln("+GPSINFO=%d,%u,%u,%u,%u", gps_info.is_valid, gps_info.gps_time_s,
                       gps_info.gps_time_us, gps_info.rf_cnt_us, gps_info.rf_cnt_curr);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_gps_gpio_setup(const char *args)
{
    int mode = 0; // 0 close, 1, open
    int gpio = 0;

    args = parse((char *)(++args), "d,d", &mode, &gpio);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_gps_gpio((unsigned char)mode, (unsigned char)gpio);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_fn_query(void)
{
    at_server_printfln("+WIOTAFN=%d,%d,%u", uc_wiota_get_is_report_fn(),
                       uc_wiota_get_is_frame_valid(), uc_wiota_get_frame_num());
    return AT_RESULT_OK;
}

static at_result_t at_wiota_fn_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (mode >= 0 && mode <= 1)
    {
        uc_wiota_set_is_report_fn(mode);
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_pa_gpio_setup(const char *args)
{
    int mode = 0; // 0 close, 1, open
    int pa_gpio = 0;
    int pa_trigger = 0;

    args = parse((char *)(++args), "d,d,d", &mode, &pa_gpio, &pa_trigger);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_pa_gpio((unsigned char)mode, (unsigned char)pa_gpio, (unsigned char)pa_trigger);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_adc_adj_query(void)
{
    uc_adc_adj_t adc_adj = {0};
    uc_wiota_get_adc_adj_info(&adc_adj);
    at_server_printfln("+WIOTADCDJ=%d,%d,%d,%d,%d,%d,%d", adc_adj.is_close,
                       adc_adj.is_valid, adc_adj.adc_trm, adc_adj.adc_ka, adc_adj.adc_mida,
                       adc_adj.adc_kb, adc_adj.adc_midb);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_adc_adj_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (mode >= 0 && mode <= 1)
    {
        uc_wiota_set_adc_adj_close(mode); // 1 means close!
    }

    return AT_RESULT_OK;
}

void at_wiota_awaken_notice(void)
{
    uint8_t awakened_cause = 0;
    uint8_t is_cs_awakened = 0;

    awakened_cause = uc_wiota_get_awakened_cause(&is_cs_awakened);
    if (awakened_cause == AWAKENED_CAUSE_SLEEP ||
        awakened_cause == AWAKENED_CAUSE_PAGING)
    {
        at_server_printfln("OK");
        at_server_printfln("+WIOTAWAKEN");
    }
}

#ifdef _QUICK_CONNECT_
static at_result_t at_wiota_quick_start(const char *args)
{
    int ret = AT_RESULT_FAILE;
    unsigned char onoff = 0;
    e_qc_mode mode = 0;
    unsigned short freq = 0;

    args = parse((char *)(++args), "d,d,d", &onoff, &freq, &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    if (uc_wiota_set_freq_info(freq))
    {
        if (mode < QC_MODE_MAX)
        {
            if (onoff == 1)
            {
                ret = wiota_quick_connect_start((unsigned short)freq, (e_qc_mode)mode);
            }
            else if (onoff == 0)
            {
                ret = wiota_quick_connect_stop();
            }
        }
    }
    else
    {
        return AT_RESULT_CHECK_FAILE;
    }

    return ret;
}

#endif

static at_result_t at_new_ldo_setup(const char *args)
{
    unsigned int value = 0;

    args = parse((char *)(++args), "d", &value);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_new_ldo((unsigned char)value);

    return AT_RESULT_OK;
}


AT_CMD_EXPORT("AT+AUTOCONNECT", "<v>", RT_NULL, at_auto_connect_query, at_auto_connect_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAVERSION", RT_NULL, RT_NULL, at_wiota_version_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAINIT", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_init_exec);
AT_CMD_EXPORT("AT+WIOTALPM", "=<mode>,<v>,<v2>", RT_NULL, RT_NULL, at_wiotalpm_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARATE", "=<mode>,<v>", RT_NULL, RT_NULL, at_wiotarate_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAPOW", "=<mode>,<power>", RT_NULL, RT_NULL, at_wiotapow_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASCANFREQ", "=<timeout>,<mode>,<dataLen>,<freqnum>", RT_NULL, RT_NULL, at_scan_freq_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAFREQ", "=<freqpint>", RT_NULL, at_freq_query, at_freq_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTADCXO", "=<dcxo>", RT_NULL, at_dcxo_query, at_dcxo_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTATXMODE", "=<mode>", RT_NULL, RT_NULL, at_wiotatxmode_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTADEVADDRESS", "=<addr>", RT_NULL, at_dev_addr_query, at_dev_addr_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAUSERID", "=<id0>", RT_NULL, at_userid_query, at_userid_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARADIO", "=<temp>,<rssi>,<ber>,<snr>,<curpow>,<maxpow>,<curmcs>,<frac>", RT_NULL, at_radio_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTACONFIG", "=<apower>,<idlen>,<symbol>,<dlul>,<bt>,<groupnum>,<specidx>,<oldv>,<bitscb>,<subsysid>",
              RT_NULL, at_system_config_query, at_system_config_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARUN", "=<state>", RT_NULL, RT_NULL, at_wiota_cfun_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTACONNECT", "=<state>,<activetime>", RT_NULL, at_connect_query, at_connect_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASEND", "=<timeout>,<len>", RT_NULL, RT_NULL, at_wiotasend_setup, at_wiotasend_exec);
AT_CMD_EXPORT("AT+WIOTASENDBYFN", "=<timeout>,<len>,<fn>", RT_NULL, RT_NULL, at_wiotasend_by_fn_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASENDNOBK", "=<timeout>,<len>", RT_NULL, RT_NULL, at_wiotasend_noblock_setup, at_wiotasend_noblock_exec);
AT_CMD_EXPORT("AT+WIOTATRANS", "=<timeout>,<end>", RT_NULL, RT_NULL, at_wiotatrans_setup, at_wiotatrans_exec);
// AT_CMD_EXPORT("AT+WIOTADTUSEND", "=<timeout>,<wait>,<end>", RT_NULL, RT_NULL, at_wiota_dtu_send_setup, at_wiota_dtu_send_exec);
AT_CMD_EXPORT("AT+WIOTARECV", "=<timeout>", RT_NULL, RT_NULL, at_wiotarecv_setup, at_wiota_recv_exec);
AT_CMD_EXPORT("AT+WIOTALOG", "=<mode>", RT_NULL, RT_NULL, at_wiotalog_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASTATS", "=<mode>,<type>", RT_NULL, at_wiotastats_query, at_wiotastats_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTACRC", "=<crclimit>", RT_NULL, at_wiotacrc_query, at_wiotacrc_setup, RT_NULL);
#ifdef WIOTA_TEST_LOOP
// not support !
AT_CMD_EXPORT("AT+THROUGHTSTART", "=<mode>,<time>", RT_NULL, RT_NULL, at_test_mode_start_setup, RT_NULL);
AT_CMD_EXPORT("AT+THROUGHTSTOP", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_test_mode_stop_exec);
#endif
AT_CMD_EXPORT("AT+WIOTAOSC", "=<mode>", RT_NULL, at_wiotaosc_query, at_wiotaosc_setup, RT_NULL);
#ifdef _LIGHT_PILOT_
AT_CMD_EXPORT("AT+WIOTALIGHT", "=<mode>", RT_NULL, RT_NULL, at_wiotalight_setup, RT_NULL);
#endif
AT_CMD_EXPORT("AT+WIOTASAVESTATIC", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_save_static_exec);
AT_CMD_EXPORT("AT+WIOTASAVEMODE", "=<mode>", RT_NULL, RT_NULL, at_wiota_save_mode_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTABITSCB", "=<mode>", RT_NULL, RT_NULL, at_wiotabitscb_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAMULTCAST", "=<id0>,<id1>,<id2>", RT_NULL, RT_NULL, at_wiotamultcast_setup, RT_NULL);
#ifdef _LPM_PAGING_
AT_CMD_EXPORT("AT+WIOTAPAGINGTX", "=<freq>,<specidx>,<band>,<symbol>,<awakenid>,<send_time>",
              RT_NULL, at_paging_tx_config_query, at_paging_tx_config_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAPAGINGRX", "=<freq>,<specidx>,<band>,<symbol>,<awakenid>,<detect_per>,<nlen>,<utimes>,<thres>,<extra>,<extra_per>",
              RT_NULL, at_paging_rx_config_query, at_paging_rx_config_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAPAGINGRXANO", "=<per_multi>,<awakenid_ano>",
              RT_NULL, at_paging_rx_config_another_query, at_paging_rx_config_another_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAPAGINGMODE", "=<rxmod>,<txmod>",
              RT_NULL, at_paging_config_mode_query, at_paging_config_mode_setup, RT_NULL);
#endif // _LPM_PAGING_
AT_CMD_EXPORT("AT+WIOTAHARDSWITCH", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_hard_switch_state_exec);
AT_CMD_EXPORT("AT+WIOTACKMEM", "=<total>,<used>,<maxused>", RT_NULL, at_memory_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAOUTERK", "=<mode>", RT_NULL, RT_NULL, at_wiota32k_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTADJRST", "=<mode>", RT_NULL, RT_NULL, at_adjust_result_query, RT_NULL);
#ifdef _LPM_PAGING_
AT_CMD_EXPORT("AT+WIOTAWAKEN", RT_NULL, RT_NULL, at_wiotawaken_query, RT_NULL, RT_NULL);
#endif
AT_CMD_EXPORT("AT+WIOTARESEND", "=<times>", RT_NULL, RT_NULL, at_wiota_resend_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAIUTEMP", "=<isuse>", RT_NULL, at_wiota_usetemp_query, at_wiota_usetemp_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAMODULEID", "=<moduid>", RT_NULL, at_module_id_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASUBFTEST", "=<mode>", RT_NULL, RT_NULL, at_wiota_test_subframe_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAGPSINFO", "=<mode>,<gpio>", RT_NULL, at_wiota_gps_query, at_wiota_gps_gpio_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAFN", "=<mode>", RT_NULL, at_wiota_fn_query, at_wiota_fn_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAEXPA", "=<mode>,<gpio>,<trig>", RT_NULL, RT_NULL, at_wiota_pa_gpio_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTADCDJ", "=<mode>", RT_NULL, at_wiota_adc_adj_query, at_wiota_adc_adj_setup, RT_NULL);

#ifdef _QUICK_CONNECT_
AT_CMD_EXPORT("AT+WIOTAQC", "=<onoff>,<freq>,<mode>", RT_NULL, RT_NULL, at_wiota_quick_start, RT_NULL);
#endif // QUICK_CONNECT

AT_CMD_EXPORT("AT+WIOTALDO", "=<value>", RT_NULL, RT_NULL, at_new_ldo_setup, RT_NULL);

#endif // UC8288_MODULE
#endif // RT_USING_AT
