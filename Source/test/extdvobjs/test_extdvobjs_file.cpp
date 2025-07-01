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



#include "config.h"
#include "purc/purc.h"
#include "private/avl.h"
#include "private/hashtable.h"
#include "purc/purc-variant.h"
#include "private/variant.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/dvobjs.h"

#include "../helpers.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

extern purc_variant_t get_variant (char *buf, size_t *length);
extern void get_variant_total_info (size_t *mem, size_t *value, size_t *resv_ord, size_t *resv_out);
#define MAX_PARAM_NR    20


TEST(dvobjs, dvobjs_file_text_head)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    struct stat filestat;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_ord_before = 0;
    size_t nr_reserved_out_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_ord_after = 0;
    size_t nr_reserved_out_after = 0;
    size_t nr_return_line;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_ord_before, &nr_reserved_out_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t file = purc_variant_load_dvobj_from_so ("FS", "FILE");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(purc_variant_is_object (file), true);

    purc_variant_t text = purc_variant_object_get_by_ckey_ex(file, "txt", true);
    ASSERT_NE(text, nullptr);
    ASSERT_EQ(purc_variant_is_object (text), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey_ex (text, "head", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    stat("/etc/passwd", &filestat);
    ASSERT_GE(filestat.st_size, 0);

    printf ("TEST text_head: nr_args=2, param1=\"/etc/passwd\", param2=0:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (0);
    ret_var = func (NULL, 2, param, false);
    ASSERT_TRUE(purc_variant_array_size (ret_var, &nr_return_line));
    printf("\t\tReturn : %ld\n", nr_return_line);
    dump_string_array (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    printf ("TEST text_head: nr_args=2, param1=\"/etc/passwd\", param2=3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (3);
    ret_var = func (NULL, 2, param, false);
    ASSERT_TRUE(purc_variant_array_size (ret_var, &nr_return_line));
    printf("\t\tReturn : %ld\n", nr_return_line);
    dump_string_array (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    printf ("TEST text_head: nr_args=2, param1=\"/etc/passwd\", param2=-3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (-3);
    ret_var = func (NULL, 2, param, false);
    ASSERT_TRUE(purc_variant_array_size (ret_var, &nr_return_line));
    printf("\t\tReturn : %ld\n", nr_return_line);
    dump_string_array (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    purc_variant_unload_dvobj (file);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_ord_after, &nr_reserved_out_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after,
            sz_total_mem_before +
            (nr_reserved_ord_after - nr_reserved_ord_before) * sizeof(purc_variant_ord) +
            (nr_reserved_out_after - nr_reserved_out_before) * sizeof(purc_variant));

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_file_text_tail)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    struct stat filestat;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_ord_before = 0;
    size_t nr_reserved_out_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_ord_after = 0;
    size_t nr_reserved_out_after = 0;
    size_t nr_return_line;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_ord_before, &nr_reserved_out_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t file = purc_variant_load_dvobj_from_so ("FS", "FILE");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(purc_variant_is_object (file), true);

    purc_variant_t text = purc_variant_object_get_by_ckey_ex (file, "txt", true);
    ASSERT_NE(text, nullptr);
    ASSERT_EQ(purc_variant_is_object (text), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey_ex (text, "tail", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    stat("/etc/passwd", &filestat);
    ASSERT_GE(filestat.st_size, 0);

    printf ("TEST text_tail: nr_args=2, param1=\"/etc/passwd\", param2=0:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (0);
    ret_var = func (NULL, 2, param, false);
    ASSERT_TRUE(purc_variant_array_size (ret_var, &nr_return_line));
    printf("\t\tReturn : %ld\n", nr_return_line);
    dump_string_array (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    printf ("TEST text_tail: nr_args=2, param1=\"/etc/passwd\", param2=3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (3);
    ret_var = func (NULL, 2, param, false);
    ASSERT_TRUE(purc_variant_array_size (ret_var, &nr_return_line));
    printf("\t\tReturn : %ld\n", nr_return_line);
    dump_string_array (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    printf ("TEST text_tail: nr_args=2, param1=\"/etc/passwd\", param2=-3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (-3);
    ret_var = func (NULL, 2, param, false);
    ASSERT_TRUE(purc_variant_array_size (ret_var, &nr_return_line));
    printf("\t\tReturn : %ld\n", nr_return_line);
    dump_string_array (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    purc_variant_unload_dvobj (file);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_ord_after, &nr_reserved_out_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after,
            sz_total_mem_before +
            (nr_reserved_ord_after - nr_reserved_ord_before) * sizeof(purc_variant_ord) +
            (nr_reserved_out_after - nr_reserved_out_before) * sizeof(purc_variant));

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_file_bin_head)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    struct stat filestat;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_ord_before = 0;
    size_t nr_reserved_out_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_ord_after = 0;
    size_t nr_reserved_out_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_ord_before, &nr_reserved_out_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t file = purc_variant_load_dvobj_from_so ("FS", "FILE");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(purc_variant_is_object (file), true);

    purc_variant_t text = purc_variant_object_get_by_ckey_ex(file, "bin", true);
    ASSERT_NE(text, nullptr);
    ASSERT_EQ(purc_variant_is_object (text), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey_ex (text, "head", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    stat("/etc/passwd", &filestat);

    printf ("TEST bin_head: nr_args=2, param1=\"/etc/passwd\", param2=0:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (0);
    ret_var = func (NULL, 2, param, false);
    ASSERT_EQ(purc_variant_bsequence_length(ret_var), filestat.st_size);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    printf ("TEST bin_head: nr_args=2, param1=\"/etc/passwd\", param2=3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (3);
    ret_var = func (NULL, 2, param, false);
    ASSERT_EQ(purc_variant_bsequence_length(ret_var), 3);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    printf ("TEST bin_head: nr_args=2, param1=\"/etc/passwd\", param2=-3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (-3);
    ret_var = func (NULL, 2, param, false);
    ASSERT_EQ(purc_variant_bsequence_length(ret_var), filestat.st_size - 3);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    purc_variant_unload_dvobj (file);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_ord_after, &nr_reserved_out_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after,
            sz_total_mem_before +
            (nr_reserved_ord_after - nr_reserved_ord_before) * sizeof(purc_variant_ord) +
            (nr_reserved_out_after - nr_reserved_out_before) * sizeof(purc_variant));

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_file_bin_tail)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    struct stat filestat;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_ord_before = 0;
    size_t nr_reserved_out_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_ord_after = 0;
    size_t nr_reserved_out_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_ord_before, &nr_reserved_out_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t file = purc_variant_load_dvobj_from_so ("FS", "FILE");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(purc_variant_is_object (file), true);

    purc_variant_t text = purc_variant_object_get_by_ckey_ex (file, "bin", true);
    ASSERT_NE(text, nullptr);
    ASSERT_EQ(purc_variant_is_object (text), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey_ex (text, "tail", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    stat("/etc/passwd", &filestat);

    printf ("TEST bin_tail: nr_args=2, param1=\"/etc/passwd\", param2=0:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (0);
    ret_var = func (NULL, 2, param, false);
    ASSERT_EQ(purc_variant_bsequence_length(ret_var), filestat.st_size);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    printf ("TEST bin_tail: nr_args=2, param1=\"/etc/passwd\", param2=3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (3);
    ret_var = func (NULL, 2, param, false);
    ASSERT_EQ(purc_variant_bsequence_length(ret_var), 3);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    printf ("TEST bin_tail: nr_args=2, param1=\"/etc/passwd\", param2=-3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (-3);
    ret_var = func (NULL, 2, param, false);
    ASSERT_EQ(purc_variant_bsequence_length(ret_var), filestat.st_size - 3);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    purc_variant_unload_dvobj (file);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_ord_after, &nr_reserved_out_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after,
            sz_total_mem_before +
            (nr_reserved_ord_after - nr_reserved_ord_before) * sizeof(purc_variant_ord) +
            (nr_reserved_out_after - nr_reserved_out_before) * sizeof(purc_variant));

    purc_cleanup ();
}

