#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <string.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "uc_string_lib.h"

#include "uc_cbor.h"
#include "uc_coding.h"
#include "uc_ota_flash.h"
#include "uc_wiota_gateway_api.h"

#ifdef AT_WIOTA_GATEWAY_API
#define GATEWAY_MAG_CODE_DL_RECV (1L << 1)
#define GATEWAY_MAG_CODE_UL_HEART (1L << 2)
#define GATEWAY_MAG_CODE_OTA_REQ (1L << 3)
#define GATEWAY_MAG_CODE_UL_MISS_DATA_REQ (1L << 4)
#define GATEWAY_MAG_CODE_EXIT (1L << 5)

#define GATEWAY_OTA_VER_PERIOD (7200000) // 2 hours

// #define GATEWAY_OTA_FLASH_BIN_SIZE 328
// #define GATEWAY_OTA_FLASH_REVERSE_SIZE 136
// #define GATEWAY_OTA_FLASH_OTA_SIZE 40
#define GATEWAY_OTA_BLOCK_SIZE 512
// #define GATEWAY_MODE_OTA_START_ADDR ((GATEWAY_OTA_FLASH_BIN_SIZE + GATEWAY_OTA_FLASH_REVERSE_SIZE) * 1024)
#define GATEWAY_WAIT_QUEUE_TIMEOUT 100
#define GATEWAY_WAIT_AUTH_TIMEOUT 6000
#define EXTERN_MCU "ex_mcu"

#define SET_BIT(value, bit) (value |= (1 << bit))
#define CLEAR_BIT(value, bit) (value &= ~(1 << bit))
#define JUDGMENT_BIT(value, bit) (value >> bit & 1)

static uc_wiota_gateway_user_recv_cb uc_wiota_gateway_user_recv = RT_NULL;
static uc_wiota_gateway_state_report_cb uc_wiota_gateway_exce_report = RT_NULL;
static uc_wiota_gateway_ex_mcu_cb uc_wiota_gateway_ex_mcu = RT_NULL;
static uc_wiota_gateway_get_rtc_cb uc_wiota_gateway_rtc = RT_NULL;
static unsigned char rtc_fmt = 0xff;
static rt_sem_t rtc_sem = RT_NULL;

#ifdef RT_USING_AT
extern void wiota_recv_callback(uc_recv_back_p data);
#endif
void uc_wiota_gateway_api_handle(void *para);
// before scantf freq.
static uc_gateway_state_t uc_gateway_state = GATEWAY_DEFAULT;
static uc_gw_connect_e wiota_gw_connect_flag = UC_GW_NO_CONNECT;
static unsigned char wiota_gateway_run_flag = RT_FALSE;

static void uc_gateway_clean_data(void);

extern void at_wiota_get_avail_freq_list(unsigned char *output_list, unsigned char list_len);

typedef enum
{
    GATEWAY_OTA_DEFAULT = 0,
    GATEWAY_OTA_DOWNLOAD = 1,
    GATEWAY_OTA_PROGRAM = 2,
    GATEWAY_OTA_STOP = 3
} uc_gateway_ota_state_t;

typedef struct uc_wiota_gateway_msg
{
    void *data;
    unsigned int data_len;
    int msg_type;
    unsigned char recv_msg_type;
} uc_wiota_gateway_msg_t;

typedef struct uc_wiota_gateway_api_mode
{
    // handler
    rt_thread_t gateway_handler;
    rt_mq_t gateway_mq;
    // rt_timer_t heart_timer;

    unsigned char gateway_mode;
    unsigned char allow_run_flag;
    e_auth_state auth_state;
    char auth_code[16];
    char device_type[16];

    // ota para
    rt_timer_t ota_timer;
    rt_timer_t ver_timer;
    char new_version[16];
    uc_gateway_ota_state_t ota_state;
    int upgrade_type;
    unsigned int block_count;
    unsigned int block_size;
    unsigned char mask_map[28];
    unsigned short mcs_len;
    unsigned short upgrade_num;
    int miss_data_num;
    int miss_data_invalid_req;

    unsigned int wiota_userid;
    unsigned int dev_address;
    unsigned short gateway_task_run;
    unsigned char exit_flag;
    unsigned char gw_verity;
} uc_wiota_gateway_api_mode_t;

static uc_wiota_gateway_api_mode_t gateway_mode = {0};
unsigned char gw_freq_list[APP_CONNECT_FREQ_NUM] = {0};

void uc_wiota_gateway_set_run_flag(unsigned char run_flag);
unsigned int uc_wiota_gateway_get_wiota_id(void);
// unsigned char uc_wiota_gateway_get_reboot_flag(void);
static void uc_wiota_gateway_send_miss_data_req_to_queue(void);
static unsigned char g_time_slot_fn = 0;

unsigned char uc_wiota_gateway_is_ex_mcu_read(void)
{
    return (gateway_mode.ota_state != GATEWAY_OTA_DOWNLOAD);
}

unsigned char uc_wiota_gateway_get_run_flag(void)
{
    return wiota_gateway_run_flag;
}

void uc_wiota_gateway_set_run_flag(unsigned char run_flag)
{
    wiota_gateway_run_flag = run_flag;
}

static void uc_wiota_gateway_set_auth_state(e_auth_state state)
{
    gateway_mode.auth_state = state;
}

// static e_auth_state uc_wiota_gateway_get_auth_state(void)
// {
//     return gateway_mode.auth_state;
// }

static void uc_wiota_gateway_set_wiota_id(unsigned int wiota_id)
{
    gateway_mode.wiota_userid = wiota_id;
}

unsigned int uc_wiota_gateway_get_wiota_id(void)
{
    return gateway_mode.wiota_userid;
}

static void uc_wiota_gateway_set_time_slot_fn(unsigned char time_slot_fn)
{
    g_time_slot_fn = time_slot_fn;
    rt_kprintf("timeSlotFn %d\n", time_slot_fn);
}

unsigned char uc_wiota_gateway_get_time_slot_fn(void)
{
    return g_time_slot_fn;
}

static void uc_gateway_stop_ota_timer(void)
{
    if (gateway_mode.ota_timer != RT_NULL)
    {
        rt_timer_stop(gateway_mode.ota_timer);
    }
}

static void uc_gateway_control_ota_timer(unsigned int timeout)
{
    if (gateway_mode.ota_timer != RT_NULL)
    {
        rt_timer_control(gateway_mode.ota_timer, RT_TIMER_CTRL_SET_TIME, (void *)&timeout);
        rt_timer_start(gateway_mode.ota_timer);
    }
}

static void uc_gateway_clear_state(void)
{
    if (gateway_mode.ota_state != GATEWAY_OTA_DEFAULT)
    {
        gateway_mode.ota_state = GATEWAY_OTA_DEFAULT;
        gateway_mode.upgrade_num = 0;
        rt_memset(gateway_mode.mask_map, 0, sizeof(gateway_mode.mask_map));
        uc_gateway_stop_ota_timer();
    }
}

static unsigned int uc_gateway_calc_single_otapage_time(void)
{
    unsigned int single_time = 0;

    single_time = (((((GATEWAY_OTA_BLOCK_SIZE + 150) / gateway_mode.mcs_len + 1) / 7 + 1) * uc_wiota_get_frame_len()) / 1000 + 1) * 3; // ota send 3 round, UPROUND

    return single_time;
}

static unsigned int uc_gateway_get_miss_data_timeout(void)
{
    unsigned int timeout;
    unsigned int userid;
    unsigned char len;
    unsigned int gw_timeout = uc_gateway_calc_single_otapage_time() * 3;

    uc_wiota_get_userid(&userid, &len);
    timeout = ((userid & 0xF) + 1) * 1000;

    return timeout % gw_timeout;
}

#if 0
static void uc_wiota_gateway_set_dev_address(unsigned int dev_address)
{
    gateway_mode.dev_address = dev_address;
}
#endif
unsigned int uc_wiota_gateway_get_dev_address(void)
{
    unsigned char serial[16] = {0};
    unsigned int *dev_addr = (unsigned int *)(&serial[4]);
    // return gateway_mode.dev_address;
    uc_wiota_get_dev_serial(serial);

    return *dev_addr;
}

unsigned char uc_wiota_gateway_get_allow_run_flag(void)
{
    return gateway_mode.allow_run_flag;
}

void uc_wiota_gateway_set_allow_run_flag(unsigned char flag)
{
    gateway_mode.allow_run_flag = flag;
}

static void *uc_wiota_gateway_create_queue(const char *name, unsigned int max_msgs, unsigned char flag)
{
    rt_mq_t mq;
    void *msgpool;

    mq = rt_malloc(sizeof(struct rt_messagequeue));
    if (RT_NULL == mq)
    {
        return RT_NULL;
    }

    msgpool = rt_malloc(4 * max_msgs);
    if (RT_NULL == msgpool)
    {
        rt_free(mq);
        return RT_NULL;
    }

    if (RT_EOK != rt_mq_init(mq, name, msgpool, 4, 4 * max_msgs, flag))
    {
        rt_free(mq);
        rt_free(msgpool);
        return RT_NULL;
    }

    return (void *)mq;
}

static int uc_wiota_gateway_recv_queue(void *queue, void **buf, signed int timeout)
{
    unsigned int address = 0;
    int result = 0;

    if (queue == RT_NULL)
    {
        rt_kprintf("gw queue null\n");
        return RT_ERROR;
    }

    result = rt_mq_recv(queue, &address, 4, timeout);
    *buf = (void *)address;

    return result;
}

static int uc_wiota_gateway_send_queue(void *queue, void *buf, signed int timeout)
{
    unsigned int address = (unsigned int)buf;

    if (queue == RT_NULL)
    {
        rt_kprintf("gw send null\n");
        return 0;
    }

    return rt_mq_send_wait(queue, &address, 4, timeout);
}

static int uc_wiota_gateway_dele_queue(void *queue)
{
    rt_err_t ret = rt_mq_detach(queue);
    rt_free(((rt_mq_t)queue)->msg_pool);
    rt_free(queue);
    return ret;
}

static void uc_wiota_gateway_send_exit(void)
{
    static uc_wiota_gateway_msg_t recv_data = {0};
    recv_data.msg_type = GATEWAY_MAG_CODE_EXIT;

    if (RT_EOK != uc_wiota_gateway_send_queue(gateway_mode.gateway_mq, &recv_data, 0))
    {
        rt_kprintf("gw send queue err\n");
    }
}

static void uc_wiota_gateway_wait_exit(void)
{
    if (gateway_mode.gateway_task_run != 1)
    {
        return;
    }

    while (1)
    {
        if (gateway_mode.exit_flag == 1)
        {
            break;
        }
        rt_thread_delay(GATEWAY_WAIT_QUEUE_TIMEOUT);
    }
}

static void uc_wiota_gateway_send_msg_to_queue(int msg_code)
{
    static uc_wiota_gateway_msg_t recv_data = {0};
    recv_data.msg_type = msg_code;

    if (RT_EOK != uc_wiota_gateway_send_queue(gateway_mode.gateway_mq, &recv_data, 0))
    {
        rt_kprintf("gw send queue err\n");
    }
}

static void uc_wiota_gateway_recv_data_callback(uc_recv_back_p recv_data)
{
    // unsigned char *msg_data = RT_NULL;
    uc_wiota_gateway_msg_t *recv_data_buf = RT_NULL;

    recv_data_buf = rt_malloc(sizeof(uc_wiota_gateway_msg_t));
    if (recv_data_buf == NULL)
    {
        rt_kprintf("gw malloc fail\n");
        return;
    }

    recv_data_buf->data = recv_data->data;
    recv_data_buf->data_len = recv_data->data_len;
    recv_data_buf->msg_type = GATEWAY_MAG_CODE_DL_RECV;
    recv_data_buf->recv_msg_type = recv_data->type;

    if (gateway_mode.gateway_mq == RT_NULL)
    {
        rt_kprintf("gw mq null\n");
        return;
    }

    if (RT_EOK != uc_wiota_gateway_send_queue(gateway_mode.gateway_mq, recv_data_buf, 10))
    {
        rt_free(recv_data->data);
        rt_free(recv_data_buf);
        rt_kprintf("gw send queue fail\n");
    }

    // rt_free(recv_data->data);
}

static int uc_wiota_gateway_send_ps_cmd_data(unsigned char *data,
                                             int len,
                                             app_ps_header_t *ps_header)
{
    int ret = 0;
    unsigned char *cmd_coding = data;
    unsigned int cmd_coding_len = len;
    unsigned char *data_coding = RT_NULL;
    unsigned int data_coding_len = 0;
    uc_op_result_e send_result = 0;

    ret = app_data_coding(ps_header, cmd_coding, cmd_coding_len, &data_coding, &data_coding_len);
    if (0 != ret)
    {
        rt_kprintf("gw coding fail %d\n", ret);
        return 0;
    }
    send_result = uc_wiota_send_data(data_coding, data_coding_len, 60000, RT_NULL);
    if (send_result != UC_OP_SUCC)
    {
        rt_kprintf("gw send fail %d\n", send_result);
        rt_free(data_coding);
        return 0;
    }

    rt_free(data_coding);

    return 1;
}

static void uc_wiota_gateway_handle_ota_recv_timer_msg(void *para)
{
    if (UC_GW_CONNECT_SUCESS == wiota_gw_connect_flag)
    {
        uc_wiota_gateway_send_miss_data_req_to_queue();
    }
    // else
    //{
    //     rt_kprintf("uc_wiota_gateway_handle_ota_recv_timer_msg error.\n");
    // }
    //  uc_wiota_gateway_send_msg_to_queue(GATEWAY_MAG_CODE_OTA_REQ);
}

static int uc_wiota_gateway_send_auth_req(void)
{
    int ret = 0;
    app_ps_auth_req_t auth_req_data = {0};
    app_ps_header_t ps_header = {0};

    auth_req_data.auth_type = 0;
    rt_strncpy(auth_req_data.aut_code, gateway_mode.auth_code, rt_strlen(gateway_mode.auth_code));
    rt_strncpy((char *)auth_req_data.freq, (const char *)gw_freq_list, APP_CONNECT_FREQ_NUM);

    app_set_header_property(PRO_SRC_ADDR, 1, &ps_header.property);

    ps_header.addr.src_addr = uc_wiota_gateway_get_dev_address();
    ps_header.cmd_type = AUTHENTICATION_REQ;
    // ps_header.packet_num = app_packet_num();

    // wiota send data
    ret = uc_wiota_gateway_send_ps_cmd_data((unsigned char *)&auth_req_data, sizeof(app_ps_auth_req_t), &ps_header);
    return ret;
}

static void uc_gateway_check_freq(unsigned char *freq_list, unsigned char num)
{
    unsigned char old_freq[16] = {255};
    unsigned int flag = 0;

    uc_wiota_get_freq_list(old_freq);

    for (int i = 0; i < num; i++)
    {
        if (*(freq_list + i) == 0xFF || *(freq_list + i) == 0 ||
            old_freq[i] == 0xFF || old_freq[i] == 0)
        {
            if ((*(freq_list + i) != 0xFF && *(freq_list + i) != 0) ||
                (old_freq[i] != 0xFF && old_freq[i] != 0))
                flag = 1;

            break;
        }
        else if (*(freq_list + i) != old_freq[i])
        {
            flag = 1;
            break;
        }
    }

    if (flag)
    {
        uc_wiota_set_freq_list(freq_list, APP_MAX_FREQ_LIST_NUM);
    }
}

static void uc_wiota_gateway_auth_res_msg(unsigned char *data, unsigned int data_len, unsigned int gw_id)
{
    app_ps_auth_res_t *auth_res_data = RT_NULL;
    unsigned char freq_list[APP_MAX_FREQ_LIST_NUM] = {0};

    auth_res_data = (app_ps_auth_res_t *)data;

    rt_kprintf("gw autho res %d wiota_id 0x%x gw_id 0x%x\n",
               auth_res_data->connect_index.state, auth_res_data->wiota_id, gw_id);
    switch (auth_res_data->connect_index.state)
    {
    case AUTHENTICATION_SUC:
        rt_thread_delay(uc_wiota_get_frame_len() / 1000); // wait send ack.
        uc_wiota_gateway_set_wiota_id(auth_res_data->wiota_id);
        uc_wiota_gateway_set_time_slot_fn(auth_res_data->connect_index.time_slot_fn);
        uc_wiota_set_gateway_id(gw_id);
        uc_wiota_get_freq_list(freq_list);
        if (!rt_memcmp(freq_list, auth_res_data->freq_list, APP_MAX_FREQ_LIST_NUM))
        {
            uc_wiota_set_freq_list(auth_res_data->freq_list, APP_MAX_FREQ_LIST_NUM);
            // uc_wiota_save_static_info();
        }
        if (0 != auth_res_data->wiota_id)
        {
            uc_wiota_disconnect();
            uc_wiota_set_userid(&auth_res_data->wiota_id, (unsigned char)sizeof(auth_res_data->wiota_id));
            uc_wiota_connect();
            // rt_thread_delay(200);
            if (0 == uc_wiota_wait_sync(8000))
            {
                // report success
                uc_gateway_state = GATEWAY_NORMAL;
                uc_wiota_save_static_info();
            }
            else
            {
                uc_gateway_state = GATEWAY_FAILED;
            }
        }
        else
        {
            uc_gateway_state = GATEWAY_NORMAL;
        }

        if (RT_NULL != uc_wiota_gateway_exce_report)
            uc_wiota_gateway_exce_report(uc_gateway_state);

        break;

    case AUTHENTICATION_NO_DATA:
    case AUTHENTICATION_FAIL:
        uc_gateway_state = GATEWAY_FAILED;

        if (RT_NULL != uc_wiota_gateway_exce_report)
            uc_wiota_gateway_exce_report(uc_gateway_state);
        break;

    case AUTHENTICATION_INFO_CHANGE:
        uc_gateway_check_freq(auth_res_data->freq_list, APP_MAX_FREQ_LIST_NUM);
        uc_wiota_save_static_info();
        break;

    case AUTHENTICATION_RECONNECT:

        if (RT_NULL != uc_wiota_gateway_exce_report)
            uc_wiota_gateway_exce_report(uc_gateway_state);

        rt_thread_delay(uc_wiota_get_frame_len() / 1000);
        uc_wiota_disconnect();
        uc_wiota_set_freq_info(auth_res_data->connect_index.freq);
        uc_wiota_connect();
        if (0 == uc_wiota_wait_sync(8000))
        {
            uc_wiota_gateway_send_auth_req();
        }
        else
        {
            if (RT_NULL != uc_wiota_gateway_exce_report)
                uc_wiota_gateway_exce_report(GATEWAY_FAILED);
        }

        break;
    default:
        break;
    }
    uc_wiota_gateway_set_auth_state(auth_res_data->connect_index.state);

    // rt_free(cmd_decoding);
}

static unsigned char uc_wiota_gateway_check_if_upgrade_required(app_ps_ota_upgrade_req_t *ota_upgrade_req)
{
    unsigned char is_upgrade_range = FALSE;
    unsigned char is_required = FALSE;
    unsigned int dev_address = uc_wiota_gateway_get_dev_address();
    unsigned char version[16] = {0};
    unsigned char is_ex_mcu = FALSE;

    if (0 == rt_memcmp(ota_upgrade_req->device_type, EXTERN_MCU, rt_strlen(EXTERN_MCU))) // extern mcu upgarde)
    {
        is_ex_mcu = TRUE;
    }

    uc_wiota_get_version(version, RT_NULL, RT_NULL, RT_NULL);

    if (ota_upgrade_req->upgrade_range == 1)
    {
        for (int i = 0; i < APP_MAX_IOTE_UPGRADE_NUM; i++)
        {
            if (dev_address == ota_upgrade_req->iote_list[i])
            {
                if (gateway_mode.ota_state == GATEWAY_OTA_STOP)
                {
                    gateway_mode.ota_state = GATEWAY_OTA_DEFAULT;
                }

                is_upgrade_range = TRUE;
                break;
            }
        }
    }
    else if (ota_upgrade_req->upgrade_range == 0 && gateway_mode.ota_state != GATEWAY_OTA_STOP)
    {
        is_upgrade_range = TRUE;
    }

    if (is_upgrade_range)
    {
        if (is_ex_mcu ||
            (0 == rt_strncmp((char *)version, ota_upgrade_req->old_version, rt_strlen(ota_upgrade_req->old_version)) &&
             0 == rt_strncmp(gateway_mode.device_type, ota_upgrade_req->device_type, rt_strlen(ota_upgrade_req->device_type))))
        {
            if (GATEWAY_OTA_DEFAULT == gateway_mode.ota_state)
            {
                if (is_ex_mcu || (0 == gateway_mode.upgrade_num && 0 == ota_upgrade_req->packet_type))
                {
                    is_required = TRUE;
                    gateway_mode.upgrade_num = ota_upgrade_req->upgrade_num;
                }
            }
            else if (GATEWAY_OTA_DOWNLOAD == gateway_mode.ota_state)
            {
                if (gateway_mode.upgrade_num != ota_upgrade_req->upgrade_num)
                {
                    rt_kprintf("gw ota state %d\n", gateway_mode.ota_state);
                    uc_gateway_clear_state();
                    gateway_mode.upgrade_num = ota_upgrade_req->upgrade_num;
                }
                is_required = TRUE;
            }
        }
    }

    rt_kprintf("is_req %d, cur_v %s, old_v %s, new_v %s, is_range %d, upd_rang %d, upgrade_num %d/%d, packet_type %d\n",
               is_required, version, ota_upgrade_req->old_version, ota_upgrade_req->new_version,
               is_upgrade_range, ota_upgrade_req->upgrade_range, ota_upgrade_req->upgrade_num,
               gateway_mode.upgrade_num, ota_upgrade_req->packet_type);

    return is_required;
}

static unsigned char uc_wiota_gateway_whether_the_ota_upgrade_data_is_recved(void)
{
    unsigned int offset = 0;
    unsigned int block_count = 0;

    for (; offset < gateway_mode.block_count; offset++)
    {
        if (0x1 == JUDGMENT_BIT(gateway_mode.mask_map[offset / 8], offset % 8))
        {
            block_count++;
        }
    }

    rt_kprintf("gw d %d/%d\n", block_count, gateway_mode.block_count);

    if (block_count >= gateway_mode.block_count)
    {
        rt_kprintf("ota data recv end\n");
        return TRUE;
    }

    return FALSE;
}

static void uc_wiota_gateway_send_miss_data_req_to_queue(void)
{
    // rt_kprintf("GATEWAY ota timer request run flag %d\n", gateway_mode.miss_data_req);
    //  if(!gateway_mode.miss_data_req)
    //{
    uc_wiota_gateway_send_msg_to_queue(GATEWAY_MAG_CODE_UL_MISS_DATA_REQ);
    // gateway_mode.miss_data_req = RT_TRUE;
    // }
}

static void uc_wiota_gateway_ota_upgrade_res_msg(unsigned char *data, unsigned int data_len)
{
    app_ps_ota_upgrade_req_t *ota_upgrade_req = (app_ps_ota_upgrade_req_t *)data;
    int bin_size = 0;
    int reserved_size = 0;
    int ota_size = 0;
    int file_size = ota_upgrade_req->file_size;

    get_partition_size(&bin_size, &reserved_size, &ota_size);
    rt_kprintf("bin size %d reserved_size %d ota_size %d\n", bin_size, reserved_size, ota_size);
    if (ota_size < file_size)
    {
        rt_kprintf("ota pk too large, %d/%d\n", file_size, ota_size);
        return;
    }
    gateway_mode.miss_data_invalid_req = 0;

    if (uc_wiota_gateway_check_if_upgrade_required(ota_upgrade_req))
    {
        if (gateway_mode.ota_state == GATEWAY_OTA_DEFAULT)
        {
            gateway_mode.mcs_len = ota_upgrade_req->mcs_len;
            gateway_mode.block_size = GATEWAY_OTA_BLOCK_SIZE;
            gateway_mode.block_count = file_size / GATEWAY_OTA_BLOCK_SIZE;
            if (file_size % GATEWAY_OTA_BLOCK_SIZE)
            {
                gateway_mode.block_count++;
            }

            uc_wiota_ota_flash_erase(bin_size + reserved_size, file_size);
            gateway_mode.ota_state = GATEWAY_OTA_DOWNLOAD;
            gateway_mode.upgrade_type = ota_upgrade_req->upgrade_type;
            rt_strncpy(gateway_mode.new_version, ota_upgrade_req->new_version, rt_strlen(ota_upgrade_req->new_version));
            uc_gateway_control_ota_timer(ota_upgrade_req->upgrade_time);
            // gateway_mode.miss_data_req = FALSE;

            // rt_kprintf("file_size %d, mcs_len %d, block_size %d, block_count %d, ota_state %d, upgrade_type %d, upgrade_time %d\n",
            //            file_size, gateway_mode.mcs_len, gateway_mode.block_size, gateway_mode.block_count, gateway_mode.ota_state,
            //            gateway_mode.upgrade_type, ota_upgrade_req->upgrade_time);
        }

        if (gateway_mode.ota_state == GATEWAY_OTA_DOWNLOAD)
        {
            unsigned int offset = ota_upgrade_req->data_offset / GATEWAY_OTA_BLOCK_SIZE;

            if (0x0 == JUDGMENT_BIT(gateway_mode.mask_map[offset / 8], offset % 8))
            {
                uc_wiota_ota_flash_write(ota_upgrade_req->data, bin_size + reserved_size + ota_upgrade_req->data_offset, ota_upgrade_req->data_length);
                SET_BIT(gateway_mode.mask_map[offset / 8], offset % 8);
                rt_kprintf("gw d offset %d mask[%d] 0x%x\n", offset, offset / 8, gateway_mode.mask_map[offset / 8]);
            }

            if (uc_wiota_gateway_whether_the_ota_upgrade_data_is_recved())
            {
                uc_gateway_stop_ota_timer();
                if (0 == uc_wiota_ota_check_flash_data(bin_size + reserved_size, file_size, ota_upgrade_req->md5))
                {
                    // internal upgrade
                    if (0 != rt_memcmp(ota_upgrade_req->device_type, EXTERN_MCU, rt_strlen(EXTERN_MCU)))
                    {
                        wiota_gw_connect_flag = UC_GW_DISCONNECT;

                        gateway_mode.ota_state = GATEWAY_OTA_PROGRAM;
                        rt_kprintf("gw jump\n");
                        // wiota_gateway_mode_flag = RT_FALSE;
                        //  set_partition_size(GATEWAY_OTA_FLASH_BIN_SIZE, GATEWAY_OTA_FLASH_REVERSE_SIZE, GATEWAY_OTA_FLASH_OTA_SIZE);
                        uc_wiota_ota_jump_program(file_size, ota_upgrade_req->upgrade_type);
                    }
                    else
                    {
                        if (uc_wiota_gateway_ex_mcu)
                        {
                            gateway_mode.ota_state = GATEWAY_OTA_DEFAULT;
                            rt_memset(gateway_mode.mask_map, 0, sizeof(gateway_mode.mask_map));
                            uc_wiota_gateway_ex_mcu(bin_size + reserved_size, file_size, ota_upgrade_req->md5);
                        }
                    }
                }
                else
                {
                    rt_kprintf("gw up f\n");
                    uc_gateway_clear_state();
                }
            }
        }
    }

    // rt_free(cmd_decoding);
}

static void uc_wiota_gateway_ota_upgrade_stop_msg(unsigned char *data, unsigned int data_len)
{
    // unsigned char *cmd_decoding = RT_NULL;
    app_ps_ota_upgrade_stop_t *ota_upgrade_stop = RT_NULL;
    unsigned int dev_address = uc_wiota_gateway_get_dev_address();

    ota_upgrade_stop = (app_ps_ota_upgrade_stop_t *)data;

    for (int i = 0; i < APP_MAX_IOTE_UPGRADE_STOP_NUM; i++)
    {
        rt_kprintf("iote_list 0x%x\n", ota_upgrade_stop->iote_list[i]);
        if (dev_address == ota_upgrade_stop->iote_list[i])
        {
            if (gateway_mode.ota_state == GATEWAY_OTA_DOWNLOAD)
            {
                uc_gateway_stop_ota_timer();
            }
            gateway_mode.ota_state = GATEWAY_OTA_STOP;
            rt_kprintf("0x%x stop ota\n", dev_address);
            break;
        }
    }

    // rt_free(cmd_decoding);
}

static void uc_wiota_gateway_ota_upgrade_state_msg(unsigned char *data, unsigned int data_len)
{
    // unsigned char *cmd_decoding = RT_NULL;
    app_ps_ota_upgrade_state_t *ota_upgrade_state = (app_ps_ota_upgrade_state_t *)data;
    unsigned char version[16] = {0};

    uc_wiota_get_version(version, RT_NULL, RT_NULL, RT_NULL);

    if (0 == rt_strncmp(ota_upgrade_state->old_version, (const char *)version, rt_strlen((const char *)version)) &&
        0 == rt_strncmp(ota_upgrade_state->new_version, gateway_mode.new_version, rt_strlen(gateway_mode.new_version)) &&
        0 == rt_strncmp(ota_upgrade_state->device_type, gateway_mode.device_type, rt_strlen(gateway_mode.device_type)) &&
        ota_upgrade_state->upgrade_type == gateway_mode.upgrade_type)
    {
        if (ota_upgrade_state->process_state == GATEWAY_OTA_END && gateway_mode.ota_state == GATEWAY_OTA_DOWNLOAD)
        {
            rt_kprintf("gw end cmd\n");
            // uc_gateway_stop_ota_timer();
            for (int offset = 0; offset < gateway_mode.block_count; offset++)
            {
                if (0x0 == JUDGMENT_BIT(gateway_mode.mask_map[offset / 8], offset % 8))
                {
                    gateway_mode.miss_data_num++;
                }
            }

            if (gateway_mode.miss_data_num > 0)
            {
                // rt_kprintf("upgrade_state_msg set timer %d\n", timeout);
                uc_gateway_control_ota_timer(uc_gateway_get_miss_data_timeout());

                gateway_mode.miss_data_invalid_req = 0;
            }
        }
    }

    // rt_free(cmd_decoding);
}

static void uc_wiota_gateway_recv_dl_msg(unsigned char *data, unsigned int data_len, int response_flag, unsigned char data_type)
{
    if (!response_flag)
    {
        if (uc_wiota_gateway_user_recv != RT_NULL)
        {
            uc_wiota_gateway_user_recv(data, data_len, data_type);
        }
    }
}

static void uc_wiota_gateway_recv_rtc_res_msg(unsigned char *data)
{
    time_t *time = (time_t *)data;

    if (uc_wiota_gateway_rtc && rtc_fmt != 0xff)
    {
        uc_wiota_gateway_rtc(rtc_fmt, *time);
        if (rtc_sem)
        {
            rt_sem_release(rtc_sem);
        }
    }
}

static int uc_wiota_gateway_version_msg(void)
{
    app_ps_iote_version_t iote_ver = {0};
    app_ps_header_t ps_header = {0};
    unsigned char gw_verity = uc_wiota_gateway_get_verity();

    if (UC_GW_CONNECT_SUCESS == wiota_gw_connect_flag)
    {
        iote_ver.dev_id = uc_wiota_gateway_get_dev_address();
        uc_wiota_get_version(iote_ver.version, RT_NULL, RT_NULL, RT_NULL);

        ps_header.cmd_type = IOTE_VERSION_RES;
        if (gw_verity)
        {
            app_set_header_property(PRO_DEST_ADDR, 1, &ps_header.property);
            ps_header.addr.dest_addr = uc_wiota_get_gateway_id();
        }
        return uc_wiota_gateway_send_ps_cmd_data((unsigned char *)&iote_ver,
                                                 sizeof(app_ps_iote_version_t),
                                                 &ps_header);
    }

    return 0;
}

static void uc_wiota_gateway_analisys_dl_msg(unsigned char *data, unsigned int data_len, unsigned char data_type)
{
    app_ps_header_t ps_header = {0};
    unsigned char *data_decoding = RT_NULL;
    unsigned int data_decoding_len = 0;
    unsigned char gw_verity = uc_wiota_gateway_get_verity();

    if (0 != app_data_decoding(data, data_len, &data_decoding, &data_decoding_len, &ps_header))
    {
        rt_kprintf("gw decode fail\n");
        return;
    }

    rt_kprintf("gw recv cmd %d.\n", ps_header.cmd_type);
    if (gw_verity && ps_header.cmd_type != AUTHENTICATION_RES)
    {
        if (ps_header.addr.src_addr != uc_wiota_get_gateway_id())
        {
            rt_kprintf("gw recv other gw 0x%x msg\n", ps_header.addr.src_addr);
            rt_free(data_decoding);
            return;
        }
    }
    switch (ps_header.cmd_type)
    {
    case AUTHENTICATION_RES:
        uc_wiota_gateway_auth_res_msg(data_decoding, data_decoding_len, ps_header.addr.src_addr);
        break;

    case OTA_UPGRADE_REQ:
        uc_wiota_gateway_ota_upgrade_res_msg(data_decoding, data_decoding_len);
        break;

    case OTA_UPGRADE_STOP:
        uc_wiota_gateway_ota_upgrade_stop_msg(data_decoding, data_decoding_len);
        break;

    case OTA_UPGRADE_STATE:
        uc_wiota_gateway_ota_upgrade_state_msg(data_decoding, data_decoding_len);
        break;

    case IOTE_GET_RTC:
        uc_wiota_gateway_recv_rtc_res_msg(data_decoding);
        break;

    case IOTE_USER_DATA:
        uc_wiota_gateway_recv_dl_msg(data_decoding, data_decoding_len, ps_header.property.response_flag, data_type);
        break;

    case IOTE_VERSION_REQ:
        uc_wiota_gateway_version_msg();
        break;

    default:
        break;
    }

    rt_free(data_decoding);
}

static int uc_wiota_gateway_send_ota_req_msg(void)
{
    app_ps_version_verify_t version_verify = {0};
    app_ps_header_t ps_header = {0};
    unsigned char version[16] = {0};
    unsigned char gw_verity = uc_wiota_gateway_get_verity();

    if (UC_GW_CONNECT_SUCESS == wiota_gw_connect_flag)
    {
        uc_wiota_get_version(version, RT_NULL, RT_NULL, RT_NULL);

        rt_strncpy(version_verify.software_version, (const char *)version, rt_strlen((const char *)version));
        uc_wiota_get_hardware_ver((unsigned char *)version_verify.hardware_version);
        rt_strncpy(version_verify.device_type, gateway_mode.device_type, rt_strlen(gateway_mode.device_type));

        // app_set_header_property(PRO_SRC_ADDR, 1, &ps_header.property);
        // ps_header.addr.src_addr = uc_wiota_gateway_get_dev_address();
        ps_header.cmd_type = VERSION_VERIFY;
        // ps_header.packet_num = app_packet_num();
        if (gw_verity)
        {
            app_set_header_property(PRO_DEST_ADDR, 1, &ps_header.property);
            ps_header.addr.dest_addr = uc_wiota_get_gateway_id();
        }
        return uc_wiota_gateway_send_ps_cmd_data((unsigned char *)&version_verify,
                                                 sizeof(app_ps_version_verify_t),
                                                 &ps_header);
    }

    return 0;
}

static void uc_wiota_gateway_ota_miss_data_req_msg(void)
{
    app_ps_header_t ps_header = {0};
    app_ps_iote_missing_data_req_t miss_data_req = {0};
    unsigned char version[16] = {0};
    unsigned char gw_verity = uc_wiota_gateway_get_verity();
    static unsigned char miss_data_continue_couter = 0;

    if (gateway_mode.miss_data_invalid_req < 8)
    {
        gateway_mode.miss_data_invalid_req++;
    }
    else
    {
        uc_gateway_stop_ota_timer();
        // rt_kprintf("gw misi  exce %d\n", gateway_mode.miss_data_invalid_req);
        return;
    }

    uc_wiota_get_version(version, RT_NULL, RT_NULL, RT_NULL);

    for (int offset = 0; offset < gateway_mode.block_count; offset++)
    {
        if (0x0 == JUDGMENT_BIT(gateway_mode.mask_map[offset / 8], offset % 8))
        {
            miss_data_req.miss_data_offset[miss_data_req.miss_data_num] = offset * GATEWAY_OTA_BLOCK_SIZE;
            miss_data_req.miss_data_length[miss_data_req.miss_data_num] = GATEWAY_OTA_BLOCK_SIZE;
            rt_kprintf("missOf[%d] %d, missLen[%d] %d\n",
                       miss_data_req.miss_data_num, miss_data_req.miss_data_offset[miss_data_req.miss_data_num],
                       miss_data_req.miss_data_num, miss_data_req.miss_data_length[miss_data_req.miss_data_num]);
            miss_data_req.miss_data_num++;
            if (miss_data_req.miss_data_num == APP_MAX_MISSING_DATA_BLOCK_NUM - 1)
            {
                break;
            }
        }
    }

    miss_data_req.upgrade_type = gateway_mode.upgrade_type;
    rt_strncpy(miss_data_req.device_type, gateway_mode.device_type, rt_strlen(gateway_mode.device_type));
    rt_strncpy(miss_data_req.new_version, gateway_mode.new_version, rt_strlen(gateway_mode.new_version));
    rt_strncpy(miss_data_req.old_version, (const char *)version, rt_strlen((const char *)version));

    // app_set_header_property(PRO_SRC_ADDR, 1, &ps_header.property);
    // ps_header.addr.src_addr = uc_wiota_gateway_get_dev_address();
    ps_header.cmd_type = IOTE_MISSING_DATA_REQ;
    // ps_header.packet_num = app_packet_num();
    if (gw_verity)
    {
        app_set_header_property(PRO_DEST_ADDR, 1, &ps_header.property);
        ps_header.addr.dest_addr = uc_wiota_get_gateway_id();
    }
    if (0 == uc_wiota_gateway_send_ps_cmd_data((unsigned char *)&miss_data_req,
                                               sizeof(app_ps_iote_missing_data_req_t),
                                               &ps_header))
    { // fail
        if (miss_data_continue_couter < 3)
        {
            uc_gateway_control_ota_timer(uc_gateway_get_miss_data_timeout());
            miss_data_continue_couter++;
        }
        else
        {
            uc_gateway_stop_ota_timer();
            miss_data_continue_couter = 0;
        }
        rt_kprintf("gw miss fail,ct %d\n", miss_data_continue_couter);
    }
    else
    { // succ
        unsigned int timeout = (uc_gateway_calc_single_otapage_time() + 1) * (miss_data_req.miss_data_num + 1);
        // rt_kprintf("miss req timer %d\n", timeout);
        uc_gateway_control_ota_timer(timeout);

        miss_data_continue_couter = 0;
        rt_kprintf("gw miss req suc\n");
    }
}

int uc_gateway_get_random(void)
{
    int random_num = 0;
    static unsigned int *ptr = (unsigned int *)(0x1a107080);

    REG(0x1a10a02c) &= ~(1 << 4);
    random_num = *ptr;
    random_num = *ptr;
    random_num = *ptr;
    random_num = *ptr;
    if (random_num < 0)
    {
        random_num = (~random_num) + 1;
    }
    REG(0x1a10a02c) |= (1 << 4);
    return random_num;
}

typedef union
{
    unsigned int data;
    struct
    {
        unsigned int reserve : 2;
        unsigned int slotIdx : 3;
        unsigned int burstIdx : 3;
        unsigned int groupIdx : 3;
    } dev_pos;
} scramble_u;

static unsigned int uc_query_scrambleid_by_userid(unsigned int id, unsigned int scb0, unsigned int scb1)
{
    unsigned int scramble_id = 0;
    unsigned int hi, lo, b0, b1;

    /* x0 = ((id[31] shr 1) xor r1)[31:0] */
    unsigned int x0 = (((id & 0x80000000) >> 1) ^ scb1) & 0x7FFFFFFF;
    /* x1 = (id xor r0)[31:0] */
    unsigned int x1 = (id ^ scb0) & 0x7FFFFFFF;
    /* length = (id[7:0] + (id[23:16] shr 16))[7:0] + 20 */
    int length = (((id & 0x7F) + ((id & 0x7F0000) >> 16)) & 0x7F) + 20;

    for (int i = 0; i < length; ++i)
    {
        /* loop xor on x0 */
        /* ------------------------------------------- */
        /*   30  29  28  27  ...  03 | 02  01  00  31' */
        /* ^ 27  26  25  24  ...  00 | 31' 30' 29' 28' */
        /* ------------------------------------------- */
        /*   31' 30' 29' 28' ...  04'| 03' 02' 01' 00' */
        /*  |<----------hi---------->|<------lo----->| */
        hi = (x0 & 0x7FFFFFF8) << 1;
        hi ^= (x0 & 0x0FFFFFFF) << 4;
        lo = ((x0 & 0x7) << 1) | ((hi >> 31) & 0x1);
        lo ^= ((x0 & 0x0) << 4) | ((hi >> 28) & 0xF);
        x0 = hi | lo;
        /* loop xor on x1 */
        /* ------------------------------------------- */
        /*   30  29  28  27  ...  03 | 02  01  00  31' */
        /*   29  28  27  26  ...  02 | 01  00  31' 30' */
        /*   28  27  26  25  ...  01 | 00  31' 30' 29' */
        /* ^ 27  26  25  24  ...  00 | 31' 30' 29' 28' */
        /* ------------------------------------------- */
        /*   31' 30' 29' 28' ...  04'| 03' 02' 01' 00' */
        /*  |<----------hi---------->|<------lo----->| */
        hi = (x1 & 0x7FFFFFF8) << 1;
        hi ^= (x1 & 0x3FFFFFFC) << 2;
        hi ^= (x1 & 0x1FFFFFFE) << 3;
        hi ^= (x1 & 0x0FFFFFFF) << 4;
        lo = ((x1 & 0x7) << 1) | ((hi >> 31) & 0x1);
        lo ^= ((x1 & 0x3) << 2) | ((hi >> 30) & 0x3);
        lo ^= ((x1 & 0x1) << 3) | ((hi >> 29) & 0x7);
        lo ^= ((x1 & 0x0) << 4) | ((hi >> 28) & 0xF);
        x1 = hi | lo;
    }

    /* b0 = x0[30] ^ x0[27] */
    b0 = ((x0 >> 30) ^ (x0 >> 27)) & 0x1;
    /* x0 = (x0[31:0] shl 1) + b0 */
    x0 = (x0 << 1) | b0;
    /* b1 = x1[30] ^ x1[29] ^ x1[28] ^ x1[27] */
    b1 = ((x1 >> 30) ^ (x1 >> 29) ^ (x1 >> 28) ^ (x1 >> 27)) & 0x1;
    /* x1 = (x1[31:0] shl 1) + b1 */
    x1 = (x1 << 1) | b1;

    scramble_id = x0 ^ x1;

    return scramble_id;
}

unsigned int uc_gateway_query_dev_pos_by_userid(unsigned int user_id)
{
    sub_system_config_t config = {0};
    unsigned int scramble_id = 0;
    unsigned int scb0 = 0;
    unsigned int scb1 = 0;
    unsigned char ul_group_num = 0;
    unsigned char group_idx = 0, burst_idx = 0, slot_idx = 0;
    unsigned int dev_id = user_id;
    unsigned int dfe = 0;

    uc_wiota_get_system_config(&config);
    ul_group_num = 1 << config.group_number;
    uc_wiota_get_userid_scb(&scb0, &scb1);
    scramble_id = uc_query_scrambleid_by_userid(dev_id, scb0, scb1);

    scramble_u scramble = {data : scramble_id};
    group_idx = scramble.dev_pos.groupIdx & (ul_group_num - 1);
    burst_idx = scramble.dev_pos.burstIdx;
    slot_idx = scramble.dev_pos.slotIdx;

    if ((group_idx == 0 && burst_idx == 0) || (group_idx == (ul_group_num - 1) && burst_idx == 7 && slot_idx == 7))
    {
        dfe = uc_wiota_get_curr_rf_cnt();
        dev_id &= 0xf000ffff;
        dev_id = (dev_id | (1 << 31)) | ((dfe & 0xfff) << 16) | (dev_id & 0xffff);
        rt_kprintf("regen dev_id 0x%x to 0x%x\n", user_id, dev_id);
        dev_id = uc_gateway_query_dev_pos_by_userid(dev_id);
    }

    return dev_id;
}

static int uc_wiota_gateway_set_id(void)
{
    unsigned int dev_id = 0;
    unsigned int userid = 0;
    unsigned char len = 4;
    uint8_t awakened_cause = 0;
    uint8_t is_cs_awakened = 0;

    awakened_cause = uc_wiota_get_awakened_cause(&is_cs_awakened);
    if (awakened_cause == AWAKENED_CAUSE_SLEEP ||
        awakened_cause == AWAKENED_CAUSE_PAGING)
    {
        return 0;
    }

    dev_id = uc_wiota_gateway_get_dev_address();

    if (dev_id == 0 || dev_id == (~0))
    {
        rt_kprintf("rand id\n");
        uc_gateway_get_random();
        uc_gateway_get_random();
        userid = uc_gateway_get_random() & 0x7FFFFFFF;
    }

    dev_id = uc_gateway_query_dev_pos_by_userid(dev_id);
    // rt_kprintf("user id 0x%x\n", dev_id);
    uc_wiota_get_userid(&userid, &len);

    if (dev_id != userid)
    {
        rt_kprintf("gw reset id\n");
        // dis connect
        uc_wiota_disconnect();
        uc_wiota_set_userid(&dev_id, 4);
        // connect
        uc_wiota_connect();

        return uc_wiota_wait_sync(5000);
    }

    return 0;
}

int uc_gatewaytest(void)
{
    if (RT_NULL != gateway_mode.gateway_handler)
    {
        if (gateway_mode.gateway_handler->type == 1)
            return 0;
        return 1;
    }

    return 0;
}

static int uc_wiota_gw_task(void)
{
    if (!uc_wiota_get_auto_connect_flag())
    {
        gateway_mode.gateway_handler = rt_thread_create("gateway",
                                                        uc_wiota_gateway_api_handle,
                                                        RT_NULL,
                                                        1024 + 256,
                                                        3,
                                                        3);
        if (gateway_mode.gateway_handler == RT_NULL)
        {
            rt_kprintf("creat gw fail\n");
            return UC_CREATE_TASK_FAIL;
        }

        // start task
        rt_thread_startup(gateway_mode.gateway_handler);
    }
    else
    {
        // set flag
        uc_wiota_gateway_set_allow_run_flag(RT_TRUE);
        rt_thread_delay(1);
    }

    return UC_GATEWAY_OK;
}

static int uc_wiota_gateway_auth(void)
{
    uc_wiota_gateway_msg_t *recv_data = RT_NULL;
    unsigned int remember_tick = 0;
    unsigned char is_auth_res = 0;

    if (!uc_wiota_gateway_send_auth_req())
    {
        rt_kprintf("gw auth req err\n");
        return 1; // fail1
    }

    remember_tick = rt_tick_get();
    while (rt_tick_get() - remember_tick < GATEWAY_WAIT_AUTH_TIMEOUT)
    {
        if (RT_EOK != uc_wiota_gateway_recv_queue(gateway_mode.gateway_mq,
                                                  (void **)&recv_data,
                                                  GATEWAY_WAIT_AUTH_TIMEOUT))
        {
            rt_kprintf("wait que timeout\n");
            return 2; // timeout
        }

        // parsing data
        if (GATEWAY_MAG_CODE_DL_RECV == recv_data->msg_type)
        {
            app_ps_header_t ps_header = {0};
            unsigned char *data_decoding = RT_NULL;
            unsigned int data_decoding_len = 0;

            if (0 != app_data_decoding(recv_data->data, recv_data->data_len, &data_decoding, &data_decoding_len, &ps_header))
            {
                rt_kprintf("gw decode fail\n");
                return 3;
            }

            if (ps_header.cmd_type == AUTHENTICATION_RES)
            {
                uc_wiota_gateway_auth_res_msg(data_decoding, data_decoding_len, ps_header.addr.src_addr);
                is_auth_res = 1;
            }
            rt_free(data_decoding);
            rt_free(recv_data->data);
            rt_free(recv_data);

            if (is_auth_res)
            {
                break;
            }
        }
    }

    if (uc_gateway_state != GATEWAY_NORMAL)
    {
        return 4;
    }

    return 0; // success
}

int uc_wiota_gateway_start(uc_gatway_mode_e mode, char *auth_key, unsigned char freq_list[APP_CONNECT_FREQ_NUM])
{
    unsigned char re_auth_count = 3;

    if (!wiota_gw_connect_flag &&
        (mode == UC_TRANSMISSION_MODE ||
         mode == UC_GATEWAY_MODE))
    {
        if ((uc_wiota_get_state() != UC_STATUS_SYNC))
        {
            rt_kprintf("gw no conn\n");
            return UC_GATEWAY_NO_CONNECT;
        }

        if (auth_key == NULL)
        {
            rt_kprintf("no auth code\n");
            return UC_GATEWAY_NO_KEY;
        }

        if (uc_wiota_gateway_set_id())
        {
            rt_kprintf("gw set id err\n");
            return UC_GATEWAY_NO_CONNECT;
        }

        // init para
        unsigned char gw_verity = gateway_mode.gw_verity;
        rt_memset(&gateway_mode, 0, sizeof(uc_wiota_gateway_api_mode_t));
        gateway_mode.gw_verity = gw_verity;
        rt_strncpy(gateway_mode.auth_code, auth_key, rt_strlen(auth_key));
        rt_strncpy(gateway_mode.device_type, "iote", 4);
        gateway_mode.gateway_mode = RT_TRUE;
        gateway_mode.auth_state = AUTHENTICATION_FAIL;

        // init queue
        gateway_mode.gateway_mq = uc_wiota_gateway_create_queue("gw_mq", 8, RT_IPC_FLAG_PRIO);
        if (gateway_mode.gateway_mq == RT_NULL)
        {
            rt_kprintf("create gw_mq err\n");
            return UC_CREATE_QUEUE_FAIL;
        }

        uc_wiota_set_crc(1);
        // register callback
        uc_wiota_register_recv_data_callback(uc_wiota_gateway_recv_data_callback, UC_CALLBACK_NORAMAL_MSG);
        wiota_gw_connect_flag = UC_GW_ATTEMPT_CONNECT;
        if (mode == UC_GATEWAY_MODE)
        {
            while (re_auth_count)
            {
                if ((uc_wiota_get_state() != UC_STATUS_SYNC))
                {
                    return UC_GATEWAY_AUTO_FAIL;
                }
                re_auth_count--;
                if (0 == uc_wiota_gateway_auth())
                {
                    break;
                }
            }

            if (0 == re_auth_count)
            {
                return UC_GATEWAY_AUTO_FAIL;
            }
        }

        wiota_gw_connect_flag = UC_GW_CONNECT_SUCESS;

        // wiota_gateway_mode_flag = RT_TRUE;

        gateway_mode.ota_timer = rt_timer_create("t_ota",
                                                 uc_wiota_gateway_handle_ota_recv_timer_msg,
                                                 RT_NULL,
                                                 10000, // tick
                                                 RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
        if (gateway_mode.ota_timer == RT_NULL)
        {
            uc_wiota_gateway_dele_queue(gateway_mode.gateway_mq);
            // rt_kprintf("uc_wiota_gateway_start req ota timer memory failed.\n");
            return UC_CREATE_MISSTIMER_FAIL;
        }

        if (UC_GATEWAY_OK != uc_wiota_gw_task())
        {
            uc_wiota_gateway_dele_queue(gateway_mode.gateway_mq);
            rt_timer_delete(gateway_mode.ota_timer);
            rt_kprintf("gw creat task fail\n");
            return UC_CREATE_TASK_FAIL;
        }

        if (RT_NULL != freq_list)
            rt_memcpy(gw_freq_list, freq_list, APP_CONNECT_FREQ_NUM);

        if (mode == UC_TRANSMISSION_MODE)
        {
            uc_gateway_state = GATEWAY_NORMAL;
        }
    }
    else
    {
        rt_kprintf("gw flag  err\n");
        return UC_GATEWAY_OTHER_FAIL;
    }

    return UC_GATEWAY_OK;
}

int uc_wiota_gateway_send_data(void *data, unsigned int data_len, unsigned int timeout)
{
    int ret = 0;
    unsigned char *data_coding = RT_NULL;
    unsigned int data_coding_len = 0;
    app_ps_header_t ps_header = {0};
    uc_op_result_e send_result = UC_OP_FAIL;
    unsigned char gw_verity = uc_wiota_gateway_get_verity();

    if (data_len > GATEWAY_SEND_MAX_LEN)
    {
        rt_kprintf("gw send err.len %d\n", data_len);
        return 0;
    }

    if (UC_STATUS_SYNC != uc_wiota_get_state())
    {
        rt_kprintf("gw  wiota state err\n");
        return 0;
    }

    if (GATEWAY_NORMAL != uc_wiota_gateway_get_state() ||
        gateway_mode.ota_state == GATEWAY_OTA_PROGRAM)
    {
        rt_kprintf("gw state err\n");
        return 0;
    }

    if (UC_GW_CONNECT_SUCESS == wiota_gw_connect_flag)
    {
        // app_set_header_property(PRO_SRC_ADDR, 1, &ps_header.property);
        // app_set_header_property(PRO_NEED_RES, 1, &ps_header.property);  //need response
        ps_header.cmd_type = IOTE_USER_DATA;
        if (gw_verity)
        {
            app_set_header_property(PRO_DEST_ADDR, 1, &ps_header.property);
            ps_header.addr.dest_addr = uc_wiota_get_gateway_id();
        }
        // ps_header.addr.src_addr = uc_wiota_gateway_get_dev_address();
        // app_set_header_property(PRO_PACKET_NUM, 1, &ps_header.property);
        // ps_header.packet_num = app_packet_num();
        ret = app_data_coding(&ps_header, data, data_len, &data_coding, &data_coding_len);
        if (0 != ret)
        {
            rt_kprintf("gw code err %d\n", ret);
            return 0;
        }

        send_result = uc_wiota_send_data(data_coding, data_coding_len, timeout, RT_NULL);

        rt_free(data_coding);

        if (send_result == UC_OP_SUCC)
        {
            return 1;
        }
        else
            rt_kprintf("gw send fail %d\n", send_result);
    }

    return 0;
}

int uc_wiota_gateway_state_update_info_msg(void)
{
    app_ps_iote_state_update_t state_update_data = {0};
    app_ps_header_t ps_header = {0};
    radio_info_t radio = {0};
    char device_type[] = {"iote"};
    unsigned char gw_verity = uc_wiota_gateway_get_verity();

    if (UC_GW_CONNECT_SUCESS == wiota_gw_connect_flag)
    {
        uc_wiota_get_radio_info(&radio);

        rt_strncpy(state_update_data.device_type, device_type, rt_strlen(device_type));
        uc_wiota_get_version((unsigned char *)state_update_data.cur_version, RT_NULL, RT_NULL, RT_NULL);
        state_update_data.freq = uc_wiota_get_freq_info();

        state_update_data.rssi = radio.rssi;
        state_update_data.snr = radio.snr;
        state_update_data.cur_power = radio.cur_power;
        state_update_data.min_power = radio.min_power;
        state_update_data.max_power = radio.max_power;
        state_update_data.cur_mcs = radio.cur_mcs;
        state_update_data.max_mcs = radio.max_mcs;

        // app_set_header_property(PRO_SRC_ADDR, 1, &ps_header.property);
        ps_header.cmd_type = IOTE_STATE_UPDATE;
        // ps_header.addr.src_addr = uc_wiota_gateway_get_dev_address();
        // ps_header.packet_num = app_packet_num();
        if (gw_verity)
        {
            app_set_header_property(PRO_DEST_ADDR, 1, &ps_header.property);
            ps_header.addr.dest_addr = uc_wiota_get_gateway_id();
        }
        return uc_wiota_gateway_send_ps_cmd_data((unsigned char *)&state_update_data,
                                                 sizeof(app_ps_iote_state_update_t),
                                                 &ps_header);
    }

    return 0;
}

int uc_wiota_gateway_register_user_recv_cb(uc_wiota_gateway_user_recv_cb user_recv_cb, uc_wiota_gateway_state_report_cb user_get_state_report_cb)
{
    uc_wiota_gateway_user_recv = user_recv_cb;

    uc_wiota_gateway_exce_report = user_get_state_report_cb;

    return 1;
}

int uc_wiota_gateway_register_get_rtc_cb(unsigned int fmt, uc_wiota_gateway_get_rtc_cb get_rtc_cb)
{
    uc_wiota_gateway_rtc = get_rtc_cb;
    rtc_fmt = fmt;

    return 1;
}

int uc_wiota_gateway_register_ex_mcu_cb(uc_wiota_gateway_ex_mcu_cb ex_mcu_cb)
{
    uc_wiota_gateway_ex_mcu = ex_mcu_cb;

    return 1;
}

int uc_wiota_gateway_ota_req(void)
{
    if (gateway_mode.gateway_mode)
    {
        return uc_wiota_gateway_send_ota_req_msg();
    }

    return 0;
}

int uc_wiota_gateway_get_rtc(unsigned int fmt, uc_wiota_gateway_get_rtc_cb get_rtc)
{
    app_ps_header_t ps_header = {0};
    unsigned char reserved = 0;
    unsigned char gw_verity = uc_wiota_gateway_get_verity();
    int ret = 0;

    if (UC_GW_CONNECT_SUCESS != wiota_gw_connect_flag)
    {
        return 1;
    }
    ps_header.cmd_type = IOTE_GET_RTC;
    if (gw_verity)
    {
        app_set_header_property(PRO_DEST_ADDR, 1, &ps_header.property);
        ps_header.addr.dest_addr = uc_wiota_get_gateway_id();
    }
    if (uc_wiota_gateway_send_ps_cmd_data(&reserved, 1, &ps_header))
    {
        uc_wiota_gateway_register_get_rtc_cb(fmt, get_rtc);
        rtc_sem = rt_sem_create("rtc_sem", 0, RT_IPC_FLAG_PRIO);
        if (RT_EOK != rt_sem_take(rtc_sem, 3000))
        {
            ret = 2;
        }
    }

    uc_wiota_gateway_register_get_rtc_cb(0xff, RT_NULL);

    if (rtc_sem)
    {
        rt_sem_delete(rtc_sem);
        rtc_sem = RT_NULL;
    }

    return ret;
}

void uc_wiota_gateway_set_verity(unsigned char is_open)
{
    gateway_mode.gw_verity = is_open;
}

unsigned char uc_wiota_gateway_get_verity(void)
{
    return gateway_mode.gw_verity;
}

static void uc_gateway_clean_data(void)
{
    uc_wiota_register_recv_data_callback(RT_NULL, UC_CALLBACK_NORAMAL_MSG);
    uc_wiota_gateway_set_allow_run_flag(RT_FALSE);

    wiota_gw_connect_flag = UC_GW_NO_CONNECT;

    uc_wiota_gateway_send_exit();
    uc_wiota_gateway_wait_exit();
#ifdef RT_USING_AT
    uc_wiota_register_recv_data_callback(wiota_recv_callback, UC_CALLBACK_NORAMAL_MSG);
    // uc_wiota_register_recv_data_callback(wiota_recv_callback, UC_CALLBACK_STATE_INFO);
#endif

    if (gateway_mode.ota_timer != RT_NULL)
    {
        // rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
        rt_timer_delete(gateway_mode.ota_timer);
        gateway_mode.ota_timer = RT_NULL;
    }

    if (gateway_mode.gateway_handler != RT_NULL)
    {
        // rt_thread_delete(gateway_mode.gateway_handler);
        gateway_mode.gateway_handler = RT_NULL;
    }

    // wiota_gateway_mode_flag = RT_FALSE;
    gateway_mode.gateway_mode = RT_FALSE;

    if (gateway_mode.gateway_mq != RT_NULL)
    {
        while (1)
        {
            uc_wiota_gateway_msg_t *recv_data;
            int ret = uc_wiota_gateway_recv_queue(gateway_mode.gateway_mq, (void **)&recv_data, 0);
            if (RT_EOK != ret)
                break;

            if (recv_data->msg_type == GATEWAY_MAG_CODE_DL_RECV)
            {
                rt_free(recv_data->data);
                rt_free(recv_data);
            }
        }

        uc_wiota_gateway_dele_queue(gateway_mode.gateway_mq);
        gateway_mode.gateway_mq = RT_NULL;
    }

    uc_gateway_state = GATEWAY_END;
}

int uc_wiota_gateway_end(void)
{

    if (UC_GW_NO_CONNECT != wiota_gw_connect_flag)
    {
        uc_wiota_gateway_user_recv = RT_NULL;

        uc_wiota_gateway_exce_report = RT_NULL;

        uc_gateway_clean_data();
    }
    else
        return 1;

    return 0;
}

uc_gateway_state_t uc_wiota_gateway_get_state(void)
{
    return uc_gateway_state;
}

void uc_wiota_gateway_api_handle(void *para)
{
    uc_wiota_gateway_msg_t *recv_data = RT_NULL;
    gateway_mode.gateway_task_run = 1;

    while (1)
    {
        if (gateway_mode.gateway_mode) // if not set 1, task out
        {
            if (gateway_mode.gateway_mq == RT_NULL)
            {
                rt_kprintf("gw mq null\n");
                break;
            }

            if (RT_EOK != uc_wiota_gateway_recv_queue(gateway_mode.gateway_mq,
                                                      (void **)&recv_data,
                                                      GATEWAY_WAIT_QUEUE_TIMEOUT))
            {
                uc_wiota_status_e connect_state = uc_wiota_get_state();

                if (UC_STATUS_SYNC_LOST == connect_state ||
                    UC_STATUS_ERROR == connect_state)
                {
                    uc_gateway_state = GATEWAY_FAILED;
                    if (RT_NULL != uc_wiota_gateway_exce_report)
                        uc_wiota_gateway_exce_report(uc_gateway_state);
                    break;
                }
                continue;
            }
            rt_kprintf("gw msgType %d\n", recv_data->msg_type);
            switch (recv_data->msg_type)
            {
            case GATEWAY_MAG_CODE_DL_RECV:
                uc_wiota_gateway_analisys_dl_msg(recv_data->data, recv_data->data_len, recv_data->recv_msg_type);
                rt_free(recv_data->data);
                rt_free(recv_data);
                break;
            case GATEWAY_MAG_CODE_UL_HEART:
                uc_wiota_gateway_state_update_info_msg();
                break;
            case GATEWAY_MAG_CODE_OTA_REQ:
                uc_wiota_gateway_send_ota_req_msg();
                break;
            case GATEWAY_MAG_CODE_UL_MISS_DATA_REQ:
                uc_wiota_gateway_ota_miss_data_req_msg();
                break;
            case GATEWAY_MAG_CODE_EXIT:
                gateway_mode.exit_flag = 1;
                return;
            }
        }
        else
        {
            rt_kprintf("gw mode out\n");
            break;
        }
    }
    gateway_mode.exit_flag = 1;
}

#endif // AT_WIOTA_GATEWAY_API
// #endif  // RT_USING_AT
// #endif  // UC8288_MODULE
