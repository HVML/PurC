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

static purc_variant_t getter(
        purc_variant_t root, size_t nr_args, purc_variant_t * argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    purc_variant_t value = purc_variant_make_number (3.1415926);
    return value;
}

static purc_variant_t setter(
        purc_variant_t root, size_t nr_args, purc_variant_t * argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    purc_variant_t value = purc_variant_make_number (2.71828828);
    return value;
}

static bool rws_releaser (void* entity)
{
    UNUSED_PARAM(entity);
    return true;
}

static struct purc_native_ops rws_ops = {
    .property_getter       = NULL,
    .property_setter       = NULL,
    .property_eraser       = NULL,
    .property_cleaner      = NULL,
    .cleaner               = NULL,
    .eraser                = rws_releaser,
    .observe               = NULL,
};

static void replace_for_bsequence(char *buf, size_t *length_sub)
{
    size_t tail = 0;
    size_t head = 0;
    char chr = 0;
    unsigned char number = 0;
    unsigned char temp = 0;

    for (tail = 0; tail < *length_sub; tail++)  {
        if (*(buf + tail) == '\\')  {
            tail++;
            chr = *(buf + tail);
            if ((chr >= '0') && (chr <= '9'))
                number = chr - '0';
            else if ((chr >= 'a') && (chr <= 'z'))
                number = chr - 'a';
            else if ((chr >= 'A') && (chr <= 'Z'))
                number = chr - 'A';
            number = number << 4;

            tail++;
            chr = *(buf + tail);
            if ((chr >= '0') && (chr <= '9'))
                temp = chr - '0';
            else if ((chr >= 'a') && (chr <= 'z'))
                temp = chr - 'a';
            else if ((chr >= 'A') && (chr <= 'Z'))
                temp = chr - 'A';
            number |= temp;

            *(buf + head) = number;
            head++;
        } else {
            *(buf + head) = *(buf + tail);
            head++;
        }
    }

    *length_sub = head;

    return;
}

purc_variant_t get_variant (char *buf, size_t *length)
{
    purc_variant_t ret_var = NULL;
    purc_variant_t val = NULL;
    char *temp = NULL;
    char *temp_end = NULL;
    char tag[64];
    double d = 0.0d;
    int64_t i64;
    uint64_t u64;
    long double ld = 0.0d;
    int number = 0;
    int i = 0;
    size_t length_sub = 0;

    *length = 0;

    temp = strchr (buf, ':');
    snprintf (tag, (temp - buf + 1), "%s", buf);

    switch (*tag) {
        case 'a':
        case 'A':
            switch (*(tag + 1))  {
                case 'r':       // array
                case 'R':
                    temp_end = strchr (temp + 1, ':');
                    snprintf (tag, (temp_end - temp), "%s", temp + 1);
                    number = atoi (tag);
                    temp = temp_end + 1;

                    ret_var = purc_variant_make_array (0, PURC_VARIANT_INVALID);
                    for (i = 0; i < number; i++) {
                        val = get_variant (temp, &length_sub);
                        purc_variant_array_append (ret_var, val);
                        purc_variant_unref (val);
                        if (i < number - 1)
                            temp += (length_sub + 1);
                    }
                    *length = temp - buf + length_sub;
                    break;
                case 't':       // atomstring
                case 'T':
                    temp = strchr (temp + 1, '\"');
                    temp_end = strchr (temp + 1, '\"');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_atom_string (temp + 1, false);
                    *length = temp_end + 1 - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        case 'b':
        case 'B':
            switch (*(tag + 1))  {
                case 'o':       // boolean
                case 'O':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    if (strncasecmp (temp + 1, "true", 4) == 0)
                        ret_var = purc_variant_make_boolean (true);
                    else
                        ret_var = purc_variant_make_boolean (false);
                    *length = temp_end - buf;
                    break;
                case 's':       // byte sequence
                case 'S':
                    temp = strchr (temp + 1, '\"');
                    temp_end = strchr (temp + 1, '\"');
                    length_sub = temp_end - temp - 1;
                    replace_for_bsequence(temp + 1, &length_sub);
                    ret_var = purc_variant_make_byte_sequence (temp + 1,
                            length_sub);
                    *length = temp_end + 1 - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        case 'd':               // dynamic
        case 'D':
            temp_end = strchr (buf, ';');
            *temp_end = 0x00;
            ret_var = purc_variant_make_dynamic (getter, setter);
            *length = temp_end - buf;
            break;
        case 'i':
        case 'I':
            temp_end = strchr (buf, ';');
            *length = temp_end - buf;
            ret_var = PURC_VARIANT_INVALID;
            break;
        case 'l':
        case 'L':
            switch (*(tag + 4))  {
                case 'd':       // long double
                case 'D':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ld = atof (temp + 1);
                    ret_var = purc_variant_make_longdouble (ld);
                    *length = temp_end - buf;
                    break;
                case 'i':       // long int
                case 'I':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    i64 = atoll (temp + 1);
                    ret_var = purc_variant_make_longint (i64);
                    *length = temp_end - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        case 'n':
        case 'N':
            switch (*(tag + 2))  {
                case 't':       // native;
                case 'T':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_native ((void *)"hello world",
                            &rws_ops);
                    *length = temp_end - buf;
                    break;
                case 'l':       // null;
                case 'L':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_null ();
                    *length = temp_end - buf;
                    break;
                case 'm':       // number
                case 'M':
                    temp_end = strchr (temp + 1, ';');
                    *temp_end = 0x00;
                    d = atof (temp + 1);
                    ret_var = purc_variant_make_number (d);
                    *length = temp_end - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        case 'o':               // object
        case 'O':
            temp_end = strchr (temp + 1, ':');
            snprintf (tag, (temp_end - temp), "%s", temp + 1);
            number = atoi (tag);
            temp = temp_end + 1;

            ret_var = purc_variant_make_object (0, PURC_VARIANT_INVALID,
                                                    PURC_VARIANT_INVALID);
            for (i = 0; i < number; i++) {
                // get key
                purc_variant_t key = PURC_VARIANT_INVALID;
                temp = strchr (temp, '\"');
                temp_end = strchr (temp + 1, '\"');
                snprintf (tag, temp_end - temp, "%s", temp + 1);
                key = purc_variant_make_string(tag, true);

                // get value
                temp = temp_end + 2;
                *length = temp - buf;
                val = get_variant (temp, &length_sub);
                purc_variant_object_set (ret_var, key, val);

                purc_variant_unref (key);
                purc_variant_unref (val);
                if (i < number - 1)
                    temp += (length_sub + 1);
            }
            *length = temp - buf + length_sub;
            break;
        case 's':
        case 'S':
            switch (*(tag + 1))  {
                case 'e':       // set
                case 'E':
                    temp_end = strchr (temp + 1, ':');
                    snprintf (tag, (temp_end - temp), "%s", temp + 1);
                    number = atoi (tag);
                    temp = temp_end + 1;

                    ret_var = purc_variant_make_set_by_ckey(0, NULL, NULL);
                    for (i = 0; i < number; i++) {
                        val = get_variant (temp, &length_sub);
                        purc_variant_set_add (ret_var, val, false);
                        purc_variant_unref (val);
                        if (i < number - 1)
                            temp += (length_sub + 1);
                    }
                    *length = temp - buf + length_sub;
                    break;
                case 't':       // sting
                case 'T':
                    temp = strchr (temp + 1, '\"');
                    temp_end = strchr (temp + 1, '\"');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_string (temp + 1, false);
                    *length = temp_end + 1 - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        case 'u':
        case 'U':
            switch (*(tag + 1))  {
                case 'l':       // unsigned long int
                case 'L':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    u64 = atoll (temp + 1);
                    ret_var = purc_variant_make_ulongint (u64);
                    *length = temp_end - buf;
                    break;
                case 'n':       // undefined
                case 'N':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_undefined ();
                    *length = temp_end - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        default:
            temp_end = strchr (buf, ';');
            *length = temp_end - buf;
            ret_var = PURC_VARIANT_INVALID;
            break;
    }

    return ret_var;
}

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
    param[2] = purc_variant_make_longint (SEEK_CUR);
    val = func (NULL, 3, param);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_LONGINT), true);
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
    param[2] = purc_variant_make_longint (SEEK_CUR);
    val = func (NULL, 3, param);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_LONGINT), true);
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
    param[2] = purc_variant_make_longint (SEEK_CUR);
    val = func (NULL, 3, param);

    ASSERT_EQ(purc_variant_is_type (val,
                    PURC_VARIANT_TYPE_LONGINT), true);
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
    purc_variant_t param[10] = {0};
    purc_variant_t for_open[10] = {0};
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t ret_result = PURC_VARIANT_INVALID;
    purc_variant_t test_file = PURC_VARIANT_INVALID;
    size_t line_number = 0;

    // get and function
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
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method close = NULL;
    close = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(close, nullptr);


    // get test file
    char* data_path = getenv("DVOBJS_TEST_PATH");
    ASSERT_NE(data_path, nullptr);

    char file_path[1024] = {0};
    strcpy (file_path, data_path);
    strcat (file_path, "/");
    strcat (file_path, "write.test");

    char test_path[1024] = {0};
    strcpy (test_path, data_path);
    strcat (test_path, "/");
    strcat (test_path, "rwstruct.test");

    char command[1024] = {0};
    strcat (command, "touch ");
    strcat (command, test_path);

    FILE *fp = fopen(file_path, "r");   // open test_list
    ASSERT_NE(fp, nullptr);

    char *line = NULL;
    size_t sz = 0;
    ssize_t read = 0;
    int j = 0;
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
                        if (param[j]) {
                            purc_variant_unref(param[j]);
                            param[j] = NULL;
                        }
                        param[j] = NULL;
                        break;
                    }
                    param[j] = get_variant (line, &length_sub);
                    j++;
                }

                // get result
                read = getline(&line, &sz, fp);
                *(line + read - 1) = 0;
                line_number ++;

                ret_result = get_variant(line, &length_sub);

                // test case end
                while (1) {
                    read = getline(&line, &sz, fp);
                    *(line + read - 1) = 0;
                    line_number ++;

                    if (strcmp (line, "test_end") == 0)  {
                        break;
                    }
                }

                // remove test file
                unlink (test_path);
                int create_ok = system (command);
                ASSERT_EQ(create_ok, 0);

                // test process
                // open
                for_open[0] = purc_variant_make_string (test_path, false);
                test_file = open (NULL, 1, for_open);
                ASSERT_EQ(purc_variant_is_type (test_file,
                            PURC_VARIANT_TYPE_NATIVE), true);

                ret_result = param[1];
                for_open[0] = test_file;
                for_open[1] = param[1];
                for_open[2] = purc_variant_make_array (0, PURC_VARIANT_INVALID);

                for (int k = 2; k < j; k++)
                    purc_variant_array_append (for_open[2], param[k]);

                // write
                ret_var = writestruct (NULL, 3, for_open);
                ASSERT_EQ(purc_variant_is_type (ret_var,
                            PURC_VARIANT_TYPE_ULONGINT), true);

                // seek to the beginning
                for_open[0] = test_file;
                for_open[1] = purc_variant_make_ulongint (0);
                for_open[2] = purc_variant_make_longint (SEEK_SET);
                ret_var = seek (NULL, 3, for_open);

                ASSERT_EQ(purc_variant_is_type (ret_var,
                    PURC_VARIANT_TYPE_LONGINT), true);
                int64_t byte_num = 0;
                purc_variant_cast_to_longint (ret_var, &byte_num, false);
                ASSERT_EQ(byte_num, 0);

                // read
                for_open[0] = test_file;
                for_open[1] = ret_result;
                ret_var = readstruct (NULL, 2, for_open);

                ASSERT_EQ(purc_variant_is_type (ret_var,
                    PURC_VARIANT_TYPE_ARRAY), true);

                // compare
                ASSERT_EQ (purc_variant_array_get_size (ret_var), j - 2);

                // close
                for_open[0] = test_file;
                close (NULL, 1, for_open);

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
    purc_cleanup ();
}
