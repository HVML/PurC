/**
 * @file element.c
 * @author Xu Xiaohong
 * @date 2021/11/18
 * @brief The internal interfaces for interpreter/element
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
 *
 */

#include "config.h"

#include "element.h"

static inline bool
element_eraser(struct pcintr_element *element)
{
    element->elem = NULL;
    free(element);
    return true;
}

static inline bool
eraser(void *native_entity)
{
    PC_ASSERT(native_entity);

    struct pcintr_element *element;
    element = (struct pcintr_element*)native_entity;

    return element_eraser(element);
}

static inline purc_variant_t
element_attr_getter_by_type(struct pcintr_element *element,
        purc_variant_t tn)
{
    UNUSED_PARAM(element);
    const char *name = purc_variant_get_string_const(tn);
    if (strcmp(name, "class") == 0) {
        const char *v_class = "attr.class:not_implemented_yet";
        // TODO: fetch element's attribute value with `class` matched
        // FIXME: static or else? who lives longer, element or v_class?
        return purc_variant_make_string_static(v_class, true);
    }
    PC_ASSERT(0); // Not implemented yet
    return PURC_VARIANT_INVALID;
}

static inline purc_variant_t
element_attr_getter(struct pcintr_element *element,
        size_t nr_args, purc_variant_t* argv)
{
    if (nr_args == 1) {
        if (argv == NULL || argv[0] == PURC_VARIANT_INVALID) {
            pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
            return PURC_VARIANT_INVALID;
        }
        purc_variant_t tn = argv[0]; // type name
        if (!purc_variant_is_string(tn)) {
            pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
            return PURC_VARIANT_INVALID;
        }
        return element_attr_getter_by_type(element, tn);
    }

    pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
    return PURC_VARIANT_INVALID;
}

static inline purc_variant_t
attr_getter(void* native_entity, size_t nr_args, purc_variant_t* argv)
{
    PC_ASSERT(native_entity);

    struct pcintr_element *element;
    element = (struct pcintr_element*)native_entity;

    return element_attr_getter(element, nr_args, argv);
}

static inline purc_nvariant_method
property_getter(const char* key_name)
{
    if (strcmp(key_name, "attr") == 0) {
        return attr_getter;
    }

    pcinst_set_error(PURC_ERROR_NOT_EXISTS);
    return NULL;
}

static inline purc_variant_t
make_element(struct pcedom_element *elem)
{
    struct pcintr_element *element;
    element = (struct pcintr_element*)calloc(1, sizeof(*element));
    if (!element) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    struct purc_native_ops ops = {
        .property_getter             = property_getter,
        .property_setter             = NULL,
        .property_eraser             = NULL,
        .property_cleaner            = NULL,
        .cleaner                     = NULL,
        .eraser                      = eraser,
        .observe                     = NULL,
    };

    purc_variant_t v;
    v = purc_variant_make_native(element, &ops);
    if (v == PURC_VARIANT_INVALID) {
        free(element);
        return PURC_VARIANT_INVALID;
    }

    element->elem = elem;

    return v;
}

static inline bool
set_make_elements(purc_variant_t set,
        size_t nr_elems, struct pcedom_element **elems)
{
    UNUSED_PARAM(set);
    for (size_t i=0; i<nr_elems; ++i) {
        struct pcedom_element *elem;
        elem = elems[i];
        PC_ASSERT(elem);
        PC_ASSERT(0); // Not implemented yet
        // if (!set_add_element(set, elem))
        //     return false;
    }
    return true;
}

