#include "purc.h"
#include "private/avl.h"
#include "private/arraylist.h"
#include "private/hashtable.h"
#include "purc-variant.h"
#include "private/variant.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

TEST(variant_set, init_with_1_str)
{
    purc_instance_extra_info info = {0, 0};
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

    purc_variant_t var = purc_variant_make_set_c(0, "hello", NULL);
    ASSERT_EQ(stat->nr_values[PVT(_SET)], 1);
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], 1);

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

TEST(variant_set, init_0_elem)
{
    purc_instance_extra_info info = {0, 0};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    purc_variant_t var = purc_variant_make_set_c(0, "hello", NULL);
    ASSERT_NE(var, nullptr);
    ASSERT_EQ(stat->nr_values[PVT(_SET)], 1);
    ASSERT_EQ(var->refc, 1);

    // purc_variant_ref(var);
    // ASSERT_EQ(stat->nr_values[PVT(_SET)], 1);
    // ASSERT_EQ(stat->nr_values[PVT(_STRING)], 1);
    // purc_variant_unref(var);

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

TEST(variant_set, add_n_str)
{
    purc_instance_extra_info info = {0, 0};
    int ret = 0;
    bool cleanup = false;
    struct purc_variant_stat *stat;

    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    stat = purc_variant_usage_stat();
    ASSERT_NE(stat, nullptr);

    purc_variant_t var = purc_variant_make_set_c(0, "hello", NULL);
    ASSERT_NE(var, nullptr);
    ASSERT_EQ(stat->nr_values[PVT(_SET)], 1);
    ASSERT_EQ(var->refc, 1);

    int count = 100;
    for (int j=0; j<count; ++j) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%d", j);
        purc_variant_t s = purc_variant_make_string(buf, false);
        ASSERT_NE(s, nullptr);
        purc_variant_t obj = purc_variant_make_object_c(1, "hello", s);
        ASSERT_NE(obj, nullptr);
        int t = purc_variant_set_add(var, obj);
        ASSERT_EQ(t, true);
        purc_variant_unref(obj);
        purc_variant_unref(s);
    }
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], count);
    ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], count);
    ASSERT_EQ(stat->nr_values[PVT(_SET)], 1);

    // int j = 0;
    // purc_variant_t val;
    // foreach_value_in_variant_set(var, val)
    //     ASSERT_EQ(val->type, PVT(_OBJECT));
    //     ++j;
    // end_foreach;
    // ASSERT_EQ(j, count);

    ASSERT_EQ(var->refc, 1);
    purc_variant_unref(var);
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


struct _avl_node {
    struct avl_node          node;
    size_t                   key;
    size_t                   val;
};
static struct _avl_node* _make_avl_node(int key, int val)
{
    struct _avl_node *p;
    p = (struct _avl_node*)calloc(1, sizeof(*p));
    if (!p) return nullptr;
    p->key = key;
    p->val = val;
    p->node.key = &p->key;
    return p;
}

static int _avl_tree_comp(const void *k1, const void *k2, void *ptr)
{
    UNUSED_PARAM(ptr);
    int delta = (*(size_t*)k1) - (*(size_t*)k2);
    return delta;
}

TEST(avl, init)
{
    struct avl_tree avl;
    pcutils_avl_init(&avl, _avl_tree_comp, false, NULL);
    int r;
    srand(time(0));
    int count = 10240;
    for (int i=0; i<count; ++i) {
        size_t key = rand();
        if (pcutils_avl_find(&avl, &key)) {
            --i;
            continue;
        }
        struct _avl_node *p = _make_avl_node(key, rand());
        r = pcutils_avl_insert(&avl, &p->node);
        ASSERT_EQ(r, 0);
    }
    struct _avl_node *p, *tmp;
    int i = 0;
    size_t prev;
    avl_remove_all_elements(&avl, p, node, tmp) {
        if (i>0) {
            ASSERT_GT(p->key, prev);
        }
        prev = p->key;
        free(p);
    }
}

