/*
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "purc.h"
#include "private/avl.h"
#include "private/debug.h"
#include "private/hashtable.h"
#include "purc-variant.h"
#include "private/variant.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

TEST(variant_array, init_with_1_str)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    const struct purc_variant_stat *stat;

    ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "test_init", &info);
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

TEST(variant_array, init_0_elem)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    const struct purc_variant_stat *stat;

    ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    purc_variant_t arr = purc_variant_make_array(0, NULL);
    ASSERT_NE(arr, nullptr);
    ASSERT_EQ(stat->nr_values[PVT(_ARRAY)], 1);
    ASSERT_EQ(arr->refc, 1);

    // purc_variant_ref(arr);
    // ASSERT_EQ(stat->nr_values[PVT(_ARRAY)], 1);
    // ASSERT_EQ(stat->nr_values[PVT(_STRING)], 1);
    // purc_variant_unref(arr);

    purc_variant_unref(arr);
    ASSERT_EQ(stat->nr_values[PVT(_ARRAY)], 0);

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

TEST(variant_array, add_1_str)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    const struct purc_variant_stat *stat;

    ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "test_init", &info);
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

TEST(variant_array, add_n_str)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    const struct purc_variant_stat *stat;

    ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    purc_variant_t arr = purc_variant_make_array(0, NULL);
    ASSERT_NE(arr, nullptr);
    ASSERT_EQ(stat->nr_values[PVT(_ARRAY)], 1);
    ASSERT_EQ(arr->refc, 1);

    int count = 100;
    for (int j=0; j<count; ++j) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%d", j);
        purc_variant_t s = purc_variant_make_string(buf, false);
        ASSERT_NE(s, nullptr);
        int t = purc_variant_array_append(arr, s);
        ASSERT_EQ(t, true);
        ASSERT_EQ(s->refc, 2);
        ASSERT_EQ(arr->refc, 1);
        purc_variant_unref(s);
    }
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], count);
    ASSERT_EQ(stat->nr_values[PVT(_ARRAY)], 1);

    purc_variant_t val;
    int j = 0;
    size_t idx;
    foreach_value_in_variant_array(arr, val, idx)
        (void)idx;
        ASSERT_EQ(val->type, PVT(_STRING));
        char buf[64];
        snprintf(buf, sizeof(buf), "%d", j++);
        ASSERT_EQ(strcmp(buf, purc_variant_get_string_const(val)), 0);
    end_foreach;

    ASSERT_EQ(arr->refc, 1);
    purc_variant_unref(arr);
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

TEST(variant_array, add_n_str_and_remove)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    const struct purc_variant_stat *stat;

    ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    purc_variant_t arr = purc_variant_make_array(0, NULL);
    ASSERT_NE(arr, nullptr);
    ASSERT_EQ(stat->nr_values[PVT(_ARRAY)], 1);
    ASSERT_EQ(arr->refc, 1);

    int count = 100;
    for (int j=0; j<count; ++j) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%d", j);
        purc_variant_t s = purc_variant_make_string(buf, false);
        ASSERT_NE(s, nullptr);
        int t = purc_variant_array_append(arr, s);
        ASSERT_EQ(t, true);
        ASSERT_EQ(s->refc, 2);
        ASSERT_EQ(arr->refc, 1);
        purc_variant_unref(s);
    }
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], count);
    ASSERT_EQ(stat->nr_values[PVT(_ARRAY)], 1);

    purc_variant_t val;
    int j = 0;
    size_t curr;
    foreach_value_in_variant_array_safe(arr, val, curr)
        ASSERT_EQ(val->type, PVT(_STRING));
        char buf[64];
        snprintf(buf, sizeof(buf), "%d", j++);
        ASSERT_EQ(strcmp(buf, purc_variant_get_string_const(val)), 0);
        bool ok = purc_variant_array_remove(arr, curr);
        ASSERT_TRUE(ok);
    end_foreach;

    size_t idx;
    foreach_value_in_variant_array(arr, val, idx)
        (void)idx;
        ASSERT_EQ(0, 1); // shall not reach here
    end_foreach;

    ASSERT_EQ(arr->refc, 1);
    purc_variant_unref(arr);
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

TEST(variant_array, add_n_str_and_remove_pub)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    const struct purc_variant_stat *stat;

    ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    purc_variant_t arr = purc_variant_make_array(0, NULL);
    ASSERT_NE(arr, nullptr);
    ASSERT_EQ(stat->nr_values[PVT(_ARRAY)], 1);
    ASSERT_EQ(arr->refc, 1);

    int count = 100;
    for (int j=0; j<count; ++j) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%d", j);
        purc_variant_t s = purc_variant_make_string(buf, false);
        ASSERT_NE(s, nullptr);
        int t = purc_variant_array_append(arr, s);
        ASSERT_EQ(t, true);
        ASSERT_EQ(s->refc, 2);
        ASSERT_EQ(arr->refc, 1);
        purc_variant_unref(s);
    }
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], count);
    ASSERT_EQ(stat->nr_values[PVT(_ARRAY)], 1);

    purc_variant_t val;
    ssize_t curr = 0;
    size_t j = 0;
    for (curr=0; curr<purc_variant_array_get_size(arr); ++curr) {
        val = purc_variant_array_get(arr, curr);
        ASSERT_NE(val, nullptr);
        ASSERT_EQ(val->type, PVT(_STRING));

        char buf[64];
        snprintf(buf, sizeof(buf), "%zd", j++);
        const char *p = purc_variant_get_string_const(val);
        ASSERT_NE(p, nullptr);
        ASSERT_EQ(strcmp(buf, p), 0);
        bool b = purc_variant_array_remove(arr, curr);
        ASSERT_EQ(b, true);
        --curr;
    }

    size_t n = purc_variant_array_get_size(arr);
    ASSERT_EQ(n, 0);

    ASSERT_EQ(arr->refc, 1);
    purc_variant_unref(arr);
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

TEST(variant_array, make_ref_add_unref_unref)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    const struct purc_variant_stat *stat;

    ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    purc_variant_t s1 = purc_variant_make_string("hello", false);
    purc_variant_t arr = purc_variant_make_array(1, s1);
    EXPECT_EQ(s1->refc, 2);
    purc_variant_unref(s1);
    EXPECT_EQ(s1->refc, 1);

    purc_variant_ref(arr);
    EXPECT_EQ(s1->refc, 1);
    s1 = purc_variant_make_string("world", false);
    EXPECT_EQ(s1->refc, 1);
    purc_variant_array_append(arr, s1);
    EXPECT_EQ(s1->refc, 2);
    purc_variant_unref(s1);
    EXPECT_EQ(s1->refc, 1);
    size_t n = purc_variant_array_get_size(arr);
    ASSERT_EQ(n, 2);
    purc_variant_unref(arr);
    n = purc_variant_array_get_size(arr);
    ASSERT_EQ(n, 2);


    ASSERT_EQ(arr->refc, 1);
    purc_variant_unref(arr);
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

static inline purc_variant_t
make_array(const int *vals, size_t nr)
{
    purc_variant_t arr = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (arr == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    bool ok = true;

    for (size_t i=0; i<nr; ++i) {
        purc_variant_t t;
        t = purc_variant_make_longint(vals[i]);
        if (t == PURC_VARIANT_INVALID) {
            ok = false;
            break;
        }
        ok = purc_variant_array_append(arr, t);
        purc_variant_unref(t);
        if (!ok)
            break;
    }

    if (!ok) {
        purc_variant_unref(arr);
        return PURC_VARIANT_INVALID;
    }

    return arr;
}

static inline int
cmp(purc_variant_t l, purc_variant_t r, void *ud)
{
    (void)ud;
    double dl = purc_variant_numberify(l);
    double dr = purc_variant_numberify(r);

    if (dl < dr)
        return -1;
    if (dl == dr)
        return 0;
    return 1;
}

TEST(variant_array, sort)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    const struct purc_variant_stat *stat;

    ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    const int ins[] = {
        3,2,4,1,7,9,6,8,5
    };
    const int outs[] = {
        1,2,3,4,5,6,7,8,9
    };

    char inbuf[8192]; {
        purc_variant_t arr = make_array(ins, PCA_TABLESIZE(ins));
        ASSERT_NE(arr, nullptr);

        int r = pcvariant_array_sort(arr, NULL, cmp);
        ASSERT_EQ(r, 0);

        r = purc_variant_stringify_buff(inbuf, sizeof(inbuf), arr);
        ASSERT_GT(r, 0);

        purc_variant_unref(arr);
    }

    char outbuf[8192]; {
        purc_variant_t arr = make_array(outs, PCA_TABLESIZE(outs));
        ASSERT_NE(arr, nullptr);

        int r = purc_variant_stringify_buff(outbuf, sizeof(outbuf), arr);
        ASSERT_GT(r, 0);

        purc_variant_unref(arr);
    }

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);

    ASSERT_STREQ(inbuf, outbuf);
}

