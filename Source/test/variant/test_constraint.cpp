#include "purc.h"
#include "private/debug.h"
#include "private/ejson-parser.h"
#include "private/variant.h"

#include "../helpers.h"

#include <gtest/gtest.h>

static int
var_diff(purc_variant_t val, const char *s)
{
    purc_variant_t v = pcejson_parser_parse_string(s, 0, 0);
    int diff = pcvariant_diff(val, v);
    if (diff) {
        PRINT_VARIANT(val);
        PRINT_VARIANT(v);
        PC_DEBUGX("%s", s);
    }
    purc_variant_unref(v);
    return diff;
}

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

    val = purc_variant_object_get_by_ckey(xu, "name");
    ASSERT_NE(val, nullptr);

    elem = purc_variant_set_get_member_by_key_values(set, val, silently);
    ASSERT_NE(elem, nullptr);

    arr = purc_variant_object_get_by_ckey(elem, "name");
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

    val = purc_variant_object_get_by_ckey(xue, "name");
    ASSERT_NE(val, nullptr);

    elem = purc_variant_set_get_member_by_key_values(set, val, silently);
    ASSERT_NE(elem, nullptr);

    arr = purc_variant_object_get_by_ckey(elem, "name");
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

    val = purc_variant_object_get_by_ckey(xue, "name");
    ASSERT_NE(val, nullptr);

    elem = purc_variant_set_get_member_by_key_values(set, val, silently);
    ASSERT_NE(elem, nullptr);

    arr = purc_variant_object_get_by_ckey(elem, "name");
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

    val = purc_variant_object_get_by_ckey(xu, "name");
    ASSERT_NE(val, nullptr);
    ASSERT_EQ(0, var_diff(val, "[{first:xiaohong,last:xu}]"));
    ASSERT_EQ(0, var_diff(set, "[!name, {name:[{first:xiaohong,last:xu}], extra:foo}, {name:[{first:shuming,last:xue}], extra:bar}]"));

    elem = purc_variant_set_get_member_by_key_values(set, val, silently);
    ASSERT_NE(elem, nullptr);
    ASSERT_EQ(0, var_diff(elem, "{name:[{first:xiaohong,last:xu}], extra:foo}"));
    ASSERT_EQ(0, var_diff(set, "[!name, {name:[{first:xiaohong,last:xu}], extra:foo}, {name:[{first:shuming,last:xue}], extra:bar}]"));

    arr = purc_variant_object_get_by_ckey(elem, "name");
    ASSERT_NE(arr, nullptr);
    ASSERT_EQ(0, var_diff(arr, "[{first:xiaohong,last:xu}]"));
    ASSERT_EQ(0, var_diff(set, "[!name, {name:[{first:xiaohong,last:xu}], extra:foo}, {name:[{first:shuming,last:xue}], extra:bar}]"));

    PRINT_VARIANT(set);
    ok = purc_variant_array_set(arr, 0, first);
    PRINT_VARIANT(set);
    ASSERT_TRUE(ok);
    ASSERT_EQ(0, var_diff(arr, "[\"shuming\"]"));
    ASSERT_EQ(0, var_diff(set, "[!name, {name:[\"shuming\"], extra:foo}, {name:[{first:shuming,last:xue}], extra:bar}]"));

    val = purc_variant_object_get_by_ckey(xue, "name");
    ASSERT_NE(val, nullptr);
    ASSERT_EQ(0, var_diff(val, "[{first:shuming,last:xue}]"));

    elem = purc_variant_set_get_member_by_key_values(set, val, silently);
    ASSERT_NE(elem, nullptr);
    ASSERT_EQ(0, var_diff(elem, "{name:[{first:shuming,last:xue}], extra:bar}"));

    purc_variant_t arr1;
    arr1 = purc_variant_object_get_by_ckey(elem, "name");
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

    val = purc_variant_object_get_by_ckey(xue, "name");
    ASSERT_NE(val, nullptr);

    elem = purc_variant_set_get_member_by_key_values(set, val, silently);
    ASSERT_NE(elem, nullptr);

    arr = purc_variant_object_get_by_ckey(elem, "name");
    ASSERT_NE(arr, nullptr);

    PRINT_VARIANT(set);
    PRINT_VARIANT(arr);
    PRINT_VARIANT(last);
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

    val = purc_variant_object_get_by_ckey(xue, "name");
    ASSERT_NE(val, nullptr);

    elem = purc_variant_set_get_member_by_key_values(set, val, silently);
    ASSERT_NE(elem, nullptr);

    arr = purc_variant_object_get_by_ckey(elem, "name");
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

    val = purc_variant_object_get_by_ckey(xu, "name");
    ASSERT_NE(val, nullptr);

    elem = purc_variant_set_get_member_by_key_values(set, val, silently);
    ASSERT_NE(elem, nullptr);

    arr = purc_variant_object_get_by_ckey(elem, "name");
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

TEST(constraint, set_child_in_different_positions)
{
    PurCInstance purc;

    const char *s;
    purc_variant_t set, xu, xue;
    purc_variant_t empty;
    purc_variant_t val, elem, arr;
    bool silently = true;
    bool ok;

    s = "{name:[{first:xiaohong,last:xu}], extra:foo}";
    xu = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(xu, nullptr);

    s = "{name:[{first:shuming,last:xue}], extra:bar}";
    xue = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(xue, nullptr);

    set = purc_variant_make_set_by_ckey(2, "name", xu, xue);
    ASSERT_NE(set, nullptr);
    ASSERT_EQ(0, var_diff(set, "[!name,{name:[{first:xiaohong,last:xu}], extra:foo},{name:[{first:shuming,last:xue}], extra:bar}]"));

    s = "[]";
    empty = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(empty, nullptr);
    ASSERT_EQ(0, var_diff(empty, "[]"));

    val = purc_variant_object_get_by_ckey(xu, "name");
    ASSERT_NE(val, nullptr);

    elem = purc_variant_set_get_member_by_key_values(set, val, silently);
    ASSERT_NE(elem, nullptr);

    arr = purc_variant_object_get_by_ckey(elem, "name");
    ASSERT_NE(arr, nullptr);

    ok = purc_variant_array_append(arr, empty);
    ASSERT_TRUE(ok);
    PRINT_VARIANT(set);
    ASSERT_EQ(0, var_diff(set, "[!name,{name:[{first:xiaohong,last:xu},[]], extra:foo},{name:[{first:shuming,last:xue}], extra:bar}]"));

    val = purc_variant_object_get_by_ckey(xue, "name");
    ASSERT_NE(val, nullptr);

    elem = purc_variant_set_get_member_by_key_values(set, val, silently);
    ASSERT_NE(elem, nullptr);

    arr = purc_variant_object_get_by_ckey(elem, "name");
    ASSERT_NE(arr, nullptr);

    ok = purc_variant_array_append(arr, empty);
    ASSERT_TRUE(ok);
    PRINT_VARIANT(set);
    ASSERT_EQ(0, var_diff(set, "[!name,{name:[{first:xiaohong,last:xu},[]], extra:foo},{name:[{first:shuming,last:xue},[]], extra:bar}]"));

    val = purc_variant_make_string("hello", true);
    ASSERT_NE(val, nullptr);
    ok = purc_variant_array_append(empty, val);
    ASSERT_TRUE(ok);
    PURC_VARIANT_SAFE_CLEAR(val);
    ASSERT_EQ(0, var_diff(set, "[!name,{name:[{first:xiaohong,last:xu},[hello]], extra:foo},{name:[{first:shuming,last:xue},[hello]], extra:bar}]"));


    PURC_VARIANT_SAFE_CLEAR(empty);
    PURC_VARIANT_SAFE_CLEAR(xue);
    PURC_VARIANT_SAFE_CLEAR(xu);
    PURC_VARIANT_SAFE_CLEAR(set);
}

TEST(constraint, perf)
{
    PurCInstance purc;

    const char *s;
    purc_variant_t v1, v2;

    s = "[!name, {name:[{first:xiaohong,last:xu}], extra:foo}, {name:[{first:shuming,last:xue}], extra:bar}]";
    v1 = pcejson_parser_parse_string(s, 0, 0);
    v2 = pcejson_parser_parse_string(s, 0, 0);

    const char *env = getenv("IS_EQUAL_TO");

    size_t nr = 1024 * 8 * 8;

    if (!env) {
        for (size_t i=0; i<nr; ++i) {
            int diff = pcvariant_diff(v1, v2);
            ASSERT_EQ(diff, 0);
        }
    }
    else {
        for (size_t i=0; i<nr; ++i) {
            bool eq = purc_variant_is_equal_to(v1, v2);
            ASSERT_TRUE(eq);
        }
    }

    PURC_VARIANT_SAFE_CLEAR(v1);
    PURC_VARIANT_SAFE_CLEAR(v2);
}

TEST(constraint, object)
{
    PurCInstance purc;

    const char *s;
    purc_variant_t set;

    s = "[!, {name:xu},{}]";
    set = pcejson_parser_parse_string(s, 0, 0);
    PRINT_VARIANT(set);

    purc_variant_t v = purc_variant_set_get_by_index(set, 1);
    PRINT_VARIANT(v);

    purc_variant_t name = purc_variant_make_string("xu", false);
    PRINT_VARIANT(name);

    bool ok;
    ok = purc_variant_object_set_by_static_ckey(v, "name", name);
    PC_DEBUGX("ok: %s", ok ? "true" : "false");
    PRINT_VARIANT(v);
    PRINT_VARIANT(set);

    PURC_VARIANT_SAFE_CLEAR(name);
    PURC_VARIANT_SAFE_CLEAR(set);
}

TEST(constraint, basic)
{
    PurCInstance purc;

    const char *s;
    purc_variant_t set, v, one, a;
    bool ok;

    if (1) {
        s = "[!, [a],[]]";
        set = pcejson_parser_parse_string(s, 0, 0);

        v = purc_variant_set_get_by_index(set, 1);

        a = purc_variant_make_string("a", false);

        purc_variant_array_append(v, a);

        EXPECT_EQ(0, var_diff(set, s));

        PURC_VARIANT_SAFE_CLEAR(a);
        PURC_VARIANT_SAFE_CLEAR(set);
    }

    if (1) {
        s = "[!, [1],[]]";
        set = pcejson_parser_parse_string(s, 0, 0);

        v = purc_variant_set_get_by_index(set, 1);

        one = purc_variant_make_longdouble(1);

        purc_variant_array_append(v, one);

        EXPECT_EQ(0, var_diff(set, s));

        PURC_VARIANT_SAFE_CLEAR(one);
        PURC_VARIANT_SAFE_CLEAR(set);
    }

    if (1) {
        s = "[!, [!, a],[!]]";
        set = pcejson_parser_parse_string(s, 0, 0);

        v = purc_variant_set_get_by_index(set, 1);

        a = purc_variant_make_string("a", false);

        PRINT_VARIANT(set);
        PRINT_VARIANT(v);
        bool overwrite = true;
        ok = purc_variant_set_add(v, a, overwrite);
        PC_DEBUGX("ok: %s", ok ? "true" : "false");
        PRINT_VARIANT(v);
        PRINT_VARIANT(set);

        EXPECT_EQ(0, var_diff(set, s));

        PURC_VARIANT_SAFE_CLEAR(a);
        PURC_VARIANT_SAFE_CLEAR(set);
    }

    if (1) {
        // number but different type internally
        s = "[!, 123L, 123.0]";
        set = pcejson_parser_parse_string(s, 0, 0);

        PRINT_VARIANT(set);

        const char *against;
        against = "[!, 123L]";
        EXPECT_EQ(0, var_diff(set, against));

        against = "[!, 123]";
        EXPECT_EQ(0, var_diff(set, against));

        against = "[!, 123.0]";
        EXPECT_EQ(0, var_diff(set, against));

        PURC_VARIANT_SAFE_CLEAR(set);
    }
}

TEST(constraint, change_order)
{
    PurCInstance purc;

    const char *s;
    purc_variant_t set, v;
    bool overwrite, silently;

    s = "[!, 2, 1, 3]";
    if (1) {
        set = pcejson_parser_parse_string(s, 0, 0);
        v = purc_variant_set_get_by_index(set, 0);

        purc_variant_ref(v);
        silently = true;
        purc_variant_set_remove(set, v, silently);
        overwrite = true;
        purc_variant_set_add(set, v, overwrite);
        PURC_VARIANT_SAFE_CLEAR(v);

        PRINT_VARIANT(set);
        ASSERT_EQ(0, var_diff(set, s));

        PURC_VARIANT_SAFE_CLEAR(set);
    }

    if (1) {
        set = pcejson_parser_parse_string(s, 0, 0);
        v = purc_variant_set_get_by_index(set, 1);

        purc_variant_ref(v);
        silently = true;
        purc_variant_set_remove(set, v, silently);
        overwrite = true;
        purc_variant_set_add(set, v, overwrite);
        PURC_VARIANT_SAFE_CLEAR(v);

        PRINT_VARIANT(set);
        ASSERT_EQ(0, var_diff(set, s));

        PURC_VARIANT_SAFE_CLEAR(set);
    }

    if (1) {
        set = pcejson_parser_parse_string(s, 0, 0);
        PRINT_VARIANT(set);
        v = purc_variant_set_get_by_index(set, 2);

        purc_variant_ref(v);
        silently = true;
        purc_variant_set_remove(set, v, silently);
        overwrite = true;
        purc_variant_set_add(set, v, overwrite);
        PURC_VARIANT_SAFE_CLEAR(v);

        PRINT_VARIANT(set);
        ASSERT_EQ(0, var_diff(set, s));

        PURC_VARIANT_SAFE_CLEAR(set);
    }
}

TEST(constraint, object_order)
{
    PurCInstance purc;

    if (1) {
        struct record {
            const char *s;
        } records[] = {
            "{first:xiaohong,last:xu}",
            "{last:xu,first:xiaohong}",
        };

        purc_variant_t v0 = pcejson_parser_parse_string(records[0].s, 0, 0);
        for (size_t i=1; i<PCA_TABLESIZE(records); ++i) {
            ASSERT_EQ(0, var_diff(v0, records[i].s));
        }
        PURC_VARIANT_SAFE_CLEAR(v0);
    }

    if (1) {
        struct record {
            const char *s;
        } records[] = {
            "{name:[{first:xiaohong,last:xu}], extra:foo}",
            "{extra:foo, name:[{first:xiaohong,last:xu}]}",
        };

        purc_variant_t v0 = pcejson_parser_parse_string(records[0].s, 0, 0);
        for (size_t i=1; i<PCA_TABLESIZE(records); ++i) {
            ASSERT_EQ(0, var_diff(v0, records[i].s));
        }
        PURC_VARIANT_SAFE_CLEAR(v0);
    }

    if (1) {
        struct record {
            const char *s;
        } records[] = {
            "[!name,{name:[{first:xiaohong,last:xu},[]], extra:foo},{name:[{first:shuming,last:xue},[]], extra:bar}]",
            "[!name,{extra:foo, name:[{first:xiaohong,last:xu},[]]},{extra:bar, name:[{first:shuming,last:xue},[]]}]",
            "[!name,{extra:bar,name:[{first:shuming,last:xue},[]]},{extra:foo,name:[{first:xiaohong,last:xu},[]]}]",
            "[!name,{name:[{first:xiaohong,last:xu},[]], extra:foo},{name:[{first:shuming,last:xue},[]], extra:bar}]",
        };

        purc_variant_t v0 = pcejson_parser_parse_string(records[0].s, 0, 0);
        for (size_t i=1; i<PCA_TABLESIZE(records); ++i) {
            ASSERT_EQ(0, var_diff(v0, records[i].s));
        }
        PURC_VARIANT_SAFE_CLEAR(v0);
    }
}

