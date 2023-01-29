/*
 * @file constraint.c
 * @author Xu Xiaohong
 * @date 2022/03/16
 * @brief The implementation for variant constraint
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

#include "purc-errors.h"
#include "private/debug.h"
#include "private/errors.h"
#include "variant-internals.h"

#include <stdlib.h>

static int
key_comp(const void *key1, const void *key2)
{
    return (char*)key1 - (char*)key2;
}

pcutils_map*
pcvar_create_rev_update_chain(void)
{
    copy_key_fn copy_key = NULL;
    free_key_fn free_key = NULL;
    copy_val_fn copy_val = NULL;
    free_val_fn free_val = NULL;
    comp_key_fn comp_key = key_comp;

    bool threads = false;
    return pcutils_map_create(copy_key, free_key, copy_val, free_val,
            comp_key, threads);
}

void
pcvar_destroy_rev_update_chain(pcutils_map *chain)
{
    if (!chain)
        return;

    size_t nr = pcutils_map_get_size(chain);
    PC_ASSERT(nr == 0);

    pcutils_map_destroy(chain);
}

static int
comp(const void *key1, const void *key2)
{
    purc_variant_t k1 = (purc_variant_t)key1;
    purc_variant_t k2 = (purc_variant_t)key2;

    return k1 - k2;
}

struct reverse_checker {
    pcutils_map           *input;     // key/val: variant old /variant new
    pcutils_map           *cache;     // as above
    pcutils_map           *output;    // as above
};

static purc_variant_t
rebuild_ex(purc_variant_t val, pcutils_map *cache);

static purc_variant_t
rebuild_arr_ex(purc_variant_t arr, pcutils_map *cache)
{
    struct pcutils_map_entry *entry;
    entry = pcutils_map_find(cache, arr);
    if (entry) {
        return purc_variant_ref((purc_variant_t)entry->val);
    }

    purc_variant_t _new = pcvar_make_arr();
    if (_new == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    int r = 0;
    do {
        purc_variant_t v;
        size_t idx;
        UNUSED_PARAM(idx);
        foreach_value_in_variant_array(arr, v, idx) {
            purc_variant_t _v = rebuild_ex(v, cache);
            if (_v == PURC_VARIANT_INVALID) {
                r = -1;
                break;
            }
            r = pcvar_arr_append(_new, _v);
            purc_variant_unref(_v);
            if (r)
                break;
        } end_foreach;
        if (r)
            break;

        // insert arr/_new pair into cache
        r = pcutils_map_insert(cache, arr, _new);
        if (r)
            break;

        return _new;
    } while (0);

    PURC_VARIANT_SAFE_CLEAR(_new);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
rebuild_obj_ex(purc_variant_t obj, pcutils_map *cache)
{
    struct pcutils_map_entry *entry;
    entry = pcutils_map_find(cache, obj);
    if (entry) {
        return purc_variant_ref((purc_variant_t)entry->val);
    }

    purc_variant_t _new = pcvar_make_obj();
    if (_new == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    int r = 0;
    do {
        purc_variant_t k, v;
        foreach_key_value_in_variant_object(obj, k, v) {
            purc_variant_t _v = rebuild_ex(v, cache);
            if (_v == PURC_VARIANT_INVALID) {
                r = -1;
                break;
            }
            r = pcvar_obj_set(_new, k, _v);
            purc_variant_unref(_v);
            if (r)
                break;
        } end_foreach;
        if (r)
            break;

        // insert obj/_new pair into cache
        r = pcutils_map_insert(cache, obj, _new);
        if (r)
            break;

        return _new;
    } while (0);

    PURC_VARIANT_SAFE_CLEAR(_new);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
rebuild_set_ex(purc_variant_t set, pcutils_map *cache)
{
    struct pcutils_map_entry *entry;
    entry = pcutils_map_find(cache, set);
    if (entry) {
        PC_ASSERT(entry->val);
        return purc_variant_ref((purc_variant_t)entry->val);
    }

    variant_set_t data = pcvar_set_get_data(set);

    purc_variant_t _new = pcvar_make_set(data);
    if (_new == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    int r = 0;
    do {
        purc_variant_t v;
        foreach_value_in_variant_set(set, v) {
            purc_variant_t _v = rebuild_ex(v, cache);
            if (_v == PURC_VARIANT_INVALID) {
                r = -1;
                break;
            }
            r = pcvar_set_add(_new, _v);
            purc_variant_unref(_v);
            if (r)
                break;
        } end_foreach;
        if (r)
            break;

        // insert set/_new pair into cache
        r = pcutils_map_insert(cache, set, _new);
        if (r)
            break;

        return _new;
    } while (0);

    PURC_VARIANT_SAFE_CLEAR(_new);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
rebuild_tuple_ex(purc_variant_t tuple, pcutils_map *cache)
{
    struct pcutils_map_entry *entry;
    entry = pcutils_map_find(cache, tuple);
    if (entry) {
        PC_ASSERT(entry->val);
        return purc_variant_ref((purc_variant_t)entry->val);
    }

    size_t sz;
    purc_variant_t *members;
    members = tuple_members(tuple, &sz);

    purc_variant_t _new = purc_variant_make_tuple(sz, members);
    if (_new == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    int r =  pcutils_map_insert(cache, tuple, _new);
    if (r) {
        PURC_VARIANT_SAFE_CLEAR(_new);
        return PURC_VARIANT_INVALID;
    }
    return _new;
}

static purc_variant_t
rebuild_ex(purc_variant_t val, pcutils_map *cache)
{
    switch (val->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            return rebuild_arr_ex(val, cache);
        case PURC_VARIANT_TYPE_OBJECT:
            return rebuild_obj_ex(val, cache);
        case PURC_VARIANT_TYPE_SET:
            return rebuild_set_ex(val, cache);
        case PURC_VARIANT_TYPE_TUPLE:
            return rebuild_tuple_ex(val, cache);
        default:
            return purc_variant_ref(val);
    }
}

static int
reverse_check_chain(pcutils_map *chain, struct reverse_checker *checker)
{
    int r = 0;
    do {
        if (!chain)
            break;

        size_t nr = pcutils_map_get_size(chain);
        if (nr == 0)
            break;

        struct pcutils_map_entry *entry;
        struct pcutils_map_iterator it;
        it = pcutils_map_it_begin_first(chain);
        while ((entry = pcutils_map_it_value(&it))) {
            purc_variant_t parent;
            parent = (purc_variant_t)entry->val;

            // rebuild _new value for edge parent
            purc_variant_t _new = rebuild_ex(parent, checker->cache);
            if (_new == PURC_VARIANT_INVALID) {
                r = -1;
                break;
            }

            // sanity check
            struct pcutils_map_entry *p;
            p = pcutils_map_find(checker->cache, parent);
            PC_ASSERT(p);
            PC_ASSERT((purc_variant_t)p->val == _new);

            // add parent/_new pair into output
            r = pcutils_map_replace_or_insert(checker->output,
                    parent, _new, NULL);
            PURC_VARIANT_SAFE_CLEAR(_new);

            if (r)
                break;

            pcutils_map_it_next(&it);
        }
        pcutils_map_it_end(&it);
        if (r)
            break;

        return 0;
    } while (0);

    return r ? -1 : 0;
}

static int
reverse_check(struct reverse_checker *checker)
{
    size_t nr;
    int r = 0;

    pcutils_map *tmp;

    struct pcutils_map_entry *entry;
    struct pcutils_map_iterator it;
    variant_arr_t arr_data;
    variant_obj_t obj_data;
    variant_set_t set_data;
    variant_tuple_t tuple_data;

again:
    nr = pcutils_map_get_size(checker->input);
    if (nr == 0)
        return 0;

    it = pcutils_map_it_begin_first(checker->input);
    while ((entry = pcutils_map_it_value(&it)) != NULL) {
        purc_variant_t _old = (purc_variant_t)entry->key;
        purc_variant_t _new = (purc_variant_t)entry->val;

        // sanity check
        struct pcutils_map_entry *p;
        p = pcutils_map_find(checker->cache, _old);
        PC_ASSERT(p);
        PC_ASSERT((purc_variant_t)p->val == _new);

        switch (_old->type) {
            case PURC_VARIANT_TYPE_ARRAY:
                arr_data = pcvar_arr_get_data(_old);
                r = reverse_check_chain(arr_data->rev_update_chain, checker);
                break;
            case PURC_VARIANT_TYPE_OBJECT:
                obj_data = pcvar_obj_get_data(_old);
                r = reverse_check_chain(obj_data->rev_update_chain, checker);
                break;
            case PURC_VARIANT_TYPE_SET:
                set_data = pcvar_set_get_data(_old);
                r = reverse_check_chain(set_data->rev_update_chain, checker);
                break;
            case PURC_VARIANT_TYPE_TUPLE:
                tuple_data = pcvar_tuple_get_data(_old);
                r = reverse_check_chain(tuple_data->rev_update_chain, checker);
                break;
            default:
                PC_ASSERT(0);
        }
        if (r)
            break;

        // remove _old/_new pair from input
        r = pcutils_map_erase(checker->input, _old);
        if (r)
            break;

        pcutils_map_it_next(&it);
    }
    pcutils_map_it_end(&it);

    if (r)
        return -1;

    // sanity check
    nr = pcutils_map_get_size(checker->input);
    PC_ASSERT(nr == 0);

    tmp = checker->input;
    checker->input = checker->output;
    checker->output = tmp;

    goto again;
}

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

int
pcvar_reverse_check(purc_variant_t _old, purc_variant_t _new)
{
    copy_key_fn copy_key = ref;
    free_key_fn free_key = unref;
    copy_val_fn copy_val = ref;
    free_val_fn free_val = unref;
    comp_key_fn comp_key = comp;

    struct reverse_checker checker = {};

    bool threads = false;
    checker.input = pcutils_map_create(copy_key, free_key,
            copy_val, free_val, comp_key, threads);
    checker.cache = pcutils_map_create(copy_key, free_key,
            copy_val, free_val, comp_key, threads);
    checker.output = pcutils_map_create(copy_key, free_key,
            copy_val, free_val, comp_key, threads);

    int r = -1;
    do {
        if (checker.input == NULL)
            break;
        if (checker.cache == NULL)
            break;
        if (checker.output == NULL)
            break;

        r = pcutils_map_insert(checker.input, _old, _new);
        if (r)
            break;

        r = pcutils_map_insert(checker.cache, _old, _new);
        if (r)
            break;

        r = reverse_check(&checker);
    } while (0);

    if (checker.output)
        pcutils_map_destroy(checker.output);
    if (checker.cache)
        pcutils_map_destroy(checker.cache);
    if (checker.input)
        pcutils_map_destroy(checker.input);

    return r ? -1 : 0;
}

struct pcutils_map*
get_chain(purc_variant_t val)
{
    variant_arr_t arr_data;
    variant_obj_t obj_data;
    variant_set_t set_data;
    variant_tuple_t tuple_data;

    switch (val->type) {
        case PURC_VARIANT_TYPE_ARRAY:
            arr_data = pcvar_arr_get_data(val);
            return arr_data->rev_update_chain;
        case PURC_VARIANT_TYPE_OBJECT:
            obj_data = pcvar_obj_get_data(val);
            return obj_data->rev_update_chain;
        case PURC_VARIANT_TYPE_SET:
            set_data = pcvar_set_get_data(val);
            return set_data->rev_update_chain;
        case PURC_VARIANT_TYPE_TUPLE:
            tuple_data = pcvar_tuple_get_data(val);
            return tuple_data->rev_update_chain;
        default:
            PC_ASSERT(0);
            break;
    }

    return NULL;
}

static int
wind_up_val(purc_variant_t val, struct reverse_checker *checker)
{
    int r = 0;

    struct pcutils_map *chain;
    chain = get_chain(val);
    if (!chain)
        return 0;

    size_t nr = pcutils_map_get_size(chain);
    if (nr == 0)
        return 0;

    struct pcutils_map_entry *entry;
    struct pcutils_map_iterator it;
    it = pcutils_map_it_begin_first(chain);
    while ((entry = pcutils_map_it_value(&it))) {
        purc_variant_t parent;
        parent = (purc_variant_t)entry->val;

        if (purc_variant_is_set(parent)) {
            struct set_node *node;
            node = (struct set_node*)entry->key;
            r = pcvar_readjust_set(parent, node);
        }
        else {
            r = pcutils_map_replace_or_insert(checker->output,
                    parent, parent, NULL);
        }
        if (r)
            break;
        pcutils_map_it_next(&it);
    }
    pcutils_map_it_end(&it);

    return r ? -1 : 0;
}

static int
wind_up(struct reverse_checker *checker)
{
    if (!checker->input)
        return 0;

    size_t nr = pcutils_map_get_size(checker->input);
    if (nr == 0)
        return 0;

    int r = 0;

    struct pcutils_map_entry *entry;
    struct pcutils_map_iterator it;
    it = pcutils_map_it_begin_first(checker->input);
    while ((entry = pcutils_map_it_value(&it))) {
        purc_variant_t val = (purc_variant_t)entry->key;
        r = wind_up_val(val, checker);
        if (r)
            break;
        r = pcutils_map_erase(checker->input, val);
        PC_ASSERT(r == 0);
        pcutils_map_it_next(&it);
    }
    pcutils_map_it_end(&it);

    return r ? -1 : 0;
}

void
pcvar_adjust_set_by_descendant(purc_variant_t val)
{
    copy_key_fn copy_key = ref;
    free_key_fn free_key = unref;
    copy_val_fn copy_val = ref;
    free_val_fn free_val = unref;
    comp_key_fn comp_key = comp;

    struct reverse_checker checker = {};

    bool threads = false;
    checker.input = pcutils_map_create(copy_key, free_key,
            copy_val, free_val, comp_key, threads);
    checker.output = pcutils_map_create(copy_key, free_key,
            copy_val, free_val, comp_key, threads);

    int r = -1;
    do {
        if (checker.input == NULL)
            break;
        if (checker.output == NULL)
            break;

        r = pcutils_map_insert(checker.input, val, val);
        if (r)
            break;

        while (1) {
            r = wind_up(&checker);
            if (r)
                break;

            // sanity check
            size_t nr = pcutils_map_get_size(checker.input);
            PC_ASSERT(nr == 0);

            nr = pcutils_map_get_size(checker.output);
            if (nr == 0)
                break;

            struct pcutils_map *tmp;
            tmp = checker.input;
            checker.input = checker.output;
            checker.output = tmp;
        }
    } while (0);

    if (checker.output)
        pcutils_map_destroy(checker.output);
    if (checker.input)
        pcutils_map_destroy(checker.input);

    PC_ASSERT(r == 0);
}

