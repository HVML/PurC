/*
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "purc/purc.h"
#include "private/avl.h"
#include "private/hashtable.h"
#include "purc/purc-variant.h"
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

static purc_variant_t _make_exception(int lvl)
{
    // what a compiler
    if (0) _make_exception(0);

    UNUSED_PARAM(lvl);
    return purc_variant_make_exception(_get_random(PURC_EXCEPT_LAST));
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
            size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t _make_dynamic(int lvl)
{
    // what a compiler
    if (0) _make_dynamic(0);

    UNUSED_PARAM(lvl);
    return purc_variant_make_dynamic(_dummy, _dummy);
}

static void _dummy_releaser (void* entity)
{
    UNUSED_PARAM(entity);
}

static struct purc_native_ops _dummy_ops = {
    .property_getter       = NULL,
    .property_setter       = NULL,
    .property_cleaner      = NULL,
    .property_eraser       = NULL,

    .updater               = NULL,
    .cleaner               = NULL,
    .eraser                = NULL,
    .did_matched         = NULL,

    .on_observe           = NULL,
    .on_forget            = NULL,
    .on_release           = _dummy_releaser,
};

static purc_variant_t _make_native(int lvl)
{
    // what a compiler
    if (0) _make_native(0);

    UNUSED_PARAM(lvl);
    return purc_variant_make_native((void*)_dummy_releaser, &_dummy_ops);
}

static size_t _nr_level       = 4;
static size_t _nr_children    = 16;
static size_t _nr_iteration   = 128;

static purc_variant_t _make_object(int lvl)
{
    // what a compiler
    if (0) _make_object(0);

    if (lvl<=0)
        return PURC_VARIANT_INVALID;

    purc_variant_t v;
    v = purc_variant_make_object_by_static_ckey(0, NULL, NULL);
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
    size_t nr_temp = PCA_TABLESIZE(temp);

    // select keys
    const char *keys[PCA_TABLESIZE(temp)];
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
    v = purc_variant_make_set_by_ckey(0, uniq, NULL);
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
            ok = purc_variant_object_set_by_static_ckey(obj, key, val);
            purc_variant_unref(val);
            if (!ok)
                break;
        }

        if (ok) {
            ok = purc_variant_set_add(v, obj, PCVRNT_CR_METHOD_OVERWRITE);
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

struct _map_s {
    const char       *name;
    _make_variant_f   func;
};

#define _MAP_REC(_x) {#_x, _make_##_x}

static struct _map_s     _maps[] = {
    _MAP_REC(null),
    _MAP_REC(undefined),
    _MAP_REC(boolean),
    _MAP_REC(exception),
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
#define _nr_maps (PCA_TABLESIZE(_maps))
static struct _map_s *_make_vars = _maps;
static size_t _nr_make_vars = _nr_maps;

static inline _make_variant_f
_get_make_variant_f(void)
{
    size_t nr = _nr_make_vars;
    int idx = _get_random(nr);
    return _make_vars[idx].func;
}

static purc_variant_t
_make_variant(int lvl)
{
    return _get_make_variant_f()(lvl);
}

TEST(random, make)
{
    int r;
    bool b;
    char *s;

    char *enable = getenv("PURC_TEST_VARIANT_RANDOM_ENABLE");
    if (!enable || strcmp(enable, "1")) {
        fprintf(stderr, "export PURC_TEST_VARIANT_RANDOM_ENABLE=1 to run\n");
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
        // duplicates as weights
        _make_vars = (struct _map_s*)calloc(strlen(s)*_nr_maps,
            sizeof(*_make_vars));
        ASSERT_NE(_make_vars, nullptr);
        _nr_make_vars = 0;
        s = strdup(s);
        ASSERT_NE(s, nullptr);

        char *ctx = s;
        const char *delim = ";";
        char *tok = strtok_r(ctx, delim, &ctx);
        while (tok) {
            for (size_t i=0; i<_nr_maps; ++i) {
                if (strcmp(tok, "all")==0) {
                    for (size_t j=0; j<_nr_maps; ++j) {
                        // duplicates as weights
                        _make_vars[_nr_make_vars++] = _maps[j];
                    }
                    break;
                }
                if (strcmp(_maps[i].name, tok))
                    continue;

                // duplicates as weights
                _make_vars[_nr_make_vars++] = _maps[i];
                break;
            }
            tok = strtok_r(ctx, delim, &ctx);
        }
        fprintf(stderr, "PURC_TEST_VARIANT_RANDOM_TYPE:      [");
        for (size_t i=0; i<_nr_make_vars; ++i) {
            if (i) {
                fprintf(stderr, ";");
            }
            fprintf(stderr, "%s", _make_vars[i].name);
        }
        if (_nr_make_vars) {
            fprintf(stderr, "]\n");
        }
        ASSERT_GT(_nr_make_vars, 0);
        free(s);
    }

    purc_instance_extra_info info = {};

    r = purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
            "test_init", &info);
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

