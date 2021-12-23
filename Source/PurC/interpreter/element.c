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

struct pcintr_element
{
    struct pcedom_element          *elem;       // NOTE: no ownership
};

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
set_add_element(purc_variant_t set, struct pcedom_element *elem)
{
    purc_variant_t v;
    v = make_element(elem);
    if (v == PURC_VARIANT_INVALID)
        return false;

    bool ok;
    ok = purc_variant_set_add(set, v, true); // FIXME: true or false!!!!
    if (!ok) {
        purc_variant_unref(v);
        return false;
    }

    return true;
}

static inline bool
set_make_elements(purc_variant_t set,
        size_t nr_elems, struct pcedom_element **elems)
{
    for (size_t i=0; i<nr_elems; ++i) {
        struct pcedom_element *elem;
        elem = elems[i];
        if (!set_add_element(set, elem))
            return false;
    }
    return true;
}

purc_variant_t
pcintr_make_elements(size_t nr_elems, struct pcedom_element **elems)
{
    purc_variant_t v;
    v = purc_variant_make_set_by_ckey(0, NULL, PURC_VARIANT_INVALID);
    if (v == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    bool ok = set_make_elements(v, nr_elems, elems);
    if (!ok) {
        purc_variant_unref(v);
        return PURC_VARIANT_INVALID;
    }

    return v;
}

typedef void (*traverse_cb)(struct pcedom_element *element, void *ud);

struct visit_args {
    purc_variant_t            elements;
    const char               *css;
};

static inline int
match_by_class(struct pcedom_element *element, struct visit_args *args)
{
    const unsigned char *s;
    size_t len;
    s = pcedom_element_class(element, &len);

    if (s && s[len]=='\0' && strcmp((const char*)s, args->css+1)==0)
        return 0;

    return -1;
}

static inline int
match_by_id(struct pcedom_element *element, struct visit_args *args)
{
    const unsigned char *s;
    size_t len;
    s = pcedom_element_id(element, &len);

    if (s && s[len]=='\0' && strcmp((const char*)s, args->css+1)==0)
        return 0;

    return -1;
}

static void visit_element(struct pcedom_element *element, void *ud)
{
    struct visit_args *args = (struct visit_args*)ud;

    if (args->css[0] == '.') {
        if (match_by_class(element, args))
            return;
    }
    else if (args->css[0] == '#') {
        if (match_by_id(element, args))
            return;
    }

    set_add_element(args->elements, element);
}

static inline void
traverse_elements(struct pcedom_element *root, traverse_cb cb, void *ud)
{
    if (!root)
        return;

    cb(root, ud);

    pcedom_node_t *node = &root->node;

    pcedom_node_t *child = node->first_child;
    for (; child; child = child->next) {
        struct pcedom_element *element;
        element = container_of(child, struct pcedom_element, node);
        cb(element, ud);
    }
}

purc_variant_t
pcintr_query_elements(struct pcedom_element *root, const char *css)
{
    if (strcmp(css, "*") != 0) {
        if (css[0] != '.' && css[0] != '#') {
            pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
            return PURC_VARIANT_INVALID;
        }
        if (css[1] == '\0') {
            pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
            return PURC_VARIANT_INVALID;
        }
    }

    struct visit_args args;
    args.elements = purc_variant_make_set_by_ckey(0,
            NULL, PURC_VARIANT_INVALID);
    args.css      = css;

    if (args.elements == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    traverse_elements(root, visit_element, &args);

    return args.elements;
}

