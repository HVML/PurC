#include "purc.h"

#include "private/ejson.h"
#include "private/utils.h"
#include "purc-rwstream.h"

#include <stdio.h>
#include <gtest/gtest.h>

struct numberify_record
{
    long double                ld;
    const char                *str;
};

static inline purc_variant_t
load_variant(const char *s)
{
    if (strcmp(s, "undefined")==0)
        return purc_variant_make_undefined();

    if (strcmp(s, "null")==0)
        return purc_variant_make_null();

    if (strcmp(s, "true")==0)
        return purc_variant_make_boolean(true);

    if (strcmp(s, "false")==0)
        return purc_variant_make_boolean(false);

    return purc_variant_make_from_json_string(s, strlen(s));
}

static inline void
do_numberify(struct numberify_record *p)
{
    purc_variant_t v;
    v = load_variant(p->str);
    if (v == PURC_VARIANT_INVALID) {
        EXPECT_NE(v,PURC_VARIANT_INVALID);
        return;
    }

    long double ld = purc_variant_numberify(v);
    purc_variant_unref(v);

    ASSERT_EQ(ld, p->ld) << "[" << p->str << "]";
}

TEST(variant, numberify)
{
    purc_instance_extra_info info = {0, 0};
    int ret;
    bool cleanup;

    // initial purc
    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    struct numberify_record records[] = {
        { 0.0L, "undefined" },
        { 0.0L, "null" },
        { 1.0L, "true" },
        { 0.0L, "false" },
        { 0.0L, "0" },
        { 0.0L, "0.0" },
        { 0.0L, "''" },
        { 0.0L, "' '" },
        { 0.0L, "'0'" },
        { 0.0L, "'0.0'" },
        { 123.34L, "'123.34'" },
        { 0.0L, "'abcd'" },
        { 10.0L, "[1,2,3,4]" },
        { 100.0L, "{'a':10,'b':20,'c':30,'d':40}" },
    };

    for (size_t i=0; i<PCA_TABLESIZE(records); ++i) {
        struct numberify_record *p = records + i;
        do_numberify(p);
    }

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

struct booleanize_record
{
    bool                       b;
    const char                *str;
};

static inline void
do_booleanize(struct booleanize_record *p)
{
    purc_variant_t v;
    v = load_variant(p->str);
    if (v == PURC_VARIANT_INVALID) {
        EXPECT_NE(v,PURC_VARIANT_INVALID);
        return;
    }

    bool b = purc_variant_booleanize(v);
    purc_variant_unref(v);

    ASSERT_EQ(b, p->b) << "[" << p->str << "]";
}

TEST(variant, booleanize)
{
    purc_instance_extra_info info = {0, 0};
    int ret;
    bool cleanup;

    // initial purc
    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    struct booleanize_record records[] = {
        { false, "undefined" },
        { false, "null" },
        { true, "true" },
        { false, "false" },
        { false, "0" },
        { false, "0.0" },
        { false, "''" },
        { false, "' '" },
        { false, "'0'" },
        { false, "'0.0'" },
        { true, "'123.34'" },
        { false, "'abcd'" },
        { true, "[1,2,3,4]" },
        { true, "{'a':10,'b':20,'c':30,'d':40}" },
    };

    for (size_t i=0; i<PCA_TABLESIZE(records); ++i) {
        struct booleanize_record *p = records + i;
        do_booleanize(p);
    }

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

struct stringify_record
{
    const char                *str;
    const char                *chk;
};

static inline void
do_stringify(struct stringify_record *p)
{
    purc_variant_t v;
    v = load_variant(p->str);
    if (v == PURC_VARIANT_INVALID) {
        EXPECT_NE(v,PURC_VARIANT_INVALID);
        return;
    }

    purc_variant_t vs = purc_variant_stringify(v);
    purc_variant_unref(v);
    ASSERT_NE(vs, nullptr) << "[" << p->str << "]";

    const char *s;
    if (purc_variant_is_atomstring(vs)) {
        s = purc_variant_get_atom_string_const(vs);
    }
    else if (purc_variant_is_string(vs)) {
        s = purc_variant_get_string_const(vs);
    }
    else {
        FAIL() << "[" << p->str << "]";
    }

    ASSERT_STREQ(s, p->chk) << "[" << p->str << "]";

    purc_variant_unref(vs);
}

TEST(variant, stringify)
{
    purc_instance_extra_info info = {0, 0};
    int ret;
    bool cleanup;

    // initial purc
    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    struct stringify_record records[] = {
        { "undefined",          "undefined" },
        { "null",               "null" },
        { "true",               "true" },
        { "false",              "false" },
        { "10 ",                  "10" },  // TODO: no tail space required
        { "0.0 ",                "0" },
        { "''",                 "null" },
        { "' '",                " " },
        { "'0'",                "0" },
        { "'0.0'",              "0.0" },
        { "'123.34'",           "123.34" },
        { "'abcd'",             "abcd" },
        { "[1,2,3,4]",          "1\n2\n3\n4\n" },
        { "{'a':10,'b':20,'c':30,'d':40}", "a:10\nb:20\nc:30\nd:40\n" },
        { "[{'id':'1','name': 'Tom', 'age': 2, 'male': true },"
          "{'id':'2','name':'Jerry','age':3,'male':true}]",
          "id:1\nname:Tom\nage:2\nmale:true\n"
          "\n"
          "id:2\nname:Jerry\nage:3\nmale:true\n"
          "\n" },
    };

    for (size_t i=0; i<PCA_TABLESIZE(records); ++i) {
        struct stringify_record *p = records + i;
        do_stringify(p);
    }

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

