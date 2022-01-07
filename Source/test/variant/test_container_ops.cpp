#include "purc.h"
#include "purc-variant.h"
#include "private/variant.h"


#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#define PRINTF(...)                                                       \
    do {                                                                  \
        fprintf(stderr, "\e[0;32m[          ] \e[0m");                    \
        fprintf(stderr, __VA_ARGS__);                                     \
    } while(false)

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

    purc_variant_t v = purc_variant_object_get_by_ckey(obj, k1, false);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);

    v = purc_variant_object_get_by_ckey(obj, k2, false);
    ASSERT_EQ(v, v2);

    v = purc_variant_object_get_by_ckey(obj, k3, false);
    ASSERT_EQ(v, v3);

    purc_variant_unref(src);
    purc_variant_unref(obj);

    purc_variant_unref(v1);
    purc_variant_unref(v2);
    purc_variant_unref(v3);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

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

    char dst_str[] = "[{\"id\":1},{\"id\":2}]";
    char src_str[] = "[{\"id\":3},{\"id\":4}]";
    char cmp_str[] = "[{\"id\":3},{\"id\":4}]";

    purc_variant_t dst = purc_variant_make_from_json_string(dst_str,
            strlen(dst_str));
    ASSERT_NE(dst, PURC_VARIANT_INVALID);

    purc_variant_t src = purc_variant_make_from_json_string(src_str,
            strlen(src_str));
    ASSERT_NE(src, PURC_VARIANT_INVALID);

    purc_variant_t cmp = purc_variant_make_from_json_string(cmp_str,
            strlen(cmp_str));
    ASSERT_NE(cmp, PURC_VARIANT_INVALID);

    bool result = purc_variant_container_displace(dst, src, true);
    ASSERT_EQ(result, true);

    char* dst_result = variant_to_string(dst);
    char* cmp_result = variant_to_string(cmp);
    PRINTF("dst=%s\n", dst_result);
    PRINTF("cmp=%s\n", cmp_result);
    ASSERT_STREQ(dst_result, cmp_result);

    free(cmp_result);
    free(dst_result);

    purc_variant_unref(cmp);
    purc_variant_unref(src);
    purc_variant_unref(dst);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

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
    PRINTF("dst=%s\n", dst_result);
    PRINTF("cmp=%s\n", cmp_result);
    ASSERT_STREQ(dst_result, cmp_result);

    free(cmp_result);
    free(dst_result);

    purc_variant_unref(cmp);
    purc_variant_unref(src);
    purc_variant_unref(dst);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(prepend, array_array)
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
    char cmp_str[] = "[{\"id\":3},{\"id\":4},{\"id\":1},{\"id\":2}]";

    purc_variant_t dst = purc_variant_make_from_json_string(dst_str,
            strlen(dst_str));
    ASSERT_NE(dst, PURC_VARIANT_INVALID);

    purc_variant_t src = purc_variant_make_from_json_string(src_str,
            strlen(src_str));
    ASSERT_NE(src, PURC_VARIANT_INVALID);

    purc_variant_t cmp = purc_variant_make_from_json_string(cmp_str,
            strlen(cmp_str));
    ASSERT_NE(cmp, PURC_VARIANT_INVALID);

    bool result = purc_variant_array_prepend_another(dst, src, true);
    ASSERT_EQ(result, true);

    char* dst_result = variant_to_string(dst);
    char* cmp_result = variant_to_string(cmp);
    PRINTF("dst=%s\n", dst_result);
    PRINTF("cmp=%s\n", cmp_result);
    ASSERT_STREQ(dst_result, cmp_result);

    free(cmp_result);
    free(dst_result);

    purc_variant_unref(cmp);
    purc_variant_unref(src);
    purc_variant_unref(dst);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(merge, object_object)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    char dst_str[] = "{\"id\":1,\"name\":\"C Language\"}";
    char src_str[] = "{\"page\":325,\"size\":1024}";
    char cmp_str[] = "{\"id\":1,\"name\":\"C Language\",\
         \"page\":325,\"size\":1024}";

    purc_variant_t dst = purc_variant_make_from_json_string(dst_str,
            strlen(dst_str));
    ASSERT_NE(dst, PURC_VARIANT_INVALID);

    purc_variant_t src = purc_variant_make_from_json_string(src_str,
            strlen(src_str));
    ASSERT_NE(src, PURC_VARIANT_INVALID);

    purc_variant_t cmp = purc_variant_make_from_json_string(cmp_str,
            strlen(cmp_str));
    ASSERT_NE(cmp, PURC_VARIANT_INVALID);

    bool result = purc_variant_object_merge_another(dst, src, true);
    ASSERT_EQ(result, true);

    char* dst_result = variant_to_string(dst);
    char* cmp_result = variant_to_string(cmp);
    PRINTF("dst=%s\n", dst_result);
    PRINTF("cmp=%s\n", cmp_result);
    ASSERT_STREQ(dst_result, cmp_result);

    free(cmp_result);
    free(dst_result);

    purc_variant_unref(cmp);
    purc_variant_unref(src);
    purc_variant_unref(dst);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(insertBefore, array_array)
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
    char cmp_str[] = "[{\"id\":1},{\"id\":3},{\"id\":4},{\"id\":2}]";

    purc_variant_t dst = purc_variant_make_from_json_string(dst_str,
            strlen(dst_str));
    ASSERT_NE(dst, PURC_VARIANT_INVALID);

    purc_variant_t src = purc_variant_make_from_json_string(src_str,
            strlen(src_str));
    ASSERT_NE(src, PURC_VARIANT_INVALID);

    purc_variant_t cmp = purc_variant_make_from_json_string(cmp_str,
            strlen(cmp_str));
    ASSERT_NE(cmp, PURC_VARIANT_INVALID);

    bool result = purc_variant_array_insert_another_before(dst, 1, src, true);
    ASSERT_EQ(result, true);

    char* dst_result = variant_to_string(dst);
    char* cmp_result = variant_to_string(cmp);
    PRINTF("dst=%s\n", dst_result);
    PRINTF("cmp=%s\n", cmp_result);
    ASSERT_STREQ(dst_result, cmp_result);

    free(cmp_result);
    free(dst_result);

    purc_variant_unref(cmp);
    purc_variant_unref(src);
    purc_variant_unref(dst);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(insertAfter, array_array)
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
    char cmp_str[] = "[{\"id\":1},{\"id\":3},{\"id\":4},{\"id\":2}]";

    purc_variant_t dst = purc_variant_make_from_json_string(dst_str,
            strlen(dst_str));
    ASSERT_NE(dst, PURC_VARIANT_INVALID);

    purc_variant_t src = purc_variant_make_from_json_string(src_str,
            strlen(src_str));
    ASSERT_NE(src, PURC_VARIANT_INVALID);

    purc_variant_t cmp = purc_variant_make_from_json_string(cmp_str,
            strlen(cmp_str));
    ASSERT_NE(cmp, PURC_VARIANT_INVALID);

    bool result = purc_variant_array_insert_another_after(dst, 0, src, true);
    ASSERT_EQ(result, true);

    char* dst_result = variant_to_string(dst);
    char* cmp_result = variant_to_string(cmp);
    PRINTF("dst=%s\n", dst_result);
    PRINTF("cmp=%s\n", cmp_result);
    ASSERT_STREQ(dst_result, cmp_result);

    free(cmp_result);
    free(dst_result);

    purc_variant_unref(cmp);
    purc_variant_unref(src);
    purc_variant_unref(dst);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(unite, set_array)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    char obj_1_str[] = "{\"id\":1,\"name\":\"1_name\"}";
    char obj_2_str[] = "{\"id\":2,\"name\":\"2_name\"}";
    char obj_3_str[] = "{\"id\":3,\"name\":\"3_name\"}";
    char src_str[] = "["\
                      "{\"id\":3,\"name\":\"3_name\"},"\
                      "{\"id\":4,\"name\":\"4_name\"},"\
                      "{\"id\":5,\"name\":\"5_name\"},"\
                      "{\"id\":6,\"name\":\"6_name\"}"\
                      "]";

    char cmp_str[] = "{"\
                      "{\"id\":1,\"name\":\"1_name\"},"\
                      "{\"id\":2,\"name\":\"2_name\"},"\
                      "{\"id\":3,\"name\":\"3_name\"},"\
                      "{\"id\":4,\"name\":\"4_name\"},"\
                      "{\"id\":5,\"name\":\"5_name\"},"\
                      "{\"id\":6,\"name\":\"6_name\"}"\
                      "]";

    purc_variant_t obj_1 = purc_variant_make_from_json_string(obj_1_str,
            strlen(obj_1_str));
    ASSERT_NE(obj_1, PURC_VARIANT_INVALID);
    purc_variant_t obj_2 = purc_variant_make_from_json_string(obj_2_str,
            strlen(obj_2_str));
    ASSERT_NE(obj_2, PURC_VARIANT_INVALID);
    purc_variant_t obj_3 = purc_variant_make_from_json_string(obj_3_str,
            strlen(obj_3_str));
    ASSERT_NE(obj_3, PURC_VARIANT_INVALID);

    purc_variant_t dst = purc_variant_make_set(3, PURC_VARIANT_INVALID,
            obj_1, obj_2, obj_3);
    ASSERT_NE(dst, PURC_VARIANT_INVALID);

    purc_variant_t src = purc_variant_make_from_json_string(src_str,
            strlen(src_str));
    ASSERT_NE(src, PURC_VARIANT_INVALID);

    bool result = purc_variant_set_unite(dst, src, true);
    ASSERT_EQ(result, true);

    char* dst_result = variant_to_string(dst);
    char* cmp_result = cmp_str;
    PRINTF("dst=%s\n", dst_result);
    PRINTF("cmp=%s\n", cmp_result);
    ASSERT_STREQ(dst_result, cmp_result);

    free(dst_result);

    purc_variant_unref(src);
    purc_variant_unref(dst);

    purc_variant_unref(obj_1);
    purc_variant_unref(obj_2);
    purc_variant_unref(obj_3);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(intersect, set_array)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    char obj_1_str[] = "{\"id\":1,\"name\":\"1_name\"}";
    char obj_2_str[] = "{\"id\":2,\"name\":\"2_name\"}";
    char obj_3_str[] = "{\"id\":3,\"name\":\"3_name\"}";
    char src_str[] = "["\
                      "{\"id\":3,\"name\":\"3_name\"},"\
                      "{\"id\":4,\"name\":\"4_name\"},"\
                      "{\"id\":5,\"name\":\"5_name\"},"\
                      "{\"id\":6,\"name\":\"6_name\"}"\
                      "]";

    char cmp_str[] = "{"\
                      "{\"id\":3,\"name\":\"3_name\"}"\
                      "]";


    purc_variant_t obj_1 = purc_variant_make_from_json_string(obj_1_str,
            strlen(obj_1_str));
    ASSERT_NE(obj_1, PURC_VARIANT_INVALID);
    purc_variant_t obj_2 = purc_variant_make_from_json_string(obj_2_str,
            strlen(obj_2_str));
    ASSERT_NE(obj_2, PURC_VARIANT_INVALID);
    purc_variant_t obj_3 = purc_variant_make_from_json_string(obj_3_str,
            strlen(obj_3_str));
    ASSERT_NE(obj_3, PURC_VARIANT_INVALID);

    purc_variant_t dst = purc_variant_make_set(3, PURC_VARIANT_INVALID,
            obj_1, obj_2, obj_3);
    ASSERT_NE(dst, PURC_VARIANT_INVALID);

    purc_variant_t src = purc_variant_make_from_json_string(src_str,
            strlen(src_str));
    ASSERT_NE(src, PURC_VARIANT_INVALID);

    bool result = purc_variant_set_intersect(dst, src, true);
    ASSERT_EQ(result, true);

    char* dst_result = variant_to_string(dst);
    char* cmp_result = cmp_str;
    PRINTF("dst=%s\n", dst_result);
    PRINTF("cmp=%s\n", cmp_result);
    ASSERT_STREQ(dst_result, cmp_result);

    free(dst_result);

    purc_variant_unref(src);
    purc_variant_unref(dst);

    purc_variant_unref(obj_1);
    purc_variant_unref(obj_2);
    purc_variant_unref(obj_3);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(subtract, set_array)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    char obj_1_str[] = "{\"id\":1,\"name\":\"1_name\"}";
    char obj_2_str[] = "{\"id\":2,\"name\":\"2_name\"}";
    char obj_3_str[] = "{\"id\":3,\"name\":\"3_name\"}";
    char src_str[] = "["\
                      "{\"id\":3,\"name\":\"3_name\"},"\
                      "{\"id\":4,\"name\":\"4_name\"},"\
                      "{\"id\":5,\"name\":\"5_name\"},"\
                      "{\"id\":6,\"name\":\"6_name\"}"\
                      "]";

    char cmp_str[] = "{"\
                      "{\"id\":1,\"name\":\"1_name\"},"\
                      "{\"id\":2,\"name\":\"2_name\"}"\
                      "]";


    purc_variant_t obj_1 = purc_variant_make_from_json_string(obj_1_str,
            strlen(obj_1_str));
    ASSERT_NE(obj_1, PURC_VARIANT_INVALID);
    purc_variant_t obj_2 = purc_variant_make_from_json_string(obj_2_str,
            strlen(obj_2_str));
    ASSERT_NE(obj_2, PURC_VARIANT_INVALID);
    purc_variant_t obj_3 = purc_variant_make_from_json_string(obj_3_str,
            strlen(obj_3_str));
    ASSERT_NE(obj_3, PURC_VARIANT_INVALID);

    purc_variant_t dst = purc_variant_make_set(3, PURC_VARIANT_INVALID,
            obj_1, obj_2, obj_3);
    ASSERT_NE(dst, PURC_VARIANT_INVALID);

    purc_variant_t src = purc_variant_make_from_json_string(src_str,
            strlen(src_str));
    ASSERT_NE(src, PURC_VARIANT_INVALID);

    bool result = purc_variant_set_subtract(dst, src, true);
    ASSERT_EQ(result, true);

    char* dst_result = variant_to_string(dst);
    char* cmp_result = cmp_str;
    PRINTF("dst=%s\n", dst_result);
    PRINTF("cmp=%s\n", cmp_result);
    ASSERT_STREQ(dst_result, cmp_result);

    free(dst_result);

    purc_variant_unref(src);
    purc_variant_unref(dst);

    purc_variant_unref(obj_1);
    purc_variant_unref(obj_2);
    purc_variant_unref(obj_3);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(xor, set_array)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    char obj_1_str[] = "{\"id\":1,\"name\":\"1_name\"}";
    char obj_2_str[] = "{\"id\":2,\"name\":\"2_name\"}";
    char obj_3_str[] = "{\"id\":3,\"name\":\"3_name\"}";
    char src_str[] = "["\
                      "{\"id\":3,\"name\":\"3_name\"},"\
                      "{\"id\":4,\"name\":\"4_name\"},"\
                      "{\"id\":5,\"name\":\"5_name\"},"\
                      "{\"id\":6,\"name\":\"6_name\"}"\
                      "]";

    char cmp_str[] = "{"\
                      "{\"id\":1,\"name\":\"1_name\"},"\
                      "{\"id\":2,\"name\":\"2_name\"},"\
                      "{\"id\":4,\"name\":\"4_name\"},"\
                      "{\"id\":5,\"name\":\"5_name\"},"\
                      "{\"id\":6,\"name\":\"6_name\"}"\
                      "]";


    purc_variant_t obj_1 = purc_variant_make_from_json_string(obj_1_str,
            strlen(obj_1_str));
    ASSERT_NE(obj_1, PURC_VARIANT_INVALID);
    purc_variant_t obj_2 = purc_variant_make_from_json_string(obj_2_str,
            strlen(obj_2_str));
    ASSERT_NE(obj_2, PURC_VARIANT_INVALID);
    purc_variant_t obj_3 = purc_variant_make_from_json_string(obj_3_str,
            strlen(obj_3_str));
    ASSERT_NE(obj_3, PURC_VARIANT_INVALID);

    purc_variant_t dst = purc_variant_make_set(3, PURC_VARIANT_INVALID,
            obj_1, obj_2, obj_3);
    ASSERT_NE(dst, PURC_VARIANT_INVALID);

    purc_variant_t src = purc_variant_make_from_json_string(src_str,
            strlen(src_str));
    ASSERT_NE(src, PURC_VARIANT_INVALID);

    bool result = purc_variant_set_xor(dst, src, true);
    ASSERT_EQ(result, true);

    char* dst_result = variant_to_string(dst);
    char* cmp_result = cmp_str;
    PRINTF("dst=%s\n", dst_result);
    PRINTF("cmp=%s\n", cmp_result);
    ASSERT_STREQ(dst_result, cmp_result);

    free(dst_result);

    purc_variant_unref(src);
    purc_variant_unref(dst);

    purc_variant_unref(obj_1);
    purc_variant_unref(obj_2);
    purc_variant_unref(obj_3);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(overwrite, set_array)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    char obj_1_str[] = "{\"id\":1,\"name\":\"1_name\"}";
    char obj_2_str[] = "{\"id\":2,\"name\":\"2_name\"}";
    char obj_3_str[] = "{\"id\":3,\"name\":\"3_name\"}";
    char src_str[] = "["\
                      "{\"id\":2,\"name\":\"2_name_update\"},"\
                      "{\"id\":3,\"name\":\"3_name_update\"},"\
                      "{\"id\":4,\"name\":\"4_name\"},"\
                      "{\"id\":5,\"name\":\"5_name\"},"\
                      "{\"id\":6,\"name\":\"6_name\"}"\
                      "]";

    char cmp_str[] = "{"\
                      "{\"id\":1,\"name\":\"1_name\"},"\
                      "{\"id\":2,\"name\":\"2_name_update\"},"\
                      "{\"id\":3,\"name\":\"3_name_update\"},"\
                      "{\"id\":4,\"name\":\"4_name\"},"\
                      "{\"id\":5,\"name\":\"5_name\"},"\
                      "{\"id\":6,\"name\":\"6_name\"}"\
                      "]";


    purc_variant_t obj_1 = purc_variant_make_from_json_string(obj_1_str,
            strlen(obj_1_str));
    ASSERT_NE(obj_1, PURC_VARIANT_INVALID);
    purc_variant_t obj_2 = purc_variant_make_from_json_string(obj_2_str,
            strlen(obj_2_str));
    ASSERT_NE(obj_2, PURC_VARIANT_INVALID);
    purc_variant_t obj_3 = purc_variant_make_from_json_string(obj_3_str,
            strlen(obj_3_str));
    ASSERT_NE(obj_3, PURC_VARIANT_INVALID);

    purc_variant_t dst = purc_variant_make_set_by_ckey(3, "id",
            obj_1, obj_2, obj_3);
    ASSERT_NE(dst, PURC_VARIANT_INVALID);

    purc_variant_t src = purc_variant_make_from_json_string(src_str,
            strlen(src_str));
    ASSERT_NE(src, PURC_VARIANT_INVALID);

    bool result = purc_variant_set_overwrite(dst, src, true);
    ASSERT_EQ(result, true);

    char* dst_result = variant_to_string(dst);
    char* cmp_result = cmp_str;
    PRINTF("dst=%s\n", dst_result);
    PRINTF("cmp=%s\n", cmp_result);
    ASSERT_STREQ(dst_result, cmp_result);

    free(dst_result);

    purc_variant_unref(src);
    purc_variant_unref(dst);

    purc_variant_unref(obj_1);
    purc_variant_unref(obj_2);
    purc_variant_unref(obj_3);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}
