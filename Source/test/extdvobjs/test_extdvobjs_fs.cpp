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
#include "purc-variant.h"

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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "list");
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
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "name");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "dev");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "inode");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "type");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "mode_str");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "nlink");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "uid");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "gid");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "rdev_major");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "rdev_minor");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "size");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "blksize");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "blocks");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "atime");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "mtime");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "ctime");
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
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "name");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "dev");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "inode");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "type");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "mode_str");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "nlink");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "uid");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "gid");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "rdev_major");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "rdev_minor");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "size");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "blksize");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "blocks");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "atime");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "mtime");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "ctime");
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
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "name");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "dev");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "inode");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "type");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "mode_str");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "nlink");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "uid");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "gid");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "rdev_major");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "rdev_minor");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "size");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "blksize");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "blocks");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "atime");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "mtime");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "ctime");
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
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "name");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "dev");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "inode");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "type");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "mode_str");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "nlink");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "uid");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "gid");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "rdev_major");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "rdev_minor");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "size");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "blksize");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "blocks");
        printf ("\t%ld  ", (long int)(tmp_var->d));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "atime");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "mtime");
        printf ("\t%s  ", purc_variant_get_string_const (tmp_var));
        tmp_var = purc_variant_object_get_by_ckey (tmp_obj, "ctime");
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


    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "list_prt");
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
    param[1] = NULL;
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "basename");
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "chgrp");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char file_path[PATH_MAX + NAME_MAX +1];
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/fs/chgrp.test");
    std::cerr << "env: " << env << "=" << file_path << std::endl;


    printf ("TEST chgrp: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // String param
    printf ("TEST chgrp: nr_args = 2, param[0] = file_path, param[1] = 'root':\n");
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "chmod");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char file_path[PATH_MAX + NAME_MAX + 1];
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/fs/chmod.test");
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
    printf ("TEST chmod: nr_args = 2, param[0] = file_path, param[1] = '754':\n");
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_string ("0754", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    // Number string param (decimal)
    printf ("TEST chmod: nr_args = 2, param[0] = file_path, param[1] = '754':\n");
    param[0] = purc_variant_make_string (file_path, true);
    param[1] = purc_variant_make_string ("511", true);
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "chown");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char file_path[PATH_MAX + NAME_MAX + 1];
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/fs/chwon.test");
    std::cerr << "env: " << env << "=" << file_path << std::endl;


    printf ("TEST chown: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // String param
    printf ("TEST chown: nr_args = 2, param[0] = file_path, param[1] = 'root':\n");
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "copy");
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
    strncat (file_path_from, "/fs/copy_from.test", sizeof(file_path_from)-1);

    char file_path_to[PATH_MAX + NAME_MAX + 1] = {};
    strncpy (file_path_to, data_path, sizeof(file_path_to)-1);
    strncat (file_path_to, "/fs/copy_to.test", sizeof(file_path_to)-1);

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
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "dirname");
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "disk_usage");
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

    // Bizarre file
    printf ("TEST disk_usage: nr_args = 1, param[0] = '/proc/meminfo':\n");
    param[0] = purc_variant_make_string ("/proc/meminfo", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    printf ("TEST disk_usage: nr_args = 1, param[0] = '/sys/devices/system':\n");
    param[0] = purc_variant_make_string ("/sys/devices/cpu", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "file_exists");
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "file_is");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);


    printf ("TEST file_is: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // Normal file
    printf ("TEST file_is: nr_args = 2, param[0] = path, param[1] = 'file':\n");
    param[0] = purc_variant_make_string ("/etc/mime.types", true);
    param[1] = purc_variant_make_string ("file", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_TRUE(pcvariant_is_true(ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    // Dir
    printf ("TEST file_is: nr_args = 2, param[0] = path, param[1] = 'dir':\n");
    param[0] = purc_variant_make_string ("/etc", true);
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
    param[0] = purc_variant_make_string ("/etc/mime.types", true);
    param[1] = purc_variant_make_string ("exe", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_TRUE(pcvariant_is_false(ret_var));
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "lchgrp");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char file_path[PATH_MAX + NAME_MAX + 1];
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/fs/chgrp.test");
    std::cerr << "env: " << env << "=" << file_path << std::endl;


    printf ("TEST chgrp: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // String param
    printf ("TEST chgrp: nr_args = 2, param[0] = file_path, param[1] = 'root':\n");
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "lchown");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    char file_path[PATH_MAX + NAME_MAX + 1];
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/fs/chwon.test");
    std::cerr << "env: " << env << "=" << file_path << std::endl;


    printf ("TEST chown: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    // String param
    printf ("TEST chown: nr_args = 2, param[0] = file_path, param[1] = 'root':\n");
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "linkinfo");
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
    printf ("TEST chown: nr_args = 2, param[0] = '/bin/vi':\n");
    param[0] = purc_variant_make_string ("/bin/vi", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf ("return: %s\n", pcvariant_to_string (ret_var));
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "lstat");
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

    // Bizarre file
    printf ("TEST lstat: nr_args = 1, param[0] = '/proc/meminfo':\n");
    param[0] = purc_variant_make_string ("/proc/meminfo", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    printf ("TEST lstat: nr_args = 1, param[0] = '/sys/devices/system':\n");
    param[0] = purc_variant_make_string ("/sys/devices/cpu", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "link");
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
    strncat (file_path_origin, "/fs/link_origin.test", sizeof(file_path_origin)-1);

    char file_path_pointer[PATH_MAX + NAME_MAX + 1] = {};
    strncpy (file_path_pointer, data_path, sizeof(file_path_pointer)-1);
    strncat (file_path_pointer, "/fs/link_pointer.test", sizeof(file_path_pointer)-1);

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
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
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


    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "mkdir");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    env = "DVOBJS_TEST_PATH";
    char file_path[PATH_MAX + NAME_MAX +1];
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/fs/dir_mkdir");
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
}

// readlink
TEST(dvobjs, dvobjs_fs_readlink)
{
}

// realpath
TEST(dvobjs, dvobjs_fs_realpath)
{
}

// rename
TEST(dvobjs, dvobjs_fs_rename)
{
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "rmdir");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    env = "DVOBJS_TEST_PATH";
    char file_path[PATH_MAX + NAME_MAX + 1];
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/fs/dir_rmdir");
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "stat");
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
    printf ("TEST stat: nr_args = 1, param[0] = '/etc/hosts':\n");
    param[0] = purc_variant_make_string ("/etc/hosts", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    // Bizarre file
    printf ("TEST stat: nr_args = 1, param[0] = '/proc/meminfo':\n");
    param[0] = purc_variant_make_string ("/proc/meminfo", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(ret_var);

    printf ("TEST stat: nr_args = 1, param[0] = '/sys/devices/system':\n");
    param[0] = purc_variant_make_string ("/sys/devices/cpu", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    dump_object (ret_var);
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

// symlink
TEST(dvobjs, dvobjs_fs_symlink)
{
}

// tempname
TEST(dvobjs, dvobjs_fs_tempname)
{
}

// umask
TEST(dvobjs, dvobjs_fs_umask)
{
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "rm");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    env = "DVOBJS_TEST_PATH";
    char file_path[PATH_MAX + NAME_MAX + 1];
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/fs/rm.test");
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "unlink");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    env = "DVOBJS_TEST_PATH";
    char file_path[PATH_MAX + NAME_MAX + 1];
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/fs/unlink.test");
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

    if (access(file_path, F_OK | R_OK) == 0)  {
        printf ("\tRemove file error!\n");
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

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (fs, "touch");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    char file_path[PATH_MAX + NAME_MAX +1];
    env = "DVOBJS_TEST_PATH";
    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
        env, "test_files/fs/touch.test");
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
    sprintf (old, "%s", ctime(&file_stat.st_atime));
    printf ("TEST list: nr_args = 1, param[0] = path:\n");
    param[0] = purc_variant_make_string (file_path, true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
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
}

// open_dir
TEST(dvobjs, dvobjs_fs_open_dir)
{
}

// dir_read
TEST(dvobjs, dvobjs_fs_read)
{
}

// dir_rewind
TEST(dvobjs, dvobjs_fs_rewind)
{
}
