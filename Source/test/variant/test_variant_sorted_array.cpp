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
#include "private/debug.h"
#include "private/errors.h"
#include "private/variant.h"
#include "purc-variant.h"

#include "../helpers.h"

#include <gtest/gtest.h>

#if 0
static void*
ref(const void *v)
{
    return purc_variant_ref((purc_variant_t)v);
}
#endif

static void
unref(void *v)
{
    purc_variant_unref((purc_variant_t)v);
}

TEST(variant, sorted_array)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_sorted_array", false);

    purc_variant_t v = purc_variant_make_sorted_array(PCVRNT_SAFLAG_ASC, 2, nullptr);
    ASSERT_NE(v, nullptr);

    size_t sz = purc_variant_sorted_array_get_size(v);
    ASSERT_EQ(sz, 0);

    unref(v);
}


TEST(variant, sorted_array_with_atom)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_sorted_array", false);

    purc_variant_t v = purc_variant_make_sorted_array(PCVRNT_SAFLAG_ASC, 2, nullptr);
    ASSERT_NE(v, nullptr);

    purc_variant_t any = purc_variant_make_atom_string("ANY", false);
    ASSERT_NE(any, nullptr);

    size_t sz = purc_variant_sorted_array_get_size(v);
    ASSERT_EQ(sz, 0);

    ssize_t r = purc_variant_sorted_array_add(v, any);
    ASSERT_EQ(r, 0);

    sz = purc_variant_sorted_array_get_size(v);
    ASSERT_EQ(sz, 1);

    r = purc_variant_sorted_array_add(v, any);
    ASSERT_EQ(r, -1);

    bool rb = purc_variant_sorted_array_find(v, any);
    ASSERT_EQ(rb, true);

    rb = purc_variant_sorted_array_remove(v, any);
    ASSERT_EQ(rb, true);

    rb = purc_variant_sorted_array_delete(v, 1);
    ASSERT_EQ(rb, false);

    unref(any);
    unref(v);
}


TEST(variant, sorted_array_with_multi_atom)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_sorted_array", false);

    purc_variant_t v = purc_variant_make_sorted_array(PCVRNT_SAFLAG_ASC, 2, nullptr);
    ASSERT_NE(v, nullptr);

    purc_variant_t any = purc_variant_make_atom_string("ANY", false);
    ASSERT_NE(any, nullptr);

    purc_variant_t nosuchkey = purc_variant_make_atom_string("NoSuchKey", false);
    ASSERT_NE(nosuchkey, nullptr);

    size_t sz = purc_variant_sorted_array_get_size(v);
    ASSERT_EQ(sz, 0);

    ssize_t r = purc_variant_sorted_array_add(v, any);
    ASSERT_EQ(r, 0);

    sz = purc_variant_sorted_array_get_size(v);
    ASSERT_EQ(sz, 1);

    r = purc_variant_sorted_array_add(v, nosuchkey);
    ASSERT_EQ(r, 1);

    sz = purc_variant_sorted_array_get_size(v);
    ASSERT_EQ(sz, 2);

    r = purc_variant_sorted_array_add(v, any);
    ASSERT_EQ(r, -1);

    r = purc_variant_sorted_array_add(v, nosuchkey);
    ASSERT_EQ(r, -1);

    bool rb = purc_variant_sorted_array_find(v, any);
    ASSERT_EQ(rb, true);

    rb = purc_variant_sorted_array_find(v, nosuchkey);
    ASSERT_EQ(rb, true);

    rb = purc_variant_sorted_array_remove(v, any);
    ASSERT_EQ(rb, true);

    rb = purc_variant_sorted_array_find(v, any);
    ASSERT_EQ(rb, false);

    rb = purc_variant_sorted_array_find(v, nosuchkey);
    ASSERT_EQ(rb, true);

    rb = purc_variant_sorted_array_delete(v, 0);
    ASSERT_EQ(rb, true);

    rb = purc_variant_sorted_array_find(v, any);
    ASSERT_EQ(rb, false);

    rb = purc_variant_sorted_array_find(v, nosuchkey);
    ASSERT_EQ(rb, false);

    unref(nosuchkey);
    unref(any);
    unref(v);
}
