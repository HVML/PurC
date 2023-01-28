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

#define IS_ELEMENTS         "is_elements"

int
pcdoc_elem_coll_update(pcdoc_elem_coll_t elem_coll);

pcdoc_elem_coll_t
pcdoc_elem_coll_new_from_element(purc_document_t doc, pcdoc_element_t elem);

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
sub_getter(void *entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(call_flags);

    purc_variant_t ret = PURC_VARIANT_INVALID;
    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) entity;
    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    if (!pcvariant_is_of_number(argv[0]) || !pcvariant_is_of_number(argv[1])) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    int64_t pos = 0;
    purc_variant_cast_to_longint(argv[0], &pos, false);

    int64_t size = 0;
    purc_variant_cast_to_longint(argv[1], &size, false);

    pcdoc_elem_coll_t coll = pcdoc_elem_coll_sub(elem_coll->doc, elem_coll,
            pos, size);
    if (!coll) {
        goto out;
    }

    ret = pcdvobjs_make_elem_coll(coll);
out:
    return ret;
}

static purc_variant_t
select_getter(void *entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(call_flags);

    purc_variant_t ret = PURC_VARIANT_INVALID;
    pcdoc_selector_t selector = NULL;
    pcdoc_elem_coll_t coll = NULL;
    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) entity;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    if (!purc_variant_is_string(argv[0])) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    selector = pcdoc_selector_new(purc_variant_get_string_const(argv[0]));
    if (!selector) {
        goto out_clear_selector;
    }

    coll = pcdoc_elem_coll_select(elem_coll->doc, elem_coll, selector);
    if (!coll) {
        goto out_clear_selector;
    }

    ret = pcdvobjs_make_elem_coll(coll);

out_clear_selector:
    pcdoc_selector_delete(selector);

out:
    return ret;
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

static purc_variant_t
attr_setter(void *entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(entity);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    int ret;
    purc_variant_t param = PURC_VARIANT_INVALID;
    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) entity;
    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        ret = -1;
        goto out;
    }
    else if (nr_args == 1) {
        if (!purc_variant_is_object(argv[0])) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            ret = -1;
            goto out;
        }
        param = purc_variant_ref(argv[0]);
    }
    else {
        if (!purc_variant_is_string(argv[1])) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            ret = -1;
            goto out;
        }

        param = purc_variant_make_object(1, argv[0], argv[1]);
    }

    ret = 0;
    size_t len = elem_coll->nr_elems;
    for (size_t i = 0; i < len; i++) {
        pcdoc_element_t elem = pcdoc_elem_coll_get(elem_coll->doc, elem_coll, i);
        purc_variant_t k, v;
        foreach_key_value_in_variant_object(param, k, v)
            char *buf = NULL;
            int total = purc_variant_stringify_alloc(&buf, v);
            if (total) {
                const char *name = purc_variant_get_string_const(k);
                pcdoc_element_set_attribute(elem_coll->doc,
                        elem, PCDOC_OP_DISPLACE,
                        name, buf, total);
                ret++;
            }
            free(buf);
        end_foreach;
    }

    purc_variant_unref(param);

out:
    return purc_variant_make_longint(ret);
}

static purc_variant_t
remove_attr_setter(void *entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(entity);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    int ret;
    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        ret = -1;
        goto out;
    }
    else if (nr_args == 1 && !purc_variant_is_string(argv[0])) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        ret = -1;
        goto out;
    }

    ret = 0;
    const char *name = purc_variant_get_string_const(argv[0]);
    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) entity;
    size_t len = elem_coll->nr_elems;
    for (size_t i = 0; i < len; i++) {
        pcdoc_element_t elem = pcdoc_elem_coll_get(elem_coll->doc, elem_coll, i);
        pcdoc_element_remove_attribute(elem_coll->doc, elem, name);
        ret++;
    }

out:
    return purc_variant_make_longint(ret);
}

static purc_variant_t
content_getter(void *entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    pcdoc_element_t elem = NULL;
    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) entity;
    size_t nr_elems = elem_coll->nr_elems;
    if (nr_elems == 0) {
        goto out;
    }

    elem = pcdoc_elem_coll_get(elem_coll->doc, elem_coll, 0);
    ret = pcdvobjs_element_content_getter(elem_coll->doc, elem,
            nr_args, argv, (call_flags & PCVRT_CALL_FLAG_SILENTLY));
out:
    return ret;
}

static purc_variant_t
content_setter(void *entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(entity);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    //TODO
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
text_content_getter(void *entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    pcdoc_element_t elem = NULL;
    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) entity;
    size_t nr_elems = elem_coll->nr_elems;
    if (nr_elems == 0) {
        goto out;
    }

    elem = pcdoc_elem_coll_get(elem_coll->doc, elem_coll, 0);
    ret = pcdvobjs_element_text_content_getter(elem_coll->doc, elem,
            nr_args, argv, (call_flags & PCVRT_CALL_FLAG_SILENTLY));
out:
    return ret;
}

static purc_variant_t
text_content_setter(void *entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(entity);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    //TODO
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
data_content_getter(void *entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    pcdoc_element_t elem = NULL;
    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) entity;
    size_t nr_elems = elem_coll->nr_elems;
    if (nr_elems == 0) {
        goto out;
    }

    elem = pcdoc_elem_coll_get(elem_coll->doc, elem_coll, 0);
    ret = pcdvobjs_element_data_content_getter(elem_coll->doc, elem,
            nr_args, argv, (call_flags & PCVRT_CALL_FLAG_SILENTLY));
out:
    return ret;
}

static purc_variant_t
data_content_setter(void *entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(entity);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    //TODO
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
has_class_getter(void *entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    bool has_class = false;
    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) entity;
    pcdoc_element_t elem = NULL;
    size_t nr_elems = elem_coll->nr_elems;
    for (size_t i = 0; i < nr_elems; i++) {
        elem = pcdoc_elem_coll_get(elem_coll->doc, elem_coll, i);

        purc_variant_t v = pcdvobjs_element_has_class_getter(elem_coll->doc,
                elem, nr_args, argv, (call_flags & PCVRT_CALL_FLAG_SILENTLY));
        if (v == PURC_VARIANT_INVALID) {
            continue;
        }

        has_class = purc_variant_booleanize(v);
        purc_variant_unref(v);

        if (has_class) {
            break;
        }
    }

    return purc_variant_make_boolean(has_class);
}

static purc_variant_t
add_class_setter(void *entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(entity);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    //TODO
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
remove_class_setter(void *entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(entity);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    //TODO
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
is_element_getter(void* native_entity, size_t nr_args, purc_variant_t* argv,
        unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    return purc_variant_make_boolean(true);
}

static struct native_property_cfg configs[] = {
    {"count",       count_getter,           NULL,                   NULL, NULL},
    {"sub",         sub_getter,             NULL,                   NULL, NULL},
    {"select",      select_getter,          NULL,                   NULL, NULL},
    {"attr",        attr_getter,            attr_setter,            NULL, NULL},
    {"removeAttr",  NULL,                   remove_attr_setter,     NULL, NULL},
    {"content",     content_getter,         content_setter,         NULL, NULL},
    {"textContent", text_content_getter,    text_content_setter,    NULL, NULL},
    {"dataContent", data_content_getter,    data_content_setter,    NULL, NULL},
    {"hasClass",    has_class_getter,       NULL,                   NULL, NULL},
    {"addClass",    NULL,                   add_class_setter,       NULL, NULL},
    {"removeClass", NULL,                   remove_class_setter,    NULL, NULL},
    {"is_elements", is_element_getter,      NULL,                   NULL, NULL},
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
    UNUSED_PARAM(call_flags);

    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) native_entity;

    pcdoc_element_t elem = NULL;
    size_t len = elem_coll->nr_elems;
    for (size_t i = 0; i < len; i++) {
        elem = pcdoc_elem_coll_get(elem_coll->doc, elem_coll, i);
        if (!elem) {
            continue;
        }

        pcdoc_element_clear(elem_coll->doc, elem);
    }

    return purc_variant_make_boolean(true);
}

static purc_variant_t
eraser(void *native_entity, unsigned call_flags)
{
    UNUSED_PARAM(call_flags);
    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) native_entity;

    pcdoc_element_t elem = NULL;
    size_t len = elem_coll->nr_elems;
    size_t nr_erase = 0;
    for (size_t i = 0; i < len; i++) {
        elem = pcdoc_elem_coll_get(elem_coll->doc, elem_coll, i);
        pcdoc_element_erase(elem_coll->doc, elem);
        nr_erase++;
    }

    return purc_variant_make_ulongint(nr_erase);
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
pcdvobjs_elem_coll_from_descendants(purc_document_t doc,
        pcdoc_element_t ancestor, const char *sel)
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

    coll = pcdoc_elem_coll_new_from_descendants(doc, ancestor, selector);

out_clear_selector:
    pcdoc_selector_delete(selector);

out:
    return coll;
}

purc_variant_t
pcdvobjs_elem_coll_query(purc_document_t doc,
        pcdoc_element_t ancestor, const char *sel)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    pcdoc_elem_coll_t coll = NULL;

    coll = pcdvobjs_elem_coll_from_descendants(doc, ancestor, sel);
    if (!coll) {
        goto out;
    }

    ret = pcdvobjs_make_elem_coll(coll);

out:
    return ret;
}

purc_variant_t
pcdvobjs_elem_coll_select_by_id(purc_document_t doc, const char *id)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;

    char *sel = (char *) malloc(strlen(id) + 1);
    if (!sel) {
        goto out;
    }

    sel[0] = '#';
    strcpy(sel + 1, id);

    ret = pcdvobjs_elem_coll_query(doc, NULL, sel);

    free(sel);

out:
    return ret;
}

bool
pcdvobjs_is_elements(purc_variant_t v)
{
    bool ret = false;
    if (!purc_variant_is_native(v)) {
        goto out;
    }
    void *entity = purc_variant_native_get_entity(v);
    struct purc_native_ops *ops = purc_variant_native_get_ops(v);
    purc_nvariant_method m = ops->property_getter(entity, IS_ELEMENTS);
    if (m) {
        ret = true;
    }

out:
    return ret;
}

purc_variant_t
pcdvobjs_elements_by_css(purc_document_t doc, const char *css)
{
    return pcdvobjs_elem_coll_query(doc, NULL, css);
}

purc_variant_t
pcdvobjs_make_elements(purc_document_t doc, pcdoc_element_t element)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    pcdoc_elem_coll_t coll = pcdoc_elem_coll_new_from_element(doc, element);
    if (coll) {
        ret = pcdvobjs_make_elem_coll(coll);
    }
    return ret;
}

pcdoc_element_t
pcdvobjs_get_element_from_elements(purc_variant_t elems, size_t idx)
{
    void *entity = purc_variant_native_get_entity(elems);
    pcdoc_elem_coll_t coll = (pcdoc_elem_coll_t)entity;

    return pcdoc_elem_coll_get(coll->doc, coll, idx);
}

