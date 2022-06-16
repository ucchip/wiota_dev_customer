#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <uc_coding.h>

typedef struct
{
    unsigned char cmd;
    unsigned char type[10];
    unsigned char state[10];
    unsigned int id;
} test_data_t;

void app_coding_test_demo(void)
{
    app_ps_header_t ps_header = {0};
    unsigned char *output_data = RT_NULL;
    unsigned int output_data_len = 0;
    test_data_t test_data[5] = {0};
    unsigned int test_data_len = sizeof(test_data_t) * 5;

    for (int i = 0; i < 5; i++)
    {
        test_data[i].cmd = i;
        test_data[i].id = 0x100 + i;
        if (i % 2 == 0)
        {
            rt_memcpy(&test_data[i].type, "light", 5);
            rt_memcpy(&test_data[i].state, "on", 2);
        }
        else
        {
            rt_memcpy(&test_data[i].type, "switch", 6);
            rt_memcpy(&test_data[i].state, "off", 3);
        }
    }

    app_set_header_property(PRO_SRC_ADDR, 1, &ps_header.property);
    app_set_header_property(PRO_DEST_ADDR, 1, &ps_header.property);
    app_set_header_property(PRO_PACKET_NUM, 1, &ps_header.property);
    app_set_header_property(PRO_NEED_RES, 1, &ps_header.property);
    app_set_header_property(PRO_RESPONSE_FLAG, 1, &ps_header.property);
    app_set_header_property(PRO_SEGMENT_FLAG, 1, &ps_header.property);
    app_set_header_property(PRO_COMPRESS_FLAG, 1, &ps_header.property);

    ps_header.addr.src_addr = 0x11223344;
    ps_header.addr.dest_addr = 0x44332211;
    ps_header.packet_num = 1;
    ps_header.segment_info.total_num = 100;
    ps_header.segment_info.current_num = 10;
    ps_header.cmd_type = 1;

    if (0 != app_data_coding(&ps_header, (unsigned char *)test_data, test_data_len, &output_data, &output_data_len))
    {
        rt_kprintf("coding failed\n");
        return;
    }

    for (int i = 0; i < output_data_len; i++)
    {
        rt_kprintf("%x ", output_data[i]);
    }
    rt_kprintf("\n");

    unsigned char *temp_data = RT_NULL;
    unsigned int temp_data_len = 0;
    app_ps_header_t temp_ps_header = {0};
    if (0 != app_data_decoding(output_data, output_data_len, &temp_data, &temp_data_len, &temp_ps_header))
    {
        rt_kprintf("decoding failed\n");
        return;
    }
    rt_kprintf("is_src_addr %d, is_dest_addr %d, is_packet_num %d, is_need_res %d, response_flag %d, \nsegment_flag %d, compress_flag %d, src_addr 0x%x, dest_addr 0x%x, packet_num %d, total_num %d, current_num %d, cmd_type %d,\n",
               temp_ps_header.property.is_src_addr,
               temp_ps_header.property.is_dest_addr,
               temp_ps_header.property.is_packet_num,
               temp_ps_header.property.is_need_res,
               temp_ps_header.property.response_flag,
               temp_ps_header.property.segment_flag,
               temp_ps_header.property.compress_flag,
               temp_ps_header.addr.src_addr,
               temp_ps_header.addr.dest_addr,
               temp_ps_header.packet_num,
               temp_ps_header.segment_info.total_num,
               temp_ps_header.segment_info.current_num,
               temp_ps_header.cmd_type);

    test_data_t *recv_data = (test_data_t *)temp_data;
    for (int i = 0; i < 5; i++)
    {
        rt_kprintf("cmd %d, id 0x%x, type %s, state %s\n", recv_data[i].cmd, recv_data[i].id, recv_data[i].type, recv_data[i].state);
    }

    rt_free(output_data); // manually release after use
    output_data = RT_NULL;
    rt_free(temp_data); // manually release after use
    temp_data = RT_NULL;
}

#endif