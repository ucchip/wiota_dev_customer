/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date                 Author                Notes
 * 20201-8-17     ucchip-wz          v0.00
 */
//#ifdef AT_USING_SERVER
#ifdef UC8288_MODULE

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <string.h>
#include "uc_wiota_api.h"
#include "at.h"
#include "ati_prs.h"
#include "uc_string_lib.h"
#include "uc_adda.h"
#include "uc_boot_download.h"
//#include "adp_mem.h"
#include "at_wiota.h"
//#include "adp_task.h"


const u16_t symLen_mcs_byte[4][8] = {{7,  9, 52, 66,  80,   0,   0, 0},
                                     {7, 15, 22, 52, 108, 157, 192, 0},
                                     {7, 15, 31, 42,  73, 136, 255, 297},
                                     {7, 15, 31, 63, 108, 220, 451, 619}};


enum at_wiota_lpm
{
    AT_WIOTA_SLEEP = 0,
    AT_WIOTA_GATING,
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
#define WIOTA_TRANS_END_STRING "$$$$"

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

#define WIOTA_CHECK_AUTOMATIC_MANAGER() \
if(uc_wiota_get_auto_connect_flag())\
 return AT_RESULT_REFUSED;

enum at_test_mode_data_type
{
    AT_TEST_MODE_RECVDATA = 0,
    AT_TEST_MODE_QUEUE_EXIT,
};

typedef struct at_test_queue_data
{
    enum at_test_mode_data_type type;
    void * data;
    void * paramenter;
}t_at_test_queue_data;

typedef struct at_test_statistical_data
{
    int type;
    int dev;
    
    int upcurrentrate;
    int upaverate;
    int upminirate;
    int upmaxrate;

    int downcurrentrate;
    int downaverate;
    int downminirate;
    int downmaxrate;
    
    int send_fail;
    int recv_fail;
    int max_mcs;
    int msc;
    int power;
    int rssi;
    int snr;
}t_at_test_statistical_data;

typedef struct at_test_data
{
    char type;
    short test_data_len;
    int time;
    int num;
    rt_timer_t test_mode_timer;
    rt_thread_t test_mode_task;
    //rt_thread_t test_data_task;
    rt_mq_t test_queue;
    rt_sem_t test_sem;
    char tast_state;
    t_at_test_statistical_data  statistical;
}t_at_test_data;

enum at_test_communication_command
{
    AT_TEST_COMMAND_DEFAULT = 0,
    AT_TEST_COMMAND_UP_TEST,
    AT_TEST_COMMAND_DOWN_TEST,
    AT_TEST_COMMAND_LOOP_TEST,
    AT_TEST_COMMAND_DATA_MODE,
   
};

#define AT_TEST_COMMUNICATION_HEAD_LEN 16
//#define AT_TEST_COMMUNICATION_RESERVED_LEN 4
#define AT_TEST_COMMUNICATION_HEAD "The test mode."
typedef struct at_test_communication
{
    char head[AT_TEST_COMMUNICATION_HEAD_LEN]; 
    char command;
    char timeout;
    char mcs_num;
    short all_len;
    char *reserved;
}t_at_test_communication;

#define AT_TEST_COMMUNICATION_DATA_LEN 40
#define AT_TEST_TIMEROUT 200

#define AT_TEST_GET_RATE(TIME, NUM, LEN, CURRENT, AVER, MIN, MAX) \
{\
 CURRENT = LEN*1000  / TIME;\
 if (AVER == 0)\
 {\
    AVER = CURRENT;\
 }\
 else \
 {\
    AVER = (AVER*NUM + CURRENT)/(NUM +1); \
 }\
 if (MIN > CURRENT || MIN == 0)\
 {\
    MIN = CURRENT;\
 }\
 if (MAX < CURRENT || MAX == 0)\
 {\
    MAX = CURRENT;}\
}

#define AT_TEST_CALCUTLATE(RESULT, ALL, BASE) \
{\
    if (0 != ALL){\
    float get_result = ((float)BASE) / ((float)ALL);\
    get_result = get_result * 100.0;\
    RESULT = (int)get_result;\
    }\
    else{\
    RESULT = 0;\
    }\
}

extern at_server_t at_get_server(void);
extern char *parse(char *b, char *f, ...);
static u8_t at_test_mode_wiota_recv_fun( uc_recv_back_p recv_data);

static int wiota_state = AT_WIOTA_DEFAULT;
static t_at_test_data g_test_data = {0};
void at_wiota_set_state(int state)
{
    wiota_state = state;
}

int at_wiota_get_state(void)
{
    return wiota_state;
}

static rt_err_t get_char_timeout(rt_tick_t timeout, char *chr)
{
    at_server_t at_server = at_get_server();
    return at_server->get_char(at_server, chr, timeout);
}

static at_result_t at_wiota_version_query(void)
{
    u8_t version[15] = {0};
    u8_t git_info[36] = {0};
    u8_t time[36] = {0};
    u32_t cce_version = 0;

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

    WIOTA_MUST_INIT(wiota_state)

    args = parse((char *)(++args), "d", &freq);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_freq_info(freq);

    return AT_RESULT_OK;
}

static u32_t nth_power(u32_t num, u32_t n)
{
    u32_t s = 1;

    for (u32_t i = 0; i < n; i++)
    {
        s *= num;
    }
    return s;
}

static void convert_string_to_int(u8_t numLen, u8_t num, const u8_t *pStart, u8_t *array)
{
    u8_t *temp = NULL;
    u8_t len = 0;
    u8_t nth = numLen;

    temp = (u8_t *)rt_malloc(numLen);
    if (temp == NULL)
    {
        rt_kprintf("convert_string_to_int malloc failed\n");
        return;
    }

    for (len = 0; len < numLen; len++)
    {
        temp[len] = pStart[len] - '0';
        array[num] += nth_power(10, nth - 1) * temp[len];
        nth--;
    }
    rt_free(temp);
    temp = NULL;
}

static u8_t convert_string_to_array(u8_t *string, u8_t *array)
{
    u8_t *pStart = string;
    u8_t *pEnd = string;
    u8_t num = 0;
    u8_t numLen = 0;

    while (*pStart != '\0')
    {
        while (*pEnd != '\0')
        {
            if (*pEnd == ',')
            {
                convert_string_to_int(numLen, num, pStart, array);
                num++;
                pEnd++;
                pStart = pEnd;
                numLen = 0;
            }
            numLen++;
            pEnd++;
        }

        convert_string_to_int(numLen, num, pStart, array);
        num++;
        pStart = pEnd;
    }
    return num;
}

static at_result_t at_scan_freq_setup(const char *args)
{
    u8_t freqNum = 0;
    u32_t timeout = 0;
    u8_t *freqString = RT_NULL;
    u8_t *tempFreq = RT_NULL;
    uc_recv_back_t result;
    u8_t convertNum = 0;
    u8_t *freqArry = NULL;
    u32_t dataLen = 0;
    u32_t strLen = 0;
    at_result_t at_re = AT_RESULT_OK;

    WIOTA_CHECK_AUTOMATIC_MANAGER();

    if (wiota_state != AT_WIOTA_RUN)
    {
        return AT_RESULT_REPETITIVE_FAILE;
    }

    args = parse((char *)(++args), "ddd", &timeout, &dataLen, &freqNum);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    strLen = dataLen;

    if (freqNum > 0)
    {
        freqString = (u8_t *)rt_malloc(dataLen);
        if (freqString == RT_NULL)
        {
            at_server_printfln("SEND FAIL");
            return AT_RESULT_NULL;
        }
        tempFreq = freqString;
        at_server_printfln("OK");
        at_server_printf(">");
        while (dataLen)
        {
            if (get_char_timeout(rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT), (char *)tempFreq) != RT_EOK)
            {
                at_server_printfln("SEND FAIL");
                rt_free(tempFreq);
                return AT_RESULT_NULL;
            }
            dataLen--;
            tempFreq++;
        }

        freqArry = (u8_t *)rt_malloc(freqNum * sizeof(u8_t));
        if (freqArry == NULL)
        {
            at_server_printfln("rt_malloc freqArry failed!");
            rt_free(freqString);
            freqString = NULL;
            return AT_RESULT_NULL;
        }
        rt_memset(freqArry, 0, freqNum * sizeof(u8_t));

        freqString[strLen - 2] = '\0';

        convertNum = convert_string_to_array(freqString, freqArry);
        if (convertNum != freqNum)
        {
            at_server_printfln("converNum error!");
            rt_free(freqString);
            freqString = NULL;
            rt_free(freqArry);
            freqArry = NULL;
            return AT_RESULT_FAILE;
        }
        rt_free(freqString);
        freqString = NULL;

        uc_wiota_scan_freq(freqArry, freqNum, timeout, RT_NULL, &result);

        if (UC_OP_SUCC == result.result)
        {
            uc_freq_scan_result_p freqlinst = (uc_freq_scan_result_p)result.data;
            int freq_num = result.data_len / sizeof(uc_freq_scan_result_t);

            at_server_printfln("+WIOTASCANFREQ:");

            for (int i = 0; i < freq_num; i++)
            {
                at_server_printfln("%d,%d,%d,%d", freqlinst->freq_idx, freqlinst->rssi, freqlinst->snr, freqlinst->is_synced);
                freqlinst++;
            }

            rt_free(result.data);
        }
        else
        {
            at_re = AT_RESULT_NULL;
        }
    }

    rt_free(freqArry);
    freqArry = NULL;
    return at_re;
}

static at_result_t at_scan_freq_exec(void)
{
    at_result_t at_re = AT_RESULT_OK;
    uc_recv_back_t result;

    WIOTA_CHECK_AUTOMATIC_MANAGER();

    if (wiota_state != AT_WIOTA_RUN)
        return AT_RESULT_REPETITIVE_FAILE;

    uc_wiota_scan_freq(RT_NULL, 0, WIOTA_SCAN_FREQ_TIMEOUT, RT_NULL, &result);
    rt_kprintf("%s result %d\n", __FUNCTION__, result.result);
    if (UC_OP_SUCC == result.result)
    {
        uc_freq_scan_result_p freqlinst = (uc_freq_scan_result_p)result.data;
        int freq_num = result.data_len / sizeof(uc_freq_scan_result_t);

        at_server_printf("+WIOTASCANF:");

        while (freq_num--)
        {
            if (!freq_num)
                at_server_printfln("%d,%d,%d,%d", freqlinst->freq_idx, freqlinst->rssi, freqlinst->snr, freqlinst->is_synced);
            else
                at_server_printf("%d,%d,%d,%d,", freqlinst->freq_idx, freqlinst->rssi, freqlinst->snr, freqlinst->is_synced);
            freqlinst++;
        }

        rt_free(result.data);
    }
    else
        at_re = AT_RESULT_NULL;

    return at_re;

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
    rt_kprintf("dcxo=0x%x\n", dcxo);
    uc_wiota_set_dcxo(dcxo);

    return AT_RESULT_OK;
}

static at_result_t at_userid_query(void)
{
    unsigned int id[2] = {0};
    unsigned char len = 0;

    uc_wiota_get_userid(&(id[0]), &len);
    at_server_printfln("+WIOTAUSERID=0x%x", id[0]);

    return AT_RESULT_OK;
}

static at_result_t at_userid_setup(const char *args)
{
    unsigned int userid[2] = {0};
    
    WIOTA_CHECK_AUTOMATIC_MANAGER();

    WIOTA_MUST_INIT(wiota_state)

    args = parse((char *)(++args), "y", &userid[0]);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    rt_kprintf("userid:%x\n", userid[0]);

    uc_wiota_set_userid(userid, 4);

    return AT_RESULT_OK;
}

static at_result_t at_radio_query(void)
{
    rt_uint32_t temp = 0;
    radio_info_t radio;
    rt_device_t adc_dev;

    if (AT_WIOTA_RUN != wiota_state)
    {
        rt_kprintf("%s line %d wiota state error %d\n", __FUNCTION__, __LINE__, wiota_state);
        return AT_RESULT_FAILE;
    }

    adc_dev = rt_device_find(ADC_DEV_NAME);
    if (RT_NULL == adc_dev)
    {
        rt_kprintf("ad find %s fail\n", ADC_DEV_NAME);
    }

    temp = rt_adc_read((rt_adc_device_t)adc_dev, ADC_CONFIG_CHANNEL_CHIP_TEMP);

    uc_wiota_get_radio_info(&radio);
    //temp,rssi,ber,snr,cur_power,max_pow,cur_mcs,max_mcs
    at_server_printfln("+WIOTARADIO=%d,-%d,%d,%d,%d,%d,%d,%d",
                       temp, radio.rssi, radio.ber, radio.snr, radio.cur_power, 
                       radio.max_power, radio.cur_mcs, radio.max_mcs);

    return AT_RESULT_OK;
}

static at_result_t at_system_config_query(void)
{
    sub_system_config_t config;
    uc_wiota_get_system_config(&config);

    at_server_printfln("+WIOTASYSTEMCONFIG=%d,%d,%d,%d,%d,%d,%d,0x%x,0x%x",
                       config.id_len, config.symbol_length, config.dlul_ratio,
                       config.btvalue, config.group_number, config.ap_max_pow,
                       config.spectrum_idx, config.systemid, config.subsystemid);

    return AT_RESULT_OK;
}

static at_result_t at_system_config_setup(const char *args)
{
    sub_system_config_t config;
    unsigned int temp[7];
    
    WIOTA_CHECK_AUTOMATIC_MANAGER();

    WIOTA_MUST_INIT(wiota_state)

    args = parse((char *)(++args), "d,d,d,d,d,d,d,y,y",
                 &temp[0], &temp[1], &temp[2],
                 &temp[3], &temp[4], &temp[5],
                 &temp[6], &config.systemid, &config.subsystemid);

    config.id_len = (unsigned char)temp[0];
    config.symbol_length = (unsigned char)temp[1];
    config.dlul_ratio = (unsigned char)temp[2];
    config.btvalue = (unsigned char)temp[3];
    config.group_number = (unsigned char)temp[4];
    config.ap_max_pow = (char)(temp[5]-20);
    config.spectrum_idx = (unsigned char)temp[6];

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    // default config
    config.pn_num = 1;

    rt_kprintf("id_len=%d,symbol_len=%d,dlul=%d,bt=%d,group_num=%d,ap_max_pow=%d,spec_idx=%d,systemid=0x%x,subsystemid=0x%x\n",
               config.id_len, config.symbol_length, config.dlul_ratio,
               config.btvalue, config.group_number, config.ap_max_pow,
               config.spectrum_idx, config.systemid, config.subsystemid);

    uc_wiota_set_system_config(&config);

    return AT_RESULT_OK;
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


static u8_t at_test_mode_wiota_recv_fun( uc_recv_back_p recv_data)
{
    unsigned int send_data_address = 0;
    t_at_test_queue_data *queue_data = RT_NULL;
    uc_recv_back_p copy_recv_data = RT_NULL;
    rt_err_t re;
    
    t_at_test_communication *test_mode_data = (t_at_test_communication *)(recv_data->data);
    
    if (!(recv_data->data_len >= sizeof(t_at_test_communication) &&\
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

    memcpy(copy_recv_data, recv_data, sizeof(uc_recv_back_t));
    
    queue_data->type = AT_TEST_MODE_RECVDATA;
    queue_data->data = copy_recv_data;
    
    send_data_address = (unsigned int)queue_data;

    re = rt_mq_send(g_test_data.test_queue, &send_data_address, 4);
    //at_send_queue(g_test_data.test_queue, data,  2000);
    if (RT_EOK != re)
    {
         rt_kprintf("%s line %d rt_mq_send error %d\n", __FUNCTION__, __LINE__, re);
         rt_free(copy_recv_data);
         rt_free(queue_data);
         
         if (0 == recv_data->result)
            rt_free(recv_data->data);
    }
    
    return 1;
}

void wiota_recv_callback(uc_recv_back_p data)
{
    rt_kprintf("wiota_recv_callback result %d\n", data->result);

    if (0 == data->result)
    {
        if (/*g_test_data.time > 0 && */at_test_mode_wiota_recv_fun(data))
            return;        
        
        if (data->type < UC_RECV_SCAN_FREQ) {
            at_server_printf("+WIOTARECV,%d,%d,", data->type, data->data_len);
        } else if (data->type == UC_RECV_SYNC_LOST) {
            at_server_printf("+WIOTASYNC,LOST");
        }

        at_send_data(data->data, data->data_len);
        at_server_printfln("");

        rt_free(data->data);
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
    rt_kprintf("state = %d\n", state);

    if (1 == state && wiota_state == AT_WIOTA_INIT)
    {
        uc_wiota_run();
        uc_wiota_register_recv_data_callback(wiota_recv_callback,UC_CALLBACK_NORAMAL_MSG);
        uc_wiota_register_recv_data_callback(wiota_recv_callback,UC_CALLBACK_STATE_INFO);
        wiota_state = AT_WIOTA_RUN;
    }
    else if (0 == state && wiota_state == AT_WIOTA_RUN)
    {
        uc_wiota_exit();
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

    rt_kprintf("state = %d, timeout=%d\n", state, timeout);

    if (wiota_state != AT_WIOTA_RUN)
        return AT_RESULT_REPETITIVE_FAILE;

    rt_kprintf("state = %d, timeout=%d\n", state, timeout);

    if (timeout)
        uc_wiota_set_active_time((unsigned int)timeout);

    if (state)
    {
        uc_wiota_connect();
    }
    else
    {
        uc_wiota_disconnect();
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiotasend_exec(void)
{
    uint8_t *sendbuffer = NULL;
    uint8_t *psendbuffer;
    rt_err_t result = RT_EOK;
    int length = 0;

    if (AT_WIOTA_RUN != wiota_state)
    {
        return AT_RESULT_FAILE;
    }

    sendbuffer = (uint8_t *)rt_malloc(WIOTA_SEND_DATA_MUX_LEN + CRC16_LEN); // reserve CRC16_LEN for low mac
    if (sendbuffer == RT_NULL)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    at_server_printf("\r\n>");
    //while(1)
    {
        psendbuffer = sendbuffer;
        length = WIOTA_SEND_DATA_MUX_LEN;

        while (length)
        {
            result = get_char_timeout(rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT), (char *)psendbuffer);
            if (result != RT_EOK)
            {
                break;
            }
            length--;
            psendbuffer++;
            //rt_kprintf("length=%d,psendbuffer=0x%x,psendbuffer=%c , 0x%x\n", length, psendbuffer, *psendbuffer, *psendbuffer);
            if (WIOTA_DATA_END == *psendbuffer)
            {
                break;
            }
        }
        if ((psendbuffer - sendbuffer) > 0)
        {
            //rt_kprintf("len=%d, sendbuffer=%s\n", psendbuffer - sendbuffer, sendbuffer);
            if (UC_OP_SUCC != uc_wiota_send_data(sendbuffer, psendbuffer - sendbuffer, WIOTA_SEND_TIMEOUT, RT_NULL))
            {
                rt_free(sendbuffer);
                sendbuffer = RT_NULL;
                at_server_printfln("SEND FAIL");
                return AT_RESULT_FAILE;
            }
        }
    }
    at_server_printfln("SEND OK");
    rt_free(sendbuffer);
    sendbuffer = RT_NULL;
    return AT_RESULT_OK;
}

static at_result_t at_wiotasend_setup(const char *args)
{
    int length = 0, timeout = 0;
    unsigned char *sendbuffer = NULL;
    unsigned char *psendbuffer;

    if (AT_WIOTA_RUN != wiota_state)
    {
        return AT_RESULT_FAILE;
    }

    args = parse((char *)(++args), "d,d", &timeout, &length);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    rt_kprintf("timeout=%d, length=%d\n", timeout, length);

    if (wiota_state != AT_WIOTA_RUN)
        return AT_RESULT_REPETITIVE_FAILE;

    if (length > 0)
    {
        sendbuffer = (unsigned char *)rt_malloc(length + CRC16_LEN); // reserve CRC16_LEN for low mac
        if (sendbuffer == NULL)
        {
            at_server_printfln("SEND FAIL");
            return AT_RESULT_NULL;
        }
        psendbuffer = sendbuffer;
        //at_server_printfln("SUCC");
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
            psendbuffer++;
        }

        if (UC_OP_SUCC == uc_wiota_send_data(sendbuffer, psendbuffer - sendbuffer, timeout > 0 ? timeout : WIOTA_SEND_TIMEOUT, RT_NULL))
        {
            rt_free(sendbuffer);
            sendbuffer = NULL;

            at_server_printfln("SEND SUCC");
            return AT_RESULT_OK;
        }
        else
        {
            rt_free(sendbuffer);
            sendbuffer = NULL;
            at_server_printfln("SEND FAIL");
            return AT_RESULT_NULL;
        }
    }
    return AT_RESULT_OK;
}

static at_result_t at_wiotatrans_process(u16_t timeout, char *strEnd)
{
    uint8_t *pBuff = RT_NULL;
    int result = 0;
    timeout = (timeout == 0) ? WIOTA_SEND_TIMEOUT : timeout;
    if ((RT_NULL == strEnd) || ('\0' == strEnd[0]))
    {
        strEnd = WIOTA_TRANS_END_STRING;
    }
    uint8_t nLenEnd = strlen(strEnd);
//    uint8_t nStrEndCount = 0;
    int16_t nSeekRx = 0;
    char nRun = 1;
//    char nCatchEnd = 0;
    char nSendFlag = 0;
    
    pBuff = (uint8_t *)rt_malloc(WIOTA_TRANS_BUFF);
    if (pBuff == RT_NULL)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    memset(pBuff, 0, WIOTA_TRANS_BUFF);
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
            do {
                // discard any characters after the end string
                result = get_char_timeout(rt_tick_from_millisecond(200), (char *)&pBuff[0]);
            } while (RT_EOK == result);
            nSendFlag = 0;
            nSeekRx = 0;
            nRun = 1;
            memset(pBuff, 0, WIOTA_TRANS_BUFF);
            continue;
        }
        
        if (nSendFlag)
        {
            nSeekRx = (nSeekRx > WIOTA_TRANS_MAX_LEN) ? WIOTA_TRANS_MAX_LEN : nSeekRx;
            if (nSeekRx > 0)
            {
                if (UC_OP_SUCC == uc_wiota_send_data(pBuff, nSeekRx, timeout, RT_NULL))
                {
                    at_server_printfln("SEND SUCC");
                }
                else
                {
                    at_server_printfln("SEND FAIL");
                }
            }
            nSeekRx = 0;
            nSendFlag = 0;
            memset(pBuff, 0, WIOTA_TRANS_BUFF);
        }
    }
    
    do {
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
    char strEnd[WIOTA_TRANS_END_STRING_MAX] = { 0 };
    
    args = parse((char *)(++args), "d,s", &timeout, (sl32_t)8, strEnd);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    if ('\r' == strEnd[0])
    {
        strEnd[0] = '\0';
    }
    else
    {
        int nLen = strlen(strEnd);
        if (nLen > 2)
        {
            strEnd[nLen - 2] = '\0';
        }
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

    rt_kprintf("timeout = %d\n", timeout);

    uc_wiota_recv_data(&result, timeout, RT_NULL);
    if (!result.result)
    {
        if (result.type < UC_RECV_SCAN_FREQ) {
            at_server_printf("+WIOTARECV,%d,%d,", result.type, result.data_len);
        } else if (result.type == UC_RECV_SYNC_LOST) {
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
        if (result.type < UC_RECV_SCAN_FREQ) {
            at_server_printf("+WIOTARECV,%d,%d,", result.type, result.data_len);
        } else if (result.type == UC_RECV_SYNC_LOST) {
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
    int mode = 0, state = 0;
    
    WIOTA_CHECK_AUTOMATIC_MANAGER();
    
    args = parse((char *)(++args), "dd", &mode, &state);

    switch (mode)
    {
    case AT_WIOTA_SLEEP:
    {
        at_server_printfln("OK");

        while (1)
            ;
    }
    case AT_WIOTA_GATING:
    {
        uc_wiota_set_is_gating(state);
        break;
    }
    default:
        return AT_RESULT_FAILE;
    }
    return AT_RESULT_OK;
}

static at_result_t at_wiotarate_setup(const char *args)
{
    int rate_mode = 0xFF;
    int rate_value = 0xFF;

    args = parse((char *)(++args), "dd", &rate_mode,&rate_value);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    at_server_printfln("uc_wiota_set_data_rate: %d, %d",(unsigned char)rate_mode,(unsigned short)rate_value);
    uc_wiota_set_data_rate((unsigned char)rate_mode,(unsigned short)rate_value);

    return AT_RESULT_OK;
}

static at_result_t at_wiotapow_setup(const char *args)
{
    int mode = 0;
    int power = 0x7F;

    WIOTA_CHECK_AUTOMATIC_MANAGER();

    args = parse((char *)(++args), "dd", &mode, &power);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    // at can't parse minus value for now
    if (mode == 0)
    {
        uc_wiota_set_cur_power((signed char)(power-20));
    }
    else if (mode == 1)
    {
        uc_wiota_set_max_power((signed char)(power-20));
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
        boot_set_uart0_baud_rate(BAUD_RATE_460800);
    }
    else if (1 == uart_number)
    {
        config.baud_rate = BAUD_RATE_115200;
        rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
        boot_set_uart0_baud_rate(BAUD_RATE_115200);
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

    args = parse((char *)(++args), "dd", &mode, &type);
    if (!args)
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
    at_server_printfln("+WIOTACRC=%d",uc_wiota_get_crc());

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

static void at_test_get_wiota_inof(t_at_test_statistical_data* info, int i_time)
{
    uc_throughput_info_t throughput_info;
    uc_stats_info_t stats_info_ptr;
    radio_info_t radio;
    
    uc_wiota_get_throughput(&throughput_info);
    uc_wiota_reset_throughput(UC_STATS_TYPE_ALL);
    
    rt_kprintf("throughput_info.ul_succ_data_len = %d\n", throughput_info.ul_succ_data_len);
    AT_TEST_GET_RATE(i_time, g_test_data.num, throughput_info.ul_succ_data_len, \
        info->upcurrentrate, info->upaverate, info->upminirate, info->upmaxrate);
    rt_kprintf("upcurren %d  upave %d min %d max %d\n",  info->upcurrentrate, info->upaverate, info->upminirate, info->upmaxrate);

    
    rt_kprintf("throughput_info.dl_succ_data_len = %d\n", throughput_info.dl_succ_data_len);
    AT_TEST_GET_RATE(i_time, g_test_data.num, throughput_info.dl_succ_data_len, \
        info->downcurrentrate, info->downaverate, info->downminirate, info->downmaxrate);
    g_test_data.num++;
    uc_wiota_get_all_stats(&stats_info_ptr);
    uc_wiota_reset_stats(UC_STATS_TYPE_ALL);

    rt_kprintf("rach_fail=%d active_fail=%d ul_succ=%d\n", stats_info_ptr.rach_fail, stats_info_ptr.active_fail, stats_info_ptr.ul_succ);
    AT_TEST_CALCUTLATE(info->send_fail, \
        stats_info_ptr.rach_fail + stats_info_ptr.active_fail + stats_info_ptr.ul_succ,\
        stats_info_ptr.rach_fail + stats_info_ptr.active_fail\
        );
    
    rt_kprintf("dl_fail=%d dl_succ=%d\n", stats_info_ptr.dl_fail, stats_info_ptr.dl_succ);
    AT_TEST_CALCUTLATE(info->recv_fail, \
        stats_info_ptr.dl_fail + stats_info_ptr.dl_succ,\
        stats_info_ptr.dl_fail\
        );

    uc_wiota_get_radio_info(&radio);
    info->max_mcs = radio.max_mcs;
    info->msc = radio.cur_mcs;
    info->power = radio.cur_power;
    info->rssi = radio.rssi;
    info->snr = radio.snr;
}

static int at_test_mcs_rate(int mcs)
{
    sub_system_config_t config;
    uc_wiota_get_system_config(&config);
    int groupNum = (1<<(config.group_number*(config.dlul_ratio+1))) + (1<<config.group_number);
    int symbolNum = 11+2*(1<<config.pn_num)+64*groupNum;
    int frameLen = symbolNum*4*128*(1<<config.symbol_length)/1000;
    int result = 8*symLen_mcs_byte[config.symbol_length][mcs]*1000/frameLen;
    return result;
}

static void at_test_report_to_uart(void)
{
    at_test_get_wiota_inof(&g_test_data.statistical, g_test_data.time);
    int rate_mcs = at_test_mcs_rate(g_test_data.statistical.max_mcs);
    switch(g_test_data.type)
    {
        
        case AT_TEST_COMMAND_UP_TEST:
                at_server_printfln("+UP: %dbps, %dbps, %d, %ddB",\
                g_test_data.statistical.upcurrentrate/1000*8, rate_mcs,\
                g_test_data.statistical.msc,g_test_data.statistical.rssi,g_test_data.statistical.snr);
                break;
        case AT_TEST_COMMAND_DOWN_TEST:
                at_server_printfln("+DOWN: %dbps, %dbps, %d, %ddB",\
                g_test_data.statistical.downcurrentrate/1000*8, rate_mcs,\
                g_test_data.statistical.msc,g_test_data.statistical.rssi,g_test_data.statistical.snr);
                break;
        case AT_TEST_COMMAND_LOOP_TEST: 
                at_server_printfln("+LOOP: %dbps, %dbps, %dbps, %d, %ddB",\
                g_test_data.statistical.upcurrentrate/1000*8,\
                g_test_data.statistical.downcurrentrate/1000*8, rate_mcs,\
                g_test_data.statistical.msc,g_test_data.statistical.rssi,g_test_data.statistical.snr);
                break;
        case AT_TEST_COMMAND_DATA_MODE:
                at_server_printfln("+DATA: %dbps, %dbps, %d, %ddB",\
                g_test_data.statistical.downcurrentrate/1000*8, rate_mcs,\
                g_test_data.statistical.msc,g_test_data.statistical.rssi,g_test_data.statistical.snr);
                break;
        default:
                break;
    }
}

static void at_test_mode_time_fun(void* parameter) 
{
    at_test_report_to_uart();
}

static at_result_t at_wiotatest_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_test_loop((unsigned char)mode);

    return AT_RESULT_OK;
}

static void at_test_mode_task_fun(void* parameter)
{
    t_at_test_data *test_data = &g_test_data;
    t_at_test_queue_data *recv_queue_data = RT_NULL;
    unsigned int queue_data_on = 0;
    int send_flag = 0;
    t_at_test_communication *communication = RT_NULL;

    test_data->test_data_len = sizeof(t_at_test_communication);

    communication= rt_malloc(test_data->test_data_len + 4);
    if (RT_NULL == communication)
    {
        rt_kprintf("at_test_mode_task_fun rt_malloc error\n");
        return ;
    }
    memset(communication, 0, sizeof(t_at_test_communication));
    memcpy(communication->head, AT_TEST_COMMUNICATION_HEAD, strlen(AT_TEST_COMMUNICATION_HEAD));
    communication->command = AT_TEST_COMMAND_DEFAULT;
    communication->timeout = 0;
    communication->all_len = test_data->test_data_len;
    test_data->test_mode_timer = RT_NULL;
    int data_test_flag = 1;
    while(1)
    {
        // recv queue data. wait start test 
        if(RT_EOK == rt_mq_recv(test_data->test_queue, &queue_data_on, 4, 100)) // RT_WAITING_NO
        {   
            recv_queue_data = (t_at_test_queue_data*)queue_data_on;
            rt_kprintf("data->type = %d\n", recv_queue_data->type);
            
            switch((int)recv_queue_data->type)
            {
                case AT_TEST_MODE_RECVDATA:
                {
                    uc_recv_back_p recv_data = recv_queue_data->data;
                    t_at_test_communication *communication_data = (t_at_test_communication *)(recv_data->data);
                    //pasre data
                    test_data->time = communication_data->timeout;
                    test_data->type = communication_data->command;
                   
                    if ( test_data->test_data_len != communication_data->all_len)
                    {
                        rt_kprintf("test_data->test_data_len %d != communication_data->all_len %d\n", test_data->test_data_len, communication_data->all_len);
                        rt_free(communication);
                         test_data->test_data_len = communication_data->all_len;
                         // re malloc
                        communication= rt_malloc(test_data->test_data_len + 4);
                        if (RT_NULL == communication)
                        {
                            rt_kprintf("at_test_mode_task_fun line %d rt_malloc error\n", __LINE__);
                            return ;
                        }
                        memset(communication, 0, communication_data->all_len);
                        memcpy(communication->head, AT_TEST_COMMUNICATION_HEAD, strlen(AT_TEST_COMMUNICATION_HEAD));
                        communication->command = test_data->type;
                        communication->timeout = 0;
                        communication->all_len = test_data->test_data_len;
                    }
                    
                    rt_kprintf("command %d time %d data_len %d\n", test_data->type,  test_data->time, test_data->test_data_len);
                    
                    if (RT_NULL == test_data->test_mode_timer &&  AT_TEST_COMMAND_DATA_MODE!=test_data->type)
                    {
                        test_data->test_mode_timer = rt_timer_create("teMode", \
                                                                    at_test_mode_time_fun, \
                                                                    RT_NULL, \
                                                                    test_data->time*1000, \
                                                                    RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER);
                        if (RT_NULL == test_data->test_mode_timer)
                        {
                            rt_kprintf("%s line %d rt_timer_create error\n", __FUNCTION__, __LINE__);
                            return;
                        }
                        rt_timer_start(test_data->test_mode_timer);
                    }
                    // loop data to ap
                    if (AT_TEST_COMMAND_LOOP_TEST == communication_data->command)
                    {
                        memcpy(communication, communication_data, communication_data->all_len);
                        send_flag = 0;
                    }
                    //free data
                    rt_free(communication_data);
                    rt_free(recv_data);
                    rt_free(recv_queue_data);
                    
                    if (AT_TEST_COMMAND_DATA_MODE == test_data->type && 1 == data_test_flag)
                    {
                        uc_wiota_set_data_rate(0,communication_data->mcs_num);
                        uc_wiota_test_loop(1);
                        data_test_flag = 0;
                    } 
                    break;
                }
                case AT_TEST_MODE_QUEUE_EXIT:
                {
                    if (4 == g_test_data.type)
                    {
                        uc_wiota_set_data_rate(0,8);
                        uc_wiota_test_loop(0);
                    }
                    else 
                    {
                        communication->command = AT_TEST_COMMAND_DATA_MODE;
                        uc_wiota_send_data((unsigned char*)communication, communication->all_len, 20000, RT_NULL);
                    }
                    
                    rt_free(recv_queue_data);
                    rt_free(communication);
                    rt_sem_release(test_data->test_sem);
                    return;
                }
            }        
        }
//        rt_kprintf("test data len %d heap size %d\n", communication->all_len, uc_heap_size());
        rt_kprintf("test data len %d\n", communication->all_len);
        
        if (test_data->type  == AT_TEST_COMMAND_UP_TEST || send_flag == 0)
        {
            UC_OP_RESULT res;
            res = uc_wiota_send_data((unsigned char*)communication, communication->all_len, 20000, RT_NULL);
//            rt_kprintf("test send data result = %d, type = %d heap size %d\n", res, test_data->type, uc_heap_size());
            rt_kprintf("test send data result = %d, type = %d\n", res, test_data->type);
            if (res == UC_OP_SUCC)
            {
                send_flag = 1;
            }
        }
    }
}


static at_result_t at_test_mode_start_exec(void)
{    
    if (wiota_state != AT_WIOTA_RUN)
        return AT_RESULT_REPETITIVE_FAILE;
    
    if (g_test_data.time > 0)
    {
        rt_kprintf("%s line %d repeated use\n", __FUNCTION__, __LINE__);
        return AT_RESULT_PARSE_FAILE;
    }
    g_test_data.time = 1;
    //create queue
    g_test_data.test_queue = rt_mq_create("teMode", 4, 8, RT_IPC_FLAG_PRIO);
    if (RT_NULL == g_test_data.test_queue)
    {
        rt_kprintf("%s line %d at_create_queue error\n", __FUNCTION__, __LINE__);
        return AT_RESULT_PARSE_FAILE;
    }

    g_test_data.test_sem = rt_sem_create("teMode", 0, RT_IPC_FLAG_PRIO);
    if (RT_NULL == g_test_data.test_sem)
    {
        rt_kprintf("%s line %d rt_sem_create error\n", __FUNCTION__, __LINE__);
        return AT_RESULT_PARSE_FAILE;
    }
    
    g_test_data.test_mode_task = rt_thread_create("teMode",\
                                  at_test_mode_task_fun,\
                                  RT_NULL,\
                                  2048,\
                                  RT_THREAD_PRIORITY_MAX / 3 - 1,\
                                  3);
    if (RT_NULL == g_test_data.test_mode_task)
    {
        rt_kprintf("%s line %d rt_thread_create error\n", __FUNCTION__, __LINE__);
        return AT_RESULT_PARSE_FAILE;
    }
    rt_thread_startup(g_test_data.test_mode_task);
    return AT_RESULT_OK;
}

static at_result_t at_test_mode_stop_exec(void)
{

    unsigned int send_data_address;
    
    if (g_test_data.time < 1)
    {
        rt_kprintf("%s line %d no run\n", __FUNCTION__, __LINE__);
        return AT_RESULT_PARSE_FAILE;
    }
    
    g_test_data.time = 0;
    
    
        
    t_at_test_queue_data *data = rt_malloc(sizeof(t_at_test_queue_data));
    
    if (RT_NULL == data)
    {
        rt_kprintf("%s line %d rt_malloc error\n", __FUNCTION__, __LINE__);
        return AT_RESULT_PARSE_FAILE;
    }
    data->type = AT_TEST_MODE_QUEUE_EXIT;
    send_data_address = (unsigned int)data;

    rt_err_t res = rt_mq_send_wait(g_test_data.test_queue, &send_data_address, 4, 1000);
    if (0 != res)
    {   
        rt_kprintf("%s line %d rt_mq_send_wait error\n", __FUNCTION__, __LINE__);
        rt_free(data);
        return AT_RESULT_PARSE_FAILE;
    }

        
    if (RT_NULL != g_test_data.test_mode_timer)
    {
        rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
        rt_timer_stop(g_test_data.test_mode_timer);
    }
    
    //wait  RT_WAITING_FOREVER
    if (RT_EOK != rt_sem_take(g_test_data.test_sem, RT_WAITING_FOREVER))
    {
        rt_kprintf("%s line %d rt_sem_take error\n", __FUNCTION__, __LINE__);
        return AT_RESULT_PARSE_FAILE;
    }
    
    if (RT_NULL != g_test_data.test_mode_timer)
    {
        rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
        rt_timer_delete(g_test_data.test_mode_timer);
    }

    rt_mq_delete(g_test_data.test_queue);
    rt_sem_delete(g_test_data.test_sem);    
    rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
    rt_thread_delete(g_test_data.test_mode_task);
    memset(&g_test_data, 0, sizeof(g_test_data));
    return AT_RESULT_OK;
    
}

static at_result_t at_wiotaosc_query(void)
{
    at_server_printfln("+WIOTAOSC=%d",uc_wiota_get_is_osc());

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

static at_result_t at_wiotatestlpm_setup(const char *args)
{
    int mode = 0;
    int value = 0;

    args = parse((char *)(++args), "dd", &mode, &value);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_test_lpm((unsigned char)mode,(unsigned char)value);

    return AT_RESULT_OK;
}

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

static at_result_t at_wiota_save_static_exec(void)
{
    WIOTA_CHECK_AUTOMATIC_MANAGER();
    if (AT_WIOTA_RUN != wiota_state)
    {
        uc_wiota_save_static();
        return AT_RESULT_OK;
    }
    return AT_RESULT_FAILE;
}


AT_CMD_EXPORT("AT+WIOTAVERSION", RT_NULL, RT_NULL, at_wiota_version_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAINIT", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_init_exec);
AT_CMD_EXPORT("AT+WIOTALPM", "=<mode>,<state>", RT_NULL, RT_NULL, at_wiotalpm_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARATE", "=<rate_mode>,<rate_value>", RT_NULL, RT_NULL, at_wiotarate_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAPOW", "=<mode>,<power>", RT_NULL, RT_NULL, at_wiotapow_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASCANFREQ", "=<timeout>,<dataLen>,<freqnum>", RT_NULL, RT_NULL, at_scan_freq_setup, at_scan_freq_exec);
AT_CMD_EXPORT("AT+WIOTAFREQ", "=<freqpint>", RT_NULL, at_freq_query, at_freq_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTADCXO", "=<dcxo>", RT_NULL, RT_NULL, at_dcxo_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAUSERID", "=<id0>", RT_NULL, at_userid_query, at_userid_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARADIO", "=<temp>,<rssi>,<ber>,<snr>,<cur_pow>,<max_pow>,<cur_mcs>", RT_NULL, at_radio_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTACONFIG", "=<id_len>,<symbol>,<dlul>,<bt>,<group_num>,<ap_max_pow>,<spec_idx>,<systemid>,<subsystemid>", 
              RT_NULL, at_system_config_query, at_system_config_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARUN", "=<state>", RT_NULL, RT_NULL, at_wiota_cfun_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTACONNECT", "=<state>,<activetime>", RT_NULL, at_connect_query, at_connect_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASEND", "=<timeout>,<len>", RT_NULL, RT_NULL, at_wiotasend_setup, at_wiotasend_exec);
AT_CMD_EXPORT("AT+WIOTATRANS", "=<timeout>,<end>", RT_NULL, RT_NULL, at_wiotatrans_setup, at_wiotatrans_exec);
AT_CMD_EXPORT("AT+WIOTARECV", "=<timeout>", RT_NULL, RT_NULL, at_wiotarecv_setup, at_wiota_recv_exec);
AT_CMD_EXPORT("AT+WIOTALOG", "=<mode>", RT_NULL, RT_NULL, at_wiotalog_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASTATS", "=<mode>,<type>", RT_NULL, at_wiotastats_query, at_wiotastats_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTACRC", "=<crc_limit>", RT_NULL, at_wiotacrc_query, at_wiotacrc_setup, RT_NULL);
AT_CMD_EXPORT("AT+THROUGHTSTART", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_test_mode_start_exec);
AT_CMD_EXPORT("AT+THROUGHTSTOP", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_test_mode_stop_exec);
AT_CMD_EXPORT("AT+WIOTAOSC", "=<mode>", RT_NULL, at_wiotaosc_query, at_wiotaosc_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTALIGHT", "=<mode>", RT_NULL, RT_NULL, at_wiotalight_setup, RT_NULL);

// below is for inter test !
AT_CMD_EXPORT("AT+WIOTATEST", "=<mode>", RT_NULL, RT_NULL, at_wiotatest_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTATESTLPM", "=<mode>,<value>", RT_NULL, RT_NULL, at_wiotatestlpm_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASAVESTATIC", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_save_static_exec);

#endif

//#endif
