#include "purc.h"
#include "private/debug.h"
#include "private/errors.h"
#include "private/variant.h"

#include "../helpers.h"

#include <gtest/gtest.h>

static void*
ref(const void *v)
{
    return purc_variant_ref((purc_variant_t)v);
}

static void
unref(void *v)
{
    purc_variant_unref((purc_variant_t)v);
}

static int
cmp(const void *l, const void *r)
{
    return pcvariant_diff((purc_variant_t)l, (purc_variant_t)r);
}

TEST(variant, map)
{
    PurCInstance purc;

    int r;
    purc_variant_t k, v;
    pcutils_map *map;
    pcutils_map_entry *entry;

    map = pcutils_map_create(ref, unref, ref, unref, cmp, false);

    k = purc_variant_make_string("foo", true);
    v = purc_variant_make_string("bar", true);
    r = pcutils_map_insert(map, k, v);
    ASSERT_EQ(r, 0);

    entry = pcutils_map_find(map, k);
    ASSERT_EQ(entry->val, (void*)v);

    PURC_VARIANT_SAFE_CLEAR(v);

    entry = pcutils_map_find(map, k);
    ASSERT_STREQ(purc_variant_get_string_const((purc_variant_t)entry->val),
            "bar");

    v = purc_variant_make_string("foo", true);
    r = pcutils_map_insert(map, k, v);
    ASSERT_NE(r, 0);

    PURC_VARIANT_SAFE_CLEAR(v);
    PURC_VARIANT_SAFE_CLEAR(k);

    pcutils_map_destroy(map);
}


