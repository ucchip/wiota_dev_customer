/**
 * @file        plc_wg_phase.c
 * @brief       wg module phase handle
 */

#include <string.h>
#include "wg_phase.h"
#include "elec_cco_api.h"
#include "elec_1376_2.h"

static rt_uint8_t wg_plc_detect_cnt;
static rt_uint8_t plc_detect_flag;//0:plc ok; 1:plc fail
static rt_uint8_t sys_addr[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};



static void U16ToBuf_little_endian(rt_uint16_t   in, rt_uint8_t* out)
{
    rt_uint8_t i = 0;

    for (i = 0; i < 2; i++)
    {
        out[i] = (in >> (8 * i)) & 0xff;

    }
}


/*************************************************************链表**************************************/


typedef struct wg_phase
{

    rt_uint8_t    node_id[6];

    //0:unknow 1:a phase 2:b phase 4: c phase
    rt_uint8_t    phase;

    rt_uint8_t    loop;

    rt_uint8_t    refresh;

    struct wg_phase*  next;

} s_wg_phase;

static s_wg_phase* node_header = NULL;



static void wg_del_node_id(s_wg_phase* p)
{
    rt_memset(p->node_id, 0, sizeof(p->node_id));
}


static s_wg_phase* wg_node_create(rt_uint8_t* id, rt_uint8_t ph)
{
    s_wg_phase* p = NULL;
    const rt_uint8_t compare[6] = {0x00};

    p = rt_malloc(sizeof(s_wg_phase));

    if (p == NULL)
    {
        return NULL;
    }

    rt_memset(p, 0, sizeof(s_wg_phase));

    rt_memcpy(p->node_id, id, sizeof(p->node_id));

    if (rt_memcmp(p->node_id, compare, sizeof(p->node_id)) == 0)
    {
        return NULL;
    }

    if (p->phase != ph)
    {
        p->phase = ph;
        p->refresh = 1;
    }
    else
    {
        p->refresh = 0;
    }

    p->next = NULL;

    return p;
}

static void wg_node_insert(s_wg_phase* new_p)
{
    s_wg_phase* p = node_header;

    while (p->next != NULL)
    {
        p = p->next;
    }

    p->next = new_p;
}

static s_wg_phase* wg_node_find_base_id(rt_uint8_t* id)
{
    s_wg_phase* p = node_header;

    while (p->next != NULL)
    {
        p = p->next;

        if (rt_memcmp(p->node_id, id, sizeof(p->node_id)) == 0)
        {
            return p;
        }

    }

    return NULL;

}


static void wg_node_del(s_wg_phase* del_p)
{
    s_wg_phase* p = node_header;
    s_wg_phase* p_temp = NULL;

    while (p->next != NULL)
    {
        p_temp = p;

        p = p->next;

        if (p == del_p)
        {
            p_temp->next = del_p->next;

            wg_del_node_id(del_p);

            rt_free(del_p);
        }

    }

}

void wg_node_init(void)
{

    node_header = rt_malloc(sizeof(s_wg_phase));

    if (node_header != NULL)
    {
        rt_memset(node_header, 0, sizeof(s_wg_phase));
    }

}



/********************************************************************功能********************************/

//1:plc ok; 0:plc fail
static rt_uint8_t plc_detect_flag_query(void)
{
    return plc_detect_flag;
}

void plc_detect_flag_set(void)
{
    plc_detect_flag = 1;

    wg_plc_detect_cnt = 0;
}

static void plc_detect_flag_clr(void)
{
    plc_detect_flag = 0;
}




typedef struct
{
    rt_uint16_t total;
    rt_uint16_t index;
    rt_uint16_t ack_num;
    rt_uint8_t  module_test_flag;
} s_wg_node_info;
static s_wg_node_info wg_node_info;


void recv_node_belong_info_ack(rt_uint8_t* nodeinfo, rt_uint16_t infolen)
{
    s_wg_phase* p = NULL;
    rt_uint16_t pos = 0;
    rt_uint16_t ack_num = 0, num_cnt = 0;

    pos = 0;

    ack_num = nodeinfo[pos];
    pos += 1;

    rt_kprintf("belong info platform ack num=%d\r\n", ack_num);

    if (ack_num > 0)
    {
        while (num_cnt < ack_num)
        {
            p = wg_node_find_base_id(&nodeinfo[pos]);
            pos += 6;

            if (p != NULL)
            {
                p->refresh = 0;
            }

            num_cnt++;
        }
    }
}

static rt_int32_t report_node_belong_info(rt_uint8_t* nodeinfo, rt_uint16_t infolen)
{

    return 0;
}


static void wg_plc_node_info_upload(void)
{
    s_wg_phase* p = node_header;
    s_wg_phase* p_temp = NULL;
    rt_uint8_t* nodeinfo = NULL;
    rt_uint8_t pos = 0, node_cnt = 0;;

    nodeinfo = rt_malloc(200);

    if (nodeinfo != NULL)
    {
        rt_memset(nodeinfo, 0, 200);

        node_cnt = 0;

        //node num postion
        pos = 1;

        while (p->next != NULL)
        {
            p_temp = p;

            p = p->next;

            if (p->refresh == 1)
            {
                rt_memcpy(&nodeinfo[pos], p->node_id, sizeof(p->node_id));
                pos += 6;

                nodeinfo[pos] = p->phase;
                pos += 1;

                nodeinfo[pos] = p->loop;
                pos += 1;

                pos += 8;

                node_cnt++;

                if (node_cnt >= 20)
                {
                    break;
                }

            }

            rt_thread_mdelay(1);

        }

        if (node_cnt > 0)
        {
            nodeinfo[0] = node_cnt;

            report_node_belong_info(nodeinfo, pos);


            rt_kprintf("report node belong info,node_cnt=%d\r\n", node_cnt);
        }
    }

}


void wg_plc_recv_node_info(rt_uint16_t fn, rt_uint8_t* data, rt_uint16_t datalen)
{
    rt_uint8_t pos = 0;
    rt_uint16_t node_total = 0;
    rt_uint8_t  ack_node_num = 0, i = 0;
    rt_uint8_t nodeid[6] = {0};
    rt_uint8_t phase = 0;

    s_wg_phase* p1 = NULL;
    s_wg_phase* p2 = NULL;


    if (fn == FN(9))
    {
        pos = 0;

        wg_node_info.total = data[pos];
        pos += 1;

        wg_node_info.total = data[pos] << 8 | wg_node_info.total;
        pos += 1;

        if (wg_node_info.total > 0)
        {
            wg_node_info.module_test_flag = 1;

        }

        rt_kprintf("wg_plc_recv_master_afn=0xf0 fn=9,node_total=%d \r\n", wg_node_info.total);

    }
    else
    {
        pos = 0;

        node_total = data[pos];
        pos += 1;

        node_total = data[pos] << 8 | node_total;
        pos += 1;

        wg_node_info.ack_num = data[pos];
        pos += 1;

        if (wg_node_info.ack_num > 0)
        {
            for (i = 0; i < wg_node_info.ack_num; i++)
            {

                rt_memcpy(nodeid, &data[pos], sizeof(nodeid));
                pos += 6;

                pos += 1;

                phase = data[pos] & 0x07;
                pos += 1;

                //if id is in list,and phase is changed,refresh it's info,else create new id in list
                p2 = wg_node_find_base_id(nodeid);

                if (p2 != NULL)
                {
                    if (p2->phase != phase)
                    {
                        p2->phase = phase;

                        p2->refresh = 1;
                    }
                }
                else
                {
                    p1 = wg_node_create(nodeid, phase);

                    if (p1 != NULL)
                    {
                        wg_node_insert(p1);
                    }
                }
            }

            wg_node_info.index += wg_node_info.ack_num;

        }


        rt_kprintf("wg_plc_recv_master_afn=0xf0 fn=23,node_ack_num=%d \r\n", wg_node_info.ack_num);

    }

}



void wg_plc_read_and_load_node_info(void)
{
    rt_uint8_t temp[3] = {0};
    rt_uint8_t pos = 0;

    static rt_uint32_t read_cycle;


    if (plc_detect_flag_query() != 1)
    {
        return;
    }

    read_cycle++;

    //task delay is 20ms there is about 10sec
    if (read_cycle < 20000)
    {
        return;
    }

    read_cycle = 0;

    //use AFN=F0,F9,query total_num,
    if (wg_node_info.total == 0)
    {
        pos = 0;

        //start index must from 1
        U16ToBuf_little_endian(1, temp);

        pos += 2;
        temp[pos] = 15; //max query num is 15
        pos += 1;


        elec_1376_2_send(sys_addr, RT_NULL, GDW1376_2_AFN_TEST_0xF0, FN(9), temp, sizeof(temp));
        //        elec_1376_2_send(sys_addr,RT_NULL,GDW1376_2_AFN_ROUTER_QUERY_0x10,FN(1),temp,sizeof(temp));

        rt_kprintf("wg_plc_send_master_afn=0xf0 fn=9\r\n");


    }
    else
    {
        if (wg_node_info.index < wg_node_info.total)
        {
            pos = 0;

            U16ToBuf_little_endian(wg_node_info.index, temp);
            pos += 2;

            temp[pos] = 20;
            pos += 1;

            elec_1376_2_send(sys_addr, RT_NULL, GDW1376_2_AFN_TEST_0xF0, FN(23), temp, sizeof(temp));
            //            elec_1376_2_send(sys_addr,RT_NULL,GDW1376_2_AFN_ROUTER_QUERY_0x10,FN(2),temp,sizeof(temp));


            rt_kprintf("query node belong info,startindex=%d,total=%d\r\n", wg_node_info.index, wg_node_info.total);

        }
        else
        {
            wg_node_info.total = 0;
            wg_node_info.index = 0;
            wg_node_info.ack_num = 0;

            wg_plc_node_info_upload();

        }
    }
}



static void wg_plc_whitelist_set(rt_uint8_t onoff)
{
    //0x10 black list off
    //0x11 black list on
    //0x20 white list off
    //0x21 white list on

    rt_uint8_t temp = 0;

    if (onoff == 1)
    {
        temp = 0x21;
    }
    else
    {
        temp = 0x20;
    }

    elec_1376_2_send(sys_addr, RT_NULL, GDW1376_2_AFN_TEST_0xF0, FN(4), &temp, 1);


    rt_kprintf("plc_module set_whitelist\r\n");
}



//set hub addr to plc_master module ,lbdjoin function auto open
static void wg_plc_set_master_addr(void)
{
    rt_uint8_t temp[6] = {0};
    rt_uint8_t i = 0;

    for (i = 0; i < 6; i++)
    {
        temp[i] = sys_addr[5 - i];
        //        temp[i]=0x55;

    }

    elec_1376_2_send(sys_addr, RT_NULL, GDW13766_2_AFN_CTRL_CMD_0x05, FN(1), temp, 6);

    rt_kprintf("plc_module set_master_addr\r\n");
}

//wg module detect in cycle
void wg_plc_mudule_detect(void)
{
    static rt_uint32_t detect_cycle;

    detect_cycle++;

    //task delay is 20ms there is about 1min
    if (detect_cycle > 30000)
    {

        elec_1376_2_send(sys_addr, RT_NULL, GDW1376_2_AFN_QUERY_0x03, FN(10), RT_NULL, 0);

        rt_kprintf("plc_module detect\r\n");

        wg_plc_detect_cnt++;

        if (wg_plc_detect_cnt >= 3)
        {
            plc_detect_flag_clr();

            rt_kprintf("plc_module detect fail\r\n");

        }

        detect_cycle = 0;

    }

}


void wg_plc_mudule_init(void)
{
    static rt_uint8_t last_plc_state;
    static rt_uint8_t init_state;
    static rt_uint32_t over_timer;

    //plc module is ok,start init schedule
    if (last_plc_state != plc_detect_flag_query())
    {
        last_plc_state = plc_detect_flag_query();

        wg_plc_set_master_addr();

        rt_thread_mdelay(100);

        wg_plc_whitelist_set(0);

    }
}

/*@return:
    0:hub and node joined sucess
    2:hub_plc_mudule is ok,but have no node joined
    -1:hub_plc_module detect fail
*/

int8_t wg_plc_test_state(void)
{
    if (plc_detect_flag_query() != 1)
    {
        rt_kprintf("PLC module detect fail\r\n");

        return -1;//master init fail
    }

    if (wg_node_info.module_test_flag == 0)
    {
        rt_kprintf("PLC module ok,but no node joined\r\n");

        return 2;
    }

    rt_kprintf("PLC module ok and have node joined\r\n");

    return 0;
}
