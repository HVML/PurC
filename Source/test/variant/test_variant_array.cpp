#include "purc.h"
#include "private/avl.h"
#include "private/arraylist.h"
#include "private/hashtable.h"
#include "purc-variant.h"
#include "private/variant.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

TEST(array, init_with_1_str)
{
    for (int i=0; i<10; ++i) {
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

        purc_variant_t arr = purc_variant_make_array(1, str);
        ASSERT_EQ(stat->nr_values[PVT(_ARRAY)], 1);
        ASSERT_EQ(stat->nr_values[PVT(_STRING)], 1);

        purc_variant_ref(arr);
        ASSERT_EQ(stat->nr_values[PVT(_ARRAY)], 1);
        ASSERT_EQ(stat->nr_values[PVT(_STRING)], 1);
        purc_variant_unref(arr);

        purc_variant_unref(arr);
        purc_variant_unref(str);
        ASSERT_EQ(stat->nr_values[PVT(_ARRAY)], 0);
        ASSERT_EQ(stat->nr_values[PVT(_STRING)], 0);

#if 0
        // testing anonymous object
        arr = purc_variant_make_array(1,
                purc_variant_make_array(1,
                    purc_variant_make_null()));
        ASSERT_NE(arr, nullptr);
        purc_variant_unref(arr);
#endif // 0
        cleanup = purc_cleanup ();
        ASSERT_EQ (cleanup, true);
    }
}

TEST(array, add_1_str)
{
    for (int i=0; i<10; ++i) {
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

        purc_variant_t arr = purc_variant_make_array(0, NULL);
        ASSERT_EQ(stat->nr_values[PVT(_ARRAY)], 1);
        ASSERT_EQ(arr->refc, 1);
        ret = purc_variant_array_append(arr, str);
        ASSERT_EQ(ret, true);
        ASSERT_EQ(arr->refc, 1);
        ASSERT_EQ(str->refc, 2);
        ASSERT_EQ(stat->nr_values[PVT(_STRING)], 1);

        // purc_variant_ref(arr);
        // ASSERT_EQ(stat->nr_values[PVT(_ARRAY)], 1);
        // ASSERT_EQ(stat->nr_values[PVT(_STRING)], 1);
        // purc_variant_unref(arr);

        purc_variant_unref(arr);
        ASSERT_EQ(str->refc, 1);
        purc_variant_unref(str);
        ASSERT_EQ(stat->nr_values[PVT(_ARRAY)], 0);
        ASSERT_EQ(stat->nr_values[PVT(_STRING)], 0);

#if 0
        // testing anonymous object
        arr = purc_variant_make_array(1,
                purc_variant_make_array(1,
                    purc_variant_make_null()));
        ASSERT_NE(arr, nullptr);
        purc_variant_unref(arr);
#endif // 0
        cleanup = purc_cleanup ();
        ASSERT_EQ (cleanup, true);
    }
}

