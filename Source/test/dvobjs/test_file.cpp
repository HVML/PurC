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
    int version = 0;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t file = purc_variant_load_from_so (
            "/usr/lib/purc-0.0/libpurc-dvobj-FILE.so", "FILE", &version);
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

    purc_variant_unload_so (file);
    purc_cleanup ();
}

TEST(dvobjs, dvobjs_file_text_tail)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;
    struct stat filestat;
    int version = 0;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t file = purc_variant_load_from_so (
            "/usr/lib/purc-0.0/libpurc-dvobj-FILE.so", "FILE", &version);
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

    purc_variant_unload_so (file);

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_file_bin_head)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;
    struct stat filestat;
    int version = 0;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t file = purc_variant_load_from_so (
            "/usr/lib/purc-0.0/libpurc-dvobj-FILE.so", "FILE", &version);
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

    purc_variant_unload_so (file);

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_file_bin_tail)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;
    struct stat filestat;
    int version = 0;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t file = purc_variant_load_from_so (
            "/usr/lib/purc-0.0/libpurc-dvobj-FILE.so", "FILE", &version);
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

    purc_variant_unload_so (file);

    purc_cleanup ();
}
