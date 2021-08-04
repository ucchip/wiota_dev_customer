#ifndef _PLC_MASTER_WG_PHASE_H
#define _PLC_MASTER_WG_PHASE_H

#include <rtthread.h>


void wg_plc_read_and_load_node_info(void);

void wg_plc_recv_node_info(rt_uint16_t fn, rt_uint8_t* data, rt_uint16_t datalen);

void recv_node_belong_info_ack(rt_uint8_t* nodeinfo, rt_uint16_t infolen);

void plc_detect_flag_set(void);

void wg_plc_mudule_detect(void);

void wg_plc_mudule_init(void);

void wg_node_init(void);

#endif

