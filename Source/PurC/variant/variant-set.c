/**
 * @file variant-set.c
 * @author Xu Xiaohong (freemine)
 * @date 2021/07/09
 * @brief The API for variant.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "config.h"
#include "private/variant.h"
#include "private/list.h"
#include "private/avl.h"
#include "private/hashtable.h"
#include "private/errors.h"
#include "purc-errors.h"
#include "variant-internals.h"


#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/*
 * set holds purc_variant_t values uniquely by keynames which is separated by space
 * once a purc_variant_t value is added into set,
 * no matter via `purc_variant_make_set` or `purc_variant_set_add/...`,
 * this value's ref + 1
 * once a purc_variant_t value in set get removed,
 * no matter via `pcvariant_set_release` or `purc_variant_set_remove`
 * or even implicitly being overwritten by `purc_variant_set_add/...`,
 * this value's ref - 1
 * note: value with unique-key can be added into set for no more than 1 time
 *
 * thinking: shall we recursively check if there's ref-loop among set and it's
 *           children element?
 */

typedef struct variant_set      *variant_set_t;
struct keyname {
    struct list_head  list;
    const char       *keyname;   // key name, no ownership
};

struct keyval {
    struct list_head list;
    purc_variant_t   val;    // value bounded to keyname
};

struct obj_node {
    struct avl_node  avl;
    purc_variant_t   obj;       // actual variant-object
    struct list_head keyvals;   // keyvals of this variant-object
};

struct variant_set {
    char                   *unique_key; // unique-key duplicated
    struct list_head        keynames;   // multiple-keys parsed from unique
    struct avl_tree         objs;       // multiple-variant-objects stored in set
};

static inline variant_set_t _pcv_set_get_data(purc_variant_t set)
{
    return (variant_set_t)set->sz_ptr[1];
}

static inline void _pcv_set_set_data(purc_variant_t set, variant_set_t data)
{
    set->sz_ptr[1]     = (uintptr_t)data;
}

static int _variant_set_keyvals_cmp (const void *k1, const void *k2, void *ptr)
{
    UNUSED_PARAM(k1);
    UNUSED_PARAM(k2);
    UNUSED_PARAM(ptr);
    return -1;
}

static int _variant_set_init(variant_set_t set, const char *unique_key)
{
    INIT_LIST_HEAD(&set->keynames);
    pcutils_avl_init(&set->objs, _variant_set_keyvals_cmp, false, set);

    set->unique_key = strdup(unique_key);
    if (!set->unique_key) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    char *ctx = set->unique_key;
    char *tok = strtok_r(ctx, " ", &ctx);
    while (tok) {
        struct keyname *k = (struct keyname*)malloc(sizeof(*k));
        if (!k) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            break;
        }

        k->keyname = tok;
        list_add_tail(&k->list, &set->keynames);

        tok = strtok_r(ctx, " ", &ctx);
    }

    if (tok)
        return -1;

    return 0;
}

static int _variant_set_cache_obj_keyval(variant_set_t set, purc_variant_t value, struct list_head *kvs)
{
    struct list_head *p;
    list_for_each(p, &set->keynames) {
        struct keyname *kn = container_of(p, struct keyname, list);
        purc_variant_t vk = purc_variant_object_get_c (value, kn->keyname);
        PC_ASSERT(vk);
        struct keyval *kv = (struct keyval*)calloc(1, sizeof(*kv));
        if (!kv) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return -1;
        }
        kv->val = vk;
        purc_variant_ref(vk);
        list_add_tail(&kv->list, kvs);
    }
    return 0;
}

static purc_variant_t _pcv_set_new(void)
{
    purc_variant_t set = pcvariant_get(PVT(_SET));
    if (!set) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    set->type          = PVT(_SET);

    variant_set_t ptr  = (variant_set_t)calloc(1, sizeof(*ptr));
    _pcv_set_set_data(set, ptr);
    set->refc          = 1;

    if (!ptr) {
        pcvariant_put(set);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    // a valid empty set
    return set;
}

static void _variant_set_obj_release_kvs(struct obj_node *on)
{
    struct list_head *p, *n;
    list_for_each_safe(p, n, &on->keyvals) {
        struct keyval *kv = container_of(p, struct keyval, list);
        n = kv->list.next;
        list_del(&kv->list);
        purc_variant_unref(kv->val);
        free(kv);
    }
}

static inline void _variant_set_release_obj(struct obj_node *p)
{
    _variant_set_obj_release_kvs(p);
    purc_variant_unref(p->obj);
}

static void _variant_set_release_objs(variant_set_t set)
{
    struct obj_node *p, *n;
    avl_remove_all_elements(&set->objs, p, avl, n) {
        _variant_set_release_obj(p);
    }
}

static void _variant_set_release_keynames(variant_set_t set)
{
    struct list_head *p, *n;
    list_for_each_safe(p, n, &set->keynames) {
        struct keyname *kn = container_of(p, struct keyname, list);
        n = kn->list.next;
        list_del(&kn->list);
        free(kn);
    }
    free(set->unique_key);
    set->unique_key = NULL;
}

static inline void _variant_set_release(variant_set_t set)
{
    _variant_set_release_objs(set);
    _variant_set_release_keynames(set);
}

static int _variant_set_obj(variant_set_t set, purc_variant_t val)
{
    struct obj_node *_new = (struct obj_node*)calloc(1, sizeof(*_new));
    if (!_new) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    if (_variant_set_cache_obj_keyval(set, val, &_new->keyvals)) {
        _variant_set_release_obj(_new);
        free(_new);
        return -1;
    }

    _new->avl.key = &_new->keyvals;
    _new->obj = val;
    purc_variant_ref(val);

    struct obj_node *p;
    p = avl_find_element(&set->objs, _new->avl.key, p, avl);

    if (p) {
        if (p->obj == val) {
            // already in
            _variant_set_release_obj(_new);
            free(_new);
            return 0;
        }
        // unref obj on the node
        purc_variant_unref(p->obj);
        p->obj = val;
        purc_variant_ref(val);

        _variant_set_release_obj(_new);
        free(_new);
        return 0;
    }

    if (pcutils_avl_insert(&set->objs, &_new->avl)) {
        _variant_set_release_obj(_new);
        free(_new);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    return 0;
}

static int _variant_set_objs_n(variant_set_t set, size_t sz, va_list ap)
{
    size_t i = 0;
    while (i<sz) {
        purc_variant_t v = va_arg(ap, purc_variant_t);
        if (!v) {
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            break;
        }

        if (_variant_set_obj(set, v)) {
            break;
        }
    }
    return i<sz ? -1 : 0;
}

purc_variant_t purc_variant_make_set_c (size_t sz, const char* unique_key, purc_variant_t value0, ...)
{
    PCVARIANT_CHECK_FAIL_RET((sz==0 && unique_key && value0==NULL) ||
                         (sz>0 && unique_key && value0),
        PURC_VARIANT_INVALID);

    purc_variant_t set = _pcv_set_new();
    if (set==PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    do {
        variant_set_t data = _pcv_set_get_data(set);
        if (_variant_set_init(data, unique_key))
            break;

        purc_variant_t  v = value0;
        if (_variant_set_obj(data, v))
            break;

        if (sz>1) {
            va_list ap;
            va_start(ap, value0);
            int r = _variant_set_objs_n(data, sz-1, ap);
            va_end(ap);
            if (r)
                break;
        }

        return set;
    } while (0);

    // cleanup
    purc_variant_unref(set);

    return PURC_VARIANT_INVALID;
}

purc_variant_t purc_variant_make_set (size_t sz, purc_variant_t unique_key, purc_variant_t value0, ...)
{
    PCVARIANT_CHECK_FAIL_RET((sz==0 && unique_key && unique_key->type==PVT(_OBJECT) && value0==NULL) ||
                         (sz>0 && unique_key && unique_key->type==PVT(_OBJECT) && value0),
        PURC_VARIANT_INVALID);

    purc_variant_t set = _pcv_set_new();
    if (set==PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    do {
        const char *key = purc_variant_get_string_const(unique_key);
        if (!key) {
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            break;
        }
        variant_set_t data = _pcv_set_get_data(set);
        if (_variant_set_init(data, key))
            break;

        purc_variant_t  v = value0;
        if (_variant_set_obj(data, v))
            break;

        if (sz>1) {
            va_list ap;
            va_start(ap, value0);
            int r = _variant_set_objs_n(data, sz-1, ap);
            va_end(ap);
            if (r)
                break;
        }

        return set;
    } while (0);

    // cleanup
    purc_variant_unref(set);

    return PURC_VARIANT_INVALID;
}

bool purc_variant_set_add (purc_variant_t set, purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) && value,
        PURC_VARIANT_INVALID);

    variant_set_t data = _pcv_set_get_data(set);
    PC_ASSERT(data);

    return _variant_set_obj(data, value) ? false : true;
}

bool purc_variant_set_remove (purc_variant_t set, purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) && value,
        PURC_VARIANT_INVALID);

    variant_set_t data = _pcv_set_get_data(set);
    PC_ASSERT(data);

    struct obj_node *_qry = (struct obj_node*)calloc(1, sizeof(*_qry));
    if (!_qry) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    if (_variant_set_cache_obj_keyval(data, value, &_qry->keyvals)) {
        _variant_set_release_obj(_qry);
        free(_qry);
        return -1;
    }

    _qry->avl.key = &_qry->keyvals;
    _qry->obj = value;
    purc_variant_ref(value);

    // remove `value` rather than by key that `value` represents
    struct obj_node *p;
    p = avl_find_element(&data->objs, _qry->avl.key, p, avl);
    if (!p) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        _variant_set_release_obj(_qry);
        free(_qry);
        return false;
    }

    pcutils_avl_delete(&data->objs, &p->avl);
    _variant_set_release_obj(p);
    free(p);
    _variant_set_release_obj(_qry);
    free(_qry);

    return true;
}

purc_variant_t purc_variant_set_get_value_c (const purc_variant_t set, const char * match_key)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET) && match_key && *match_key,
        PURC_VARIANT_INVALID);

    variant_set_t data = _pcv_set_get_data(set);
    PC_ASSERT(data);
    // to do
    return PURC_VARIANT_INVALID;
}

size_t purc_variant_set_get_size(const purc_variant_t set)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET),
        -1);

    variant_set_t data = _pcv_set_get_data(set);
    PC_ASSERT(data);

    return data->objs.count;
}

struct purc_variant_set_iterator {
    purc_variant_t       set;
    struct obj_node     *curr;
};

struct purc_variant_set_iterator* purc_variant_set_make_iterator_begin (purc_variant_t set)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET),
        NULL);
    struct purc_variant_set_iterator *it = (struct purc_variant_set_iterator*)calloc(1, sizeof(*it));
    if (!it) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    it->set = set;

    variant_set_t data = _pcv_set_get_data(set);
    PC_ASSERT(data);

    struct obj_node *p;
    p = avl_first_element(&data->objs, p, avl);
    if (!p) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        free(it);
        return NULL;
    }

    it->curr = p;
    return it;
}

struct purc_variant_set_iterator* purc_variant_set_make_iterator_end (purc_variant_t set)
{
    PCVARIANT_CHECK_FAIL_RET(set && set->type==PVT(_SET),
        NULL);
    struct purc_variant_set_iterator *it = (struct purc_variant_set_iterator*)calloc(1, sizeof(*it));
    if (!it) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    it->set = set;

    variant_set_t data = _pcv_set_get_data(set);
    PC_ASSERT(data);

    struct obj_node *p;
    p = avl_last_element(&data->objs, p, avl);
    if (!p) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        free(it);
        return NULL;
    }

    it->curr = p;
    return it;
}

void purc_variant_set_release_iterator (struct purc_variant_set_iterator* it)
{
    free(it);
}

bool purc_variant_set_iterator_next (struct purc_variant_set_iterator* it)
{
    PCVARIANT_CHECK_FAIL_RET(it && it->set && it->set->type==PVT(_SET) && it->curr,
        false);

    variant_set_t data = _pcv_set_get_data(it->set);
    PC_ASSERT(data);

    struct obj_node *p;
    p = avl_last_element(&data->objs, p, avl);
    if (it->curr==p) {
        it->curr = NULL;
        pcinst_set_error(PURC_ERROR_OK);
        return false;
    }
    it->curr = avl_next_element(it->curr, avl);
    if (!it->curr) {
        pcinst_set_error(PURC_ERROR_OK);
        return false;
    }
    return true;
}

bool purc_variant_set_iterator_prev (struct purc_variant_set_iterator* it)
{
    PCVARIANT_CHECK_FAIL_RET(it && it->set && it->set->type==PVT(_SET) && it->curr,
        false);

    variant_set_t data = _pcv_set_get_data(it->set);
    PC_ASSERT(data);

    struct obj_node *p;
    p = avl_first_element(&data->objs, p, avl);
    if (it->curr==p) {
        it->curr = NULL;
        pcinst_set_error(PURC_ERROR_OK);
        return false;
    }
    it->curr = avl_prev_element(it->curr, avl);
    if (!it->curr) {
        pcinst_set_error(PURC_ERROR_OK);
        return false;
    }
    return true;
}

purc_variant_t purc_variant_set_iterator_get_value (struct purc_variant_set_iterator* it)
{
    PCVARIANT_CHECK_FAIL_RET(it && it->set && it->set->type==PVT(_SET) && it->curr,
        PURC_VARIANT_INVALID);

    return it->curr->obj;
}

void pcvariant_set_release (purc_variant_t value)
{
    variant_set_t data = _pcv_set_get_data(value);
    PC_ASSERT(data);

    _variant_set_release(data);
}

int pcvariant_set_compare (purc_variant_t lv, purc_variant_t rv)
{
    variant_set_t ldata = _pcv_set_get_data(lv);
    variant_set_t rdata = _pcv_set_get_data(rv);
    PC_ASSERT(ldata && rdata);

    struct obj_node *ln, *rn;
    ln = avl_first_element(&ldata->objs, ln, avl);
    rn = avl_first_element(&rdata->objs, rn, avl);
    for (; ln && rn;
        ln = avl_next_element(ln, avl),
        rn = avl_next_element(rn, avl))
    {
        int t = purc_variant_compare(ln->obj, rn->obj);
        if (t)
            return t;
    }

    return ln ? 1 : -1;
}

