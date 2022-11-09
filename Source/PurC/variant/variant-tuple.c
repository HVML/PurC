/**
 * @file variant-tuple.c
 * @author Vincent Wei
 * @date 2022/06/06
 * @brief The implement of tuple variant.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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
#include "private/errors.h"
#include "variant-internals.h"
#include "purc-errors.h"
#include "purc-utils.h"

static inline void
changed(purc_variant_t tuple, purc_variant_t pos,
        purc_variant_t o, purc_variant_t n,
        bool check)
{
    if (!check)
        return;

    purc_variant_t vals[] = { pos, o, n };

    pcvariant_on_post_fired(tuple, PCVAR_OPERATION_CHANGE,
            PCA_TABLESIZE(vals), vals);
}

static inline bool
change(purc_variant_t tuple, purc_variant_t pos,
        purc_variant_t o, purc_variant_t n,
        bool check)
{
    if (!check)
        return true;

    purc_variant_t vals[] = { pos, o, n };

    return pcvariant_on_pre_fired(tuple, PCVAR_OPERATION_CHANGE,
            PCA_TABLESIZE(vals), vals);
}

static int
check_change(purc_variant_t tuple, size_t idx, purc_variant_t val)
{
    if (!pcvar_container_belongs_to_set(tuple))
        return 0;

    size_t sz;
    purc_variant_t *members;
    members = tuple_members(tuple, &sz);

    purc_variant_t _new = purc_variant_make_tuple(sz, NULL);
    if (_new == PURC_VARIANT_INVALID) {
        return -1;
    }

    bool r = false;
    do {
        bool found = false;
        purc_variant_t v;
        for (size_t i = 0; i < sz; i++) {
            v = members[i];
            if (i == idx) {
                found = true;
            }

            r = purc_variant_tuple_set(_new, i, i == idx ? val : v);
            if (!r)
                break;
        };

        if (!r)
            break;

        PC_ASSERT(found);

        r = pcvar_reverse_check(tuple, _new);
        if (r)
            break;

        PURC_VARIANT_SAFE_CLEAR(_new);

        return 0;
    } while (0);

    PURC_VARIANT_SAFE_CLEAR(_new);
    return -1;
}

purc_variant_t purc_variant_make_tuple(size_t argc, purc_variant_t *argv)
{
    purc_variant_t vrt = pcvariant_get(PVT(_TUPLE));
    if (!vrt) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    variant_tuple_t data = (variant_tuple_t)calloc(1, sizeof(*data));
    if (!data) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    data->members = calloc(argc, sizeof(purc_variant_t));
    if (data->members == NULL) {
        pcvariant_put(vrt);
        free(data);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    vrt->sz_ptr[0] = (uintptr_t)argc;   /* real size of the tuple */
    vrt->sz_ptr[1] = (uintptr_t)data;


    size_t inited = 0;
    if (argv) {
        for (size_t n = 0; n < argc; n++) {
            if (argv[n]) {
                data->members[n] = purc_variant_ref(argv[n]);
                inited = n + 1;
            }
            else {
                break;
            }
        }
    }

    /* initialize left members as null variants. */
    for (size_t n = inited; n < argc; n++) {
        data->members[n] = purc_variant_make_null();
    }

    vrt->type = PURC_VARIANT_TYPE_TUPLE;
    vrt->flags = PCVARIANT_FLAG_EXTRA_SIZE;
    vrt->refc = 1;
    return vrt;
}

bool purc_variant_tuple_size(purc_variant_t tuple, size_t *sz)
{
    purc_variant_t *members = tuple_members(tuple, sz);
    if (members == NULL)
        return false;

    return true;
}

purc_variant_t purc_variant_tuple_get(purc_variant_t tuple, size_t idx)
{
    size_t sz;

    purc_variant_t *members = tuple_members(tuple, &sz);
    if (members == NULL || idx >= sz)
        return PURC_VARIANT_INVALID;

    return members[idx];
}

bool purc_variant_tuple_set(purc_variant_t tuple,
        size_t idx, purc_variant_t value)
{
    size_t sz;

    purc_variant_t *members = tuple_members(tuple, &sz);
    if (members == NULL || idx >= sz)
        return false;

    assert(value);
    /* do not change */
    if (value == members[idx])
        return true;


    purc_variant_t old = purc_variant_ref(members[idx]);
    purc_variant_t pos = purc_variant_make_longint(idx);
    if (!change(tuple, pos, old, value, true)) {
        purc_variant_unref(old);
        purc_variant_unref(pos);
        return false;
    }

    if (check_change(tuple, idx, value)) {
        purc_variant_unref(old);
        purc_variant_unref(pos);
        return false;
    }

    purc_variant_unref(members[idx]);
    members[idx] = purc_variant_ref(value);

    pcvar_adjust_set_by_descendant(tuple);
    changed(tuple, pos, old, value, true);

    purc_variant_unref(old);
    purc_variant_unref(pos);
    return true;
}

purc_variant_t
pcvariant_tuple_clone(purc_variant_t tuple, bool recursively)
{
    size_t sz;
    purc_variant_t *members = tuple_members(tuple, &sz);
    purc_variant_t cloned;

    cloned = purc_variant_make_tuple(sz, NULL);
    if (cloned == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    for (size_t n = 0; n < sz; n++) {
        purc_variant_t nv;
        if (recursively) {
            nv = pcvariant_container_clone(members[n], recursively);
            if (nv == PURC_VARIANT_INVALID) {
                goto failed;
            }
        }
        else {
            nv = purc_variant_ref(members[n]);
        }

        members[n] = nv;
    }

    return cloned;

failed:
    purc_variant_unref(cloned);
    return PURC_VARIANT_INVALID;
}

void pcvariant_tuple_release(purc_variant_t tuple)
{
    size_t sz;
    purc_variant_t *members = tuple_members(tuple, &sz);
    assert(members != NULL);

    for (size_t n = 0; n < sz; n++) {
        PURC_VARIANT_SAFE_CLEAR(members[n]);
    }

    variant_tuple_t data = (variant_tuple_t) tuple->sz_ptr[1];
    free(data->members);
    free(data);
}

static void
it_refresh(struct tuple_iterator *it, size_t idx)
{
    size_t sz;
    purc_variant_t *members = tuple_members(it->tuple, &sz);

    it->idx = idx;
    it->curr = members[idx];
    it->prev = idx > 0 ? members[idx - 1] : PURC_VARIANT_INVALID;
    it->next = idx < sz - 1 ? members[idx + 1] : PURC_VARIANT_INVALID;
}

struct tuple_iterator
pcvar_tuple_it_first(purc_variant_t tuple)
{
    struct tuple_iterator it = {
        .tuple = tuple,
        .nr_members = 0,
        .idx  = -1,
        .curr = PURC_VARIANT_INVALID,
        .next = PURC_VARIANT_INVALID,
        .prev = PURC_VARIANT_INVALID,
    };

    if (tuple == PURC_VARIANT_INVALID) {
        goto out;
    }

    size_t nr = purc_variant_tuple_get_size(tuple);
    if (nr == 0) {
        goto out;
    }

    it.nr_members = nr;
    it_refresh(&it, 0);
out:
    return it;
}

struct tuple_iterator
pcvar_tuple_it_last(purc_variant_t tuple)
{
    struct tuple_iterator it = {
        .tuple = tuple,
        .nr_members = 0,
        .idx  = -1,
        .curr = PURC_VARIANT_INVALID,
        .next = PURC_VARIANT_INVALID,
        .prev = PURC_VARIANT_INVALID,
    };

    if (tuple == PURC_VARIANT_INVALID) {
        goto out;
    }

    size_t nr = purc_variant_tuple_get_size(tuple);
    if (nr == 0) {
        goto out;
    }

    it.nr_members = nr;
    it_refresh(&it, nr - 1);
out:
    return it;
}

void
pcvar_tuple_it_next(struct tuple_iterator *it)
{
    if (it->curr == PURC_VARIANT_INVALID) {
        goto out;
    }

    size_t idx = it->idx + 1;
    if (idx < it->nr_members) {
        it_refresh(it, idx);
    }
    else {
        it->idx = -1;
        it->curr = PURC_VARIANT_INVALID;
        it->next = PURC_VARIANT_INVALID;
        it->prev = PURC_VARIANT_INVALID;
    }

out:
    return;
}

void
pcvar_tuple_it_prev(struct tuple_iterator *it)
{
    if (it->curr == PURC_VARIANT_INVALID) {
        goto out;
    }

    if (it->idx > 0) {
        it_refresh(it, it->idx - 1);
    }
    else {
        it->idx = -1;
        it->curr = PURC_VARIANT_INVALID;
        it->next = PURC_VARIANT_INVALID;
        it->prev = PURC_VARIANT_INVALID;
    }

out:
    return;
}

variant_tuple_t
pcvar_tuple_get_data(purc_variant_t tuple)
{
    return (variant_tuple_t)tuple->sz_ptr[1];
}

void
pcvar_tuple_break_edge_to_parent(purc_variant_t tuple,
        struct pcvar_rev_update_edge *edge)
{
    PC_ASSERT(purc_variant_is_tuple(tuple));
    variant_tuple_t data = pcvar_tuple_get_data(tuple);
    if (!data)
        return;

    if (!data->rev_update_chain)
        return;

    pcutils_map_erase(data->rev_update_chain, edge->tuple_me);
}

int
pcvar_tuple_build_edge_to_parent(purc_variant_t tuple,
        struct pcvar_rev_update_edge *edge)
{
    PC_ASSERT(purc_variant_is_tuple(tuple));
    variant_tuple_t data = pcvar_tuple_get_data(tuple);
    if (!data)
        return 0;

    if (!data->rev_update_chain) {
        data->rev_update_chain = pcvar_create_rev_update_chain();
        if (!data->rev_update_chain)
            return -1;
    }

    pcutils_map_entry *entry;
    entry = pcutils_map_find(data->rev_update_chain, edge->tuple_me);
    if (entry)
        return 0;

    int r;
    r = pcutils_map_insert(data->rev_update_chain,
            edge->tuple_me, edge->parent);

    return r ? -1 : 0;
}

int
pcvar_tuple_build_rue_downward(purc_variant_t tuple)
{
    PC_ASSERT(purc_variant_is_tuple(tuple));

    variant_tuple_t data = pcvar_tuple_get_data(tuple);
    if (!data)
        return 0;

    size_t sz;
    purc_variant_t *members;
    members = tuple_members(tuple, &sz);

    purc_variant_t v;
    for (size_t idx = 0; idx < sz; idx++) {
        v = members[idx];
        struct pcvar_rev_update_edge edge = {
            .parent         = tuple,
            .tuple_me       = (struct tuple_node*)v,
        };
        int r = pcvar_build_edge_to_parent(v, &edge);
        if (r)
            return -1;
        r = pcvar_build_rue_downward(v);
        if (r)
            return -1;
    }

    return 0;
}

void
pcvar_tuple_break_rue_downward(purc_variant_t tuple)
{
    PC_ASSERT(purc_variant_is_tuple(tuple));

    variant_tuple_t data = pcvar_tuple_get_data(tuple);
    if (!data)
        return;

    size_t sz;
    purc_variant_t *members;
    members = tuple_members(tuple, &sz);

    purc_variant_t v;
    for (size_t idx = 0; idx < sz; idx++) {
        v = members[idx];
        struct pcvar_rev_update_edge edge = {
            .parent         = tuple,
            .tuple_me       = (struct tuple_node*)v,
        };
        pcvar_break_edge_to_parent(v, &edge);
        pcvar_break_rue_downward(v);
    }
}

