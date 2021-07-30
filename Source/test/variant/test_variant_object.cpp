#include "purc.h"
#include "purc-variant.h"
#include "private/variant.h"


#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

static inline void
_check_get_by_key_c(purc_variant_t obj, const char *key, purc_variant_t val,
    bool found)
{
    size_t refc = 0;
    if (val!=PURC_VARIANT_INVALID)
        refc = val->refc;
    purc_variant_t v = purc_variant_object_get_c(obj, key);
    ASSERT_EQ(v, val);
    if (found) {
        ASSERT_GT(refc, 0);
        ASSERT_EQ(val->refc, refc);
        purc_variant_ref(v);
        purc_variant_unref(v);
    } else {
        ASSERT_EQ(v, PURC_VARIANT_INVALID);
    }
}

static inline void
_check_get_by_key(purc_variant_t obj, purc_variant_t key, purc_variant_t val,
    bool found)
{
    size_t refc = 0;
    if (val!=PURC_VARIANT_INVALID)
        refc = val->refc;
    purc_variant_t v = purc_variant_object_get(obj, key);
    ASSERT_EQ(v, val);
    if (found) {
        ASSERT_GT(refc, 0);
        ASSERT_EQ(val->refc, refc);
        purc_variant_ref(v);
        purc_variant_unref(v);
    } else {
        ASSERT_EQ(v, PURC_VARIANT_INVALID);
    }
}

TEST(object, make_object_c)
{
    purc_instance_extra_info info = {0, 0};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    bool ok;
    const char     *k1 = "hello";
    purc_variant_t  v1 = purc_variant_make_string("world", false);
    const char     *k2 = "foo";
    purc_variant_t  v2 = purc_variant_make_string("bar", true);
    const char     *k3 = "damn";
    purc_variant_t  v3 = purc_variant_make_string("good", true);

    purc_variant_t obj;
    obj = purc_variant_make_object_c(0,
            NULL, PURC_VARIANT_INVALID);
    ASSERT_NE(obj, PURC_VARIANT_INVALID);
    ASSERT_EQ(obj->refc, 1);
    _check_get_by_key_c(obj, k1, PURC_VARIANT_INVALID, false);
    purc_variant_unref(obj);

    obj = purc_variant_make_object_c(1, k1, v1);
    ASSERT_NE(obj, PURC_VARIANT_INVALID);
    ASSERT_EQ(obj->refc, 1);
    ASSERT_EQ(v1->refc, 2);
    _check_get_by_key_c(obj, k1, v1, true);
    _check_get_by_key_c(obj, k2, PURC_VARIANT_INVALID, false);
    purc_variant_unref(obj);
    ASSERT_EQ(v1->refc, 1);

    obj = purc_variant_make_object_c(2, k1, v1, k2, v2);
    ASSERT_NE(obj, PURC_VARIANT_INVALID);
    ASSERT_EQ(obj->refc, 1);
    ASSERT_EQ(v1->refc, 2);
    ASSERT_EQ(v2->refc, 2);
    _check_get_by_key_c(obj, k1, v1, true);
    _check_get_by_key_c(obj, k2, v2, true);
    _check_get_by_key_c(obj, "hello_foo", PURC_VARIANT_INVALID, false);

    ok = purc_variant_object_set_c(obj, k1, v1);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(v1->refc, 2);

    ok = purc_variant_object_set_c(obj, k1, v2);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(v1->refc, 1);
    ASSERT_EQ(v2->refc, 3);

    ok = purc_variant_object_set_c(obj, k1, v1);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(v1->refc, 2);
    ASSERT_EQ(v2->refc, 2);

    ok = purc_variant_object_set_c(obj, k3, v3);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(v1->refc, 2);
    ASSERT_EQ(v2->refc, 2);
    ASSERT_EQ(v3->refc, 2);

    purc_variant_unref(obj);
    ASSERT_EQ(v1->refc, 1);
    ASSERT_EQ(v2->refc, 1);
    ASSERT_EQ(v3->refc, 1);

    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 3);


    purc_variant_unref(v1);
    purc_variant_unref(v2);
    purc_variant_unref(v3);

    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 0);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(object, make_object)
{
    purc_instance_extra_info info = {0, 0};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    bool ok;
    purc_variant_t  k1 = purc_variant_make_string("hello", false);
    purc_variant_t  v1 = purc_variant_make_string("world", false);
    purc_variant_t  k2 = purc_variant_make_string("foo", true);
    purc_variant_t  v2 = purc_variant_make_string("bar", true);
    purc_variant_t  k3 = purc_variant_make_string("damn", true);
    purc_variant_t  v3 = purc_variant_make_string("good", true);

    purc_variant_t obj;
    obj = purc_variant_make_object_c(0,
            NULL, PURC_VARIANT_INVALID);
    ASSERT_NE(obj, PURC_VARIANT_INVALID);
    ASSERT_EQ(obj->refc, 1);
    _check_get_by_key_c(obj, "hello", PURC_VARIANT_INVALID, false);
    purc_variant_unref(obj);

    obj = purc_variant_make_object(1, k1, v1);
    ASSERT_NE(obj, PURC_VARIANT_INVALID);
    ASSERT_EQ(obj->refc, 1);
    ASSERT_EQ(k1->refc, 1);
    ASSERT_EQ(v1->refc, 2);
    _check_get_by_key(obj, k1, v1, true);
    _check_get_by_key_c(obj, "foo", PURC_VARIANT_INVALID, false);
    _check_get_by_key_c(obj, "hello", v1, true);
    purc_variant_unref(obj);
    ASSERT_EQ(k1->refc, 1);
    ASSERT_EQ(v1->refc, 1);

    obj = purc_variant_make_object(2, k1, v1, k2, v2);
    ASSERT_NE(obj, PURC_VARIANT_INVALID);
    ASSERT_EQ(obj->refc, 1);
    ASSERT_EQ(k1->refc, 1);
    ASSERT_EQ(k2->refc, 1);
    ASSERT_EQ(v1->refc, 2);
    ASSERT_EQ(v2->refc, 2);
    _check_get_by_key(obj, k1, v1, true);
    _check_get_by_key(obj, k2, v2, true);
    _check_get_by_key_c(obj, "hello_foo", PURC_VARIANT_INVALID, false);
    _check_get_by_key_c(obj, "hello", v1, true);
    _check_get_by_key_c(obj, "foo", v2, true);

    ok = purc_variant_object_set(obj, k1, v1);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(v1->refc, 2);

    ok = purc_variant_object_set(obj, k1, v2);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(v1->refc, 1);
    ASSERT_EQ(v2->refc, 3);

    ok = purc_variant_object_set(obj, k1, v1);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(v1->refc, 2);
    ASSERT_EQ(v2->refc, 2);

    ok = purc_variant_object_set(obj, k3, v3);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(v1->refc, 2);
    ASSERT_EQ(v2->refc, 2);
    ASSERT_EQ(v3->refc, 2);

    purc_variant_unref(obj);
    ASSERT_EQ(k1->refc, 1);
    ASSERT_EQ(k2->refc, 1);
    ASSERT_EQ(k3->refc, 1);
    ASSERT_EQ(v1->refc, 1);
    ASSERT_EQ(v2->refc, 1);
    ASSERT_EQ(v3->refc, 1);

    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 6);

    purc_variant_unref(k1);
    purc_variant_unref(k2);
    purc_variant_unref(k3);
    purc_variant_unref(v1);
    purc_variant_unref(v2);
    purc_variant_unref(v3);

    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 0);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(object, unref)
{
    purc_instance_extra_info info = {0, 0};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    bool ok;
    size_t nr;
    const char     *k1 = "hello";
    purc_variant_t  v1 = purc_variant_make_string("world", false);
    const char     *k2 = "foo";
    purc_variant_t  v2 = purc_variant_make_string("bar", true);

    purc_variant_t obj;
    obj = purc_variant_make_object_c(1, k1, v1);
    ASSERT_NE(obj, PURC_VARIANT_INVALID);
    ASSERT_EQ(obj->refc, 1);
    purc_variant_unref(v1);
    ASSERT_EQ(v1->refc, 1);

    purc_variant_ref(obj);
    ASSERT_EQ(obj->refc, 2);
    ASSERT_EQ(v1->refc, 2);

    ok = purc_variant_object_set_c(obj, k2, v2);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(obj->refc, 2);
    purc_variant_unref(v2);
    ASSERT_EQ(v1->refc, 2);
    ASSERT_EQ(v2->refc, 1);

    nr = purc_variant_object_get_size(obj);
    ASSERT_EQ(nr, 2);

    purc_variant_unref(obj);
    ASSERT_EQ(obj->refc, 1);
    ASSERT_EQ(v1->refc, 1);

    nr = purc_variant_object_get_size(obj);
    ASSERT_EQ(nr, 1);

    purc_variant_unref(obj);

    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 0);
    ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 0);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}
