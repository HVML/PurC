#include "purc.h"
#include "private/debug.h"
#include "private/ejson-parser.h"
#include "private/variant.h"

#include "../helpers.h"

#include <gtest/gtest.h>

TEST(constraint, set_add)
{
    PurCInstance purc;

    const char *s;
    purc_variant_t set, xu, xue;
    purc_variant_t first, last;
    purc_variant_t val, elem, arr, name;
    bool silently = true;
    bool ok;

    s = "{name:[{first:xiaohong,last:xu}], extra:foo}";
    xu = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(xu, nullptr);

    s = "{name:[{first:shuming,last:xue}], extra:bar}";
    xue = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(xue, nullptr);

    s = "shuming";
    first = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(first, nullptr);

    s = "xue";
    last = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(last, nullptr);

    set = purc_variant_make_set_by_ckey(2, "name", xu, xue);
    ASSERT_NE(set, nullptr);

    val = purc_variant_object_get_by_ckey(xu, "name", silently);
    ASSERT_NE(val, nullptr);

    elem = purc_variant_set_get_member_by_key_values(set, val, silently);
    ASSERT_NE(elem, nullptr);

    arr = purc_variant_object_get_by_ckey(elem, "name", silently);
    ASSERT_NE(arr, nullptr);

    name = purc_variant_array_get(arr, 0);
    ASSERT_NE(name, nullptr);

    PRINT_VARIANT(set);
    PRINT_VARIANT(name);
    ok = purc_variant_object_set_by_static_ckey(name, "first", first);
    PRINT_VARIANT(name);
    PRINT_VARIANT(set);
    ASSERT_TRUE(ok);

    ok = purc_variant_object_set_by_static_ckey(name, "last", last);
    PRINT_VARIANT(name);
    PRINT_VARIANT(set);
    ASSERT_TRUE(ok);

    PURC_VARIANT_SAFE_CLEAR(last);
    PURC_VARIANT_SAFE_CLEAR(first);
    PURC_VARIANT_SAFE_CLEAR(xue);
    PURC_VARIANT_SAFE_CLEAR(xu);
    PURC_VARIANT_SAFE_CLEAR(set);
}

