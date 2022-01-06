#include "purc.h"
#include "purc-variant.h"
#include "private/variant.h"


#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#define MIN_BUFFER     512
#define MAX_BUFFER     1024 * 1024 * 1024

char* variant_to_string(purc_variant_t v)
{
    purc_rwstream_t my_rws = purc_rwstream_new_buffer(MIN_BUFFER, MAX_BUFFER);
    size_t len_expected = 0;
    purc_variant_serialize(v, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    char* buf = (char*)purc_rwstream_get_mem_buffer_ex(my_rws, NULL, NULL, true);
    purc_rwstream_destroy(my_rws);
    return buf;
}

TEST(displace, object_object)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    size_t sz = 0;
    const char     *k1 = "hello";
    purc_variant_t  v1 = purc_variant_make_string("world", false);
    const char     *k2 = "foo";
    purc_variant_t  v2 = purc_variant_make_string("bar", true);
    const char     *k3 = "damn";
    purc_variant_t  v3 = purc_variant_make_string("good", true);

    purc_variant_t obj;
    obj = purc_variant_make_object_by_static_ckey(3, k1, v1, k2, v2, k3, v3);
    ASSERT_NE(obj, PURC_VARIANT_INVALID);
    ASSERT_EQ(obj->refc, 1);
    sz = purc_variant_object_get_size(obj);
    ASSERT_EQ(sz, 3);


    purc_variant_t src;
    src = purc_variant_make_object_by_static_ckey(2, k2, v2, k3, v3);
    ASSERT_NE(src, PURC_VARIANT_INVALID);
    ASSERT_EQ(src->refc, 1);
    sz = purc_variant_object_get_size(src);
    ASSERT_EQ(sz, 2);

    bool result = purc_variant_container_displace(obj, src, true);
    ASSERT_EQ(result, true);

    sz = purc_variant_object_get_size(obj);
    ASSERT_EQ(sz, 2);

    purc_variant_t v = purc_variant_object_get_by_ckey(obj, k1);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);

    v = purc_variant_object_get_by_ckey(obj, k2);
    ASSERT_EQ(v, v2);

    v = purc_variant_object_get_by_ckey(obj, k3);
    ASSERT_EQ(v, v3);

    purc_variant_unref(src);
    purc_variant_unref(obj);

    purc_variant_unref(v1);
    purc_variant_unref(v2);
    purc_variant_unref(v3);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

#if 0
TEST(displace, array_array)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    size_t sz = 0;

    char json[] = "[{\"id\":1},{\"id\":2}]";
    purc_variant_t array = purc_variant_make_from_json_string(json,
            strlen(json));
    ASSERT_NE(array, PURC_VARIANT_INVALID);
    ASSERT_EQ(array->refc, 1);
    sz = purc_variant_array_get_size(array);
    ASSERT_EQ(sz, 2);


    char json2[] = "[{\"id\":3},{\"id\":4}]";
    purc_variant_t src = purc_variant_make_from_json_string(json2,
            strlen(json2));
    ASSERT_NE(src, PURC_VARIANT_INVALID);
    ASSERT_EQ(src->refc, 1);
    sz = purc_variant_array_get_size(array);
    ASSERT_EQ(sz, 2);

    bool result = purc_variant_container_displace(array, src, true);
    ASSERT_EQ(result, true);

    char* array_str = variant_to_string(array);
    char* src_str = variant_to_string(src);
    ASSERT_STREQ(array_str, src_str);
    fprintf(stderr, "dst=%s\n", array_str);
    fprintf(stderr, "src=%s\n", src_str);

    purc_variant_unref(array);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}
#endif

TEST(append, array_array)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    char dst_str[] = "[{\"id\":1},{\"id\":2}]";
    char src_str[] = "[{\"id\":3},{\"id\":4}]";
    char cmp_str[] = "[{\"id\":1},{\"id\":2},{\"id\":3},{\"id\":4}]";

    purc_variant_t dst = purc_variant_make_from_json_string(dst_str,
            strlen(dst_str));
    ASSERT_NE(dst, PURC_VARIANT_INVALID);

    purc_variant_t src = purc_variant_make_from_json_string(src_str,
            strlen(src_str));
    ASSERT_NE(src, PURC_VARIANT_INVALID);

    purc_variant_t cmp = purc_variant_make_from_json_string(cmp_str,
            strlen(cmp_str));
    ASSERT_NE(cmp, PURC_VARIANT_INVALID);

    bool result = purc_variant_array_append_another(dst, src, true);
    ASSERT_EQ(result, true);

    char* dst_result = variant_to_string(dst);
    char* cmp_result = variant_to_string(cmp);
    ASSERT_STREQ(dst_result, cmp_result);

    free(cmp_result);
    free(dst_result);

    purc_variant_unref(cmp);
    purc_variant_unref(src);
    purc_variant_unref(dst);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}
