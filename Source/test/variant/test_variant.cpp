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
#include "private/debug.h"
#include "private/errors.h"
#include "private/variant.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#ifndef MAX
#define MAX(a, b)   (a) > (b)? (a) : (b)
#endif

TEST(variant, pcvariant_init_once)
{
    purc_instance_extra_info info = {};
    int i = 0;
    size_t size = sizeof(purc_variant);
    int ret = 0;
    bool cleanup = false;

    // initial purc
    ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);

    ASSERT_EQ (ret, PURC_ERROR_OK);

    // get statitics information
    const struct purc_variant_stat * stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);

    EXPECT_EQ (stat->nr_values[PURC_VARIANT_TYPE_NULL], 0);
    EXPECT_EQ (stat->sz_mem[PURC_VARIANT_TYPE_NULL], size);

    EXPECT_EQ (stat->nr_values[PURC_VARIANT_TYPE_UNDEFINED], 0);
    EXPECT_EQ (stat->sz_mem[PURC_VARIANT_TYPE_UNDEFINED], size);

    EXPECT_EQ (stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN], 0);
    EXPECT_EQ (stat->sz_mem[PURC_VARIANT_TYPE_BOOLEAN], size * 2);

    for (i = PURC_VARIANT_TYPE_NUMBER; i < PURC_VARIANT_TYPE_NR; i++) {
        EXPECT_EQ (stat->nr_values[i], 0);
        EXPECT_EQ (stat->sz_mem[i], 0);
    }

    EXPECT_EQ (stat->nr_total_values, 4);
    EXPECT_EQ (stat->sz_total_mem, 4 * size);
    EXPECT_EQ (stat->nr_reserved, 0);
    EXPECT_EQ (stat->nr_max_reserved, MAX_RESERVED_VARIANTS);


    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(variant, pcvariant_init_10_times)
{
    purc_instance_extra_info info = {};
    int i = 0;
    size_t size = sizeof(purc_variant);
    int ret = 0;
    bool cleanup = false;
    int times = 0;

    for (times = 0; times < 10; times ++) {
        // initial purc
        ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);

        ASSERT_EQ (ret, PURC_ERROR_OK);

        // get statitics information
        const struct purc_variant_stat * stat = purc_variant_usage_stat ();

        ASSERT_NE(stat, nullptr);

        EXPECT_EQ (stat->nr_values[PURC_VARIANT_TYPE_NULL], 0);
        EXPECT_EQ (stat->sz_mem[PURC_VARIANT_TYPE_NULL], size);

        EXPECT_EQ (stat->nr_values[PURC_VARIANT_TYPE_UNDEFINED], 0);
        EXPECT_EQ (stat->sz_mem[PURC_VARIANT_TYPE_UNDEFINED], size);

        EXPECT_EQ (stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN], 0);
        EXPECT_EQ (stat->sz_mem[PURC_VARIANT_TYPE_BOOLEAN], size * 2);

        for (i = PURC_VARIANT_TYPE_NUMBER; i < PURC_VARIANT_TYPE_NR; i++) {
            EXPECT_EQ (stat->nr_values[i], 0);
            EXPECT_EQ (stat->sz_mem[i], 0);
        }

        EXPECT_EQ (stat->nr_total_values, 4);
        EXPECT_EQ (stat->sz_total_mem, 4 * size);
        EXPECT_EQ (stat->nr_reserved, 0);
        EXPECT_EQ (stat->nr_max_reserved, MAX_RESERVED_VARIANTS);

        cleanup = purc_cleanup ();
        ASSERT_EQ (cleanup, true);
    }
}

// to test: only one instance of null variant type.
// purc_variant_make_null
TEST(variant, pcvariant_null)
{
#if 0
    size_t size = sizeof(purc_variant);
    int times = 0;
    int module_times = 0;
    const struct purc_variant_stat * stat = NULL;
    purc_variant_t value = NULL;
    purc_variant_t value_prev = NULL;

    // init and deinit module for 10 times
    for (module_times = 0; module_times < 10; module_times++) {

        purc_instance_extra_info info = {};
        int ret = purc_init_ex ("cn.fmsfot.hvml.test", "variant", &info);
        ASSERT_EQ (ret, PURC_ERROR_OK);

        // get initial statitics information
        stat = purc_variant_usage_stat ();
        ASSERT_NE(stat, nullptr);

        size_t nr_values_before = stat->nr_values[PURC_VARIANT_TYPE_NULL];
        size_t sz_mem_before = stat->sz_mem[PURC_VARIANT_TYPE_NULL];
        size_t nr_total_values_before = stat->nr_total_values;
        size_t sz_total_mem_before = stat->sz_total_mem;
        EXPECT_EQ (nr_values_before, 1);
        EXPECT_EQ (sz_mem_before, size);
        EXPECT_EQ (nr_total_values_before, 4);
        EXPECT_EQ (sz_total_mem_before, 4 * size);

        // create constant variant for 5 times, check the statistics
        for (times = 0; times < 5; times++) {

            // create null variant
            value = purc_variant_make_null ();
            ASSERT_NE(value, nullptr);

            if (value_prev == NULL)
                value_prev = value;

            // all undefined purc_variant_t are same
            ASSERT_EQ (value, value_prev);

            // check ref
            ASSERT_EQ (value->refc, 1 + times);

            // check statitics information
            stat = purc_variant_usage_stat ();
            ASSERT_NE(stat, nullptr);

            EXPECT_EQ (nr_values_before, stat->nr_values[PURC_VARIANT_TYPE_NULL]);
            EXPECT_EQ (sz_mem_before, stat->sz_mem[PURC_VARIANT_TYPE_NULL]);
            EXPECT_EQ (nr_total_values_before, stat->nr_total_values);
            EXPECT_EQ (sz_total_mem_before, stat->sz_total_mem);
        }

        // invoke unref 6 times.
        // for null variant, the minimum refc is 1 or 0 ???
        unsigned int refc = 0;
        for (; times > 0; times--) {
            refc = purc_variant_unref (value);
            ASSERT_GE (refc, 0);
        }

        // check statitics information
        stat = purc_variant_usage_stat ();
        ASSERT_NE(stat, nullptr);

        EXPECT_EQ (nr_values_before, stat->nr_values[PURC_VARIANT_TYPE_NULL]);
        EXPECT_EQ (sz_mem_before, stat->sz_mem[PURC_VARIANT_TYPE_NULL]);
        EXPECT_EQ (nr_total_values_before, stat->nr_total_values);
        EXPECT_EQ (sz_total_mem_before, stat->sz_total_mem);

        purc_cleanup ();

        value = NULL;
        value_prev = NULL;
    }
#endif // 0
}


// to test: only one instance of undefined variant type.
// purc_variant_make_undefined
TEST(variant, pcvariant_undefined)
{
#if 0
    size_t size = sizeof(purc_variant);
    int times = 0;
    int module_times = 0;
    const struct purc_variant_stat * stat = NULL;
    purc_variant_t value = NULL;
    purc_variant_t value_prev = NULL;

    // init and deinit module for 10 times
    for (module_times = 0; module_times < 10; module_times++) {

        purc_instance_extra_info info = {};
        int ret = purc_init_ex ("cn.fmsfot.hvml.test", "variant", &info);
        ASSERT_EQ (ret, PURC_ERROR_OK);

        // get initial statitics information
        stat = purc_variant_usage_stat ();
        ASSERT_NE(stat, nullptr);

        size_t nr_values_before = stat->nr_values[PURC_VARIANT_TYPE_UNDEFINED];
        size_t sz_mem_before = stat->sz_mem[PURC_VARIANT_TYPE_UNDEFINED];
        size_t nr_total_values_before = stat->nr_total_values;
        size_t sz_total_mem_before = stat->sz_total_mem;
        EXPECT_EQ (nr_values_before, 1);
        EXPECT_EQ (sz_mem_before, size);
        EXPECT_EQ (nr_total_values_before, 4);
        EXPECT_EQ (sz_total_mem_before, 4 * size);

        // create constant variant for 5 times, check the statistics
        for (times = 0; times < 5; times++) {

            // create undefined variant
            value = purc_variant_make_undefined ();
            ASSERT_NE(value, nullptr);

            if (value_prev == NULL)
                value_prev = value;

            // all undefined purc_variant_t are same
            ASSERT_EQ (value, value_prev);

            // check ref
            ASSERT_EQ (value->refc, 1 + times);

            // check statitics information
            stat = purc_variant_usage_stat ();
            ASSERT_NE(stat, nullptr);

            EXPECT_EQ (nr_values_before, stat->nr_values[PURC_VARIANT_TYPE_UNDEFINED]);
            EXPECT_EQ (sz_mem_before, stat->sz_mem[PURC_VARIANT_TYPE_UNDEFINED]);
            EXPECT_EQ (nr_total_values_before, stat->nr_total_values);
            EXPECT_EQ (sz_total_mem_before, stat->sz_total_mem);
        }

        // invoke unref 6 times.
        // for undefined variant, the minimum refc is 1 or 0 ???
        unsigned int refc = 0;
        for (; times > 0; times--) {
            refc = purc_variant_unref (value);
            ASSERT_GE (refc, 0);
        }

        // check statitics information
        stat = purc_variant_usage_stat ();
        ASSERT_NE(stat, nullptr);

        EXPECT_EQ (nr_values_before, stat->nr_values[PURC_VARIANT_TYPE_UNDEFINED]);
        EXPECT_EQ (sz_mem_before, stat->sz_mem[PURC_VARIANT_TYPE_UNDEFINED]);
        EXPECT_EQ (nr_total_values_before, stat->nr_total_values);
        EXPECT_EQ (sz_total_mem_before, stat->sz_total_mem);

        purc_cleanup ();

        value = NULL;
        value_prev = NULL;
    }
#endif
}

// to test: only one true and one false variant instance, but they are calculated
//          in one field of const struct purc_variant_stat
// purc_variant_make_undefined
TEST(variant, pcvariant_boolean)
{
#if 0
    size_t size = sizeof(purc_variant);
    int times = 0;
    int module_times = 0;
    const struct purc_variant_stat * stat = NULL;
    purc_variant_t value_true = NULL;
    purc_variant_t value_true_prev = NULL;
    purc_variant_t value_false = NULL;
    purc_variant_t value_false_prev = NULL;

    // init and deinit module for 10 times
    for (module_times = 0; module_times < 10; module_times++) {

        purc_instance_extra_info info = {};
        int ret = purc_init_ex ("cn.fmsfot.hvml.test", "variant", &info);
        ASSERT_EQ (ret, PURC_ERROR_OK);

        // get initial statitics information
        stat = purc_variant_usage_stat ();
        ASSERT_NE(stat, nullptr);

        size_t nr_values_before = stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN];
        size_t sz_mem_before = stat->sz_mem[PURC_VARIANT_TYPE_BOOLEAN];
        size_t nr_total_values_before = stat->nr_total_values;
        size_t sz_total_mem_before = stat->sz_total_mem;
        EXPECT_EQ (nr_values_before, 2);
        EXPECT_EQ (sz_mem_before, 2 * size);
        EXPECT_EQ (nr_total_values_before, 4);
        EXPECT_EQ (sz_total_mem_before, 4 * size);

        // create constant variant for 5 times, check the statistics
        for (times = 0; times < 5; times++) {

            // create true variant
            value_true = purc_variant_make_boolean (true);
            ASSERT_NE(value_true, nullptr);

            if (value_true_prev == NULL)
                value_true_prev = value_true;

            // all undefined purc_variant_t are same
            ASSERT_EQ (value_true, value_true_prev);

            // check ref
            ASSERT_EQ (value_true->refc, 1 + times);

            // check statitics information
            stat = purc_variant_usage_stat ();
            ASSERT_NE(stat, nullptr);

            EXPECT_EQ (nr_values_before, stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN]);
            EXPECT_EQ (sz_mem_before, stat->sz_mem[PURC_VARIANT_TYPE_BOOLEAN]);
            EXPECT_EQ (nr_total_values_before, stat->nr_total_values);
            EXPECT_EQ (sz_total_mem_before, stat->sz_total_mem);


            // create false variant
            value_false = purc_variant_make_boolean (false);
            ASSERT_NE(value_false, nullptr);

            if (value_false_prev == NULL)
                value_false_prev = value_false;

            // all undefined purc_variant_t are same
            ASSERT_EQ (value_false, value_false_prev);

            // check ref
            ASSERT_EQ (value_false->refc, 1 + times);

            // check statitics information
            stat = purc_variant_usage_stat ();
            ASSERT_NE(stat, nullptr);

            EXPECT_EQ (nr_values_before, stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN]);
            EXPECT_EQ (sz_mem_before, stat->sz_mem[PURC_VARIANT_TYPE_BOOLEAN]);
            EXPECT_EQ (nr_total_values_before, stat->nr_total_values);
            EXPECT_EQ (sz_total_mem_before, stat->sz_total_mem);
        }

        // invoke unref 6 times.
        // for boolean variant, the minimum refc is 2 or 0 ???
        unsigned int refc = 0;
        for (; times > 0; times--) {
            refc = purc_variant_unref (value_true);
            ASSERT_GE (refc, 0);
        }

        for (times = 5; times > 0; times--) {
            refc = purc_variant_unref (value_false);
            ASSERT_GE (refc, 0);
        }

        // check statitics information
        stat = purc_variant_usage_stat ();
        ASSERT_NE(stat, nullptr);

        EXPECT_EQ (nr_values_before, stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN]);
        EXPECT_EQ (sz_mem_before, stat->sz_mem[PURC_VARIANT_TYPE_BOOLEAN]);
        EXPECT_EQ (nr_total_values_before, stat->nr_total_values);
        EXPECT_EQ (sz_total_mem_before, stat->sz_total_mem);

        purc_cleanup ();

        value_true = NULL;
        value_true_prev = NULL;
        value_false = NULL;
        value_false_prev = NULL;
    }
#endif // 0
}


TEST(variant, pcvariant_exception)
{
    purc_atom_t atom = 0;
    purc_variant_t value = NULL;
    purc_instance_extra_info info = {};

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    atom = purc_get_except_atom_by_id(PURC_EXCEPT_BAD_ENCODING);
    value = purc_variant_make_exception (atom);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    ASSERT_STREQ ("BadEncoding", purc_variant_get_exception_string_const (value));
    purc_variant_unref(value);

    atom = purc_atom_from_string("TestString");
    ASSERT_NE (atom, 0);
    value = purc_variant_make_exception (atom);
    ASSERT_EQ(value, PURC_VARIANT_INVALID);
    ASSERT_EQ(purc_get_last_error(), PURC_ERROR_INVALID_VALUE);


    purc_cleanup ();
}

// to test:
// purc_variant_make_number ()
// purc_variant_serialize ()
TEST(variant, pcvariant_number)
{
    purc_variant_t value = NULL;
    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create with double
    // serialize option ???
    double number = 123.4560000;
    value = purc_variant_make_number (number);
    ASSERT_NE(value, PURC_VARIANT_INVALID);

    // serialize
    char buf[32];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(value, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_NOZERO, &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "123.456");

    purc_variant_unref(value);
    purc_rwstream_destroy(my_rws);

    purc_cleanup ();
}

// to test:
// purc_variant_make_longuint ()
// purc_variant_serialize ()
TEST(variant, pcvariant_ulongint)
{
    purc_variant_t value = NULL;
    purc_instance_extra_info info = {};

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create longuint variant with valild value, and serialize
    // expected: get the variant
    uint64_t number = 0xFFFFFFFFFFFFFFFF;
    value = purc_variant_make_ulongint (number);
    ASSERT_NE(value, PURC_VARIANT_INVALID);

    // serialize
    char buf[128];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(value, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_REAL_EJSON | PCVARIANT_SERIALIZE_OPT_NOZERO,
            &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;

    char buffer [256];
    snprintf (buffer, sizeof(buffer), "%lluUL", (unsigned long long)number);

    ASSERT_STREQ(buffer, buf);
    purc_variant_unref(value);

    // create longuint variant with negatives, and serialize
    int64_t negative = 0xFFFFFFFFFFFFFFFF;
    value = purc_variant_make_ulongint (negative);
    ASSERT_NE(value, PURC_VARIANT_INVALID);

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(value, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_REAL_EJSON | PCVARIANT_SERIALIZE_OPT_NOZERO,
            &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buffer, buf);

    purc_variant_unref(value);
    purc_rwstream_destroy(my_rws);

    purc_cleanup ();
}

// to test:
// purc_variant_make_longint ()
// purc_variant_serialize ()
TEST(variant, pcvariant_longint)
{
    purc_variant_t value = NULL;
    purc_instance_extra_info info = {};

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create longint variant with valild value, and serialize
    // expected: get the variant
    int64_t number = 0x7FFFFFFFFFFFFFFF;
    value = purc_variant_make_longint (number);
    ASSERT_NE(value, PURC_VARIANT_INVALID);

    // serialize
    char buf[128];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(value, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_REAL_EJSON | PCVARIANT_SERIALIZE_OPT_NOZERO,
            &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;

    char buffer [256];
    snprintf (buffer, sizeof(buffer), "%lldL", (long long)number);

    ASSERT_STREQ(buffer, buf);
    purc_variant_unref(value);


    // create longuint variant with negatives, and serialize
    uint64_t positive = 0xFFFFFFFFFFFFFFFF;
    value = purc_variant_make_ulongint (positive);
    ASSERT_NE(value, PURC_VARIANT_INVALID);

    purc_rwstream_seek(my_rws, 0, SEEK_SET);
    len_expected = 0;
    n = purc_variant_serialize(value, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_REAL_EJSON | PCVARIANT_SERIALIZE_OPT_NOZERO,
            &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    snprintf (buffer, sizeof(buffer), "%lluUL", (unsigned long long)positive);
    ASSERT_STREQ(buffer, buf);

    purc_variant_unref(value);
    purc_rwstream_destroy(my_rws);

    purc_cleanup ();
}

// to test:
// purc_variant_make_longdouble ()
// purc_variant_serialize ()
TEST(variant, pcvariant_longdouble)
{
    purc_variant_t value = NULL;
    purc_instance_extra_info info = {};

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create longdouble variant with valild value, and serialize
    // expected: get the variant
    long double number = 123.4560000;
    value = purc_variant_make_number (number);
    ASSERT_NE(value, PURC_VARIANT_INVALID);

    // serialize
    char buf[32];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(value, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_REAL_EJSON | PCVARIANT_SERIALIZE_OPT_NOZERO,
            &len_expected);
    ASSERT_GT(n, 0);

    buf[n] = 0;
    ASSERT_STREQ(buf, "123.456");

    purc_variant_unref(value);
    purc_rwstream_destroy(my_rws);

    purc_cleanup ();
}

// to test:
// purc_variant_make_string ();
// purc_variant_get_string_const ();
// purc_variant_string_size ();
// purc_variant_serialize ()
TEST(variant, pcvariant_string)
{
    purc_variant_t value = NULL;
    purc_instance_extra_info info = {};

    const char *short_ok = "\x61\x62\xE5\x8C\x97\xE4\xBA\xAC\xE4\xB8\x8A\xE6\xB5\xB7\x00";   // ab北京上海
    const char *short_err = "\x61\x62\xE5\x02\x97\xE4\xBA\xAC\xE4\xB8\x8A\xE6\xB5\xB7\x00";   // ab
    const char *long_ok = "\x61\x62\xE5\x8C\x97\xE4\xBA\xAC\xE4\xB8\x8A\xE6\xB5\xB7\xE5\x8C\x97\xE4\xBA\xAC\xE4\xB8\x8A\xE6\xB5\xB7\x00";   // ab北京上海北京上海
    const char *long_err = "\x61\x62\xE5\x02\x97\xE4\xBA\xAC\xE4\xB8\x8A\xE6\xB5\xB7\xE5\x8C\x97\xE4\xBA\xAC\xE4\xB8\x8A\xE6\xB5\xB7\x00";   // ab
    size_t length = 0;
    size_t real_size = MAX (sizeof(long double), sizeof(void*) * 2);
    size_t nr_chars = 0;

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create short string variant without checking, input in utf8-encoding
    // expected: get the variant with original string
    value = purc_variant_make_string (short_ok, false);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    length = purc_variant_string_size (value);
    ASSERT_EQ (length, strlen(purc_variant_get_string_const (value)) + 1);
    ASSERT_LT (length, real_size);

    purc_variant_string_chars (value, &nr_chars);
    ASSERT_EQ (nr_chars, 6);
    purc_variant_unref(value);

    // create short string variant without checking, input not in utf8-encoding
    // expected: get the variant with original string
    value = purc_variant_make_string (short_err, false);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    length = purc_variant_string_size (value);
    ASSERT_EQ (length, strlen(purc_variant_get_string_const (value)) + 1);
    ASSERT_LT (length, real_size);

    purc_variant_string_chars (value, &nr_chars);
    ASSERT_EQ (nr_chars, 2);
    purc_variant_unref(value);

    // create short string variant with checking, input in utf8-encoding
    // expected: get the variant with original string
    value = purc_variant_make_string (short_ok, true);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    length = purc_variant_string_size (value);
    ASSERT_EQ (length, strlen(purc_variant_get_string_const (value)) + 1);
    ASSERT_LT (length, real_size);

    purc_variant_string_chars (value, &nr_chars);
    ASSERT_EQ (nr_chars, 6);
    purc_variant_unref(value);

    // create short string variant with checking, input is not in utf8-encoding
    // expected: get PURC_VARIANT_INVALID
    value = purc_variant_make_string (short_err, true);
    ASSERT_EQ(value, PURC_VARIANT_INVALID);

    // create long string variant without checking, input in utf8-encoding
    // expected: get the variant with original string
    value = purc_variant_make_string (long_ok, false);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    length = purc_variant_string_size (value);
    ASSERT_EQ (length, strlen(purc_variant_get_string_const (value)) + 1);
    ASSERT_GT (length, real_size);

    purc_variant_string_chars (value, &nr_chars);
    ASSERT_EQ (nr_chars, 10);
    purc_variant_unref(value);

    // create long string variant without checking, input not in utf8-encoding
    // expected: get the variant with original string
    value = purc_variant_make_string (long_err, false);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    length = purc_variant_string_size (value);
    ASSERT_EQ (length, strlen(purc_variant_get_string_const (value)) + 1);
    // ASSERT_GT (length, real_size);

    purc_variant_string_chars (value, &nr_chars);
    ASSERT_EQ (nr_chars, 2);
    purc_variant_unref(value);

    // create long string variant with checking, input in utf8-encoding
    // expected: get the variant with original string
    value = purc_variant_make_string (long_ok, true);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    length = purc_variant_string_size (value);
    ASSERT_EQ (length, strlen(purc_variant_get_string_const (value)) + 1);
    // ASSERT_GT (length, real_size);

    purc_variant_string_chars (value, &nr_chars);
    ASSERT_EQ (nr_chars, 10);
    purc_variant_unref(value);

    // create long string variant with checking, input not in utf8-encoding
    // expected: get PURC_VARIANT_INVALID
    value = purc_variant_make_string (long_err, true);
    ASSERT_EQ(value, PURC_VARIANT_INVALID);

    // create variant from a static string without checking
    const char *static_str = "PURC";
    value = purc_variant_make_string_static (static_str, false);
    const char* tmp = purc_variant_get_string_const (value);
    ASSERT_EQ (strcmp(tmp, static_str), 0);

    purc_variant_string_chars (value, &nr_chars);
    ASSERT_EQ (nr_chars, 4);
    purc_variant_unref(value);

    // create short string variant with null pointer
    value = purc_variant_make_string (NULL, true);
    ASSERT_EQ(value, PURC_VARIANT_INVALID);

    purc_cleanup ();
}


// to test:
// purc_variant_make_atom_string ();
// purc_variant_make_atom_string_static ();
// purc_variant_get_atom_string_const ();
// purc_variant_serialize ()
TEST(variant, pcvariant_atom_string)
{
    purc_variant_t value = NULL;
    purc_variant_t dup = NULL;
    purc_instance_extra_info info = {};

    const char *string_ok = "\x61\x62\xE5\x8C\x97\xE4\xBA\xAC\xE4\xB8\x8A\xE6\xB5\xB7\xE5\x8C\x97\xE4\xBA\xAC\xE4\xB8\x8A\xE6\xB5\xB7\x00";   // ab北京上海北京上海
    const char *string_err = "\x61\x62\xE5\x02\x97\xE4\xBA\xAC\xE4\xB8\x8A\xE6\xB5\xB7\xE5\x8C\x97\xE4\xBA\xAC\xE4\xB8\x8A\xE6\xB5\xB7\x00";   // ab北京上海北京上海
    const char *string_test = "\xE5\x8C\x97\xE4\xBA\xAC\xE4\xB8\x8A\xE6\xB5\xB7\xE5\x8C\x97\xE4\xBA\xAC\xE4\xB8\x8A\xE6\xB5\xB7\x00";   // ab北京上海北京上海

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);


    // create atom string variant without checking, input in utf8-encoding,
    // expected: get the variant with string.
    value = purc_variant_make_atom_string (string_ok, false);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    ASSERT_STREQ (string_ok, purc_variant_get_atom_string_const (value));
    purc_variant_unref(value);


    // create atom string variant without checking, input not in utf8-encoding
    // expected: get the variant with string.
    value = purc_variant_make_atom_string (string_err, false);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    ASSERT_STREQ (string_err, purc_variant_get_atom_string_const (value));
    purc_variant_unref(value);


    // create atom string variant with checking, input in utf8-encoding
    // expected: get the variant with string.
    value = purc_variant_make_atom_string (string_ok, true);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    ASSERT_STREQ (string_ok, purc_variant_get_atom_string_const (value));
    purc_variant_unref(value);


    // create atom string variant with checking, input is not in utf8-encoding
    // expected: get PURC_VARIANT_INVALID
    value = purc_variant_make_atom_string (string_err, true);
    ASSERT_EQ(value, PURC_VARIANT_INVALID);

    // create static atom string variant without checking, input in utf8-encoding
    // expected: get the variant with string.
    value = purc_variant_make_atom_string_static (string_ok, false);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    ASSERT_STREQ (string_ok, purc_variant_get_atom_string_const (value));
    purc_variant_unref(value);


    // create static atom string variant without checking, input not in utf8-encoding
    // expected: get the variant with string.
    value = purc_variant_make_atom_string_static (string_err, false);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    ASSERT_STREQ (string_err, purc_variant_get_atom_string_const (value));
    purc_variant_unref(value);


    // create static atom string variant with checking, input in utf8-encoding
    // expected: get the variant with string.
    value = purc_variant_make_atom_string_static (string_ok, true);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    ASSERT_STREQ (string_ok, purc_variant_get_atom_string_const (value));
    purc_variant_unref(value);



    // create static atom string variant with checking, input not in utf8-encoding
    // expected: get PURC_VARIANT_INVALID
    value = purc_variant_make_atom_string_static (string_err, true);
    ASSERT_EQ(value, PURC_VARIANT_INVALID);

    // create two atom string variants with same input string, check the atom 
    //        and string pointer whether are equal 
    // expected: atom and string pointer is same. 
    value = purc_variant_make_atom_string (string_ok, true);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    const char * value_str = purc_variant_get_atom_string_const (value);
    ASSERT_NE (string_ok, value_str);           // string pointers are different

    dup = purc_variant_make_atom_string (string_ok, true);
    ASSERT_NE(dup, PURC_VARIANT_INVALID);
    const char * dup_str = purc_variant_get_atom_string_const (value);
    ASSERT_NE (string_ok, dup_str);             // string pointers are different

    ASSERT_EQ(dup->atom, value->atom);        // atoms are same
    ASSERT_STREQ(value_str, dup_str);                   // strings are same
    purc_variant_unref(value);
    purc_variant_unref(dup);

    // create two static atom string variants with same input string, check the atom
    //        and string pointer
    // expected: atom and string pointer is same. 
    value = purc_variant_make_atom_string_static (string_test, true);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    value_str = purc_variant_get_atom_string_const (value);
    ASSERT_EQ (string_test, value_str);           // string pointers are different
    purc_variant_unref(value);


    // create atom string variant with null pointer
    value = purc_variant_make_atom_string (NULL, true);
    ASSERT_EQ(value, PURC_VARIANT_INVALID);

    value = purc_variant_make_atom_string_static (NULL, true);
    ASSERT_EQ(value, PURC_VARIANT_INVALID);

    purc_cleanup ();
}

// to test:
// purc_variant_make_byte_sequence ();
// purc_variant_get_bytes_const ();
// purc_variant_bsequence_length ();
// purc_variant_serialize ()
TEST(variant, pcvariant_sequence)
{
    purc_variant_t value = NULL;
    purc_instance_extra_info info = {};
    size_t length = 0;
    size_t real_size = MAX (sizeof(long double), sizeof(void*) * 2);

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    const unsigned char short_bytes[] = "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F";
    const unsigned char long_bytes[] = "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x0F\x0E\x0D\x0C\x0B\x0A\x09\x08\x07\x06\x05\x04\x03\x02\x01";
    
    // create short sequence variant
    // expected: get the variant with original byte sequence
    value = purc_variant_make_byte_sequence (short_bytes, 15);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    length = purc_variant_bsequence_length (value);
    ASSERT_LT (length, real_size);
    ASSERT_EQ (length, 15);
    purc_variant_unref(value);


    // create long sequence variant
    // expected: get the variant with original string
    value = purc_variant_make_byte_sequence (long_bytes, 30);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    length = purc_variant_bsequence_length (value);
    ASSERT_GT (length, real_size);
    ASSERT_EQ (length, 30);
    purc_variant_unref(value);


    // create sequence variant with null pointer, 0 size
    // expected: return PURC_VARIANT_INVALID. 
    value = purc_variant_make_byte_sequence (NULL, 15);
    ASSERT_EQ(value, PURC_VARIANT_INVALID);

    value = purc_variant_make_byte_sequence (NULL, 0);
    ASSERT_EQ(value, PURC_VARIANT_INVALID);

    value = purc_variant_make_byte_sequence (short_bytes, 0);
    ASSERT_EQ(value, PURC_VARIANT_INVALID);

    purc_cleanup ();
}


// to test:
// purc_variant_make_dynamic_value ();
// purc_variant_serialize ()
purc_variant_t getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    purc_variant_t value = purc_variant_make_number (3.1415926);
    return value;
}

purc_variant_t setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    purc_variant_t value = purc_variant_make_number (2.71828828);
    return value;
}

TEST(variant, pcvariant_dynamic)
{
    purc_variant_t value = NULL;
    purc_instance_extra_info info = {};

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create dynamic variant with valid pointer
    // expected: get the dynamic variant with valid pointer
    value = purc_variant_make_dynamic (getter, setter);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    ASSERT_EQ(value->ptr_ptr[0], getter);
    ASSERT_EQ(value->ptr_ptr[1], setter);
    purc_variant_unref(value);

    // create dynamic variant with setting getter pointer to null
    value = purc_variant_make_dynamic (NULL, setter);
    ASSERT_EQ(value, PURC_VARIANT_INVALID);

    // create dynamic variant with setting setter pointer to null
    value = purc_variant_make_dynamic (getter, NULL);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    purc_variant_unref(value);

    purc_cleanup ();
}

// to test:
// purc_variant_make_native ();
// purc_variant_serialize ()
static void rws_releaser (void* entity)
{
    UNUSED_PARAM(entity);
}

static struct purc_native_ops _rws_ops = {
    .property_getter       = NULL,
    .property_setter       = NULL,
    .property_cleaner      = NULL,
    .property_eraser       = NULL,

    .updater               = NULL,
    .cleaner               = NULL,
    .eraser                = NULL,
    .match_observe         = NULL,

    .on_observe           = NULL,
    .on_forget            = NULL,
    .on_release           = rws_releaser,
};

TEST(variant, pcvariant_native)
{
    purc_variant_t value = NULL;
    purc_instance_extra_info info = {};

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create native variant with valid pointer
    // expected: get the native variant with valid pointer
    char buf[32];
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);

    value = purc_variant_make_native (my_rws, &_rws_ops);
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    purc_variant_unref(value);

    // create native variant with native_entity = NULL
    // expected: return PURC_VARIANT_INVALID 
    value = purc_variant_make_native (NULL, &_rws_ops);
    ASSERT_EQ(value, PURC_VARIANT_INVALID);

    // create native variant with valid native_entity and releaser = NULL
    // expected: get native variant with releaser = NULL ???
    value = purc_variant_make_native (my_rws, NULL);
    ASSERT_NE(value, PURC_VARIANT_INVALID);

    purc_variant_unref(value);
    purc_rwstream_destroy(my_rws);

    purc_cleanup ();
}

// to test:
// loop buffer in heap 
TEST(variant, pcvariant_loopbuffer_one)
{
    int times = 0;
    purc_variant_t value = NULL;
    purc_instance_extra_info info = {};
    const char long_str[] = "\x61\x62\xE5\x8C\x97\xE4\xBA\xAC\xE4\xB8\x8A\xE6\xB5\xB7\xE5\x8C\x97\xE4\xBA\xAC\xE4\xB8\x8A\xE6\xB5\xB7\x00";   // ab北京上海北京上海
    size_t old_size = 0;
    size_t new_size = 0;
    size_t block = sizeof(purc_variant);

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // get statitics information
    const struct purc_variant_stat * stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);
    old_size = stat->sz_total_mem;

    // create one variant value, and release it. check totol_mem
    // expect : initial value + sizeof(purc_variant)
    // note: only use one resered memory space
    for (times = 0; times < MAX_RESERVED_VARIANTS - 1; times++) { 
        // create variant
        value = purc_variant_make_string (long_str, true);

        // get the total memory
        stat = purc_variant_usage_stat ();
        ASSERT_NE(stat, nullptr);
        new_size = stat->sz_total_mem;

        ASSERT_EQ (new_size, old_size + block + purc_variant_string_size (value));

        // unref
        purc_variant_unref (value);

        // get the total memory
        stat = purc_variant_usage_stat ();
        ASSERT_NE(stat, nullptr);
        new_size = stat->sz_total_mem;

        ASSERT_EQ (new_size, old_size + block);
    }

    purc_cleanup ();
}


// to test:
// loop buffer in heap 
TEST(variant, pcvariant_loopbuffer_all)
{
    if (1) return;
    int i = 0;
    purc_variant_t value[MAX_RESERVED_VARIANTS];
    purc_instance_extra_info info = {};
    const char long_str[] = "\x61\x62\xE5\x8C\x97\xE4\xBA\xAC\xE4\xB8\x8A\xE6\xB5\xB7\xE5\x8C\x97\xE4\xBA\xAC\xE4\xB8\x8A\xE6\xB5\xB7\x00";   // ab北京上海北京上海
    size_t old_size = 0;
    size_t calc_size = 0;
    size_t new_size = 0;
    size_t block = sizeof(purc_variant);

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // get statitics information
    const struct purc_variant_stat * stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);
    old_size = stat->sz_total_mem;

    // create MAX_RESERVED_VARIANTS variant values, and release it. check totol_mem
    // expect : initial value + sizeof(purc_variant) * (MAX_RESERVED_VARIANTS - 1)
    // note: only (MAX_RESERVED_VARIANTS - 1) spaces can be used
    for (i = 0; i < MAX_RESERVED_VARIANTS; i++) { 
        // create variant
        value[i] = purc_variant_make_string (long_str, true);

        // get the total memory
        stat = purc_variant_usage_stat ();
        ASSERT_NE(stat, nullptr);
        new_size = stat->sz_total_mem;
        calc_size += block + purc_variant_string_size (value[i]);

        ASSERT_EQ (new_size, old_size + calc_size);
    }

    // release all variant, the loop buufers are not released
    for (i = 0; i < MAX_RESERVED_VARIANTS - 1; i++) { 
        calc_size -= purc_variant_string_size (value[i]);
        purc_variant_unref (value[i]);

        // get the total memory
        stat = purc_variant_usage_stat ();
        ASSERT_NE(stat, nullptr);
        new_size = stat->sz_total_mem;

        ASSERT_EQ (new_size, old_size + calc_size);

    }

    calc_size -= purc_variant_string_size (value[MAX_RESERVED_VARIANTS - 1]);
    purc_variant_unref (value[MAX_RESERVED_VARIANTS - 1]);
    stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);
    new_size = stat->sz_total_mem;
    calc_size -= block;

    ASSERT_EQ (new_size, old_size + calc_size);

    // buffers are full, create MAX_RESERVED_VARIANTS variant values again
    for (i = 0; i < MAX_RESERVED_VARIANTS - 1; i++) { 
        // create variant
        value[i] = purc_variant_make_string (long_str, true);

        // get the total memory
        stat = purc_variant_usage_stat ();
        ASSERT_NE(stat, nullptr);
        new_size = stat->sz_total_mem;
        calc_size += purc_variant_string_size (value[i]);

        ASSERT_EQ (new_size, old_size + calc_size);
    }

    // create another variant
    value[MAX_RESERVED_VARIANTS -1] = purc_variant_make_string (long_str, true);
    stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);
    new_size = stat->sz_total_mem;
    calc_size += purc_variant_string_size (value[i]);

    ASSERT_EQ (new_size, old_size + calc_size + block);



    purc_cleanup ();
}

static inline purc_variant_t
_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    abort();
    return PURC_VARIANT_INVALID;
}

static void
_nr_native_releaser(void* entity)
{
    size_t nr = *(size_t*)entity;
    if (nr!=1)
        abort();
}

static struct purc_native_ops _nr_ops = {
    .property_getter       = NULL,
    .property_setter       = NULL,
    .property_cleaner      = NULL,
    .property_eraser       = NULL,

    .updater               = NULL,
    .cleaner               = NULL,
    .eraser                = NULL,
    .match_observe         = NULL,

    .on_observe           = NULL,
    .on_forget            = NULL,
    .on_release           = _nr_native_releaser,
};


TEST(variant, api_edge_case_bad_arg)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    const char *utf8 = "我们";

    purc_variant_t v;
    v = purc_variant_make_string(NULL, false);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);

    v = purc_variant_make_string(utf8, false);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    v = purc_variant_make_string(utf8, true);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    // this is not a strict utf8-checker yet
    // int pos = 2;
    // char c = utf8[pos];
    // utf8[pos] = '\x0';
    // v = purc_variant_make_string(utf8, true);
    // ASSERT_EQ(v, PURC_VARIANT_INVALID);
    // utf8[pos] = c;

    const char* s;
    s = purc_variant_get_string_const(PURC_VARIANT_INVALID);
    ASSERT_EQ(s, nullptr);

    v = purc_variant_make_number(1.0);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    s = purc_variant_get_string_const(v);
    ASSERT_EQ(s, nullptr);
    purc_variant_unref(v);

    v = purc_variant_make_atom_string(NULL, false);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);

    v = purc_variant_make_atom_string(utf8, false);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    v = purc_variant_make_atom_string(utf8, true);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    v = purc_variant_make_atom_string_static(NULL, false);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);

    v = purc_variant_make_atom_string_static(utf8, false);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    v = purc_variant_make_atom_string_static(utf8, true);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    s = purc_variant_get_atom_string_const(PURC_VARIANT_INVALID);
    ASSERT_EQ(s, nullptr);

    v = purc_variant_make_number(1.0);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    s = purc_variant_get_atom_string_const(v);
    ASSERT_EQ(s, nullptr);
    purc_variant_unref(v);

    v = purc_variant_make_byte_sequence(NULL, 0);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);

    v = purc_variant_make_byte_sequence(utf8, 0);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);

    v = purc_variant_make_byte_sequence(utf8, 1);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    const unsigned char *bytes;
    size_t nr;
    bytes = purc_variant_get_bytes_const(PURC_VARIANT_INVALID, NULL);
    ASSERT_EQ(bytes, nullptr);

    v = purc_variant_make_number(1.0);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    bytes = purc_variant_get_bytes_const(v, NULL);
    ASSERT_EQ(bytes, nullptr);
    bytes = purc_variant_get_bytes_const(v, &nr);
    ASSERT_EQ(bytes, nullptr);
    purc_variant_unref(v);

    v = purc_variant_make_dynamic(NULL, NULL);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);
    v = purc_variant_make_dynamic(_getter, NULL);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);
    v = purc_variant_make_dynamic(_getter, _getter);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    v = purc_variant_make_native(NULL, NULL);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);
    v = purc_variant_make_native((void*)1, NULL);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);
    nr = 1;
    v = purc_variant_make_native(&nr, &_nr_ops);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    purc_cleanup ();
}

TEST(variant, four_constants)
{
    if (1) return;
    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    const struct purc_variant_stat * stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_UNDEFINED], 0);
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_NULL], 0);
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN], 0);

    purc_variant_t v;

    // create first undefined variant
    v = purc_variant_make_undefined();
    ASSERT_NE(v, PURC_VARIANT_INVALID);

    // create second undefined variant
    v = purc_variant_make_undefined();
    ASSERT_NE(v, PURC_VARIANT_INVALID);

    // create third undefined variant
    v = purc_variant_make_undefined();
    ASSERT_NE(v, PURC_VARIANT_INVALID);

    ASSERT_EQ(v->refc, 3);

    // get the reference times
    stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_UNDEFINED], 3);

    // it is const, unref 2 times
    purc_variant_unref(v);
    purc_variant_unref(v);
    stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_UNDEFINED], 1);



    // create first null variant
    v = purc_variant_make_null();
    ASSERT_NE(v, PURC_VARIANT_INVALID);

    // create second null variant
    v = purc_variant_make_null();
    ASSERT_NE(v, PURC_VARIANT_INVALID);

    // create third null variant
    v = purc_variant_make_null();
    ASSERT_NE(v, PURC_VARIANT_INVALID);

    ASSERT_EQ(v->refc, 3);

    // get the reference times
    stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_NULL], 3);

    // it is const, unref 3 times
    purc_variant_unref(v);
    purc_variant_unref(v);
    purc_variant_unref(v);
    stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_NULL], 0);



    // create first true variant
    v = purc_variant_make_boolean(true);
    ASSERT_NE(v, PURC_VARIANT_INVALID);

    // create second true variant
    v = purc_variant_make_boolean(true);
    ASSERT_NE(v, PURC_VARIANT_INVALID);

    ASSERT_EQ(v->refc, 2);

    // get the reference times
    stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN], 2);


    // create first false variant
    v = purc_variant_make_boolean(false);
    ASSERT_NE(v, PURC_VARIANT_INVALID);

    // create second false variant
    v = purc_variant_make_boolean(false);
    ASSERT_NE(v, PURC_VARIANT_INVALID);

    // it must be 2. ture and false variants are two different constant
    ASSERT_EQ(v->refc, 2);

    // get the reference times
    stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);
    // it must be 4, because true and flase variants are same type
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN], 4);

    // unref false variant 2 times
    purc_variant_unref(v);
    purc_variant_unref(v);
    ASSERT_EQ(v->refc, 0);

    // get the reference times
    stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);
    // it must be 2, because true and flase variants are same type
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN], 2);

    purc_cleanup ();
}

#if 0
TEST(variant, load_from_so)
{
    purc_variant_t value = NULL;
    purc_instance_extra_info info = {};

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    value = purc_variant_load_dvobj_from_so ("/usr/local/lib/purc-0.0/libpurc-dvobj-FS.so", "FS");
    ASSERT_NE(value, PURC_VARIANT_INVALID);
    ASSERT_EQ(purc_variant_is_type (value, PURC_VARIANT_TYPE_OBJECT), true);

    purc_variant_t val = purc_variant_object_get_by_ckey (value, "__intr_dlhandle", false);
    uint64_t u64 = 0;
    purc_variant_cast_to_ulongint (val, &u64, false);

    bool close = purc_variant_unload_dvobj (value);
    ASSERT_EQ(close, true);
    purc_cleanup ();
}
#endif

static void get_variant_total_info (size_t *mem, size_t *value, size_t *resv)
{
    const struct purc_variant_stat * stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);

    *mem = stat->sz_total_mem;
    *value = stat->nr_total_values;
    *resv = stat->nr_reserved;
}

TEST(variant, empty_object)
{
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_variant_t value = NULL;
    purc_instance_extra_info info = {};

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    value = purc_variant_make_object (0, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);
    purc_variant_t key = purc_variant_make_string ("beijing", false);
    purc_variant_t val = purc_variant_make_string ("guangzhou", false);
    purc_variant_object_set (value, key, val);
    purc_variant_unref (key);
    purc_variant_unref (val);
    purc_variant_unref (value);

    get_variant_total_info (&sz_total_mem_after, &sz_total_values_after,
            &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after
                - nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}


TEST(variant, variant_compare)
{
    purc_variant_t value1 = NULL;
    purc_variant_t value2 = NULL;
    int compare= 0;
    purc_instance_extra_info info = {};

    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test", "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    value1 = purc_variant_make_object (0, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);
    purc_variant_t key1 = purc_variant_make_string ("beijing", false);
    purc_variant_t val1 = purc_variant_make_string ("guangzhou", false);
    purc_variant_object_set (value1, key1, val1);

    value2 = purc_variant_make_object (0, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);
    purc_variant_t key2 = purc_variant_make_string ("beijing", false);
    purc_variant_t val2 = purc_variant_make_string ("guangzhou", false);
    purc_variant_object_set (value2, key2, val2);

    compare = purc_variant_compare_ex (value1, value2, PCVARIANT_COMPARE_OPT_CASELESS);
    ASSERT_EQ(compare, 0);

    compare = purc_variant_compare_ex (value1, value2, PCVARIANT_COMPARE_OPT_CASE);
    ASSERT_EQ(compare, 0);

    compare = purc_variant_compare_ex (value1, value2, PCVARIANT_COMPARE_OPT_NUMBER);
    ASSERT_EQ(compare, 0);

    compare = purc_variant_compare_ex (value1, value2, PCVARIANT_COMPARE_OPT_AUTO);
    ASSERT_EQ(compare, 0);

    purc_variant_unref (key1);
    purc_variant_unref (val1);
    purc_variant_unref (key2);
    purc_variant_unref (val2);
    purc_variant_unref (value1);
    purc_variant_unref (value2);


    value1 = purc_variant_make_number (1.0);
    value2 = purc_variant_make_number (2.0);
    compare = purc_variant_compare_ex (value1, value2, PCVARIANT_COMPARE_OPT_AUTO);
    ASSERT_LT(compare, 0);
    compare = purc_variant_compare_ex (value1, value2, PCVARIANT_COMPARE_OPT_CASE);
    ASSERT_LT(compare, 0);
    purc_variant_unref (value1);
    purc_variant_unref (value2);


    purc_cleanup ();
}

TEST(variant, reuse_buff)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex(PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test",
            "variant", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_rwstream_t rws;
    rws = purc_rwstream_new_buffer (32, 1024);
    purc_rwstream_write(rws, "hello", 5);
    purc_rwstream_write(rws, "\0", 1);

    size_t content_size, raw_size;
    char *s;
    s = (char*)purc_rwstream_get_mem_buffer_ex(rws,
            &content_size, &raw_size, true);

    ASSERT_NE(s, nullptr);
    ASSERT_EQ(content_size, 6);
    ASSERT_GT(raw_size, content_size);
    ASSERT_EQ(memcmp("hello", s, 5), 0);

    purc_rwstream_destroy(rws);

    purc_variant_t v;
    v = purc_variant_make_string_reuse_buff(s, content_size, false);
    purc_variant_unref(v);

    purc_cleanup ();
}

