#include "purc.h"
#include "private/hashtable.h"
#include "purc-variant.h"
#include "private/variant.h"
#include "private/ejson-parser.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <gtest/gtest.h>

static inline bool
sanity_check(purc_variant_t set)
{
    size_t sz;
    bool ok;
    ok = purc_variant_set_size(set, &sz);
    if (!ok)
        return false;

    for (size_t i=0; i<sz; ++i) {
        purc_variant_t v = purc_variant_set_get_by_index(set, i);
        if (v == PURC_VARIANT_INVALID)
            return false;
    }

    return true;
}

TEST(variant_set, init_with_1_str)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    const char *s = "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar "
        "helloworld damngood foobar ";
    purc_variant_t str = purc_variant_make_string(s, false);
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 1);

    purc_variant_t var = purc_variant_make_set_by_ckey(0, "hello", NULL);
    ASSERT_EQ(stat->nr_values[PVT(_SET)], 1);
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 1);
    ASSERT_TRUE(sanity_check(var));

    purc_variant_ref(var);
    ASSERT_EQ(stat->nr_values[PVT(_SET)], 1);
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 1);
    purc_variant_unref(var);

    purc_variant_unref(var);
    purc_variant_unref(str);
    ASSERT_EQ(stat->nr_values[PVT(_SET)], 0);
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 0);

#if 0
    // testing anonymous object
    var = purc_variant_make_set(1,
            purc_variant_make_set(1,
                purc_variant_make_null()));
    ASSERT_NE(var, nullptr);
    purc_variant_unref(var);
#endif // 0
    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(variant_set, non_object)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    const char *elems[] = {
        "hello",
        "world",
        "foo",
        "bar",
        "great",
        "wall",
    };

    const size_t idx_to_set = 3;
    const char *s_to_set = "foobar";

    purc_variant_t set;
    set = purc_variant_make_set_by_ckey(0, NULL, PURC_VARIANT_INVALID);
    ASSERT_NE(set, PURC_VARIANT_INVALID);

    if (1) {
        for (size_t i=0; i<PCA_TABLESIZE(elems); ++i) {
            const char *elem = elems[i];
            purc_variant_t s;
            s = purc_variant_make_string_static(elem, false);
            ASSERT_NE(s, PURC_VARIANT_INVALID);
            bool ok = purc_variant_set_add(set, s, false);
            ASSERT_FALSE(ok);
            purc_variant_unref(s);
        }

        purc_variant_t v;
        v = purc_variant_make_string_static(s_to_set, false);
        ASSERT_NE(v, PURC_VARIANT_INVALID);
        bool ok = purc_variant_set_set_by_index(set, idx_to_set, v);
        ASSERT_FALSE(ok);
        purc_variant_unref(v);
    }

    purc_variant_unref(set);
    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(variant_set, init_0_elem)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    purc_variant_t var = purc_variant_make_set_by_ckey(0, "hello", NULL);
    ASSERT_NE(var, nullptr);
    ASSERT_EQ(stat->nr_values[PVT(_SET)], 1);
    ASSERT_EQ(var->refc, 1);

    ASSERT_TRUE(sanity_check(var));

    purc_variant_ref(var);
    ASSERT_EQ(stat->nr_values[PVT(_SET)], 1);
    purc_variant_unref(var);

    purc_variant_unref(var);
    ASSERT_EQ(stat->nr_values[PVT(_SET)], 0);

#if 0
    // testing anonymous object
    var = purc_variant_make_set(1,
            purc_variant_make_set(1,
                purc_variant_make_null()));
    ASSERT_NE(var, nullptr);
    purc_variant_unref(var);
#endif // 0
    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(variant_set, add_1_str)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    purc_variant_t var = purc_variant_make_set_by_ckey(0, "hello", NULL);
    ASSERT_NE(var, nullptr);
    ASSERT_EQ(stat->nr_values[PVT(_SET)], 1);
    ASSERT_EQ(var->refc, 1);

    ASSERT_TRUE(sanity_check(var));

    purc_variant_t s = purc_variant_make_string("world", false);
    ASSERT_NE(s, nullptr);
    purc_variant_t obj;
    obj = purc_variant_make_object_by_static_ckey(1, "hello", s);
    ASSERT_NE(obj, nullptr);
    ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 1);
    bool t = purc_variant_set_add(var, obj, false);
    ASSERT_EQ(t, true);

    ASSERT_TRUE(sanity_check(var));

    purc_variant_unref(obj);
    purc_variant_unref(s);
    ASSERT_EQ(obj->refc, 1);

    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 2);
    ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 1);
    ASSERT_EQ(stat->nr_values[PVT(_SET)], 1);

    ASSERT_EQ(var->refc, 1);
    purc_variant_unref(var);
    ASSERT_EQ(stat->nr_values[PVT(_SET)], 0);
    ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 0);
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 0);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(variant_set, add_n_str)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    purc_variant_t var = purc_variant_make_set_by_ckey(0, "hello", NULL);
    ASSERT_NE(var, nullptr);
    ASSERT_EQ(stat->nr_values[PVT(_SET)], 1);
    ASSERT_EQ(var->refc, 1);

    ASSERT_TRUE(sanity_check(var));

    int count = 1024;
    char buf[64];
    for (int j=0; j<count; ++j) {
        snprintf(buf, sizeof(buf), "%d", j);
        purc_variant_t s = purc_variant_make_string(buf, false);
        ASSERT_NE(s, nullptr);
        purc_variant_t obj;
        obj = purc_variant_make_object_by_static_ckey(1, "hello", s);
        ASSERT_NE(obj, nullptr);
        ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], j+1);
        bool t = purc_variant_set_add(var, obj, false);
        ASSERT_EQ(t, true);

        ASSERT_TRUE(sanity_check(var));

        purc_variant_unref(obj);
        purc_variant_unref(s);
        ASSERT_EQ(obj->refc, 1);
    }
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], count*2);
    ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], count);
    ASSERT_EQ(stat->nr_values[PVT(_SET)], 1);

    int j = 0;
    purc_variant_set_iterator *it;
    bool having = true;
    for (it = purc_variant_set_make_iterator_begin(var);
         it && having;
         having = purc_variant_set_iterator_next(it))
    {
        ++j;
    }
    ASSERT_EQ(j, count);
    if (it)
        purc_variant_set_release_iterator(it);

    having = true;
    for (it = purc_variant_set_make_iterator_begin(var);
         it && having;
         having = purc_variant_set_iterator_next(it))
    {
        purc_variant_t v = purc_variant_set_iterator_get_value(it);
        ASSERT_NE(v, nullptr);
        ASSERT_EQ(v->type, PVT(_OBJECT));
        ASSERT_EQ(v->refc, 1);
        if (1) {
            bool ok = purc_variant_set_remove(var, v, false);
            ASSERT_EQ(ok, true);

            ASSERT_TRUE(sanity_check(var));
        }
        break;
    }
    if (it)
        purc_variant_set_release_iterator(it);

    if (1) {
        purc_variant_t v;
        purc_variant_t q = purc_variant_make_string("20", false);
        v = purc_variant_set_get_member_by_key_values(var, q);
        ASSERT_NE(v, nullptr);
        purc_variant_unref(q);
        q = purc_variant_make_string("abc", false);
        v = purc_variant_set_get_member_by_key_values(var, q);
        ASSERT_EQ(v, nullptr);
        purc_variant_unref(q);

        ASSERT_TRUE(sanity_check(var));
    }

    if (1) {
        purc_variant_t v;
        purc_variant_t q = purc_variant_make_string("20", false);
        v = purc_variant_set_remove_member_by_key_values(var, q);
        ASSERT_NE(v, nullptr);
        purc_variant_unref(v);
        purc_variant_unref(q);
        q = purc_variant_make_string("abc", false);
        v = purc_variant_set_get_member_by_key_values(var, q);
        ASSERT_EQ(v, nullptr);
        purc_variant_unref(q);

        ASSERT_TRUE(sanity_check(var));
    }

    // int idx = _get_random(count);
    // snprintf(buf, sizeof(buf), "%d", idx);
    // purc_variant_t k = purc_variant_make_string(buf, false);
    // purc_variant_t v;
    // v = purc_variant_set_get_member_by_key_values(var, k);
    // ASSERT_NE(v, nullptr);
    // purc_variant_unref(v);
    // purc_variant_unref(k);

    ASSERT_EQ(var->refc, 1);
    purc_variant_unref(var);
    ASSERT_EQ(stat->nr_values[PVT(_SET)], 0);
    ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 0);
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 0);

#if 0
    // testing anonymous object
    var = purc_variant_make_set(1,
            purc_variant_make_set(1,
                purc_variant_make_null()));
    ASSERT_NE(var, nullptr);
    purc_variant_unref(var);
#endif // 0
    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(variant_set, dup)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    purc_variant_t set = purc_variant_make_set_by_ckey(0, "hello",
            PURC_VARIANT_INVALID);
    ASSERT_NE(set, nullptr);

    if (1) {
        purc_variant_t v;
        v = purc_variant_make_object_by_static_ckey(0,
                NULL, PURC_VARIANT_INVALID);
        ASSERT_NE(v, nullptr);
        bool ok;
        ok = purc_variant_set_add(set, v, true);
        purc_variant_unref(v);
        ASSERT_TRUE(ok);
    }
    if (1) {
        purc_variant_t v;
        v = purc_variant_make_object_by_static_ckey(0,
                NULL, PURC_VARIANT_INVALID);
        ASSERT_NE(v, nullptr);
        bool ok;
        ok = purc_variant_set_add(set, v, true);
        purc_variant_unref(v);
        ASSERT_TRUE(ok);
    }
    if (1) {
        purc_variant_t foo = purc_variant_make_string_static("foo", false);
        ASSERT_NE(foo, nullptr);
        purc_variant_t v;
        v = purc_variant_make_object_by_static_ckey(1,
                "hello", foo);
        purc_variant_unref(foo);
        ASSERT_NE(v, nullptr);
        bool ok;
        ok = purc_variant_set_add(set, v, true);
        ASSERT_TRUE(ok);
        ok = purc_variant_set_add(set, v, true);
        purc_variant_unref(v);
        ASSERT_TRUE(ok);
    }

    purc_variant_unref(set);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

static inline purc_variant_t
make_set(const int *vals, size_t nr)
{
    purc_variant_t set = purc_variant_make_set_by_ckey(0,
            NULL, PURC_VARIANT_INVALID);
    if (set == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    bool ok = true;

    for (size_t i=0; i<nr; ++i) {
        purc_variant_t v;
        v = purc_variant_make_longint(vals[i]);
        if (v == PURC_VARIANT_INVALID) {
            ok = false;
            break;
        }
        purc_variant_t o;
        o = purc_variant_make_object_by_static_ckey(1, "id", v);
        purc_variant_unref(v);
        if (o == PURC_VARIANT_INVALID) {
            ok = false;
            break;
        }
        ok = purc_variant_set_add(set, o, true);
        purc_variant_unref(o);
        if (!ok)
            break;
    }

    if (!ok) {
        purc_variant_unref(set);
        return PURC_VARIANT_INVALID;
    }

    return set;
}

static inline int
cmp(size_t nr_keynames,
        purc_variant_t l[], purc_variant_t r[], void *ud)
{
    (void)ud;
    for (size_t i=0; i<nr_keynames; ++i) {
        char lbuf[1024], rbuf[1024];
        purc_variant_stringify(lbuf, sizeof(lbuf), l[i]);
        purc_variant_stringify(rbuf, sizeof(rbuf), r[i]);
        int r = strcmp(lbuf, rbuf);
        if (r)
            return r;
    }

    return 0;
}

TEST(variant_set, sort)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    const int ins[] = {
        3,2,4,1,7,9,6,8,5
    };
    const int outs[] = {
        1,2,3,4,5,6,7,8,9
    };

    char inbuf[8192]; {
        purc_variant_t set = make_set(ins, PCA_TABLESIZE(ins));
        ASSERT_NE(set, nullptr);

        int r = pcvariant_set_sort(set, NULL, cmp);
        ASSERT_EQ(r, 0);

        r = purc_variant_stringify(inbuf, sizeof(inbuf), set);
        ASSERT_GT(r, 0);

        purc_variant_unref(set);
    }

    char outbuf[8192]; {
        purc_variant_t set = make_set(outs, PCA_TABLESIZE(outs));
        ASSERT_NE(set, nullptr);

        int r = purc_variant_stringify(outbuf, sizeof(outbuf), set);
        ASSERT_GT(r, 0);

        purc_variant_unref(set);
    }

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);

    ASSERT_STREQ(inbuf, outbuf);
}

static inline purc_variant_t
make_generic_set(size_t nr, ...)
{
    purc_variant_t obj;
    obj = purc_variant_make_object_by_static_ckey(0,
            NULL, PURC_VARIANT_INVALID);
    if (obj == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    bool ok = true;
    va_list ap;
    va_start(ap, nr);
    for (size_t i=0; i<nr; ++i) {
        const char *k = va_arg(ap, const char*);
        const char *v = va_arg(ap, const char*);
        if (!k || !v) {
            ok = false;
            break;
        }
        purc_variant_t val = purc_variant_make_string_static(v, false);
        if (val == PURC_VARIANT_INVALID) {
            ok = false;
            break;
        }
        ok = purc_variant_object_set_by_static_ckey(obj, k, val);
        purc_variant_unref(val);
        if (!ok)
            break;
    }
    va_end(ap);

    if (!ok) {
        purc_variant_unref(obj);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t set;
    set = purc_variant_make_set_by_ckey(0, NULL, PURC_VARIANT_INVALID);
    if (set == PURC_VARIANT_INVALID) {
        purc_variant_unref(obj);
        return PURC_VARIANT_INVALID;
    }

    ok = purc_variant_set_add(set, obj, false);
    purc_variant_unref(obj);
    if (!ok) {
        purc_variant_unref(set);
        return PURC_VARIANT_INVALID;
    }

    return set;
}

TEST(variant_set, generic)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    char inbuf[8192]; {
        purc_variant_t set = make_generic_set(2, "id", "1", "name", "foo");
        ASSERT_NE(set, nullptr);

        int r = purc_variant_stringify(inbuf, sizeof(inbuf), set);
        ASSERT_GT(r, 0);

        purc_variant_unref(set);
    }

    char outbuf[8192]; {
        purc_variant_t set = make_generic_set(2, "name", "foo", "id", "1");
        ASSERT_NE(set, nullptr);

        int r = purc_variant_stringify(outbuf, sizeof(outbuf), set);
        ASSERT_GT(r, 0);

        purc_variant_unref(set);
    }

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);

    ASSERT_STREQ(inbuf, outbuf);
}

TEST(variant_set, constraint_non_mutable_keyval)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    const char *s;
    purc_variant_t set;
    s = "{!name, {name:'foo', count:3}}";
    set = pcejson_parser_parse_string(s, 1, 1);
    ASSERT_NE(set, nullptr);
    purc_variant_unref(set);

    s = "{!name, {name:[], count:3}}";
    set = pcejson_parser_parse_string(s, 1, 1);
    ASSERT_EQ(set, nullptr);

    s = "{!name, {name:{}, count:3}}";
    set = pcejson_parser_parse_string(s, 1, 1);
    ASSERT_EQ(set, nullptr);

    s = "{!, {name:{}, count:3}}";
    set = pcejson_parser_parse_string(s, 1, 1);
    ASSERT_EQ(set, nullptr);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(variant_set, constraint_non_valid_set)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    const char *s;
    purc_variant_t set, v;
    s = "{!name, {name:'foo', count:3}}";
    set = pcejson_parser_parse_string(s, 1, 1);
    ASSERT_NE(set, nullptr);
    purc_variant_unref(set);

    s = "{!attr, {name:'foo', count:3}, {name:'bar', count:4}}";
    set = pcejson_parser_parse_string(s, 1, 1);
    ASSERT_NE(set, nullptr);
    ASSERT_EQ(1, purc_variant_set_get_size(set));
    purc_variant_unref(set);

    s = "{!'name attr', {name:'foo', count:3}, {name:'bar', count:4}}";
    set = pcejson_parser_parse_string(s, 1, 1);
    ASSERT_NE(set, nullptr);
    ASSERT_EQ(2, purc_variant_set_get_size(set));
    purc_variant_unref(set);

    s = "{!'name attr', {name:'foo', count:3}, {name:'foo', count:4}}";
    set = pcejson_parser_parse_string(s, 1, 1);
    ASSERT_NE(set, nullptr);
    ASSERT_EQ(1, purc_variant_set_get_size(set));
    purc_variant_unref(set);

    s = "{!'name count', {name:'foo', count:3}, {name:'foo', count:4}}";
    set = pcejson_parser_parse_string(s, 1, 1);
    ASSERT_NE(set, nullptr);
    ASSERT_EQ(2, purc_variant_set_get_size(set));
    purc_variant_unref(set);

    s = "{!'name', {name:'foo', count:3}, {name:'bar', count:4}}";
    set = pcejson_parser_parse_string(s, 1, 1);
    ASSERT_NE(set, nullptr);
    ASSERT_EQ(2, purc_variant_set_get_size(set));
    foreach_value_in_variant_set(set, v)
        ASSERT_TRUE(purc_variant_is_object(v));
        v = purc_variant_object_get_by_ckey(v, "name", false);
        ASSERT_NE(v, nullptr);
        ASSERT_TRUE(purc_variant_is_string(v));
        const char *s = purc_variant_get_string_const(v);
        if (strcmp(s, "foo")==0) {
            purc_variant_t x = purc_variant_make_string_static("bar", false);
            ASSERT_NE(x, nullptr);
            ASSERT_FALSE(purc_variant_object_set_by_static_ckey(v, "name", x));
            purc_variant_unref(x);
        }
    end_foreach;
    ASSERT_EQ(2, purc_variant_set_get_size(set));
    purc_variant_unref(set);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

