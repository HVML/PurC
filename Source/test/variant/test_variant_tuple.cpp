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
#include "private/debug.h"
#include "private/errors.h"
#include "private/variant.h"

#include "../helpers.h"

#include <gtest/gtest.h>

static void*
ref(const void *v)
{
    return purc_variant_ref((purc_variant_t)v);
}

static void
unref(void *v)
{
    purc_variant_unref((purc_variant_t)v);
}

TEST(variant, tuple)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_tuple", false);

    purc_variant_t v = purc_variant_make_tuple(3, NULL);
    ASSERT_NE(v, nullptr);

    size_t sz = purc_variant_tuple_get_size(v);
    ASSERT_EQ(sz, 3);

    bool b = purc_variant_is_type(v, PURC_VARIANT_TYPE_TUPLE);
    ASSERT_EQ(b, true);

    b = purc_variant_is_tuple(v);
    ASSERT_EQ(b, true);

    purc_variant_t m = purc_variant_tuple_get(v, 0);
    ASSERT_NE(m, nullptr);

    b = purc_variant_is_null(m);
    ASSERT_EQ(b, true);

    b = purc_variant_is_undefined(m);
    ASSERT_EQ(b, false);

    unref(v);
}


TEST(variant, tuple_member)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_tuple", false);

    purc_variant_t tuple = purc_variant_make_tuple(3, NULL);
    ASSERT_NE(tuple, nullptr);

    purc_variant_t object = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    ASSERT_NE(object, nullptr);

    bool r = purc_variant_tuple_set(tuple, 0, object);
    ASSERT_EQ(r, true);

    purc_variant_t v = purc_variant_tuple_get(tuple, 0);
    ASSERT_EQ(v, object);

    purc_variant_t array = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    ASSERT_NE(array, nullptr);

    r = purc_variant_tuple_set(tuple, 1, array);
    ASSERT_EQ(r, true);

    v = purc_variant_tuple_get(tuple, 1);
    ASSERT_EQ(v, array);


    purc_variant_t st = purc_variant_make_set(0, NULL, PURC_VARIANT_INVALID);
    ASSERT_NE(st, nullptr);

    r = purc_variant_tuple_set(tuple, 2, st);
    ASSERT_EQ(r, true);

    v = purc_variant_tuple_get(tuple, 2);
    ASSERT_EQ(v, st);


    purc_variant_t s = purc_variant_make_string("test", false);
    ASSERT_NE(s, nullptr);

    r = purc_variant_tuple_set(tuple, 0, s);
    ASSERT_EQ(r, true);

    v = purc_variant_tuple_get(tuple, 0);
    ASSERT_EQ(v, s);
    v = purc_variant_tuple_get(tuple, 1);
    ASSERT_EQ(v, array);
    v = purc_variant_tuple_get(tuple, 2);
    ASSERT_EQ(v, st);

    r = purc_variant_tuple_set(tuple, 5, s);
    ASSERT_EQ(r, false);

    v = purc_variant_tuple_get(tuple, 0);
    ASSERT_EQ(v, s);
    v = purc_variant_tuple_get(tuple, 1);
    ASSERT_EQ(v, array);
    v = purc_variant_tuple_get(tuple, 2);
    ASSERT_EQ(v, st);

    r = purc_variant_tuple_set(tuple, 1, s);
    ASSERT_EQ(r, true);

    v = purc_variant_tuple_get(tuple, 0);
    ASSERT_EQ(v, s);
    v = purc_variant_tuple_get(tuple, 1);
    ASSERT_EQ(v, s);
    v = purc_variant_tuple_get(tuple, 2);
    ASSERT_EQ(v, st);

    unref(s);
    unref(array);
    unref(object);
    unref(tuple);
}

TEST(variant, tuple_as_member)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_tuple", false);

    purc_variant_t tuple = purc_variant_make_tuple(3, NULL);
    ASSERT_NE(tuple, nullptr);

    purc_variant_t object = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    ASSERT_NE(object, nullptr);
    bool r = purc_variant_object_set_by_static_ckey(object, "tuple", tuple);
    ASSERT_EQ(r, true);
    purc_variant_t v = purc_variant_object_get_by_ckey(object, "tuple");
    ASSERT_EQ(v, tuple);


    purc_variant_t array = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    ASSERT_NE(array, nullptr);
    r = purc_variant_array_append(array, tuple);
    ASSERT_EQ(r, true);
    v = purc_variant_array_get(array, 0);
    ASSERT_EQ(v, tuple);

    purc_variant_t st = purc_variant_make_set(0, NULL, PURC_VARIANT_INVALID);
    ASSERT_NE(st, nullptr);
    r = purc_variant_set_add(st, tuple, true);
    ASSERT_EQ(r, true);
    v = purc_variant_set_get_by_index(st, 0);
    ASSERT_EQ(v, tuple);


    unref(st);
    unref(array);
    unref(object);
    unref(tuple);
}
