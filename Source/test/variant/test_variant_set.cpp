#include "purc.h"
#include "private/avl.h"
#include "private/arraylist.h"
#include "private/hashtable.h"
#include "purc-variant.h"
#include "private/variant.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

static int _get_random(int max)
{
    static int seeded = 0;
    if (!seeded) {
        srand(time(0));
        seeded = 1;
    }

    return (max<=0) ? rand() : rand() % max;
}

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

    int count = 1;
    char buf[64];
    for (int j=0; j<count; ++j) {
        snprintf(buf, sizeof(buf), "%d", j);
        purc_variant_t s = purc_variant_make_string(buf, false);
        ASSERT_NE(s, nullptr);
        purc_variant_t obj = purc_variant_make_object_c(1, "hello", s);
        ASSERT_NE(obj, nullptr);
        ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], 1);
        bool t = purc_variant_set_add(var, obj, false);
        ASSERT_EQ(t, true);
        purc_variant_unref(obj);
        purc_variant_unref(s);
    }
    ASSERT_EQ(stat->nr_values[PVT(_STRING)], count);
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
        bool ok = purc_variant_set_remove(var, v);
        ASSERT_EQ(ok, true);
        break;
    }
    if (it)
        purc_variant_set_release_iterator(it);
    purc_variant_set_release_iterator(NULL);

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
    int count = 10240;
    for (int i=0; i<count; ++i) {
        size_t key = _get_random(0);
        if (pcutils_avl_find(&avl, &key)) {
            --i;
            continue;
        }
        struct _avl_node *p = _make_avl_node(key, _get_random(0));
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

