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

static int
cmp(const void *l, const void *r)
{
    return pcvariant_diff((purc_variant_t)l, (purc_variant_t)r);
}

TEST(variant, map)
{
    PurCInstance purc;

    int r;
    purc_variant_t k, v;
    pcutils_map *map;
    pcutils_map_entry *entry;

    map = pcutils_map_create(ref, unref, ref, unref, cmp, false);

    k = purc_variant_make_string("foo", true);
    v = purc_variant_make_string("bar", true);
    r = pcutils_map_insert(map, k, v);
    ASSERT_EQ(r, 0);

    entry = pcutils_map_find(map, k);
    ASSERT_EQ(entry->val, (void*)v);

    PURC_VARIANT_SAFE_CLEAR(v);

    entry = pcutils_map_find(map, k);
    ASSERT_STREQ(purc_variant_get_string_const((purc_variant_t)entry->val),
            "bar");

    v = purc_variant_make_string("foo", true);
    r = pcutils_map_insert(map, k, v);
    ASSERT_NE(r, 0);

    PURC_VARIANT_SAFE_CLEAR(v);
    PURC_VARIANT_SAFE_CLEAR(k);

    pcutils_map_destroy(map);
}


