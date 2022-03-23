#include "purc.h"
#include "private/debug.h"
#include "private/ejson-parser.h"
#include "private/variant.h"

#include "../helpers.h"

#include <gtest/gtest.h>

TEST(constraint, set_modify_children_of_uniqkey_from_outside)
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
    ok = purc_variant_object_set_by_static_ckey(name, "first", first);
    ASSERT_TRUE(ok);

    ok = purc_variant_object_set_by_static_ckey(name, "last", last);
    PRINT_VARIANT(set);
    ASSERT_FALSE(ok);

    PURC_VARIANT_SAFE_CLEAR(last);
    PURC_VARIANT_SAFE_CLEAR(first);
    PURC_VARIANT_SAFE_CLEAR(xue);
    PURC_VARIANT_SAFE_CLEAR(xu);
    PURC_VARIANT_SAFE_CLEAR(set);
}

TEST(constraint, set_grow_children_of_uniqkey_from_outside)
{
    PurCInstance purc;

    const char *s;
    purc_variant_t set, xu, xue;
    purc_variant_t last;
    purc_variant_t val, elem, arr, name;
    bool silently = true;
    bool ok;

    s = "{name:[{first:xiaohong,last:xu}], extra:foo}";
    xu = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(xu, nullptr);

    s = "{name:[{first:xiaohong}], extra:bar}";
    xue = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(xue, nullptr);

    s = "xu";
    last = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(last, nullptr);

    set = purc_variant_make_set_by_ckey(2, "name", xu, xue);
    ASSERT_NE(set, nullptr);

    val = purc_variant_object_get_by_ckey(xue, "name", silently);
    ASSERT_NE(val, nullptr);

    elem = purc_variant_set_get_member_by_key_values(set, val, silently);
    ASSERT_NE(elem, nullptr);

    arr = purc_variant_object_get_by_ckey(elem, "name", silently);
    ASSERT_NE(arr, nullptr);

    name = purc_variant_array_get(arr, 0);
    ASSERT_NE(name, nullptr);

    PRINT_VARIANT(set);
    ok = purc_variant_object_set_by_static_ckey(name, "last", last);
    PRINT_VARIANT(set);
    ASSERT_FALSE(ok);

    PURC_VARIANT_SAFE_CLEAR(last);
    PURC_VARIANT_SAFE_CLEAR(xue);
    PURC_VARIANT_SAFE_CLEAR(xu);
    PURC_VARIANT_SAFE_CLEAR(set);
}

TEST(constraint, set_shrink_children_of_uniqkey_from_outside)
{
    PurCInstance purc;

    const char *s;
    purc_variant_t set, xu, xue;
    purc_variant_t foo;
    purc_variant_t val, elem, arr, name;
    bool silently = true;
    bool ok;

    s = "{name:[{first:xiaohong,last:xu}], extra:foo}";
    xu = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(xu, nullptr);

    s = "{name:[{first:xiaohong,last:xu,foo:bar}], extra:bar}";
    xue = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(xue, nullptr);

    s = "foo";
    foo = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(foo, nullptr);

    set = purc_variant_make_set_by_ckey(2, "name", xu, xue);
    ASSERT_NE(set, nullptr);

    val = purc_variant_object_get_by_ckey(xue, "name", silently);
    ASSERT_NE(val, nullptr);

    elem = purc_variant_set_get_member_by_key_values(set, val, silently);
    ASSERT_NE(elem, nullptr);

    arr = purc_variant_object_get_by_ckey(elem, "name", silently);
    ASSERT_NE(arr, nullptr);

    name = purc_variant_array_get(arr, 0);
    ASSERT_NE(name, nullptr);

    PRINT_VARIANT(set);
    ok = purc_variant_object_remove(name, foo, silently);
    PRINT_VARIANT(set);
    ASSERT_FALSE(ok);

    PURC_VARIANT_SAFE_CLEAR(foo);
    PURC_VARIANT_SAFE_CLEAR(xue);
    PURC_VARIANT_SAFE_CLEAR(xu);
    PURC_VARIANT_SAFE_CLEAR(set);
}

static int
var_diff(purc_variant_t val, const char *s)
{
    purc_variant_t v = pcejson_parser_parse_string(s, 0, 0);
    int diff = pcvariant_diff(val, v);
    purc_variant_unref(v);
    return diff;
}

TEST(constraint, set_modify_children_of_uniqkey_from_outside_arr)
{
    PurCInstance purc;

    const char *s;
    purc_variant_t set, xu, xue;
    purc_variant_t first;
    purc_variant_t val, elem, arr;
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

    set = purc_variant_make_set_by_ckey(2, "name", xu, xue);
    ASSERT_NE(set, nullptr);
    ASSERT_EQ(0, var_diff(set, "[!name, {name:[{first:xiaohong,last:xu}], extra:foo}, {name:[{first:shuming,last:xue}], extra:bar}]"));

    val = purc_variant_object_get_by_ckey(xu, "name", silently);
    ASSERT_NE(val, nullptr);
    ASSERT_EQ(0, var_diff(val, "[{first:xiaohong,last:xu}]"));
    ASSERT_EQ(0, var_diff(set, "[!name, {name:[{first:xiaohong,last:xu}], extra:foo}, {name:[{first:shuming,last:xue}], extra:bar}]"));

    elem = purc_variant_set_get_member_by_key_values(set, val, silently);
    ASSERT_NE(elem, nullptr);
    ASSERT_EQ(0, var_diff(elem, "{name:[{first:xiaohong,last:xu}], extra:foo}"));
    ASSERT_EQ(0, var_diff(set, "[!name, {name:[{first:xiaohong,last:xu}], extra:foo}, {name:[{first:shuming,last:xue}], extra:bar}]"));

    arr = purc_variant_object_get_by_ckey(elem, "name", silently);
    ASSERT_NE(arr, nullptr);
    ASSERT_EQ(0, var_diff(arr, "[{first:xiaohong,last:xu}]"));
    ASSERT_EQ(0, var_diff(set, "[!name, {name:[{first:xiaohong,last:xu}], extra:foo}, {name:[{first:shuming,last:xue}], extra:bar}]"));

    PRINT_VARIANT(set);
    ok = purc_variant_array_set(arr, 0, first);
    PRINT_VARIANT(set);
    ASSERT_TRUE(ok);
    ASSERT_EQ(0, var_diff(arr, "[\"shuming\"]"));
    ASSERT_EQ(0, var_diff(set, "[!name, {name:[\"shuming\"], extra:foo}, {name:[{first:shuming,last:xue}], extra:bar}]"));

    val = purc_variant_object_get_by_ckey(xue, "name", silently);
    ASSERT_NE(val, nullptr);
    ASSERT_EQ(0, var_diff(val, "[{first:shuming,last:xue}]"));

    elem = purc_variant_set_get_member_by_key_values(set, val, silently);
    ASSERT_NE(elem, nullptr);
    ASSERT_EQ(0, var_diff(elem, "{name:[{first:shuming,last:xue}], extra:bar}"));

    purc_variant_t arr1;
    arr1 = purc_variant_object_get_by_ckey(elem, "name", silently);
    ASSERT_NE(arr1, nullptr);
    ASSERT_EQ(0, var_diff(arr1, "[{first:shuming,last:xue}]"));

    purc_variant_t elem1;
    elem1 = purc_variant_array_get(arr1, 0);
    ASSERT_NE(elem1, nullptr);
    ASSERT_EQ(0, var_diff(elem1, "{first:shuming,last:xue}"));

    // elem1 = purc_variant_container_clone_recursively(elem1);
    // ASSERT_NE(elem1, nullptr);
    // ASSERT_EQ(0, var_diff(elem1, "{first:shuming,last:xue}"));

    PRINT_VARIANT(set);
    PRINT_VARIANT(arr);
    PRINT_VARIANT(elem1);
    ok = purc_variant_array_set(arr, 0, elem1);
    // PURC_VARIANT_SAFE_CLEAR(elem1);
    PRINT_VARIANT(set);
    ASSERT_FALSE(ok);

    PURC_VARIANT_SAFE_CLEAR(first);
    PURC_VARIANT_SAFE_CLEAR(xue);
    PURC_VARIANT_SAFE_CLEAR(xu);
    PURC_VARIANT_SAFE_CLEAR(set);
}

TEST(constraint, set_grow_children_of_uniqkey_from_outside_arr)
{
    PurCInstance purc;

    const char *s;
    purc_variant_t set, xu, xue;
    purc_variant_t last;
    purc_variant_t val, elem, arr;
    bool silently = true;
    bool ok;

    s = "{name:[xiaohong,xu], extra:foo}";
    xu = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(xu, nullptr);

    s = "{name:[xiaohong], extra:bar}";
    xue = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(xue, nullptr);

    s = "xu";
    last = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(last, nullptr);

    set = purc_variant_make_set_by_ckey(2, "name", xu, xue);
    ASSERT_NE(set, nullptr);

    val = purc_variant_object_get_by_ckey(xue, "name", silently);
    ASSERT_NE(val, nullptr);

    elem = purc_variant_set_get_member_by_key_values(set, val, silently);
    ASSERT_NE(elem, nullptr);

    arr = purc_variant_object_get_by_ckey(elem, "name", silently);
    ASSERT_NE(arr, nullptr);

    PRINT_VARIANT(set);
    PRINT_VARIANT(arr);
    ok = purc_variant_array_append(arr, last);
    PRINT_VARIANT(set);
    ASSERT_FALSE(ok);

    PURC_VARIANT_SAFE_CLEAR(last);
    PURC_VARIANT_SAFE_CLEAR(xue);
    PURC_VARIANT_SAFE_CLEAR(xu);
    PURC_VARIANT_SAFE_CLEAR(set);
}

TEST(constraint, set_shrink_children_of_uniqkey_from_outside_arr)
{
    PurCInstance purc;

    const char *s;
    purc_variant_t set, xu, xue;
    purc_variant_t last;
    purc_variant_t val, elem, arr;
    bool silently = true;
    bool ok;

    s = "{name:[xiaohong,xu], extra:foo}";
    xu = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(xu, nullptr);

    s = "{name:[xiaohong,xu,foo], extra:bar}";
    xue = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(xue, nullptr);

    s = "xu";
    last = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(last, nullptr);

    set = purc_variant_make_set_by_ckey(2, "name", xu, xue);
    ASSERT_NE(set, nullptr);

    val = purc_variant_object_get_by_ckey(xue, "name", silently);
    ASSERT_NE(val, nullptr);

    elem = purc_variant_set_get_member_by_key_values(set, val, silently);
    ASSERT_NE(elem, nullptr);

    arr = purc_variant_object_get_by_ckey(elem, "name", silently);
    ASSERT_NE(arr, nullptr);

    PRINT_VARIANT(set);
    PRINT_VARIANT(arr);
    ok = purc_variant_array_remove(arr, 2);
    PRINT_VARIANT(set);
    ASSERT_FALSE(ok);

    PURC_VARIANT_SAFE_CLEAR(last);
    PURC_VARIANT_SAFE_CLEAR(xue);
    PURC_VARIANT_SAFE_CLEAR(xu);
    PURC_VARIANT_SAFE_CLEAR(set);
}

TEST(constraint, set_add_children_of_uniqkey_to_other_container)
{
    PurCInstance purc;

    const char *s;
    purc_variant_t set, xu, xue;
    purc_variant_t first, last;
    purc_variant_t val, elem, arr, name, other;
    bool silently = true;

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

    other = purc_variant_make_array(1, name);
    ASSERT_NE(other, nullptr);
    PURC_VARIANT_SAFE_CLEAR(other);

    other = purc_variant_make_object_by_static_ckey(1, "foo", name);
    ASSERT_NE(other, nullptr);
    PURC_VARIANT_SAFE_CLEAR(other);

    PURC_VARIANT_SAFE_CLEAR(last);
    PURC_VARIANT_SAFE_CLEAR(first);
    PURC_VARIANT_SAFE_CLEAR(xue);
    PURC_VARIANT_SAFE_CLEAR(xu);
    PURC_VARIANT_SAFE_CLEAR(set);
}

