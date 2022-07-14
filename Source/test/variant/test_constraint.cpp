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

#undef NDEBUG
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

    if (1) {
        struct record {
            const char *s;
        } records[] = {
            "[!,{name:[{first:xiaohong,last:xu},[]], extra:foo},{name:[{first:shuming,last:xue},[]], extra:bar}]",
            "[!,{extra:foo, name:[{first:xiaohong,last:xu},[]]},{extra:bar, name:[{first:shuming,last:xue},[]]}]",
            "[!,{extra:bar,name:[{first:shuming,last:xue},[]]},{extra:foo,name:[{first:xiaohong,last:xu},[]]}]",
            "[!,{name:[{first:xiaohong,last:xu},[]], extra:foo},{name:[{first:shuming,last:xue},[]], extra:bar}]",
        };

        purc_variant_t v0 = pcejson_parser_parse_string(records[0].s, 0, 0);
        for (size_t i=1; i<PCA_TABLESIZE(records); ++i) {
            ASSERT_EQ(0, var_diff(v0, records[i].s));
        }
        PURC_VARIANT_SAFE_CLEAR(v0);
    }
}

static pcutils_map       *obj_to_listener;

static int comp_by_key (const void *key1, const void *key2)
{
    uintptr_t k1 = (uintptr_t)key1;
    uintptr_t k2 = (uintptr_t)key2;
    return k1 - k2;
}

static int
map_generate(void)
{
    if (obj_to_listener)
        return 0;

    copy_key_fn copy_key = nullptr;
    free_key_fn free_key = nullptr;
    copy_val_fn copy_val = nullptr;
    free_val_fn free_val = nullptr;
    comp_key_fn comp_key = comp_by_key;
    bool threads = false;

    pcutils_map *map;
    map = pcutils_map_create(copy_key, free_key,
        copy_val, free_val,
        comp_key, threads);

    if (!map)
        return -1;

    obj_to_listener = map;

    return 0;
}

static void
map_destroy(void)
{
    if (!obj_to_listener)
        return;

    pcutils_map *map = obj_to_listener;
    size_t nr = pcutils_map_get_size(map);
    ASSERT_EQ(nr, 0);

    pcutils_map_destroy(map);

    obj_to_listener = nullptr;
}

static int
map_set_listener(purc_variant_t obj, struct pcvar_listener *listener)
{
    map_generate();
    pcutils_map *map = obj_to_listener;
    if (!map)
        return -1;

    pcutils_map_entry *entry;
    entry = pcutils_map_find(map, obj);
    if (entry)
        return -1;

    return pcutils_map_insert(map, obj, listener);
}

static void
map_remove_listener(purc_variant_t obj)
{
    map_generate();
    pcutils_map *map = obj_to_listener;
    if (!map)
        return;

    pcutils_map_entry *entry;
    entry = pcutils_map_find(map, obj);
    if (entry == nullptr)
        return;

    struct pcvar_listener *listener;
    listener = (struct pcvar_listener*)(entry->val);

    pcutils_map_erase(map, obj);

    purc_variant_revoke_listener(obj, listener);
}

static bool set_on_obj_grown(purc_variant_t set, purc_variant_t obj,
        purc_variant_t k, purc_variant_t v)
{
    PRINT_VARIANT(set);
    PRINT_VARIANT(obj);
    PRINT_VARIANT(k);
    PRINT_VARIANT(v);

    return true;
}

static bool set_on_obj_changed(purc_variant_t set, purc_variant_t obj,
        purc_variant_t ko, purc_variant_t vo,
        purc_variant_t kn, purc_variant_t vn)
{
    PRINT_VARIANT(set);
    PRINT_VARIANT(obj);
    PRINT_VARIANT(ko);
    PRINT_VARIANT(vo);
    PRINT_VARIANT(kn);
    PRINT_VARIANT(vn);
    PC_DEBUGX("dddddddddddddddddddddddddddddddddddddd");

    return true;
}

static bool set_on_obj_shrunk(purc_variant_t set, purc_variant_t obj,
        purc_variant_t k, purc_variant_t v)
{
    PRINT_VARIANT(set);
    PRINT_VARIANT(obj);
    PRINT_VARIANT(k);
    PRINT_VARIANT(v);

    return true;
}


static bool obj_post_handler (
        purc_variant_t src,  // the source variant.
        pcvar_op_t op,       // the operation identifier.
        void *ctxt,          // the context stored when registering the handler.
        size_t nr_args,      // the number of the relevant child variants.
        purc_variant_t *argv // the array of all relevant child variants.
        )
{
    UNUSED_PARAM(src);
    UNUSED_PARAM(op);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    purc_variant_t set = (purc_variant_t)ctxt;
    PC_ASSERT(set);

    switch (op) {
        case PCVAR_OPERATION_GROW:
            PC_ASSERT(nr_args == 2);
            PC_ASSERT(argv);
            return set_on_obj_grown(set, src, argv[0], argv[1]);

        case PCVAR_OPERATION_CHANGE:
            PC_ASSERT(nr_args == 4);
            PC_ASSERT(argv);
            return set_on_obj_changed(set, src, argv[0], argv[1], argv[2], argv[3]);

        case PCVAR_OPERATION_SHRINK:
            PC_ASSERT(nr_args == 2);
            PC_ASSERT(argv);
            return set_on_obj_shrunk(set, src, argv[0], argv[1]);

        default:
            PC_ASSERT(0);
    }

    return true;
}

static bool set_on_grown(purc_variant_t set, purc_variant_t obj)
{
    PRINT_VARIANT(set);
    PRINT_VARIANT(obj);

    struct pcvar_listener *listener;

    int op = PCVAR_OPERATION_GROW |
        PCVAR_OPERATION_SHRINK |
        PCVAR_OPERATION_CHANGE;
    listener = purc_variant_register_post_listener(obj, (pcvar_op_t)op,
            obj_post_handler, set);
    EXPECT_NE(listener, nullptr);

    int r = map_set_listener(obj, listener);
    assert(r == 0);

    return true;
}

static bool set_on_changed(purc_variant_t set,
        purc_variant_t _old, purc_variant_t _new)
{
    PRINT_VARIANT(set);
    PRINT_VARIANT(_old);
    PRINT_VARIANT(_new);

    map_remove_listener(_old);

    struct pcvar_listener *listener;

    int op = PCVAR_OPERATION_GROW |
        PCVAR_OPERATION_SHRINK |
        PCVAR_OPERATION_CHANGE;
    listener = purc_variant_register_post_listener(_new, (pcvar_op_t)op,
            obj_post_handler, set);
    EXPECT_NE(listener, nullptr);

    int r = map_set_listener(_new, listener);
    assert(r == 0);

    return true;
}

static bool set_on_shrunk(purc_variant_t set, purc_variant_t obj)
{
    PRINT_VARIANT(set);
    PRINT_VARIANT(obj);

    map_remove_listener(obj);
    return true;
}

static bool set_post_handler (
        purc_variant_t src,  // the source variant.
        pcvar_op_t op,       // the operation identifier.
        void *ctxt,          // the context stored when registering the handler.
        size_t nr_args,      // the number of the relevant child variants.
        purc_variant_t *argv // the array of all relevant child variants.
        )
{
    UNUSED_PARAM(src);
    UNUSED_PARAM(op);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    switch (op) {
        case PCVAR_OPERATION_GROW:
            PC_ASSERT(nr_args == 1);
            PC_ASSERT(argv);
            PC_ASSERT(src == ctxt);
            return set_on_grown(src, argv[0]);

        case PCVAR_OPERATION_CHANGE:
            PC_ASSERT(nr_args == 2);
            PC_ASSERT(argv);
            PC_ASSERT(src == ctxt);
            return set_on_changed(src, argv[0], argv[1]);

        case PCVAR_OPERATION_SHRINK:
            PC_ASSERT(nr_args == 1);
            PC_ASSERT(argv);
            PC_ASSERT(src == ctxt);
            return set_on_shrunk(src, argv[0]);

        default:
            PC_ASSERT(0);
    }

    return true;
}

TEST(constraint, set_change)
{
    PurCInstance purc;

    bool overwrite = true;
    bool ok;
    const char *s;
    purc_variant_t set, obj;

    // s = "[!id,{id:foo,val:yes},{id:bar,val:no}]";
    s = "[!id]";
    set = pcejson_parser_parse_string(s, 0, 0);
    PRINT_VARIANT(set);
    EXPECT_NE(set, nullptr);

    struct pcvar_listener *listener;
    int op = PCVAR_OPERATION_GROW |
        PCVAR_OPERATION_SHRINK |
        PCVAR_OPERATION_CHANGE;

    listener = purc_variant_register_post_listener(set, (pcvar_op_t)op,
            set_post_handler, set);
    EXPECT_NE(listener, nullptr);


    s = "{id:foo,val:yes}";
    obj = pcejson_parser_parse_string(s, 0, 0);
    EXPECT_NE(obj, nullptr);

    ok = purc_variant_set_add(set, obj, overwrite);
    EXPECT_TRUE(ok);
    PURC_VARIANT_SAFE_CLEAR(obj);
    PRINT_VARIANT(set);


    s = "{id:bar,val:yes}";
    obj = pcejson_parser_parse_string(s, 0, 0);
    EXPECT_NE(obj, nullptr);

    ok = purc_variant_set_add(set, obj, overwrite);
    EXPECT_TRUE(ok);
    PRINT_VARIANT(set);
    PURC_VARIANT_SAFE_CLEAR(obj);


    s = "{id:foo,val:no}";
    obj = pcejson_parser_parse_string(s, 0, 0);
    EXPECT_NE(obj, nullptr);

    ok = purc_variant_set_add(set, obj, overwrite);
    EXPECT_TRUE(ok);
    PRINT_VARIANT(set);

    PC_DEBUGX("");
    purc_variant_t v = purc_variant_make_string("foobar", false);
    ok = purc_variant_object_set_by_static_ckey(obj, "val", v);
    EXPECT_TRUE(ok);
    PRINT_VARIANT(set);
    PC_DEBUGX("");
    PURC_VARIANT_SAFE_CLEAR(v);
    PURC_VARIANT_SAFE_CLEAR(obj);

    // intentionally remove set elements to trigger removing of bounded listeners
    while (1) {
        ssize_t nr;
        nr = purc_variant_set_get_size(set);
        if (nr == 0)
            break;
        purc_variant_t obj;
        obj = purc_variant_set_remove_by_index(set, 0);
        PURC_VARIANT_SAFE_CLEAR(obj);
    }

    // now, it's safe to remove set's own listener
    if (listener)
        purc_variant_revoke_listener(set, listener);

    PURC_VARIANT_SAFE_CLEAR(set);

    // remove map to satisfy valgrind
    map_destroy();
}

