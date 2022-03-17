#include "purc-variant.h"
#include "purc-dvobjs.h"

#include "config.h"
#include "../helpers.h"

#include <stdio.h>
#include <errno.h>
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
    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test",
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
    if (strcmp(name, "SYSTEM") == 0) {
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
            "$SYSTEM.const('HVML_SPEC_VERSION')",
            get_system_const, NULL },
        { "HVML_SPEC_RELEASE",
            "$SYSTEM.const('HVML_SPEC_RELEASE')",
            get_system_const, NULL },
        { "HVML_PREDEF_VARS_SPEC_VERSION",
            "$SYSTEM.const('HVML_PREDEF_VARS_SPEC_VERSION')",
            get_system_const, NULL },
        { "HVML_PREDEF_VARS_SPEC_RELEASE",
            "$SYSTEM.const('HVML_PREDEF_VARS_SPEC_RELEASE')",
            get_system_const, NULL },
        { "HVML_INTRPR_NAME",
            "$SYSTEM.const('HVML_INTRPR_NAME')",
            get_system_const, NULL },
        { "HVML_INTRPR_VERSION",
            "$SYSTEM.const('HVML_INTRPR_VERSION')",
            get_system_const, NULL },
        { "HVML_INTRPR_RELEASE",
            "$SYSTEM.const('HVML_INTRPR_RELEASE')",
            get_system_const, NULL },
        { "nonexistent",
            "$SYSTEM.const('nonexistent')",
            get_system_const, NULL },
        { "nonexistent",
            "$SYSTEM.nonexistent",
            NULL, NULL },
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobj", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object(sys), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        struct purc_ejson_parse_tree *ptree;
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        ptree = purc_variant_ejson_parse_string(test_cases[i].ejson,
                strlen(test_cases[i].ejson));
        result = purc_variant_ejson_parse_tree_evalute(ptree,
                get_dvobj_system, sys, true);
        purc_variant_ejson_parse_tree_destroy(ptree);

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
    static const struct ejson_result test_cases[] = {
        { "uname -s",
            "$SYSTEM.uname()['kernel-name']",
            get_system_uname, NULL },
        { "uname -r",
            "$SYSTEM.uname()['kernel-release']",
            get_system_uname, NULL },
        { "uname -v",
            "$SYSTEM.uname()['kernel-version']",
            get_system_uname, NULL },
        { "uname -m",
            "$SYSTEM.uname()['machine']",
            get_system_uname, NULL },
        { "uname -p",
            "$SYSTEM.uname()['processor']",
            get_system_uname, NULL },
        { "uname -i",
            "$SYSTEM.uname()['hardware-platform']",
            get_system_uname, NULL },
        { "uname -o",
            "$SYSTEM.uname()['operating-system']",
            get_system_uname, NULL },
        /* FIXME: uncomment this testcase after fixed the bug of
           purc_variant_ejson_parse_tree_evalute()
        { "uname -z",
            "$SYSTEM.uname()['bad-part-name']",
            get_system_uname, NULL },
         */
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobj", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object(sys), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        struct purc_ejson_parse_tree *ptree;
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        ptree = purc_variant_ejson_parse_string(test_cases[i].ejson,
                strlen(test_cases[i].ejson));
        result = purc_variant_ejson_parse_tree_evalute(ptree,
                get_dvobj_system, sys, true);
        purc_variant_ejson_parse_tree_destroy(ptree);

        /* FIXME: purc_variant_ejson_parse_tree_evalute should not return NULL
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
            "$SYSTEM.uname_prt('invalid-part-name')",
            get_system_uname, NULL },
        { "uname -s",
            "$SYSTEM.uname_prt('kernel-name')",
            get_system_uname, NULL },
        { "uname -r",
            "$SYSTEM.uname_prt('kernel-release')",
            get_system_uname, NULL },
        { "uname -v",
            "$SYSTEM.uname_prt('kernel-version')",
            get_system_uname, NULL },
        { "uname -m",
            "$SYSTEM.uname_prt('machine')",
            get_system_uname, NULL },
        { "uname -p",
            "$SYSTEM.uname_prt('processor')",
            get_system_uname, NULL },
        { "uname -i",
            "$SYSTEM.uname_prt('hardware-platform')",
            get_system_uname, NULL },
        { "uname -o",
            "$SYSTEM.uname_prt['operating-system']",
            get_system_uname, NULL },
        { "uname -a",
            "$SYSTEM.uname_prt('all')",
            get_system_uname, NULL },
        { "uname",
            "$SYSTEM.uname_prt('default')",
            get_system_uname, NULL },
        { "uname -s -r -v",
            "$SYSTEM.uname_prt(' kernel-name \t\nkernel-release \t\nkernel-version')",
            get_system_uname, NULL },
        { "uname -m -o",
            "$SYSTEM.uname_prt(' machine \tinvalid-part-name \toperating-system')",
            get_system_uname, NULL },
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobj", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object(sys), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        struct purc_ejson_parse_tree *ptree;
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        ptree = purc_variant_ejson_parse_string(test_cases[i].ejson,
                strlen(test_cases[i].ejson));
        result = purc_variant_ejson_parse_tree_evalute(ptree,
                get_dvobj_system, sys, true);
        purc_variant_ejson_parse_tree_destroy(ptree);

        /* FIXME: purc_variant_ejson_parse_tree_evalute should not return NULL
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

purc_variant_t system_time(purc_variant_t dvobj, const char* name)
{
    (void)dvobj;

    if (strcmp(name, "get") == 0) {
        return purc_variant_make_ulongint((uint64_t)time(NULL));
    }
    else if (strcmp(name, "set") == 0) {
        return purc_variant_make_boolean(false);
    }
    else if (strcmp(name, "bad-set") == 0) {
        return purc_variant_make_boolean(false);
    }

    return purc_variant_make_string_static("", true);
}

static bool time_vrtcmp(purc_variant_t t1, purc_variant_t t2)
{
    uint64_t u1, u2;

    if (purc_variant_is_ulongint(t1) && purc_variant_is_ulongint(t2)) {
        purc_variant_cast_to_ulongint(t1, &u1, false);
        purc_variant_cast_to_ulongint(t2, &u2, false);

        if (u1 == u2 || (u1 + 1) == u2)
            return true;
    }

    return false;
}

TEST(dvobjs, time)
{
    static const struct ejson_result test_cases[] = {
        { "bad-set",
            "$SYSTEM.time(! )",
            system_time, NULL },
        { "set",
            "$SYSTEM.time(! 100 )",
            system_time, NULL },
        { "get",
            "$SYSTEM.time()",
            system_time, time_vrtcmp },
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobj", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object(sys), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        struct purc_ejson_parse_tree *ptree;
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        ptree = purc_variant_ejson_parse_string(test_cases[i].ejson,
                strlen(test_cases[i].ejson));
        result = purc_variant_ejson_parse_tree_evalute(ptree,
                get_dvobj_system, sys, true);
        purc_variant_ejson_parse_tree_destroy(ptree);

        /* FIXME: purc_variant_ejson_parse_tree_evalute should not return NULL
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

