#include "purc.h"
#include "private/avl.h"
#include "private/arraylist.h"
#include "private/hashtable.h"
#include "purc-variant.h"
#include "private/variant.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/dvobjs.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

TEST(dvobjs, dvobjs_file_text_head)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;
    struct stat filestat;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t file = purc_variant_load_dvobj_from_so (
            "/usr/lib/purc-0.0/libpurc-dvobj-FS.so", "FILE");
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

    printf ("TEST text_head: nr_args=2, param1=\"/etc/passwd\", param2=0:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (0);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_EQ(purc_variant_string_length (ret_var), filestat.st_size);

    printf ("TEST text_head: nr_args=2, param1=\"/etc/passwd\", param2=3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (3);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    printf("\t\tReturn : %s\n", purc_variant_get_string_const (ret_var));

    printf ("TEST text_head: nr_args=2, param1=\"/etc/passwd\", param2=-3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (-3);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    printf("\t\tReturn : %s\n", purc_variant_get_string_const (ret_var));

    purc_variant_unload_dvobj (file);
    purc_cleanup ();
}

TEST(dvobjs, dvobjs_file_text_tail)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;
    struct stat filestat;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t file = purc_variant_load_dvobj_from_so (
            "/usr/lib/purc-0.0/libpurc-dvobj-FS.so", "FILE");
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

    printf ("TEST text_tail: nr_args=2, param1=\"/etc/passwd\", param2=0:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (0);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_EQ(purc_variant_string_length (ret_var), 0);

    printf ("TEST text_tail: nr_args=2, param1=\"/etc/passwd\", param2=3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (3);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    printf("\t\tReturn : %s\n", purc_variant_get_string_const (ret_var));

    printf ("TEST text_tail: nr_args=2, param1=\"/etc/passwd\", param2=-3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (-3);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    printf("\t\tReturn : %s\n", purc_variant_get_string_const (ret_var));

    purc_variant_unload_dvobj (file);

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_file_bin_head)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;
    struct stat filestat;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t file = purc_variant_load_dvobj_from_so (
            "/usr/lib/purc-0.0/libpurc-dvobj-FS.so", "FILE");
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

    printf ("TEST bin_head: nr_args=2, param1=\"/etc/passwd\", param2=0 :\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (0);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_EQ(purc_variant_sequence_length(ret_var), filestat.st_size);

    printf ("TEST bin_head: nr_args=2, param1=\"/etc/passwd\", param2=3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (3);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_EQ(purc_variant_sequence_length(ret_var), 3);

    printf ("TEST bin_head: nr_args=2, param1=\"/etc/passwd\", param2=-3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (-3);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_EQ(purc_variant_sequence_length(ret_var), filestat.st_size - 3);

    purc_variant_unload_dvobj (file);

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_file_bin_tail)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;
    struct stat filestat;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t file = purc_variant_load_dvobj_from_so (
            "/usr/lib/purc-0.0/libpurc-dvobj-FS.so", "FILE");
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
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_EQ(purc_variant_sequence_length(ret_var), filestat.st_size);

    printf ("TEST bin_tail: nr_args=2, param1=\"/etc/passwd\", param2=3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (3);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_EQ(purc_variant_sequence_length(ret_var), 3);

    printf ("TEST bin_tail: nr_args=2, param1=\"/etc/passwd\", param2=-3:\n");
    param[0] = purc_variant_make_string ("/etc/passwd", false);
    param[1] = purc_variant_make_number (-3);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_EQ(purc_variant_sequence_length(ret_var), filestat.st_size - 3);

    purc_variant_unload_dvobj (file);

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_file_stream_open_seek_close)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;
    purc_variant_t val = NULL;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t file = purc_variant_load_dvobj_from_so (
            "/usr/lib/purc-0.0/libpurc-dvobj-FS.so", "FILE");
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

    char *data_path = getenv("DVOBJS_TEST_PATH");
    ASSERT_NE(data_path, nullptr);

    char file_path[1024] = {0};
    strcpy (file_path, data_path);
    strcat (file_path, "/stream.test");

    printf ("TEST stream_open_seek_close: nr_args=2, param1=\"test_files/stream.test\":\n");
    param[0] = purc_variant_make_string (file_path, false);
    ret_var = func (NULL, 1, param);

    ASSERT_EQ(purc_variant_is_type (ret_var,
                    PURC_VARIANT_TYPE_NATIVE), true);


    // seek
    dynamic = purc_variant_object_get_by_ckey (stream, "seek");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    param[1] = purc_variant_make_ulongint (17);
    val = func (NULL, 2, param);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_ULONGINT), true);
    int64_t byte_num = 0;
    purc_variant_cast_to_longint (val, &byte_num, false); 
    ASSERT_EQ(byte_num, 17);

    // close
    dynamic = purc_variant_object_get_by_ckey (stream, "close");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);

    ASSERT_EQ(purc_variant_is_type (ret_var,
                    PURC_VARIANT_TYPE_BOOLEAN), true);

    purc_variant_unref(ret_var);
    purc_variant_unload_dvobj (file);

    purc_cleanup ();
}


TEST(dvobjs, dvobjs_file_stream_readbytes)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;
    purc_variant_t val = NULL;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t file = purc_variant_load_dvobj_from_so (
            "/usr/lib/purc-0.0/libpurc-dvobj-FS.so", "FILE");
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

    char *data_path = getenv("DVOBJS_TEST_PATH");
    ASSERT_NE(data_path, nullptr);

    char file_path[1024] = {0};
    strcpy (file_path, data_path);
    strcat (file_path, "/stream.test");

    printf ("TEST stream_readbytes: nr_args=2, param1=\"test_files/stream.test\":\n");
    param[0] = purc_variant_make_string (file_path, false);
    ret_var = func (NULL, 1, param);

    ASSERT_EQ(purc_variant_is_type (ret_var,
                    PURC_VARIANT_TYPE_NATIVE), true);


    // seek
    dynamic = purc_variant_object_get_by_ckey (stream, "seek");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    param[1] = purc_variant_make_ulongint (10);
    val = func (NULL, 2, param);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_ULONGINT), true);
    int64_t byte_num = 0;
    purc_variant_cast_to_longint (val, &byte_num, false); 
    ASSERT_EQ(byte_num, 10);
    purc_variant_unref(val);

    // readbytes
    dynamic = purc_variant_object_get_by_ckey (stream, "readbytes");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    param[1] = purc_variant_make_ulongint (20);
    val = func (NULL, 2, param);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_BSEQUENCE), true);
    ASSERT_EQ(purc_variant_sequence_length (val), 20);
    purc_variant_unref(val);

    // close
    dynamic = purc_variant_object_get_by_ckey (stream, "close");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);

    ASSERT_EQ(purc_variant_is_type (ret_var,
                    PURC_VARIANT_TYPE_BOOLEAN), true);

    purc_variant_unref(ret_var);
    purc_variant_unload_dvobj (file);

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_file_stream_readlines)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;
    purc_variant_t val = NULL;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t file = purc_variant_load_dvobj_from_so (
            "/usr/lib/purc-0.0/libpurc-dvobj-FS.so", "FILE");
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

    char *data_path = getenv("DVOBJS_TEST_PATH");
    ASSERT_NE(data_path, nullptr);

    char file_path[1024] = {0};
    strcpy (file_path, data_path);
    strcat (file_path, "/stream.test");

    printf ("TEST stream_readlines: nr_args=2, param1=\"test_files/stream.test\":\n");
    param[0] = purc_variant_make_string (file_path, false);
    ret_var = func (NULL, 1, param);

    ASSERT_EQ(purc_variant_is_type (ret_var,
                    PURC_VARIANT_TYPE_NATIVE), true);


    // seek
    dynamic = purc_variant_object_get_by_ckey (stream, "seek");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    param[1] = purc_variant_make_ulongint (0);
    val = func (NULL, 2, param);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_ULONGINT), true);
    int64_t byte_num = 0;
    purc_variant_cast_to_longint (val, &byte_num, false); 
    ASSERT_EQ(byte_num, 0);
    purc_variant_unref(val);

    // readbytes
    dynamic = purc_variant_object_get_by_ckey (stream, "readlines");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    param[1] = purc_variant_make_ulongint (1);
    val = func (NULL, 2, param);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_STRING), true);
    ASSERT_STREQ(purc_variant_get_string_const(val), "root:x:0:0:root:/root:/bin/bash");
    purc_variant_unref(val);

    // close
    dynamic = purc_variant_object_get_by_ckey (stream, "close");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);

    ASSERT_EQ(purc_variant_is_type (ret_var,
                    PURC_VARIANT_TYPE_BOOLEAN), true);

    purc_variant_unref(ret_var);
    purc_variant_unload_dvobj (file);

    purc_cleanup ();
}


TEST(dvobjs, dvobjs_file_stream_read_write_struct)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;
    purc_variant_t val = NULL;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t file = purc_variant_load_dvobj_from_so (
            "/usr/lib/purc-0.0/libpurc-dvobj-FS.so", "FILE");
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

    char *data_path = getenv("DVOBJS_TEST_PATH");
    ASSERT_NE(data_path, nullptr);

    char file_path[1024] = {0};
    strcpy (file_path, data_path);
    strcat (file_path, "/stream.test");

    printf ("TEST stream_readlines: nr_args=2, param1=\"test_files/stream.test\":\n");
    param[0] = purc_variant_make_string (file_path, false);
    ret_var = func (NULL, 1, param);

    ASSERT_EQ(purc_variant_is_type (ret_var,
                    PURC_VARIANT_TYPE_NATIVE), true);


    // seek
    dynamic = purc_variant_object_get_by_ckey (stream, "seek");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    param[1] = purc_variant_make_ulongint (0);
    val = func (NULL, 2, param);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_ULONGINT), true);
    int64_t byte_num = 0;
    purc_variant_cast_to_longint (val, &byte_num, false); 
    ASSERT_EQ(byte_num, 0);
    purc_variant_unref(val);

    // readbytes
    dynamic = purc_variant_object_get_by_ckey (stream, "readlines");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    param[1] = purc_variant_make_ulongint (1);
    val = func (NULL, 2, param);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_STRING), true);
    ASSERT_STREQ(purc_variant_get_string_const(val), "root:x:0:0:root:/root:/bin/bash");
    purc_variant_unref(val);

    // close
    dynamic = purc_variant_object_get_by_ckey (stream, "close");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = ret_var;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);

    ASSERT_EQ(purc_variant_is_type (ret_var,
                    PURC_VARIANT_TYPE_BOOLEAN), true);

    purc_variant_unref(ret_var);
    purc_variant_unload_dvobj (file);

    purc_cleanup ();
}
