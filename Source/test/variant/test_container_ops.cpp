#include "purc.h"
#include "purc-variant.h"
#include "private/variant.h"


#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>


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

