/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-03-30     chenyong     first version
 * 2018-04-14     chenyong     modify parse arguments
 */

#include <at.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <rthw.h>
#include <rtdevice.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"

#define LOG_TAG "at.svr"
#include <at_log.h>

#ifdef AT_USING_SERVER

#define AT_CMD_CHAR_0 '0'
#define AT_CMD_CHAR_9 '9'
#define AT_CMD_QUESTION_MARK '?'
#define AT_CMD_EQUAL_MARK '='
#define AT_CMD_L_SQ_BRACKET '['
#define AT_CMD_R_SQ_BRACKET ']'
#define AT_CMD_L_ANGLE_BRACKET '<'
#define AT_CMD_R_ANGLE_BRACKET '>'
#define AT_CMD_COMMA_MARK ','
#define AT_CMD_SEMICOLON ';'
#define AT_CMD_CR '\r'
#define AT_CMD_LF '\n'

static at_server_t at_server_local = RT_NULL;
static at_cmd_t cmd_table = RT_NULL;
static rt_size_t cmd_num;
// dtu_send_t g_dtu_send = RT_NULL;

#ifdef SUPPORT_SPI_AT
__attribute__((section(".spicmd"))) volatile spi_at_buf spi_at_cmd;
__attribute__((section(".spiresult"))) volatile spi_at_buf spi_at_result;
extern void at_vprintf(spi_at_buf *buf, const char *format, va_list args);
extern void at_vprintfln(spi_at_buf *buf, const char *format, va_list args);
#else
extern void at_vprintf(rt_device_t device, const char *format, va_list args);
extern void at_vprintfln(rt_device_t device, const char *format, va_list args);
#endif
// extern void dtu_send_process(void);
/**
 * AT server send data to AT device
 *
 * @param format the input format
 */
void at_server_printf(const char *format, ...)
{
    va_list args;

    va_start(args, format);
#ifdef SUPPORT_SPI_AT
    at_vprintf(at_server_local->spi_result, format, args);
#else
    at_vprintf(at_server_local->device, format, args);
#endif
    va_end(args);
}

void at_send_data(const void *buffer, unsigned int len)
{
#ifdef SUPPORT_SPI_AT
    const void *data = buffer;
    int timeout = 5000; /* 5s */

    while (len > 0 && timeout > 0)
    {
        if (SPI_FLAG_IDLE == at_server_local->spi_result->flag)
        {
            if (len > SPI_AT_DATA_MAX)
            {
                memcpy(at_server_local->spi_result->data, data, SPI_AT_DATA_MAX);
                len -= SPI_AT_DATA_MAX;
                at_server_local->spi_result->len = SPI_AT_DATA_MAX;
            }
            else
            {
                memcpy(at_server_local->spi_result->data, data, len);
                at_server_local->spi_result->len = len;
                len = 0;
            }
            at_server_local->spi_result->flag = SPI_FLAG_READY;
            continue;
        }
        rt_thread_mdelay(4);
        timeout -= 4;
    }
#else
    rt_device_write(at_server_local->device, 0, buffer, len);
#endif
}

/**
 * AT server send data and newline to AT device
 *
 * @param format the input format
 */
void at_server_printfln(const char *format, ...)
{
    va_list args;

    va_start(args, format);
#ifdef SUPPORT_SPI_AT
    at_vprintfln(at_server_local->spi_result, format, args);
#else
    at_vprintfln(at_server_local->device, format, args);
#endif
    va_end(args);
}

/**
 * AT server request arguments parse arguments
 *
 * @param req_args request arguments
 * @param req_expr request expression
 *
 * @return  -1 : parse arguments failed
 *           0 : parse without match
 *          >0 : The number of arguments successfully parsed
 */
int at_req_parse_args(const char *req_args, const char *req_expr, ...)
{
    va_list args;
    int req_args_num = 0;

    RT_ASSERT(req_args);
    RT_ASSERT(req_expr);

    va_start(args, req_expr);

    req_args_num = vsscanf(req_args, req_expr, args);

    va_end(args);

    return req_args_num;
}

/**
 * AT server send command execute result to AT device
 *
 * @param result AT command execute result
 */
void at_server_print_result(at_result_t result)
{
    switch (result)
    {
    case AT_RESULT_OK:
        //            at_server_printfln("");
        at_server_printfln("OK");
        break;

    case AT_RESULT_FAILE:
    case AT_RESULT_NULL:
        //            at_server_printfln("");
        at_server_printfln("ERROR");
        break;

        // case AT_RESULT_NULL:
        //     break;

    case AT_RESULT_CMD_ERR:
        at_server_printfln("ERR CMD MATCH FAILED!");
        at_server_print_result(AT_RESULT_FAILE);
        break;

    case AT_RESULT_CHECK_FAILE:
        at_server_printfln("ERR CHECK ARGS FORMAT FAILED!");
        at_server_print_result(AT_RESULT_FAILE);
        break;

    case AT_RESULT_PARSE_FAILE:
        at_server_printfln("ERR PARSE ARGS FAILED!");
        at_server_print_result(AT_RESULT_FAILE);
        break;
    case AT_RESULT_REPETITIVE_FAILE:
        at_server_printfln("ERR REPETITIVE OPERATION FAILED!");
        at_server_print_result(AT_RESULT_FAILE);
        break;
    case AT_RESULT_REFUSED:
        at_server_printfln("ERR REFUSED OPERATION!");
        at_server_print_result(AT_RESULT_FAILE);
        break;
    default:
        // at_server_printfln("");
        // at_server_printfln("ERROR");
        // break;
        break;
    }
}

/**
 *  AT server print all commands to AT device
 */
void rt_at_server_print_all_cmd(void)
{
    rt_size_t i = 0;

    at_server_printfln("Commands list : ");

    for (i = 0; i < cmd_num; i++)
    {
        at_server_printf("%s", cmd_table[i].name);

        if (cmd_table[i].args_expr)
        {
            at_server_printfln("%s", cmd_table[i].args_expr);
        }
        else
        {
            at_server_printf("%c%c", AT_CMD_CR, AT_CMD_LF);
        }
    }
}
#ifndef SUPPORT_SPI_AT
/**
 * Send data to AT Client by uart device.
 *
 * @param server current AT server object
 * @param buf   send data buffer
 * @param size  send fixed data size
 *
 * @return >0: send data size
 *         =0: send failed
 */
rt_size_t at_server_send(at_server_t server, const char *buf, rt_size_t size)
{
    RT_ASSERT(buf);

    if (server == RT_NULL)
    {
        LOG_E("input AT Server object is NULL, please create or get AT Server object!");
        return 0;
    }

    return rt_device_write(server->device, 0, buf, size);
}
#endif
/**
 * AT Server receive fixed-length data.
 *
 * @param client current AT Server object
 * @param buf   receive data buffer
 * @param size  receive fixed data size
 * @param timeout  receive data timeout (ms)
 *
 * @note this function can only be used in execution function of AT commands
 *
 * @return >0: receive data size
 *         =0: receive failed
 */
rt_size_t at_server_recv(at_server_t server, char *buf, rt_size_t size, rt_int32_t timeout)
{
    rt_size_t read_idx = 0;
    rt_err_t result = RT_EOK;
    char ch = 0;

    RT_ASSERT(buf);

    if (server == RT_NULL)
    {
        LOG_E("input AT Server object is NULL, please create or get AT Server object!");
        return 0;
    }

    while (1)
    {
        if (read_idx < size)
        {
            /* check get data value */
            result = server->get_char(server, &ch, timeout);
            if (result != RT_EOK)
            {
                LOG_E("AT Server receive failed, uart device get data error.");
                return 0;
            }

            buf[read_idx++] = ch;
        }
        else
        {
            break;
        }
    }

    return read_idx;
}

at_server_t at_get_server(void)
{
    RT_ASSERT(at_server_local);
    RT_ASSERT(at_server_local->status != AT_STATUS_UNINITIALIZED);

    return at_server_local;
}

static rt_err_t at_check_args(const char *args, const char *args_format)
{
    rt_size_t left_sq_bracket_num = 0, right_sq_bracket_num = 0;
    rt_size_t left_angle_bracket_num = 0, right_angle_bracket_num = 0;
    rt_size_t comma_mark_num = 0;
    rt_size_t i = 0;

    RT_ASSERT(args);
    RT_ASSERT(args_format);

    for (i = 0; i < strlen(args_format); i++)
    {
        switch (args_format[i])
        {
        case AT_CMD_L_SQ_BRACKET:
            left_sq_bracket_num++;
            break;

        case AT_CMD_R_SQ_BRACKET:
            right_sq_bracket_num++;
            break;

        case AT_CMD_L_ANGLE_BRACKET:
            left_angle_bracket_num++;
            break;

        case AT_CMD_R_ANGLE_BRACKET:
            right_angle_bracket_num++;
            break;

        default:
            break;
        }
    }

    if (left_sq_bracket_num != right_sq_bracket_num || left_angle_bracket_num != right_angle_bracket_num || left_sq_bracket_num > left_angle_bracket_num)
    {
        return -RT_ERROR;
    }

    for (i = 0; i < strlen(args); i++)
    {
        if (args[i] == AT_CMD_COMMA_MARK)
        {
            comma_mark_num++;
        }
    }

    if ((comma_mark_num + 1 < left_angle_bracket_num - left_sq_bracket_num) || comma_mark_num + 1 > left_angle_bracket_num)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t at_cmd_process(at_cmd_t cmd, const char *cmd_args)
{
    at_result_t result = AT_RESULT_OK;

    RT_ASSERT(cmd);
    RT_ASSERT(cmd_args);

    if (cmd_args[0] == AT_CMD_EQUAL_MARK && cmd_args[1] == AT_CMD_QUESTION_MARK && cmd_args[2] == AT_CMD_CR)
    {
        if (cmd->test == RT_NULL)
        {
            at_server_print_result(AT_RESULT_CMD_ERR);
            return -RT_ERROR;
        }

        result = cmd->test();
        at_server_print_result(result);
    }
    else if (cmd_args[0] == AT_CMD_QUESTION_MARK && cmd_args[1] == AT_CMD_CR)
    {
        if (cmd->query == RT_NULL)
        {
            at_server_print_result(AT_RESULT_CMD_ERR);
            return -RT_ERROR;
        }

        result = cmd->query();
        at_server_print_result(result);
    }
    else if (cmd_args[0] == AT_CMD_EQUAL_MARK || (cmd_args[0] >= AT_CMD_CHAR_0 && cmd_args[0] <= AT_CMD_CHAR_9 && cmd_args[1] == AT_CMD_CR))
    {
        if (cmd->setup == RT_NULL)
        {
            at_server_print_result(AT_RESULT_CMD_ERR);
            return -RT_ERROR;
        }

        if (at_check_args(cmd_args, cmd->args_expr) < 0)
        {
            at_server_print_result(AT_RESULT_CHECK_FAILE);
            return -RT_ERROR;
        }

        result = cmd->setup(cmd_args);
        at_server_print_result(result);
    }
    else if (cmd_args[0] == AT_CMD_CR)
    {
        if (cmd->exec == RT_NULL)
        {
            at_server_print_result(AT_RESULT_CMD_ERR);
            return -RT_ERROR;
        }

        result = cmd->exec();
        at_server_print_result(result);
    }
    else
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static at_cmd_t at_find_cmd(const char *cmd)
{
    rt_size_t i = 0;

    RT_ASSERT(cmd_table);

    for (i = 0; i < cmd_num; i++)
    {
        if (!rt_strcasecmp(cmd, cmd_table[i].name))
        {
            return &cmd_table[i];
        }
    }
    return RT_NULL;
}

static rt_err_t at_cmd_get_name(const char *cmd_buffer, char *cmd_name)
{
    rt_size_t cmd_name_len = 0, i = 0;

    RT_ASSERT(cmd_name);
    RT_ASSERT(cmd_buffer);

    for (i = 0; i < strlen(cmd_buffer) + 1; i++)
    {
        if (*(cmd_buffer + i) == AT_CMD_QUESTION_MARK || *(cmd_buffer + i) == AT_CMD_EQUAL_MARK || *(cmd_buffer + i) == AT_CMD_CR || (*(cmd_buffer + i) >= AT_CMD_CHAR_0 && *(cmd_buffer + i) <= AT_CMD_CHAR_9))
        {
            cmd_name_len = i;
            if (cmd_name_len >= AT_CMD_NAME_LEN)
            {
                return -RT_ERROR;
            }
            memcpy(cmd_name, cmd_buffer, cmd_name_len);
            *(cmd_name + cmd_name_len) = '\0';

            return RT_EOK;
        }
    }

    return -RT_ERROR;
}
#ifdef SUPPORT_SPI_AT
static rt_err_t at_server_getchar(at_server_t server, char *ch, rt_int32_t timeout)
{
    static char *tmp = NULL;
    if (RT_NULL == tmp)
        tmp = server->spi_cmd->data;

    while (timeout < 0 ? 1 : timeout)
    {
        if (SPI_FLAG_READY == server->spi_cmd->flag /*&& ((server->spi_cmd->len -- ) > 0)*/)
        {
            *ch = *tmp;
            tmp++;
            return RT_EOK;
        }
        else
        {
            tmp = server->spi_cmd->data;
        }
        rt_thread_mdelay(4);
        timeout -= 4;
    }

    return RT_ETIMEOUT;
}

#else
static rt_err_t at_server_getchar(at_server_t server, char *ch, rt_int32_t timeout)
{
    rt_err_t result = RT_EOK;

    while (rt_device_read(at_server_local->device, 0, ch, 1) == 0)
    {
        //        rt_sem_control(at_server_local->rx_notice, RT_IPC_CMD_RESET, RT_NULL);
        result = rt_sem_take(at_server_local->rx_notice, rt_tick_from_millisecond(timeout));
        if (result != RT_EOK)
        {
            return result;
        }
    }

    return result;
}
#endif

static void server_parser(at_server_t server)
{
#define ESC_KEY 0x1B
#define BACKSPACE_KEY 0x08
#define DELECT_KEY 0x7F

    char cur_cmd_name[AT_CMD_NAME_LEN] = {0};
    at_cmd_t cur_cmd = RT_NULL;
    char *cur_cmd_args = RT_NULL, ch = 0, last_ch = 0;

    RT_ASSERT(server);
    RT_ASSERT(server->status != AT_STATUS_UNINITIALIZED);

    // if (g_dtu_send->flag)
    // {
    //     at_server_printfln("DTU SEND MODE");
    // }

    while (1)
    {
        // if (g_dtu_send->flag)
        // {
        //     dtu_send_process();
        // }
        if (RT_EOK != server->get_char(server, &ch, RT_WAITING_FOREVER))
            continue;

        /* fix bug241: AT dead after receiving wrong data */
        if (ch == '\0')
            continue;

        if (ESC_KEY == ch)
        {
            break;
        }

        if (server->echo_mode)
        {
            if (ch == AT_CMD_CR || (ch == AT_CMD_LF && last_ch != AT_CMD_CR))
            {
                at_server_printf("%c%c", AT_CMD_CR, AT_CMD_LF);
            }
            else if (ch == AT_CMD_LF)
            {
                // skip the end sign check
            }
            else if (ch == BACKSPACE_KEY || ch == DELECT_KEY)
            {
                if (server->cur_recv_len)
                {
                    server->recv_buffer[--server->cur_recv_len] = 0;
                    at_server_printf("\b \b");
                }

                continue;
            }
            else
            {
                at_server_printf("%c", ch);
            }
        }

        server->recv_buffer[server->cur_recv_len++] = ch;
        last_ch = ch;

        if (!rt_strstr(server->recv_buffer, server->end_mark))
        {
            if ((server->cur_recv_len + 1) == AT_SERVER_RECV_BUFF_LEN)
            {
                server->cur_recv_len = 0;
                memset(server->recv_buffer, 0x00, AT_SERVER_RECV_BUFF_LEN);
            }
            continue;
        }

        if (at_cmd_get_name(server->recv_buffer, cur_cmd_name) < 0)
        {
            at_server_print_result(AT_RESULT_CMD_ERR);
            goto __retry;
        }

        cur_cmd = at_find_cmd(cur_cmd_name);
        if (!cur_cmd)
        {
            at_server_print_result(AT_RESULT_CMD_ERR);
            goto __retry;
        }

        cur_cmd_args = server->recv_buffer + strlen(cur_cmd_name);
        if (at_cmd_process(cur_cmd, cur_cmd_args) < 0)
        {
            goto __retry;
        }

    __retry:
        memset(server->recv_buffer, 0x00, AT_SERVER_RECV_BUFF_LEN);
        server->cur_recv_len = 0;

#ifdef SUPPORT_SPI_AT
        memset(server->spi_cmd->data, 0x00, SPI_AT_DATA_MAX);
        server->spi_cmd->flag = SPI_FLAG_IDLE;
#endif
    }
}
#ifndef SUPPORT_SPI_AT
static rt_err_t at_rx_ind(rt_device_t dev, rt_size_t size)
{
    if (size > 0)
    {
        rt_sem_release(at_server_local->rx_notice);
    }

    return RT_EOK;
}
#endif
#if defined(__ICCARM__) || defined(__ICCRX__) /* for IAR compiler */
#pragma section = "RtAtCmdTab"
#endif

int at_server_init(void)
{
    rt_err_t result = RT_EOK;
#ifndef SUPPORT_SPI_AT
    rt_err_t open_result = RT_EOK;
#endif

    if (at_server_local)
    {
        return result;
    }

    /* initialize the AT commands table.*/
#if defined(__CC_ARM) /* ARM C Compiler */
    extern const int RtAtCmdTab$$Base;
    extern const int RtAtCmdTab$$Limit;
    cmd_table = (at_cmd_t)&RtAtCmdTab$$Base;
    cmd_num = (at_cmd_t)&RtAtCmdTab$$Limit - cmd_table;
#elif defined(__ICCARM__) || defined(__ICCRX__) /* for IAR Compiler */
    cmd_table = (at_cmd_t)__section_begin("RtAtCmdTab");
    cmd_num = (at_cmd_t)__section_end("RtAtCmdTab") - cmd_table;
#elif defined(__GNUC__)                         /* for GCC Compiler */
    extern const int __rtatcmdtab_start;
    extern const int __rtatcmdtab_end;
    cmd_table = (at_cmd_t)&__rtatcmdtab_start;
    cmd_num = (at_cmd_t)&__rtatcmdtab_end - cmd_table;
#endif                                          /* defined(__CC_ARM) */

    at_server_local = (at_server_t)rt_calloc(1, sizeof(struct at_server));
    // g_dtu_send = (dtu_send_t)rt_calloc(1, sizeof(struct dtu_send));
    if ((!at_server_local) /*|| (!g_dtu_send)*/)
    {
        result = -RT_ENOMEM;
        LOG_E("AT server session initialize failed! No memory for at_server structure !");
        goto __exit;
    }
    // uc_wiota_get_dtu_config((u8_t *)g_dtu_send);
    // uc_wiota_get_dtu_config((dtu_info_t *)g_dtu_send);
    // rt_kprintf("dtu flag %d %d %d %d", g_dtu_send->flag,
    //            g_dtu_send->at_show, g_dtu_send->timeout, g_dtu_send->wait);

    at_server_local->echo_mode = 0;
    at_server_local->status = AT_STATUS_UNINITIALIZED;

    memset(at_server_local->recv_buffer, 0x00, AT_SERVER_RECV_BUFF_LEN);
    at_server_local->cur_recv_len = 0;
#ifdef SUPPORT_SPI_AT

    at_server_local->spi_cmd = &spi_at_cmd;
    at_server_local->spi_cmd->flag = SPI_FLAG_IDLE;

    at_server_local->spi_result = &spi_at_result;
    at_server_local->spi_result->flag = SPI_FLAG_IDLE;

#else
    at_server_local->rx_notice = rt_sem_create("at_svr", 0, RT_IPC_FLAG_FIFO);
    if (!at_server_local->rx_notice)
    {
        LOG_E("AT server session initialize failed! at_rx_notice semaphore create failed!");
        result = -RT_ENOMEM;
        goto __exit;
    }

    /* Find and open command device */
    at_server_local->device = rt_device_find(AT_SERVER_DEVICE);

    if (at_server_local->device)
    {
        struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT; /*init default parment*/
        RT_ASSERT(at_server_local->device->type == RT_Device_Class_Char);
        // config baud rate 115200
        config.baud_rate = uc_wiota_get_at_baud_rate(); // BAUD_RATE_115200;
        rt_device_control(at_server_local->device, RT_DEVICE_CTRL_CONFIG, &config);
        /* using DMA mode first */
        // open_result = rt_device_open(at_server_local->device, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_DMA_RX);
        /* using interrupt mode when DMA mode not supported */
        // if (open_result == -RT_EIO)
        //{
        open_result = rt_device_open(at_server_local->device, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
        //}
        RT_ASSERT(open_result == RT_EOK);

        rt_device_set_rx_indicate(at_server_local->device, at_rx_ind);
    }
    else
    {
        LOG_E("AT device initialize failed! Not find the device : %s.", AT_SERVER_DEVICE);
        result = -RT_ERROR;
        goto __exit;
    }
#endif

    at_server_local->get_char = at_server_getchar;
    memcpy(at_server_local->end_mark, AT_CMD_END_MARK, sizeof(AT_CMD_END_MARK));

    at_server_local->parser_entry = server_parser;
    at_server_local->parser = rt_thread_create("at_svr",
                                               (void (*)(void *parameter))server_parser,
                                               at_server_local,
                                               2 * 1524,
                                               /*RT_THREAD_PRIORITY_MAX / 3 - 1*/ 3,
                                               5);
    if (at_server_local->parser == RT_NULL)
    {
        result = -RT_ENOMEM;
        goto __exit;
    }

__exit:
    if (!result)
    {
        at_server_local->status = AT_STATUS_INITIALIZED;

        rt_thread_startup(at_server_local->parser);

        // LOG_I("RT-Thread AT server (V%s) initialize success.", AT_SW_VERSION);
    }
    else
    {
        if (at_server_local)
        {
            rt_free(at_server_local);
        }

        LOG_E("AT (V%s) init fail(%d).", AT_SW_VERSION, result);
    }

    return result;
}
// INIT_COMPONENT_EXPORT(at_server_init); // will before main() exec 22.5.12 -- jrhe

RT_WEAK void at_port_reset(void)
{
    LOG_E("The reset for AT server is not implement.");
}

RT_WEAK void at_port_factory_reset(void)
{
    LOG_E("The factory reset for AT server is not implement.");
}

#endif /* AT_USING_SERVER */
