/**
 * @file variant-array.c
 * @author Xu Xiaohong (freemine)
 * @date 2021/07/08
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
#include "private/arraylist.h"
#include "private/errors.h"
#include "variant-internals.h"
#include "purc-errors.h"


#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


static void _fill_empty_with_undefined(struct pcutils_arrlist *al)
{
    PC_ASSERT(al);
    for (size_t i=0; i<al->length; ++i) {
        purc_variant_t val = (purc_variant_t)al->array[i];
        if (!val) {
            val = purc_variant_make_undefined();
            al->array[i] = val;
        }
    }
}

purc_variant_t purc_variant_make_array (size_t sz, purc_variant_t value0, ...)
{
    PCVARIANT_CHECK_FAIL_RET((sz==0 && value0==NULL) || (sz>0 && value0),
        PURC_VARIANT_INVALID);

    purc_variant_t var = pcvariant_get(PVT(_ARRAY));
    if (!var) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    do {
        var->type          = PVT(_ARRAY);
        var->refc          = 0; // we'll call purc_variant_ref later

        size_t initial_size = ARRAY_LIST_DEFAULT_SIZE;
        if (sz>initial_size)
            initial_size = sz;

        struct pcutils_arrlist *al;
        al = pcutils_arrlist_new_ex(NULL, initial_size);

        if (!al) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            break;
        }
        var->sz_ptr[1]     = (uintptr_t)al;

        if (sz==0) {
            // empty array
            purc_variant_ref(var);
            return var;
        }

        va_list ap;
        va_start(ap, value0);

        purc_variant_t v = value0;
        // question: shall we track mem for al->array?
        if (pcutils_arrlist_add(al, v)) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            break;
        }

        size_t i = 1;
        while (i<sz) {
            v = va_arg(ap, purc_variant_t);
            if (!v) {
                pcinst_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }

            if (pcutils_arrlist_add(al, v)) {
                pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
                break;
            }
        }
        va_end(ap);

        if (i<sz)
            break;

        purc_variant_ref(var);

        size_t extra = sizeof(*al) + al->size * sizeof(*al->array);
        pcvariant_stat_set_extra_size(var, extra);

        return var;
    } while (0);

    pcvariant_array_release(var);
    pcvariant_put(var);

    return PURC_VARIANT_INVALID;
}

void pcvariant_array_release (purc_variant_t value)
{
    struct pcutils_arrlist *al = (struct pcutils_arrlist*)value->sz_ptr[1];
    if (!al)
        return;

    pcutils_arrlist_free(al);
    value->sz_ptr[1] = (uintptr_t)NULL;
}

int pcvariant_array_compare (purc_variant_t lv, purc_variant_t rv)
{
    // only called via purc_variant_compare
    struct pcutils_arrlist *lal = (struct pcutils_arrlist*)lv->sz_ptr[1];
    struct pcutils_arrlist *ral = (struct pcutils_arrlist*)rv->sz_ptr[1];
    size_t                  lnr = pcutils_arrlist_length(lal);
    size_t                  rnr = pcutils_arrlist_length(ral);

    size_t i = 0;
    for (; i<lnr && i<rnr; ++i) {
        purc_variant_t l = (purc_variant_t)lal->array[i];
        purc_variant_t r = (purc_variant_t)ral->array[i];
        int t = pcvariant_array_compare(l, r);
        if (t)
            return t;
    }

    return i<lnr ? 1 : -1;
}

bool purc_variant_array_append (purc_variant_t array, purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) && value,
        PURC_VARIANT_INVALID);

    struct pcutils_arrlist *al = (struct pcutils_arrlist*)array->sz_ptr[1];
    size_t             nr = pcutils_arrlist_length(al);
    return purc_variant_array_insert_before (array, nr, value);
}

bool purc_variant_array_prepend (purc_variant_t array, purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) && value,
        PURC_VARIANT_INVALID);

    return purc_variant_array_insert_before (array, 0, value);
}

purc_variant_t purc_variant_array_get (purc_variant_t array, int idx)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) && idx>=0,
        PURC_VARIANT_INVALID);

    struct pcutils_arrlist *al = (struct pcutils_arrlist*)array->sz_ptr[1];
    size_t             nr = pcutils_arrlist_length(al);
    PCVARIANT_CHECK_FAIL_RET((size_t)idx>=nr,
        PURC_VARIANT_INVALID);

    purc_variant_t var = (purc_variant_t)pcutils_arrlist_get_idx(al, idx);
    PC_ASSERT(var);

    return var;
}

size_t purc_variant_array_get_size(const purc_variant_t array)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY),
        (size_t)-1);

    struct pcutils_arrlist *al = (struct pcutils_arrlist*)array->sz_ptr[1];
    return pcutils_arrlist_length(al);
}

bool purc_variant_array_set (purc_variant_t array, int idx,
        purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) &&
        idx>=0 && value && array != value,
        PURC_VARIANT_INVALID);

    struct pcutils_arrlist *al = (struct pcutils_arrlist*)array->sz_ptr[1];
    size_t             nr = pcutils_arrlist_length(al);
    if ((size_t)idx>=nr) {
        int t = pcutils_arrlist_put_idx(al, idx, value);

        if (t) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return false;
        }
        // fill empty slot with undefined value
        _fill_empty_with_undefined(al);
        // above two steps might be combined into one for better performance

        // since value is put into array
        purc_variant_ref(value);

        size_t extra = sizeof(*al) + al->size * sizeof(*al->array);
        pcvariant_stat_set_extra_size(array, extra);

        return true;
    } else {
        purc_variant_t v = (purc_variant_t)al->array[idx];
        if (v!=value) {
            purc_variant_unref(v);
            al->array[idx] = value;
        }
        purc_variant_ref(value);
        return true;
    }
}

bool purc_variant_array_remove (purc_variant_t array, int idx)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) && idx>=0,
        PURC_VARIANT_INVALID);

    struct pcutils_arrlist *al = (struct pcutils_arrlist*)array->sz_ptr[1];
    size_t             nr = pcutils_arrlist_length(al);
    if ((size_t)idx>=nr)
        return true; // or false?

    purc_variant_t v = (purc_variant_t)al->array[idx];
    // pcutils_arrlist_del_idx will shrink internally
    if (pcutils_arrlist_del_idx(al, idx, 1)) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }
    purc_variant_unref(v);

    size_t extra = sizeof(*al) + al->size * sizeof(*al->array);
    pcvariant_stat_set_extra_size(array, extra);

    return true;
}

bool purc_variant_array_insert_before (purc_variant_t array, int idx,
        purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) &&
        idx>=0 && value && array != value,
        PURC_VARIANT_INVALID);

    struct pcutils_arrlist *al = (struct pcutils_arrlist*)array->sz_ptr[1];
    size_t             nr = pcutils_arrlist_length(al);
    if ((size_t)idx>=nr)
        idx = (int)nr;

    // expand by 1 empty slot
    if (pcutils_arrlist_shrink(al, 1)) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }

    if ((size_t)idx<nr) {
        // move idx~nr-1 to idx+1~nr
        // pcutils_arrlist has no such api, we have to hack it whatsoever
        // note: overlap problem? man or test!
        memmove(al->array + idx + 1,
                al->array + idx,
                (nr-idx) * sizeof(void *));
    }
    al->array[idx] = value;

    purc_variant_ref(value);

    size_t extra = sizeof(*al) + al->size * sizeof(*al->array);
    pcvariant_stat_set_extra_size(array, extra);

    return true;
}

bool purc_variant_array_insert_after (purc_variant_t array, int idx,
        purc_variant_t value)
{
    return purc_variant_array_insert_before(array, idx+1, value);
}

