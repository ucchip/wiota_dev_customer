#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include "uc_cbor.h"
#include "uc_wiota_api.h"
#include "test_wiota_api.h"
#include "at.h"

#define ENCODED_SIZE 1024
#define DEMO_LIST_MAX_NUM 30
#define DEMO_LIST_NUM 5

/*
basic demo like below map:

{
 0:"data0",
 1:"data1",
 2:"data2",
 3:"data3",
 4:"data4",
 5:"data5",
 6:"data6",
}

more examples in uc_cbor_test.c

*/

// return size, if -1 means fail
unsigned char *codec_demo_encode(int *size_out, unsigned short list_num_in, codec_data_list_t *data_in)
{
    cn_cbor_errback err;
    cn_cbor *cb_map = cn_cbor_map_create(&err);
    cn_cbor *cb_data;
    codec_data_list_t *data_next = data_in;
    int size_local;
    unsigned char *encoded_out = rt_malloc(ENCODED_SIZE);

    rt_kprintf("codec_demo_encode start\n");

    for (unsigned short i = 0; i < list_num_in; i++)
    {
        if (data_next && data_next->length != 0 && data_next->data_ptr)
        {
            cb_data = cn_cbor_data_create((const unsigned char *)(data_next->data_ptr), data_next->length, &err);
            if (err.err != CN_CBOR_NO_ERROR)
            {
                rt_kprintf("err %d\n", err.err);
            }
            else
            {
                cn_cbor_mapput_int(cb_map, i, cb_data, &err);
            }
        }

        data_next += 1;
    }

    size_local = cn_cbor_encoder_write(encoded_out, 0, ENCODED_SIZE, cb_map);

    rt_kprintf("codec_demo_encode %ld : ", size_local);
    for (size_t j = 0; j < size_local; j++)
    {
        rt_kprintf("%02x ", encoded_out[j]);
    }
    rt_kprintf("\n");

    cn_cbor_free(cb_map);

    *size_out = size_local;

    rt_kprintf("codec_demo_encode end\n");

    return encoded_out;
}

codec_data_list_t *codec_demo_decode(unsigned char *encoded_in, unsigned short len, unsigned short *list_num_out)
{
    cn_cbor_errback err;
    const cn_cbor *val;
    cn_cbor *cb_decode;
    codec_data_list_t *decode_data = rt_malloc(sizeof(codec_data_list_t) * DEMO_LIST_MAX_NUM);
    unsigned short list_num_local = 0;
    codec_data_list_t *data_out = decode_data;

    rt_kprintf("codec_demo_decode start\n");

    memset(decode_data, 0, sizeof(codec_data_list_t) * DEMO_LIST_MAX_NUM);
    cb_decode = cn_cbor_decode(encoded_in, len, &err);

    for (unsigned short i = 0; i < DEMO_LIST_MAX_NUM; i++)
    {
        val = cn_cbor_mapget_int(cb_decode, i);
        if (val)
        {
            list_num_local++;
            decode_data->length = val->length;
            decode_data->data_ptr = rt_malloc(val->length);
            memcpy(decode_data->data_ptr, val->v.str, val->length);
            decode_data += 1; // jump to next codec_data_list_t
        }
        else
        {
            break;
        }
    }

    cn_cbor_free(cb_decode);

    *list_num_out = list_num_local;
    if (0 == list_num_local)
    {
        rt_free(decode_data);
        decode_data = NULL;
        data_out = NULL;
    }

    rt_kprintf("codec_demo_decode end\n");

    return data_out;
}

void codec_demon_test(void)
{
    codec_data_list_t *encode_data = rt_malloc(sizeof(codec_data_list_t) * DEMO_LIST_MAX_NUM);
    codec_data_list_t *cur_node = encode_data;
    codec_data_list_t *decode_data = NULL;
    unsigned char *encoded = NULL;
    int encoded_size = 0;
    unsigned short list_num = 0;
    unsigned short i, j;

    memset(encode_data, 0, sizeof(codec_data_list_t) * DEMO_LIST_MAX_NUM);

    // init data
    for (i = 0; i < DEMO_LIST_NUM; i++)
    {
        cur_node->length = i + 1;
        cur_node->data_ptr = rt_malloc(cur_node->length);
        for (j = 0; j < cur_node->length; j++)
        {
            cur_node->data_ptr[j] = j + 1;
        }
        cur_node += 1; // jump to next codec_data_list_t
    }

    // encode
    encoded = codec_demo_encode(&encoded_size, DEMO_LIST_NUM, encode_data);
    rt_kprintf("demon encode %d : ", encoded_size);
    for (j = 0; j < encoded_size; j++)
    {
        rt_kprintf("%02x ", encoded[j]);
    }
    rt_kprintf("\n");

    // clear data
    cur_node = encode_data;
    for (i = 0; i < DEMO_LIST_NUM; i++)
    {
        if (cur_node->data_ptr)
        {
            rt_free(cur_node->data_ptr);
            cur_node->data_ptr = NULL;
        }
        cur_node += 1; // jump to next codec_data_list_t
    }
    rt_free(encode_data);

    // -------------------------------------------------------------------------------------
    // decode
    decode_data = codec_demo_decode(encoded, encoded_size, &list_num);

    rt_free(encoded);

    // check data
    cur_node = decode_data;
    for (i = 0; i < list_num; i++)
    {
        if (cur_node->data_ptr)
        {
            rt_kprintf("demon decode len %d data: ", cur_node->length);
            for (j = 0; j < cur_node->length; j++)
            {
                rt_kprintf("%02x ", cur_node->data_ptr[j]);
            }
            rt_kprintf("\n");
            rt_free(cur_node->data_ptr);
        }
        cur_node += 1; // jump to next codec_data_list_t
    }
    rt_free(decode_data);
}

#endif // WIOTA_APP_DEMO
