#if 0 //def WIOTA_APP_DEMO
#include "uc_cbor.h"
#ifdef _RT_THREAD_
#include <rtthread.h>
#endif
#include "uc_wiota_api.h"
#include "test_wiota_api.h"
#include "at.h"
#include "ps_sys.h"
#ifdef _LINUX_
#include <stdlib.h>
#include <string.h>
#else

#endif

void assert_true(int real, const char* caller, int line);
#define ASSERT_TRUE(real) assert_true(real, __FILE__, __LINE__)

void assert_not_null(const void* real, const char* caller, int line);
#define ASSERT_NOT_NULL(real) assert_not_null(real, __FILE__, __LINE__)

void assert_str(const char* exp, const char* real, const char* caller, int line);
#define ASSERT_STR(exp, real) assert_str(exp, real, __FILE__, __LINE__)

void assert_not_null(const void* real, const char* caller, int line)
{
    if (real == NULL) {
        rt_kprintf("%s:%d  should not be NULL\n", caller, line);
    }
}

void assert_true(int real, const char* caller, int line)
{
    if ((real) == 0) {
        rt_kprintf("%s:%d  should be true\n", caller, line);
    }
}

void assert_str(const char* exp, const char* real, const char* caller, int line)
{
    if ((exp == NULL && real != NULL) || (exp != NULL && real == NULL) || (exp && real && strcmp(exp, real) != 0)) {
        rt_kprintf("%s:%d  expected '%s', got '%s'\n", caller, line, exp, real);
    }
}


void codec_test_main(void)
{
    cn_cbor_errback err;
    const cn_cbor *val;
    const char *data = "abc";
    cn_cbor *cb_map = cn_cbor_map_create(&err);
    cn_cbor *cb_int;
    cn_cbor *cb_data;
//    unsigned char encoded[300];
    unsigned char* encoded = rt_malloc(300);
    int enc_sz;
    cn_cbor *cb_decode;
    char val_result[10];

    cn_cbor *a = cn_cbor_array_create(&err);
    ASSERT_NOT_NULL(a);
    ASSERT_TRUE(err.err == CN_CBOR_NO_ERROR);

    cn_cbor_array_append(a, cn_cbor_int_create(256,&err), &err);
    ASSERT_TRUE(err.err == CN_CBOR_NO_ERROR);

    cn_cbor_array_append(a, cn_cbor_string_create("five",&err), &err);
    ASSERT_TRUE(err.err == CN_CBOR_NO_ERROR);


    rt_kprintf("test start 1\n");

    // encode process

    ASSERT_NOT_NULL(cb_map);
    ASSERT_TRUE(err.err == CN_CBOR_NO_ERROR);

    cb_int = cn_cbor_int_create(256, &err);
    ASSERT_NOT_NULL(cb_int);
    ASSERT_TRUE(err.err == CN_CBOR_NO_ERROR);

    cb_data = cn_cbor_data_create((const uint8_t *)data, 4 , &err);
    ASSERT_NOT_NULL(cb_data);
    ASSERT_TRUE(err.err == CN_CBOR_NO_ERROR);

    rt_kprintf("test start 2\n");

    cn_cbor_mapput_int(cb_map, 5, cb_int, &err);
    ASSERT_TRUE(err.err == CN_CBOR_NO_ERROR);
    ASSERT_TRUE(cb_map->length == 2);

    cn_cbor_mapput_int(cb_map, -7, cb_data, &err);
    ASSERT_TRUE(err.err == CN_CBOR_NO_ERROR);
    ASSERT_TRUE(cb_map->length == 4);

    cn_cbor_mapput_string(cb_map, "foo", cn_cbor_string_create(data, &err), &err);
    ASSERT_TRUE(err.err == CN_CBOR_NO_ERROR);
    ASSERT_TRUE(cb_map->length == 6);

    cn_cbor_map_put(cb_map, cn_cbor_string_create("bar", &err), cn_cbor_string_create("qux", &err), &err);
    ASSERT_TRUE(err.err == CN_CBOR_NO_ERROR);
    ASSERT_TRUE(cb_map->length == 8);

    cn_cbor_map_put(cb_map, cn_cbor_string_create("arr", &err), a, &err);
    ASSERT_TRUE(err.err == CN_CBOR_NO_ERROR);

    val = cn_cbor_mapget_int(cb_map, 5);
    ASSERT_NOT_NULL(val);
    ASSERT_TRUE(val->v.sint == 256);

    val = cn_cbor_mapget_int(cb_map, -7);
    ASSERT_NOT_NULL(val);
    ASSERT_STR(val->v.str, "abc");
    val = cn_cbor_mapget_string(cb_map, "foo");
    ASSERT_NOT_NULL(val);
    ASSERT_STR(val->v.str, "abc");

    enc_sz = cn_cbor_encoder_write(encoded, 0, 300, cb_map);

    CBOR_ASSERT(enc_sz != -1);

    rt_kprintf("test parse size %ld : ",enc_sz);
    for (size_t j= 0; j<enc_sz; j++) {
        rt_kprintf("%02x ",encoded[j]);
    }
    rt_kprintf("\n");

    rt_kprintf("cb_map 0x%x 0x%x 0x%x\n",cb_map,cb_map->parent,cb_map->next);

    cn_cbor_free(cb_map);

    // decode process

    cb_decode = cn_cbor_decode(encoded, enc_sz, &err);

    val = cn_cbor_mapget_int(cb_decode, 5);
    ASSERT_NOT_NULL(val);

    rt_kprintf("test int : %d\n",val->v.sint);

    ASSERT_TRUE(val->v.sint == 256);

    val = cn_cbor_mapget_int(cb_decode, -7);
    ASSERT_NOT_NULL(val);
    ASSERT_STR(val->v.str, "abc");

    rt_kprintf("test 5\n");

    val = cn_cbor_mapget_string(cb_decode, "foo");
    ASSERT_NOT_NULL(val);

    memcpy(val_result,val->v.str,val->length);
    val_result[val->length] = '\0';

    ASSERT_STR((const char*)val_result, "abc");

    rt_kprintf("test 6\n");

    val = cn_cbor_mapget_string(cb_decode, "bar");
    ASSERT_NOT_NULL(val);
    ASSERT_STR(val->v.str, "qux");

    cn_cbor_free(cb_decode);

    rt_free(encoded);

}

#endif // WIOTA_APP_DEMO
