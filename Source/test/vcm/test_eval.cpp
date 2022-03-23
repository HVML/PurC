#include "purc.h"
#include "private/vcm.h"

#include <gtest/gtest.h>

purc_variant_t find_var(void* ctxt, const char* name)
{
    UNUSED_PARAM(name);
    return purc_variant_t(ctxt);
}

TEST(vcm, eval)
{
    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "vcm_eval", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    struct purc_ejson_parse_tree *ptree;
    purc_variant_t result;
    char object[] = "{\"title\":\"this is title\"}";
    char ejson[] = "$BUTTON.title";

    purc_variant_t obj = purc_variant_make_from_json_string(object, strlen(object));
    ASSERT_NE(obj, nullptr);

    ptree = purc_variant_ejson_parse_string(ejson, strlen(ejson));
    result = purc_variant_ejson_parse_tree_evalute(ptree,
            find_var, obj, false);
    purc_variant_ejson_parse_tree_destroy(ptree);

    ASSERT_NE(result, nullptr);

    if (result) {
        purc_variant_unref(result);
    }
    purc_variant_unref(obj);

    purc_cleanup();
}

TEST(vcm, eval_silently)
{
    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "vcm_eval", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    struct purc_ejson_parse_tree *ptree;
    purc_variant_t result;
    char object[] = "{\"title\":\"this is title\"}";
    char ejson[] = "$BUTTON.id";

    purc_variant_t obj = purc_variant_make_from_json_string(object, strlen(object));
    ASSERT_NE(obj, nullptr);

    ptree = purc_variant_ejson_parse_string(ejson, strlen(ejson));
    result = purc_variant_ejson_parse_tree_evalute(ptree,
            find_var, obj, true);
    purc_variant_ejson_parse_tree_destroy(ptree);

    ASSERT_NE(result, nullptr);
    ASSERT_EQ(purc_variant_is_undefined(result), true);

    if (result) {
        purc_variant_unref(result);
    }
    purc_variant_unref(obj);

    purc_cleanup();
}

