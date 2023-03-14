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

#include "private/variant.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

static inline int my_puts(const char* str)
{
#if 0
    return fputs(str, stderr);
#else
    (void)str;
    return 0;
#endif
}

// to test: serialize a boolean
TEST(variant, serialize_boolean)
{
    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t my_boolean = purc_variant_make_boolean(true);
    ASSERT_NE(my_boolean, PURC_VARIANT_INVALID);
    ASSERT_EQ(my_boolean->type, PURC_VARIANT_TYPE_BOOLEAN);

    char buf[8];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(my_boolean, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_EQ(len_expected, 4);
    ASSERT_EQ(n, 4);

    buf[4] = 0;
    ASSERT_STREQ(buf, "true");

    len_expected = 0;
    n = purc_variant_serialize(my_boolean, my_rws,
            0, PCVRNT_SERIALIZE_OPT_IGNORE_ERRORS, &len_expected);
    ASSERT_EQ(n, 3);
    ASSERT_EQ(len_expected, 4);

    buf[7] = 0;
    ASSERT_STREQ(buf, "truetru");

    purc_rwstream_destroy(my_rws);

    purc_variant_unref(my_boolean);

    purc_cleanup ();
}

// to test: serialize a null
TEST(variant, serialize_null)
{
    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t my_null = purc_variant_make_null();
    ASSERT_NE(my_null, PURC_VARIANT_INVALID);

    char buf[8];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(my_null, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_EQ(len_expected, 4);
    ASSERT_EQ(n, 4);

    buf[4] = 0;
    ASSERT_STREQ(buf, "null");

    len_expected = 0;
    n = purc_variant_serialize(my_null, my_rws,
            0, PCVRNT_SERIALIZE_OPT_IGNORE_ERRORS, &len_expected);
    ASSERT_EQ(n, 3);
    ASSERT_EQ(len_expected, 4);

    buf[7] = 0;
    ASSERT_STREQ(buf, "nullnul");

    purc_rwstream_destroy(my_rws);

    purc_variant_unref(my_null);

    purc_cleanup ();
}

// to test: serialize an undefined
TEST(variant, serialize_undefined)
{
    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t my_undefined = purc_variant_make_undefined();
    ASSERT_NE(my_undefined, PURC_VARIANT_INVALID);

    char buf[15];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(my_undefined, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GE(len_expected, 4);
    ASSERT_EQ(n, 4);

    buf[n] = 0;
    ASSERT_STREQ(buf, "null");

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    n = purc_variant_serialize(my_undefined, my_rws,
            0, PCVRNT_SERIALIZE_OPT_RUNTIME_STRING | PCVRNT_SERIALIZE_OPT_PLAIN,
            &len_expected);
    ASSERT_GE(len_expected, 13);
    ASSERT_EQ(n, 13);

    buf[n] = 0;
    ASSERT_STREQ(buf, "\"<undefined>\"");

    n = purc_variant_serialize(my_undefined, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GE(len_expected, 4);
    ASSERT_EQ(n, -1);

    purc_rwstream_destroy(my_rws);

    purc_variant_unref(my_undefined);

    purc_cleanup ();
}

// to test: serialize an exception
TEST(variant, serialize_exception)
{
    purc_variant_t my_variant;
    purc_rwstream_t my_rws;
    size_t len_expected;
    ssize_t n;
    char buf[64];

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    purc_atom_t atom = purc_get_except_atom_by_id(PURC_EXCEPT_BAD_ENCODING);
    my_variant = purc_variant_make_exception(atom);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "\"BadEncoding\"");

    purc_variant_unref(my_variant);

    purc_rwstream_destroy(my_rws);
    purc_cleanup ();
}

// to test: serialize a number
TEST(variant, serialize_number)
{
    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    /* case 1: no decimal */
    purc_variant_t my_variant = purc_variant_make_number(123.0);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    char buf[32];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "123");
    purc_variant_unref(my_variant);

    /* case 2: nozero */
    my_variant = purc_variant_make_number(123.456000000);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_NOZERO, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "123.456");
    purc_variant_unref(my_variant);

    /* case 3: customized double format */
    my_variant = purc_variant_make_number(1.1234567890);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    len_expected = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    purc_set_local_data("format-double", (uintptr_t)"%.6f", NULL);
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "1.123457");
    purc_variant_unref(my_variant);

    /* case 4: customized double format with nozero */
    my_variant = purc_variant_make_number(1.12345600123);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    len_expected = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    purc_set_local_data("format-double", (uintptr_t)"%.7f", NULL);
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_NOZERO, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "1.123456");
    purc_variant_unref(my_variant);

    purc_rwstream_destroy(my_rws);

    purc_cleanup ();
}

// to test: serialize a long integer
TEST(variant, serialize_longint)
{
    purc_variant_t my_variant;
    purc_rwstream_t my_rws;
    size_t len_expected;
    ssize_t n;
    char buf[64];

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    /* case 1: unsigned long int */
    my_variant = purc_variant_make_longint(123456789L);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "123456789");

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_REAL_EJSON | PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "123456789L");

    purc_variant_unref(my_variant);

    /* case 2: unsigned long int */
    my_variant = purc_variant_make_ulongint(123456789UL);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_NOZERO, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "123456789");

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_REAL_EJSON | PCVRNT_SERIALIZE_OPT_NOZERO, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "123456789UL");

    purc_variant_unref(my_variant);

    purc_rwstream_destroy(my_rws);
    purc_cleanup ();
}

// to test: serialize a long double
TEST(variant, serialize_longdouble)
{
    purc_variant_t my_variant;
    purc_rwstream_t my_rws;
    size_t len_expected;
    ssize_t n;
    char buf[128];

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    /* case 1: long double */
    my_variant = purc_variant_make_longdouble(123456789.2345);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_REAL_EJSON | PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "123456789.23450001FL");

    purc_variant_unref(my_variant);

    /* case 2: customized double format */
    my_variant = purc_variant_make_longdouble(1.1234567890);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    len_expected = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    purc_set_local_data("format-long-double", (uintptr_t)"%.6Lf", NULL);
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_REAL_EJSON | PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "1.123457FL");
    purc_variant_unref(my_variant);

    /* case 4: customized double format with nozero */
    my_variant = purc_variant_make_longdouble(1.12345600123);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    len_expected = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    purc_set_local_data("format-long-double", (uintptr_t)"%.7Lf", NULL);
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_REAL_EJSON | PCVRNT_SERIALIZE_OPT_NOZERO, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "1.123456FL");
    purc_variant_unref(my_variant);
    purc_rwstream_destroy(my_rws);
    purc_cleanup ();
}

static purc_variant_t my_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    (void)root;
    (void)call_flags;

    if (nr_args > 0)
        return * argv;

    return purc_variant_make_undefined();
}

static purc_variant_t my_setter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    (void)(root);
    (void)(nr_args);
    (void)(argv);
    (void)call_flags;

    return purc_variant_make_boolean(false);
}

// to test: serialize a dynamic value
TEST(variant, serialize_dynamic)
{
    purc_variant_t my_variant;
    purc_rwstream_t my_rws;
    size_t len_expected;
    ssize_t n;
    char buf[128];

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    my_variant = purc_variant_make_dynamic(my_getter, my_setter);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "null");

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_RUNTIME_STRING | PCVRNT_SERIALIZE_OPT_PLAIN,
            &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "\"<dynamic>\"");

    purc_variant_unref(my_variant);

    purc_rwstream_destroy(my_rws);
    purc_cleanup ();
}

static void _my_releaser (void* native_entity)
{
    my_puts("my_releaser is called\n");
    free (native_entity);
}

static struct purc_native_ops _my_ops = {
    .property_getter       = NULL,
    .property_setter       = NULL,
    .property_cleaner      = NULL,
    .property_eraser       = NULL,

    .updater               = NULL,
    .cleaner               = NULL,
    .eraser                = NULL,
    .did_matched           = NULL,

    .on_observe           = NULL,
    .on_forget            = NULL,
    .on_release           = _my_releaser,
};

// to test: serialize a native entity
TEST(variant, serialize_native)
{
    purc_variant_t my_variant;
    purc_rwstream_t my_rws;
    size_t len_expected;
    ssize_t n;
    char buf[128];

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    my_variant = purc_variant_make_native(strdup("HVML"), &_my_ops);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "null");

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_RUNTIME_STRING | PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "\"<native>\"");
    purc_variant_unref(my_variant);

    purc_rwstream_destroy(my_rws);
    purc_cleanup ();
}

// to test: serialize an atomstring
TEST(variant, serialize_atomstring)
{
    purc_variant_t my_variant;
    purc_rwstream_t my_rws;
    size_t len_expected;
    ssize_t n;
    char buf[64];

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    /* case 1: static string */
    my_variant = purc_variant_make_atom_string_static("HVML", false);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "\"HVML\"");

    purc_variant_unref(my_variant);

    /* case 2: non-static string */
    my_variant = purc_variant_make_atom_string("PurC", false);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_NOZERO, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "\"PurC\"");

    purc_variant_unref(my_variant);

    purc_rwstream_destroy(my_rws);
    purc_cleanup ();
}

// to test: serialize a string
TEST(variant, serialize_string)
{
    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t my_variant =
        purc_variant_make_string("\r\n\b\f\t\"\x1c'", false);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    char buf[64] = { };
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);
    my_puts("Serialized string with PCVRNT_SERIALIZE_OPT_PLAIN flag:");
    my_puts(buf);

    buf[n] = 0;
    ASSERT_STREQ(buf, "\"\\r\\n\\b\\f\\t\\\"\\u001c'\"");

    purc_variant_unref(my_variant);

    memset(buf, '~', sizeof(buf));
    buf[sizeof(buf) - 1] = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);

    my_variant =
        purc_variant_make_string("这是一个很长的中文字符串", false);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    my_puts("Serialized string with PCVRNT_SERIALIZE_OPT_PLAIN flag:");
    my_puts(buf);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "\"这是一个很长的中文字符串\"");

    purc_variant_unref(my_variant);

    purc_rwstream_destroy(my_rws);

    purc_cleanup ();
}

// to test: serialize a byte sequence
TEST(variant, serialize_bsequence)
{
    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t my_variant =
        purc_variant_make_byte_sequence("\x59\x1C\x88\xAF", 4);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    char buf[128];
    size_t len_expected;
    ssize_t n;

    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    /* case 0: hex string */
    memset(buf, '~', sizeof(buf));
    buf[sizeof(buf) - 1] = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_BSEQUENCE_HEX_STRING, &len_expected);
    ASSERT_GT(n, 0);
    my_puts("Serialized byte sequence with PCVRNT_SERIALIZE_OPT_BSEQUENCE_HEX_STRING flag:");
    my_puts(buf);
    buf[n] = 0;
    ASSERT_STREQ(buf, "\"591c88af\"");

    /* case 1: hex */
    memset(buf, '~', sizeof(buf));
    buf[sizeof(buf) - 1] = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_BSEQUENCE_HEX, &len_expected);
    ASSERT_GT(n, 0);
    my_puts("Serialized byte sequence with PCVRNT_SERIALIZE_OPT_BSEQUENCE_HEX flag:");
    my_puts(buf);
    buf[n] = 0;
    ASSERT_STREQ(buf, "bx591c88af");

    /* case 2: binary */
    memset(buf, '~', sizeof(buf));
    buf[sizeof(buf) - 1] = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_BSEQUENCE_BIN, &len_expected);
    ASSERT_GT(n, 0);
    my_puts("Serialized byte sequence with PCVRNT_SERIALIZE_OPT_BSEQUENCE_BIN flag:");
    my_puts(buf);
    buf[n] = 0;
    ASSERT_STREQ(buf, "bb01011001000111001000100010101111");

    /* case 3: binary with dot */
    memset(buf, '~', sizeof(buf));
    buf[sizeof(buf) - 1] = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_BSEQUENCE_BIN_DOT, &len_expected);
    ASSERT_GT(n, 0);
    my_puts("Serialized byte sequence with PCVRNT_SERIALIZE_OPT_BSEQUENCE_BIN_DOT flag:");
    my_puts(buf);
    buf[n] = 0;
    ASSERT_STREQ(buf, "bb0101.1001.0001.1100.1000.1000.1010.1111");

    /* case 4: Base64 */
    memset(buf, '~', sizeof(buf));
    buf[sizeof(buf) - 1] = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_BSEQUENCE_BASE64, &len_expected);
    ASSERT_GT(n, 0);
    my_puts("Serialized byte sequence with PCVRNT_SERIALIZE_OPT_BSEQUENCE_BASE64 flag:");
    my_puts(buf);
    buf[n] = 0;
    ASSERT_STREQ(buf, "b64WRyIrw==");

    purc_variant_unref(my_variant);

    /* case 4: long sequence */
    my_variant =
        purc_variant_make_byte_sequence("\x59\x1C\x88\xAF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\xEF\x00", 55);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    memset(buf, '~', sizeof(buf));
    buf[sizeof(buf) - 1] = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_BSEQUENCE_HEX, &len_expected);
    ASSERT_GT(n, 0);
    my_puts("Serialized byte sequence with PCVRNT_SERIALIZE_OPT_BSEQUENCE_HEX flag:");
    my_puts(buf);
    buf[n] = 0;
    ASSERT_STREQ(buf, "bx591c88afefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefef00");

    purc_variant_unref(my_variant);

    purc_rwstream_destroy(my_rws);

    purc_cleanup ();
}

// to test: serialize an array
TEST(variant, serialize_array)
{
    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t v1 = purc_variant_make_number(123.0);
    purc_variant_t v2 = purc_variant_make_number(123.456);

    purc_variant_t my_variant = purc_variant_make_array(2, v1, v2);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    char buf[64] = { };
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    my_puts("Serialized array with PCVRNT_SERIALIZE_OPT_PLAIN flag:");
    my_puts(buf);
    ASSERT_GT(n, 0);
    ASSERT_GT(len_expected, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "[123,123.456]");

    len_expected = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    memset(buf, 0, sizeof(buf));
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_SPACED, &len_expected);
    my_puts("Serialized array with PCVRNT_SERIALIZE_OPT_SPACED flag:");
    my_puts(buf);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "[ 123, 123.456 ]");

    len_expected = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    memset(buf, 0, sizeof(buf));
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_NOZERO, &len_expected);
    my_puts("Serialized array with PCVRNT_SERIALIZE_OPT_NOZERO flag:");
    my_puts(buf);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "[123,123.456]");

    len_expected = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    memset(buf, 0, sizeof(buf));
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PRETTY, &len_expected);
    my_puts("Serialized array with PCVRNT_SERIALIZE_OPT_PRETTY flag:");
    my_puts(buf);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "[\n  123,\n  123.456\n]");

    len_expected = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    memset(buf, 0, sizeof(buf));
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PRETTY |
            PCVRNT_SERIALIZE_OPT_PRETTY_TAB, &len_expected);
    my_puts("Serialized array with PCVRNT_SERIALIZE_OPT_PRETTY and PCVRNT_SERIALIZE_OPT_PRETTY_TAB flag:");
    my_puts(buf);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "[\n\t123,\n\t123.456\n]");

    purc_variant_unref(my_variant);
    purc_variant_unref(v1);
    purc_variant_unref(v2);

    purc_rwstream_destroy(my_rws);

    purc_cleanup ();
}

// to test: serialize an object
TEST(variant, serialize_object)
{
    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t v1 = purc_variant_make_number(123.0);
    purc_variant_t v2 = purc_variant_make_number(123.456);
    ASSERT_EQ(v1->refc, 1);
    ASSERT_EQ(v2->refc, 1);

    purc_variant_t my_variant =
        purc_variant_make_object_by_static_ckey(1, "v2", v2);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);
    ASSERT_EQ(v1->refc, 1);
    ASSERT_EQ(v2->refc, 2);
    ASSERT_EQ(my_variant->refc, 1);

    char buf[32];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);
    ASSERT_EQ(v1->refc, 1);
    ASSERT_EQ(v2->refc, 2);
    ASSERT_EQ(my_variant->refc, 1);

    buf[n] = 0;
    ASSERT_STREQ(buf, "{\"v2\":123.456}");

    purc_rwstream_seek(my_rws, 0, SEEK_SET);

    len_expected = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PRETTY, &len_expected);
    my_puts("Serialized object with PCVRNT_SERIALIZE_OPT_PRETTY flag:");
    my_puts(buf);
    ASSERT_GT(n, 0);
    ASSERT_EQ(v1->refc, 1);
    ASSERT_EQ(v2->refc, 2);
    ASSERT_EQ(my_variant->refc, 1);

    buf[n] = 0;
    ASSERT_STREQ(buf, "{\n  \"v2\":123.456\n}");

    len_expected = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    memset(buf, 0, sizeof(buf));
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PRETTY |
            PCVRNT_SERIALIZE_OPT_PRETTY_TAB, &len_expected);
    my_puts("Serialized object with PCVRNT_SERIALIZE_OPT_PRETTY and PCVRNT_SERIALIZE_OPT_PRETTY_TAB flag:");
    my_puts(buf);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "{\n\t\"v2\":123.456\n}");

    len_expected = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    memset(buf, 0, sizeof(buf));
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_SPACED, &len_expected);
    my_puts("Serialized array with PCVRNT_SERIALIZE_OPT_SPACED flag:");
    my_puts(buf);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "{ \"v2\": 123.456 }");

    purc_variant_unref(v1);
    purc_variant_unref(v2);
    ASSERT_EQ(v2->refc, 1);
    ASSERT_EQ(my_variant->refc, 1);
    purc_variant_unref(my_variant);

    purc_rwstream_destroy(my_rws);

    purc_cleanup ();
}

// to test: serialize an object with empty key str
TEST(variant, serialize_object_with_empty_key)
{
    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t v1 = purc_variant_make_number(123);
    ASSERT_EQ(v1->refc, 1);

    purc_variant_t my_variant =
        purc_variant_make_object_by_static_ckey(1, "", v1);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);
    ASSERT_EQ(v1->refc, 2);
    ASSERT_EQ(my_variant->refc, 1);

    char buf[32];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);
    ASSERT_EQ(v1->refc, 2);
    ASSERT_EQ(my_variant->refc, 1);

    buf[n] = 0;
    fprintf(stderr, "%ld[%s]\n", n, buf);
    ASSERT_STREQ(buf, "{\"\":123}");

    purc_variant_unref(v1);
    ASSERT_EQ(v1->refc, 1);
    ASSERT_EQ(my_variant->refc, 1);
    purc_variant_unref(my_variant);

    purc_rwstream_destroy(my_rws);

    purc_cleanup ();
}

// to test: serialize an object with empty key str
TEST(variant, serialize_object_with_empty_key2)
{
    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t v1 = purc_variant_make_number(123);
    ASSERT_EQ(v1->refc, 1);

    purc_variant_t my_variant =
        purc_variant_make_object_by_static_ckey(1, "", v1);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);
    ASSERT_EQ(v1->refc, 2);
    ASSERT_EQ(my_variant->refc, 1);

    char buf[32];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(my_variant, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);
    ASSERT_EQ(v1->refc, 2);
    ASSERT_EQ(my_variant->refc, 1);

    buf[n] = 0;
    fprintf(stderr, "%ld[%s]\n", n, buf);
    ASSERT_STREQ(buf, "{\"\":123}");

    ASSERT_EQ(my_variant->refc, 1);
    purc_variant_unref(my_variant);
    ASSERT_EQ(v1->refc, 1);
    purc_variant_unref(v1);

    purc_rwstream_destroy(my_rws);

    purc_cleanup ();
}
