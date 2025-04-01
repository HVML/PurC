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

#include "purc/purc.h"
#include "purc/purc-variant.h"
#include "private/variant.h"
#include "private/ejson-parser.h"
#include "../helpers.h"


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
    purc_variant_t v = purc_variant_object_get_by_ckey(obj, key, true);
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
    purc_variant_t v = purc_variant_object_get(obj, key, true);
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
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    const struct purc_variant_stat *stat;

    ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "test_init", &info);
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
    obj = purc_variant_make_object_by_static_ckey(0,
            NULL, PURC_VARIANT_INVALID);
    ASSERT_NE(obj, PURC_VARIANT_INVALID);
    ASSERT_EQ(obj->refc, 1);
    _check_get_by_key_c(obj, k1, PURC_VARIANT_INVALID, false);

    pcvrnt_object_iterator* it;
    it = pcvrnt_object_iterator_create_begin(obj);
    int j = 0;
    while (it) {
        ++j;
        const char     *key = pcvrnt_object_iterator_get_ckey(it);
        purc_variant_t  val = pcvrnt_object_iterator_get_value(it);
        fprintf(stderr, "key%d:%s\n", j, key);
        fprintf(stderr, "val%d:%s\n", j, purc_variant_get_string_const(val));
        bool having = pcvrnt_object_iterator_next(it);
        // behavior of accessing `val`/`key` is un-defined
        if (!having) {
            // behavior of accessing `it` is un-defined
            break;
        }
    }
    pcvrnt_object_iterator_release(it);
    ASSERT_EQ(j, 0);

    purc_variant_unref(obj);

    obj = purc_variant_make_object_by_static_ckey(1, k1, v1);
    ASSERT_NE(obj, PURC_VARIANT_INVALID);
    ASSERT_EQ(obj->refc, 1);
    ASSERT_EQ(v1->refc, 2);
    _check_get_by_key_c(obj, k1, v1, true);
    _check_get_by_key_c(obj, k2, PURC_VARIANT_INVALID, false);
    it = pcvrnt_object_iterator_create_begin(obj);
    j = 0;
    while (it) {
        ++j;
        const char     *key = pcvrnt_object_iterator_get_ckey(it);
        purc_variant_t  val = pcvrnt_object_iterator_get_value(it);
        fprintf(stderr, "key%d:%s\n", j, key);
        fprintf(stderr, "val%d:%s\n", j, purc_variant_get_string_const(val));
        bool having = pcvrnt_object_iterator_next(it);
        // behavior of accessing `val`/`key` is un-defined
        if (!having) {
            // behavior of accessing `it` is un-defined
            break;
        }
    }
    pcvrnt_object_iterator_release(it);
    ASSERT_EQ(j, 1);
    purc_variant_unref(obj);
    ASSERT_EQ(v1->refc, 1);

    obj = purc_variant_make_object_by_static_ckey(2, k1, v1, k2, v2);
    ASSERT_NE(obj, PURC_VARIANT_INVALID);
    ASSERT_EQ(obj->refc, 1);
    ASSERT_EQ(v1->refc, 2);
    ASSERT_EQ(v2->refc, 2);
    _check_get_by_key_c(obj, k1, v1, true);
    _check_get_by_key_c(obj, k2, v2, true);
    _check_get_by_key_c(obj, "hello_foo", PURC_VARIANT_INVALID, false);
    it = pcvrnt_object_iterator_create_begin(obj);
    j = 0;
    while (it) {
        ++j;
        const char     *key = pcvrnt_object_iterator_get_ckey(it);
        purc_variant_t  val = pcvrnt_object_iterator_get_value(it);
        fprintf(stderr, "key%d:%s\n", j, key);
        fprintf(stderr, "val%d:%s\n", j, purc_variant_get_string_const(val));
        bool having = pcvrnt_object_iterator_next(it);
        // behavior of accessing `val`/`key` is un-defined
        if (!having) {
            // behavior of accessing `it` is un-defined
            break;
        }
    }
    pcvrnt_object_iterator_release(it);
    ASSERT_EQ(j, 2);

    ok = purc_variant_object_set_by_static_ckey(obj, k1, v1);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(v1->refc, 2);
    it = pcvrnt_object_iterator_create_begin(obj);
    j = 0;
    while (it) {
        ++j;
        const char     *key = pcvrnt_object_iterator_get_ckey(it);
        purc_variant_t  val = pcvrnt_object_iterator_get_value(it);
        fprintf(stderr, "key%d:%s\n", j, key);
        fprintf(stderr, "val%d:%s\n", j, purc_variant_get_string_const(val));
        bool having = pcvrnt_object_iterator_next(it);
        // behavior of accessing `val`/`key` is un-defined
        if (!having) {
            // behavior of accessing `it` is un-defined
            break;
        }
    }
    pcvrnt_object_iterator_release(it);
    ASSERT_EQ(j, 2);

    ok = purc_variant_object_set_by_static_ckey(obj, k1, v2);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(v1->refc, 1);
    ASSERT_EQ(v2->refc, 3);
    it = pcvrnt_object_iterator_create_begin(obj);
    j = 0;
    while (it) {
        ++j;
        const char     *key = pcvrnt_object_iterator_get_ckey(it);
        purc_variant_t  val = pcvrnt_object_iterator_get_value(it);
        fprintf(stderr, "key%d:%s\n", j, key);
        fprintf(stderr, "val%d:%s\n", j, purc_variant_get_string_const(val));
        bool having = pcvrnt_object_iterator_next(it);
        // behavior of accessing `val`/`key` is un-defined
        if (!having) {
            // behavior of accessing `it` is un-defined
            break;
        }
    }
    pcvrnt_object_iterator_release(it);
    ASSERT_EQ(j, 2);

    ok = purc_variant_object_set_by_static_ckey(obj, k1, v1);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(v1->refc, 2);
    ASSERT_EQ(v2->refc, 2);
    it = pcvrnt_object_iterator_create_begin(obj);
    j = 0;
    while (it) {
        ++j;
        const char     *key = pcvrnt_object_iterator_get_ckey(it);
        purc_variant_t  val = pcvrnt_object_iterator_get_value(it);
        fprintf(stderr, "key%d:%s\n", j, key);
        fprintf(stderr, "val%d:%s\n", j, purc_variant_get_string_const(val));
        bool having = pcvrnt_object_iterator_next(it);
        // behavior of accessing `val`/`key` is un-defined
        if (!having) {
            // behavior of accessing `it` is un-defined
            break;
        }
    }
    pcvrnt_object_iterator_release(it);
    ASSERT_EQ(j, 2);

    ok = purc_variant_object_set_by_static_ckey(obj, k3, v3);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(v1->refc, 2);
    ASSERT_EQ(v2->refc, 2);
    ASSERT_EQ(v3->refc, 2);
    it = pcvrnt_object_iterator_create_begin(obj);
    j = 0;
    while (it) {
        ++j;
        const char     *key = pcvrnt_object_iterator_get_ckey(it);
        purc_variant_t  val = pcvrnt_object_iterator_get_value(it);
        fprintf(stderr, "key%d:%s\n", j, key);
        fprintf(stderr, "val%d:%s\n", j, purc_variant_get_string_const(val));
        bool having = pcvrnt_object_iterator_next(it);
        // behavior of accessing `val`/`key` is un-defined
        if (!having) {
            // behavior of accessing `it` is un-defined
            break;
        }
    }
    pcvrnt_object_iterator_release(it);
    ASSERT_EQ(j, 3);

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
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    const struct purc_variant_stat *stat;

    ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "test_init", &info);
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
    obj = purc_variant_make_object_by_static_ckey(0,
            NULL, PURC_VARIANT_INVALID);
    ASSERT_NE(obj, PURC_VARIANT_INVALID);
    ASSERT_EQ(obj->refc, 1);
    _check_get_by_key_c(obj, "hello", PURC_VARIANT_INVALID, false);
    purc_variant_unref(obj);

    obj = purc_variant_make_object(1, k1, v1);
    ASSERT_NE(obj, PURC_VARIANT_INVALID);
    ASSERT_EQ(obj->refc, 1);
    /* uomap key */
    ASSERT_EQ(k1->refc, 2);
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
    ASSERT_EQ(k1->refc, 2);
    ASSERT_EQ(k2->refc, 2);
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
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    const struct purc_variant_stat *stat;

    ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "test_init", &info);
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
    obj = purc_variant_make_object_by_static_ckey(1, k1, v1);
    ASSERT_NE(obj, PURC_VARIANT_INVALID);
    ASSERT_EQ(obj->refc, 1);
    purc_variant_unref(v1);
    ASSERT_EQ(v1->refc, 1);

    purc_variant_ref(obj);
    ASSERT_EQ(obj->refc, 2);
    ASSERT_EQ(v1->refc, 1);

    ASSERT_EQ(v2->refc, 1);
    ok = purc_variant_object_set_by_static_ckey(obj, k2, v2);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(v2->refc, 2);
    ASSERT_EQ(obj->refc, 2);
    purc_variant_unref(v2);
    ASSERT_EQ(v1->refc, 1);
    ASSERT_EQ(v2->refc, 1);

    nr = purc_variant_object_get_size(obj);
    ASSERT_EQ(nr, 2);

    purc_variant_unref(obj);
    ASSERT_EQ(obj->refc, 1);
    ASSERT_EQ(v1->refc, 1);

    nr = purc_variant_object_get_size(obj);
    ASSERT_EQ(nr, 2);

    purc_variant_unref(obj);

    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 0);
    ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 0);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(object, compare)
{
    PurCInstance purc;

    int diff;
    const char *s;
    purc_variant_t obj1, obj2;

    s = "{first:xiaohong,last:xu}";
    obj1 = pcejson_parser_parse_string(s, 0, 0);
    if (obj1 == PURC_VARIANT_INVALID) {
        ADD_FAILURE() << "failed to parse: " << s << std::endl;
        return;
    }

    s = "{last:xu,first:xiaohong}";
    obj2 = pcejson_parser_parse_string(s, 0, 0);
    if (obj2 == PURC_VARIANT_INVALID) {
        purc_variant_unref(obj1);
        ADD_FAILURE() << "failed to parse: " << s << std::endl;
        return;
    }

    diff = purc_variant_compare_ex(obj1, obj2, PCVRNT_COMPARE_METHOD_AUTO);
    if (diff) {
        PRINT_VARIANT(obj1);
        PRINT_VARIANT(obj2);
        ADD_FAILURE() << "diff" << std::endl;
    }
    purc_variant_unref(obj1);
    purc_variant_unref(obj2);
}

