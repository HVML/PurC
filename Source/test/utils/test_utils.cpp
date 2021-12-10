#include "purc.h"
#include "private/list.h"
#include "private/avl.h"
#include "private/hashtable.h"
#include "private/map.h"
#include "private/rbtree.h"
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

/* test arrlist.double_free */
static size_t _arrlist_items_free = 0;
static void _arrlist_item_free(void *data)
{
    free(data);
    ++_arrlist_items_free;
}

TEST(arrlist, double_free)
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

/* test hashtable.double_free */
static size_t _hash_table_items_free = 0;
static void _hash_table_item_free(pchash_entry *e)
{
    free(pchash_entry_k(e));
    free(pchash_entry_v(e));
    ++_hash_table_items_free;
}

TEST(hashtable, double_free)
{
    int t;
    // reset
    _hash_table_items_free = 0;

    struct pchash_table *ht = pchash_kchar_table_new(3, _hash_table_item_free);

    const char *k1 = "hello";
    t = pchash_table_insert(ht, strdup(k1), strdup(k1));
    ASSERT_EQ(t, 0);

    struct pchash_entry *e = pchash_table_lookup_entry(ht, k1);
    EXPECT_NE(e, nullptr);
    const char *kk = (const char*)pchash_entry_k(e);
    ASSERT_NE(k1, kk);
    EXPECT_STREQ(k1, kk);
    ASSERT_EQ(0, strcmp(k1, kk));
    pchash_table_free(ht);

    // test check
    ASSERT_EQ(_hash_table_items_free, 1);
}

struct string_s {
    struct list_head      list;
    char                 *s;
};

struct strings_s {
    struct list_head      head;
};

TEST(utils, list)
{
    struct strings_s     strings;
    INIT_LIST_HEAD(&strings.head);
    for (int i=0; i<10; ++i) {
        struct string_s *str = (struct string_s*)malloc(sizeof(*str));
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", i+1);
        str->s = strdup(buf);
        list_add_tail(&str->list, &strings.head);
    }

    struct list_head *p, *n;
    int i = 0;
    list_for_each(p, &strings.head) {
        struct string_s *str = list_entry(p, struct string_s, list);
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", ++i);
        EXPECT_STREQ(buf, str->s);
    }

    i = 0;
    list_for_each_safe(p, n, &strings.head) {
        struct string_s *str = list_entry(p, struct string_s, list);
        n = str->list.next;
        list_del_init(&str->list);
        free(str->s);
        free(str);
    }
}

struct name_s {
    struct avl_node   node;
    char             *s;
};

struct names_s {
    struct avl_tree   root;
};

static int _avl_cmp (const void *k1, const void *k2, void *ptr) {
    const char *s1 = (const char*)k1;
    const char *s2 = (const char*)k2;
    (void)ptr;
    return strcmp(s1, s2);
}

TEST(utils, avl)
{
    struct names_s names;
    pcutils_avl_init(&names.root, _avl_cmp, false, nullptr);
    for (int i=0; i<10; ++i) {
        struct name_s *name = (struct name_s*)malloc(sizeof(*name));
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", i+1);
        name->s = strdup(buf);
        name->node.key = name->s;
        int t = pcutils_avl_insert(&names.root, &name->node);
        ASSERT_EQ(t, 0);
    }

    struct name_s *name;
    avl_for_each_element(&names.root, name, node) {
        fprintf(stderr, "%s\n", name->s);
    }

    name = avl_find_element(&names.root, "9", name, node);
    ASSERT_NE(name, nullptr);
    ASSERT_STREQ((const char*)name->node.key, "9");

    struct name_s *ptr;
    avl_for_each_element_safe(&names.root, name, node, ptr) {
        ptr = avl_next_element(name, node);
        free(name->s);
        free(name);
    }
}

struct str_node {
    struct rb_node  node;
    const char     *str;
};

int cmp(const void *key, struct rb_node *node)
{
    struct str_node *p = container_of(node, struct str_node, node);
    const char *k = (const char*)key;
    return strcmp(k, p->str);
}

static inline void
do_insert(struct rb_root *root, const char *str, bool *ok)
{
    struct rb_node **pnode = &root->rb_node;
    struct rb_node *parent = NULL;
    struct rb_node *entry = NULL;
    while (*pnode) {
        int ret = cmp(str, *pnode);

        parent = *pnode;

        if (ret < 0)
            pnode = &parent->rb_left;
        else if (ret > 0)
            pnode = &parent->rb_right;
        else{
            entry = *pnode;
            break;
        }
    }

    if (!entry) { //new the entry
        struct str_node *snode = (struct str_node*)calloc(1, sizeof(*snode));
        if (!snode) {
            *ok = false;
            return;
        }
        snode->str = str;
        entry = &snode->node;

        pcutils_rbtree_link_node(entry, parent, pnode);
        pcutils_rbtree_insert_color(entry, root);
        *ok = true;
        return;
    }

    struct str_node *p = container_of(entry, struct str_node, node);
    p->str = str;
    *ok = true;
}

TEST(utils, rbtree)
{
    const char *samples[] = {
        "hello",
        "world",
        "foo",
        "bar",
        "great",
        "wall",
    };

    bool ok = true;

    struct rb_root root = RB_ROOT;
    struct rb_node *node;
    node = pcutils_rbtree_first(&root);
    ASSERT_EQ(node, nullptr);

    for (size_t i=0; i<PCA_TABLESIZE(samples); ++i) {
        const char *sample = samples[i];
        do_insert(&root, sample, &ok);
        if (!ok)
            break;
    }

    node = pcutils_rbtree_first(&root);
    for (; node; node = pcutils_rbtree_next(node)) {
        struct str_node *p = container_of(node, struct str_node, node);
        ASSERT_NE(p, nullptr);
    }

    node = pcutils_rbtree_first(&root);
    struct rb_node *next;
    for (; ({node && (next = pcutils_rbtree_next(node)); node;}); node=next) {
        struct str_node *p = container_of(node, struct str_node, node);
        pcutils_rbtree_erase(node, &root);
        free(p);
    }

    ASSERT_TRUE(ok);
}

static inline int
map_cmp(const void *key1, const void *key2)
{
    const char *s1 = (const char*)key1;
    const char *s2 = (const char*)key2;
    return strcmp(s1, s2);
}

static inline int
map_visit(void *key, void *val, void *ud)
{
    (void)ud;

    int r;
    r = strcmp((const char*)key, "name");
    if (r)
        return r;

    size_t v = (size_t)val;
    if (v < 12)
        return -1;
    if (v == 12)
        return 0;
    return 1;
}

TEST(utils, map)
{
    pcutils_map *map;
    map = pcutils_map_create(NULL, NULL, NULL, NULL,
            map_cmp, false);
    ASSERT_NE(map, nullptr);

    pcutils_map_entry *entry;
    int r;

    r = pcutils_map_insert(map, "name", (const void*)(size_t)1);
    ASSERT_EQ(r, 0);
    entry = pcutils_map_find(map, "name");
    ASSERT_NE(entry, nullptr);
    ASSERT_EQ((const char*)entry->key, "name");
    ASSERT_EQ((size_t)entry->val, 1);

    r = pcutils_map_insert(map, "name", (const void*)(size_t)12);
    ASSERT_EQ(r, 0);
    entry = pcutils_map_find(map, "name");
    ASSERT_NE(entry, nullptr);
    ASSERT_EQ((const char*)entry->key, "name");
    ASSERT_EQ((size_t)entry->val, 12);

    r = pcutils_map_traverse(map, NULL, map_visit);
    pcutils_map_destroy(map);
    ASSERT_EQ(r, 0);
}

