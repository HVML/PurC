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
#include "TestExtDVObj.h"
#include "../helpers.h"

TEST(dvobjs, basic)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);

    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_unref(fs);
    purc_cleanup();
}

TEST(dvobjs, hee)
{
    TestExtDVObj tester;
    tester.run_testcases_in_file("fs");
}

#include "private/avl.h"
#include "private/hashtable.h"
#include "private/variant.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/dvobjs.h"

#include "../helpers.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

extern void get_variant_total_info (size_t *mem, size_t *value, size_t *resv);
#define MAX_PARAM_NR    20

// list
TEST(dvobjs, dvobjs_fs_list)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    purc_variant_t tmp_var = NULL;
    purc_variant_t tmp_obj = NULL;
    size_t i = 0;
    size_t size = 0;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    const char *env;
    env = "DVOBJS_SO_PATH";
    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "list", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    char file_path[PATH_MAX + NAME_MAX +1];
    env = "DVOBJS_TEST_PATH";
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/fs");
    std::cerr << "env: " << env << "=" << file_path << std::endl;


    printf ("TEST list: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST list: nr_args = 1, param[0] = NUMBER:\n");
    param[0] = purc_variant_make_number (1);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref(param[0]);

    printf ("TEST list: nr_args = 1, param[0] = wrong path:\n");
    param[0] = purc_variant_make_string ("/abcdefg/123", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref(param[0]);

    printf ("TEST list: nr_args = 1, param[0] = path:\n");
    param[0] = purc_variant_make_string (file_path, true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);

    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_ARRAY), true);
    size = purc_variant_array_get_size (ret_var);

    for (i = 0; i < size; i++)  {
        tmp_obj = purc_variant_array_get (ret_var, i);
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "name", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "dev", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "inode", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "type", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "mode_str", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "nlink", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "uid", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "gid", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "rdev_major", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "rdev_minor", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "size", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "blksize", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "blocks", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "atime", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "mtime", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "ctime", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));

        printf ("\n");
    }
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    printf ("TEST list: nr_args = 1, param[0] = path, param[1] = *.md:\n");
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_string ("*.md", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);

    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_ARRAY), true);
    size = purc_variant_array_get_size (ret_var);

    for (i = 0; i < size; i++)  {
        tmp_obj = purc_variant_array_get (ret_var, i);
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "name", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "dev", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "inode", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "type", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "mode_str", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "nlink", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "uid", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "gid", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "rdev_major", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "rdev_minor", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "size", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "blksize", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "blocks", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "atime", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "mtime", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "ctime", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));

        printf ("\n");
    }
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);


    printf ("TEST list: nr_args = 1, param[0] = path, param[1] = *.test:\n");
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_string ("*.test", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);

    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_ARRAY), true);
    size = purc_variant_array_get_size (ret_var);

    for (i = 0; i < size; i++)  {
        tmp_obj = purc_variant_array_get (ret_var, i);
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "name", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "dev", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "inode", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "type", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "mode_str", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "nlink", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "uid", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "gid", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "rdev_major", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "rdev_minor", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "size", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "blksize", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "blocks", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "atime", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "mtime", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "ctime", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));

        printf ("\n");
    }
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    printf ("TEST list: nr_args = 1, \
            param[0] = path, param[1] = *.test;*.md:\n");
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_string ("*.md;*.test", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);

    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_ARRAY), true);
    size = purc_variant_array_get_size (ret_var);

    for (i = 0; i < size; i++)  {
        tmp_obj = purc_variant_array_get (ret_var, i);
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "name", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "dev", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "inode", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "type", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "mode_str", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "nlink", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "uid", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "gid", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "rdev_major", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "rdev_minor", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "size", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "blksize", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "blocks", true);
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "atime", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "mtime", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "ctime", true);
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));

        printf ("\n");
    }
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// list_prt
TEST(dvobjs, dvobjs_fs_list_prt)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    purc_variant_t tmp_var = NULL;
    size_t i = 0;
    size_t size = 0;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    const char *env;
    env = "DVOBJS_SO_PATH";
    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);


    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "list_prt", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    char file_path[PATH_MAX + NAME_MAX +1];
    env = "DVOBJS_TEST_PATH";
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/fs");
    std::cerr << "env: " << env << "=" << file_path << std::endl;


    printf ("TEST list_prt: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST list_prt: nr_args = 1, param[0] = NUMBER:\n");
    param[0] = purc_variant_make_number (1);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref(param[0]);

    printf ("TEST list_prt: nr_args = 1, param[0] = wrong path:\n");
    param[0] = purc_variant_make_string ("/abcdefg/123", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref(param[0]);

    printf ("TEST list: nr_args = 1, param[0] = path:\n");
    param[0] = purc_variant_make_string (file_path, true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);

    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_ARRAY), true);
    size = purc_variant_array_get_size (ret_var);
    for (i = 0; i < size; i++)  {
        tmp_var = purc_variant_array_get (ret_var, i);
        printf ("\t%s\n", purc_variant_get_string_const (tmp_var));
    }
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    printf ("TEST list: nr_args = 1, \
            param[0] = path, param[1] = NULL, param[2] = name size:\n");
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_null();
    param[2] = purc_variant_make_string ("name size", true);
    ret_var = func (NULL, 3, param, false);
    ASSERT_NE(ret_var, nullptr);

    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_ARRAY), true);
    size = purc_variant_array_get_size (ret_var);
    for (i = 0; i < size; i++)  {
        tmp_var = purc_variant_array_get (ret_var, i);
        printf ("\t%s\n", purc_variant_get_string_const (tmp_var));
    }
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(param[2]);
    purc_variant_unref(ret_var);

    printf ("TEST list: nr_args = 1, \
            param[0] = path, param[1] = *.md, param[2] = name size mode:\n");
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_string ("*.md", true);
    param[2] = purc_variant_make_string ("name size mode", true);
    ret_var = func (NULL, 3, param, false);
    ASSERT_NE(ret_var, nullptr);

    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_ARRAY), true);
    size = purc_variant_array_get_size (ret_var);
    for (i = 0; i < size; i++)  {
        tmp_var = purc_variant_array_get (ret_var, i);
        printf ("\t%s\n", purc_variant_get_string_const (tmp_var));
    }
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(param[2]);
    purc_variant_unref(ret_var);

    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// basename
#if 0
TEST(dvobjs, dvobjs_fs_basename)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;
    const char *func_result = NULL;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "basename", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);


    printf ("TEST basenagme: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // 1) basename("/etc/sudoers.d", ".d")
    // 1) sudoers
    printf ("TEST basename: nr_args = 2, param[0] = '/etc/sudoers.d', param[1] = '.d':\n");
    param[0] = purc_variant_make_string ("/etc/sudoers.d", true);
    param[1] = purc_variant_make_string (".d", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    func_result = purc_variant_get_string_const(ret_var);
    ASSERT_NE(func_result, nullptr);
    ASSERT_STREQ (func_result, "sudoers");
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    // 2) basename("/etc/sudoers.d")
    // 2) sudoers.d
    printf ("TEST basename: nr_args = 1, param[0] = '/etc/sudoers.d':\n");
    param[0] = purc_variant_make_string ("/etc/sudoers.d", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    func_result = purc_variant_get_string_const(ret_var);
    ASSERT_NE(func_result, nullptr);
    ASSERT_STREQ (func_result, "sudoers.d");
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // 3) basename("/etc/passwd")
    // 3) passwd
    printf ("TEST basename: nr_args = 1, param[0] = '/etc/passwd':\n");
    param[0] = purc_variant_make_string ("/etc/passwd", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    func_result = purc_variant_get_string_const(ret_var);
    ASSERT_NE(func_result, nullptr);
    ASSERT_STREQ (func_result, "passwd");
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // 4) basename("/etc/")
    // 4) etc
    printf ("TEST basename: nr_args = 1, param[0] = '/etc/':\n");
    param[0] = purc_variant_make_string ("/etc/", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    func_result = purc_variant_get_string_const(ret_var);
    ASSERT_NE(func_result, nullptr);
    ASSERT_STREQ (func_result, "etc");
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // 5) basename(".")
    // 5) .
    printf ("TEST basename: nr_args = 1, param[0] = '.':\n");
    param[0] = purc_variant_make_string (".", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    func_result = purc_variant_get_string_const(ret_var);
    ASSERT_NE(func_result, nullptr);
    ASSERT_STREQ (func_result, ".");
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // 6) basename("/")
    // 6) 
    printf ("TEST basename: nr_args = 1, param[0] = '/':\n");
    param[0] = purc_variant_make_string ("/", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    func_result = purc_variant_get_string_const(ret_var);
    ASSERT_NE(func_result, nullptr);
    ASSERT_STREQ (func_result, "");
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}
#endif

// chgrp
TEST(dvobjs, dvobjs_fs_chgrp)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "chgrp", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char file_path[PATH_MAX + NAME_MAX +1];
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/chgrp.test");
    std::cerr << "env: " << env << "=" << file_path << std::endl;


    printf ("TEST chgrp: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // String param
    printf ("TEST chgrp: nr_args = 2, param[0] = %s, param[1] = 'root':\n", file_path);
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_string ("sys", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    // Number param
    printf ("TEST chgrp: nr_args = 2, param[0] = file_path, param[1] = 2:\n");
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_ulongint (1); // daemon
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);


    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// chmod
TEST(dvobjs, dvobjs_fs_chmod)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "chmod", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char file_path[PATH_MAX + NAME_MAX + 1];
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/chmod.test");
    std::cerr << "env: " << env << "=" << file_path << std::endl;


    printf ("TEST chmod: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // Code string param
    printf ("TEST chmod: nr_args = 2, param[0] = file_path, param[1] = 'u+x,g-w':\n");
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_string ("u+x,g-w", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    // Number string param (octal)
    printf ("TEST chmod: nr_args = 2, param[0] = file_path, param[1] = '0754':\n");
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_string ("0754", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    // Number string param (decimal)
    printf ("TEST chmod: nr_args = 2, param[0] = file_path, param[1] = '492':\n");
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_string ("492", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// chown
TEST(dvobjs, dvobjs_fs_chown)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "chown", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char file_path[PATH_MAX + NAME_MAX + 1];
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/chwon.test");
    std::cerr << "env: " << env << "=" << file_path << std::endl;


    printf ("TEST chown: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
/*
    // String param
    printf ("TEST chown: nr_args = 2, param[0] = file_path, param[1] = 'sys':\n");
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_string ("root", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    // Number param
    printf ("TEST chown: nr_args = 2, param[0] = file_path, param[1] = 2:\n");
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_ulongint (1); // daemon
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);
*/

    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// copy
TEST(dvobjs, dvobjs_fs_copy)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "copy", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char data_path[PATH_MAX+1];
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "test_files");
    std::cerr << "env: " << env << "=" << data_path << std::endl;

    char file_path_from[PATH_MAX + NAME_MAX + 1] = {};
    strncpy (file_path_from, data_path, sizeof(file_path_from)-1);
    strncat (file_path_from, "/copy_from.test", sizeof(file_path_from)-1);

    char file_path_to[PATH_MAX + NAME_MAX + 1] = {};
    strncpy (file_path_to, data_path, sizeof(file_path_to)-1);
    strncat (file_path_to, "/copy_to.test", sizeof(file_path_to)-1);

    FILE *fp = fopen (file_path_from, "wb");
    ASSERT_NE(fp, nullptr);
    char ch;
    for (ch = 0; ch < 100; ch++) {
        fwrite (&ch, 1, 1, fp);
    }
    fclose (fp);

    printf ("TEST copy: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // String param
    printf ("TEST copy: nr_args = 2, param[0] = file_path_from, param[1] = file_path_to:\n");
    param[0] = purc_variant_make_string (file_path_from, true);
    param[1] = purc_variant_make_string (file_path_to, true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_TRUE(pcvariant_is_true(ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    fp = fopen (file_path_to, "rb");
    ASSERT_NE(fp, nullptr);
    fseek (fp, 0, SEEK_SET);

    char i;
    size_t sz;
    for (i = 0; i < 100; i++) {
        sz = fread (&ch, 1, 1, fp);
        ASSERT_EQ (sz, 1);
        ASSERT_EQ (ch, i);
    }
    fclose (fp);

    // Clean up
    remove (file_path_from);
    remove (file_path_to);
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// dirname
#if 0
TEST(dvobjs, dvobjs_fs_dirname)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;
    const char *func_result = NULL;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "dirname", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);


    printf ("TEST basenagme: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // 1) dirname(".")
    // 1) .
    printf ("TEST basename: nr_args = 1, param[0] = '.':\n");
    param[0] = purc_variant_make_string (".", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    func_result = purc_variant_get_string_const(ret_var);
    ASSERT_NE(func_result, nullptr);
    ASSERT_STREQ (func_result, ".");
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // 2) dirname("/")
    // 2) /
    printf ("TEST basename: nr_args = 1, param[0] = '/':\n");
    param[0] = purc_variant_make_string ("/", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    func_result = purc_variant_get_string_const(ret_var);
    ASSERT_NE(func_result, nullptr);
    ASSERT_STREQ (func_result, "/");
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // 3) dirname("/etc/passwd")
    // 3) /etc
    printf ("TEST basename: nr_args = 1, param[0] = '/etc/passwd':\n");
    param[0] = purc_variant_make_string ("/etc/passwd", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    func_result = purc_variant_get_string_const(ret_var);
    ASSERT_NE(func_result, nullptr);
    ASSERT_STREQ (func_result, "/etc");
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // 4) dirname("/etc/")
    // 4) /
    printf ("TEST basename: nr_args = 1, param[0] = '/etc/':\n");
    param[0] = purc_variant_make_string ("/etc/", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    func_result = purc_variant_get_string_const(ret_var);
    ASSERT_NE(func_result, nullptr);
    ASSERT_STREQ (func_result, "/");
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // 5) dirname("../hello")
    // 5) ../
    printf ("TEST basename: nr_args = 1, param[0] = '../hello':\n");
    param[0] = purc_variant_make_string ("../hello", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    func_result = purc_variant_get_string_const(ret_var);
    ASSERT_NE(func_result, nullptr);
    ASSERT_STREQ (func_result, "../");
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // 6) dirname("/usr/local/lib", 2)
    // 6) /usr
    printf ("TEST basename: nr_args = 2, param[0] = '/usr/local/lib', param[1] = 2:\n");
    param[0] = purc_variant_make_string ("/usr/local/lib", true);
    param[1] = purc_variant_make_ulongint (2);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    func_result = purc_variant_get_string_const(ret_var);
    ASSERT_NE(func_result, nullptr);
    ASSERT_STREQ (func_result, "/usr");
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);


    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}
#endif

// disk_usage
TEST(dvobjs, dvobjs_fs_disk_usage)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "disk_usage", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char data_path[PATH_MAX+1];
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "");
    std::cerr << "env: " << env << "=" << data_path << std::endl;


    printf ("TEST disk_usage: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // Normal dir
    printf ("TEST disk_usage: nr_args = 1, param[0] = path:\n");
    param[0] = purc_variant_make_string (data_path, true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

#if OS(LINUX)
    // Bizarre file
    printf ("TEST disk_usage: nr_args = 1, param[0] = '/proc/meminfo':\n");
    param[0] = purc_variant_make_string ("/proc/meminfo", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    printf ("TEST disk_usage: nr_args = 1, param[0] = '/sys/devices/system':\n");
    param[0] = purc_variant_make_string ("/sys/devices/system", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);
#endif

    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// file_exists
TEST(dvobjs, dvobjs_fs_file_exists)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "file_exists", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char data_path[PATH_MAX + NAME_MAX +1];
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "test_files/file_exist.test");
    std::cerr << "env: " << env << "=" << data_path << std::endl;


    printf ("TEST file_exists: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // File exist
    FILE *fp = fopen (data_path, "wb");
    ASSERT_NE(fp, nullptr);
    char ch;
    for (ch = 0; ch < 100; ch++) {
        fwrite (&ch, 1, 1, fp);
    }
    fclose (fp);

    printf ("TEST file_exists: nr_args = 1, param[0] = path: (file exist, return true)\n");
    param[0] = purc_variant_make_string (data_path, true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_TRUE(pcvariant_is_true(ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // File not exist
    remove (data_path);

    printf ("TEST file_exists: nr_args = 1, param[0] = path: (file removed, return false)\n");
    param[0] = purc_variant_make_string (data_path, true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_TRUE(pcvariant_is_false(ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// file_is
TEST(dvobjs, dvobjs_fs_file_is)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "file_is", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);


    printf ("TEST file_is: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // regular file
    printf ("TEST file_is: nr_args = 2, param[0] = path, param[1] = 'regular':\n");
    param[0] = purc_variant_make_string ("/bin/ls", true);
    param[1] = purc_variant_make_string ("regular read exe", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_TRUE(pcvariant_is_true(ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    // Dir
    printf ("TEST file_is: nr_args = 2, param[0] = path, param[1] = 'dir':\n");
    param[0] = purc_variant_make_string ("/", true);
    param[1] = purc_variant_make_string ("dir", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_TRUE(pcvariant_is_true(ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    // Executable
    printf ("TEST file_is: nr_args = 2, param[0] = path, param[1] = 'exe':\n");
    param[0] = purc_variant_make_string ("/bin/ls", true);
    param[1] = purc_variant_make_string ("exe", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_TRUE(pcvariant_is_true(ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    // Un-executable
    printf ("TEST file_is: nr_args = 2, param[0] = un_exe, param[1] = 'exe':\n");
    param[0] = purc_variant_make_string ("/bin/ls", true);
    param[1] = purc_variant_make_string ("exe", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_TRUE(pcvariant_is_true(ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// lchgrp
TEST(dvobjs, dvobjs_fs_lchgrp)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "lchgrp", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char file_path[PATH_MAX + NAME_MAX + 1];
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/chgrp.test");
    std::cerr << "env: " << env << "=" << file_path << std::endl;


    printf ("TEST chgrp: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

/*
    // String param
    printf ("TEST chgrp: nr_args = 2, param[0] = file_path, param[1] = 'sys':\n");
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_string ("sys", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    // Number param
    printf ("TEST chgrp: nr_args = 2, param[0] = file_path, param[1] = 2:\n");
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_ulongint (1); // daemon
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);
*/

    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// lchown
TEST(dvobjs, dvobjs_fs_lchown)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "lchown", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char file_path[PATH_MAX + NAME_MAX + 1];
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/chwon.test");
    std::cerr << "env: " << env << "=" << file_path << std::endl;


    printf ("TEST chown: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

/*
    // String param
    printf ("TEST chown: nr_args = 2, param[0] = file_path, param[1] = 'sys':\n");
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_string ("sys", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    // Number param
    printf ("TEST chown: nr_args = 2, param[0] = file_path, param[1] = 2:\n");
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_ulongint (1); // daemon
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);
*/

    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// linkinfo
TEST(dvobjs, dvobjs_fs_linkinfo)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "linkinfo", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);


    printf ("TEST linkinfo: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // String param
    printf ("TEST chown: nr_args = 1, param[0] = '/bin/ls':\n");
    param[0] = purc_variant_make_string ("/bin/ls", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf ("return: %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);


    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// lstat
TEST(dvobjs, dvobjs_fs_lstat)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "lstat", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);


    printf ("TEST lstat: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // Normal file
    printf ("TEST lstat: nr_args = 1, param[0] = '/etc/hosts':\n");
    param[0] = purc_variant_make_string ("/etc/hosts", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

#if OS(LINUX)
    // Bizarre file
    printf ("TEST lstat: nr_args = 1, param[0] = '/proc/meminfo':\n");
    param[0] = purc_variant_make_string ("/proc/meminfo", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    printf ("TEST lstat: nr_args = 1, param[0] = '/sys/devices/system':\n");
    param[0] = purc_variant_make_string ("/sys/devices/system", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);
#endif

    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// link
TEST(dvobjs, dvobjs_fs_link)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "link", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char data_path[PATH_MAX+1];
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "test_files");
    std::cerr << "env: " << env << "=" << data_path << std::endl;

    char file_path_origin[PATH_MAX + NAME_MAX + 1] = {};
    strncpy (file_path_origin, data_path, sizeof(file_path_origin)-1);
    strncat (file_path_origin, "/link_origin.test", sizeof(file_path_origin)-1);

    char file_path_pointer[PATH_MAX + NAME_MAX + 1] = {};
    strncpy (file_path_pointer, data_path, sizeof(file_path_pointer)-1);
    strncat (file_path_pointer, "/link_pointer.test", sizeof(file_path_pointer)-1);

    FILE *fp = fopen (file_path_origin, "wb");
    const char content[] = "This is a test file: link origin file.";
    ASSERT_NE(fp, nullptr);
    fwrite (content, 1, sizeof(content), fp);
    fclose (fp);

    printf ("TEST link: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // String param
    printf ("TEST link: nr_args = 2, param[0] = file_path_origin, param[1] = file_path_pointer:\n");
    param[0] = purc_variant_make_string (file_path_origin, true);
    param[1] = purc_variant_make_string (file_path_pointer, true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_TRUE(pcvariant_is_true(ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    ASSERT_EQ (access(file_path_pointer, F_OK | R_OK), 0);

    // Clean up
    remove (file_path_origin);
    remove (file_path_pointer);
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// mkdir
TEST(dvobjs, dvobjs_fs_mkdir)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    const char *env;
    env = "DVOBJS_SO_PATH";
    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);


    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "mkdir", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    env = "DVOBJS_TEST_PATH";
    char file_path[PATH_MAX + NAME_MAX +1];
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/dir_mkdir");
    std::cerr << "env: " << env << "=" << file_path << std::endl;


    printf ("TEST list_prt: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST list_prt: nr_args = 1, param[0] = NUMBER:\n");
    param[0] = purc_variant_make_number (1);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref(param[0]);

    printf ("TEST list: nr_args = 1, param[0] = path:\n");
    param[0] = purc_variant_make_string (file_path, true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    if (access(file_path, F_OK | R_OK) != 0)  {
        printf ("\tCreate directory error!\n");
    }
    else  {
        rmdir (file_path);
    }

    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// pathinfo
TEST(dvobjs, dvobjs_fs_pathinfo)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "pathinfo", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);


    printf ("TEST pathinfo: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // Path with extension name
    printf ("TEST pathinfo: nr_args = 2, param[0] = path, param[1] = 'all':\n");
    param[0] = purc_variant_make_string ("/sys/devices/system.test", true);
    param[1] = purc_variant_make_string ("all", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    // Path without extension name
    printf ("TEST pathinfo: nr_args = 2, param[0] = path, param[1] = 'all':\n");
    param[0] = purc_variant_make_string ("/sys/devices/system", true);
    param[1] = purc_variant_make_string ("all", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// readlink
TEST(dvobjs, dvobjs_fs_readlink)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "readlink", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);


    printf ("TEST readlink: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // String param
    printf ("TEST readlink: nr_args = 1, param[0] = '/bin/vi':\n");
    param[0] = purc_variant_make_string ("/bin/vi", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf ("return: %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // Test not-link param
    printf ("TEST readlink: nr_args = 1, param[0] = '/etc/hosts' (should return false):\n");
    param[0] = purc_variant_make_string ("/etc/hosts", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_TRUE(pcvariant_is_false(ret_var));
    printf ("return: %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// realpath
TEST(dvobjs, dvobjs_fs_realpath)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "realpath", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char data_path[PATH_MAX+1];
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "test_files");
    std::cerr << "env: " << env << "=" << data_path << std::endl;

    char file_path_before[PATH_MAX + NAME_MAX + 1] = {};
    strncpy (file_path_before, data_path, sizeof(file_path_before)-1);
    strncat (file_path_before, "/../../../../README.md", sizeof(file_path_before)-1);

    printf ("TEST realpath: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // String param
    printf ("TEST realpath: nr_args = 1, param[0] = './../../../../README.md':\n");
    param[0] = purc_variant_make_string (data_path, true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_STRING), true);
    printf ("return: %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    printf ("TEST realpath: nr_args = 1, param[0] = '/tmp/':\n");
    param[0] = purc_variant_make_string ("/tmp/", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf ("return: %s\n", purc_variant_get_string_const (ret_var));
#if OS(LINUX)
    ASSERT_STREQ (purc_variant_get_string_const(ret_var), "/tmp");
#elif PLATFORM(MAC)
    ASSERT_STREQ (purc_variant_get_string_const(ret_var), "/private/tmp");
#endif
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// rename
TEST(dvobjs, dvobjs_fs_rename)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "rename", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char data_path[PATH_MAX+1];
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "test_files");
    std::cerr << "env: " << env << "=" << data_path << std::endl;

    char file_path_before[PATH_MAX + NAME_MAX + 1] = {};
    strncpy (file_path_before, data_path, sizeof(file_path_before)-1);
    strncat (file_path_before, "/link_origin.test", sizeof(file_path_before)-1);

    char file_path_after[PATH_MAX + NAME_MAX + 1] = {};
    strncpy (file_path_after, data_path, sizeof(file_path_after)-1);
    strncat (file_path_after, "/link_pointer.test", sizeof(file_path_after)-1);

    FILE *fp = fopen (file_path_before, "wb");
    const char content[] = "This is a test file.";
    ASSERT_NE(fp, nullptr);
    fwrite (content, 1, sizeof(content), fp);
    fclose (fp);

    printf ("TEST rename: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // String param
    printf ("TEST link: nr_args = 2, param[0] = file_path_before, param[1] = file_path_after:\n");
    param[0] = purc_variant_make_string (file_path_before, true);
    param[1] = purc_variant_make_string (file_path_after, true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_TRUE(pcvariant_is_true(ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    ASSERT_NE (access(file_path_before, F_OK | R_OK), 0);
    ASSERT_EQ (access(file_path_after,  F_OK | R_OK), 0);

    // Clean up
    remove (file_path_before);
    remove (file_path_after);
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// rmdir
TEST(dvobjs, dvobjs_fs_rmdir)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    const char *env;
    env = "DVOBJS_SO_PATH";
    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "rmdir", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    env = "DVOBJS_TEST_PATH";
    char file_path[PATH_MAX + NAME_MAX + 1];
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/dir_rmdir");
    std::cerr << "env: " << env << "=" << file_path << std::endl;


    printf ("TEST list_prt: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST list_prt: nr_args = 1, param[0] = NUMBER:\n");
    param[0] = purc_variant_make_number (1);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref(param[0]);

    mkdir (file_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    printf ("TEST list: nr_args = 1, param[0] = path:\n");
    param[0] = purc_variant_make_string (file_path, true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_TRUE(pcvariant_is_true(ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    if (access(file_path, F_OK | R_OK) == 0)  {
        printf ("\tRemove directory error!\n");
    }
    else  {
        rmdir (file_path);
    }

    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// stat
TEST(dvobjs, dvobjs_fs_stat)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "stat", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);


    printf ("TEST stat: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // Normal file
    printf ("TEST stat: nr_args = 1, param[0] = '/etc/hosts', param[1] = 'all':\n");
    param[0] = purc_variant_make_string ("/etc/hosts", true);
    param[1] = purc_variant_make_string ("all", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

#if OS(LINUX)
    // Bizarre file
    printf ("TEST stat: nr_args = 1, param[0] = '/proc/meminfo':\n");
    param[0] = purc_variant_make_string ("/proc/meminfo", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    printf ("TEST stat: nr_args = 1, param[0] = '/sys/devices/system':\n");
    param[0] = purc_variant_make_string ("/sys/devices/system", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);
#endif

    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// symlink
TEST(dvobjs, dvobjs_fs_symlink)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "symlink", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char data_path[PATH_MAX+1];
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "test_files");
    std::cerr << "env: " << env << "=" << data_path << std::endl;

    char file_path_origin[PATH_MAX + NAME_MAX + 1] = {};
    strncpy (file_path_origin, data_path, sizeof(file_path_origin)-1);
    strncat (file_path_origin, "/symlink_origin.test", sizeof(file_path_origin)-1);

    char file_path_pointer[PATH_MAX + NAME_MAX + 1] = {};
    strncpy (file_path_pointer, data_path, sizeof(file_path_pointer)-1);
    strncat (file_path_pointer, "/symlink_pointer.test", sizeof(file_path_pointer)-1);

    FILE *fp = fopen (file_path_origin, "wb");
    const char content[] = "This is a test file: symlink origin file.";
    ASSERT_NE(fp, nullptr);
    fwrite (content, 1, sizeof(content), fp);
    fclose (fp);

    printf ("TEST symlink: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // String param
    printf ("TEST symlink: nr_args = 2, param[0] = file_path_origin, param[1] = file_path_pointer:\n");
    param[0] = purc_variant_make_string (file_path_origin, true);
    param[1] = purc_variant_make_string (file_path_pointer, true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_TRUE(pcvariant_is_true(ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    ASSERT_EQ (access(file_path_pointer, F_OK | R_OK), 0);

    // Clean up
    remove (file_path_origin);
    remove (file_path_pointer);
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// tempname
TEST(dvobjs, dvobjs_fs_tempname)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;
    const char *temp_filename = NULL;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "tempname", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char data_path[PATH_MAX+1];
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "");
    std::cerr << "env: " << env << "=" << data_path << std::endl;


    printf ("TEST tempname: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // String param
    printf ("TEST tempname: nr_args = 1, param[0] = path:\n");
    param[0] = purc_variant_make_string (data_path, true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    temp_filename = purc_variant_get_string_const (ret_var);
    printf ("return: %s\n", temp_filename);
    remove (temp_filename);
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // With prefix
    printf ("TEST tempname: nr_args = 1, param[0] = path, param[1] = prefix:\n");
    param[0] = purc_variant_make_string (data_path, true);
    param[1] = purc_variant_make_string ("test_prefix_hello_world_", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    temp_filename = purc_variant_get_string_const (ret_var);
    printf ("return: %s\n", temp_filename);
    remove (temp_filename);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// umask
TEST(dvobjs, dvobjs_fs_umask)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "umask", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    // Without param
    printf ("TEST umask: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf ("return: %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref(ret_var);

    // String param
    printf ("TEST umask: nr_args = 1, param[0] = mask:\n");
    param[0] = purc_variant_make_string ("0755", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf ("return: %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// rm
TEST(dvobjs, dvobjs_fs_rm)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    const char *env;
    env = "DVOBJS_SO_PATH";
    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "rm", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    env = "DVOBJS_TEST_PATH";
    char file_path[PATH_MAX + NAME_MAX + 1];
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/rm.test");
    std::cerr << "env: " << env << "=" << file_path << std::endl;


    printf ("TEST list_prt: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST list_prt: nr_args = 1, param[0] = NUMBER:\n");
    param[0] = purc_variant_make_number (1);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref(param[0]);

    mkdir (file_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    printf ("TEST list: nr_args = 1, param[0] = path:\n");
    param[0] = purc_variant_make_string (file_path, true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    if (access(file_path, F_OK | R_OK) == 0)  {
        printf ("\tRemove directory error!\n");
    }
    else  {
        rmdir (file_path);
    }

    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// unlink
TEST(dvobjs, dvobjs_fs_unlink)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    const char *env;
    env = "DVOBJS_SO_PATH";
    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "unlink", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    env = "DVOBJS_TEST_PATH";
    char link_target[PATH_MAX + NAME_MAX + 1];
    char unlink_path[PATH_MAX + NAME_MAX + 1];
    test_getpath_from_env_or_rel(link_target, sizeof(link_target),
        env, "test_files/link_target.test");
    test_getpath_from_env_or_rel(unlink_path, sizeof(unlink_path),
        env, "test_files/unlink.test");
    std::cerr << "env: " << env << "=" << unlink_path << std::endl;

    ASSERT_EQ (symlink(link_target, unlink_path), 0);

    printf ("TEST list_prt: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST list_prt: nr_args = 1, param[0] = NUMBER:\n");
    param[0] = purc_variant_make_number (1);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref(param[0]);


    printf ("TEST list: nr_args = 1, param[0] = path:\n");
    param[0] = purc_variant_make_string (unlink_path, true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    if (access(unlink_path, F_OK | R_OK) == 0)  {
        printf ("\tUnlink error!\n");
    }

    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// touch
TEST(dvobjs, dvobjs_fs_touch)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    struct stat file_stat;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    const char *env;
    env = "DVOBJS_SO_PATH";
    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "touch", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    char file_path[PATH_MAX + NAME_MAX +1];
    env = "DVOBJS_TEST_PATH";
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/touch.test");
    std::cerr << "env: " << env << "=" << file_path << std::endl;


    printf ("TEST list_prt: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST list_prt: nr_args = 1, param[0] = NUMBER:\n");
    param[0] = purc_variant_make_number (1);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref(param[0]);

    memset(&file_stat, 0, sizeof(file_stat));
    stat(file_path, &file_stat);
    char old[128];
    strcpy(old, ctime(&file_stat.st_atime));
    printf ("TEST list: nr_args = 1, param[0] = path:\n");
    param[0] = purc_variant_make_string (file_path, true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_TRUE(pcvariant_is_true(ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    stat(file_path, &file_stat);
    char * newtime = ctime(&file_stat.st_atime);

    ASSERT_STRNE (old, newtime);

    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    unlink (file_path);

    purc_cleanup ();
}

// file_contents
TEST(dvobjs, dvobjs_fs_file_contents)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "file_contents", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char data_path[PATH_MAX+1];
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "test_files/file_contents.test");
    std::cerr << "env: " << env << "=" << data_path << std::endl;

    FILE *fp = fopen (data_path, "wb");
    ASSERT_NE(fp, nullptr);
    char ch;
    int i;
    for (i = 0; i < 100; i++) {
        ch = '0' + i % 10;
        fwrite (&ch, 1, 1, fp);
    }
    fclose (fp);


    printf ("TEST file_contents: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // param: filename, flag, offset, length
    printf ("TEST file_contents: nr_args = 2, param[0] = path, param[1] = flag, \
param[2] = offset, param[3] = length:\n");
    param[0] = purc_variant_make_string (data_path, false);
    param[1] = purc_variant_make_string ("string", false);
    param[2] = purc_variant_make_longint (25);
    param[3] = purc_variant_make_ulongint (20);
    ret_var = func (NULL, 4, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf ("return: %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(param[2]);
    purc_variant_unref(param[3]);
    purc_variant_unref(ret_var);

    // File that does not exist
    printf ("TEST file_contents: nr_args = 2, param[0] = NOT_EXIST_FILE, \
param[1] = flag, param[2] = offset, param[3] = length:\n");
    param[0] = purc_variant_make_string ("/abcdefg/123", false);
    param[1] = purc_variant_make_string ("string", false);
    param[2] = purc_variant_make_longint (25);
    param[3] = purc_variant_make_ulongint (20);
    ret_var = func (NULL, 4, param, false);
    ASSERT_EQ(ret_var, nullptr);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(param[2]);
    purc_variant_unref(param[3]);

    // Clean up
    remove (data_path);
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

// open_dir
TEST(dvobjs, dvobjs_fs_open_dir)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t dynamic_opendir = NULL;
    purc_variant_t dynamic_closedir = NULL;
    purc_dvariant_method func_opendir = NULL;
    purc_dvariant_method func_closedir = NULL;
    purc_variant_t dir_object = NULL;
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t fs = purc_variant_load_dvobj_from_so (NULL, "FS");
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(purc_variant_is_object (fs), true);

    dynamic_opendir = purc_variant_object_get_by_ckey (fs, "opendir", true);
    ASSERT_NE(dynamic_opendir, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic_opendir), true);

    dynamic_closedir = purc_variant_object_get_by_ckey (fs, "closedir", true);
    ASSERT_NE(dynamic_closedir, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic_closedir), true);

    func_opendir = purc_variant_dynamic_get_getter (dynamic_opendir);
    ASSERT_NE(func_opendir, nullptr);

    func_closedir = purc_variant_dynamic_get_getter (dynamic_closedir);
    ASSERT_NE(func_closedir, nullptr);

    printf ("TEST opendir: nr_args = 0, param = NULL:\n");
    ret_var = func_opendir (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST closedir: nr_args = 0, param = NULL:\n");
    ret_var = func_closedir (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");


    // opendir param: dir_path
    printf ("TEST opendir: nr_args = 1, param[0] = dir_path:\n");
    param[0] = purc_variant_make_string ("/", false);
    dir_object = func_opendir (NULL, 1, param, false);
    ASSERT_NE(dir_object, nullptr);
    purc_variant_unref(param[0]);

    struct purc_native_ops *ops = purc_variant_native_get_ops(dir_object);
    ASSERT_NE(ops, nullptr);
    void *native = purc_variant_native_get_entity(dir_object);
    ASSERT_NE(native, nullptr);

    purc_nvariant_method func_dir_read = ops->property_getter(native, "read");
    ASSERT_NE(func_dir_read, nullptr);
    purc_nvariant_method func_dir_rewind = ops->property_getter(native, "rewind");
    ASSERT_NE(func_dir_rewind, nullptr);

    // read dir
    printf ("TEST dir_read:\n");
    int i;
    for (i = 0; i < 5; i++) {
        ret_var = func_dir_read (native, "read", 0, param, 0);
        ASSERT_NE(ret_var, nullptr);
        printf ("dir_read: %s\n", purc_variant_get_string_const (ret_var));
        purc_variant_unref(ret_var);
    }

    // rewind dir
    printf ("TEST dir_rewind:\n");
    ret_var = func_dir_rewind (native, "rewind", 0, param, 0);
    ASSERT_NE(ret_var, nullptr);
    char *s = pcvariant_to_string(ret_var);
    printf ("dir_rewind return: %s\n", s);
    free(s);
    ASSERT_TRUE(pcvariant_is_true(ret_var));
    purc_variant_unref(ret_var);

    // read dir
    printf ("TEST dir_read:\n");
    for (i = 0; i < 3; i++) {
        ret_var = func_dir_read (native, "read", 0, param, 0);
        ASSERT_NE(dir_object, nullptr);
        printf ("dir_read: %s\n", purc_variant_get_string_const (ret_var));
        purc_variant_unref(ret_var);
    }

    // closedir param: dir_object
    printf ("TEST closedir: nr_args = 1, param[0] = dir_object:\n");
    param[0] = dir_object;
    ret_var = func_closedir (NULL, 1, param, false);
    ASSERT_TRUE(pcvariant_is_true(ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);


    // Clean up
    purc_variant_unload_dvobj (fs);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}
