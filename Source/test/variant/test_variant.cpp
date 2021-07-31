#include "purc.h"
#include "private/avl.h"
#include "private/arraylist.h"
#include "private/hashtable.h"
#include "purc-variant.h"
#include "private/variant.h"

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
    free(pchash_entry_k(e));
    free(pchash_entry_v(e));
    ++_hash_table_items_free;
}

TEST(variant, pchash_table_double_free)
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

TEST(variant, list)
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

/*
void pcutils_avl_init(struct avl_tree *, avl_tree_comp, bool, void *);
struct avl_node *pcutils_avl_find(const struct avl_tree *, const void *);
struct avl_node *pcutils_avl_find_greaterequal(const struct avl_tree *tree, const void *key);
struct avl_node *pcutils_avl_find_lessequal(const struct avl_tree *tree, const void *key);
int pcutils_avl_insert(struct avl_tree *, struct avl_node *);
void pcutils_avl_delete(struct avl_tree *, struct avl_node *);

int pcutils_avl_strcmp(const void *k1, const void *k2, void *ptr);
*/

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

TEST(variant, avl)
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

TEST(variant, pcvariant_init_once)
{
    purc_instance_extra_info info = {0, 0};
    int i = 0;
    // size_t size = sizeof(purc_variant);
    int ret = 0;
    bool cleanup = false;

    // initial purc
    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);

    ASSERT_EQ (ret, PURC_ERROR_OK);

    // get statitics information
    struct purc_variant_stat * stat = purc_variant_usage_stat ();

    ASSERT_NE(stat, nullptr);

    // EXPECT_EQ (stat->nr_values[PURC_VARIANT_TYPE_NULL], 1);
    // EXPECT_EQ (stat->sz_mem[PURC_VARIANT_TYPE_NULL], size);

    // EXPECT_EQ (stat->nr_values[PURC_VARIANT_TYPE_UNDEFINED], 1);
    // EXPECT_EQ (stat->sz_mem[PURC_VARIANT_TYPE_UNDEFINED], size);

    // EXPECT_EQ (stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN], 2);
    // EXPECT_EQ (stat->sz_mem[PURC_VARIANT_TYPE_BOOLEAN], size * 2);

    for (i = PURC_VARIANT_TYPE_NUMBER; i < PURC_VARIANT_TYPE_MAX; i++) {
        EXPECT_EQ (stat->nr_values[i], 0);
        EXPECT_EQ (stat->sz_mem[i], 0);
    } 

    // EXPECT_EQ (stat->nr_total_values, 4);
    // EXPECT_EQ (stat->sz_total_mem, 4 * size);
    // EXPECT_EQ (stat->nr_reserved, 0);
    // EXPECT_EQ (stat->nr_max_reserved, MAX_RESERVED_VARIANTS);


    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

TEST(variant, pcvariant_init_10_times)
{
    purc_instance_extra_info info = {0, 0};
    // int i = 0;
    // size_t size = sizeof(purc_variant);
    int ret = 0;
    bool cleanup = false;
    int times = 0;

    for (times = 0; times < 10; times ++) {
        // initial purc
        ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);

        ASSERT_EQ (ret, PURC_ERROR_OK);

        // get statitics information
        struct purc_variant_stat * stat = purc_variant_usage_stat ();

        ASSERT_NE(stat, nullptr);

        // EXPECT_EQ (stat->nr_values[PURC_VARIANT_TYPE_NULL], 1);
        // EXPECT_EQ (stat->sz_mem[PURC_VARIANT_TYPE_NULL], size);

        // EXPECT_EQ (stat->nr_values[PURC_VARIANT_TYPE_UNDEFINED], 1);
        // EXPECT_EQ (stat->sz_mem[PURC_VARIANT_TYPE_UNDEFINED], size);

        // EXPECT_EQ (stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN], 2);
        // EXPECT_EQ (stat->sz_mem[PURC_VARIANT_TYPE_BOOLEAN], size * 2);

        // for (i = PURC_VARIANT_TYPE_NUMBER; i < PURC_VARIANT_TYPE_MAX; i++) {
        //     EXPECT_EQ (stat->nr_values[i], 0);
        //     EXPECT_EQ (stat->sz_mem[i], 0);
        // } 

        // EXPECT_EQ (stat->nr_total_values, 4);
        // EXPECT_EQ (stat->sz_total_mem, 4 * size);
        // EXPECT_EQ (stat->nr_reserved, 0);
        // EXPECT_EQ (stat->nr_max_reserved, MAX_RESERVED_VARIANTS);

        cleanup = purc_cleanup ();
        ASSERT_EQ (cleanup, true);
    }
}


// to test:
// purc_variant_make_number ()
// purc_variant_serialize ()
TEST(variant, pcvariant_number)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create number variant with valild value, and serialize
    // expected: get the variant

    purc_cleanup ();
}

// to test:
// purc_variant_make_longuint ()
// purc_variant_serialize ()
TEST(variant, pcvariant_longuint)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create longuint variant with valild value, and serialize
    // expected: get the variant

    // create longuint variant with negatives, and serialize
    // expected: get the variant with convert value

    purc_cleanup ();
}

// to test:
// purc_variant_make_longint ()
// purc_variant_serialize ()
TEST(variant, pcvariant_longint)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create longint variant with valild value, and serialize
    // expected: get the variant

    // create longint variant with invalid value, and serialize
    // expected: get the variant with convert value

    purc_cleanup ();
}

// to test:
// purc_variant_make_longdouble ()
// purc_variant_serialize ()
TEST(variant, pcvariant_longdouble)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create longdouble variant with valild value, and serialize
    // expected: get the variant

    purc_cleanup ();
}

// to test:
// purc_variant_make_string ();
// purc_variant_get_string_const ();
// purc_variant_string_length ();
// purc_variant_serialize ()
TEST(variant, pcvariant_string)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create short string variant without checking, input in utf8-encoding
    // expected: get the variant with original string

    // create short string variant without checking, input not in utf8-encoding
    // expected: get the variant with original string

    // create short string variant with checking, input in utf8-encoding
    // expected: get the variant with original string

    // create short string variant with checking, input is not in utf8-encoding
    // expected: get PURC_VARIANT_INVALID

    // create long string variant without checking, input in utf8-encoding
    // expected: get the variant with original string

    // create long string variant without checking, input not in utf8-encoding
    // expected: get the variant with original string

    // create long string variant with checking, input in utf8-encoding
    // expected: get the variant with original string

    // create long string variant with checking, input not in utf8-encoding
    // expected: get PURC_VARIANT_INVALID

    // create short string variant with null pointer
    // expected: return an empty string variant, with size is 0, and pointer is NULL. 

    // create long string variant with null pointer
    // expected: return an empty string variant, with size is 0, and pointer is NULL. 

    // create string variant with extraordinary long string 
    // expected: it is depend on the free memory. 

    purc_cleanup ();
}


// to test:
// purc_variant_make_atom_string ();
// purc_variant_make_atom_string_static ();
// purc_variant_get_atom_string_const ();
// purc_variant_serialize ()
TEST(variant, pcvariant_atom_string)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create atom string variant without checking, input in utf8-encoding,
    //        and check the input pointer and pointer from variant API.
    // expected: get the variant with string, and pointers are not same.

    // create atom string variant without checking, input not in utf8-encoding
    //        and check the input pointer and pointer from variant API.
    // expected: get the variant with string, and pointers are not same.

    // create atom string variant with checking, input in utf8-encoding
    //        and check the input pointer and pointer from variant API.
    // expected: get the variant with string, and pointers are not same.

    // create atom string variant with checking, input is not in utf8-encoding
    // expected: get PURC_VARIANT_INVALID

    // create static atom string variant without checking, input in utf8-encoding
    //        and check the input pointer and pointer from variant API.
    // expected: get the variant with string, and pointers are same.

    // create static atom string variant without checking, input not in utf8-encoding
    //        and check the input pointer and pointer from variant API.
    // expected: get the variant with string, and pointers are same.

    // create static atom string variant with checking, input in utf8-encoding
    //        and check the input pointer and pointer from variant API.
    // expected: get the variant with string, and pointers are same.

    // create static atom string variant with checking, input not in utf8-encoding
    // expected: get PURC_VARIANT_INVALID

    // create atom string variant with null pointer
    // expected: depend on code. 

    // create static atom string variant with null pointer
    // expected: depend on code. 

    // create two atom string variants with same input string, check the atom 
    //        and string pointer whether are equal 
    // expected: atom and string pointer is same. 

    // create two static atom string variants with same input string, check the atom 
    //        and string pointer
    // expected: atom and string pointer is same. 

    purc_cleanup ();
}

// to test:
// purc_variant_make_byte_sequence ();
// purc_variant_get_bytes_const ();
// purc_variant_sequence_length ();
// purc_variant_serialize ()
TEST(variant, pcvariant_sequence)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create short sequence variant
    // expected: get the variant with original byte sequence

    // create long sequence variant
    // expected: get the variant with original string

    // create short sequence variant with null pointer
    // expected: return an empty string variant, with size is 0, and pointer is NULL. 

    // create long sequence variant with null pointer
    // expected: return an empty string variant, with size is 0, and pointer is NULL. 

    // create string variant with extraordinary long string 
    // expected: it is depend on the free memory. 

    purc_cleanup ();
}


// to test:
// purc_variant_make_dynamic_value ();
// purc_variant_serialize ()
TEST(variant, pcvariant_dynamic)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create dynamic variant with valid pointer
    // expected: get the dynamic variant with valid pointer

    // create dynamic variant with null pointer
    // expected: get the variant with null  ???

    purc_cleanup ();
}

// to test:
// purc_variant_make_native ();
// purc_variant_serialize ()
TEST(variant, pcvariant_native)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create native variant with valid pointer
    // expected: get the native variant with valid pointer

    // create native variant with native_entity = NULL
    // expected: return PURC_VARIANT_INVALID 

    // create native variant with valid native_entity and releaser = NULL
    // expected: get native variant with releaser = NULL ???

    purc_cleanup ();
}

// to test:
// purc_variant_ref 
TEST(variant, pcvariant_ref)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);


    purc_cleanup ();
}


// to test:
// purc_variant_unref and memory 
TEST(variant, pcvariant_unref)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);


    purc_cleanup ();
}


// to test:
// purc_variant_serialize
TEST(variant, pcvariant_serialize)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // create an object variant and serialize
    // create an array variant and serialize
    // create an set variant and serialize

    purc_cleanup ();
}


// to test:
// loop buffer in heap 
TEST(variant, pcvariant_loopbuffer)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    // 1. check heap nr_reserved, and v_reserved, headpos, writepos, nr_reserved

    // 2. create 32 variants, store the pointer in an array

    // 3. unref 31 variants one by one, and check v_reserved, headpos, writepos,
    //       and nr_reserved

    // 4. unref the last, and check v_reserved, headpos, writepos, nr_reserved

    // 5. create a new variant, check the pointer whether in an array

    purc_cleanup ();
}

static inline purc_variant_t
_getter(purc_variant_t root, int nr_args, purc_variant_t arg0, ...)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(arg0);
    abort();
    return PURC_VARIANT_INVALID;
}

static inline bool
_native_releaser(void* entity)
{
    size_t nr = *(size_t*)entity;
    if (nr!=1)
        abort();
    return true;
}

TEST(variant, api_edge_case_bad_arg)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    char utf8[] = "我们";

    purc_variant_t v;
    v = purc_variant_make_string(NULL, false);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);

    v = purc_variant_make_string(utf8, false);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    v = purc_variant_make_string(utf8, true);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    // this is not a strict utf8-checker yet
    // int pos = 2;
    // char c = utf8[pos];
    // utf8[pos] = '\x0';
    // v = purc_variant_make_string(utf8, true);
    // ASSERT_EQ(v, PURC_VARIANT_INVALID);
    // utf8[pos] = c;

    const char* s;
    s = purc_variant_get_string_const(PURC_VARIANT_INVALID);
    ASSERT_EQ(s, nullptr);

    v = purc_variant_make_number(1.0);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    s = purc_variant_get_string_const(v);
    ASSERT_EQ(s, nullptr);
    purc_variant_unref(v);

    v = purc_variant_make_atom_string(NULL, false);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);

    v = purc_variant_make_atom_string(utf8, false);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    v = purc_variant_make_atom_string(utf8, true);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    v = purc_variant_make_atom_string_static(NULL, false);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);

    v = purc_variant_make_atom_string_static(utf8, false);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    v = purc_variant_make_atom_string_static(utf8, true);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    s = purc_variant_get_atom_string_const(PURC_VARIANT_INVALID);
    ASSERT_EQ(s, nullptr);

    v = purc_variant_make_number(1.0);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    s = purc_variant_get_atom_string_const(v);
    ASSERT_EQ(s, nullptr);
    purc_variant_unref(v);

    v = purc_variant_make_byte_sequence(NULL, 0);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);

    v = purc_variant_make_byte_sequence(utf8, 0);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);

    v = purc_variant_make_byte_sequence(utf8, 1);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    const unsigned char *bytes;
    size_t nr;
    bytes = purc_variant_get_bytes_const(PURC_VARIANT_INVALID, NULL);
    ASSERT_EQ(bytes, nullptr);

    v = purc_variant_make_number(1.0);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    bytes = purc_variant_get_bytes_const(v, NULL);
    ASSERT_EQ(bytes, nullptr);
    bytes = purc_variant_get_bytes_const(v, &nr);
    ASSERT_EQ(bytes, nullptr);
    purc_variant_unref(v);

    v = purc_variant_make_dynamic(NULL, NULL);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);
    v = purc_variant_make_dynamic(_getter, NULL);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);
    v = purc_variant_make_dynamic(_getter, _getter);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    v = purc_variant_make_native(NULL, NULL);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);
    v = purc_variant_make_native((void*)1, NULL);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);
    nr = 1;
    v = purc_variant_make_native(&nr, _native_releaser);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    purc_variant_unref(v);

    purc_cleanup ();
}

TEST(variant, four_constants)
{
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    struct purc_variant_stat * stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_UNDEFINED], 1);
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_NULL], 1);
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN], 2);

    purc_variant_t v;

    v = purc_variant_make_undefined();
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    ASSERT_EQ(v->refc, 1);
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_UNDEFINED], 1);
    purc_variant_unref(v);

    v = purc_variant_make_null();
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    ASSERT_EQ(v->refc, 1);
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_NULL], 1);
    purc_variant_unref(v);

    v = purc_variant_make_boolean(true);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    ASSERT_EQ(v->refc, 1);
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN], 2);
    purc_variant_unref(v);

    v = purc_variant_make_boolean(false);
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    ASSERT_EQ(v->refc, 1);
    purc_variant_ref(v);
    ASSERT_EQ(v->refc, 2);
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN], 2);
    purc_variant_unref(v);
    purc_variant_unref(v);

    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_UNDEFINED], 1);
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_NULL], 1);
    ASSERT_EQ(stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN], 2);

    purc_cleanup ();
}


