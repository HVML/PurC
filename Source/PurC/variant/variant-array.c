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
#include "purc-errors.h"


#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


/*
 * array holds purc_variant_t values
 * once a purc_variant_t value is added into array,
 * no matter via `purc_variant_make_array` or `purc_variant_array_set/append/...`,
 * this value's ref + 1
 * once a purc_variant_t value in array get removed,
 * no matter via `pcvariant_array_release` or `purc_variant_array_remove`
 * or even implicitly being overwritten by `purc_variant_array_set/append/...`,
 * this value's ref - 1
 * note: value can be added into array for more than 1 times,
 *       but being noted, ref + 1 once it gets added
 *
 * thinking: shall we recursively check if there's ref-loop among array and it's
 *           children element?
 */

static void _array_item_free(void *data)
// shall we move implementation of this function to the bottom of this file?
{
    PURC_VARIANT_ASSERT(data);
    purc_variant_t val = (purc_variant_t)data;
    purc_variant_unref(val);
}

static void _fill_empty_with_undefined(struct pcutils_arrlist *al)
{
    PURC_VARIANT_ASSERT(al);
    for (size_t i=0; i<pcutils_arrlist_length(al); ++i) {
        purc_variant_t val = (purc_variant_t)pcutils_arrlist_get_idx(al, i);
        if (!val) {
            // this is an empty slot
            // we might choose to let it be
            // and check NULL elsewhere
            val = purc_variant_make_undefined();
            int r = pcutils_arrlist_put_idx(al, i, val);
            if (r) {
                // we shall check in both debug and release build
                PURC_VARIANT_ASSERT(r==0); // shall NOT happen
            }
            // no need unref val, ownership is transfered to array
        }
    }
}

purc_variant_t purc_variant_make_array (size_t sz, purc_variant_t value0, ...)
{
    if (sz==0 && value0) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    // later, we'll use MACRO rather than malloc directly
    purc_variant_t var = (purc_variant_t)malloc(sizeof(*var));
    if (!var) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    do {
        var->type          = PVT(_ARRAY);
        var->refc          = 1;

        size_t initial_size = ARRAY_LIST_DEFAULT_SIZE;
        if (sz>initial_size)
            initial_size = sz;

        struct pcutils_arrlist *al = pcutils_arrlist_new_ex(_array_item_free, initial_size);

        if (!al)
            break;
        var->sz_ptr[1]     = (uintptr_t)al;

        if (sz==0) {
            // empty array
            return var;
        }

        va_list ap;
        va_start(ap, value0);

        purc_variant_t v = value0;
        if (pcutils_arrlist_add(al, v)) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            break;
        }
        // add ref
        purc_variant_ref(v);

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
            // add ref
            purc_variant_ref(v);
        }
        va_end(ap);

        if (i<sz)
            break;

        return var;
    } while (0);

    // cleanup
    struct pcutils_arrlist *al = (struct pcutils_arrlist*)var->sz_ptr[1];
    pcutils_arrlist_free(al);
    var->sz_ptr[1] = (uintptr_t)NULL; // say no to double free
    // todo: use macro instead
    free(var);

    return PURC_VARIANT_INVALID;
}

void pcvariant_array_release (purc_variant_t value)
{
    // this would be called only via purc_variant_unref once value's refc dec'd to 0
    // thus we don't check argument

    struct pcutils_arrlist *al = (struct pcutils_arrlist*)value->sz_ptr[1];
    if (!al) {
        // this shall happen only when purc_variant_make_array failed OOM
        return;
    }

    // all element in pcutils_arrlist shall be called upon being removed
    // we choose unref via free_fn
    pcutils_arrlist_free(al);
    value->sz_ptr[1] = (uintptr_t)NULL; // no dangling pointer
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
    if (!array || array->type!=PVT(_ARRAY) || !value || array == value || !array->sz_ptr[1]) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    struct pcutils_arrlist *al = (struct pcutils_arrlist*)array->sz_ptr[1];
    size_t             nr = pcutils_arrlist_length(al);
    return purc_variant_array_insert_before (array, nr, value);
}

bool purc_variant_array_prepend (purc_variant_t array, purc_variant_t value)
{
    if (!array || array->type!=PVT(_ARRAY) || !value || array == value || !array->sz_ptr[1]) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    return purc_variant_array_insert_before (array, 0, value);
}

purc_variant_t purc_variant_array_get (purc_variant_t array, int idx)
{
    if (!array || array->type!=PVT(_ARRAY) || idx<0 || !array->sz_ptr[1]) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    struct pcutils_arrlist *al = (struct pcutils_arrlist*)array->sz_ptr[1];
    size_t             nr = pcutils_arrlist_length(al);
    if ((size_t)idx>=nr) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID; // NULL or undefined variant?
    }

    purc_variant_t var = (purc_variant_t)pcutils_arrlist_get_idx(al, idx);
    PURC_VARIANT_ASSERT(var); // must valid element, even if undefined or null
    // shall we ref+1?
    // we choose ref+1 here, currently, thus, caller shall unref
    purc_variant_ref(var);

    return var;
}

size_t purc_variant_array_get_size(const purc_variant_t array)
{
    if (!array || array->type!=PVT(_ARRAY) || !array->sz_ptr[1]) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1; // api signature?
    }

    struct pcutils_arrlist *al = (struct pcutils_arrlist*)array->sz_ptr[1];
    return pcutils_arrlist_length(al);
}

bool purc_variant_array_set (purc_variant_t array, int idx, purc_variant_t value)
{
    if (!array || array->type!=PVT(_ARRAY) || !value || array == value || idx<0 || !array->sz_ptr[1]) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    struct pcutils_arrlist *al = (struct pcutils_arrlist*)array->sz_ptr[1];

    // note: for a valid element in al[idx], al->free_fn would be called upon,
    //       we shall unref that element via al->free_fn
    //       to make element's ref count balance

    // to avoid self-overwritten
    purc_variant_ref(value);
    int t = pcutils_arrlist_put_idx(al, idx, value);
    purc_variant_unref(value);

    if (t) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }
    // fill empty slot with undefined value
    _fill_empty_with_undefined(al);
    // above two steps might be combined into one for better performance

    // since value is put into array
    purc_variant_ref(value);

    return true;
}

bool purc_variant_array_remove (purc_variant_t array, int idx)
{
    if (!array || array->type!=PVT(_ARRAY) || idx<0 || !array->sz_ptr[1]) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    struct pcutils_arrlist *al = (struct pcutils_arrlist*)array->sz_ptr[1];
    size_t             nr = pcutils_arrlist_length(al);
    if ((size_t)idx>=nr)
        return true; // or false?

    // pcutils_arrlist_del_idx will shrink internally
    if (pcutils_arrlist_del_idx(al, idx, 1)) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }

    return true;
}

bool purc_variant_array_insert_before (purc_variant_t array, int idx, purc_variant_t value)
{
    if (!array || array->type!=PVT(_ARRAY) || !value || array == value || idx<0 || !array->sz_ptr[1]) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    struct pcutils_arrlist *al = (struct pcutils_arrlist*)array->sz_ptr[1];
    size_t             nr = pcutils_arrlist_length(al);
    if ((size_t)idx>=nr)
        idx = (int)nr;

    // expand by 1 empty slot
    if (pcutils_arrlist_shrink(al, 1)) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }
    // move idx~nr-1 to idx+1~nr
    // pcutils_arrlist has no such api, we have to hack it whatsoever
    // note: overlap problem? man or test!
	memmove(al->array + idx + 1, al->array + idx, 1 * sizeof(void *));
    al->array[idx] = NULL; // say no to double free

    // note: for a valid element in al[idx], al->free_fn would be called upon,
    //       we shall unref that element via al->free_fn
    //       to make ref count balance
    if (pcutils_arrlist_put_idx(al, idx, value)) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }

    // since this value is added
    purc_variant_ref(value);

    return true;
}

bool purc_variant_array_insert_after (purc_variant_t array, int idx, purc_variant_t value)
{
    return purc_variant_array_insert_before(array, idx+1, value);
}

