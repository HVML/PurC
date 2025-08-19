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
#include "private/dvobjs.h"
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

extern void get_variant_total_info (size_t *mem, size_t *value, size_t *resv);
#define MAX_PARAM_NR    20

static void
_trim_tail_spaces(char *dest, size_t n)
{
    while (n>1) {
        if (!isspace(dest[n-1]))
            break;
        dest[--n] = '\0';
    }
}

static size_t
_fetch_cmd_output(const char *cmd, char *dest, size_t sz)
{
    FILE *fin = NULL;
    size_t n = 0;

    fin = popen(cmd, "r");
    if (!fin)
        return 0;

    n = fread(dest, 1, sz - 1, fin);
    dest[n] = '\0';

    if (pclose(fin)) {
        return 0;
    }

    _trim_tail_spaces(dest, n);
    return n;
}

TEST(dvobjs, basic)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobj", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t dvobj;

    dvobj = purc_dvobj_system_new();
    ASSERT_EQ(purc_variant_is_object(dvobj), true);
    purc_variant_unref(dvobj);

    purc_cleanup();
}

static purc_variant_t get_dvobj_system(void* ctxt, const char* name)
{
    if (strcmp(name, "SYS") == 0) {
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

purc_variant_t get_system_const(purc_variant_t dvobj, const char* name)
{
    const char *result = NULL;

    (void)dvobj;
    if (strcmp(name, "HVML_SPEC_VERSION") == 0) {
        result = HVML_SPEC_VERSION;
    }
    else if (strcmp(name, "HVML_SPEC_RELEASE") == 0) {
        result = HVML_SPEC_RELEASE;
    }
    else if (strcmp(name, "HVML_PREDEF_VARS_SPEC_VERSION") == 0) {
        result = HVML_PREDEF_VARS_SPEC_VERSION;
    }
    else if (strcmp(name, "HVML_PREDEF_VARS_SPEC_RELEASE") == 0) {
        result = HVML_PREDEF_VARS_SPEC_RELEASE;
    }
    else if (strcmp(name, "HVML_INTRPR_NAME") == 0) {
        result = HVML_INTRPR_NAME;
    }
    else if (strcmp(name, "HVML_INTRPR_VERSION") == 0) {
        result = HVML_INTRPR_VERSION;
    }
    else if (strcmp(name, "HVML_INTRPR_RELEASE") == 0) {
        result = HVML_INTRPR_RELEASE;
    }

    if (result)
        return purc_variant_make_string_static(result, false);

    return purc_variant_make_undefined();
}

TEST(dvobjs, const)
{
    static const struct ejson_result test_cases[] = {
        { "HVML_SPEC_VERSION",
            "$SYS.const('HVML_SPEC_VERSION')",
            get_system_const, NULL, 0 },
        { "HVML_SPEC_RELEASE",
            "$SYS.const('HVML_SPEC_RELEASE')",
            get_system_const, NULL, 0 },
        { "HVML_PREDEF_VARS_SPEC_VERSION",
            "$SYS.const('HVML_PREDEF_VARS_SPEC_VERSION')",
            get_system_const, NULL, 0 },
        { "HVML_PREDEF_VARS_SPEC_RELEASE",
            "$SYS.const('HVML_PREDEF_VARS_SPEC_RELEASE')",
            get_system_const, NULL, 0 },
        { "HVML_INTRPR_NAME",
            "$SYS.const('HVML_INTRPR_NAME')",
            get_system_const, NULL, 0 },
        { "HVML_INTRPR_VERSION",
            "$SYS.const('HVML_INTRPR_VERSION')",
            get_system_const, NULL, 0 },
        { "HVML_INTRPR_RELEASE",
            "$SYS.const('HVML_INTRPR_RELEASE')",
            get_system_const, NULL, 0 },
        { "nonexistent",
            "$SYS.const('nonexistent')",
            get_system_const, NULL, PURC_ERROR_INVALID_VALUE },
        { "nonexistent",
            "$SYS.nonexistent",
            get_system_const, NULL, 0 },
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobj", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object(sys), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        struct purc_ejson_parsing_tree *ptree;
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        ptree = purc_variant_ejson_parse_string(test_cases[i].ejson,
                strlen(test_cases[i].ejson));
        result = purc_ejson_parsing_tree_evalute(ptree,
                get_dvobj_system, sys, true);
        purc_ejson_parsing_tree_destroy(ptree);

        ASSERT_NE(result, nullptr);

        if (test_cases[i].expected) {
            expected = test_cases[i].expected(sys, test_cases[i].name);

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

            purc_variant_unref(expected);
        }
        else {
            ASSERT_EQ(purc_variant_get_type(result), PURC_VARIANT_TYPE_NULL);
        }

        purc_variant_unref(result);
    }

    purc_variant_unref(sys);
    purc_cleanup();
}

purc_variant_t get_system_uname(purc_variant_t dvobj, const char* name)
{
    char result[4096];

    (void)dvobj;

    if (name) {
        size_t n = _fetch_cmd_output(name, result, sizeof(result));
        if (n == 0) {
            return purc_variant_make_undefined();
        }
        return purc_variant_make_string(result, true);
    }

    return purc_variant_make_string_static("", true);
}

TEST(dvobjs, uname)
{
    if (access("/usr/bin/uname", F_OK)) {
        return;
    }

    static const struct ejson_result test_cases[] = {
        { "uname -s",
            "$SYS.uname()['kernel-name']",
            get_system_uname, NULL, 0 },
        { "uname -r",
            "$SYS.uname()['kernel-release']",
            get_system_uname, NULL, 0 },
        { "uname -v",
            "$SYS.uname()['kernel-version']",
            get_system_uname, NULL, 0 },
        { "uname -m",
            "$SYS.uname()['machine']",
            get_system_uname, NULL, 0 },
#if OS(LINUX)
        { "uname -p",
            "$SYS.uname()['processor']",
            get_system_uname, NULL, 0 },
        { "uname -i",
            "$SYS.uname()['hardware-platform']",
            get_system_uname, NULL, 0 },
        { "uname -o",
            "$SYS.uname()['operating-system']",
            get_system_uname, NULL, 0 },
#endif
        /* FIXME: uncomment this testcase after fixed the bug of
           purc_ejson_parsing_tree_evalute()
        { "uname -z",
            "$SYS.uname()['bad-part-name']",
            get_system_uname, NULL, 0 },
         */
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobj", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object(sys), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        struct purc_ejson_parsing_tree *ptree;
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        ptree = purc_variant_ejson_parse_string(test_cases[i].ejson,
                strlen(test_cases[i].ejson));
        result = purc_ejson_parsing_tree_evalute(ptree,
                get_dvobj_system, sys, true);
        purc_ejson_parsing_tree_destroy(ptree);

        /* FIXME: purc_ejson_parsing_tree_evalute should not return NULL
           when evaluating silently */
        ASSERT_NE(result, nullptr);

        if (test_cases[i].expected) {
            expected = test_cases[i].expected(sys, test_cases[i].name);

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

            purc_variant_unref(expected);
        }
        else {
            ASSERT_EQ(purc_variant_get_type(result), PURC_VARIANT_TYPE_NULL);
        }

        purc_variant_unref(result);
    }

    purc_variant_unref(sys);
    purc_cleanup();
}

TEST(dvobjs, uname_ptr)
{
    static const struct ejson_result test_cases[] = {
        { NULL,
            "$SYS.uname_prt('invalid-part-name')",
            get_system_uname, NULL, 0 },
        { "uname -s",
            "$SYS.uname_prt('kernel-name')",
            get_system_uname, NULL, 0 },
        { "uname -r",
            "$SYS.uname_prt('kernel-release')",
            get_system_uname, NULL, 0 },
        { "uname -v",
            "$SYS.uname_prt('kernel-version')",
            get_system_uname, NULL, 0 },
        { "uname -m",
            "$SYS.uname_prt('machine')",
            get_system_uname, NULL, 0 },
#if OS(LINUX)
        { "uname -p",
            "$SYS.uname_prt('processor')",
            get_system_uname, NULL, 0 },
        { "uname -i",
            "$SYS.uname_prt('hardware-platform')",
            get_system_uname, NULL, 0 },
        { "uname -o",
            "$SYS.uname_prt['  operating-system  ']",
            get_system_uname, NULL, 0 },
        { "uname -a",
            "$SYS.uname_prt('  all ')",
            get_system_uname, NULL, 0 },
        { "uname -m -o",
            "$SYS.uname_prt(' machine \\tinvalid-part-name \\toperating-system')",
            get_system_uname, NULL, 0 },
#endif
        { "uname",
            "$SYS.uname_prt('\\ndefault\\t ')",
            get_system_uname, NULL, 0 },
        { "uname",
            "$SYS.uname_prt.default",
            get_system_uname, NULL, 0 },
        { "uname -a",
            "$SYS.uname_prt.all",
            get_system_uname, NULL, 0 },
        { "uname -s -r -v",
            "$SYS.uname_prt(' kernel-name \\t\\nkernel-release \\t\\nkernel-version')",
            get_system_uname, NULL, 0 },
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobj", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object(sys), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        struct purc_ejson_parsing_tree *ptree;
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        ptree = purc_variant_ejson_parse_string(test_cases[i].ejson,
                strlen(test_cases[i].ejson));
        result = purc_ejson_parsing_tree_evalute(ptree,
                get_dvobj_system, sys, true);
        purc_ejson_parsing_tree_destroy(ptree);

        /* FIXME: purc_ejson_parsing_tree_evalute should not return NULL
           when evaluating silently */
        ASSERT_NE(result, nullptr);

        if (test_cases[i].expected) {
            expected = test_cases[i].expected(sys, test_cases[i].name);

            if (purc_variant_get_type(result) != purc_variant_get_type(expected)) {
                purc_log_error("result type: %s, error message: %s\n",
                        purc_variant_typename(purc_variant_get_type(result)),
                        purc_get_error_message(purc_get_last_error()));
            }

            if (test_cases[i].vrtcmp) {
                ASSERT_EQ(test_cases[i].vrtcmp(result, expected), true);
            }
            else {
                purc_log_error("result: %s\n",
                        purc_variant_get_string_const(result));
                purc_log_error("expected: %s\n",
                        purc_variant_get_string_const(expected));
                ASSERT_EQ(purc_variant_is_equal_to(result, expected), true);
            }

            purc_variant_unref(expected);
        }
        else {
            ASSERT_EQ(purc_variant_get_type(result), PURC_VARIANT_TYPE_NULL);
        }

        purc_variant_unref(result);
    }

    purc_variant_unref(sys);
    purc_cleanup();
}

purc_variant_t system_time(purc_variant_t dvobj, const char* name)
{
    (void)dvobj;

    if (strcmp(name, "get") == 0) {
        return purc_variant_make_longint((int64_t)time(NULL));
    }
    else if (strcmp(name, "set") == 0) {
        return purc_variant_make_boolean(false);
    }
    else if (strcmp(name, "bad-set") == 0) {
        return purc_variant_make_boolean(false);
    }
    else if (strcmp(name, "negative") == 0) {
        return purc_variant_make_boolean(false);
    }

    return purc_variant_make_undefined();
}

static bool time_vrtcmp(purc_variant_t t1, purc_variant_t t2)
{
    int64_t u1, u2;

    if (purc_variant_is_longint(t1) && purc_variant_is_longint(t2)) {
        purc_variant_cast_to_longint(t1, &u1, false);
        purc_variant_cast_to_longint(t2, &u2, false);

        if (u1 == u2 || (u1 + 1) == u2)
            return true;
    }

    return false;
}

TEST(dvobjs, time)
{
    static const struct ejson_result test_cases[] = {
        { "bad-set",
            "$SYS.time!( )",
            system_time, NULL, PURC_ERROR_ARGUMENT_MISSED },
#if OS(LINUX)
        { "negative",
            "$SYS.time!( -100L )",
            system_time, NULL, PURC_ERROR_INVALID_VALUE },
        { "negative",
            "$SYS.time!( -100UL )",
            system_time, NULL, PURC_ERROR_INVALID_VALUE },
        { "negative",
            "$SYS.time!( -1000.0FL )",
            system_time, NULL, PURC_ERROR_INVALID_VALUE  },
#endif
        { "set",
            "$SYS.time!( 100 )",
            system_time, NULL, PURC_ERROR_ACCESS_DENIED },
        { "get",
            "$SYS.time()",
            system_time, time_vrtcmp, 0 },
        { "get",
            "$SYS.time",
            system_time, time_vrtcmp, 0 },
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobj", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object(sys), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        struct purc_ejson_parsing_tree *ptree;
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        ptree = purc_variant_ejson_parse_string(test_cases[i].ejson,
                strlen(test_cases[i].ejson));
        result = purc_ejson_parsing_tree_evalute(ptree,
                get_dvobj_system, sys, true);
        purc_ejson_parsing_tree_destroy(ptree);

        /* FIXME: purc_ejson_parsing_tree_evalute should not return NULL
           when evaluating silently */
        ASSERT_NE(result, nullptr);

        if (test_cases[i].expected) {
            expected = test_cases[i].expected(sys, test_cases[i].name);

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
                if (purc_get_last_error() != PURC_ERROR_ACCESS_DENIED) {
                    EXPECT_EQ(purc_get_last_error(), test_cases[i].errcode);
                }
            }

            purc_variant_unref(expected);
        }
        else {
            ASSERT_EQ(purc_variant_get_type(result), PURC_VARIANT_TYPE_NULL);
        }

        purc_variant_unref(result);
    }

    purc_variant_unref(sys);
    purc_cleanup();
}

purc_variant_t system_time_us(purc_variant_t dvobj, const char* name)
{
    (void)dvobj;

    if (strcmp(name, "getobject") == 0) {
        // create an empty object
        purc_variant_t retv = purc_variant_make_object(0,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

        struct timeval tv;
        gettimeofday(&tv, NULL);

        purc_variant_t val = purc_variant_make_longint((int64_t)tv.tv_sec);
        purc_variant_object_set_by_static_ckey(retv, "sec", val);
        purc_variant_unref(val);

        val = purc_variant_make_longint((int64_t)tv.tv_usec);
        purc_variant_object_set_by_static_ckey(retv, "usec", val);
        purc_variant_unref(val);

        return retv;
    }
    else if (strcmp(name, "getlongdouble") == 0) {
        struct timeval tv;
        gettimeofday(&tv, NULL);

        long double time_ld = (long double)tv.tv_sec;
        time_ld += tv.tv_usec/1000000.0L;

        return purc_variant_make_longdouble(time_ld);
    }
    else if (strcmp(name, "set") == 0) {
        return purc_variant_make_boolean(false);
    }
    else if (strcmp(name, "bad-set") == 0) {
        return purc_variant_make_boolean(false);
    }
    else if (strcmp(name, "negative") == 0) {
        return purc_variant_make_boolean(false);
    }

    return purc_variant_make_undefined();
}

static bool time_us_vrtcmp(purc_variant_t t1, purc_variant_t t2)
{
    int64_t u1, u2;
    purc_variant_t v1, v2;

    if (purc_variant_is_object(t1) && purc_variant_is_object(t2)) {
        v1 = purc_variant_object_get_by_ckey_ex(t1, "sec", true);
        v2 = purc_variant_object_get_by_ckey_ex(t2, "sec", true);

        if (purc_variant_is_longint(v1) && purc_variant_is_longint(v2)) {
            purc_variant_cast_to_longint(v1, &u1, false);
            purc_variant_cast_to_longint(v2, &u2, false);

            if (u1 == u2 || (u1 + 1) == u2)
                return true;
        }
    }
    else if (purc_variant_is_longdouble(t1) && purc_variant_is_longdouble(t2)) {
        purc_variant_cast_to_longint(t1, &u1, false);
        purc_variant_cast_to_longint(t2, &u2, false);

        if (u1 == u2 || (u1 + 1) == u2)
            return true;
    }

    return false;
}

TEST(dvobjs, time_us)
{
    static const struct ejson_result test_cases[] = {
        { "bad-set",
            "$SYS.time_us!( )",
            system_time_us, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad-set",
            "$SYS.time_us!( 100UL )",
            system_time_us, NULL, PURC_ERROR_ACCESS_DENIED },
        { "bad-set",
            "$SYS.time_us!( {sec: 100UL, usec: 10000000 } )",
            system_time_us, NULL, PURC_ERROR_INVALID_VALUE  },
        { "bad-set",
            "$SYS.time_us!( {sdfsec: 100UL, sdfusec: 1000 } )",
            system_time_us, NULL, PURC_ERROR_INVALID_VALUE },
        { "bad-set",
            "$SYS.time_us!( {sec: 100UL, sdfusec: 1000 } )",
            system_time_us, NULL, PURC_ERROR_INVALID_VALUE },
        { "negative",
            "$SYS.time_us!( -10000.0 )",
            system_time_us, NULL, PURC_ERROR_INVALID_VALUE },
        { "set",
            "$SYS.time_us!( {sec: 100UL, usec: 1000} )",
            system_time_us, NULL, PURC_ERROR_ACCESS_DENIED },
        { "getlongdouble",
            "$SYS.time_us()",
            system_time_us, time_us_vrtcmp, 0 },
        { "getlongdouble",
            "$SYS.time_us(true)",
            system_time_us, time_us_vrtcmp, 0 },
        { "getlongdouble",
            "$SYS.time_us('longdouble')",
            system_time_us, time_us_vrtcmp, 0 },
        { "getlongdouble",
            "$SYS.time_us('badkeyword')",
            system_time_us, time_us_vrtcmp, 0 },
        { "getobject",
            "$SYS.time_us('object')",
            system_time_us, time_us_vrtcmp, 0 },
        { "getlongdouble",
            "$SYS.time_us",
            system_time_us, time_us_vrtcmp, 0 },
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobj", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object(sys), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        struct purc_ejson_parsing_tree *ptree;
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        ptree = purc_variant_ejson_parse_string(test_cases[i].ejson,
                strlen(test_cases[i].ejson));
        result = purc_ejson_parsing_tree_evalute(ptree,
                get_dvobj_system, sys, true);
        purc_ejson_parsing_tree_destroy(ptree);

        /* FIXME: purc_ejson_parsing_tree_evalute should not return NULL
           when evaluating silently */
        ASSERT_NE(result, nullptr);

        if (test_cases[i].expected) {
            expected = test_cases[i].expected(sys, test_cases[i].name);

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

    purc_variant_unref(sys);
    purc_cleanup();
}

purc_variant_t system_locale_get(purc_variant_t dvobj, const char* name)
{
    int category = (int)(intptr_t)name;

    (void)dvobj;

    if (category >= 0) {
        char *locale = setlocale(category, NULL);

        if (locale) {
            char *end = strchr(locale, '.');
            size_t length;

            if (end)
                length = end - locale;
            else
                length = strlen(locale);
            return purc_variant_make_string_ex(locale, length, false);
        }
    }

    return purc_variant_make_undefined();
}

TEST(dvobjs, locale)
{
    static const struct ejson_result test_cases[] = {
        { (const char*)LC_COLLATE,
            "$SYS.locale('collate')",
            system_locale_get, NULL, 0 },
        { (const char*)LC_CTYPE,
            "$SYS.locale('ctype')",
            system_locale_get, NULL, 0 },
        { (const char*)LC_TIME,
            "$SYS.locale('time')",
            system_locale_get, NULL, 0 },
        { (const char*)LC_NUMERIC,
            "$SYS.locale('numeric')",
            system_locale_get, NULL, 0 },
        { (const char*)LC_MONETARY,
            "$SYS.locale('monetary')",
            system_locale_get, NULL, 0 },
        { (const char*)-1,
            "$SYS.locale('all')",
            system_locale_get, NULL, PURC_ERROR_NOT_SUPPORTED },
#ifdef LC_ADDRESS
        { (const char*)LC_ADDRESS,
            "$SYS.locale('address')",
            system_locale_get, NULL, 0 },
#endif
#ifdef LC_IDENTIFICATION
        { (const char*)LC_IDENTIFICATION,
            "$SYS.locale('identification')",
            system_locale_get, NULL, 0 },
#endif
#ifdef LC_MEASUREMENT
        { (const char*)LC_MEASUREMENT,
            "$SYS.locale('measurement')",
            system_locale_get, NULL, 0 },
#endif
#ifdef LC_MESSAGES
        { (const char*)LC_MESSAGES,
            "$SYS.locale('messages')",
            system_locale_get, NULL, 0 },
#endif
#ifdef LC_NAME
        { (const char*)LC_NAME,
            "$SYS.locale('name')",
            system_locale_get, NULL, 0 },
#endif
#ifdef LC_PAPER
        { (const char*)LC_PAPER,
            "$SYS.locale('paper')",
            system_locale_get, NULL, 0 },
#endif
#ifdef LC_TELEPHONE
        { (const char*)LC_TELEPHONE,
            "$SYS.locale('telephone')",
            system_locale_get, NULL, 0 },
#endif
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobj", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object(sys), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        struct purc_ejson_parsing_tree *ptree;
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        ptree = purc_variant_ejson_parse_string(test_cases[i].ejson,
                strlen(test_cases[i].ejson));
        result = purc_ejson_parsing_tree_evalute(ptree,
                get_dvobj_system, sys, true);
        purc_ejson_parsing_tree_destroy(ptree);

        /* FIXME: purc_ejson_parsing_tree_evalute should not return NULL
           when evaluating silently */
        ASSERT_NE(result, nullptr);

        if (test_cases[i].expected) {
            expected = test_cases[i].expected(sys, test_cases[i].name);

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

    purc_variant_unref(sys);
    purc_cleanup();
}

purc_variant_t system_timezone(purc_variant_t dvobj, const char* name)
{
    (void)dvobj;

    if (strcmp(name, "get") == 0) {
        const char *timezone;

        char path[PATH_MAX + 1];
        const char *env_tz = getenv("TZ");
        if (env_tz) {
               if (env_tz[0] == ':')
                   timezone = env_tz + 1;
        }
        else {

            ssize_t nr_bytes;
            nr_bytes = readlink(PURC_SYS_TZ_FILE, path, sizeof(path)-1);
            if (nr_bytes > 0)
                path[nr_bytes] = '\0';
            if ((nr_bytes > 0) &&
                    (strstr(path, PURC_SYS_TZ_DIR) != NULL)) {
                     timezone = strstr(path, PURC_SYS_TZ_DIR);
                     timezone += sizeof(PURC_SYS_TZ_DIR) - 1;
            }
            else {
                purc_log_error("Cannot determine timezone for test.\n");
                return purc_variant_make_boolean(false);
            }
        }

        purc_log_info("expected timezone: %s; tzname[0]: %s; tzname[1]: %s\n",
                timezone, tzname[0], tzname[1]);
        return purc_variant_make_string(timezone, false);
    }
    else if (strcmp(name, "set") == 0) {
        return purc_variant_make_boolean(true);
    }

    return purc_variant_make_boolean(false);
}

TEST(dvobjs, timezone)
{
    static const struct ejson_result test_cases[] = {
        { "get",
            "$SYS.timezone()",
            system_timezone, NULL, 0 },
        { "bad-set",
            "$SYS.timezone!()",
            system_timezone, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad-set",
            "$SYS.timezone!( 'asdfasf')",
            system_timezone, NULL, PURC_ERROR_INVALID_VALUE },
        { "set",
            "$SYS.timezone!( 'Pacific/Auckland' )",
            system_timezone, NULL, 0 },
        { "set",
            "$SYS.timezone!( 'Pacific/Auckland', 'local ' )",
            system_timezone, NULL, 0 },
        { "set",
            "$SYS.timezone!( 'Pacific/Auckland', 'bad' )",
            system_timezone, NULL, 0 },
        { "get",
            "$SYS.timezone()",
            system_timezone, NULL, 0 },
        { "set",
            "$SYS.timezone!( 'Pacific/Auckland', true )",
            system_timezone, NULL, 0 },
        { "failed-set",
            "$SYS.timezone!( 'Pacific/Auckland', ' global' )",
            system_timezone, NULL, PURC_ERROR_ACCESS_DENIED },
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobj", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object(sys), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        struct purc_ejson_parsing_tree *ptree;
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        ptree = purc_variant_ejson_parse_string(test_cases[i].ejson,
                strlen(test_cases[i].ejson));
        result = purc_ejson_parsing_tree_evalute(ptree,
                get_dvobj_system, sys, true);
        purc_ejson_parsing_tree_destroy(ptree);

        /* FIXME: purc_ejson_parsing_tree_evalute should not return NULL
           when evaluating silently */
        ASSERT_NE(result, nullptr);

        if (test_cases[i].expected) {
            expected = test_cases[i].expected(sys, test_cases[i].name);

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

    purc_variant_unref(sys);
    purc_cleanup();
}

purc_variant_t system_random(purc_variant_t dvobj, const char* name)
{
    (void)dvobj;

    if (strcmp(name, "default") == 0) {
        return purc_variant_make_longint((int64_t)random());
    }
    else if (strcmp(name, "number") == 0) {
        return purc_variant_make_number(1.0 * random() / RAND_MAX);
    }
    else if (strcmp(name, "ulongint") == 0) {
        return purc_variant_make_ulongint(100 * random() / RAND_MAX);
    }
    else if (strcmp(name, "longdouble") == 0) {
        return purc_variant_make_longdouble(-1000000.0L * random() / RAND_MAX);
    }
    else if (strcmp(name, "set") == 0) {
        return purc_variant_make_boolean(true);
    }

    return purc_variant_make_boolean(false);
}

static bool random_vrtcmp(purc_variant_t r1, purc_variant_t r2)
{

    if (purc_variant_is_number(r1)) {
        double d1, d2;

        purc_variant_cast_to_number(r1, &d1, false);
        purc_variant_cast_to_number(r2, &d2, false);

        return (d1 >= 0 && d1 <= 1.0 && d2 >= 0 && d2 <= 1.0);
    }
    else if (purc_variant_is_longint(r1)) {
        int64_t d1, d2;

        purc_variant_cast_to_longint(r1, &d1, false);
        purc_variant_cast_to_longint(r2, &d2, false);

        return (d1 >= 0 && d1 <= RAND_MAX && d2 >= 0 && d2 <= RAND_MAX);
    }
    else if (purc_variant_is_ulongint(r1)) {
        uint64_t d1, d2;

        purc_variant_cast_to_ulongint(r1, &d1, false);
        purc_variant_cast_to_ulongint(r2, &d2, false);

        return (d1 <= 100 && d2 <= 100);
    }
    else if (purc_variant_is_longdouble(r1)) {
        long double d1, d2;

        purc_variant_cast_to_longdouble(r1, &d1, false);
        purc_variant_cast_to_longdouble(r2, &d2, false);

        return (d1 <= 0 && d1 >= -1000000.0L && d2 <= 0 && d2 >= -1000000.0L);
    }

    return false;
}

TEST(dvobjs, random)
{
    static const struct ejson_result test_cases[] = {
        { "default",
            "$SYS.random()",
            system_random, random_vrtcmp, 0 },
        { "number",
            "$SYS.random(1.0)",
            system_random, random_vrtcmp, 0 },
        { "ulongint",
            "$SYS.random(100UL)",
            system_random, random_vrtcmp, 0 },
        { "longdouble",
            "$SYS.random(-1000000.0FL)",
            system_random, random_vrtcmp, 0 },
        { "bad-set",
            "$SYS.random!()",
            system_random, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad-set",
            "$SYS.random!( 'asdfasf')",
            system_random, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad-set",
            "$SYS.random!( 1000, 300 )",
            system_random, NULL, PURC_ERROR_INVALID_VALUE },
        { "failed-set",
            "$SYS.random!( 'Pacific/Auckland', true )",
            system_random, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "set",
            "$SYS.random!( 1000 )",
            system_random, NULL, 0 },
        { "set",
            "$SYS.random!( 11000, 256 )",
            system_random, NULL, 0 },
        { "longdouble",
            "$SYS.random(-1000000.0FL)",
            system_random, random_vrtcmp, 0 },
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobj", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object(sys), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        struct purc_ejson_parsing_tree *ptree;
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        ptree = purc_variant_ejson_parse_string(test_cases[i].ejson,
                strlen(test_cases[i].ejson));
        result = purc_ejson_parsing_tree_evalute(ptree,
                get_dvobj_system, sys, true);
        purc_ejson_parsing_tree_destroy(ptree);

        /* FIXME: purc_ejson_parsing_tree_evalute should not return NULL
           when evaluating silently */
        ASSERT_NE(result, nullptr);

        if (test_cases[i].expected) {
            expected = test_cases[i].expected(sys, test_cases[i].name);

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

    purc_variant_unref(sys);
    purc_cleanup();
}

purc_variant_t system_cwd(purc_variant_t dvobj, const char* name)
{
    (void)dvobj;

    if (strcmp(name, "bad") == 0) {
        return purc_variant_make_boolean(false);
    }
    else if (strcmp(name, "current") == 0) {
        char path[PATH_MAX + 1];
        if (getcwd(path, sizeof(path)) == NULL)
            return purc_variant_make_boolean(false);

        return purc_variant_make_string(path, false);
    }
    else {
        if (chdir("/var/tmp"))
            return purc_variant_make_boolean(false);

        return purc_variant_make_boolean(true);
    }

    return purc_variant_make_boolean(false);
}

static bool cwd_vrtcmp(purc_variant_t r1, purc_variant_t r2)
{
    const char *d1, *d2;

    if (purc_variant_is_boolean(r1) && purc_variant_is_boolean(r2)) {
        return purc_variant_is_true(r1) && purc_variant_is_true(r2);
    }
    else {

        d1 = purc_variant_get_string_const(r1);
        d2 = purc_variant_get_string_const(r2);

        if (d1 == NULL || d2 == NULL)
            return false;

        return (strcmp(d1, d2) == 0);
    }

    return false;
}

TEST(dvobjs, cwd)
{
    static const struct ejson_result test_cases[] = {
        { "bad",
            "$SYS.cwd!( )",
            system_cwd, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad",
            "$SYS.cwd!( false )",
            system_cwd, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$SYS.cwd!( '/not/existe' )",
            system_cwd, NULL, PURC_ERROR_NOT_EXISTS },
        { "bad",
            "$SYS.cwd!( '/bin/echo' )",
            system_cwd, NULL, PURC_ERROR_NOT_DESIRED_ENTITY },
#if OS(LINUX)
        { "bad",
            "$SYS.cwd!( '/root' )",
            system_cwd, NULL, PURC_ERROR_ACCESS_DENIED },
#else
        { "bad",
            "$SYS.cwd!( '/root' )",
            system_cwd, NULL, PURC_ERROR_NOT_EXISTS },
#endif
        { "current",
            "$SYS.cwd",
            system_cwd, cwd_vrtcmp, 0 },
        { "current",
            "$SYS.cwd()",
            system_cwd, cwd_vrtcmp, 0 },
        { "set",
            "$SYS.cwd!( '/var/tmp' )",
            system_cwd, cwd_vrtcmp, 0 },
        { "current",
            "$SYS.cwd",
            system_cwd, cwd_vrtcmp, 0 },
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobj", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object(sys), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        struct purc_ejson_parsing_tree *ptree;
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        ptree = purc_variant_ejson_parse_string(test_cases[i].ejson,
                strlen(test_cases[i].ejson));
        result = purc_ejson_parsing_tree_evalute(ptree,
                get_dvobj_system, sys, true);
        purc_ejson_parsing_tree_destroy(ptree);

        /* FIXME: purc_ejson_parsing_tree_evalute should not return NULL
           when evaluating silently */
        ASSERT_NE(result, nullptr);

        if (test_cases[i].expected) {
            expected = test_cases[i].expected(sys, test_cases[i].name);

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

    purc_variant_unref(sys);
    purc_cleanup();
}

purc_variant_t system_env(purc_variant_t dvobj, const char* name)
{
    (void)dvobj;

    if (strcmp(name, "bad") == 0) {
        return purc_variant_make_undefined();
    }
    else if (strcmp(name, "bad-set") == 0) {
        return purc_variant_make_boolean(false);
    }
    else if (strcmp(name, "set") == 0) {
        char *env = getenv("PURC_TEST");
        if (env && strcmp(env, "on") == 0)
            return purc_variant_make_boolean(true);
        return purc_variant_make_boolean(false);
    }
    else if (strcmp(name, "test-set") == 0) {
        char *env = getenv("PURC_TEST");
        if (env)
            return purc_variant_make_string(env, false);
        return purc_variant_make_undefined();
    }
    else if (strcmp(name, "unset") == 0) {
        char *env = getenv("PURC_TEST");
        if (env == NULL)
            return purc_variant_make_boolean(true);
        return purc_variant_make_boolean(false);
    }
    else if (strcmp(name, "test-unset") == 0) {
        char *env = getenv("PURC_TEST");
        if (env == NULL)
            return purc_variant_make_undefined();
        return purc_variant_make_string(env, false);
    }

    return purc_variant_make_undefined();
}

static bool env_vrtcmp(purc_variant_t r1, purc_variant_t r2)
{
    const char *d1, *d2;

    if (purc_variant_is_boolean(r1) && purc_variant_is_boolean(r2)) {
        return purc_variant_is_true(r1) && purc_variant_is_true(r2);
    }
    else if (purc_variant_is_undefined(r1) && purc_variant_is_undefined(r2)) {
        return true;
    }
    else {

        d1 = purc_variant_get_string_const(r1);
        d2 = purc_variant_get_string_const(r2);

        if (d1 == NULL || d2 == NULL)
            return false;

        return (strcmp(d1, d2) == 0);
    }

    return false;
}

TEST(dvobjs, env)
{
    static const struct ejson_result test_cases[] = {
        { "bad",
            "$SYS.env",
            system_env, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad",
            "$SYS.env( false )",
            system_env, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$SYS.env( null )",
            system_env, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad-set",
            "$SYS.env!( false )",
            system_env, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad-set",
            "$SYS.env!( false, null )",
            system_env, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad-set",
            "$SYS.env!( 'PURC_TEST', false )",
            system_env, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "set",
            "$SYS.env!( 'PURC_TEST', 'on' )",
            system_env, env_vrtcmp, 0 },
        { "test-set",
            "$SYS.env('PURC_TEST')",
            system_env, env_vrtcmp, 0 },
        { "unset",
            "$SYS.env!( 'PURC_TEST', undefined )",
            system_env, env_vrtcmp, 0 },
        { "test-unset",
            "$SYS.env('PURC_TEST')",
            system_env, env_vrtcmp, 0 },
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobj", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object(sys), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        struct purc_ejson_parsing_tree *ptree;
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        ptree = purc_variant_ejson_parse_string(test_cases[i].ejson,
                strlen(test_cases[i].ejson));
        result = purc_ejson_parsing_tree_evalute(ptree,
                get_dvobj_system, sys, true);
        purc_ejson_parsing_tree_destroy(ptree);

        /* FIXME: purc_ejson_parsing_tree_evalute should not return NULL
           when evaluating silently */
        ASSERT_NE(result, nullptr);

        if (test_cases[i].expected) {
            expected = test_cases[i].expected(sys, test_cases[i].name);

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
                EXPECT_EQ(purc_get_last_error(), test_cases[i].errcode);
            }

            purc_variant_unref(expected);
        }
        else {
            ASSERT_EQ(purc_variant_get_type(result), PURC_VARIANT_TYPE_NULL);
        }

        purc_variant_unref(result);
    }

    purc_variant_unref(sys);
    purc_cleanup();
}

