#include "purc.h"
#include "private/avl.h"
#include "private/arraylist.h"
#include "private/hashtable.h"
#include "purc-variant.h"
#include "private/variant.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

TEST(variant_object, init_with_1_str)
{
    purc_instance_extra_info info = {0, 0};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    const char *s = "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar ";
    purc_variant_t str = purc_variant_make_string(s, false);
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 1);
    ASSERT_EQ(str->refc, 1);

    purc_variant_t obj = purc_variant_make_object_c(1, "foo", str);
    ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 1);
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 1);
    ASSERT_EQ(str->refc, 2);

    int to_demon_bug = 1;
    if (to_demon_bug) {
        purc_variant_ref(obj);
        ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 1);
        ASSERT_EQ(stat->nr_values[PVT(_STRING)], 1);
        ASSERT_EQ(str->refc, 3);
        purc_variant_unref(obj);
        ASSERT_EQ(str->refc, 2);
    }

    ASSERT_EQ(obj->refc, 1);
    purc_variant_unref(obj);
    ASSERT_EQ(str->refc, 1);
    purc_variant_unref(str);
    ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 0);
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 0);

#if 0
    // testing anonymous object
    obj = purc_variant_make_object(1,
            purc_variant_make_object(1,
                purc_variant_make_null()));
    ASSERT_NE(obj, nullptr);
    purc_variant_unref(obj);
#endif // 0
    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(variant_object, init_0_elem)
{
    purc_instance_extra_info info = {0, 0};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    purc_variant_t obj = purc_variant_make_object_c(0, NULL, NULL);
    ASSERT_NE(obj, nullptr);
    ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 1);
    ASSERT_EQ(obj->refc, 1);

    // purc_variant_ref(obj);
    // ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 1);
    // ASSERT_EQ(stat->nr_values[PVT(_STRING)], 1);
    // purc_variant_unref(obj);

    purc_variant_unref(obj);
    ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 0);

#if 0
    // testing anonymous object
    obj = purc_variant_make_object(1,
            purc_variant_make_object(1,
                purc_variant_make_null()));
    ASSERT_NE(obj, nullptr);
    purc_variant_unref(obj);
#endif // 0
    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(variant_object, add_1_str)
{
    purc_instance_extra_info info = {0, 0};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    const char *s = "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar ";
    purc_variant_t str = purc_variant_make_string(s, false);
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 1);
    ASSERT_EQ(str->refc, 1);

    purc_variant_t obj = purc_variant_make_object_c(0, NULL, NULL);
    ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 1);
    ASSERT_EQ(obj->refc, 1);
    ret = purc_variant_object_set_c(obj, "hello", str);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(obj->refc, 1);
    ASSERT_EQ(str->refc, 2);
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 1);

    // purc_variant_ref(obj);
    // ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 1);
    // ASSERT_EQ(stat->nr_values[PVT(_STRING)], 1);
    // purc_variant_unref(obj);

    purc_variant_unref(obj);
    ASSERT_EQ(str->refc, 1);
    purc_variant_unref(str);
    ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 0);
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 0);

#if 0
    // testing anonymous object
    obj = purc_variant_make_object(1,
            purc_variant_make_object(1,
                purc_variant_make_null()));
    ASSERT_NE(obj, nullptr);
    purc_variant_unref(obj);
#endif // 0
    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(variant_object, add_n_str)
{
    purc_instance_extra_info info = {0, 0};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    purc_variant_t obj = purc_variant_make_object_c(0, NULL, NULL);
    ASSERT_NE(obj, nullptr);
    ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 1);
    ASSERT_EQ(obj->refc, 1);

    int count = 1024;
    for (int j=0; j<count; ++j) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%d", j);
        purc_variant_t s = purc_variant_make_string(buf, false);
        ASSERT_NE(s, nullptr);
        int t = purc_variant_object_set_c(obj, buf, s);
        ASSERT_EQ(t, true);
        purc_variant_unref(s);
    }
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], count);
    ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 1);

    purc_variant_t val;
    int j = 0;
    foreach_value_in_variant_object(obj, val)
        ASSERT_EQ(val->type, PVT(_STRING));
        char buf[64];
        snprintf(buf, sizeof(buf), "%d", j++);
        ASSERT_EQ(strcmp(buf, purc_variant_get_string_const(val)), 0);
    end_foreach;

    ASSERT_EQ(obj->refc, 1);
    purc_variant_unref(obj);
    ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 0);
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 0);

#if 0
    // testing anonymous object
    obj = purc_variant_make_object(1,
            purc_variant_make_object(1,
                purc_variant_make_null()));
    ASSERT_NE(obj, nullptr);
    purc_variant_unref(obj);
#endif // 0
    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

