#include "purc.h"
#include "private/vcm.h"

#include <gtest/gtest.h>

purc_variant_t find_var(void* ctxt, const char* name)
{
    UNUSED_PARAM(name);
    return purc_variant_t(ctxt);
}

struct vcm_eval_test_data {
    const char *object;
    const char *jsonee;
    bool silently;
    bool result;
    purc_variant_type type;
};


static const struct vcm_eval_test_data test_cases[] = {
    {
        "{ \"title\" : \"Object title\" }",
        "$BUTTON.title",
        false,
        true,
        PURC_VARIANT_TYPE_STRING
    },
    {
        "{ \"title\" : \"Object title\" }",
        "$BUTTON.id",
        false,
        false,
        PURC_VARIANT_TYPE_UNDEFINED
    },
    {
        "{ \"title\" : \"Object title\" }",
        "$BUTTON.name",
        true,
        true,
        PURC_VARIANT_TYPE_UNDEFINED
    },
};

class test_vcm_eval : public testing::TestWithParam<vcm_eval_test_data>
{
protected:
    void SetUp() {
        purc_init_ex(PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test",
                "vcm_eval", NULL);
    }
    void TearDown() {
        purc_cleanup ();
    }
};


TEST_P(test_vcm_eval, eval_silently)
{
    vcm_eval_test_data data = GetParam();

    struct purc_ejson_parse_tree *ptree;
    purc_variant_t result;

    purc_variant_t obj = purc_variant_make_from_json_string(data.object,
            strlen(data.object));
    ASSERT_NE(obj, nullptr);

    ptree = purc_variant_ejson_parse_string(data.jsonee, strlen(data.jsonee));
    result = purc_variant_ejson_parse_tree_evalute(ptree,
            find_var, obj, data.silently);
    purc_variant_ejson_parse_tree_destroy(ptree);

    if (data.result) {
        ASSERT_NE(result, nullptr);
        ASSERT_EQ(purc_variant_get_type(result), data.type);
        purc_variant_unref(result);
    }
    else {
        ASSERT_EQ(result, nullptr);
    }

    purc_variant_unref(obj);

    purc_cleanup();
}

INSTANTIATE_TEST_SUITE_P(vcm_eval, test_vcm_eval,
        testing::ValuesIn(test_cases));
