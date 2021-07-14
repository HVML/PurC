#include "purc.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

// to test: serialize a null
TEST(variant, serialize_null)
{
    int ret = purc_init ("cn.fmsoft.hybridos.test", "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t my_null = purc_variant_make_null();
    ASSERT_NE(my_null, PURC_VARIANT_INVALID);

    char buf[8];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(my_null, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_EQ(len_expected, 4);
    ASSERT_EQ(n, 4);

    buf[4] = 0;
    ASSERT_STREQ(buf, "null");

    len_expected = 0;
    n = purc_variant_serialize(my_null, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_IGNORE_ERRORS, &len_expected);
    ASSERT_EQ(n, 3);
    ASSERT_EQ(len_expected, 4);

    buf[7] = 0;
    ASSERT_STREQ(buf, "nullnul");

    purc_cleanup ();
}

// to test: serialize an undefined
TEST(variant, serialize_undefined)
{
    int ret = purc_init ("cn.fmsoft.hybridos.test", "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t my_undefined = purc_variant_make_undefined();
    ASSERT_NE(my_undefined, PURC_VARIANT_INVALID);

    char buf[18];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(my_undefined, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GE(len_expected, 9);
    ASSERT_EQ(n, 9);

    buf[n] = 0;
    ASSERT_STREQ(buf, "undefined");

    len_expected = 0;
    n = purc_variant_serialize(my_undefined, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GE(len_expected, 9);
    ASSERT_EQ(n, -1);

    purc_cleanup ();
}

// to test: serialize a number
TEST(variant, serialize_number)
{
    int ret = purc_init ("cn.fmsoft.hybridos.test", "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t my_variant = purc_variant_make_number(123.0);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    char buf[32];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(my_variant, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "123");

    my_variant = purc_variant_make_number(123.456000000);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    purc_variant_unref(my_variant);

    purc_rwstream_seek(my_rws, 0, SEEK_SET);

    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_NOZERO, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "123.456");

    purc_cleanup ();
}

// to test: serialize an array
TEST(variant, serialize_array)
{
    int ret = purc_init ("cn.fmsoft.hybridos.test", "variant", NULL);
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
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    puts(buf);
    ASSERT_GT(n, 0);
    ASSERT_GT(len_expected, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "[123,123.456]");

    len_expected = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_NOZERO, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "[123,123.456]");

    len_expected = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PRETTY, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "[\n  123,\n  123.456\n]");

    len_expected = 0;
    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PRETTY |
            PCVARIANT_SERIALIZE_OPT_PRETTY_TAB, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "[\n\t123,\n\t123.456\n]");

    purc_variant_unref(my_variant);
    purc_variant_unref(v1);
    purc_variant_unref(v2);

    purc_cleanup ();
}

// to test: serialize an object
TEST(variant, serialize_object)
{
    int ret = purc_init ("cn.fmsoft.hybridos.test", "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t v1 = purc_variant_make_number(123.0);
    purc_variant_t v2 = purc_variant_make_number(123.456);

    purc_variant_t my_variant =
        purc_variant_make_object_c(2, "v1", v1, "v2", v2);
    ASSERT_NE(my_variant, PURC_VARIANT_INVALID);

    char buf[32];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(my_variant, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "{\"v1\":123,\"v2\":123.45600000000000000}");

    purc_rwstream_seek(my_rws, 0, SEEK_SET);

    len_expected = 0;
    n = purc_variant_serialize(my_variant, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_NOZERO, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "{\"v1\":123,\"v2\":123.456}");

    purc_variant_unref(my_variant);
    purc_variant_unref(v1);
    purc_variant_unref(v2);

    purc_cleanup ();
}

