#include "purc.h"
#include "private/list.h"
#include "private/avl.h"
#include "private/sorted-array.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

// to test basic functions of atom
TEST(utils, atom_basic)
{
    int ret = purc_init ("cn.fmsoft.hybridos.test", "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);
    char buff [] = "HVML";
    const char *str;

    purc_atom_t atom;

    atom = purc_atom_from_static_string(NULL);
    ASSERT_EQ(atom, 0);

    atom = purc_atom_from_string(NULL);
    ASSERT_EQ(atom, 0);

    atom = purc_atom_from_static_string("HVML");
    ASSERT_EQ(atom, 1);

    str = purc_atom_to_string(1);
    ASSERT_STREQ(str, "HVML");

    atom = purc_atom_try_string("HVML");
    ASSERT_EQ(atom, 1);

    atom = purc_atom_try_string("PurC");
    ASSERT_EQ(atom, 0);

    atom = purc_atom_try_string(NULL);
    ASSERT_EQ(atom, 0);

    atom = purc_atom_from_string(buff);
    ASSERT_EQ(atom, 1);

    atom = purc_atom_from_string("PurC");
    ASSERT_EQ(atom, 2);

    atom = purc_atom_try_string("HVML");
    ASSERT_EQ(atom, 1);

    atom = purc_atom_try_string("PurC");
    ASSERT_EQ(atom, 2);

    str = purc_atom_to_string(1);
    ASSERT_STREQ(str, "HVML");

    str = purc_atom_to_string(2);
    ASSERT_STREQ(str, "PurC");

    str = purc_atom_to_string(3);
    ASSERT_EQ(str, nullptr);

    atom = purc_atom_try_string(NULL);
    ASSERT_EQ(atom, 0);

    purc_cleanup ();
}

// to test sorted array
static int sortv[10] = { 1, 8, 7, 5, 4, 6, 9, 0, 2, 3 };

static int
intcmp(const void *sortv1, const void *sortv2)
{
    int i = (int)(intptr_t)sortv1;
    int j = (int)(intptr_t)sortv2;

    return i - j;
}

TEST(utils, sorted_array_asc)
{
    struct sorted_array *sa;
    int n;

    sa = sorted_array_create(SAFLAG_DEFAULT, 4, NULL,
            intcmp);

    n = (int)sorted_array_count (sa);
    ASSERT_EQ(n, 0);

    for (int i = 0; i < 10; i++) {
        int ret = sorted_array_add (sa, (void *)(intptr_t)sortv[i],
                (void *)(intptr_t)(sortv[i] + 100));
        ASSERT_EQ(ret, 0);
    }

    n = (int)sorted_array_count (sa);
    ASSERT_EQ(n, 10);

    for (int i = 0; i < n; i++) {
        void *data;
        int sortv = (int)(intptr_t)sorted_array_get (sa, i, &data);

        ASSERT_EQ((int)(intptr_t)data, sortv + 100);
        ASSERT_EQ(sortv, i);
    }

    sorted_array_remove (sa, (void *)(intptr_t)0);
    sorted_array_remove (sa, (void *)(intptr_t)9);
    sorted_array_delete (sa, 0);

    n = (int)sorted_array_count (sa);
    ASSERT_EQ(n, 7);

    for (int i = 0; i < n; i++) {
        void *data;
        int sortv = (int)(intptr_t)sorted_array_get (sa, i, &data);

        ASSERT_EQ((int)(intptr_t)data, sortv + 100);
        ASSERT_EQ(sortv, i + 2);
    }

    sorted_array_destroy(sa);
}

TEST(utils, sorted_array_desc)
{
    struct sorted_array *sa;
    int n;

    sa = sorted_array_create(SAFLAG_ORDER_DESC, 4, NULL,
            intcmp);

    n = (int)sorted_array_count (sa);
    ASSERT_EQ(n, 0);

    for (int i = 0; i < 10; i++) {
        int ret = sorted_array_add (sa, (void *)(intptr_t)sortv[i],
                (void *)(intptr_t)(sortv[i] + 100));
        ASSERT_EQ(ret, 0);
    }

    n = (int)sorted_array_count (sa);
    ASSERT_EQ(n, 10);

    for (int i = 0; i < n; i++) {
        void *data;
        int sortv = (int)(intptr_t)sorted_array_get (sa, i, &data);

        ASSERT_EQ((int)(intptr_t)data, sortv + 100);
        sortv = 9 - sortv;
        ASSERT_EQ(sortv, i);
    }

    sorted_array_remove (sa, (void *)(intptr_t)0);
    sorted_array_remove (sa, (void *)(intptr_t)9);

    n = (int)sorted_array_count (sa);
    ASSERT_EQ(n, 8);

    for (int i = 0; i < n; i++) {
        void *data;
        int sortv = (int)(intptr_t)sorted_array_get (sa, i, &data);

        ASSERT_EQ((int)(intptr_t)data, sortv + 100);
        sortv = 8 - sortv;
        ASSERT_EQ(sortv, i);
    }

    bool found;
    found = sorted_array_find (sa, (void *)(intptr_t)0, NULL);
    ASSERT_EQ(found, false);
    found = sorted_array_find (sa, (void *)(intptr_t)9, NULL);
    ASSERT_EQ(found, false);

    for (int i = 1; i < 9; i++) {
        void *data;
        found = sorted_array_find (sa, (void *)(intptr_t)i, &data);

        ASSERT_EQ(found, true);
        ASSERT_EQ((int)(intptr_t)data, i + 100);
    }

    sorted_array_destroy(sa);
}

struct node
{
    struct list_head          node;
    int                       v;
};

TEST(utils, list_head)
{
    struct list_head list;
    INIT_LIST_HEAD(&list);

    struct node n1, n2, n3, n4;
    n1.v = 1;
    n2.v = 2;
    n3.v = 3;
    n4.v = 4;

    list_add_tail(&n1.node, &list);
    list_add_tail(&n2.node, &list);
    list_add_tail(&n3.node, &list);
    list_add_tail(&n4.node, &list);

    int v = 1;
    struct list_head *p;
    list_for_each(p, &list) {
        struct node *node;
        node = container_of(p, struct node, node);
        ASSERT_EQ(node->v, v++);
    }
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
    (void)ptr;
    int delta = (*(size_t*)k1) - (*(size_t*)k2);
    return delta;
}

static int _get_random(int max)
{
    static int seeded = 0;
    if (!seeded) {
        srand(time(0));
        seeded = 1;
    }

    if (max==0)
        return 0;

    return (max<0) ? rand() : rand() % max;
}

TEST(avl, init)
{
    struct avl_tree avl;
    pcutils_avl_init(&avl, _avl_tree_comp, false, NULL);
    int r;
    int count = 10240;
    for (int i=0; i<count; ++i) {
        size_t key = _get_random(-1);
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
