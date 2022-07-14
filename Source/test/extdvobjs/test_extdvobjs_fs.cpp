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

    char data_path[PATH_MAX+1];
    env = "DVOBJS_TEST_PATH";
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "test_files");
    std::cerr << "env: " << env << "=" << data_path << std::endl;

    char file_path[1024] = {0};
    strcpy (file_path, data_path);
    strcat (file_path, "/fs");

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

    char data_path[PATH_MAX+1];
    env = "DVOBJS_TEST_PATH";
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "test_files");
    std::cerr << "env: " << env << "=" << data_path << std::endl;


    char file_path[1024] = {0};
    strcpy (file_path, data_path);
    strcat (file_path, "/fs");

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

    char data_path[PATH_MAX+1];
    env = "DVOBJS_TEST_PATH";
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "test_files");
    std::cerr << "env: " << env << "=" << data_path << std::endl;

    char file_path[1024] = {0};
    strcpy (file_path, data_path);
    strcat (file_path, "/fs/test");

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

    char data_path[PATH_MAX+1];
    env = "DVOBJS_TEST_PATH";
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "test_files");
    std::cerr << "env: " << env << "=" << data_path << std::endl;

    char file_path[1024] = {0};
    strcpy (file_path, data_path);
    strcat (file_path, "/fs/test");

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

    char data_path[PATH_MAX+1];
    env = "DVOBJS_TEST_PATH";
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "test_files");
    std::cerr << "env: " << env << "=" << data_path << std::endl;

    char file_path[1024] = {0};
    strcpy (file_path, data_path);
    strcat (file_path, "/fs/test");

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

    char data_path[PATH_MAX+1];
    env = "DVOBJS_TEST_PATH";
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "test_files");
    std::cerr << "env: " << env << "=" << data_path << std::endl;

    char file_path[1024] = {0};
    strcpy (file_path, data_path);
    strcat (file_path, "/fs/streq.test");

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

    char data_path[PATH_MAX+1];
    env = "DVOBJS_TEST_PATH";
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "test_files");
    std::cerr << "env: " << env << "=" << data_path << std::endl;

    char file_path[1024] = {0};
    strcpy (file_path, data_path);
    strcat (file_path, "/fs/temp.and.test");

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
