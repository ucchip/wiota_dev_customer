#include <rtthread.h>
#if defined(WIOTA_APP_DEMO) || defined(AT_WIOTA_GATEWAY_API)
#include <uc_coding.h>
#include "fastlz.h"
#ifdef AT_WIOTA_GATEWAY_API
#include "uc_cbor.h"
#endif

unsigned char app_packet_num(void)
{
    static unsigned char packet_num = 0;
    return packet_num++;
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
    // unsigned int int_len = sizeof(unsigned int);
    // unsigned char char_len = sizeof(unsigned char);
    unsigned int comp_data_len = 0;
    unsigned char *comp_data = RT_NULL;

    if (ps_header == RT_NULL || input_data_len > APP_MAX_CODING_DATA_LEN)
    {
        rt_kprintf("%s, %d input_data error\n", __FUNCTION__, __LINE__);
        return -1;
    }

    // malloc output_data space, maybe > output_data_len
    buf = (unsigned char *)rt_malloc(buf_len + 4);
    if (RT_NULL == buf)
    {
        rt_kprintf("%s, %d malloc fail\n", __FUNCTION__, __LINE__);
        return -2;
    }
    rt_memset(buf, 0, buf_len);

    // copy property
    //rt_memcpy(buf + offset, &ps_header->property, sizeof(app_ps_property_t));
    //offset += sizeof(app_ps_property_t);
    *((app_ps_property_t *)buf) = ps_header->property;
    offset ++;

    // copy src_addr
    if (is_src_addr == 1)
    {
        //rt_memcpy(buf + offset, &ps_header->addr.src_addr, int_len);
        //offset += int_len;
        *((unsigned int *)(buf + offset)) = ps_header->addr.src_addr;
        offset += 4;
    }

    // copy dest_addr
    if (is_dest_addr == 1)
    {
        //rt_memcpy(buf + offset, &ps_header->addr.dest_addr, int_len);
        //offset += int_len;
        *((unsigned int *)(buf + offset)) = ps_header->addr.dest_addr;
        offset += 4;
    }

    // copy packet_num
    if (is_packet_num == 1)
    {
        //rt_memcpy(buf + offset, &ps_header->packet_num, char_len);
        //offset += char_len;
        *(buf + offset) = ps_header->packet_num;
        offset ++;
    }

    // copy segment_info
    if (segment_flag == 1)
    {
        //rt_memcpy(buf + offset, &ps_header->segment_info, (char_len * 2));
        //offset += (char_len * 2);
        ((app_ps_segment_info_t *)(buf + offset))->total_num = ps_header->segment_info.total_num;
        ((app_ps_segment_info_t *)(buf + offset))->current_num = ps_header->segment_info.current_num;
        offset += 2;
    }

    // copy cmd_type
    //rt_memcpy(buf + offset, &ps_header->cmd_type, char_len);
    //offset += char_len;
    *(buf + offset) = ps_header->cmd_type;
    offset ++;

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
    // unsigned int int_len = sizeof(unsigned int);
    // unsigned char char_len = sizeof(unsigned char);
    unsigned char is_src_addr ;
    unsigned char is_dest_addr ;
    unsigned char is_packet_num;
    unsigned char segment_flag;
    unsigned char compress_flag;

    if (input_data == RT_NULL || input_data_len == 0 || input_data_len > APP_MAX_DECODING_DATA_LEN)
    {
        rt_kprintf("%s, line %d input_data error.input_data_len %d\n", __FUNCTION__, __LINE__, input_data_len);
        return -1;
    }

    // parse property
    //rt_memcpy(&ps_header->property, input_data + offset, sizeof(app_ps_property_t));
    ps_header->property = *((app_ps_property_t *)input_data);
    offset ++;

    is_src_addr = ps_header->property.is_src_addr;
    is_dest_addr = ps_header->property.is_dest_addr;
    is_packet_num = ps_header->property.is_packet_num;
    segment_flag = ps_header->property.segment_flag;
    compress_flag = ps_header->property.compress_flag;

    // rt_kprintf("is_src_addr=%d,is_dest_addr=%d, is_packet_num=%d,segment_flag=%d,compress_flag=%d\n", is_src_addr, is_dest_addr, is_packet_num, segment_flag, compress_flag);
    //offset += sizeof(app_ps_property_t);

    // parse src_addr
    if (is_src_addr == 1)
    {
        //rt_memcpy(&ps_header->addr.src_addr, input_data + offset, int_len);
        ps_header->addr.src_addr = *((unsigned int *)(input_data + offset));
        offset += 4;
    }

    // parse dest_addr
    if (is_dest_addr == 1)
    {
        //rt_memcpy(&ps_header->addr.dest_addr, input_data + offset, int_len);
        ps_header->addr.dest_addr = *((unsigned int *)(input_data + offset));
        offset += 4;
    }

    // parse packet_num
    if (is_packet_num == 1)
    {
        //rt_memcpy(&ps_header->packet_num, input_data + offset, char_len);
        ps_header->packet_num = *( input_data + offset);
        offset ++;
    }

    // parse segment_info
    if (segment_flag == 1)
    {
        //rt_memcpy(&ps_header->segment_info, input_data + offset, (char_len * 2));
        //offset += (char_len * 2);
        // ps_header->segment_info = *((app_ps_segment_info_t *)(input_data + offset));
        ps_header->segment_info.total_num = ((app_ps_segment_info_t *)(input_data + offset))->total_num;
        ps_header->segment_info.current_num = ((app_ps_segment_info_t *)(input_data + offset))->current_num;
        offset += 2;
    }

    // parse cmd_type
    //rt_memcpy(&ps_header->cmd_type, input_data + offset, char_len);
    ps_header->cmd_type = *(input_data + offset);
    offset++;

    // malloc space to save data
    buf_len = input_data_len - offset;
    rt_kprintf("buf_len=%d,offset=%d,total_num=0x%x,current_num=0x%x,cmd_type=0x%x\n", buf_len, offset,
        ps_header->segment_info.total_num,
        ps_header->segment_info.current_num,
        ps_header->cmd_type);

    if (input_data_len > offset)
    {
        buf = (unsigned char *)rt_malloc(buf_len + 1);
        if (RT_NULL == buf)
        {
            rt_kprintf("%s, %d malloc fail\n", __FUNCTION__, __LINE__);
            return -2;
        }
        //rt_memset(buf, 0, buf_len + 1);

        // parse data
        rt_memcpy(buf, input_data + offset, buf_len);
        *(buf + buf_len) = 0;

        // handle data decompress, output data and data_len
        if (compress_flag == 1)
        {
            decomp_data = (unsigned char *)rt_malloc(APP_MAX_DECODING_DATA_LEN);
            if (RT_NULL == decomp_data)
            {
                rt_kprintf("%s line %d malloc fail. len %d\n", __FUNCTION__, __LINE__, APP_MAX_DECODING_DATA_LEN);
                rt_free(buf);
                buf = RT_NULL;
                return -3;
            }
            rt_memset(decomp_data, 0, APP_MAX_DECODING_DATA_LEN);

            rt_kprintf("data len before decompression %d\n", buf_len);
            decomp_data_len = fastlz_decompress(buf, buf_len, decomp_data, APP_MAX_DECODING_DATA_LEN);
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

#ifdef AT_WIOTA_GATEWAY_API
static int app_cmd_auth_req_coding(unsigned char *input_cmd,
                                   unsigned char **output_data,
                                   unsigned int *output_data_len)
{
    app_ps_auth_req_p ps_cmd_p = (app_ps_auth_req_p)input_cmd;
    cn_cbor_errback err;
    cn_cbor *cb_map = cn_cbor_map_create(&err);
    cn_cbor *cb_data;
    int size_local = 0;
    unsigned char *encoded_out = RT_NULL;

    cb_data = cn_cbor_int_create(ps_cmd_p->auth_type, &err);
    cn_cbor_mapput_int(cb_map, 1, cb_data, &err);

    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("app_cmd_auth_req_coding line %d err %d\n", __LINE__, err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }
    cb_data = cn_cbor_string_create((const char *)(ps_cmd_p->aut_code), &err);
    cn_cbor_mapput_int(cb_map, 2, cb_data, &err);

    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("app_cmd_auth_req_coding line %d err %d\n", __LINE__, err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    encoded_out = rt_malloc(APP_MAX_CODING_DATA_LEN);

    if (RT_NULL == encoded_out)
    {
        cn_cbor_free(cb_map);
        rt_kprintf("app_cmd_auth_req_coding line %d\n", __LINE__);
        return -1;
    }

    size_local = cn_cbor_encoder_write(encoded_out, 0, APP_MAX_CODING_DATA_LEN, cb_map);

    // rt_kprintf("codec_demo_encode %ld : ", size_local);
    // for (size_t j = 0; j < size_local; j++)
    // {
    //     rt_kprintf("%02x ", encoded_out[j]);
    // }
    // rt_kprintf("\n");

    if (-1 == size_local)
    {
        rt_free(encoded_out);
        encoded_out = RT_NULL;
        cn_cbor_free(cb_map);
        rt_kprintf("app_cmd_auth_req_coding line %d\n", __LINE__);
        return -1;
    }

    cn_cbor_free(cb_map);

    *output_data = encoded_out;
    *output_data_len = size_local;

    return 0;
}

static int app_cmd_auth_req_decoding(unsigned char *input_data,
                                     unsigned int input_data_len,
                                     unsigned char **output_data)
{
    cn_cbor_errback err = {0};
    const cn_cbor *cb_data;
    cn_cbor *cb_decode;

    app_ps_auth_req_p decode_data = NULL;

    cb_decode = cn_cbor_decode(input_data, input_data_len, &err);

    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("app_cmd_auth_req_decoding err %d\n", err.err);
        return -1;
    }
    else
    {
        decode_data = rt_malloc(sizeof(app_ps_auth_req_t));
        memset(decode_data, 0, sizeof(app_ps_auth_req_t));
    }

    cb_data = cn_cbor_mapget_int(cb_decode, 1);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        rt_kprintf("app_cmd_auth_req_decoding err line %d\n", __LINE__);
        return -2;
    }
    decode_data->auth_type = cb_data->v.sint;
    cb_data = cn_cbor_mapget_int(cb_decode, 2);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        rt_kprintf("app_cmd_auth_req_decoding err line %d\n", __LINE__);
        return -3;
    }
    //decode_data->aut_code = rt_malloc(cb_data->length);
    memcpy(decode_data->aut_code, cb_data->v.str, cb_data->length);

    cn_cbor_free(cb_decode);
    *output_data = (unsigned char *)decode_data;

    return 0;
}

static int app_cmd_auth_res_coding(unsigned char *input_cmd,
                                   unsigned char **output_data,
                                   unsigned int *output_data_len)
{
    app_ps_auth_res_p ps_cmd_p = (app_ps_auth_res_p)input_cmd;
    cn_cbor_errback err;
    cn_cbor *cb_map;
    cn_cbor *cb_data;
    cn_cbor *cb_arr;
    int size_local = 0;
    int freq;
    unsigned char *encoded_out = RT_NULL;
    unsigned int temp = 0;

    cb_map = cn_cbor_map_create(&err);
    
    //xyang
    // cb_data = cn_cbor_int_create(ps_cmd_p->connect_index.state, &err);
    temp = (ps_cmd_p->connect_index.state << 24);
    temp = (ps_cmd_p->connect_index.freq << 16);
    temp = (ps_cmd_p->connect_index.na1 << 8);
    temp = ps_cmd_p->connect_index.na2;
    cb_data = cn_cbor_int_create(temp, &err);
    cn_cbor_mapput_int(cb_map, 1, cb_data, &err);

    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("app_cmd_auth_res_coding line %d err %d\n", __LINE__, err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_int_create(ps_cmd_p->wiota_id, &err);
    cn_cbor_mapput_int(cb_map, 2, cb_data, &err);

    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("app_cmd_auth_res_coding line %d err %d\n", __LINE__, err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_arr = cn_cbor_array_create(&err);
    for (int i = 0; i < APP_MAX_FREQ_LIST_NUM; i++)
    {
        freq = ps_cmd_p->freq_list[i];

        cb_data = cn_cbor_int_create(freq, &err);

        cn_cbor_array_append(cb_arr, cb_data, &err);

        if (err.err != CN_CBOR_NO_ERROR)
        {
            rt_kprintf("err %d\n", err.err);
            cn_cbor_free(cb_map);
            cn_cbor_free(cb_data);
            cn_cbor_free(cb_arr);
            return -1;
        }

        if (255 == freq)
        {
            break;
        }
    }
    cn_cbor_mapput_int(cb_map, 3, cb_arr, &err);

    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        cn_cbor_free(cb_arr);
        return -1;
    }

    encoded_out = rt_malloc(APP_MAX_CODING_DATA_LEN);

    if (NULL == encoded_out)
    {
        cn_cbor_free(cb_map);
        return -1;
    }

    size_local = cn_cbor_encoder_write(encoded_out, 0, APP_MAX_CODING_DATA_LEN, cb_map);

    if (-1 == size_local)
    {
        rt_free(encoded_out);
        encoded_out = RT_NULL;
        cn_cbor_free(cb_map);
        return -1;
    }

    cn_cbor_free(cb_map);

    *output_data = encoded_out;
    *output_data_len = size_local;

    return 0;
}

static int app_cmd_auth_res_decoding(unsigned char *input_data,
                                     unsigned int input_data_len,
                                     unsigned char **output_data)
{
    cn_cbor_errback err = {0};
    const cn_cbor *cb_data;
    const cn_cbor *cb_arr;
    cn_cbor *cb_decode;

    app_ps_auth_res_p decode_data = RT_NULL;

    cb_decode = cn_cbor_decode(input_data, input_data_len, &err);
#if 0
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        return -1;
    }
    else
#endif
    {
        decode_data = rt_malloc(sizeof(app_ps_auth_res_t));
        memset(decode_data, 0, sizeof(app_ps_auth_res_t));
    }

    cb_data = cn_cbor_mapget_int(cb_decode, 1);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        rt_kprintf("app_cmd_auth_res_decoding line %d error \n", __LINE__);
        return -1;
    }

    //xyang
    decode_data->connect_index.state = (cb_data->v.sint >> 24);
    decode_data->connect_index.freq = (cb_data->v.sint >> 16);
    decode_data->connect_index.na1 = (cb_data->v.sint >> 8);
    decode_data->connect_index.na2 = cb_data->v.sint;

    cb_data = cn_cbor_mapget_int(cb_decode, 2);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        rt_kprintf("app_cmd_auth_res_decoding line %d error \n", __LINE__);
        return -1;
    }
    decode_data->wiota_id = cb_data->v.sint;

    cb_arr = cn_cbor_mapget_int(cb_decode, 3);
    if (!cb_arr)
    {
        cn_cbor_free(cb_decode);
        rt_kprintf("app_cmd_auth_res_decoding line %d error \n", __LINE__);
        return -1;
    }
    cb_data = cb_arr->first_child;
    for (int i = 0; i < cb_arr->length; i++)
    {
        decode_data->freq_list[i] = cb_data->v.sint;
        cb_data = cb_data->next;
    }

    cn_cbor_free(cb_decode);
    *output_data = (unsigned char *)decode_data;

    return 0;
}

static int app_cmd_version_verify_coding(unsigned char *input_cmd,
                                         unsigned char **output_data,
                                         unsigned int *output_data_len)
{
    app_ps_version_verify_p ps_cmd_p = (app_ps_version_verify_p)input_cmd;
    cn_cbor_errback err;
    cn_cbor *cb_map;
    cn_cbor *cb_data;
    int size_local = 0;
    unsigned char *encoded_out = RT_NULL;

    cb_map = cn_cbor_map_create(&err);

    cb_data = cn_cbor_data_create((const unsigned char *)(ps_cmd_p->software_version), strlen(ps_cmd_p->software_version) + 1, &err);
    cn_cbor_mapput_int(cb_map, 1, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_data_create((const unsigned char *)(ps_cmd_p->hardware_version), strlen(ps_cmd_p->hardware_version) + 1, &err);
    cn_cbor_mapput_int(cb_map, 2, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_data_create((const unsigned char *)(ps_cmd_p->device_type), strlen(ps_cmd_p->device_type) + 1, &err);
    cn_cbor_mapput_int(cb_map, 3, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    encoded_out = rt_malloc(APP_MAX_CODING_DATA_LEN);

    if (RT_NULL == encoded_out)
    {
        cn_cbor_free(cb_map);
        return -1;
    }

    size_local = cn_cbor_encoder_write(encoded_out, 0, APP_MAX_CODING_DATA_LEN, cb_map);

    if (-1 == size_local)
    {
        rt_free(encoded_out);
        encoded_out = RT_NULL;
        cn_cbor_free(cb_map);
        return -1;
    }

    cn_cbor_free(cb_map);

    *output_data = encoded_out;
    *output_data_len = size_local;

    return 0;
}

static int app_cmd_version_verify_decoding(unsigned char *input_data,
                                           unsigned int input_data_len,
                                           unsigned char **output_data)
{
    cn_cbor_errback err;
    const cn_cbor *cb_data;
    cn_cbor *cb_decode;

    app_ps_version_verify_p decode_data = NULL;

    cb_decode = cn_cbor_decode(input_data, input_data_len, &err);
#if 0
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        return -1;
    }
    else
#endif
    {
        decode_data = rt_malloc(sizeof(app_ps_version_verify_t));
        memset(decode_data, 0, sizeof(app_ps_version_verify_t));
    }

    cb_data = cn_cbor_mapget_int(cb_decode, 1);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    memcpy(decode_data->software_version, cb_data->v.str, cb_data->length);

    cb_data = cn_cbor_mapget_int(cb_decode, 2);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    memcpy(decode_data->hardware_version, cb_data->v.str, cb_data->length);

    cb_data = cn_cbor_mapget_int(cb_decode, 3);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    memcpy(decode_data->device_type, cb_data->v.str, cb_data->length);

    cn_cbor_free(cb_decode);
    *output_data = (unsigned char *)decode_data;

    return 0;
}

static int app_cmd_ota_upgrade_req_coding(unsigned char *input_cmd,
                                          unsigned char **output_data,
                                          unsigned int *output_data_len)
{
    app_ps_ota_upgrade_req_p ps_cmd_p = (app_ps_ota_upgrade_req_p)input_cmd;
    cn_cbor_errback err;
    cn_cbor *cb_map;
    cn_cbor *cb_data;
    cn_cbor *cb_arr;
    int size_local = 0;
    int iote_id;
    unsigned char *encoded_out = RT_NULL;

    cb_map = cn_cbor_map_create(&err);

    cb_data = cn_cbor_int_create(ps_cmd_p->upgrade_type, &err);
    cn_cbor_mapput_int(cb_map, 1, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_int_create(ps_cmd_p->upgrade_range, &err);
    cn_cbor_mapput_int(cb_map, 2, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_arr = cn_cbor_array_create(&err);
    for (int i = 0; i < APP_MAX_IOTE_UPGRADE_NUM; i++)
    {
        iote_id = ps_cmd_p->iote_list[i];

        cb_data = cn_cbor_int_create(iote_id, &err);
        cn_cbor_array_append(cb_arr, cb_data, &err);
        if (err.err != CN_CBOR_NO_ERROR)
        {
            rt_kprintf("err %d\n", err.err);
            cn_cbor_free(cb_map);
            cn_cbor_free(cb_data);
            cn_cbor_free(cb_arr);
            return -1;
        }

        if (0 == iote_id)
        {
            break;
        }
    }
    cn_cbor_mapput_int(cb_map, 3, cb_arr, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        cn_cbor_free(cb_arr);
        return -1;
    }

    cb_data = cn_cbor_data_create((const unsigned char *)(ps_cmd_p->new_version), strlen(ps_cmd_p->new_version) + 1, &err);
    cn_cbor_mapput_int(cb_map, 4, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_data_create((const unsigned char *)(ps_cmd_p->old_version), strlen(ps_cmd_p->old_version) + 1, &err);
    cn_cbor_mapput_int(cb_map, 5, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_data_create((const unsigned char *)(ps_cmd_p->md5), strlen(ps_cmd_p->md5) + 1, &err);
    cn_cbor_mapput_int(cb_map, 6, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_int_create(ps_cmd_p->file_size, &err);
    cn_cbor_mapput_int(cb_map, 7, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_int_create(ps_cmd_p->upgrade_time, &err);
    cn_cbor_mapput_int(cb_map, 8, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_data_create((const unsigned char *)(ps_cmd_p->device_type), strlen(ps_cmd_p->device_type) + 1, &err);
    cn_cbor_mapput_int(cb_map, 9, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_int_create(ps_cmd_p->data_offset, &err);
    cn_cbor_mapput_int(cb_map, 10, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    // cb_data = cn_cbor_int_create(ps_cmd_p->data_length, &err);
    // cn_cbor_mapput_int(cb_map, 10, cb_data, &err);

    cb_data = cn_cbor_data_create((const unsigned char *)(ps_cmd_p->data), ps_cmd_p->data_length, &err);
    cn_cbor_mapput_int(cb_map, 11, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    encoded_out = rt_malloc(APP_MAX_CODING_DATA_LEN);

    if (RT_NULL == encoded_out)
    {
        cn_cbor_free(cb_map);
        return -1;
    }
    size_local = cn_cbor_encoder_write(encoded_out, 0, APP_MAX_CODING_DATA_LEN, cb_map);

    if (-1 == size_local)
    {
        rt_free(encoded_out);
        encoded_out = RT_NULL;
        cn_cbor_free(cb_map);
        return -1;
    }

    cn_cbor_free(cb_map);

    *output_data = encoded_out;
    *output_data_len = size_local;

    return 0;
}

static int app_cmd_ota_upgrade_req_decoding(unsigned char *input_data,
                                            unsigned int input_data_len,
                                            unsigned char **output_data)
{
    cn_cbor_errback err;
    const cn_cbor *cb_data;
    const cn_cbor *cb_arr;
    cn_cbor *cb_decode;

    app_ps_ota_upgrade_req_p decode_data = NULL;

    cb_decode = cn_cbor_decode(input_data, input_data_len, &err);
#if 0
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        return -1;
    }
    else
#endif
    {
        decode_data = rt_malloc(sizeof(app_ps_ota_upgrade_req_t));
        memset(decode_data, 0, sizeof(app_ps_ota_upgrade_req_t));
    }

    cb_data = cn_cbor_mapget_int(cb_decode, 1);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    decode_data->upgrade_type = cb_data->v.sint;

    cb_data = cn_cbor_mapget_int(cb_decode, 2);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    decode_data->upgrade_range = cb_data->v.sint;

    cb_arr = cn_cbor_mapget_int(cb_decode, 3);
    if (!cb_arr)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    cb_data = cb_arr->first_child;
    for (int i = 0; i < cb_arr->length; i++)
    {
        decode_data->iote_list[i] = cb_data->v.sint;
        cb_data = cb_data->next;
    }

    cb_data = cn_cbor_mapget_int(cb_decode, 4);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    memcpy(decode_data->new_version, cb_data->v.str, cb_data->length);

    cb_data = cn_cbor_mapget_int(cb_decode, 5);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    memcpy(decode_data->old_version, cb_data->v.str, cb_data->length);

    cb_data = cn_cbor_mapget_int(cb_decode, 6);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    memcpy(decode_data->md5, cb_data->v.str, cb_data->length);

    cb_data = cn_cbor_mapget_int(cb_decode, 7);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    decode_data->file_size = cb_data->v.sint;

    cb_data = cn_cbor_mapget_int(cb_decode, 8);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    decode_data->upgrade_time = cb_data->v.sint;

    cb_data = cn_cbor_mapget_int(cb_decode, 9);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    memcpy(decode_data->device_type, cb_data->v.str, cb_data->length);

    cb_data = cn_cbor_mapget_int(cb_decode, 10);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    decode_data->data_offset = cb_data->v.sint;
    // cb_data = cn_cbor_mapget_int(cb_decode, 10);
    // decode_data->data_length = cb_data->v.sint;
    cb_data = cn_cbor_mapget_int(cb_decode, 11);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    decode_data->data_length = cb_data->length;

    memcpy(decode_data->data, cb_data->v.str, cb_data->length);

    cn_cbor_free(cb_decode);
    *output_data = (unsigned char *)decode_data;

    return 0;
}

static int app_cmd_ota_upgrade_stop_coding(unsigned char *input_cmd,
                                           unsigned char **output_data,
                                           unsigned int *output_data_len)
{
    app_ps_ota_upgrade_stop_p ps_cmd_p = (app_ps_ota_upgrade_stop_p)input_cmd;
    cn_cbor_errback err;
    cn_cbor *cb_map;
    cn_cbor *cb_data;
    cn_cbor *cb_arr;
    int size_local = 0;
    unsigned int iote_id;
    unsigned char *encoded_out = RT_NULL;

    cb_map = cn_cbor_map_create(&err);

    cb_arr = cn_cbor_array_create(&err);
    for (int i = 0; i < APP_MAX_IOTE_UPGRADE_STOP_NUM; i++)
    {
        iote_id = ps_cmd_p->iote_list[i];

        cb_data = cn_cbor_int_create(iote_id, &err);

        cn_cbor_array_append(cb_arr, cb_data, &err);

        if (err.err != CN_CBOR_NO_ERROR)
        {
            rt_kprintf("err %d\n", err.err);
            cn_cbor_free(cb_map);
            cn_cbor_free(cb_data);
            cn_cbor_free(cb_arr);
            return -1;
        }

        if (0 == iote_id)
        {
            break;
        }
    }
    cn_cbor_mapput_int(cb_map, 1, cb_arr, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        cn_cbor_free(cb_arr);
        return -1;
    }

    encoded_out = rt_malloc(APP_MAX_CODING_DATA_LEN);

    if (RT_NULL == encoded_out)
    {
        cn_cbor_free(cb_map);
        return -1;
    }

    size_local = cn_cbor_encoder_write(encoded_out, 0, APP_MAX_CODING_DATA_LEN, cb_map);

    if (-1 == size_local)
    {
        rt_free(encoded_out);
        encoded_out = RT_NULL;
        cn_cbor_free(cb_map);
        return -1;
    }

    cn_cbor_free(cb_map);

    *output_data = encoded_out;
    *output_data_len = size_local;

    return 0;
}

static int app_cmd_ota_upgrade_stop_decoding(unsigned char *input_data,
                                             unsigned int input_data_len,
                                             unsigned char **output_data)
{
    cn_cbor_errback err;
    const cn_cbor *cb_data;
    const cn_cbor *cb_arr;
    cn_cbor *cb_decode;

    app_ps_ota_upgrade_stop_p decode_data = NULL;

    cb_decode = cn_cbor_decode(input_data, input_data_len, &err);
#if 0
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        return -1;
    }
    else
#endif
    {
        decode_data = rt_malloc(sizeof(app_ps_ota_upgrade_stop_t));
        memset(decode_data, 0, sizeof(app_ps_ota_upgrade_stop_t));
    }

    cb_arr = cn_cbor_mapget_int(cb_decode, 1);
    if (!cb_arr)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    cb_data = cb_arr->first_child;
    for (int i = 0; i < cb_arr->length; i++)
    {
        decode_data->iote_list[i] = cb_data->v.sint;
        cb_data = cb_data->next;
    }

    cn_cbor_free(cb_decode);
    *output_data = (unsigned char *)decode_data;

    return 0;
}

static int app_cmd_ota_upgrade_state_coding(unsigned char *input_cmd,
                                            unsigned char **output_data,
                                            unsigned int *output_data_len)
{
    app_ps_ota_upgrade_state_p ps_cmd_p = (app_ps_ota_upgrade_state_p)input_cmd;
    cn_cbor_errback err;
    cn_cbor *cb_map;
    cn_cbor *cb_data;
    int size_local = 0;
    unsigned char *encoded_out = RT_NULL;

    cb_map = cn_cbor_map_create(&err);

    cb_data = cn_cbor_int_create(ps_cmd_p->upgrade_type, &err);
    cn_cbor_mapput_int(cb_map, 1, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_data_create((const unsigned char *)(ps_cmd_p->new_version), strlen(ps_cmd_p->new_version) + 1, &err);
    cn_cbor_mapput_int(cb_map, 2, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_data_create((const unsigned char *)(ps_cmd_p->old_version), strlen(ps_cmd_p->old_version) + 1, &err);
    cn_cbor_mapput_int(cb_map, 3, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_data_create((const unsigned char *)(ps_cmd_p->device_type), strlen(ps_cmd_p->device_type) + 1, &err);
    cn_cbor_mapput_int(cb_map, 4, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_int_create(ps_cmd_p->process_state, &err);
    cn_cbor_mapput_int(cb_map, 5, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    encoded_out = rt_malloc(APP_MAX_CODING_DATA_LEN);

    if (RT_NULL == encoded_out)
    {
        cn_cbor_free(cb_map);
        return -1;
    }

    size_local = cn_cbor_encoder_write(encoded_out, 0, APP_MAX_CODING_DATA_LEN, cb_map);

    if (-1 == size_local)
    {
        rt_free(encoded_out);
        encoded_out = NULL;
        cn_cbor_free(cb_map);
        return -1;
    }

    cn_cbor_free(cb_map);

    *output_data = encoded_out;
    *output_data_len = size_local;

    return 0;
}

static int app_cmd_ota_upgrade_state_decoding(unsigned char *input_data,
                                              unsigned int input_data_len,
                                              unsigned char **output_data)
{
    cn_cbor_errback err = {0};
    const cn_cbor *cb_data;
    cn_cbor *cb_decode;

    app_ps_ota_upgrade_state_p decode_data = NULL;

    cb_decode = cn_cbor_decode(input_data, input_data_len, &err);
#if 1
    if (RT_NULL == cb_decode)
    {
        rt_kprintf("%s line %d err %d\n", __FUNCTION__, __LINE__, err.err);
        return -1;
    }
    else
#endif
    {
        decode_data = rt_malloc(sizeof(app_ps_ota_upgrade_state_t));
        memset(decode_data, 0, sizeof(app_ps_ota_upgrade_state_t));
    }

    cb_data = cn_cbor_mapget_int(cb_decode, 1);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    decode_data->upgrade_type = cb_data->v.sint;

    cb_data = cn_cbor_mapget_int(cb_decode, 2);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    memcpy(decode_data->new_version, cb_data->v.str, cb_data->length);

    cb_data = cn_cbor_mapget_int(cb_decode, 3);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    memcpy(decode_data->old_version, cb_data->v.str, cb_data->length);

    cb_data = cn_cbor_mapget_int(cb_decode, 4);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    memcpy(decode_data->device_type, cb_data->v.str, cb_data->length);

    cb_data = cn_cbor_mapget_int(cb_decode, 5);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    decode_data->process_state = cb_data->v.sint;

    cn_cbor_free(cb_decode);
    *output_data = (unsigned char *)decode_data;

    return 0;
}

static int app_cmd_data_miss_req_coding(unsigned char *input_cmd,
                                        unsigned char **output_data,
                                        unsigned int *output_data_len)
{
    app_ps_iote_missing_data_req_p ps_cmd_p = (app_ps_iote_missing_data_req_p)input_cmd;
    cn_cbor_errback err;
    cn_cbor *cb_map;
    cn_cbor *cb_data;
    cn_cbor *cb_arr;
    int size_local = 0;
    int offset_data;
    unsigned char *encoded_out = RT_NULL;

    cb_map = cn_cbor_map_create(&err);

    cb_data = cn_cbor_data_create((const unsigned char *)(ps_cmd_p->device_type), strlen(ps_cmd_p->device_type) + 1, &err);
    cn_cbor_mapput_int(cb_map, 1, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_data_create((const unsigned char *)(ps_cmd_p->new_version), strlen(ps_cmd_p->new_version) + 1, &err);
    cn_cbor_mapput_int(cb_map, 2, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_data_create((const unsigned char *)(ps_cmd_p->old_version), strlen(ps_cmd_p->old_version) + 1, &err);
    cn_cbor_mapput_int(cb_map, 3, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_int_create(ps_cmd_p->upgrade_type, &err);
    cn_cbor_mapput_int(cb_map, 4, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    // cb_data = cn_cbor_int_create(ps_cmd_p->miss_data_num, &err);
    // cn_cbor_mapput_int(cb_map, 5, cb_data, &err);

    cb_arr = cn_cbor_array_create(&err);
    for (int i = 0; i < ps_cmd_p->miss_data_num && i < APP_MAX_MISSING_DATA_BLOCK_NUM; i++)
    {
        offset_data = ps_cmd_p->miss_data_offset[i];

        cb_data = cn_cbor_int_create(offset_data, &err);

        cn_cbor_array_append(cb_arr, cb_data, &err);

        if (err.err != CN_CBOR_NO_ERROR)
        {
            rt_kprintf("err %d\n", err.err);
            cn_cbor_free(cb_map);
            cn_cbor_free(cb_data);
            cn_cbor_free(cb_arr);
            return -1;
        }
    }
    cn_cbor_mapput_int(cb_map, 5, cb_arr, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        cn_cbor_free(cb_arr);
        return -1;
    }

    cb_arr = cn_cbor_array_create(&err);
    for (int i = 0; i < ps_cmd_p->miss_data_num && i < APP_MAX_MISSING_DATA_BLOCK_NUM; i++)
    {
        offset_data = ps_cmd_p->miss_data_length[i];

        cb_data = cn_cbor_int_create(offset_data, &err);

        cn_cbor_array_append(cb_arr, cb_data, &err);
        if (err.err != CN_CBOR_NO_ERROR)
        {
            rt_kprintf("err %d\n", err.err);
            cn_cbor_free(cb_map);
            cn_cbor_free(cb_data);
            cn_cbor_free(cb_arr);
            return -1;
        }
    }
    cn_cbor_mapput_int(cb_map, 6, cb_arr, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        cn_cbor_free(cb_arr);
        return -1;
    }

    encoded_out = rt_malloc(APP_MAX_CODING_DATA_LEN);

    if (RT_NULL == encoded_out)
    {
        cn_cbor_free(cb_map);
        return -1;
    }
    size_local = cn_cbor_encoder_write(encoded_out, 0, APP_MAX_CODING_DATA_LEN, cb_map);

    if (-1 == size_local)
    {
        rt_free(encoded_out);
        encoded_out = RT_NULL;
        cn_cbor_free(cb_map);
        return -1;
    }

    cn_cbor_free(cb_map);

    *output_data = encoded_out;
    *output_data_len = size_local;

    return 0;
}

static int app_cmd_data_miss_req_decoding(unsigned char *input_data,
                                          unsigned int input_data_len,
                                          unsigned char **output_data)
{
    cn_cbor_errback err;
    const cn_cbor *cb_data;
    const cn_cbor *cb_arr;
    cn_cbor *cb_decode;

    app_ps_iote_missing_data_req_p decode_data = NULL;

    cb_decode = cn_cbor_decode(input_data, input_data_len, &err);
#if 0
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        return -1;
    }
    else
#endif
    {
        decode_data = rt_malloc(sizeof(app_ps_iote_missing_data_req_t));
        memset(decode_data, 0, sizeof(app_ps_iote_missing_data_req_t));
    }

    cb_data = cn_cbor_mapget_int(cb_decode, 1);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    memcpy(decode_data->device_type, cb_data->v.str, cb_data->length);

    cb_data = cn_cbor_mapget_int(cb_decode, 2);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    memcpy(decode_data->new_version, cb_data->v.str, cb_data->length);

    cb_data = cn_cbor_mapget_int(cb_decode, 3);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    memcpy(decode_data->old_version, cb_data->v.str, cb_data->length);

    cb_data = cn_cbor_mapget_int(cb_decode, 4);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    decode_data->upgrade_type = cb_data->v.sint;

    cb_arr = cn_cbor_mapget_int(cb_decode, 5);
    if (!cb_arr)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    cb_data = cb_arr->first_child;
    decode_data->miss_data_num = cb_arr->length;
    for (int i = 0; i < cb_arr->length && i < APP_MAX_MISSING_DATA_BLOCK_NUM; i++)
    {
        decode_data->miss_data_offset[i] = cb_data->v.sint;
        cb_data = cb_data->next;
    }

    cb_arr = cn_cbor_mapget_int(cb_decode, 6);
    if (!cb_arr)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    cb_data = cb_arr->first_child;
    for (int i = 0; i < cb_arr->length && i < APP_MAX_MISSING_DATA_BLOCK_NUM; i++)
    {
        decode_data->miss_data_length[i] = cb_data->v.sint;
        cb_data = cb_data->next;
    }

    cn_cbor_free(cb_decode);
    *output_data = (unsigned char *)decode_data;

    return 0;
}

static int app_cmd_iote_state_update_coding(unsigned char *input_cmd,
                                            unsigned char **output_data,
                                            unsigned int *output_data_len)
{
    app_ps_iote_state_update_p ps_cmd_p = (app_ps_iote_state_update_p)input_cmd;
    cn_cbor_errback err;
    cn_cbor *cb_map;
    cn_cbor *cb_data;
    int size_local = 0;
    unsigned char *encoded_out = RT_NULL;

    cb_map = cn_cbor_map_create(&err);

    cb_data = cn_cbor_data_create((const unsigned char *)(ps_cmd_p->device_type), strlen(ps_cmd_p->device_type) + 1, &err);
    cn_cbor_mapput_int(cb_map, 1, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_int_create(ps_cmd_p->rssi, &err);
    cn_cbor_mapput_int(cb_map, 2, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cb_data = cn_cbor_int_create(ps_cmd_p->snr, &err);
    cn_cbor_mapput_int(cb_map, 3, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    encoded_out = rt_malloc(APP_MAX_CODING_DATA_LEN);

    if (RT_NULL == encoded_out)
    {
        cn_cbor_free(cb_map);
        return -1;
    }
    size_local = cn_cbor_encoder_write(encoded_out, 0, APP_MAX_CODING_DATA_LEN, cb_map);

    if (-1 == size_local)
    {
        rt_free(encoded_out);
        encoded_out = RT_NULL;
        cn_cbor_free(cb_map);
        return -1;
    }

    cn_cbor_free(cb_map);

    *output_data = encoded_out;
    *output_data_len = size_local;

    return 0;
}

static int app_cmd_iote_response_state_coding(unsigned char *input_cmd,
                                              unsigned char **output_data,
                                              unsigned int *output_data_len)
{
    int *ps_cmd_p = (int *)input_cmd;
    cn_cbor_errback err;
    cn_cbor *cb_map;
    cn_cbor *cb_data;
    int size_local = 0;
    unsigned char *encoded_out = RT_NULL;

    cb_map = cn_cbor_map_create(&err);

    cb_data = cn_cbor_int_create(*ps_cmd_p, &err);
    cn_cbor_mapput_int(cb_map, 1, cb_data, &err);
    if (err.err != CN_CBOR_NO_ERROR)
    {
        rt_kprintf("err %d\n", err.err);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    encoded_out = rt_malloc(APP_MAX_CODING_DATA_LEN);

    if (RT_NULL == encoded_out)
    {
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }
    size_local = cn_cbor_encoder_write(encoded_out, 0, APP_MAX_CODING_DATA_LEN, cb_map);

    if (-1 == size_local)
    {
        rt_free(encoded_out);
        cn_cbor_free(cb_map);
        cn_cbor_free(cb_data);
        return -1;
    }

    cn_cbor_free(cb_map);

    *output_data = encoded_out;
    *output_data_len = size_local;

    return 0;
}

static int app_cmd_iote_state_update_decoding(unsigned char *input_data,
                                              unsigned int input_data_len,
                                              unsigned char **output_data)
{
    cn_cbor_errback err;
    const cn_cbor *cb_data;
    cn_cbor *cb_decode;

    app_ps_iote_state_update_p decode_data = NULL;

    cb_decode = cn_cbor_decode(input_data, input_data_len, &err);
#if 1
    if (cb_decode == RT_NULL)
    {
        rt_kprintf("%s line %d err %d\n", __FUNCTION__, __LINE__, err.err);
        return -1;
    }
    else
#endif
    {
        decode_data = rt_malloc(sizeof(app_ps_iote_state_update_t));
        memset(decode_data, 0, sizeof(app_ps_iote_state_update_t));
    }

    cb_data = cn_cbor_mapget_int(cb_decode, 1);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    //decode_data->device_type = rt_malloc(cb_data->length);
    memcpy(decode_data->device_type, cb_data->v.str, cb_data->length);
    cb_data = cn_cbor_mapget_int(cb_decode, 2);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    decode_data->rssi = cb_data->v.sint;
    cb_data = cn_cbor_mapget_int(cb_decode, 3);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }
    decode_data->snr = cb_data->v.sint;

    cn_cbor_free(cb_decode);
    *output_data = (unsigned char *)decode_data;

    return 0;
}

static int app_cmd_iote_response_state_decoding(unsigned char *input_data,
                                                unsigned int input_data_len,
                                                unsigned char **output_data)
{
    cn_cbor_errback err;
    const cn_cbor *cb_data;
    cn_cbor *cb_decode;

    int *decode_data = NULL;

    cb_decode = cn_cbor_decode(input_data, input_data_len, &err);

    if (cb_decode == RT_NULL)
    {
        rt_kprintf("%s line %d cn_cbor_decode err %d\n", __FUNCTION__, __LINE__, err.err);
        return -1;
    }
    else
    {
        decode_data = rt_malloc(sizeof(int));
        memset(decode_data, 0, sizeof(int));
    }

    cb_data = cn_cbor_mapget_int(cb_decode, 1);
    if (!cb_data)
    {
        cn_cbor_free(cb_decode);
        return -1;
    }

    *decode_data = cb_data->v.sint;

    cn_cbor_free(cb_decode);
    *output_data = (unsigned char *)decode_data;

    return 0;
}

int app_cmd_coding(app_ps_cmd_e input_cmd_type,
                   unsigned char *input_cmd,
                   unsigned char **output_data,
                   unsigned int *output_data_len)
{
    int result = -1;

    switch (input_cmd_type)
    {
    case AUTHENTICATION_REQ:
        result = app_cmd_auth_req_coding(input_cmd, output_data, output_data_len);
        break;
    case AUTHENTICATION_RES:
        result = app_cmd_auth_res_coding(input_cmd, output_data, output_data_len);
        break;
    case VERSION_VERIFY:
        result = app_cmd_version_verify_coding(input_cmd, output_data, output_data_len);
        break;
    case OTA_UPGRADE_REQ:
        result = app_cmd_ota_upgrade_req_coding(input_cmd, output_data, output_data_len);
        break;
    case OTA_UPGRADE_STOP:
        result = app_cmd_ota_upgrade_stop_coding(input_cmd, output_data, output_data_len);
        break;
    case OTA_UPGRADE_STATE:
        result = app_cmd_ota_upgrade_state_coding(input_cmd, output_data, output_data_len);
        break;
    case IOTE_MISSING_DATA_REQ:
        result = app_cmd_data_miss_req_coding(input_cmd, output_data, output_data_len);
        break;
    case IOTE_STATE_UPDATE:
        result = app_cmd_iote_state_update_coding(input_cmd, output_data, output_data_len);
        break;
    case IOTE_RESPON_STATE:
        result = app_cmd_iote_response_state_coding(input_cmd, output_data, output_data_len);
        break;
    default:
        rt_kprintf("wrong cmd\n");
        break;
    }

    return result;
}

int app_cmd_decoding(app_ps_cmd_e input_cmd_type,
                     unsigned char *input_data,
                     unsigned int input_data_len,
                     unsigned char **output_data)
{
    int result = -1;

    // rt_kprintf("app_cmd_decoding input_data_len %d\n", input_data_len);
    // for (int i = 0; i < input_data_len; i++)
    // {
    //     if (i != 0 && i % 16 == 0)
    //     {
    //         rt_kprintf("\n");
    //     }
    //     rt_kprintf("%x ", input_data[i]);
    // }
    // rt_kprintf("\n");

    switch (input_cmd_type)
    {
    case AUTHENTICATION_REQ:
        result = app_cmd_auth_req_decoding(input_data, input_data_len, output_data);
        break;
    case AUTHENTICATION_RES:
        result = app_cmd_auth_res_decoding(input_data, input_data_len, output_data);
        break;
    case VERSION_VERIFY:
        result = app_cmd_version_verify_decoding(input_data, input_data_len, output_data);
        break;
    case OTA_UPGRADE_REQ:
        result = app_cmd_ota_upgrade_req_decoding(input_data, input_data_len, output_data);
        break;
    case OTA_UPGRADE_STOP:
        result = app_cmd_ota_upgrade_stop_decoding(input_data, input_data_len, output_data);
        break;
    case OTA_UPGRADE_STATE:
        result = app_cmd_ota_upgrade_state_decoding(input_data, input_data_len, output_data);
        break;
    case IOTE_MISSING_DATA_REQ:
        result = app_cmd_data_miss_req_decoding(input_data, input_data_len, output_data);
        break;
    case IOTE_STATE_UPDATE:
        result = app_cmd_iote_state_update_decoding(input_data, input_data_len, output_data);
        break;
    case IOTE_RESPON_STATE:
        result = app_cmd_iote_response_state_decoding(input_data, input_data_len, output_data);
        break;

    default:
        rt_kprintf("wrong cmd\n");
        break;
    }

    return result;
}
#endif
#endif // defined(WIOTA_APP_DEMO)
