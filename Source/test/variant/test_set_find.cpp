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

TEST(set, unique_key_find)
{
    if (0)
        return;

    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    char obj_1_str[] = "{\"id\":\"clock\",\"interval\":1000,\"active\":\"yes\"}";
    char obj_2_str[] = "{\"id\":\"input\",\"interval\":1500,\"active\":\"yes\"}";
    char obj_3_str[] = "{\"id\":\"input\",\"active\":\"no\"}";


    purc_variant_t obj_1 = purc_variant_make_from_json_string(obj_1_str,
            strlen(obj_1_str));
    ASSERT_NE(obj_1, PURC_VARIANT_INVALID);
    purc_variant_t obj_2 = purc_variant_make_from_json_string(obj_2_str,
            strlen(obj_2_str));
    ASSERT_NE(obj_2, PURC_VARIANT_INVALID);
    purc_variant_t obj_3 = purc_variant_make_from_json_string(obj_3_str,
            strlen(obj_3_str));
    ASSERT_NE(obj_3, PURC_VARIANT_INVALID);

    purc_variant_t set = purc_variant_make_set_by_ckey(2, "id", obj_1, obj_2);
    ASSERT_NE(set, PURC_VARIANT_INVALID);

    purc_variant_t v = pcvariant_set_find(set, obj_3);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    ASSERT_EQ(obj_2, v);

    bool overwrite = purc_variant_set_overwrite(set, obj_3, true);
    ASSERT_EQ(overwrite, true);

    v = pcvariant_set_find(set, obj_3);
    ASSERT_NE(v, PURC_VARIANT_INVALID);

    if (1) {
        bool ok = purc_variant_set_add(set, obj_3, true);
        ASSERT_TRUE(ok);
        purc_variant_t v;
        v = pcvariant_set_find(set, obj_3);
        ASSERT_NE(v, PURC_VARIANT_INVALID);
        ASSERT_EQ(v, obj_3);
        v = pcvariant_set_find(set, obj_2);
        ASSERT_NE(v, PURC_VARIANT_INVALID);
        ASSERT_EQ(v, obj_3);
    }


    purc_variant_unref(set);

    purc_variant_unref(obj_1);
    purc_variant_unref(obj_2);
    purc_variant_unref(obj_3);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

