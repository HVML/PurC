#include "purc-variant.h"
#include "purc-dvobjs.h"

#include "config.h"
#include "../helpers.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

extern void get_variant_total_info (size_t *mem, size_t *value, size_t *resv);
#define MAX_PARAM_NR    20

TEST(dvobjs, basic)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);
    bool ok;
    purc_variant_t v;

    v = purc_variant_load_dvobj_from_so(NULL, "MATH");
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    ok = purc_variant_unload_dvobj(v);
    ASSERT_TRUE(ok);

    purc_cleanup();
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
    const char *result = "Unknown";

    (void)dvobj;

    if (strcmp(name, "kernel-name") == 0) {
#if OS(LINUX)
        result = "Linux";
#elif OS(MAC)
        result = "Darwin";
#endif
    }

    return purc_variant_make_string_static(result, false);
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
        { "kernel-name",
            "$SYSTEM.uname['kernel-name']",
            get_system_uname },
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test",
            "test_init", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object(sys), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        struct purc_ejson_parse_tree *ptree;
        purc_variant_t result, expected;

        ptree = purc_variant_ejson_parse_string(test_cases[i].ejson,
                strlen(test_cases[i].ejson));
        result = purc_variant_ejson_parse_tree_evalute(ptree,
                get_dvobj_system, sys, true);
        purc_variant_ejson_parse_tree_destroy(ptree);

        expected = test_cases[i].get_expected(sys, test_cases[i].name);

        if (purc_variant_get_type(result) != purc_variant_get_type(expected)) {
            printf("result type: %d, error message: %s\n",
                    purc_variant_get_type(result),
                    purc_get_error_message(purc_get_last_error()));
        }
        else {
            printf("result: %s\n", purc_variant_get_string_const(result));
        }

        printf("expected: %s\n", purc_variant_get_string_const(expected));

        ASSERT_EQ(purc_variant_is_equal_to(result, expected), true);

        purc_variant_unref(result);
        purc_variant_unref(expected);
    }

    purc_variant_unref(sys);
    purc_cleanup();
}

TEST(dvobjs, reuse_buff)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "test_init", &info);
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

