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
    {
        NULL,
        NULL,
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

    size_t sz = data.object ? strlen(data.object) : 0;
    ptree = purc_variant_ejson_parse_string(data.object, sz);
    purc_variant_t obj = purc_variant_ejson_parse_tree_evalute(ptree, NULL,
            PURC_VARIANT_INVALID, data.silently);
    purc_variant_ejson_parse_tree_destroy(ptree);
    ASSERT_NE(obj, nullptr);

    sz = data.jsonee ? strlen(data.jsonee) : 0;
    ptree = purc_variant_ejson_parse_string(data.jsonee, sz);
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
