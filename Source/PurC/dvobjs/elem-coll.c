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
#include "purc-document.h"
#include "purc-variant.h"
#include "helper.h"
#include "element.h"

#include <limits.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define IS_ELEMENTS         "is_elements"
#define BUFF_MIN            1024
#define BUFF_MAX            1024 * 1024 * 4
#define ATTR_CLASS          "class"

int
pcdoc_elem_coll_update(pcdoc_elem_coll_t elem_coll);

pcdoc_elem_coll_t
pcdoc_elem_coll_new_from_element(purc_document_t doc, pcdoc_element_t elem);

static purc_variant_t
count_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) entity;
    return purc_variant_make_ulongint(elem_coll->nr_elems);
}

static purc_variant_t
sub_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
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
select_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
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
attr_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
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
attr_setter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(entity);
    UNUSED_PARAM(property_name);
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
                pcintr_util_set_attribute(elem_coll->doc, elem,
                        PCDOC_OP_DISPLACE, name, buf, total, true, true);
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
remove_attr_setter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(entity);
    UNUSED_PARAM(property_name);
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
        pcintr_util_set_attribute(elem_coll->doc, elem,
                        PCDOC_OP_ERASE, name, NULL, 0, true, true);
        ret++;
    }

out:
    return purc_variant_make_longint(ret);
}

static purc_variant_t
contents_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);

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
contents_setter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    int ret = -1;
    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }
    else if (nr_args == 1 && !purc_variant_is_string(argv[0])) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    const char *content = purc_variant_get_string_const(argv[0]);
    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) entity;
    size_t len = elem_coll->nr_elems;
    for (size_t i = 0; i < len; i++) {
        pcdoc_element_t elem = pcdoc_elem_coll_get(elem_coll->doc, elem_coll, i);
        pcintr_util_new_content(elem_coll->doc, elem, PCDOC_OP_DISPLACE,
                content, 0, PURC_VARIANT_INVALID, true, true);
        ret++;
    }


out:
    return purc_variant_make_longint(ret);
}

static purc_variant_t
text_content_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
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
text_content_setter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    int ret = -1;
    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }
    else if (nr_args == 1 && !purc_variant_is_string(argv[0])) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    const char *content = purc_variant_get_string_const(argv[0]);
    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) entity;
    size_t len = elem_coll->nr_elems;
    for (size_t i = 0; i < len; i++) {
        pcdoc_element_t elem = pcdoc_elem_coll_get(elem_coll->doc, elem_coll, i);
        pcintr_util_new_text_content(elem_coll->doc, elem, PCDOC_OP_DISPLACE,
                content, 0, true, true);
        ret++;
    }


out:
    return purc_variant_make_longint(ret);
}

static purc_variant_t
data_content_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);

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
data_content_setter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    int ret = -1;
    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) entity;
    size_t len = elem_coll->nr_elems;
    for (size_t i = 0; i < len; i++) {
        pcdoc_element_t elem = pcdoc_elem_coll_get(elem_coll->doc, elem_coll, i);
        pcintr_util_set_data_content(elem_coll->doc, elem, PCDOC_OP_DISPLACE,
                argv[0], true, true);
        ret++;
    }


out:
    return purc_variant_make_longint(ret);
}

static purc_variant_t
has_class_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
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

#define CLASS_SEPARATOR " \f\n\r\t\v"

static purc_variant_t
get_elem_classes(purc_document_t doc, pcdoc_element_t elem, char **klass)
{
    const char *value;
    size_t len;

    purc_variant_t ret = PURC_VARIANT_INVALID;

    /* Acquire read lock */
    pcdoc_document_lock_for_read(doc);

    pcdoc_element_get_special_attr(doc, elem, PCDOC_ATTR_CLASS, &value, &len);

    /* Release lock */
    pcdoc_document_unlock(doc);

    if (value == NULL) {
        goto out;
    }

    ret = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    char *haystack = strndup(value, len);

    char *str;
    char *saveptr;
    char *token;
    for (str = haystack; ; str = NULL) {
        token = strtok_r(str, CLASS_SEPARATOR, &saveptr);
        if (token) {
            purc_variant_t v = purc_variant_make_string(token, false);
            purc_variant_array_append(ret, v);
            purc_variant_unref(v);
        }
        else {
            break;
        }
    }

    if (klass) {
        *klass = haystack;
    }
    else {
        free(haystack);
    }

out:
    return ret;
}

static purc_variant_t
add_class_setter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    int ret = -1;
    purc_variant_t param = PURC_VARIANT_INVALID;
    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }
    else if (nr_args == 1) {
        if (purc_variant_is_string(argv[0])) {
            param = purc_variant_make_array(1, argv[0]);
        }
        else if (purc_variant_is_array(argv[0])) {
            param = purc_variant_ref(argv[0]);
        }
        else {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto out;
        }
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) entity;
    pcdoc_element_t elem = NULL;
    size_t nr_elems = elem_coll->nr_elems;
    size_t nr_param = purc_variant_array_get_size(param);

    ret = 0;
    for (size_t i = 0; i < nr_elems; i++) {
        elem = pcdoc_elem_coll_get(elem_coll->doc, elem_coll, i);
        purc_rwstream_t rws = purc_rwstream_new_buffer(BUFF_MIN, BUFF_MAX);
        if (!rws) {
            goto out_clear_param;
        }

        const char *value;
        size_t len;

        /* Acquire read lock */
        pcdoc_document_lock_for_read(elem_coll->doc);

        pcdoc_element_get_special_attr(elem_coll->doc, elem, PCDOC_ATTR_CLASS,
                &value, &len);

        /* Release lock */
        pcdoc_document_unlock(elem_coll->doc);
        if (value) {
            purc_rwstream_write(rws, value, len);
            purc_rwstream_write(rws, " ", 1);
        }

        for (size_t j = 0; j < nr_param; j++) {
            purc_variant_t v = purc_variant_array_get(param, j);

            char *buf = NULL;
            int total = purc_variant_stringify_alloc(&buf, v);
            if (total) {
                purc_rwstream_write(rws, buf, strlen(buf));
                if (j < nr_param - 1) {
                    purc_rwstream_write(rws, " ", 1);
                }
            }
            free(buf);
        }

        size_t nr_kls = 0;
        const char *kls = purc_rwstream_get_mem_buffer(rws, &nr_kls);
        pcintr_util_set_attribute(elem_coll->doc, elem,
                PCDOC_OP_DISPLACE, ATTR_CLASS, kls, nr_kls, true, true);
        purc_rwstream_destroy(rws);
        ret++;
    }

out_clear_param:
    purc_variant_unref(param);

out:
    return purc_variant_make_longint(ret);
}

static purc_variant_t
remove_class_setter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    int ret = -1;
    purc_variant_t param = PURC_VARIANT_INVALID;
    if (nr_args < 1) {
        param = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    }
    else if (nr_args == 1) {
        if (purc_variant_is_string(argv[0])) {
            param = purc_variant_make_array(1, argv[0]);
        }
        else if (purc_variant_is_array(argv[0])) {
            param = purc_variant_ref(argv[0]);
        }
        else {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto out;
        }
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    pcdoc_element_t elem = NULL;
    pcdoc_elem_coll_t elem_coll = (pcdoc_elem_coll_t) entity;
    size_t nr_elems = elem_coll->nr_elems;
    size_t nr_param = purc_variant_array_get_size(param);

    ret = 0;
    if (nr_param == 0) {
        for (size_t i = 0; i < nr_elems; i++) {
            elem = pcdoc_elem_coll_get(elem_coll->doc, elem_coll, i);
            pcintr_util_set_attribute(elem_coll->doc, elem,
                            PCDOC_OP_ERASE, ATTR_CLASS, NULL, 0, true, true);
            ret++;
        }
        goto out;
    }

    for (size_t i = 0; i < nr_elems; i++) {
        elem = pcdoc_elem_coll_get(elem_coll->doc, elem_coll, i);
        char *klass = NULL;

        purc_variant_t v_kls = get_elem_classes(elem_coll->doc, elem, &klass);
        if (!v_kls) {
            goto out;
        }

        purc_variant_t dst = purc_variant_make_array(0, PURC_VARIANT_INVALID);
        if (!dst) {
            purc_variant_unref(v_kls);
            free(klass);
            goto out;
        }

        size_t nr_kls = purc_variant_array_get_size(v_kls);
        for (size_t j = 0; j < nr_kls; j++) {
            purc_variant_t v = purc_variant_array_get(v_kls, j);
            const char *vs = purc_variant_get_string_const(v);

            bool match = false;
            for (size_t k = 0; k < nr_param; k++) {
                purc_variant_t vk = purc_variant_array_get(param, k);
                const char *vks = purc_variant_get_string_const(vk);
                if (strcasecmp(vs, vks) == 0) {
                    match = true;
                    break;
                }
            }

            if (!match) {
                purc_variant_array_append(dst, v);
            }
        }

        purc_variant_unref(v_kls);
        free(klass);

        size_t nr_dst = purc_variant_array_get_size(dst);
        if (nr_kls != nr_dst) {
            purc_rwstream_t rws = purc_rwstream_new_buffer(BUFF_MIN, BUFF_MAX);
            if (!rws) {
                purc_variant_unref(dst);
                goto out;
            }
            for (size_t x = 0; x < nr_dst; x++) {
                purc_variant_t xv = purc_variant_array_get(dst, x);
                const char *xs = purc_variant_get_string_const(xv);
                purc_rwstream_write(rws, xs, strlen(xs));
                if (x < nr_dst - 1) {
                    purc_rwstream_write(rws, " ", 1);
                }
            }
            size_t nr_s = 0;
            const char *s = purc_rwstream_get_mem_buffer(rws, &nr_s);
            pcintr_util_set_attribute(elem_coll->doc, elem,
                    PCDOC_OP_DISPLACE, ATTR_CLASS, s, nr_s, true, true);
            purc_rwstream_destroy(rws);
        }
        purc_variant_unref(dst);
        ret++;
    }

out:
    if (param) {
        purc_variant_unref(param);
    }
    return purc_variant_make_longint(ret);
}

static purc_variant_t
is_element_getter(void* native_entity, const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
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
    {"contents",    contents_getter,        contents_setter,        NULL, NULL},
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
    struct native_property_cfg *cfg =
        key_name ? property_cfg_by_name(key_name) : NULL;
    if (cfg && cfg->property_getter)
        return cfg->property_getter;

    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

static purc_nvariant_method
property_setter(void *entity, const char *key_name)
{
    UNUSED_PARAM(entity);
    struct native_property_cfg *cfg =
        key_name ? property_cfg_by_name(key_name) : NULL;
    if (cfg && cfg->property_setter)
        return cfg->property_setter;

    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

static purc_nvariant_method
property_eraser(void *entity, const char *key_name)
{
    UNUSED_PARAM(entity);
    struct native_property_cfg *cfg =
        key_name ? property_cfg_by_name(key_name) : NULL;

    if (cfg && cfg->property_eraser)
        return cfg->property_eraser;

    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

static purc_nvariant_method
property_cleaner(void *entity, const char *key_name)
{
    UNUSED_PARAM(entity);
    PC_ASSERT(key_name);
    struct native_property_cfg *cfg =
        key_name ? property_cfg_by_name(key_name) : NULL;

    if (cfg && cfg->property_cleaner)
       return cfg->property_cleaner;

    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
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

        pcintr_util_clear_element(elem_coll->doc, elem, true);
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
        pcintr_util_erase_element(elem_coll->doc, elem, true);
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

    pcdoc_document_lock_for_read(doc);
    elem = pcdoc_find_element_in_document(doc, selector);
    pcdoc_document_unlock(doc);

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

    pcdoc_document_lock_for_read(doc);
    coll = pcdoc_elem_coll_new_from_descendants(doc, ancestor, selector);
    pcdoc_document_unlock(doc);

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

    char *sel = (char *) malloc(strlen(id) + 2);
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

    /* No need to acquire a read lock here since we're only checking
     * if the property getter exists, not accessing document data */
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
    pcdoc_elem_coll_t coll;

    /* No need to acquire a read lock here since pcdoc_elem_coll_new_from_element
     * only creates a new collection and doesn't access document data structures
     * that would require synchronization */
    coll = pcdoc_elem_coll_new_from_element(doc, element);

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

    /* No need to acquire a read lock here since pcdoc_elem_coll_get
     * only accesses the array list of elements that was already collected,
     * and doesn't access document data structures that would require synchronization */
    return pcdoc_elem_coll_get(coll->doc, coll, idx);
}

