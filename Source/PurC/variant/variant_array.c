/**
 * @file variant.c
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
 * no matter via `_variant_array_release` or `purc_variant_array_remove`
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

static void _fill_empty_with_undefined(purc_variant_t array, struct array_list *al)
{
    PURC_VARIANT_ASSERT(al);
    for (size_t i=0; i<array_list_length(al); ++i) {
        purc_variant_t val = (purc_variant_t)array_list_get_idx(al, i);
        if (!val) {
            // this is an empty slot
            // we might choose to let it be
            // and check NULL elsewhere
            val = purc_variant_make_undefined();
            int r = array_list_put_idx(al, i, val);
            PURC_VARIANT_ASSERT(r==0); // shall NOT happen
            // no need unref val, ownership is transfered to array
        }
    }
}

PCA_EXPORT purc_variant_t purc_variant_make_array (size_t sz, purc_variant_t value0, ...)
{
    PURC_VARIANT_ASSERT(sz>=1); 
    // what about create empty array [] ?
    // in case when user want to create an empty array [],
    // how shall he call this api?
    // purc_variant_make_array(0, NULL);
    PURC_VARIANT_ASSERT(value0);

    // later, we'll use MACRO rather than malloc directly
    purc_variant_t var = (purc_variant_t)malloc(sizeof(*var));
    if (!var)
        return PURC_VARIANT_INVALID;

    do {
        var->type          = PVT(_ARRAY);
        var->refc          = 1;

        struct array_list *al = array_list_new2(_array_item_free, sz); // what if sz==0, test it!
        if (!al)
            break;
        var->sz_ptr[1]     = al;

        va_list ap;
        va_start(ap, value0);

        purc_variant_t v = value0;
        if (array_list_add(al, v))
            break;
        // add ref
        purc_variant_ref(v);

        size_t i = 1;
        while (i<sz) {
            v = va_arg(ap, purc_variant_t);
            PURC_VARIANT_ASSERT(v);

            if (array_list_add(al, v))
                break;
            // add ref
            purc_variant_ref(v);
        }
        va_end(ap);

        return var;
    } while (0);

    purc_variant_unref(var);
    return PURC_VARIANT_INVALID;
}

extern void _variant_array_release (purc_variant_t value)
{
    // this would be called only via purc_variant_unref once value's refc dec'd to 0
    PURC_VARIANT_ASSERT(array);
    PURC_VARIANT_ASSERT(array->type==PVT(_ARRAY));

    struct array_list *al = (struct array_list*)array->sz_ptr[1];
    if (!al) {
        // this shall happen only when purc_variant_make_array failed OOM
        return;
    }

    // all element in array_list shall be called upon being removed
    // we choose unref via free_fn
    array_list_free(al);
    array->sz_ptr[1] = NULL; // no dangling pointer
}

extern int _variant_array_compare (purc_variant_t lv, purc_variant_t rv)
{
    // todo
    UNUSED_PARAM(lv);
    UNUSED_PARAM(rv);
    return -1;
}

PCA_EXPORT bool purc_variant_array_append (purc_variant_t array, purc_variant_t value)
{
    PURC_VARIANT_ASSERT(array);
    PURC_VARIANT_ASSERT(array->type==PVT(_ARRAY));
    PURC_VARIANT_ASSERT(value);
    PURC_VARIANT_ASSERT(array != value);
    PURC_VARIANT_ASSERT(array->sz_ptr[1]);
    struct array_list *al = (struct array_list*)array->sz_ptr[1];
    size_t             nr = array_list_length(al);
    return purc_variant_array_insert_before (array, nr, value);
}

PCA_EXPORT bool purc_variant_array_prepend (purc_variant_t array, purc_variant_t value)
{
    PURC_VARIANT_ASSERT(array);
    PURC_VARIANT_ASSERT(array->type==PVT(_ARRAY));
    PURC_VARIANT_ASSERT(value);
    PURC_VARIANT_ASSERT(array != value);
    PURC_VARIANT_ASSERT(array->sz_ptr[1]);
    struct array_list *al = (struct array_list*)array->sz_ptr[1];
    return purc_variant_array_insert_before (array, 0, value);
}

PCA_EXPORT purc_variant_t purc_variant_array_get (purc_variant_t array, int idx)
{
    PURC_VARIANT_ASSERT(array);
    PURC_VARIANT_ASSERT(array->type==PVT(_ARRAY));
    PURC_VARIANT_ASSERT(array->sz_ptr[1]);
    struct array_list *al = (struct array_list*)array->sz_ptr[1];
    size_t             nr = array_list_length(al);
    if (idx<0 || idx>=nr) {
        return PURC_VARIANT_INVALID; // NULL or undefined variant?
    }

    purc_variant_t var = (purc_variant_t)array_list_get_idx(al, idx);
    PURC_VARIANT_ASSERT(var); // must valid element, even if undefined or null
    // shall we ref+1?
    // we choose ref+1 here, currently, thus, caller shall unref
    purc_variant_unref(var);

    return var;
}

PCA_EXPORT size_t purc_variant_array_get_size(const purc_variant_t array)
{
    PURC_VARIANT_ASSERT(array);
    PURC_VARIANT_ASSERT(array->type==PVT(_ARRAY));
    PURC_VARIANT_ASSERT(array->sz_ptr[1]);
    struct array_list *al = (struct array_list*)array->sz_ptr[1];
    return array_list_length(al);
}

PCA_EXPORT bool purc_variant_array_set (purc_variant_t array, int idx, purc_variant_t value)
{
    PURC_VARIANT_ASSERT(array);
    PURC_VARIANT_ASSERT(array->type==PVT(_ARRAY));
    PURC_VARIANT_ASSERT(value);
    PURC_VARIANT_ASSERT(idx>=0);
    PURC_VARIANT_ASSERT(array->sz_ptr[1]);
    struct array_list *al = (struct array_list*)array->sz_ptr[1];

    // note: for a valid element in al[idx], al->free_fn would be called upon,
    //       we shall unref that element via al->free_fn
    //       to make element's ref count balance
    if (array_list_put_idx(al, idx, value)) {
        return false;
    }
    // fill empty slot with undefined value
    _fill_empty_with_undefined(array, al);
    // above two steps might be combined into one for better performance

    // since value is put into array
    purc_variant_ref(value);

    return true;
}

PCA_EXPORT bool purc_variant_array_remove (purc_variant_t array, int idx)
{
    PURC_VARIANT_ASSERT(array);
    PURC_VARIANT_ASSERT(array->type==PVT(_ARRAY));
    PURC_VARIANT_ASSERT(idx>=0);
    PURC_VARIANT_ASSERT(array->sz_ptr[1]);
    struct array_list *al = (struct array_list*)array->sz_ptr[1];
    size_t             nr = array_list_length(al);
    if (idx>=nr)
        return true; // or false?

    // array_list_del_idx will shrink internally
    if (array_list_del_idx(al, idx, 1)) {
        return false;
    }

    return true;
}

PCA_EXPORT bool purc_variant_array_insert_before (purc_variant_t array, int idx, purc_variant_t value)
{
    PURC_VARIANT_ASSERT(array);
    PURC_VARIANT_ASSERT(array->type==PVT(_ARRAY));
    PURC_VARIANT_ASSERT(value);
    PURC_VARIANT_ASSERT(idx>=0);
    PURC_VARIANT_ASSERT(array->sz_ptr[1]);
    struct array_list *al = (struct array_list*)array->sz_ptr[1];
    size_t             nr = array_list_length(al);
    if (idx>=nr)
        idx = nr;

    // expand by 1 empty slot
    if (array_list_shrink(al, 1)) {
        return false;
    }
    // move idx~nr-1 to idx+1~nr
    // array_list has no such api, we have to hack it whatsoever
    // note: overlap problem? man or test!
	memmove(al->array + idx + 1, arr->array + idx, 1 * sizeof(void *));
    al->array[idx] = NULL; // say no to double free

    // note: for a valid element in al[idx], al->free_fn would be called upon,
    //       we shall unref that element via al->free_fn
    //       to make ref count balance
    if (array_list_put_idx(al, idx, value)) {
        return false;
    }

    // since this value is added
    purc_variant_ref(value);

    return true;
}

PCA_EXPORT bool purc_variant_array_insert_after (purc_variant_t array, int idx, purc_variant_t value)
{
    return purc_variant_array_insert_before(array, idx+1, value);
}









// funcs other than array-related to implement later
extern void _variant_object_release (purc_variant_t value)
{
    // todo
    UNUSED_PARAM(value);
}

extern int _variant_object_compare (purc_variant_t lv, purc_variant_t rv)
{
    // todo
    UNUSED_PARAM(lv);
    UNUSED_PARAM(rv);
    return -1;
}

extern void _variant_set_release (purc_variant_t value)
{
    // todo
    UNUSED_PARAM(value);
}

extern int _variant_set_compare (purc_variant_t lv, purc_variant_t rv)
{
    // todo
    UNUSED_PARAM(lv);
    UNUSED_PARAM(rv);
    return -1;
}

