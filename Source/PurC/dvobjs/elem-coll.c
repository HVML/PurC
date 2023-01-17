/*
 * @file elem-coll.c
 * @author Xue Shuming
 * @date 2023/01/17
 * @brief The implementation of element collection dynamic variant object.
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

#include "private/instance.h"
#include "private/errors.h"
#include "private/dvobjs.h"
#include "purc-variant.h"
#include "helper.h"
#include "internal.h"

#include <limits.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int
pcdoc_elem_coll_update(pcdoc_elem_coll_t elem_coll);

static purc_variant_t
count_getter(void *entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) entity;
    return purc_variant_make_ulongint(elem_coll->nr_elems);
}

static purc_variant_t
attr_getter(void *entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) entity;
    if (elem_coll && elem_coll->nr_elems) {
        pcdoc_element_t elem = pcdoc_elem_coll_get(elem_coll->doc, elem_coll, 0);
        return pcdvobjs_element_attr_getter(elem_coll->doc, elem,
            nr_args, argv, (call_flags & PCVRT_CALL_FLAG_SILENTLY));
    }
    else {
        return PURC_VARIANT_INVALID;
    }
}

static struct native_property_cfg configs[] = {
    {"count",   count_getter,   NULL,   NULL,   NULL},
    {"attr",    attr_getter,    NULL,   NULL,   NULL},
};

static struct native_property_cfg*
property_cfg_by_name(const char *key_name)
{
    for (size_t i = 0; i < PCA_TABLESIZE(configs); ++i) {
        struct native_property_cfg *cfg = configs + i;
        const char *property_name = cfg->property_name;
        if (strcmp(property_name, key_name) == 0) {
            return cfg;
        }
    }
    return NULL;
}

static purc_nvariant_method
property_getter(void *entity, const char *key_name)
{
    UNUSED_PARAM(entity);
    struct native_property_cfg *cfg = property_cfg_by_name(key_name);
    return cfg ? cfg->property_getter : NULL;
}

static purc_nvariant_method
property_setter(void *entity, const char *key_name)
{
    UNUSED_PARAM(entity);
    struct native_property_cfg *cfg = property_cfg_by_name(key_name);
    return cfg ? cfg->property_setter : NULL;
}

static purc_nvariant_method
property_eraser(void *entity, const char *key_name)
{
    UNUSED_PARAM(entity);
    struct native_property_cfg *cfg = property_cfg_by_name(key_name);
    return cfg ? cfg->property_eraser : NULL;
}

static purc_nvariant_method
property_cleaner(void *entity, const char *key_name)
{
    UNUSED_PARAM(entity);
    PC_ASSERT(key_name);
    struct native_property_cfg *cfg = property_cfg_by_name(key_name);
    return cfg ? cfg->property_cleaner : NULL;
}

static purc_variant_t
cleaner(void *native_entity, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(call_flags);
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return purc_variant_make_boolean(false);
}

static purc_variant_t
eraser(void *native_entity, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(call_flags);
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return purc_variant_make_ulongint(0);
}

static bool
did_matched(void *native_entity, purc_variant_t val)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(val);

    bool ret = false;
    void *comp = NULL;
    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) native_entity;

    if (purc_variant_is_string(val)) {
        const char *selector = purc_variant_get_string_const(val);
        comp = pcdvobjs_find_element_in_doc(elem_coll->doc, selector);
    }
    else if (purc_variant_is_native(val)) {
        comp = purc_variant_native_get_entity(val);
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (elem_coll->doc->age > elem_coll->doc_age) {
        int r = pcdoc_elem_coll_update(elem_coll);
        if (r) {
            goto out;
        }
    }

    if (comp && elem_coll->nr_elems) {
        pcdoc_element_t elem = NULL;
        for (size_t i = 0; i < elem_coll->nr_elems; i++) {
            elem = pcdoc_elem_coll_get(elem_coll->doc, elem_coll, i);
            if (elem == comp) {
                ret = true;
                break;
            }
        }
    }

out:
    return ret;
}

static bool
on_observe(void *native_entity,
        const char *event_name, const char *event_subname)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(event_name);
    UNUSED_PARAM(event_subname);
    return true;
}

static void
on_release(void *native_entity)
{
    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) native_entity;
    pcdoc_elem_coll_delete(elem_coll->doc, elem_coll);
}

purc_variant_t
pcdvobjs_make_elem_coll(pcdoc_elem_coll_t elem_coll)
{
    static struct purc_native_ops ops = {
        .property_getter            = property_getter,
        .property_setter            = property_setter,
        .property_eraser            = property_eraser,
        .property_cleaner           = property_cleaner,

        .updater                    = NULL,
        .cleaner                    = cleaner,
        .eraser                     = eraser,
        .did_matched                = did_matched,

        .on_observe                 = on_observe,
        .on_release                 = on_release,
    };

    return purc_variant_make_native(elem_coll, &ops);
}

pcdoc_element_t
pcdvobjs_find_element_in_doc(purc_document_t doc, const char *sel)
{
    pcdoc_element_t elem = NULL;
    pcdoc_selector_t selector = NULL;
    if (!doc || !sel) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    selector = pcdoc_selector_new(sel);
    if (!selector) {
        goto out_clear_selector;
    }

    elem = pcdoc_find_element_in_document(doc, selector);

out_clear_selector:
    pcdoc_selector_delete(selector);

out:
    return elem;
}

pcdoc_elem_coll_t
pcdvobjs_elem_coll_from_doc(purc_document_t doc, const char *sel)
{
    pcdoc_elem_coll_t coll = NULL;
    pcdoc_selector_t selector = NULL;

    if (!doc || !sel) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    selector = pcdoc_selector_new(sel);
    if (!selector) {
        goto out_clear_selector;
    }

    coll = pcdoc_elem_coll_new_from_document(doc, selector);

out_clear_selector:
    pcdoc_selector_delete(selector);

out:
    if (selector) {
        pcdoc_selector_delete(selector);
    }
    return coll;
}

purc_variant_t
pcdvobjs_elem_coll_select_by_id(purc_document_t doc, const char *id)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    pcdoc_elem_coll_t coll = NULL;

    char *sel = (char *) malloc(strlen(id) + 1);
    if (!sel) {
        goto out;
    }

    sel[0] = '#';
    strcpy(sel + 1, id);

    coll = pcdvobjs_elem_coll_from_doc(doc, sel);
    if (!coll) {
        goto out_clear_sel;
    }

    ret = pcdvobjs_make_elem_coll(coll);

out_clear_sel:
    free(sel);

out:
    return ret;
}

purc_variant_t
pcdvobjs_elem_coll_query(purc_document_t doc, const char *sel)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    pcdoc_elem_coll_t coll = NULL;

    coll = pcdvobjs_elem_coll_from_doc(doc, sel);
    if (!coll) {
        goto out;
    }

    ret = pcdvobjs_make_elem_coll(coll);

out:
    return ret;
}

