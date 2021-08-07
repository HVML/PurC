#include "purc.h"
#include "private/avl.h"
#include "private/arraylist.h"
#include "private/hashtable.h"
#include "purc-variant.h"
#include "private/variant.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <gtest/gtest.h>

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

    int count = 1024;
    char buf[64];
    for (int j=0; j<count; ++j) {
        snprintf(buf, sizeof(buf), "%d", j);
        purc_variant_t s = purc_variant_make_string(buf, false);
        ASSERT_NE(s, nullptr);
        purc_variant_t obj = purc_variant_make_object_c(1, "hello", s);
        ASSERT_NE(obj, nullptr);
        ASSERT_EQ(stat->nr_values[PVT(_OBJECT)], j+1);
        bool t = purc_variant_set_add(var, obj, false);
        ASSERT_EQ(t, true);
        purc_variant_unref(obj);
        purc_variant_unref(s);
        ASSERT_EQ(obj->refc, 1);
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
        ASSERT_EQ(v->refc, 1);
        if (1) {
            bool ok = purc_variant_set_remove(var, v);
            ASSERT_EQ(ok, true);
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

static inline int
_keys_prepare(int **ar_vals, size_t n)
{
    int    *p = (int*)malloc(sizeof(*p)*n);
    EXPECT_NE(p, nullptr);
    if (!p)
        return -1;

    for (size_t i=0; i<n; ++i) {
        p[i] = i;
    }

    for (size_t i=0; i<n; ++i) {
        int l = _get_random(n);
        int r = _get_random(n);
        int v = p[l];
        p[l]  = p[r];
        p[r]  = v;
    }

    *ar_vals = p;

    return 0;
}


typedef purc_variant_t (*_make_variant_f)(int lvl);

static purc_variant_t _make_variant(int lvl);

static purc_variant_t _make_null(int lvl)
{
    // what a compiler
    if (0) _make_null(0);

    UNUSED_PARAM(lvl);
    return purc_variant_make_null();
}

static purc_variant_t _make_undefined(int lvl)
{
    // what a compiler
    if (0) _make_undefined(0);

    UNUSED_PARAM(lvl);
    return purc_variant_make_undefined();
}

static purc_variant_t _make_boolean(int lvl)
{
    // what a compiler
    if (0) _make_boolean(0);

    UNUSED_PARAM(lvl);
    bool b = _get_random(2) ? true : false;
    return purc_variant_make_boolean(b);
}

static purc_variant_t _make_number(int lvl)
{
    // what a compiler
    if (0) _make_number(0);

    UNUSED_PARAM(lvl);
    double v;
    uint8_t *p = (uint8_t*)&v;
    for (size_t i=0; i<sizeof(v)/sizeof(*p); ++i) {
        p[i] = _get_random(UINT8_MAX);
    }
    return purc_variant_make_number(v);
}

static purc_variant_t _make_longint(int lvl)
{
    // what a compiler
    if (0) _make_longint(0);

    UNUSED_PARAM(lvl);
    int64_t v;
    uint8_t *p = (uint8_t*)&v;
    for (size_t i=0; i<sizeof(v)/sizeof(*p); ++i) {
        p[i] = _get_random(UINT8_MAX);
    }
    return purc_variant_make_longint(v);
}

static purc_variant_t _make_ulongint(int lvl)
{
    // what a compiler
    if (0) _make_ulongint(0);

    UNUSED_PARAM(lvl);
    uint64_t v;
    uint8_t *p = (uint8_t*)&v;
    for (size_t i=0; i<sizeof(v)/sizeof(*p); ++i) {
        p[i] = _get_random(UINT8_MAX);
    }
    return purc_variant_make_ulongint(v);
}

static purc_variant_t _make_longdouble(int lvl)
{
    // what a compiler
    if (0) _make_longdouble(0);

    UNUSED_PARAM(lvl);
    long double v;
    uint8_t *p = (uint8_t*)&v;
    for (size_t i=0; i<sizeof(v)/sizeof(*p); ++i) {
        p[i] = _get_random(UINT8_MAX);
    }
    return purc_variant_make_longdouble(v);
}

#define _generate(_tmpl, _dst, _sz) do {                  \
    for (size_t _i = 0; _i<_sz; ++_i) {                   \
        _dst[_i] = _tmpl[_get_random(sizeof(_tmpl))];     \
    }                                                     \
} while (0)

static purc_variant_t _make_atom_string(int lvl)
{
    // what a compiler
    if (0) _make_atom_string(0);

    UNUSED_PARAM(lvl);
    char temp[] =
        "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "`~!@#$%^&*()-_=+[{]}|;:',<.>/?"
        "\\\"";
    char v[256];
    _generate(temp, v, sizeof(v));
    v[_get_random(sizeof(v))] = '\0';
    return purc_variant_make_atom_string(v, false);
}

static purc_variant_t _make_string(int lvl)
{
    // what a compiler
    if (0) _make_string(0);

    UNUSED_PARAM(lvl);
    char temp[] =
        "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "`~!@#$%^&*()-_=+[{]}|;:',<.>/?"
        "\\\"";
    char v[512];
    _generate(temp, v, sizeof(v));
    v[_get_random(sizeof(v))] = '\0';
    return purc_variant_make_string(v, false);
}

static purc_variant_t _make_bsequence(int lvl)
{
    // what a compiler
    if (0) _make_bsequence(0);

    UNUSED_PARAM(lvl);
    unsigned char v[512];
    for (size_t i=0; i<sizeof(v)-1; ++i) {
        v[i] = _get_random(256);
    }
    return purc_variant_make_byte_sequence(v, 1+_get_random(sizeof(v)-1));
}

static purc_variant_t _dummy(purc_variant_t root,
            int nr_args, purc_variant_t arg0, ...)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(arg0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t _make_dynamic(int lvl)
{
    // what a compiler
    if (0) _make_dynamic(0);

    UNUSED_PARAM(lvl);
    return purc_variant_make_dynamic(_dummy, _dummy);
}

static bool _dummy_releaser (void* entity)
{
    UNUSED_PARAM(entity);
    return true;
}

static purc_variant_t _make_native(int lvl)
{
    // what a compiler
    if (0) _make_native(0);

    UNUSED_PARAM(lvl);
    return purc_variant_make_native((void*)_dummy_releaser, _dummy_releaser);
}

static size_t _nr_level       = 4;
static size_t _nr_children    = 16;
static size_t _nr_iteration   = 128;
static _make_variant_f _make_func = NULL;

static purc_variant_t _make_object(int lvl)
{
    // what a compiler
    if (0) _make_object(0);

    if (lvl<=0)
        return PURC_VARIANT_INVALID;

    purc_variant_t v;
    v = purc_variant_make_object_c(0, NULL, NULL);
    if (v==PURC_VARIANT_INVALID)
        return v;

    bool ok = true;
    size_t n = _get_random(_nr_children);
    purc_variant_t key = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    for (size_t i=0; i<n && lvl>1; ++i) {
        key = _make_string(1);
        if (key==PURC_VARIANT_INVALID) {
            ok = false;
            break;
        }
        val = _make_variant(lvl-1);
        if (val==PURC_VARIANT_INVALID) {
            ok = false;
            break;
        }
        ok = purc_variant_object_set(v, key, val);
        purc_variant_unref(key);
        purc_variant_unref(val);
        key = PURC_VARIANT_INVALID;
        val = PURC_VARIANT_INVALID;
        if (!ok) {
            break;
        }
    }
    if (key!=PURC_VARIANT_INVALID) {
        purc_variant_unref(key);
        key = PURC_VARIANT_INVALID;
    }
    if (val!=PURC_VARIANT_INVALID) {
        purc_variant_unref(val);
        val = PURC_VARIANT_INVALID;
    }
    if (!ok) {
        purc_variant_unref(v);
        return PURC_VARIANT_INVALID;
    }
    return v;
}

static purc_variant_t _make_array(int lvl)
{
    // what a compiler
    if (0) _make_array(0);

    if (lvl<=0)
        return PURC_VARIANT_INVALID;

    purc_variant_t v;
    v = purc_variant_make_array(0, NULL);
    if (v==PURC_VARIANT_INVALID)
        return v;

    bool ok = true;
    size_t n = _get_random(_nr_children);
    for (size_t i=0; i<n && lvl>1; ++i) {
        purc_variant_t val = _make_variant(lvl-1);
        if (val==PURC_VARIANT_INVALID) {
            ok = false;
            break;
        }
        int action = _get_random(2);
        if (action) {
            ok = purc_variant_array_append(v, val);
        } else {
            ok = purc_variant_array_prepend(v, val);
        }
        if (!ok) {
            purc_variant_unref(val);
            break;
        }
        purc_variant_unref(val);
    }
    if (!ok) {
        purc_variant_unref(v);
        return PURC_VARIANT_INVALID;
    }
    return v;
}

static purc_variant_t _make_set(int lvl)
{
    // what a compiler
    if (0) _make_set(0);

    if (lvl<=0)
        return PURC_VARIANT_INVALID;

    const char *temp[] = {
        "name",
        "sex",
        "age",
        "country",
        "ethic",
    };
    size_t nr_temp = sizeof(temp)/sizeof(temp[0]);

    // select keys
    const char *keys[sizeof(temp)/sizeof(temp[0])];
    size_t nr_keys = _get_random(nr_temp);
    for (size_t i=0; i<nr_keys; ++i) {
        keys[i] = temp[_get_random(nr_temp)];
    }

    // make uniq
    char uniq[256];
    uniq[0] = '\0';
    for (size_t i=0; i<nr_keys; ++i) {
        if (i) {
            strcat(uniq, " ");
        }
        strcat(uniq, keys[_get_random(nr_keys)]);
    }

    purc_variant_t v;
    v = purc_variant_make_set_c(0, uniq, NULL);
    if (v==PURC_VARIANT_INVALID)
        return v;

    bool ok = true;
    size_t n = _get_random(_nr_children);
    for (size_t i=0; i<n && lvl>1; ++i) {
        purc_variant_t obj = _make_object(lvl-1);
        if (obj==PURC_VARIANT_INVALID) {
            ok = false;
            break;
        }

        size_t jn = _get_random(nr_keys);
        for (size_t j=0; j<jn; ++j) {
            const char *key = keys[_get_random(nr_keys)];
            purc_variant_t val = _make_string(lvl-1);
            if (val==PURC_VARIANT_INVALID) {
                ok = false;
                break;
            }
            ok = purc_variant_object_set_c(obj, key, val);
            purc_variant_unref(val);
            if (!ok)
                break;
        }

        if (ok) {
            ok = purc_variant_set_add(v, obj, true);
        }
        purc_variant_unref(obj);
        if (!ok) {
            break;
        }
    }

    if (!ok) {
        purc_variant_unref(v);
        return PURC_VARIANT_INVALID;
    }

    return v;
}

static _make_variant_f _ar_make_vars[] = {
    _make_null,
    _make_undefined,
    _make_boolean,
    _make_number,
    _make_longint,
    _make_ulongint,
    _make_longdouble,
    _make_atom_string,
    _make_string,
    _make_bsequence,
    _make_dynamic,
    _make_native,
    _make_object,
    _make_array,
    _make_set,
};

static _make_variant_f *_make_vars = _ar_make_vars;
static size_t _nr_make_vars = sizeof(_ar_make_vars)/sizeof(_ar_make_vars[0]);

struct _map_s {
    const char       *name;
    _make_variant_f   func;
};

#define _MAP_REC(_x) {#_x, _make_##_x}

static struct _map_s     _maps[] = {
    _MAP_REC(null),
    _MAP_REC(undefined),
    _MAP_REC(boolean),
    _MAP_REC(number),
    _MAP_REC(longint),
    _MAP_REC(ulongint),
    _MAP_REC(longdouble),
    _MAP_REC(atom_string),
    _MAP_REC(string),
    _MAP_REC(bsequence),
    _MAP_REC(dynamic),
    _MAP_REC(native),
    _MAP_REC(object),
    _MAP_REC(array),
    _MAP_REC(set),
};

static inline _make_variant_f
_get_make_variant_f(void)
{
    if (_make_func) {
        return _make_func;
    }
    size_t nr = _nr_make_vars;
    int idx = _get_random(nr);
    return _make_vars[idx];
}

static purc_variant_t
_make_variant(int lvl)
{
    return _get_make_variant_f()(lvl);
}

TEST(variant_set, gen_random)
{
    int r;
    bool b;
    char *s;

    char *enable = getenv("PURC_TEST_VARIANT_RANDOM_ENABLE");
    if (!enable || strcmp(enable, "1")) {
        return;
    }

    s = getenv("PURC_TEST_VARIANT_RANDOM_CHILDREN");
    r = s ? atoi(s) : _nr_children;
    if (r>0)
        _nr_children = r;
    fprintf(stderr, "PURC_TEST_VARIANT_RANDOM_CHILDREN:  [%zd]\n",
        _nr_children);

    s = getenv("PURC_TEST_VARIANT_RANDOM_LEVEL");
    r = s ? atoi(s) : _nr_level;
    if (r>0)
        _nr_level = r;
    fprintf(stderr, "PURC_TEST_VARIANT_RANDOM_LEVEL:     [%zd]\n",
        _nr_level);

    s = getenv("PURC_TEST_VARIANT_RANDOM_ITERATION");
    r = s ? atoi(s) : _nr_iteration;
    if (r>0)
        _nr_iteration = r;
    fprintf(stderr, "PURC_TEST_VARIANT_RANDOM_ITERATION: [%zd]\n",
        _nr_iteration);

    s = getenv("PURC_TEST_VARIANT_RANDOM_TYPE");
    if (s) {
        _make_vars = (_make_variant_f*)calloc(strlen(s),
            sizeof(*_make_vars));
        ASSERT_NE(_make_vars, nullptr);
        _nr_make_vars = 0;
        s = strdup(s);
        ASSERT_NE(s, nullptr);

        char *ctx = s;
        const char *delim = ";";
        char *tok = strtok_r(ctx, delim, &ctx);
        while (tok) {
            for (size_t i=0; i<sizeof(_maps)/sizeof(_maps[0]); ++i) {
                if (strcmp(_maps[i].name, tok)==0) {
                    _make_vars[_nr_make_vars++] = _maps[i].func;
                    if (_nr_make_vars==1) {
                        fprintf(stderr, "PURC_TEST_VARIANT_RANDOM_TYPE:      [");
                    } else {
                        fprintf(stderr, ";");
                    }
                    fprintf(stderr, "%s", tok);
                    break;
                }
            }
            tok = strtok_r(ctx, delim, &ctx);
        }
        if (_nr_make_vars) {
            fprintf(stderr, "]\n");
        }
        ASSERT_GT(_nr_make_vars, 0);
        free(s);
    }

    purc_instance_extra_info info = {0, 0};

    r = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(r, PURC_ERROR_OK);

    for (size_t i=0; i<_nr_iteration; ++i) {
        purc_variant_t v = _make_variant(_nr_level);
        if (v==nullptr) {
            ASSERT_EQ(purc_get_last_error(), PURC_ERROR_OUT_OF_MEMORY);
        }
        ASSERT_EQ(v->refc, 1);
        purc_variant_unref(v);
        if ((i+1)%16==0) {
            fprintf(stderr, "iterations: %zd\n", i+1);
        }
    }

    b = purc_cleanup ();
    ASSERT_EQ (b, true);
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

