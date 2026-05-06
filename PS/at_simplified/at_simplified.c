#include <rtthread.h>
#include <rtdevice.h>
#include <at.h>
#include "uc_uart.h"
#include "uc_gpio.h"
#include "ati_prs.h"
#include "fastlz.h"
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "uc_wiota_gateway_api.h"
#include "uc_ota_flash.h"

typedef enum
{
    SYMBOL_LENGTH_128 = 0,
    SYMBOL_LENGTH_256,
    SYMBOL_LENGTH_512,
    SYMBOL_LENGTH_1024,
} symbol_length_e;

typedef enum
{
    DEEP_SLEEP_MODE = 0, // 深度睡眠
    ALARM_WAKEUP_MODE,   // 定时唤醒
    RF_WAKEUP_MODE,      // 射频唤醒
    SLEEP_MODE_MAX,
} sleep_mode_e;

typedef enum
{
    RATE_MODE_1 = 1, // 128+mcs3, ul 6.23kbps, dl 3.12kbps
    RATE_MODE_2,     // 128+mcs2, ul 5.33kbps, dl 2.66kbps
    RATE_MODE_3,     // 256+mcs3, ul 2.66kbps, dl 1.33kbps
    RATE_MODE_4,     // 256+mcs2, ul 1.32kbps, dl 0.66kbps
    RATE_MODE_5,     // 256+mcs1, ul 0.92kbps, dl 0.46kbps
    RATE_MODE_6,     // 256+mcs0, ul 0.42kbps, dl 0.21kbps
    RATE_MODE_7,     // 512+mcs0, ul 0.21kbps, dl 0.10kbps
    RATE_MODE_8,     // 1024+mcs0, ul 0.10kbps, dl 0.05kbps
    RATE_MODE_MAX,
} rate_mode_e;

typedef struct
{
    unsigned char symbol_length;
    unsigned char dlul_ratio;
    unsigned char mcs;
} rate_mode_t;

typedef struct
{
    unsigned char is_compress : 1; // 是否压缩
    unsigned char reserved : 7;    // 保留位
} app_header_t;

typedef struct
{
    unsigned int addr;                            // 终端地址(user id)，支持热配置
    unsigned int subsystemid;                     // 终端子系统id，支持热配置
    unsigned int base_freq;                       // 频谱基础频率
    unsigned int cur_freq;                        // 当前频率列表，支持热配置
    unsigned char spectrum_idx;                   // 频谱索引，不支持热配置
    unsigned char power;                          // 功率，支持热配置
    unsigned char rate_mode;                      // rate_mode_e，其中的symbol_length和dlul_ratio不支持热配置，mcs支持热配置
    rate_mode_t rate_mode_map[RATE_MODE_MAX - 1]; // 速率模式映射表
} at_simplified_t;

#define AT_SEND_TIMEOUT 60000 // ms
#define MIN_SLEEP_TIME 100    // ms
#define MAX_SLEEP_TIME 44000  // ms
#define SERIAL_NUM_LEN 20
#define SOFTWARE_VER_LEN 15

#define USERID_LENGTH 4               // 用户ID长度
#define MAX_SUBSYSTEMID 16777215      // 同步字最大值
#define DEFAULT_SUBSYSTEMID 354689    // 默认子系统id
#define DEFAULE_FREQ 502000000        // 默认频率502000000Hz
#define DEFAULE_BW_HZ 200000          // 默认带宽200000Hz
#define DEFAULE_POWER 20              // 默认功率20dbm
#define POWER_MIN 0                   // 最小功率，单位dBm
#define POWER_MAX 22                  // 最大功率，单位dBm
#define DEFAULE_RATE_MODE RATE_MODE_4 // 256+mcs2, ul 1.32kbps, dl 0.66
#define INDEX_TO_FREQ(index, base_freq, bw) ((index) * bw + base_freq)
#define FREQ_TO_INDEX(freq, base_freq, bw) ((freq - base_freq) / bw)

#define RUN_STATE_PIN GPIO_PIN_15
#define TX_STATE_PIN GPIO_PIN_17
#define RX_STATE_PIN GPIO_PIN_16

#define INVALID_FREQ 0xFF
#define FREQ_LIST_NUM 16

#define RATE_MODE_MAP                            \
    {                                            \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_128,  \
            .dlul_ratio = 1,                     \
            .mcs = UC_MCS_LEVEL_3,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_128,  \
            .dlul_ratio = 1,                     \
            .mcs = UC_MCS_LEVEL_2,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_256,  \
            .dlul_ratio = 1,                     \
            .mcs = UC_MCS_LEVEL_3,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_256,  \
            .dlul_ratio = 1,                     \
            .mcs = UC_MCS_LEVEL_2,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_256,  \
            .dlul_ratio = 1,                     \
            .mcs = UC_MCS_LEVEL_1,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_256,  \
            .dlul_ratio = 1,                     \
            .mcs = UC_MCS_LEVEL_0,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_512,  \
            .dlul_ratio = 1,                     \
            .mcs = UC_MCS_LEVEL_0,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_1024, \
            .dlul_ratio = 1,                     \
            .mcs = UC_MCS_LEVEL_0,               \
        },                                       \
    };

#define BASE_FREQ 223000000
#define FREQ_SPECTRUM_COUNT 8 // 频率频谱总数

const unsigned int FREQ_RANGES[FREQ_SPECTRUM_COUNT][2] = {
    {BASE_FREQ, 235000000}, // 223MHz~235MHz，sepctrum_idx 0
    {400000000, 440000000}, // 400MHz~480MHz，sepctrum_idx 1
    {433050000, 434790000}, // 433.05MHz~434.79MHz，sepctrum_idx 2
    {470000000, 510000000}, // 470MHz~510MHz，sepctrum_idx 3
    {779000000, 787000000}, // 779MHz~787MHz，sepctrum_idx 4
    {840000000, 845000000}, // 840MHz~845MHz，sepctrum_idx 5
    {863000000, 870000000}, // 863MHz~870MHz，sepctrum_idx 6
    {902000000, 928000000}, // 902MHz~928MHz，sepctrum_idx 7
};

// 各频谱下的最大频率数
static const unsigned char g_max_freq_num_table[FREQ_SPECTRUM_COUNT] = {60, 200, 8, 200, 40, 25, 35, 130};

// 各符号长度对应的最大唤醒ID
static const unsigned short g_max_awaken_id_table[SYMBOL_LENGTH_1024 + 1] = {1023, 4095, 16383, 65535};

extern at_server_t at_get_server(void);
static at_simplified_t at_simplified_local = {0};

// GPIO引脚定义表
static const unsigned char g_state_pins[] = {RUN_STATE_PIN, TX_STATE_PIN, RX_STATE_PIN};
#define STATE_PINS_COUNT (sizeof(g_state_pins) / sizeof(g_state_pins[0]))

// 常量定义
#define DECOMPRESS_MAX_LEN 1024
#define COMPRESS_BUFFER_FACTOR 2 // 压缩缓冲区大小倍数
#define RECV_DATA_TIMEOUT 10000  // 接收数据超时时间，单位ms
#define PIN_INIT_MDELAY 1000     // 引脚初始化延迟
#define RX_COMPLETE_MDELAY 1     // RX完成延迟，单位ms

// 网关模式相关
#define AUTH_CODE_LEN 18
#define OTA_READ_LEN 512
#define MD5_SIZE 36
static unsigned int ota_start_addr = 0;
static unsigned int ota_file_size = 0;
static char ota_file_md5[MD5_SIZE] = {0};
static unsigned char is_gateway_mode = 0;
static unsigned char gw_mode = 0;
static char gw_auth_code[AUTH_CODE_LEN] = {0};
static int at_gateway_mode_enter(unsigned char mode, char *auth_code);

static void at_state_pin_init(void)
{
    for (int i = 0; i < STATE_PINS_COUNT; i++)
    {
        gpio_set_pin_mux(UC_GPIO_CFG, g_state_pins[i], GPIO_FUNC_0);
        rt_pin_mode(g_state_pins[i], PIN_MODE_OUTPUT);
        rt_pin_write(g_state_pins[i], PIN_LOW);
    }
}

void at_simplified_running(void)
{
    rt_pin_write(RUN_STATE_PIN, PIN_HIGH);
    rt_thread_mdelay(PIN_INIT_MDELAY);
    rt_pin_write(RUN_STATE_PIN, PIN_LOW);
    rt_thread_mdelay(PIN_INIT_MDELAY);
}

static inline void at_tx_start(void)
{
    rt_pin_write(TX_STATE_PIN, PIN_HIGH);
}

static inline void at_tx_end(void)
{
    rt_pin_write(TX_STATE_PIN, PIN_LOW);
}

static inline void at_rx_notity(void)
{
    rt_pin_write(RX_STATE_PIN, PIN_HIGH);
}

static inline void at_rx_complete(void)
{
    rt_thread_mdelay(RX_COMPLETE_MDELAY);
    rt_pin_write(RX_STATE_PIN, PIN_LOW);
}
static inline int is_space(char c)
{
    // 匹配空格、制表符、换行符等空白字符，核心只处理空格即可
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f');
}

static inline int str_to_int(const char *str, int len)
{
    int num = 0;
    for (int i = 0; i < len; i++)
    {
        if (str[i] < '0' || str[i] > '9')
        {
            return -1;
        }
        num = num * 10 + (str[i] - '0');
    }
    return num;
}

static int str_to_int_array(const char *input_str, unsigned int *out_array, int *out_count)
{
    if (input_str == NULL || out_array == NULL || out_count == NULL)
    {
        return -1;
    }
    *out_count = 0;

    int input_len = rt_strlen(input_str);
    int start = 0, end = input_len - 1;
    while (start <= end && is_space(input_str[start]))
    {
        start++;
    }
    while (end >= start && is_space(input_str[end]))
    {
        end--;
    }
    if (start > end)
    {
        return -1;
    }

    int num_start = start;
    for (int i = start; i <= end + 1; i++)
    {
        if (i == end + 1 || input_str[i] == ',')
        {
            int seg_start = num_start;
            int seg_end = i - 1;
            while (seg_start <= seg_end && is_space(input_str[seg_start]))
            {
                seg_start++;
            }
            while (seg_end >= seg_start && is_space(input_str[seg_end]))
            {
                seg_end--;
            }
            if (seg_start > seg_end)
            {
                num_start = i + 1;
                continue;
            }

            if (*out_count >= FREQ_LIST_NUM)
            {
                return -2;
            }

            int num_len = seg_end - seg_start + 1;
            int num = str_to_int(input_str + seg_start, num_len);
            if (num == -1)
            {
                return -3;
            }

            out_array[*out_count] = num;
            (*out_count)++;

            num_start = i + 1;
        }
    }

    return 0;
}

static unsigned int at_get_base_freq_and_spectrum_idx(unsigned int freq, unsigned char *spectrum_idx, unsigned int *actual_freq)
{
    for (int i = FREQ_SPECTRUM_COUNT - 1; i >= 0; i--)
    {
        if (freq >= FREQ_RANGES[i][0] && freq <= FREQ_RANGES[i][1])
        {
            unsigned int base_freq = FREQ_RANGES[i][0];
            int diff_freq_hz = (int)(freq - base_freq);

            if (actual_freq != RT_NULL)
            {
                if (diff_freq_hz % DEFAULE_BW_HZ != 0)
                {
                    // 不为带宽的整数倍，向下取整修正
                    *actual_freq = base_freq + (diff_freq_hz / DEFAULE_BW_HZ) * DEFAULE_BW_HZ;
                }
                else
                {
                    *actual_freq = freq;
                }
            }
            if (spectrum_idx != RT_NULL)
            {
                *spectrum_idx = i;
            }
            return base_freq; // 返回base freq
        }
    }

    return 1; // 不在范围
}

static inline unsigned short at_get_max_awaken_id(void)
{
    rate_mode_t *rate_mode = &at_simplified_local.rate_mode_map[at_simplified_local.rate_mode];
    unsigned char symbol_length = rate_mode->symbol_length;
    return g_max_awaken_id_table[symbol_length];
}

static inline unsigned char at_get_max_freq_num(unsigned char spectrum_idx)
{
    if (spectrum_idx >= FREQ_SPECTRUM_COUNT)
    {
        return 0;
    }
    return g_max_freq_num_table[spectrum_idx];
}

static inline unsigned int at_get_address(void)
{
    unsigned int addr = 0;
    unsigned char len;

    uc_wiota_get_userid(&addr, &len);
    return addr;
}

static inline int at_set_address(unsigned int addr)
{
    if (at_simplified_local.addr != addr)
    {
        if (uc_wiota_set_userid(&addr, USERID_LENGTH) == 0)
        {
            at_simplified_local.addr = addr;
            return 0;
        }
        return 1;
    }
    return 0;
}

static inline int at_set_subsystemid(unsigned int subsystemid)
{
    if (at_simplified_local.subsystemid != subsystemid)
    {
        sub_system_config_t config;
        uc_wiota_get_system_config(&config);
        config.subsystemid = subsystemid;
        if (uc_wiota_set_system_config(&config))
        {
            at_simplified_local.subsystemid = subsystemid;
            return 0;
        }
        return 1;
    }
    return 0;
}

static inline int at_set_power(unsigned char power)
{
    if (at_simplified_local.power != power)
    {
        at_simplified_local.power = power;
        uc_wiota_set_cur_power(power);
    }
    return 0;
}

static inline unsigned char at_get_rssi(void)
{
    radio_info_t radio = {0};

    uc_wiota_get_radio_info(&radio);
    return radio.rssi;
}

static int at_get_freq_list(const char *freq_str, unsigned char *freq_list, int *freq_num, unsigned char *spectrum_idx, unsigned int *base_freq)
{
    unsigned int freq_array[FREQ_LIST_NUM] = {0};
    int freq_num_temp = 0;

    if (0 != str_to_int_array(freq_str, freq_array, &freq_num_temp) || freq_num_temp > FREQ_LIST_NUM)
    {
        rt_kprintf("invalid freq list\n");
        return 1;
    }

    // 检查该组频率是否属于同一频谱，如果不属于，则返回错误
    unsigned int base_freq_temp = 0;
    unsigned int last_base_freq = 0;
    unsigned char spectrum_idx_temp = 0;
    unsigned int actual_freq_temp = 0;
    for (int i = 0; i < freq_num_temp; i++)
    {
        base_freq_temp = at_get_base_freq_and_spectrum_idx(freq_array[i], &spectrum_idx_temp, &actual_freq_temp);
        if (last_base_freq > 0 && base_freq_temp != last_base_freq)
        {
            rt_kprintf("freq not in same spectrum\n");
            return 2;
        }
        last_base_freq = base_freq_temp;
        freq_list[i] = FREQ_TO_INDEX(actual_freq_temp, base_freq_temp, DEFAULE_BW_HZ);
        if (freq_list[i] > at_get_max_freq_num(spectrum_idx_temp))
        {
            rt_kprintf("freq out of range\n");
            return 3;
        }
    }

    if (freq_num_temp < FREQ_LIST_NUM)
    {
        freq_list[freq_num_temp] = INVALID_FREQ;
    }
    *spectrum_idx = spectrum_idx_temp;
    *freq_num = freq_num_temp;
    *base_freq = base_freq_temp;

    return 0;
}

#define SCAN_SINGLE_TIMEOUT (10000) // 扫描单个频点超时时间
#define CONNECT_SYNC_TIMEOUT 5000   // 连接同步超时时间，单位ms
#define CONNECT_RETRY_COUNT 2       // 连接重试次数
#define RATE_MODE_OFFSET 1          // 速率模式偏移量，模式从1开始，下标从0开始

// Sleep/Wakeup相关常数
#define SLEEP_TIME_TO_SEC_DIVISOR 1000 // 毫秒转秒的除数
#define SLEEP_TIME_ROUNDING_VALUE 999  // 毫秒向上取整时的加值

// RTOS任务相关常数
#define WIOTA_MQ_MSG_QUEUE_SIZE 10 // WIOTA消息队列容量
#define WIOTA_TASK_STACK_SIZE 1024 // WIOTA任务栈大小
#define WIOTA_TASK_PRIORITY 10     // WIOTA任务优先级
#define WIOTA_TASK_TIME_SLICE 10   // WIOTA任务时间片

typedef enum
{
    WIOTA_CMD_CONNECT,
    WIOTA_CMD_SCAN_FREQ, // rssi query
    WIOTA_CMD_MAX,
} wiota_cmd_e;

typedef enum
{
    SYNC_SUC = 0,
    SYNC_FAIL,
    SYNC_LOST,
    STATE_MAX,
} connect_state_e;

typedef struct
{
    int cmd; // wiota_cmd_e
    void *data;
    int data_len;
} wiota_msg_t;

typedef struct
{
    unsigned char freq_list[FREQ_LIST_NUM];
    int freq_num;
    unsigned int base_freq;
    unsigned int timeout;
} scan_msg_t;

typedef struct
{
    rt_mq_t wiota_mq;
    rt_sem_t scan_sem;
    unsigned char init_flag : 1;
    unsigned char scan_result : 1;
    unsigned char is_scan_req : 1;
    unsigned char connnect_state : 5; // connect_state_e
} wiota_info_t;

static wiota_info_t g_wiota_info = {0};

static inline void at_printf_connect_state(void)
{
    const char *connect_state_str[STATE_MAX] = {"SYNC_SUC", "SYNC_FAIL", "SYNC_LOST"};
    at_server_printfln("+CONST:%s", connect_state_str[g_wiota_info.connnect_state]);
}

static unsigned char wiota_get_freq_list(unsigned char *freq_list)
{
    unsigned char freq_num = 0;
    unsigned char temp_list[FREQ_LIST_NUM] = {0};

    if (freq_list == RT_NULL)
    {
        return 0;
    }

    rt_enter_critical();
    uc_wiota_get_freq_list(temp_list);
    rt_exit_critical();
    for (unsigned char i = 0; i < FREQ_LIST_NUM; i++)
    {
        if (temp_list[i] == INVALID_FREQ)
        {
            break;
        }

        freq_list[i] = temp_list[i];
        freq_num++;
    }

    return freq_num;
}

static int wiota_send_msg(int cmd, void *data, int data_len)
{
    wiota_msg_t msg = {0};

    msg.cmd = cmd;
    msg.data = data;
    msg.data_len = data_len;

    if (rt_mq_send(g_wiota_info.wiota_mq, &msg, sizeof(wiota_msg_t)) != RT_EOK)
    {
        rt_kprintf("wiota send msg failed\n");
        return 1;
    }
    return 0;
}

static inline void wiota_send_connect_msg(void)
{
    // 每次connect前，需重置回调函数(在同步上后再重新注册)，避免在扫频过程中重复接收到UC_RECV_SYNC_LOST消息，导致重复发起连接
    uc_wiota_register_recv_data_callback(RT_NULL, UC_CALLBACK_NORAMAL_MSG);
    uc_wiota_register_recv_data_callback(RT_NULL, UC_CALLBACK_STATE_INFO);
    if (is_gateway_mode)
    {
        uc_wiota_gateway_end();
    }
    wiota_send_msg(WIOTA_CMD_CONNECT, RT_NULL, 0);
}

static int wiota_send_scan_freq_msg(unsigned char *freq_list, int freq_num, unsigned int base_freq, unsigned int timeout)
{
    scan_msg_t *scan_msg = (scan_msg_t *)rt_malloc(sizeof(scan_msg_t));
    if (scan_msg == RT_NULL)
    {
        return 1;
    }
    rt_memset(scan_msg, 0, sizeof(scan_msg_t));

    if (freq_list != RT_NULL && freq_num > 0)
    {
        rt_memcpy(scan_msg->freq_list, freq_list, freq_num);
    }
    scan_msg->freq_num = freq_num;
    scan_msg->base_freq = base_freq;
    scan_msg->timeout = timeout;

    if (wiota_send_msg(WIOTA_CMD_SCAN_FREQ, scan_msg, sizeof(scan_msg_t)) != 0)
    {
        rt_free(scan_msg);
        return 2;
    }

    return 0;
}

static inline int wiota_wait_scan_done(unsigned int timeout)
{
    if (rt_sem_take(g_wiota_info.scan_sem, timeout) != RT_EOK)
    {
        return 1;
    }
    return 0;
}

static inline void wiota_scan_done(void)
{
    rt_sem_release(g_wiota_info.scan_sem);
}

static unsigned char wiota_scan_freq_for_connect(unsigned char *freq_list, unsigned char freq_num)
{
    uc_recv_back_t scan_recv = {0};
    uc_freq_scan_result_t *freq_info = RT_NULL;
    unsigned char first_flag = 0;
    signed char best_rssi = 0;
    unsigned char best_freq_idx = INVALID_FREQ;

    uc_wiota_scan_freq(freq_list, freq_num, 0, SCAN_SINGLE_TIMEOUT * freq_num, RT_NULL, &scan_recv);
    if (scan_recv.result == UC_OP_SUCC && scan_recv.data)
    {
        freq_info = (uc_freq_scan_result_t *)scan_recv.data;

        for (unsigned char j = 0; j < scan_recv.data_len / sizeof(uc_freq_scan_result_t); j++)
        {
            if (freq_info[j].is_synced == 1)
            {
                if (0 == first_flag)
                {
                    best_rssi = freq_info[j].rssi;
                    best_freq_idx = freq_info[j].freq_idx;
                    first_flag = 1;
                }
                else if (freq_info[j].rssi > best_rssi)
                {
                    best_rssi = freq_info[j].rssi;
                    best_freq_idx = freq_info[j].freq_idx;
                }
            }
        }
        rt_free(freq_info);
    }

    return best_freq_idx;
}

static int wiota_scan_freq_for_rssi(unsigned char *freq_list, int freq_num, unsigned int base_freq, unsigned int timeout)
{
    uc_recv_back_t scan_result = {0};

    uc_wiota_scan_freq(freq_list, freq_num * sizeof(uc_freq_scan_req_t), 0, timeout, RT_NULL, &scan_result);
    if (UC_OP_SUCC == scan_result.result && scan_result.data != RT_NULL)
    {
        uc_freq_scan_result_t *freq_scan_result = (uc_freq_scan_result_t *)scan_result.data;
        for (int i = 0; i < scan_result.data_len / sizeof(uc_freq_scan_result_t); i++)
        {
            at_server_printfln("+RSSI:%d,%d,%d",
                               INDEX_TO_FREQ(freq_scan_result[i].freq_idx, base_freq, DEFAULE_BW_HZ),
                               freq_scan_result[i].rssi,
                               freq_scan_result[i].is_synced);
        }
        rt_free(scan_result.data);
        return 0;
    }
    return 1;
}

static int wiota_connect_to_ap(void)
{
    uc_wiota_connect();
    if (uc_wiota_wait_sync(CONNECT_SYNC_TIMEOUT, CONNECT_RETRY_COUNT))
    {
        return 1;
    }
    return 0;
}

// 尝试在多个频率上连接到接入点
static int wiota_try_multi_freq_connect(unsigned char *freq_list, int freq_num)
{
    for (int i = 0; i < freq_num; i++)
    {
        // 检查是否有新的扫描请求
        if (g_wiota_info.is_scan_req)
        {
            return 1;
        }

        uc_wiota_set_freq_info(freq_list[i]);
        if (wiota_connect_to_ap() == 0)
        {
            return 0;
        }
    }
    return 1;
}

static int wiota_connect(void)
{
    unsigned char freq_list[FREQ_LIST_NUM] = {0};
    unsigned char freq_num = 0;

    if (g_wiota_info.is_scan_req)
    {
        return 1;
    }

    freq_num = wiota_get_freq_list(freq_list);
    RT_ASSERT(freq_num > 0);

    if (freq_num == 1)
    {
        uc_wiota_set_freq_info(freq_list[0]);
    }
    else
    {
        unsigned char best_freq_idx = wiota_scan_freq_for_connect(freq_list, freq_num);
        if (best_freq_idx == INVALID_FREQ)
        {
            // 未扫描到同步频率，尝试轮流连接
            return wiota_try_multi_freq_connect(freq_list, freq_num);
        }
        uc_wiota_set_freq_info(best_freq_idx);
    }

    return wiota_connect_to_ap();
}

static void at_wiota_data_printf(int type, unsigned char *data, unsigned short data_len)
{
    at_rx_notity();
    at_server_printf("+RECV:%d,-%d,%d,", type, at_get_rssi(), data_len);
    at_send_data(data, data_len);
    at_server_printfln("");
    at_rx_complete();
}

// 网关模式extern了该函数
void wiota_recv_callback(uc_recv_back_p data)
{
    switch (data->type)
    {
    case UC_RECV_MSG:
    case UC_RECV_BC:
    case UC_RECV_OTA:
    {
        if (data == RT_NULL || data->data == RT_NULL || data->data_len < sizeof(app_header_t))
        {
            rt_kprintf("invalid recv_data\n");
            return;
        }
        app_header_t *app_header = (app_header_t *)data->data;
        int data_len = data->data_len - sizeof(app_header_t);
        unsigned char *recv_data = &data->data[sizeof(app_header_t)];
        unsigned char *decompress_data = RT_NULL;

        if (app_header->is_compress)
        {
            // 解压数据
            decompress_data = (unsigned char *)rt_malloc(DECOMPRESS_MAX_LEN);
            if (decompress_data == RT_NULL)
            {
                rt_kprintf("decompress data malloc failed\n");
                rt_free(data->data);
                return;
            }
            rt_memset(decompress_data, 0, DECOMPRESS_MAX_LEN);

            int decompress_len = fastlz_decompress(recv_data, data_len, decompress_data, DECOMPRESS_MAX_LEN);
            if (decompress_len > 0)
            {
                recv_data = decompress_data;
                data_len = decompress_len;
            }
            else
            {
                rt_kprintf("decompress data failed\n");
                rt_free(data->data);
                rt_free(decompress_data);
                return;
            }
        }

        at_wiota_data_printf(data->type, recv_data, data_len);

        if (decompress_data != RT_NULL)
        {
            rt_free(decompress_data);
        }
        rt_free(data->data);
        break;
    }
    case UC_RECV_SYNC_LOST:
        g_wiota_info.connnect_state = SYNC_LOST;
        at_printf_connect_state();
        wiota_send_connect_msg();
        break;
    // case UC_RECV_PG_TX_DONE:
    //     at_server_printfln("+PGTX:DONE");
    //     break;
    default:
        break;
    }
}

static void at_wiota_task_entry(void *parameter)
{
    wiota_msg_t msg = {0};

    while (1)
    {
        if (rt_mq_recv(g_wiota_info.wiota_mq, &msg, sizeof(wiota_msg_t), RT_WAITING_FOREVER) != RT_EOK)
        {
            continue;
        }

        switch (msg.cmd)
        {
        case WIOTA_CMD_CONNECT:
            if (wiota_connect() == 0)
            {
                uc_wiota_register_recv_data_callback(wiota_recv_callback, UC_CALLBACK_NORAMAL_MSG);
                uc_wiota_register_recv_data_callback(wiota_recv_callback, UC_CALLBACK_STATE_INFO);
                g_wiota_info.connnect_state = SYNC_SUC;
                at_simplified_local.cur_freq = INDEX_TO_FREQ(uc_wiota_get_freq_info(), at_simplified_local.base_freq, DEFAULE_BW_HZ);
                at_server_printfln("+FREQ:%u", at_simplified_local.cur_freq);
                at_printf_connect_state();
                if (is_gateway_mode)
                {
                    if (at_gateway_mode_enter(gw_mode, gw_auth_code) != 0)
                    {
                        rt_kprintf("enter gw_mode failed, please exec at+gwinit\n");
                    }
                }
            }
            else
            {
                g_wiota_info.connnect_state = SYNC_FAIL;
                at_printf_connect_state();
                wiota_send_connect_msg();
            }
            break;
        case WIOTA_CMD_SCAN_FREQ:
        {
            scan_msg_t *scan_msg = (scan_msg_t *)msg.data;
            g_wiota_info.scan_result = wiota_scan_freq_for_rssi(scan_msg->freq_list, scan_msg->freq_num, scan_msg->base_freq, scan_msg->timeout);
            wiota_scan_done();
            // 扫频完成后需要重新连接
            if (g_wiota_info.connnect_state == SYNC_SUC)
            {
                // 如果扫频之前已经处于同步状态，则直接重连
                if (wiota_connect_to_ap() != 0)
                {
                    wiota_send_connect_msg();
                }
            }
            else
            {
                wiota_send_connect_msg();
            }
            g_wiota_info.is_scan_req = 0;
            rt_free(scan_msg);
            break;
        }
        default:
            rt_kprintf("unknow w_cmd %d\n", msg.cmd);
            break;
        }
    }
}

static void at_wiota_task_init(void)
{
    if (!g_wiota_info.init_flag)
    {
        g_wiota_info.wiota_mq = rt_mq_create("wiota_mq", sizeof(wiota_msg_t), WIOTA_MQ_MSG_QUEUE_SIZE, RT_IPC_FLAG_FIFO);
        RT_ASSERT(g_wiota_info.wiota_mq != RT_NULL);
        g_wiota_info.scan_sem = rt_sem_create("scan_sem", 0, RT_IPC_FLAG_FIFO);
        RT_ASSERT(g_wiota_info.scan_sem != RT_NULL);
        rt_thread_t wiota_task = rt_thread_create("wiota_task", at_wiota_task_entry, RT_NULL, WIOTA_TASK_STACK_SIZE, WIOTA_TASK_PRIORITY, WIOTA_TASK_TIME_SLICE);
        RT_ASSERT(wiota_task != RT_NULL);
        rt_thread_startup(wiota_task);
        g_wiota_info.init_flag = 1;
    }
}

static void at_wiota_run(void)
{
    uc_wiota_exit();
    uc_wiota_init();

    rate_mode_t *rate_mode = &at_simplified_local.rate_mode_map[at_simplified_local.rate_mode - RATE_MODE_OFFSET];
    sub_system_config_t config = {0};

    uc_wiota_get_system_config(&config);
    config.symbol_length = rate_mode->symbol_length;
    config.dlul_ratio = rate_mode->dlul_ratio;
    config.group_number = rate_mode->dlul_ratio;
    config.spectrum_idx = at_simplified_local.spectrum_idx;
    config.freq_idx = FREQ_TO_INDEX(at_simplified_local.cur_freq, at_simplified_local.base_freq, DEFAULE_BW_HZ);
    config.freq_list[0] = config.freq_idx;
    config.freq_list[1] = INVALID_FREQ;
    config.subsystemid = at_simplified_local.subsystemid;
    rt_kprintf("pow %d, sym_len %d, du_ratio %d, spec_idx %d, freq_idx %d, subsystemid %u(0x%x), mcs %d\n",
               config.ap_tx_power, config.symbol_length, config.dlul_ratio, config.spectrum_idx,
               config.freq_idx, config.subsystemid, config.subsystemid, rate_mode->mcs);
    uc_wiota_set_system_config(&config);
    uc_wiota_set_userid(&at_simplified_local.addr, USERID_LENGTH);

    uc_wiota_run();
    uc_wiota_set_cur_power(at_simplified_local.power);
    uc_wiota_set_data_rate(0, rate_mode->mcs); // mcs
    uc_wiota_set_crc(1);                       // crc校验常开

    at_wiota_task_init();
    if (g_wiota_info.connnect_state == SYNC_SUC)
    {
        wiota_send_connect_msg();
    }
    rt_kprintf("at_wiota_run ok\n");
}

void at_simplified_init(void)
{
    at_state_pin_init();

    // 初始化默认配置
    // -1，模式从1开始，下标从0开始
    const rate_mode_t rate_mode_map[RATE_MODE_MAX - 1] = RATE_MODE_MAP;

    at_simplified_local.addr = at_get_address();
    at_simplified_local.subsystemid = DEFAULT_SUBSYSTEMID;
    // cur_freq 5020000, base_freq 470000000, spectrum_idx 3
    at_simplified_local.base_freq = at_get_base_freq_and_spectrum_idx(DEFAULE_FREQ, &at_simplified_local.spectrum_idx, &at_simplified_local.cur_freq);
    RT_ASSERT(at_simplified_local.base_freq >= BASE_FREQ);
    at_simplified_local.power = DEFAULE_POWER;
    // rate_mode 4, symbol_length 256, mcs 2
    at_simplified_local.rate_mode = DEFAULE_RATE_MODE;
    rt_memcpy(at_simplified_local.rate_mode_map, rate_mode_map, sizeof(rate_mode_map));
    rt_kprintf("at_simplified_init, addr %u(0x%x), freq %uHz-%uHz, power %ddBm, rate mode %d(sym_len 1, mcs 2)\n",
               at_simplified_local.addr, at_simplified_local.addr,
               at_simplified_local.base_freq, at_simplified_local.cur_freq,
               at_simplified_local.power, at_simplified_local.rate_mode);

    // 启动wiota协议栈
    at_wiota_run();
}

static at_result_t at_wiota_sn_query(void)
{
    unsigned char serial[SERIAL_NUM_LEN] = {0};

    uc_wiota_get_module_id(serial);
    at_server_printfln("+SN:%s", serial);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_version_query(void)
{
    unsigned char version[SOFTWARE_VER_LEN] = {0};

    uc_wiota_get_version(version, RT_NULL, RT_NULL, RT_NULL);
    at_server_printfln("+VER:%s", version);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_connect_state_query(void)
{
    at_printf_connect_state();
    return AT_RESULT_OK;
}

static at_result_t at_wiota_rssi_test(void)
{
    at_server_printfln("AT+RSSI=<freq1>[,<freq2>[,...,<freq16>]]");
    return AT_RESULT_OK;
}

static at_result_t at_wiota_scan_freq(unsigned char *freq_list, int freq_num, unsigned char spectrum_idx, unsigned int base_freq)
{
    unsigned int timeout = SCAN_SINGLE_TIMEOUT * (freq_num == 0 ? at_get_max_freq_num(spectrum_idx) : freq_num);

    g_wiota_info.is_scan_req = 1;
    wiota_send_scan_freq_msg(freq_list, freq_num, base_freq, timeout);
    if (wiota_wait_scan_done(timeout) != 0 || g_wiota_info.scan_result != 0)
    {
        return AT_RESULT_FAILE;
    }
    return AT_RESULT_OK;
}

static at_result_t at_wiota_rssi_query(void)
{
    // 查询当前频率的环境信号
    unsigned char freq_idx = uc_wiota_get_freq_info();

    return at_wiota_scan_freq(&freq_idx, 1, at_simplified_local.spectrum_idx, at_simplified_local.base_freq);
}

static at_result_t at_wiota_rssi_setup(const char *args)
{
    // 查询指定频率列表的环境信号
    unsigned char freq_list[FREQ_LIST_NUM] = {0};
    int freq_num = 0;
    unsigned char spectrum_idx = 0;
    unsigned int base_freq = 0;

    if (0 != at_get_freq_list(args + 1, freq_list, &freq_num, &spectrum_idx, &base_freq))
    {
        return AT_RESULT_PARSE_FAILE;
    }

    return at_wiota_scan_freq(freq_list, freq_num, spectrum_idx, base_freq);
}

static at_result_t at_wiota_rssi_exec(void)
{
    // 查询当前频谱的所有频率的环境信号
    return at_wiota_scan_freq(RT_NULL, 0, at_simplified_local.spectrum_idx, at_simplified_local.base_freq);
}

static at_result_t at_wiota_sync_test(void)
{
    at_server_printfln("AT+SYNC=<sync[1-%d]>", MAX_SUBSYSTEMID);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_sync_query(void)
{
    at_server_printfln("+SYNC:%u", at_simplified_local.subsystemid);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_sync_setup(const char *args)
{
    unsigned int subsystemid = 0;

    args = parse((char *)(++args), "d", &subsystemid);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (subsystemid <= MAX_SUBSYSTEMID)
    {
        if (0 == at_set_subsystemid(subsystemid))
        {
            return AT_RESULT_OK;
        }
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_address_test(void)
{
    at_server_printfln("AT+ADDR=<addr[1-4294967295]>");
    return AT_RESULT_OK;
}

static at_result_t at_wiota_address_query(void)
{
    at_server_printfln("+ADDR:%u", at_simplified_local.addr);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_address_setup(const char *args)
{
    unsigned int addr = 0;

    args = parse((char *)(++args), "d", &addr);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 == at_set_address(addr))
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_power_test(void)
{
    at_server_printfln("AT+POWER=<power[%d-%d]>", POWER_MIN, POWER_MAX);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_power_query(void)
{
    at_server_printfln("+POWER:%d", at_simplified_local.power);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_power_setup(const char *args)
{
    int power = 0;

    args = parse((char *)(++args), "d", &power);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (power >= POWER_MIN && power <= POWER_MAX)
    {
        if (0 == at_set_power(power))
        {
            return AT_RESULT_OK;
        }
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_freq_test(void)
{
    at_server_printfln("AT+FREQ=<freq1>[,<freq2>[,...,<freq16>]]");
    return AT_RESULT_OK;
}

static at_result_t at_wiota_freq_query(void)
{
    at_server_printfln("+FREQ:%u", INDEX_TO_FREQ(uc_wiota_get_freq_info(), at_simplified_local.base_freq, DEFAULE_BW_HZ));
    return AT_RESULT_OK;
}

static at_result_t at_wiota_freq_setup(const char *args)
{
    unsigned char freq_list[FREQ_LIST_NUM] = {0};
    int freq_num = 0;
    unsigned char spectrum_idx = 0;

    if (0 != at_get_freq_list(args + 1, freq_list, &freq_num, &spectrum_idx, &at_simplified_local.base_freq))
    {
        return AT_RESULT_PARSE_FAILE;
    }

    rt_enter_critical();
    uc_wiota_set_freq_list(freq_list, freq_num);
    rt_exit_critical();
    if (spectrum_idx != at_simplified_local.spectrum_idx)
    {
        at_simplified_local.spectrum_idx = spectrum_idx;
        at_wiota_run();
    }
    else
    {
        // 如果本身处于未同步状态，无需发送连接消息，重连机制会自动连接
        if (g_wiota_info.connnect_state == SYNC_SUC)
        {
            wiota_send_connect_msg();
        }
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_rate_test(void)
{
    at_server_printfln("AT+RATE=<rate[%d-%d]>", RATE_MODE_1, RATE_MODE_MAX - 1);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_rate_query(void)
{
    at_server_printfln("+RATE:%d", at_simplified_local.rate_mode);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_rate_setup(const char *args)
{
    unsigned int rate = 0;

    args = parse((char *)(++args), "d", &rate);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (rate <= RATE_MODE_8 && rate >= RATE_MODE_1)
    {
        unsigned char last_rate = at_simplified_local.rate_mode;
        rate_mode_t *last_rate_mode = &at_simplified_local.rate_mode_map[last_rate - RATE_MODE_OFFSET];
        rate_mode_t *cur_rate_mode = &at_simplified_local.rate_mode_map[rate - RATE_MODE_OFFSET];

        if (last_rate != rate)
        {
            if ((last_rate_mode->symbol_length != cur_rate_mode->symbol_length) ||
                last_rate_mode->dlul_ratio != cur_rate_mode->dlul_ratio)
            {
                at_simplified_local.rate_mode = rate;
                at_wiota_run();
            }
            else
            {
                uc_wiota_set_data_rate(0, cur_rate_mode->mcs); // mcs
            }
        }
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

// 数据压缩辅助函数
// 返回值：0 成功, 非0 失败
static int at_wiota_compress_and_pack(unsigned char *data_buf, unsigned int data_len,
                                      unsigned char *header, int header_len,
                                      int *output_len)
{
    if (data_buf == NULL || header == NULL || output_len == NULL)
    {
        return -1;
    }

    int comp_data_len = data_len * COMPRESS_BUFFER_FACTOR;
    unsigned char *comp_data = rt_malloc(comp_data_len);
    if (comp_data == NULL)
    {
        return -2;
    }
    rt_memset(comp_data, 0, comp_data_len);

    // 压缩数据部分
    int compressed_len = fastlz_compress(data_buf, data_len, comp_data);
    if (compressed_len < data_len)
    {
        // 压缩成功，使用压缩数据
        app_header_t *app_header = (app_header_t *)header;
        app_header->is_compress = 1;
        rt_memcpy(data_buf, comp_data, compressed_len);
        *output_len = header_len + compressed_len;
    }
    else
    {
        // 压缩无效，使用原始数据
        app_header_t *app_header = (app_header_t *)header;
        app_header->is_compress = 0;
        *output_len = header_len + data_len;
    }

    rt_free(comp_data);
    return 0;
}

#define SEND_MAX_LEN(gw_mode) (gw_mode ? GATEWAY_SEND_MAX_LEN : (UC_DATA_LENGTH_LIMIT - sizeof(app_header_t)))

static int at_wiota_recv_payload(unsigned char *payload, unsigned int payload_len)
{
    at_server_printfln(">");
    if (at_server_recv(at_get_server(), (char *)payload, payload_len,
                       rt_tick_from_millisecond(RECV_DATA_TIMEOUT)) != payload_len)
    {
        rt_kprintf("at_server_recv timeout\n");
        return -1;
    }
    return 0;
}

static at_result_t at_wiota_send_test(void)
{
    at_server_printfln("AT+SEND=<len[1-%d]>", SEND_MAX_LEN(is_gateway_mode));
    return AT_RESULT_OK;
}

static at_result_t at_wiota_send_setup(const char *args)
{
    unsigned char *recv_buf = RT_NULL;
    unsigned int recv_len = 0;
    int send_status = AT_RESULT_FAILE;

    args = parse((char *)(++args), "d", &recv_len);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (recv_len < 1 || recv_len > SEND_MAX_LEN(is_gateway_mode))
    {
        return AT_RESULT_FAILE;
    }

    if (g_wiota_info.connnect_state != SYNC_SUC)
    {
        rt_kprintf("not connect!\n");
        return AT_RESULT_FAILE;
    }

    if (is_gateway_mode)
    {
        if (uc_wiota_gateway_get_state() == GATEWAY_OTA_UPGRADE)
        {
            rt_kprintf("ota is running, not send data\n");
            return AT_RESULT_FAILE;
        }

        recv_buf = rt_malloc(recv_len + CRC16_LEN);
        if (recv_buf == RT_NULL)
        {
            return AT_RESULT_FAILE;
        }

        if (at_wiota_recv_payload(recv_buf, recv_len) != 0)
        {
            rt_free(recv_buf);
            return AT_RESULT_FAILE;
        }

        at_tx_start();
        int send_result = uc_wiota_gateway_send_data(recv_buf, recv_len, AT_SEND_TIMEOUT);
        at_tx_end();

        rt_free(recv_buf);
        send_status = (send_result != 0) ? AT_RESULT_OK : AT_RESULT_FAILE;
    }
    else
    {
        int header_len = sizeof(app_header_t);
        int payload_len = recv_len;
        int malloc_len = header_len + payload_len + CRC16_LEN;

        recv_buf = rt_malloc(malloc_len);
        if (recv_buf == RT_NULL)
        {
            return AT_RESULT_FAILE;
        }
        rt_memset(recv_buf, 0, malloc_len);

        if (at_wiota_recv_payload(recv_buf + header_len, payload_len) != 0)
        {
            rt_free(recv_buf);
            return AT_RESULT_FAILE;
        }

        app_header_t *app_header = (app_header_t *)recv_buf;
        app_header->is_compress = 0;

        int data_len = 0;
        if (at_wiota_compress_and_pack(recv_buf + header_len, payload_len, recv_buf, header_len, &data_len) != 0)
        {
            rt_free(recv_buf);
            return AT_RESULT_FAILE;
        }

        at_tx_start();
        uc_op_result_e result = uc_wiota_send_data(recv_buf, data_len, AT_SEND_TIMEOUT, RT_NULL);
        at_tx_end();

        rt_free(recv_buf);
        send_status = (result == UC_OP_SUCC) ? AT_RESULT_OK : AT_RESULT_FAILE;
    }

    return send_status;
}

static at_result_t at_wiota_sleep_test(void)
{
    at_server_printfln("AT+SLEEP=<mode[0-2]>,<time[%d-%d]>", MIN_SLEEP_TIME, MAX_SLEEP_TIME);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_sleep_setup(const char *args)
{
    unsigned int mode = 0, time = 0; // ms

    args = parse((char *)(++args), "d,d", &mode, &time);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (mode < SLEEP_MODE_MAX)
    {
        // 模式DEEP_SLEEP_MODE不用检查time
        if (mode >= ALARM_WAKEUP_MODE && (time < MIN_SLEEP_TIME || time > MAX_SLEEP_TIME))
        {
            return AT_RESULT_FAILE;
        }

        at_server_printfln("OK"); // 睡眠之后无法在输出，需提前输出OK
        uart_wait_tx_done();
        if (mode == DEEP_SLEEP_MODE)
        {
            uc_wiota_sleep_enter(0, 0);
        }
        else if (mode == ALARM_WAKEUP_MODE)
        {
            // 向上取整秒
            int sleep_time = (time + SLEEP_TIME_ROUNDING_VALUE) / SLEEP_TIME_TO_SEC_DIVISOR; // ms -> s
            uc_wiota_set_alarm_time(sleep_time);
            uc_wiota_sleep_enter(0, 0);
        }
        else if (mode == RF_WAKEUP_MODE)
        {
            uc_lpm_rx_cfg_t pr_cfg = {0};
            sub_system_config_t config = {0};
            unsigned short max_awaken_id = at_get_max_awaken_id();

            uc_wiota_get_system_config(&config);
            uc_wiota_get_paging_rx_cfg(&pr_cfg);

            pr_cfg.mode = 1;
            pr_cfg.spectrum_idx = config.spectrum_idx;
            pr_cfg.symbol_length = config.symbol_length;
            pr_cfg.freq = config.freq_idx;
            pr_cfg.detect_period = time;
            pr_cfg.period_multiple = 1;
            pr_cfg.awaken_id = at_simplified_local.addr % (max_awaken_id - 1); // 单播唤醒ID
            pr_cfg.awaken_id_another = max_awaken_id;                          // 取最后一个为广播唤醒ID

            uc_wiota_set_paging_rx_cfg(&pr_cfg);
            uc_wiota_paging_rx_enter(0, 0);
        }
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_wakeup_test(void)
{
    at_server_printfln("AT+WAKEUP=<addr[0-4294967295]>,<time[%d-%d]>", MIN_SLEEP_TIME, MAX_SLEEP_TIME);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_wakeup_setup(const char *args)
{
    unsigned int addr = 0, time = 0; // ms

    args = parse((char *)(++args), "d,d", &addr, &time);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (time >= MIN_SLEEP_TIME && time <= MAX_SLEEP_TIME)
    {
        uc_lpm_tx_cfg_t pt_cfg = {0};
        sub_system_config_t config = {0};
        unsigned short max_awaken_id = at_get_max_awaken_id();

        uc_wiota_get_system_config(&config);
        uc_wiota_get_paging_tx_cfg(&pt_cfg);

        pt_cfg.mode = 1;
        pt_cfg.spectrum_idx = config.spectrum_idx;
        pt_cfg.symbol_length = config.symbol_length;
        pt_cfg.freq = config.freq_idx;
        pt_cfg.send_time = time + 100;
        if (addr == 0)
        {
            // 广播唤醒
            pt_cfg.awaken_id = max_awaken_id;
        }
        else
        {
            // 单播唤醒
            pt_cfg.awaken_id = addr % (max_awaken_id - 1);
        }
        uc_wiota_set_paging_tx_cfg(&pt_cfg);
        uc_wiota_paging_tx_start();
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

// 以下是需要同步网关配合的AT指令
static at_result_t at_gateway_state_printf(int gw_state)
{
    const char *gw_state_str[GATEWAY_OTA_PROGRAMING + 1] = {
        "DEFAULT",
        "NORMAL",
        "ABNORMAL",
        "EXIT",
        "RECONNECT",
        "OTA_START",
        "OTA_END",
    };

    if (gw_state > GATEWAY_OTA_PROGRAMING)
    {
        return AT_RESULT_FAILE;
    }
    at_server_printfln("+GWINIT:%s", gw_state_str[gw_state]);
    return AT_RESULT_OK;
}

static void at_gateway_data_callback(void *data, unsigned int data_len, unsigned char data_type)
{
    at_wiota_data_printf(data_type, data, data_len);
}

static void at_gateway_state_callback(unsigned char gw_state)
{
    at_gateway_state_printf(gw_state);
}

static void at_gateway_ex_mcu_callback(unsigned int start_addr, unsigned int len, char *md5)
{
    int md5_size = rt_strlen(md5);

    ota_start_addr = start_addr;
    ota_file_size = len;
    rt_memcpy(ota_file_md5, md5, md5_size > MD5_SIZE ? MD5_SIZE : md5_size);
    at_server_printfln("+GWMCU:%u,%u,%s", ota_start_addr, ota_file_size, ota_file_md5);
}

static int at_gateway_mode_enter(unsigned char mode, char *auth_code)
{
    unsigned char freq_list[FREQ_LIST_NUM] = {0};

    uc_wiota_gateway_register_user_recv_cb(at_gateway_data_callback, at_gateway_state_callback);
    uc_wiota_gateway_register_ex_mcu_cb(at_gateway_ex_mcu_callback);
    wiota_get_freq_list(freq_list);

    // 保存模式和鉴权码，当配置了不支持热配置的参数，需要自动重新进入网关模式
    gw_mode = mode;
    rt_memcpy(gw_auth_code, auth_code, AUTH_CODE_LEN);

    int init_state = uc_wiota_gateway_start(mode, auth_code, freq_list);
    rt_thread_delay(2);
    if (UC_GATEWAY_OK == init_state)
    {
        return 0;
    }
    uc_wiota_gateway_end();
    return 1;
}

static at_result_t at_gateway_init_test(void)
{
    at_server_printfln("AT+GWINIT=<mode[0-3]>,<auth_code[16bytes]>");
    return AT_RESULT_OK;
}

static at_result_t at_gateway_init_query(void)
{
    return at_gateway_state_printf(uc_wiota_gateway_get_state());
}

static at_result_t at_gateway_init_setup(const char *args)
{
    char auth_code[AUTH_CODE_LEN] = {0xFF};
    unsigned int mode = 0;

    args = parse((char *)(++args), "d,s", &mode, AUTH_CODE_LEN, auth_code);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (mode > UC_GATEWAY_TIME_SLOT_MODE)
    {
        return AT_RESULT_FAILE;
    }

    if (mode == 0)
    {
        uc_wiota_gateway_end();
        is_gateway_mode = 0;
    }
    else
    {
        int auth_code_len = rt_strlen(auth_code);
        auth_code[auth_code_len - 2] = '\0'; //  remove '\r\n'
        auth_code[auth_code_len - 1] = '\0';
        if (mode == 1)
        {
            mode = UC_GATEWAY_MODE;
        }
        if (at_gateway_mode_enter(mode, auth_code) != 0)
        {
            return AT_RESULT_FAILE;
        }
        is_gateway_mode = 1;
    }

    return AT_RESULT_OK;
}

static at_result_t at_gateway_test_exec(void)
{
    if (uc_wiota_gateway_state_update_info_msg())
    {
        return AT_RESULT_OK;
    }
    return AT_RESULT_FAILE;
}

static at_result_t at_gateway_ex_mcu_test(void)
{
    int bin_size;
    int reserved_size;
    int ota_size;

    get_partition_size(&bin_size, &reserved_size, &ota_size);
    int max_size = bin_size + reserved_size + ota_size;
    at_server_printfln("AT+GWINIT=<read_addr[%d-%d]>,<read_len[1-%d]>", bin_size, max_size, OTA_READ_LEN);
    return AT_RESULT_OK;
}

static at_result_t at_gateway_ex_mcu_query(void)
{
    at_server_printfln("+GWMCU:%u,%u,%s", ota_start_addr, ota_file_size, ota_file_md5);
    return AT_RESULT_OK;
}

static at_result_t at_gateway_ex_mcu_setup(const char *args)
{
    unsigned int read_addr = 0;
    unsigned int read_len = 0;

    args = parse((char *)(++args), "d,d", &read_addr, &read_len);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (read_len == 0 || read_len > OTA_READ_LEN || uc_wiota_gateway_get_ota_state() == 1)
    {
        return AT_RESULT_FAILE;
    }

    unsigned char *read_buffer = rt_malloc(read_len);
    if (read_buffer == RT_NULL)
    {
        return AT_RESULT_FAILE;
    }
    rt_memset(read_buffer, 0, read_len);

    uc_wiota_suspend_connect();
    rt_thread_mdelay(uc_wiota_get_frame_len() / 1000 + 2);
    if (uc_wiota_ota_flash_read(read_addr, read_buffer, read_len) != read_len)
    {
        rt_free(read_buffer);
        uc_wiota_recover_connect();
        return AT_RESULT_FAILE;
    }
    uc_wiota_recover_connect();

    at_server_printfln("+GWMCU:%d", read_len);
    at_send_data(read_buffer, read_len);
    at_server_printfln("");

    rt_free(read_buffer);

    return AT_RESULT_OK;
}

AT_CMD_EXPORT("AT+SN", RT_NULL, RT_NULL, at_wiota_sn_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+VER", RT_NULL, RT_NULL, at_wiota_version_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+CONST", RT_NULL, RT_NULL, at_wiota_connect_state_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+RSSI", "=<freq>", at_wiota_rssi_test, at_wiota_rssi_query, at_wiota_rssi_setup, at_wiota_rssi_exec);
AT_CMD_EXPORT("AT+SYNC", "=<sync>", at_wiota_sync_test, at_wiota_sync_query, at_wiota_sync_setup, RT_NULL);
AT_CMD_EXPORT("AT+ADDR", "=<addr>", at_wiota_address_test, at_wiota_address_query, at_wiota_address_setup, RT_NULL);
AT_CMD_EXPORT("AT+POWER", "=<power>", at_wiota_power_test, at_wiota_power_query, at_wiota_power_setup, RT_NULL);
AT_CMD_EXPORT("AT+RATE", "=<rate>", at_wiota_rate_test, at_wiota_rate_query, at_wiota_rate_setup, RT_NULL);
AT_CMD_EXPORT("AT+FREQ", "=<freq>", at_wiota_freq_test, at_wiota_freq_query, at_wiota_freq_setup, RT_NULL);
AT_CMD_EXPORT("AT+SEND", "=<len>", at_wiota_send_test, RT_NULL, at_wiota_send_setup, RT_NULL);
AT_CMD_EXPORT("AT+SLEEP", "=<mode>,<time>", at_wiota_sleep_test, RT_NULL, at_wiota_sleep_setup, RT_NULL);
AT_CMD_EXPORT("AT+WAKEUP", "=<addr>,<time>", at_wiota_wakeup_test, RT_NULL, at_wiota_wakeup_setup, RT_NULL);
// 以下是需要同步网关配合的AT指令
AT_CMD_EXPORT("AT+GWINIT", "=<mode>,<auth_code>", at_gateway_init_test, at_gateway_init_query, at_gateway_init_setup, RT_NULL);
AT_CMD_EXPORT("AT+GWTEST", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_gateway_test_exec);
AT_CMD_EXPORT("AT+GWMCU", "=<read_addr>,<read_len>", at_gateway_ex_mcu_test, at_gateway_ex_mcu_query, at_gateway_ex_mcu_setup, RT_NULL);
