#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <uc_coding.h>
#include "fastlz.h"

unsigned char app_packet_num(void)
{
    static unsigned char packet_num = 0;
    return packet_num ++;
}

void app_set_header_property(header_property_e bit, unsigned char value, app_ps_property_t *ps_property)
{
    unsigned char *property = (unsigned char *)ps_property;

    if (bit > PRO_SRC_ADDR)
    {
        rt_kprintf("%s, %d bit error\n", __FUNCTION__, __LINE__);
        return;
    }

    if (value > 1)
    {
        value = 1;
    }
    value = value << bit;
    *property |= value;

    return;
}

int app_data_coding(app_ps_header_t *ps_header,
                    unsigned char *input_data,
                    unsigned int input_data_len,
                    unsigned char **output_data,
                    unsigned int *output_data_len)
{
    unsigned char is_src_addr = ps_header->property.is_src_addr;
    unsigned char is_dest_addr = ps_header->property.is_dest_addr;
    unsigned char is_packet_num = ps_header->property.is_packet_num;
    unsigned char segment_flag = ps_header->property.segment_flag;
    unsigned char compress_flag = ps_header->property.compress_flag;
    unsigned int buf_len = sizeof(app_ps_header_t) + input_data_len;
    unsigned char *buf = RT_NULL;
    unsigned int offset = 0;
    unsigned int int_len = sizeof(unsigned int);
    unsigned char char_len = sizeof(unsigned char);
    unsigned int comp_data_len = 0;
    unsigned char *comp_data = RT_NULL;

    if (ps_header == RT_NULL || input_data_len > APP_MAX_CODING_DATA_LEN)
    {
        rt_kprintf("%s, %d input_data error\n", __FUNCTION__, __LINE__);
        return -1;
    }

    // malloc output_data space, maybe > output_data_len
    buf = (unsigned char *)rt_malloc(buf_len);
    if (RT_NULL == buf)
    {
        rt_kprintf("%s, %d malloc fail\n", __FUNCTION__, __LINE__);
        return -2;
    }
    rt_memset(buf, 0, buf_len);

    // copy property
    rt_memcpy(buf + offset, &ps_header->property, sizeof(app_ps_property_t));
    offset += sizeof(app_ps_property_t);

    // copy src_addr
    if (is_src_addr == 1)
    {
        rt_memcpy(buf + offset, &ps_header->addr.src_addr, int_len);
        offset += int_len;
    }

    // copy dest_addr
    if (is_dest_addr == 1)
    {
        rt_memcpy(buf + offset, &ps_header->addr.dest_addr, int_len);
        offset += int_len;
    }

    // copy packet_num
    if (is_packet_num == 1)
    {
        rt_memcpy(buf + offset, &ps_header->packet_num, char_len);
        offset += char_len;
    }

    // copy segment_info
    if (segment_flag == 1)
    {
        rt_memcpy(buf + offset, &ps_header->segment_info, (char_len * 2));
        offset += (char_len * 2);
    }

    // copy cmd_type
    rt_memcpy(buf + offset, &ps_header->cmd_type, char_len);
    offset += char_len;

    if (input_data != RT_NULL && input_data_len != 0)
    {
        // handle data compress, output data and data len
        if (compress_flag == 1)
        {
            rt_kprintf("data len before compression %d\n", input_data_len);
            comp_data = rt_malloc(APP_MAX_CODING_DATA_LEN);
            if (comp_data == RT_NULL)
            {
                rt_kprintf("%s, %d malloc fail\n", __FUNCTION__, __LINE__);
                rt_free(buf);
                buf = RT_NULL;
                return -3;
            }
            rt_memset(comp_data, 0, APP_MAX_CODING_DATA_LEN);

            comp_data_len = fastlz_compress(input_data, input_data_len, comp_data);
            rt_kprintf("data len after compression %d\n", comp_data_len);
            // copy data
            if (comp_data_len < input_data_len)
            {
                rt_memcpy(buf + offset, comp_data, comp_data_len);
                offset += comp_data_len;
            }
            else
            {
                rt_kprintf("compressed data is bigger, uncompress\n");
                ps_header->property.compress_flag = 0;
                rt_memcpy(buf, &ps_header->property, sizeof(app_ps_property_t));
                rt_memcpy(buf + offset, input_data, input_data_len);
                offset += input_data_len;
            }
            rt_free(comp_data);
            comp_data = RT_NULL;
        }
        else
        {
            // copy data
            rt_memcpy(buf + offset, input_data, input_data_len);
            offset += input_data_len;
        }
    }

    *output_data = buf;
    *output_data_len = offset;

    return 0;
}

int app_data_decoding(unsigned char *input_data,
                      unsigned int input_data_len,
                      unsigned char **output_data,
                      unsigned int *output_data_len,
                      app_ps_header_t *ps_header)
{
    unsigned char *decomp_data = RT_NULL;
    unsigned int decomp_data_len = 0;
    unsigned char *buf = RT_NULL;
    unsigned int buf_len = 0;
    unsigned int offset = 0;
    unsigned int int_len = sizeof(unsigned int);
    unsigned char char_len = sizeof(unsigned char);

    if (input_data == RT_NULL || input_data_len == 0 || input_data_len > APP_MAX_DECODING_DATA_LEN)
    {
        rt_kprintf("%s, %d input_data error\n", __FUNCTION__, __LINE__);
        return -1;
    }

    // parse property
    rt_memcpy(&ps_header->property, input_data + offset, sizeof(app_ps_property_t));

    unsigned char is_src_addr = ps_header->property.is_src_addr;
    unsigned char is_dest_addr = ps_header->property.is_dest_addr;
    unsigned char is_packet_num = ps_header->property.is_packet_num;
    unsigned char segment_flag = ps_header->property.segment_flag;
    unsigned char compress_flag = ps_header->property.compress_flag;

    rt_kprintf("is_src_addr=%d,is_dest_addr=%d, is_packet_num=%d,segment_flag=%d,compress_flag=%d\n", is_src_addr, is_dest_addr, is_packet_num, segment_flag, compress_flag);
    offset += sizeof(app_ps_property_t);

    // parse src_addr
    if (is_src_addr == 1)
    {
        rt_memcpy(&ps_header->addr.src_addr, input_data + offset, int_len);
        offset += int_len;
    }

    // parse dest_addr
    if (is_dest_addr == 1)
    {
        rt_memcpy(&ps_header->addr.dest_addr, input_data + offset, int_len);
        offset += int_len;
    }

    // parse packet_num
    if (is_packet_num == 1)
    {
        rt_memcpy(&ps_header->packet_num, input_data + offset, char_len);
        offset += char_len;
    }

    // parse segment_info
    if (segment_flag == 1)
    {
        rt_memcpy(&ps_header->segment_info, input_data + offset, (char_len * 2));
        offset += (char_len * 2);
    }

    // parse cmd_type
    rt_memcpy(&ps_header->cmd_type, input_data + offset, char_len);
    offset += char_len;

    // malloc space to save data
    buf_len = input_data_len - offset;
    rt_kprintf("buf_len=%d,offset=%d\n", buf_len, offset);

    if (buf_len > 0)
    {
        buf = (unsigned char *)rt_malloc(buf_len+1);
        if (RT_NULL == buf)
        {
            rt_kprintf("%s, %d malloc fail\n", __FUNCTION__, __LINE__);
            return -2;
        }
        rt_memset(buf, 0, buf_len+1);

        // parse data
        rt_memcpy(buf, input_data + offset, buf_len);

        // handle data decompress, output data and data_len
        if (compress_flag == 1)
        {
            decomp_data = (unsigned char *)rt_malloc(APP_MAX_CODING_DATA_LEN);
            if (RT_NULL == decomp_data)
            {
                rt_kprintf("%s, %d malloc fail\n", __FUNCTION__, __LINE__);
                rt_free(buf);
                buf = RT_NULL;
                return -3;
            }
            rt_memset(decomp_data, 0, APP_MAX_CODING_DATA_LEN);

            rt_kprintf("data len before decompression %d\n", buf_len);
            decomp_data_len = fastlz_decompress(buf, buf_len, decomp_data, APP_MAX_CODING_DATA_LEN);
            rt_kprintf("data len after decompression %d\n", decomp_data_len);
            *output_data = decomp_data;
            *output_data_len = decomp_data_len;

            rt_free(buf);
            buf = RT_NULL;
        }
        else
        {
            *output_data = buf;
            *output_data_len = buf_len;
        }
    }
    return 0;
}
#endif
