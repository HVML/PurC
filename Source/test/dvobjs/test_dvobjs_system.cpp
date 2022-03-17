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

TEST(dvobjs, reuse_buff)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex(PURC_MODULE_VARIANT, "cn.fmsfot.hvml.test",
            "dvobj", &info);
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

static purc_variant_t get_dvobj_system(void* ctxt, const char* name)
{
    if (strcmp(name, "SYSTEM") == 0) {
        return (purc_variant_t)ctxt;
    }

    return PURC_VARIANT_INVALID;
}

typedef purc_variant_t (*fn_get_expected)(purc_variant_t dvobj, const char* name);

struct ejson_result {
    const char             *name;
    const char             *ejson;
    fn_get_expected         get_expected;
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

purc_variant_t get_system_uname(purc_variant_t dvobj, const char* name)
{
    char result[4096];

    (void)dvobj;

    if (name) {
        size_t n = _fetch_cmd_output(name, result, sizeof(result));
        if (n == 0) {
            return purc_variant_make_undefined();
        }
    }

    return purc_variant_make_string(result, true);
}

TEST(dvobjs, system)
{
    static const struct ejson_result test_cases[] = {
        { "HVML_SPEC_VERSION",
            "$SYSTEM.const('HVML_SPEC_VERSION')",
            get_system_const },
        { "HVML_SPEC_RELEASE",
            "$SYSTEM.const('HVML_SPEC_RELEASE')",
            get_system_const },
        { "HVML_PREDEF_VARS_SPEC_VERSION",
            "$SYSTEM.const('HVML_PREDEF_VARS_SPEC_VERSION')",
            get_system_const },
        { "HVML_PREDEF_VARS_SPEC_RELEASE",
            "$SYSTEM.const('HVML_PREDEF_VARS_SPEC_RELEASE')",
            get_system_const },
        { "HVML_INTRPR_NAME",
            "$SYSTEM.const('HVML_INTRPR_NAME')",
            get_system_const },
        { "HVML_INTRPR_VERSION",
            "$SYSTEM.const('HVML_INTRPR_VERSION')",
            get_system_const },
        { "HVML_INTRPR_RELEASE",
            "$SYSTEM.const('HVML_INTRPR_RELEASE')",
            get_system_const },
        { "nonexistent",
            "$SYSTEM.const('nonexistent')",
            get_system_const },
        { "nonexistent",
            "$SYSTEM.nonexistent)",
            NULL },
        { "uname -s",
            "$SYSTEM.uname()['kernel-name']",
            get_system_uname },
        { "uname -r",
            "$SYSTEM.uname()['kernel-release']",
            get_system_uname },
        { "uname -v",
            "$SYSTEM.uname()['kernel-version']",
            get_system_uname },
        { "uname -m",
            "$SYSTEM.uname()['machine']",
            get_system_uname },
        { "uname -p",
            "$SYSTEM.uname()['processor']",
            get_system_uname },
        { "uname -i",
            "$SYSTEM.uname()['hardware-platform']",
            get_system_uname },
        { "uname -o",
            "$SYSTEM.uname()['operating-system']",
            get_system_uname },
        /* FIXME: uncomment this testcase after fixed the bug of
           purc_variant_ejson_parse_tree_evalute()
        { "uname -z",
            "$SYSTEM.uname()['bad-part-name']",
            get_system_uname },
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

        if (test_cases[i].get_expected) {
            expected = test_cases[i].get_expected(sys, test_cases[i].name);

            if (purc_variant_get_type(result) != purc_variant_get_type(expected)) {
                purc_log_error("result type: %s, error message: %s\n",
                        purc_variant_typename(purc_variant_get_type(result)),
                        purc_get_error_message(purc_get_last_error()));
            }

            ASSERT_EQ(purc_variant_is_equal_to(result, expected), true);

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

