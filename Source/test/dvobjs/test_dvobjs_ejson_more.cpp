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

#include "purc/purc-variant.h"
#include "purc/purc-dvobjs.h"
#include "purc/purc-ports.h"

#include "config.h"
#include "../helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <locale.h>
#include <limits.h>
#include <unistd.h>
#include <sys/time.h>

#include <gtest/gtest.h>

TEST(dvobjs, basic)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t dvobj;

    dvobj = purc_dvobj_ejson_new();
    ASSERT_EQ(purc_variant_is_object(dvobj), true);
    purc_variant_unref(dvobj);

    purc_cleanup();
}

static purc_variant_t get_dvobj_ejson(void* ctxt, const char* name)
{
    if (strcmp(name, "EJSON") == 0) {
        return (purc_variant_t)ctxt;
    }

    return PURC_VARIANT_INVALID;
}

typedef purc_variant_t (*fn_expected)(purc_variant_t dvobj, const char* name);
typedef bool (*fn_cmp)(purc_variant_t result, purc_variant_t expected);

struct ejson_result {
    const char             *name;
    const char             *ejson;

    fn_expected             expected;
    fn_cmp                  vrtcmp;
    int                     errcode;
};

static void run_testcases(const struct ejson_result *test_cases, size_t n)
{
    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobjs", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t dvobj = purc_dvobj_ejson_new();
    ASSERT_NE(dvobj, nullptr);
    ASSERT_EQ(purc_variant_is_object(dvobj), true);

    for (size_t i = 0; i < n; i++) {
        struct purc_ejson_parse_tree *ptree;
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        ptree = purc_variant_ejson_parse_string(test_cases[i].ejson,
                strlen(test_cases[i].ejson));
        result = purc_variant_ejson_parse_tree_evalute(ptree,
                get_dvobj_ejson, dvobj, true);
        purc_variant_ejson_parse_tree_destroy(ptree);

        /* FIXME: purc_variant_ejson_parse_tree_evalute should not return NULL
           when evaluating silently */
        ASSERT_NE(result, nullptr);

        if (test_cases[i].expected) {
            expected = test_cases[i].expected(dvobj, test_cases[i].name);

            if (purc_variant_get_type(result) != purc_variant_get_type(expected)) {
                purc_log_error("result type: %s, error message: %s\n",
                        purc_variant_typename(purc_variant_get_type(result)),
                        purc_get_error_message(purc_get_last_error()));
            }

            if (test_cases[i].vrtcmp) {
                ASSERT_EQ(test_cases[i].vrtcmp(result, expected), true);
            }
            else {
                ASSERT_EQ(purc_variant_is_equal_to(result, expected), true);
            }

            if (test_cases[i].errcode) {
                ASSERT_EQ(purc_get_last_error(), test_cases[i].errcode);
            }

            purc_variant_unref(expected);
        }
        else {
            ASSERT_EQ(purc_variant_get_type(result), PURC_VARIANT_TYPE_NULL);
        }

        purc_variant_unref(result);
    }

    purc_variant_unref(dvobj);
    purc_cleanup();
}

purc_variant_t numerify(purc_variant_t dvobj, const char* name)
{
    double d = 0;

    (void)dvobj;

    if (strcmp(name, "zero") == 0) {
        d = 0;
    }
    else {
        d = strtod(name, NULL);
    }

    return purc_variant_make_number(d);
}

static bool numerify_vrtcmp(purc_variant_t result, purc_variant_t expected)
{
    double r1, r2;

    if (purc_variant_cast_to_number(result, &r1, false) &&
            purc_variant_cast_to_number(expected, &r2, false)) {
        return r1 == r2;
    }

    return false;
}

TEST(dvobjs, numerify)
{
    static const struct ejson_result test_cases[] = {
        { "zero",
            "$EJSON.numerify",
            numerify, numerify_vrtcmp, 0 },
        { "zero",
            "$EJSON.numerify(null)",
            numerify, numerify_vrtcmp, 0 },
        { "zero",
            "$EJSON.numerify(false)",
            numerify, numerify_vrtcmp, 0 },
        { "zero",
            "$EJSON.numerify([])",
            numerify, numerify_vrtcmp, 0 },
        { "zero",
            "$EJSON.numerify({})",
            numerify, numerify_vrtcmp, 0 },
        { "1.0",
            "$EJSON.numerify(true)",
            numerify, numerify_vrtcmp, 0 },
        { "1.0",
            "$EJSON.numerify(1.0)",
            numerify, numerify_vrtcmp, 0 },
        { "1.0",
            "$EJSON.numerify('1.0')",
            numerify, numerify_vrtcmp, 0 },
        { "2.0",
            "$EJSON.numerify([1.0, 1.0])",
            numerify, numerify_vrtcmp, 0 },
        { "2.0",
            "$EJSON.numerify({x:1.0, y:1.0})",
            numerify, numerify_vrtcmp, 0 },
        { "0",
            "$EJSON.numerify($EJSON)",
            numerify, numerify_vrtcmp, 0 },
        { "3.0",
            "$EJSON.numerify($EJSON.numerify(3.0))",
            numerify, numerify_vrtcmp, 0 },
    };

    run_testcases(test_cases, PCA_TABLESIZE(test_cases));
}

purc_variant_t booleanize(purc_variant_t dvobj, const char* name)
{
    bool b = false;

    (void)dvobj;

    if (strcmp(name, "true") == 0) {
        b = true;
    }

    return purc_variant_make_boolean(b);
}

static bool booleanize_vrtcmp(purc_variant_t result, purc_variant_t expected)
{
    if (purc_variant_is_true(result) && purc_variant_is_true(expected)) {
        return true;
    }
    else if (purc_variant_is_false(result) && purc_variant_is_false(expected)) {
        return true;
    }

    return false;
}

TEST(dvobjs, booleanize)
{
    static const struct ejson_result test_cases[] = {
        { "false",
            "$EJSON.booleanize",
            booleanize, booleanize_vrtcmp, 0 },
        { "false",
            "$EJSON.booleanize(null)",
            booleanize, booleanize_vrtcmp, 0 },
        { "false",
            "$EJSON.booleanize(false)",
            booleanize, booleanize_vrtcmp, 0 },
        { "true",
            "$EJSON.booleanize(true)",
            booleanize, booleanize_vrtcmp, 0 },
        { "false",
            "$EJSON.booleanize(0)",
            booleanize, booleanize_vrtcmp, 0 },
        { "false",
            "$EJSON.booleanize('')",
            booleanize, booleanize_vrtcmp, 0 },
        { "false",
            "$EJSON.booleanize({})",
            booleanize, booleanize_vrtcmp, 0 },
        { "false",
            "$EJSON.booleanize([])",
            booleanize, booleanize_vrtcmp, 0 },
        { "true",
            "$EJSON.booleanize(1.0)",
            booleanize, booleanize_vrtcmp, 0 },
        { "true",
            "$EJSON.booleanize('123')",
            booleanize, booleanize_vrtcmp, 0 },
        { "true",
            "$EJSON.booleanize('0')",
            booleanize, booleanize_vrtcmp, 0 },
        { "true",
            "$EJSON.booleanize($EJSON)",
            booleanize, booleanize_vrtcmp, 0 },
        { "false",
            "$EJSON.booleanize($EJSON.booleanize)",
            booleanize, booleanize_vrtcmp, 0 },
    };

    run_testcases(test_cases, PCA_TABLESIZE(test_cases));
}

purc_variant_t stringify(purc_variant_t dvobj, const char* name)
{
    (void)dvobj;

    return purc_variant_make_string(name, false);
}

static bool stringify_vrtcmp(purc_variant_t result, purc_variant_t expected)
{
    const char *v1 = purc_variant_get_string_const(result);
    const char *v2 = purc_variant_get_string_const(expected);

    if (v1 == NULL || v2 == NULL)
        return false;

    return (strcmp(v1, v2) == 0);
}

TEST(dvobjs, stringify)
{
    static const struct ejson_result test_cases[] = {
        { "undefined",
            "$EJSON.stringify",
            stringify, stringify_vrtcmp, 0 },
        { "undefined",
            "$EJSON.stringify(undefined)",
            stringify, stringify_vrtcmp, 0 },
        { "null",
            "$EJSON.stringify(null)",
            stringify, stringify_vrtcmp, 0 },
        { "false",
            "$EJSON.stringify(false)",
            stringify, stringify_vrtcmp, 0 },
        { "true",
            "$EJSON.stringify(true)",
            stringify, stringify_vrtcmp, 0 },
        { "0",
            "$EJSON.stringify(0)",
            stringify, stringify_vrtcmp, 0 },
        { "",
            "$EJSON.stringify('')",
            stringify, stringify_vrtcmp, 0 },
        { "",
            "$EJSON.stringify({})",
            stringify, stringify_vrtcmp, 0 },
        { "x:1\ny:2\nz:3\n",
            "$EJSON.stringify({x:1, y:2, z: 3})",
            stringify, stringify_vrtcmp, 0 },
        { "",
            "$EJSON.stringify([])",
            stringify, stringify_vrtcmp, 0 },
        { "1\n2\n3\n",
            "$EJSON.stringify([1, 2, 3])",
            stringify, stringify_vrtcmp, 0 },
        { "1",
            "$EJSON.stringify(1.0)",
            stringify, stringify_vrtcmp, 0 },
        { "123",
            "$EJSON.stringify('123')",
            stringify, stringify_vrtcmp, 0 },
        { "0",
            "$EJSON.stringify('0')",
            stringify, stringify_vrtcmp, 0 },
        { "undefined",
            "$EJSON.stringify($EJSON.stringify)",
            stringify, stringify_vrtcmp, 0 },
    };

    run_testcases(test_cases, PCA_TABLESIZE(test_cases));
}

purc_variant_t isequal(purc_variant_t dvobj, const char* name)
{
    (void)dvobj;

    if (strcmp(name, "bad") == 0) {
        return purc_variant_make_undefined();
    }
    else if (strcmp(name, "true") == 0) {
        return purc_variant_make_boolean(true);
    }

    return purc_variant_make_boolean(false);
}

static bool isequal_vrtcmp(purc_variant_t result, purc_variant_t expected)
{
    if (purc_variant_is_true(result) && purc_variant_is_true(expected))
        return true;

    if (purc_variant_is_false(result) && purc_variant_is_false(expected))
        return true;

    return false;
}

TEST(dvobjs, isequal)
{
    static const struct ejson_result test_cases[] = {
        { "bad",
            "$EJSON.isequal",
            isequal, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad",
            "$EJSON.isequal(undefined)",
            isequal, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "true",
            "$EJSON.isequal(undefined, undefined)",
            isequal, isequal_vrtcmp, 0 },
        { "true",
            "$EJSON.isequal(null, null)",
            isequal, isequal_vrtcmp, 0 },
        { "true",
            "$EJSON.isequal(true, true)",
            isequal, isequal_vrtcmp, 0 },
        { "true",
            "$EJSON.isequal(false, false)",
            isequal, isequal_vrtcmp, 0 },
        { "true",
            "$EJSON.isequal(0, 0)",
            isequal, isequal_vrtcmp, 0 },
        { "true",
            "$EJSON.isequal('', '')",
            isequal, isequal_vrtcmp, 0 },
        { "true",
            "$EJSON.isequal([], [])",
            isequal, isequal_vrtcmp, 0 },
        { "true",
            "$EJSON.isequal({}, {})",
            isequal, isequal_vrtcmp, 0 },
        { "false",
            "$EJSON.isequal(0, '0')",
            isequal, isequal_vrtcmp, 0 },
        { "false",
            "$EJSON.isequal(undefined, null)",
            isequal, isequal_vrtcmp, 0 },
        { "false",
            "$EJSON.isequal(true, false)",
            isequal, isequal_vrtcmp, 0 },
        { "false",
            "$EJSON.isequal('0', '1')",
            isequal, isequal_vrtcmp, 0 },
        { "false",
            "$EJSON.isequal([], {})",
            isequal, isequal_vrtcmp, 0 },
        { "false",
            "$EJSON.isequal([0], [])",
            isequal, isequal_vrtcmp, 0 },
        { "true",
            "$EJSON.isequal($EJSON.booleanize, $EJSON.booleanize)",
            isequal, isequal_vrtcmp, 0 },
        { "false",
            "$EJSON.isequal($EJSON.booleanize, $EJSON.numerify)",
            isequal, isequal_vrtcmp, 0 },
    };

    run_testcases(test_cases, PCA_TABLESIZE(test_cases));
}

purc_variant_t fetchreal(purc_variant_t dvobj, const char* name)
{
    (void)dvobj;

    if (name[0] == 'i') {
        unsigned long long u = strtoull(name + 1, NULL, 16);
        return purc_variant_make_longint((int64_t)u);
    }
    else if (name[0] == 'u') {
        unsigned long long u = strtoull(name + 1, NULL, 16);
        return purc_variant_make_ulongint(u);
    }
    else if (name[0] == 'd') {
        double d = strtod(name + 1, NULL);
        return purc_variant_make_number(d);
    }

    return purc_variant_make_undefined();
}

static bool fetchreal_vrtcmp(purc_variant_t result, purc_variant_t expected)
{
    long double ld1, ld2;

    if (purc_variant_cast_to_longdouble(result, &ld1, false) &&
            purc_variant_cast_to_longdouble(expected, &ld2, false)) {
        return ld1 == ld2;
    }

    return false;
}

TEST(dvobjs, fetchreal)
{
    static const struct ejson_result test_cases[] = {
        { "bad",
            "$EJSON.fetchreal",
            fetchreal, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad",
            "$EJSON.fetchreal(undefined)",
            fetchreal, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad",
            "$EJSON.fetchreal(undefined, false)",
            fetchreal, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$EJSON.fetchreal(bx00, 'i8', 2)",
            fetchreal, NULL, PURC_ERROR_INVALID_VALUE },
        { "bad",
            "$EJSON.fetchreal(bx00, 'i8', 1)",
            fetchreal, NULL, PURC_ERROR_INVALID_VALUE },
        { "i00",
            "$EJSON.fetchreal(bx00, 'i8', 0)",
            fetchreal, fetchreal_vrtcmp, 0 },
        { "uFF",
            "$EJSON.fetchreal(bxFF, 'u8', 0)",
            fetchreal, fetchreal_vrtcmp, 0 },
        { "uFF",
            "$EJSON.fetchreal(bxFF, 'u8', -1)",
            fetchreal, fetchreal_vrtcmp, 0 },
        { "i3412",
            "$EJSON.fetchreal(bx1234, 'i16le', 0)",
            fetchreal, fetchreal_vrtcmp, 0 },
        { "i1234",
            "$EJSON.fetchreal(bx1234, 'i16be', 0)",
            fetchreal, fetchreal_vrtcmp, 0 },
        { "uFFEE",
            "$EJSON.fetchreal(bxEEFF, 'u16le', 0)",
            fetchreal, fetchreal_vrtcmp, 0 },
        { "uEEFF",
            "$EJSON.fetchreal(bxEEFF, 'u16be', 0)",
            fetchreal, fetchreal_vrtcmp, 0 },
        { "i78563412",
            "$EJSON.fetchreal(bx12345678, 'i32le', 0)",
            fetchreal, fetchreal_vrtcmp, 0 },
        { "i12345678",
            "$EJSON.fetchreal(bx12345678, 'i32be', 0)",
            fetchreal, fetchreal_vrtcmp, 0 },
        { "uFFEEDDCC",
            "$EJSON.fetchreal(bxCCDDEEFF, 'u32le', 0)",
            fetchreal, fetchreal_vrtcmp, 0 },
        { "uCCDDEEFF",
            "$EJSON.fetchreal(bxCCDDEEFF, 'u32be', 0)",
            fetchreal, fetchreal_vrtcmp, 0 },
        { "i8877665544332211",
            "$EJSON.fetchreal(bx1122334455667788, 'i64le', 0)",
            fetchreal, fetchreal_vrtcmp, 0 },
        { "i1122334455667788",
            "$EJSON.fetchreal(bx1122334455667788, 'i64be', 0)",
            fetchreal, fetchreal_vrtcmp, 0 },
        { "uFFEEDDCCBBAA9988",
            "$EJSON.fetchreal(bx8899AABBCCDDEEFF, 'u64le', 0)",
            fetchreal, fetchreal_vrtcmp, 0 },
        { "u8899AABBCCDDEEFF",
            "$EJSON.fetchreal(bx8899AABBCCDDEEFF, 'u64be', 0)",
            fetchreal, fetchreal_vrtcmp, 0 },
        { "i4433",
            "$EJSON.fetchreal(bx1122334455667788, 'i16le', 2)",
            fetchreal, fetchreal_vrtcmp, 0 },
        { "i7766",
            "$EJSON.fetchreal(bx1122334455667788, 'i16le', -3)",
            fetchreal, fetchreal_vrtcmp, 0 },
    };

    run_testcases(test_cases, PCA_TABLESIZE(test_cases));
}

purc_variant_t fetchstr(purc_variant_t dvobj, const char* name)
{
    (void)dvobj;

    if (strcmp(name, "bad") == 0) {
        return purc_variant_make_string_static("", false);
    }
    else {
        return purc_variant_make_string_static(name, false);
    }

    return purc_variant_make_undefined();
}

static bool fetchstr_vrtcmp(purc_variant_t result, purc_variant_t expected)
{
    const char *s1, *s2;

    s1 = purc_variant_get_string_const(result);
    s2 = purc_variant_get_string_const(expected);
    return (s1 && s2 && strcmp(s1, s2) == 0);
}

TEST(dvobjs, fetchstr)
{
    static const struct ejson_result test_cases[] = {
        { "bad",
            "$EJSON.fetchstr",
            fetchstr, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad",
            "$EJSON.fetchstr(undefined)",
            fetchstr, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad",
            "$EJSON.fetchstr(undefined, false)",
            fetchstr, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$EJSON.fetchstr(bx00, 'bad')",
            fetchstr, NULL, PURC_ERROR_INVALID_VALUE },
        { "bad",
            "$EJSON.fetchstr(bx00, false)",
            fetchstr, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$EJSON.fetchstr(bx00, '')",
            fetchstr, NULL, PURC_ERROR_INVALID_VALUE },
        { "bad",
            "$EJSON.fetchstr(bx00, 'utf8', 2)",
            fetchstr, NULL, PURC_ERROR_INVALID_VALUE },
        { "bad",
            "$EJSON.fetchstr(bx00, 'utf8', 1, 1)",
            fetchstr, NULL, PURC_ERROR_INVALID_VALUE },
        { "bad",
            "$EJSON.fetchstr(bx00, 'utf8', null, -2)",
            fetchstr, NULL, PURC_ERROR_INVALID_VALUE },
        { "bad",
            "$EJSON.fetchstr(bx00, 'utf8', false, -2)",
            fetchstr, NULL, PURC_ERROR_INVALID_VALUE },
        { "bad",
            "$EJSON.fetchstr(bxE58C97E4BAACE4B88AE6B5B7, 'unknow', 6, 6)",
            fetchstr, NULL, PURC_ERROR_INVALID_VALUE },
        { "",
            "$EJSON.fetchstr(bxE58C97E4BAACE4B88AE6B5B7, 'utf16', null, 11)",
            fetchstr, fetchstr_vrtcmp, 0 },
        { "",
            "$EJSON.fetchstr(bxE58C97E4BAACE4B88AE6B5B7, 'utf16le', null, 11)",
            fetchstr, fetchstr_vrtcmp, 0 },
        { "",
            "$EJSON.fetchstr(bxE58C97E4BAACE4B88AE6B5B7, 'utf16be', null, 11)",
            fetchstr, fetchstr_vrtcmp, 0 },
        { "",
            "$EJSON.fetchstr(bxE58C97E4BAACE4B88AE6B5B7, 'utf32', null, 10)",
            fetchstr, fetchstr_vrtcmp, 0 },
        { "",
            "$EJSON.fetchstr(bxE58C97E4BAACE4B88AE6B5B7, 'utf32le', null, 10)",
            fetchstr, fetchstr_vrtcmp, 0 },
        { "",
            "$EJSON.fetchstr(bxE58C97E4BAACE4B88AE6B5B7, 'utf32be', null, 10)",
            fetchstr, fetchstr_vrtcmp, 0 },
        { "上海",
            "$EJSON.fetchstr(bxE58C97E4BAACE4B88AE6B5B7, 'utf8', 6, 6)",
            fetchstr, fetchstr_vrtcmp, 0 },
        { "上海",
            "$EJSON.fetchstr(bxE58C97E4BAACE4B88AE6B5B7, 'utf8 ', 6, 6)",
            fetchstr, fetchstr_vrtcmp, 0 },
        { "北京上海",
            "$EJSON.fetchstr(bxE58C97E4BAACE4B88AE6B5B7, 'utf8 ', null, 0)",
            fetchstr, fetchstr_vrtcmp, 0 },
        { "海",
            "$EJSON.fetchstr(bxE58C97E4BAACE4B88AE6B5B7, 'utf8 ', null, 9)",
            fetchstr, fetchstr_vrtcmp, 0 },
        { "HVML",
            "$EJSON.fetchstr(bx48564D4CE698AFE585A8E79083E9A696E4B8AAE58FAFE7BC96E7A88BE6A087E8AEB0E8AFADE8A880EFBC81, 'utf8 ', 4, 0)",
            fetchstr, fetchstr_vrtcmp, 0 },
        { "HVML",
            "$EJSON.fetchstr(bx48564D4CE698AFE585A8E79083E9A696E4B8AAE58FAFE7BC96E7A88BE6A087E8AEB0E8AFADE8A880EFBC81, 'utf8:4 ')",
            fetchstr, fetchstr_vrtcmp, 0 },
        { "HVML是全球首个可编程标记语言！", // with BOM
            "$EJSON.fetchstr(bxFFFE480056004D004C002F666851037496992A4EEF53167F0B7A0768B08BED8B008A01FF, 'utf16 ', null, 0)",
            fetchstr, fetchstr_vrtcmp, 0 },
        { "HVML是全球首个可编程标记语言！", // without BOM
            "$EJSON.fetchstr(bx480056004D004C002F666851037496992A4EEF53167F0B7A0768B08BED8B008A01FF, 'utf16le ', null, 0)",
            fetchstr, fetchstr_vrtcmp, 0 },
        { "HVML是全球首个可编程标记语言！",// with BOM
            "$EJSON.fetchstr(bx0000FEFF00000048000000560000004D0000004C0000662F00005168000074030000999600004E2A000053EF00007F1600007A0B0000680700008BB000008BED00008A000000FF01, 'utf32 ', null, 0)",
            fetchstr, fetchstr_vrtcmp, 0 },
        { "HVML是全球首个可编程标记语言！",// with BOM
            "$EJSON.fetchstr(bxFFFE000048000000560000004D0000004C0000002F6600006851000003740000969900002A4E0000EF530000167F00000B7A000007680000B08B0000ED8B0000008A000001FF0000, 'utf32 ', null, 0)",
            fetchstr, fetchstr_vrtcmp, 0 },
        { "HVML是全球首个可编程标记语言！",// without BOM
            "$EJSON.fetchstr(bx00000048000000560000004D0000004C0000662F00005168000074030000999600004E2A000053EF00007F1600007A0B0000680700008BB000008BED00008A000000FF01, 'utf32be ', null, 0)",
            fetchstr, fetchstr_vrtcmp, 0 },
    };

    run_testcases(test_cases, PCA_TABLESIZE(test_cases));
}

purc_variant_t sort(purc_variant_t dvobj, const char* name)
{
    (void)dvobj;

    if (strcmp(name, "bad") == 0) {
        return purc_variant_make_boolean(false);
    }
    else {
        return purc_variant_make_string_static(name, false);
    }

    return purc_variant_make_boolean(false);
}

static bool sort_vrtcmp(purc_variant_t result, purc_variant_t expected)
{
    const char *s1, *s2;

    s1 = purc_variant_get_string_const(result);
    s2 = purc_variant_get_string_const(expected);
    return (s1 && s2 && strcmp(s1, s2) == 0);
}

TEST(dvobjs, sort)
{
    static const struct ejson_result test_cases[] = {
        { "bad",
            "$EJSON.sort",
            sort, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad",
            "$EJSON.sort(undefined)",
            sort, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$EJSON.sort(undefined, false)",
            sort, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$EJSON.sort([1, 2, 3], 'asc', false)",
            sort, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$EJSON.sort([1, 2, 3], 'asc', 'unknown')",
            sort, NULL, PURC_ERROR_INVALID_VALUE },
        { "[]",
            "$EJSON.serialize($EJSON.sort([], 'asc'))",
            sort, sort_vrtcmp, 0 },
        { "[1]",
            "$EJSON.serialize($EJSON.sort([1], 'desc'))",
            sort, sort_vrtcmp, 0 },
        { "[1,2,3]",
            "$EJSON.serialize($EJSON.sort([3, 2, 1]))",
            sort, sort_vrtcmp, 0 },
        { "[3,2,1]",
            "$EJSON.serialize($EJSON.sort([1, 2, 3], 'desc'))",
            sort, sort_vrtcmp, 0 },
        { "[\"003\",\"002\",\"001\"]",
            "$EJSON.serialize($EJSON.sort(['001', '002', '003'], 'desc', 'case'))",
            sort, sort_vrtcmp, 0 },
        { "[\"1\",\"02\",\"003\"]",
            "$EJSON.serialize($EJSON.sort(['1', '02', '003'], 'desc', 'case'))",
            sort, sort_vrtcmp, 0 },
        { "[\"003\",\"02\",\"1\"]",
            "$EJSON.serialize($EJSON.sort(['1', '02', '003'], 'desc', 'number'))",
            sort, sort_vrtcmp, 0 },
        { "[\"3\",\"02\",1]",
            "$EJSON.serialize($EJSON.sort([1, '02', '3'], 'desc', 'auto'))",
            sort, sort_vrtcmp, 0 },
    };

    run_testcases(test_cases, PCA_TABLESIZE(test_cases));
}

purc_variant_t shuffle(purc_variant_t dvobj, const char* name)
{
    (void)dvobj;

    if (strcmp(name, "bad") == 0) {
        return purc_variant_make_boolean(false);
    }
    else {
        return purc_variant_make_string_static(name, false);
    }

    return purc_variant_make_boolean(false);
}

static bool shuffle_vrtcmp(purc_variant_t result, purc_variant_t expected)
{
    const char *s1, *s2;

    s1 = purc_variant_get_string_const(result);
    s2 = purc_variant_get_string_const(expected);
    if (s1 == NULL || s2 == NULL)
        return false;

    purc_log_debug("result: %s; expected: %s\n", s1, s2);

    char str[strlen(s2) + 1];
    strcpy(str, s2);

    char *deli = strchr(str, '\t');
    if (deli) {
        *deli = '\0';
        const char* second = deli + 1;

        return (strcmp(s1, str) == 0) || (strcmp(s1, second) == 0);
    }

    return (strcmp(s1, s2) == 0);
}

TEST(dvobjs, shuffle)
{
    static const struct ejson_result test_cases[] = {
        { "bad",
            "$EJSON.shuffle",
            shuffle, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad",
            "$EJSON.shuffle(undefined)",
            shuffle, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$EJSON.shuffle(false)",
            shuffle, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$EJSON.shuffle(null)",
            shuffle, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "[]",
            "$EJSON.serialize($EJSON.shuffle([]))",
            shuffle, shuffle_vrtcmp, 0 },
        { "[1]",
            "$EJSON.serialize($EJSON.shuffle([1]))",
            shuffle, shuffle_vrtcmp, 0 },
        { "[1,2]\t[2,1]",
            "$EJSON.serialize($EJSON.shuffle([1, 2]))",
            shuffle, shuffle_vrtcmp, 0 },
    };

    run_testcases(test_cases, PCA_TABLESIZE(test_cases));
}

purc_variant_t parse(purc_variant_t dvobj, const char* name)
{
    (void)dvobj;

    if (strcmp(name, "bad") == 0) {
        return purc_variant_make_undefined();
    }
    else {
        return purc_variant_make_string_static(name, false);
    }

    return purc_variant_make_undefined();
}

static bool parse_vrtcmp(purc_variant_t result, purc_variant_t expected)
{
    const char *s1, *s2;

    s1 = purc_variant_get_string_const(result);
    s2 = purc_variant_get_string_const(expected);
    if (s1 == NULL || s2 == NULL)
        return false;

    return (strcmp(s1, s2) == 0);
}

TEST(dvobjs, parse)
{
    static const struct ejson_result test_cases[] = {
        { "bad",
            "$EJSON.parse",
            parse, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad",
            "$EJSON.parse(undefined)",
            parse, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$EJSON.parse(false)",
            parse, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$EJSON.parse(null)",
            parse, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$EJSON.parse('[')",
            parse, NULL, PCEJSON_ERROR_UNEXPECTED_EOF },
        { "null",
            "$EJSON.serialize($EJSON.parse('['))",
            parse, parse_vrtcmp, 0 },
        { "[]",
            "$EJSON.serialize($EJSON.parse('[]'))",
            parse, parse_vrtcmp, 0 },
        { "[1]",
            "$EJSON.serialize($EJSON.parse('[1]'))",
            parse, parse_vrtcmp, 0 },
        { "[1,2]",
            "$EJSON.serialize($EJSON.parse('[1, 2]'))",
            parse, parse_vrtcmp, 0 },
    };

    run_testcases(test_cases, PCA_TABLESIZE(test_cases));
}

purc_variant_t serialize(purc_variant_t dvobj, const char* name)
{
    (void)dvobj;

    return purc_variant_make_string_static(name, false);
}

static bool serialize_vrtcmp(purc_variant_t result, purc_variant_t expected)
{
    const char *s1, *s2;

    s1 = purc_variant_get_string_const(result);
    s2 = purc_variant_get_string_const(expected);
    if (s1 == NULL || s2 == NULL)
        return false;

    purc_log_info("result: %s; expected: %s\n", s1, s2);
    return (strcmp(s1, s2) == 0);
}

TEST(dvobjs, serialize)
{
    static const struct ejson_result test_cases[] = {
        { "null",
            "$EJSON.serialize",
            serialize, serialize_vrtcmp, 0 },
        { "null",
            "$EJSON.serialize(undefined, false)",
            serialize, serialize_vrtcmp, 0 },
        { "null",
            "$EJSON.serialize(undefined, 'unknown')",
            serialize, serialize_vrtcmp, 0 },
        { "null",
            "$EJSON.serialize",
            serialize, serialize_vrtcmp, 0 },
        { "\"<undefined>\"",
            "$EJSON.serialize(undefined, 'runtime-string')",
            serialize, serialize_vrtcmp, 0 },
        { "\"11223344\"",
            "$EJSON.serialize(bx11223344)",
            serialize, serialize_vrtcmp, 0 },
        { "\"11223344\"",
            "$EJSON.serialize(bx11223344, 'bseq-hex-string')",
            serialize, serialize_vrtcmp, 0 },
        { "bx11223344",
            "$EJSON.serialize(bx11223344, 'bseq-hex')",
            serialize, serialize_vrtcmp, 0 },
        { "bb00010001001000100011001101000100",
            "$EJSON.serialize(bx11223344, 'bseq-bin')",
            serialize, serialize_vrtcmp, 0 },
        { "bb0001.0001.0010.0010.0011.0011.0100.0100",
            "$EJSON.serialize(bx11223344, 'bseq-bin-dots')",
            serialize, serialize_vrtcmp, 0 },
        { "b64ESIzRA==",
            "$EJSON.serialize(bx11223344, 'bseq-base64')",
            serialize, serialize_vrtcmp, 0 },
        { "[1,2,b64ESIzRA==]",
            "$EJSON.serialize([1.0FL, 2.0, bx11223344], '\nreal-json  bseq-base64 ')",
            serialize, serialize_vrtcmp, 0 },
        { "[1FL,-2L,2UL,b64ESIzRA==]",
            "$EJSON.serialize([1.0FL, -2L, 2UL, bx11223344], '\nreal-ejson  bseq-base64 ')",
            serialize, serialize_vrtcmp, 0 },
    };

    run_testcases(test_cases, PCA_TABLESIZE(test_cases));
}

