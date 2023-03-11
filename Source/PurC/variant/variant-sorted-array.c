/**
 * @file variant-sorted-array.c
 * @author Xue Shuming
 * @date 2022/11/08
 * @brief The implement of sorted array variant.
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
#include "private/sorted-array.h"
#include "private/atom-buckets.h"
#include "variant-internals.h"
#include "purc-errors.h"
#include "purc-utils.h"

#define SORTED_ARRAY_PROP_TYPE      "__pcvariant_sorted_array_type"

#define SORTED_ARRAY_ATOM_BUCKET    ATOM_BUCKET_DEF

static purc_atom_t sorted_array_type_atom = 0;

struct pcvariant_sorted_array {
    struct sorted_array           *sa;
};

void sacb_free_def(void *sortv, void *data)
{
    UNUSED_PARAM(data);
    purc_variant_unref((purc_variant_t)sortv);
}

int sacb_compare_def(const void *sortv1, const void *sortv2)
{
    purc_variant_t v1 = (purc_variant_t) sortv1;
    purc_variant_t v2 = (purc_variant_t) sortv2;
    return purc_variant_compare_ex(v1, v2, PCVRNT_COMPARE_METHOD_AUTO);
}

static purc_variant_t
type_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    return purc_variant_make_boolean(true);
}

struct sorted_array *
get_sorted_array(purc_variant_t array)
{
    void *entity;
    struct purc_native_ops *ops;
    struct pcvariant_sorted_array *psa;
    struct sorted_array *sa = NULL;

    if (UNLIKELY(!(array && array->type == PVT(_NATIVE)))) {
        goto out;
    }

    entity = purc_variant_native_get_entity(array);
    ops = purc_variant_native_get_ops(array);
    if (!ops->property_getter(entity, SORTED_ARRAY_PROP_TYPE)) {
        goto out;
    }

    psa = (struct pcvariant_sorted_array *) entity;
    sa = psa->sa;
out:
    return sa;
}

static purc_nvariant_method
property_getter(void *entity, const char *name)
{
    purc_nvariant_method method = NULL;
    if (!entity || !name) {
        goto out;
    }

    purc_atom_t atom = purc_atom_try_string_ex(SORTED_ARRAY_ATOM_BUCKET, name);
    if (atom == 0) {
        goto out;
    }

    if (atom == sorted_array_type_atom) {
        method = type_getter;
    }

out:
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return method;
}

void on_release(void *native_entity)
{
    struct pcvariant_sorted_array *psa;
    psa = (struct pcvariant_sorted_array *)native_entity;

    struct sorted_array *sa = psa->sa;
    pcutils_sorted_array_destroy(sa);
    free(psa);
}

purc_variant_t
purc_variant_make_sorted_array(unsigned int flags, size_t sz_init,
        pcvrnt_compare_cb cmp)
{
    UNUSED_PARAM(flags);
    UNUSED_PARAM(sz_init);
    UNUSED_PARAM(cmp);

    unsigned int sa_flags = 0;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    struct pcvariant_sorted_array *data;
    data = (struct pcvariant_sorted_array *)calloc(1, sizeof(*data));

    if (!data) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    if (flags & PCVRNT_SAFLAG_DESC) {
        sa_flags |= PCVRNT_SAFLAG_DESC;
    }
    sacb_compare cmp_fn = cmp ? (sacb_compare)cmp : sacb_compare_def;

    data->sa = pcutils_sorted_array_create(sa_flags, sz_init, sacb_free_def,
            cmp_fn);
    if (!data->sa) {
        goto out;
    }

    static const struct purc_native_ops ops = {
        .property_getter = property_getter,
        .on_release = on_release,
    };

    if (sorted_array_type_atom == 0) {
        sorted_array_type_atom = purc_atom_from_static_string_ex(
                SORTED_ARRAY_ATOM_BUCKET, SORTED_ARRAY_PROP_TYPE);
    }

    ret_var = purc_variant_make_native(data, &ops);

out:
    return ret_var;
}


ssize_t
purc_variant_sorted_array_add(purc_variant_t array, purc_variant_t value)
{
    ssize_t ret = -1;
    struct sorted_array *sa = get_sorted_array(array);
    if (sa == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    int r = pcutils_sorted_array_add(sa, value, value, &ret);
    switch (r) {
    case 0:
        purc_variant_ref(value);
        break;

    case -1:
        ret = -1;
        purc_set_error(PURC_ERROR_DUPLICATED);
        break;

    case -2:
        ret = -1;
        purc_set_error(PURC_ERROR_TOO_MANY);
        break;

    case -3:
        ret = -1;
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        break;
    }

out:
    return ret;
}

bool
purc_variant_sorted_array_remove(purc_variant_t array, purc_variant_t value)
{
    struct sorted_array *sa = get_sorted_array(array);
    if (sa == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }
    return pcutils_sorted_array_remove(sa, value);
}

bool
purc_variant_sorted_array_delete(purc_variant_t array, size_t idx)
{
    struct sorted_array *sa = get_sorted_array(array);
    if (sa == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }

    size_t nr_size = pcutils_sorted_array_count(sa);
    if (idx >= nr_size) {
        return false;
    }

    pcutils_sorted_array_delete(sa, idx);
    return true;
}

ssize_t
purc_variant_sorted_array_find(purc_variant_t array, purc_variant_t value)
{
    ssize_t r = -1;
    struct sorted_array *sa = get_sorted_array(array);
    if (sa == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }
    pcutils_sorted_array_find(sa, value, NULL, &r);
out:
    return r;
}

purc_variant_t
purc_variant_sorted_array_get(purc_variant_t array, size_t idx)
{
    struct sorted_array *sa = get_sorted_array(array);
    if (sa == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    size_t nr_size = pcutils_sorted_array_count(sa);
    if (idx > nr_size) {
        return PURC_VARIANT_INVALID;
    }

    void *data = NULL;
    pcutils_sorted_array_get(sa, idx, &data);
    if (data) {
        return (purc_variant_t)data;
    }

    return PURC_VARIANT_INVALID;
}

bool
purc_variant_sorted_array_size(purc_variant_t array, size_t *sz)
{
    struct sorted_array *sa = get_sorted_array(array);
    if (sa == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }

    if (sz) {
        *sz = pcutils_sorted_array_count(sa);
    }
    return true;
}

bool
pcvariant_is_sorted_array(purc_variant_t v)
{
    struct sorted_array *sa = get_sorted_array(v);
    return (sa != NULL);
}

