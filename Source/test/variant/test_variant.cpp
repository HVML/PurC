#include "private/arraylist.h"
#include "private/hashtable.h"
#include "purc-variant.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

/* test variant.pcutils_arrlist_double_free */
static size_t _arrlist_items_free = 0;
static void _arrlist_item_free(void *data)
{
    free(data);
    ++_arrlist_items_free;
}

TEST(variant, pcutils_arrlist_double_free)
{
    int t;
    // reset
    _arrlist_items_free = 0;

    struct pcutils_arrlist *al = pcutils_arrlist_new_ex(_arrlist_item_free, 3);

    char *s1 = strdup("hello");
    t = pcutils_arrlist_put_idx(al, 0, s1);
    ASSERT_EQ(t, 0);
    // double-free intentionally
    t = pcutils_arrlist_put_idx(al, 0, s1);
    ASSERT_EQ(t, 0);

    pcutils_arrlist_free(al);

    // test check
    ASSERT_EQ(_arrlist_items_free, 1);
}

/* test variant.pchash_table_double_free */
static size_t _hash_table_items_free = 0;
static void _hash_table_item_free(pchash_entry *e)
{
    free(pchash_entry_v(e));
    ++_hash_table_items_free;
}

TEST(variant, pchash_table_double_free)
{
    int t;
    // reset
    _hash_table_items_free = 0;

    struct pchash_table *ht = pchash_kchar_table_new(3, _hash_table_item_free);

    char *s1 = strdup("hello");
    t = pchash_table_insert(ht, "hello", s1);
    ASSERT_EQ(t, 0);

    t = pchash_table_insert(ht, "hello", s1);
    ASSERT_EQ(t, 0);

    pchash_table_free(ht);

    // test check
    ASSERT_EQ(_hash_table_items_free, 1);
}

