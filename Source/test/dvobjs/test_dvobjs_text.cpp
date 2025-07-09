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
#include "private/avl.h"
#include "private/hashtable.h"
#include "private/variant.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/dvobjs.h"
#include "private/vdom.h"

#include "../helpers.h"

#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <gtest/gtest.h>

extern purc_variant_t get_variant (char *buf, size_t *length);
extern void get_variant_total_info (size_t *mem, size_t *value, size_t *resv_ord, size_t *resv_out);
#define MAX_PARAM_NR    20

TEST(dvobjs, dvobjs_t_getter)
{
    purc_variant_t param[MAX_PARAM_NR] = {0};
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_scalar_before = 0;
    size_t nr_reserved_vector_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_scalar_after = 0;
    size_t nr_reserved_vector_after = 0;
    const char *s = NULL;

    // get and function
    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t t = purc_dvobj_text_new();
    ASSERT_NE(t, nullptr);
    ASSERT_EQ(purc_variant_is_object (t), true);

    purc_variant_t map = purc_variant_object_get_by_ckey_ex (t, "map", true);
    ASSERT_EQ(purc_variant_is_object (map), true);

    purc_variant_t val = purc_variant_make_string ("world", false);
    purc_variant_object_set_by_static_ckey (map, "hello", val);
    purc_variant_unref (val);

    val = purc_variant_make_string ("beijing", false);
    purc_variant_object_set_by_static_ckey (map, "city", val);
    purc_variant_unref (val);

    val = purc_variant_make_string ("china", false);
    purc_variant_object_set_by_static_ckey (map, "country", val);
    purc_variant_unref (val);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey_ex (t, "get", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method getter = NULL;
    getter = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(getter, nullptr);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_scalar_before, &nr_reserved_vector_before);

    param[0] = purc_variant_make_string ("world", false);
    ret_var = getter (t, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    s = purc_variant_get_string_const (ret_var);
    ASSERT_STREQ (s, "world");
    purc_variant_unref(ret_var);
    purc_variant_unref(param[0]);

    param[0] = purc_variant_make_string ("city", false);
    ret_var = getter (t, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    s = purc_variant_get_string_const (ret_var);
    ASSERT_STREQ (s, "beijing");
    purc_variant_unref(ret_var);
    purc_variant_unref(param[0]);

    param[0] = purc_variant_make_string ("country", false);
    ret_var = getter (t, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    s = purc_variant_get_string_const (ret_var);
    ASSERT_STREQ (s, "china");
    purc_variant_unref(ret_var);
    purc_variant_unref(param[0]);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_scalar_after, &nr_reserved_vector_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after,
            sz_total_mem_before +
            (nr_reserved_scalar_after - nr_reserved_scalar_before) * sizeof(purc_variant_scalar) +
            (nr_reserved_vector_after - nr_reserved_vector_before) * sizeof(purc_variant));

    purc_variant_unref(t);
    purc_cleanup ();
}
