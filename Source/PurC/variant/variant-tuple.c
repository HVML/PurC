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

    purc_variant_unref(members[idx]);
    members[idx] = purc_variant_ref(value);
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

