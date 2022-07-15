#include "config.h"
#include "purc.h"
#include "private/avl.h"
#include "private/hashtable.h"
#include "purc-variant.h"
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
extern void get_variant_total_info (size_t *mem, size_t *value, size_t *resv);
#define MAX_PARAM_NR    20

TEST(dvobjs, dvobjs_file_text_head)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    struct stat filestat;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;
    size_t nr_return_line;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t file = purc_variant_load_dvobj_from_so ("FS", "FILE");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(purc_variant_is_object (file), true);

    purc_variant_t text = purc_variant_object_get_by_ckey(file, "text");
    ASSERT_NE(text, nullptr);
    ASSERT_EQ(purc_variant_is_object (text), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (text, "head");
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
    //printf("\t\tReturn : %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    printf ("TEST text_head: nr_args=2, param1=\"/etc/passwd\", param2=3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (3);
    ret_var = func (NULL, 2, param, false);
    ASSERT_TRUE(purc_variant_array_size (ret_var, &nr_return_line));
    printf("\t\tReturn : %ld\n", nr_return_line);
    //printf("\t\tReturn : %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    printf ("TEST text_head: nr_args=2, param1=\"/etc/passwd\", param2=-3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (-3);
    ret_var = func (NULL, 2, param, false);
    ASSERT_TRUE(purc_variant_array_size (ret_var, &nr_return_line));
    printf("\t\tReturn : %ld\n", nr_return_line);
    //printf("\t\tReturn : %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    purc_variant_unload_dvobj (file);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_file_text_tail)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    struct stat filestat;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;
    size_t nr_return_line;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    purc_variant_t file = purc_variant_load_dvobj_from_so ("FS", "FILE");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(purc_variant_is_object (file), true);

    purc_variant_t text = purc_variant_object_get_by_ckey (file, "text");
    ASSERT_NE(text, nullptr);
    ASSERT_EQ(purc_variant_is_object (text), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (text, "tail");
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
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    printf ("TEST text_tail: nr_args=2, param1=\"/etc/passwd\", param2=3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (3);
    ret_var = func (NULL, 2, param, false);
    ASSERT_TRUE(purc_variant_array_size (ret_var, &nr_return_line));
    printf("\t\tReturn : %ld\n", nr_return_line);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    printf ("TEST text_tail: nr_args=2, param1=\"/etc/passwd\", param2=-3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (-3);
    ret_var = func (NULL, 2, param, false);
    ASSERT_TRUE(purc_variant_array_size (ret_var, &nr_return_line));
    printf("\t\tReturn : %ld\n", nr_return_line);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);
    purc_variant_unref(ret_var);

    purc_variant_unload_dvobj (file);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_file_bin_head)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    struct stat filestat;
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
    purc_variant_t file = purc_variant_load_dvobj_from_so ("FS", "FILE");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(purc_variant_is_object (file), true);

    purc_variant_t text = purc_variant_object_get_by_ckey(file, "bin");
    ASSERT_NE(text, nullptr);
    ASSERT_EQ(purc_variant_is_object (text), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (text, "head");
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
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_file_bin_tail)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    struct stat filestat;
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
    purc_variant_t file = purc_variant_load_dvobj_from_so ("FS", "FILE");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(purc_variant_is_object (file), true);

    purc_variant_t text = purc_variant_object_get_by_ckey (file, "bin");
    ASSERT_NE(text, nullptr);
    ASSERT_EQ(purc_variant_is_object (text), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (text, "tail");
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
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_file_stream_open_seek_close)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    purc_variant_t val = NULL;
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
    purc_variant_t file = purc_variant_load_dvobj_from_so ("FS", "FILE");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(purc_variant_is_object (file), true);

    purc_variant_t stream = purc_variant_object_get_by_ckey (file, "stream");
    ASSERT_NE(stream, nullptr);
    ASSERT_EQ(purc_variant_is_object (stream), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (stream, "open");
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
    strcat (file_path, "/stream.test");

    printf ("TEST stream_open_seek_close: nr_args=2, \
            param1=\"test_files/stream.test\":\n");
    param[0] = purc_variant_make_string (file_path, false);
    param[1] = purc_variant_make_string ("r+", false);
    ret_var = func (NULL, 2, param, false);

    ASSERT_EQ(purc_variant_is_type (ret_var,
                    PURC_VARIANT_TYPE_NATIVE), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);

    // seek
    dynamic = purc_variant_object_get_by_ckey (stream, "seek");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    param[1] = purc_variant_make_ulongint (17);
    param[2] = purc_variant_make_longint (SEEK_CUR);
    val = func (NULL, 3, param, false);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_LONGINT), true);
    int64_t byte_num = 0;
    purc_variant_cast_to_longint (val, &byte_num, false);
    ASSERT_EQ(byte_num, 17);
    purc_variant_unref(param[1]);
    purc_variant_unref(param[2]);
    purc_variant_unref(val);

    // close
    dynamic = purc_variant_object_get_by_ckey (stream, "close");
#if 0 // no such method
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    val = func (NULL, 1, param, false);
    ASSERT_NE(val, nullptr);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(val);
#else
    ASSERT_EQ(dynamic, nullptr);
#endif

    purc_variant_unref(ret_var);

    purc_variant_unload_dvobj (file);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}


TEST(dvobjs, dvobjs_file_stream_readbytes)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    purc_variant_t val = NULL;
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
    purc_variant_t file = purc_variant_load_dvobj_from_so ("FS", "FILE");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(purc_variant_is_object (file), true);


    purc_variant_t stream = purc_variant_object_get_by_ckey (file, "stream");
    ASSERT_NE(stream, nullptr);
    ASSERT_EQ(purc_variant_is_object (stream), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (stream, "open");
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
    strcat (file_path, "/stream.test");

    printf ("TEST stream_readbytes: \
            nr_args=2, param1=\"test_files/stream.test\":\n");
    param[0] = purc_variant_make_string (file_path, false);
    param[1] = purc_variant_make_string ("r+", false);
    ret_var = func (NULL, 2, param, false);
    ASSERT_EQ(purc_variant_is_type (ret_var,
                    PURC_VARIANT_TYPE_NATIVE), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);

    // seek
    dynamic = purc_variant_object_get_by_ckey (stream, "seek");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    param[1] = purc_variant_make_ulongint (10);
    param[2] = purc_variant_make_longint (SEEK_CUR);
    val = func (NULL, 3, param, false);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_LONGINT), true);
    int64_t byte_num = 0;
    purc_variant_cast_to_longint (val, &byte_num, false);
    ASSERT_EQ(byte_num, 10);
    purc_variant_unref(param[1]);
    purc_variant_unref(param[2]);
    purc_variant_unref(val);

    // readbytes
    dynamic = purc_variant_object_get_by_ckey (stream, "readbytes");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    param[1] = purc_variant_make_ulongint (20);
    val = func (NULL, 2, param, false);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_BSEQUENCE), true);
    ASSERT_EQ(purc_variant_bsequence_length (val), 20);
    purc_variant_unref(param[1]);
    purc_variant_unref(val);

    // close
    dynamic = purc_variant_object_get_by_ckey (stream, "close");
#if 0
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    val = func (NULL, 1, param, false);
    ASSERT_NE(val, nullptr);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(val);
#else
    ASSERT_EQ(dynamic, nullptr);
#endif
    purc_variant_unref(ret_var);

    purc_variant_unload_dvobj (file);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_file_stream_readlines)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    purc_variant_t val = NULL;
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
    purc_variant_t file = purc_variant_load_dvobj_from_so ("FS", "FILE");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(purc_variant_is_object (file), true);

    purc_variant_t stream = purc_variant_object_get_by_ckey (file, "stream");
    ASSERT_NE(stream, nullptr);
    ASSERT_EQ(purc_variant_is_object (stream), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (stream, "open");
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
    strcat (file_path, "/stream.test");

    printf ("TEST stream_readlines: nr_args=2, \
            param1=\"test_files/stream.test\":\n");
    param[0] = purc_variant_make_string (file_path, false);
    param[1] = purc_variant_make_string ("r+", false);
    ret_var = func (NULL, 2, param, false);

    ASSERT_EQ(purc_variant_is_type (ret_var,
                    PURC_VARIANT_TYPE_NATIVE), true);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);

    // seek
    dynamic = purc_variant_object_get_by_ckey (stream, "seek");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    param[1] = purc_variant_make_ulongint (0);
    param[2] = purc_variant_make_longint (SEEK_CUR);
    val = func (NULL, 3, param, false);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_LONGINT), true);
    int64_t byte_num = 0;
    purc_variant_cast_to_longint (val, &byte_num, false);
    ASSERT_EQ(byte_num, 0);
    purc_variant_unref(val);
    purc_variant_unref(param[1]);
    purc_variant_unref(param[2]);

    // readbytes
    dynamic = purc_variant_object_get_by_ckey (stream, "readlines");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    param[1] = purc_variant_make_ulongint (1);
    val = func (NULL, 2, param, false);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_STRING), true);
    ASSERT_STREQ(purc_variant_get_string_const(val), \
            "root:x:0:0:root:/root:/bin/bash");
    purc_variant_unref(val);
    purc_variant_unref(param[1]);

    // close
    dynamic = purc_variant_object_get_by_ckey (stream, "close");
#if 0
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    val = func (NULL, 1, param, false);
    ASSERT_NE(val, nullptr);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_BOOLEAN), true);
    purc_variant_unref(val);
#else
    ASSERT_EQ(dynamic, nullptr);
#endif
    purc_variant_unref(ret_var);

    purc_variant_unload_dvobj (file);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_file_stream_read_write_struct)
{
    purc_variant_t param[MAX_PARAM_NR] = {PURC_VARIANT_INVALID};
    purc_variant_t for_open[MAX_PARAM_NR] = {PURC_VARIANT_INVALID};
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t ret_result = PURC_VARIANT_INVALID;
    purc_variant_t test_file = PURC_VARIANT_INVALID;
    size_t line_number = 0;
    int equal = 0;
    const unsigned char *buf1 = NULL;
    const unsigned char *buf2 = NULL;
    size_t bsize = 0;
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
    purc_variant_t file = purc_variant_load_dvobj_from_so ("FS", "FILE");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(purc_variant_is_object (file), true);

    purc_variant_t stream = purc_variant_object_get_by_ckey (file, "stream");
    ASSERT_NE(stream, nullptr);
    ASSERT_EQ(purc_variant_is_object (stream), true);

    // open
    purc_variant_t dynamic = purc_variant_object_get_by_ckey (stream, "open");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method open = NULL;
    open = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(open, nullptr);

    // writestruct
    dynamic = purc_variant_object_get_by_ckey (stream, "writestruct");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method writestruct = NULL;
    writestruct = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(writestruct, nullptr);

    // readstruct
    dynamic = purc_variant_object_get_by_ckey (stream, "readstruct");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method readstruct = NULL;
    readstruct = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(readstruct, nullptr);

    // seek
    dynamic = purc_variant_object_get_by_ckey (stream, "seek");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method seek = NULL;
    seek = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(seek, nullptr);

    // close
    dynamic = purc_variant_object_get_by_ckey (stream, "close");
#if 0
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method close = NULL;
    close = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(close, nullptr);
#else
    ASSERT_EQ(dynamic, nullptr);
#endif

    char data_path[PATH_MAX+1];
    env = "DVOBJS_TEST_PATH";
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "test_files");
    std::cerr << "env: " << env << "=" << data_path << std::endl;

    // get test case list file
    char test_case[1024] = {0};
    strcpy (test_case, data_path);
    strcat (test_case, "/");
    strcat (test_case, "write.test");

    // get the temp file for write and read
    char temp_file[1024] = {0};
    strcpy (temp_file, data_path);
    strcat (temp_file, "/");
    strcat (temp_file, "rwstruct.test");

    char command[1024] = {0};
    strcat (command, "touch ");
    strcat (command, temp_file);

    FILE *fp = fopen(test_case, "r");   // open test case list
    ASSERT_NE(fp, nullptr);

    char *line = NULL;
    size_t sz = 0;
    ssize_t read = 0;
    size_t j = 0;
    size_t length_sub = 0;

    line_number = 0;

    while ((read = getline(&line, &sz, fp)) != -1) {
        *(line + read - 1) = 0;
        line_number ++;

        if (strncasecmp (line, "test_begin", 10) == 0)  {
            printf ("\ttest case on line %ld\n", line_number);

            // get parameters
            read = getline(&line, &sz, fp);
            *(line + read - 1) = 0;
            line_number ++;

            if (strcmp (line, "param_begin") == 0)  {
                j = 1;

                // get param
                while (1) {
                    read = getline(&line, &sz, fp);
                    *(line + read - 1) = 0;
                    line_number ++;

                    if (strcmp (line, "param_end") == 0)  {
                        break;
                    }
                    param[j] = get_variant (line, &length_sub);
                    j++;
                    ASSERT_LE(j, MAX_PARAM_NR);
                }

                // get result
                read = getline(&line, &sz, fp);
                *(line + read - 1) = 0;
                line_number ++;

                // test_result is useless
                ret_result = get_variant(line, &length_sub);
                purc_variant_unref(ret_result);

                // test case end
                while (1) {
                    read = getline(&line, &sz, fp);
                    *(line + read - 1) = 0;
                    line_number ++;

                    if (strcmp (line, "test_end") == 0)  {
                        break;
                    }
                }

                // remove temp file
                unlink (temp_file);
                int create_ok = system (command);
                ASSERT_EQ(create_ok, 0);

                // test process
                // open
                for_open[0] = purc_variant_make_string (temp_file, false);
                for_open[1] = purc_variant_make_string ("r+", false);
                test_file = open (NULL, 2, for_open, false);
                ASSERT_EQ(purc_variant_is_type (test_file,
                            PURC_VARIANT_TYPE_NATIVE), true);
                purc_variant_unref(for_open[0]);
                purc_variant_unref(for_open[1]);

                ret_result = param[1];
                for_open[0] = test_file;
                for_open[1] = param[1];
                for_open[2] = purc_variant_make_array (0, PURC_VARIANT_INVALID);

                for (size_t k = 2; k < j; k++)
                    purc_variant_array_append (for_open[2], param[k]);

                // write
                ret_var = writestruct (NULL, 3, for_open, false);
                ASSERT_EQ(purc_variant_is_type (ret_var,
                            PURC_VARIANT_TYPE_ULONGINT), true);
                purc_variant_unref(for_open[2]);
                purc_variant_unref(ret_var);

                // seek to the beginning
                for_open[0] = test_file;
                for_open[1] = purc_variant_make_ulongint (0);
                for_open[2] = purc_variant_make_longint (SEEK_SET);
                ret_var = seek (NULL, 3, for_open, false);

                ASSERT_EQ(purc_variant_is_type (ret_var,
                    PURC_VARIANT_TYPE_LONGINT), true);
                int64_t byte_num = 0;
                purc_variant_cast_to_longint (ret_var, &byte_num, false);
                ASSERT_EQ(byte_num, 0);
                purc_variant_unref(for_open[1]);
                purc_variant_unref(for_open[2]);
                purc_variant_unref(ret_var);

                // read
                for_open[0] = test_file;
                for_open[1] = ret_result;
                ret_var = readstruct (NULL, 2, for_open, false);

                ASSERT_EQ(purc_variant_is_type (ret_var,
                    PURC_VARIANT_TYPE_ARRAY), true);

                // compare
                ASSERT_EQ (purc_variant_array_get_size (ret_var), j - 2);
                for (size_t k = 0; k < j - 2; k++) {
                    double d1, d2;
                    int64_t i1, i2;
                    uint64_t u1, u2;
                    long double ld1, ld2;

                    ret_result = purc_variant_array_get (ret_var, k);
                    ASSERT_EQ(purc_variant_get_type (param[ k+ 2]),
                            purc_variant_get_type (ret_result));
                    switch (purc_variant_get_type (ret_result))   {
                        case PURC_VARIANT_TYPE_NUMBER:
                            purc_variant_cast_to_number (ret_result,
                                    &d1, false);
                            purc_variant_cast_to_number (param[k + 2],
                                    &d2, false);
                            ASSERT_EQ (d1, d2);
                            break;
                        case PURC_VARIANT_TYPE_LONGINT:
                            purc_variant_cast_to_longint (ret_result,
                                    &i1, false);
                            purc_variant_cast_to_longint (param[k + 2],
                                    &i2, false);
                            ASSERT_EQ (i1, i2);
                            break;
                        case PURC_VARIANT_TYPE_ULONGINT:
                            purc_variant_cast_to_ulongint (ret_result,
                                    &u1, false);
                            purc_variant_cast_to_ulongint (param[k + 2],
                                    &u2, false);
                            ASSERT_EQ (u1, u2);
                            break;
                        case PURC_VARIANT_TYPE_LONGDOUBLE:
                            purc_variant_cast_to_longdouble (ret_result,
                                    &ld1, false);
                            purc_variant_cast_to_longdouble (param[k + 2],
                                    &ld2, false);
                            ASSERT_EQ (ld1, ld2);
                            break;
                        case PURC_VARIANT_TYPE_STRING:
                            ASSERT_STREQ(purc_variant_get_string_const(
                                        ret_result),
                                    purc_variant_get_string_const (
                                        param[k + 2]));
                            break;
                        case PURC_VARIANT_TYPE_BSEQUENCE:
                            buf1 = purc_variant_get_bytes_const (ret_result,
                                    &bsize);
                            buf2 = purc_variant_get_bytes_const (param[k + 2],
                                    &bsize);
                            equal = memcmp (buf1, buf2, bsize);
                            ASSERT_EQ (equal, 0);
                            break;
                        default:
                            break;
                    }
                }
                purc_variant_unref(ret_var);

                // close
#if 0
                for_open[0] = test_file;
                ret_var = close (NULL, 1, for_open, false);
                purc_variant_unref(test_file);

                if (ret_var != PURC_VARIANT_INVALID) {
                    purc_variant_unref(ret_var);
                    ret_var = PURC_VARIANT_INVALID;
                }
#else
                purc_variant_unref(test_file);
#endif

                for (size_t i = 0; i < j; ++i) {
                    if (param[i] != PURC_VARIANT_INVALID) {
                        purc_variant_unref(param[i]);
                        param[i] = PURC_VARIANT_INVALID;
                    }
                }

                unlink (temp_file);
            }
            else
                continue;
        }
        else
            continue;
    }

    length_sub++;
    fclose(fp);
    if (line)
        free(line);

    purc_variant_unref(file);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    unlink (temp_file);

    purc_cleanup ();
}
