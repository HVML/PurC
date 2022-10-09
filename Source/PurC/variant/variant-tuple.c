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

    purc_variant_t *members;
    if (argc < PCVARIANT_MIN_TUPLE_SIZE_USING_EXTRA_SPACE) {
        vrt->size = argc;

        members = vrt->vrt_vrt;
    }
    else {
        members = calloc(argc, sizeof(purc_variant_t));

        vrt->size = PCVARIANT_MIN_TUPLE_SIZE_USING_EXTRA_SPACE;
        vrt->sz_ptr[0] = (uintptr_t)argc;   /* real size of the tuple */
        vrt->sz_ptr[1] = (uintptr_t)members;
    }

    if (members == NULL) {
        pcvariant_put(vrt);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    size_t inited = 0;
    if (argv) {
        for (size_t n = 0; n < argc; n++) {
            if (argv[n]) {
                members[n] = purc_variant_ref(argv[n]);
                inited = n + 1;
            }
            else {
                break;
            }
        }
    }

    /* initialize left members as null variants. */
    for (size_t n = inited; n < argc; n++) {
        members[n] = purc_variant_make_null();
    }

    vrt->type = PURC_VARIANT_TYPE_TUPLE;
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

    if (tuple->size >= PCVARIANT_MIN_TUPLE_SIZE_USING_EXTRA_SPACE) {
        free((void *)tuple->sz_ptr[1]);
    }
}

