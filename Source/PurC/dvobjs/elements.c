/**
 * @file elements.c
 * @author Xu Xiaohong
 * @date 2021/11/18
 * @brief The implementation for elements native variant
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

#include "internal.h"

static int
elements_init(struct pcintr_elements *elements)
{
    PC_ASSERT(elements);

    elements->elements = pcutils_array_create();
    if (!elements->elements)
        return -1;

    return 0;
}


static void
elements_release(struct pcintr_elements *elements)
{
    if (elements && elements->elements) {
        pcutils_array_destroy(elements->elements, true);
        elements->elements = NULL;
    }
}

static void
elements_destroy(struct pcintr_elements *elements)
{
    if (elements) {
        elements_release(elements);
        free(elements);
    }
}

static inline purc_variant_t
count_getter(void *entity,
        size_t nr_args, purc_variant_t * argv)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    PC_ASSERT(entity);
    struct pcintr_elements *elements= (struct pcintr_elements*)entity;
    pcutils_array_t *arr = elements->elements;
    PC_ASSERT(arr);
    size_t len = pcutils_array_length(arr);
    return purc_variant_make_ulongint(len);
}

static struct native_property_cfg configs[] = {
    {"count", count_getter, NULL, NULL, NULL},
};

static struct native_property_cfg*
property_cfg_by_name(const char *key_name)
{
    for (size_t i=0; i<PCA_TABLESIZE(configs); ++i) {
        struct native_property_cfg *cfg = configs + i;
        const char *property_name = cfg->property_name;
        PC_ASSERT(property_name);
        if (strcmp(property_name, key_name) == 0) {
            return cfg;
        }
    }
    return NULL;
}

// query the getter for a specific property.
static purc_nvariant_method
property_getter(const char* key_name)
{
    PC_ASSERT(key_name);
    struct native_property_cfg *cfg = property_cfg_by_name(key_name);
    if (cfg)
        return cfg->property_getter;
    return NULL;
}

// query the setter for a specific property.
static purc_nvariant_method
property_setter(const char* key_name)
{
    PC_ASSERT(key_name);
    struct native_property_cfg *cfg = property_cfg_by_name(key_name);
    if (cfg)
        return cfg->property_setter;
    return NULL;
}

// query the eraser for a specific property.
static purc_nvariant_method
property_eraser(const char* key_name)
{
    PC_ASSERT(key_name);
    struct native_property_cfg *cfg = property_cfg_by_name(key_name);
    if (cfg)
        return cfg->property_eraser;
    return NULL;
}

// query the cleaner for a specific property.
static purc_nvariant_method
property_cleaner(const char* key_name)
{
    PC_ASSERT(key_name);
    struct native_property_cfg *cfg = property_cfg_by_name(key_name);
    if (cfg)
        return cfg->property_cleaner;
    return NULL;
}

// the cleaner to clear the content of the native entity.
static bool
cleaner(void* native_entity)
{
    UNUSED_PARAM(native_entity);
    PC_ASSERT(0); // Not implemented yet
    return false;
}

// the eraser to erase the native entity.
static bool
eraser(void* native_entity)
{
    UNUSED_PARAM(native_entity);
    PC_ASSERT(0); // Not implemented yet
    return false;
}

// the callback when the variant was observed (nullable).
static bool
observe(void* native_entity, ...)
{
    UNUSED_PARAM(native_entity);
    PC_ASSERT(0); // Not implemented yet
    return false;
}

static purc_variant_t
make_elements(void)
{
    static struct purc_native_ops ops = {
        .property_getter            = property_getter,
        .property_setter            = property_setter,
        .property_eraser            = property_eraser,
        .property_cleaner           = property_cleaner,

        .cleaner                    = cleaner,
        .eraser                     = eraser,
        .observe                    = observe,
    };

    struct pcintr_elements *elements;
    elements = (struct pcintr_elements*)calloc(1, sizeof(elements));
    if (!elements) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    if (elements_init(elements)) {
        free(elements);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = purc_variant_make_native(elements, &ops);
    if (v == PURC_VARIANT_INVALID) {
        elements_destroy(elements);
        return PURC_VARIANT_INVALID;
    }

    return v;
}

static inline bool
add_element(struct pcintr_elements *elements, struct pcedom_element *elem)
{
    PC_ASSERT(elements);
    PC_ASSERT(elements->elements);
    pcutils_array_t *arr = elements->elements;
    unsigned int ui;
    ui = pcutils_array_push(arr, elem);
    if (ui) {
        PC_ASSERT(ui == PURC_ERROR_OUT_OF_MEMORY);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }

    return true;
}

typedef int (*traverse_cb)(struct pcedom_element *element, void *ud);

struct visit_args {
    struct pcintr_elements   *elements;
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

static int
visit_element(struct pcedom_element *element, void *ud)
{
    struct visit_args *args = (struct visit_args*)ud;

    if (args->css[0] == '.') {
        if (match_by_class(element, args))
            return 0;
    }
    else if (args->css[0] == '#') {
        if (match_by_id(element, args))
            return 0;
    }

    if (!add_element(args->elements, element))
        return -1;

    return 0;
}

static inline int
traverse_elements(struct pcedom_element *root, traverse_cb cb, void *ud)
{
    PC_ASSERT(root);

    int r = cb(root, ud);
    if (r)
        return -1;

    pcedom_node_t *node = &root->node;

    pcedom_node_t *child = node->first_child;
    for (; child; child = child->next) {
        if (child->type != PCEDOM_NODE_TYPE_ELEMENT)
            continue;
        struct pcedom_element *element;
        element = container_of(child, struct pcedom_element, node);
        r = cb(element, ud);
        if (r)
            return -1;
    }

    return 0;
}

purc_variant_t
pcintr_query_elements(struct pcedom_element *root, const char *css)
{
    PC_ASSERT(root);

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

    purc_variant_t elements = make_elements();
    if (elements == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    PC_ASSERT(purc_variant_is_type(elements, PURC_VARIANT_TYPE_NATIVE));
    void *entity = purc_variant_native_get_entity(elements);
    PC_ASSERT(entity);

    struct visit_args args;
    args.elements = entity;
    args.css      = css;

    int r = traverse_elements(root, visit_element, &args);
    if (r) {
        purc_variant_unref(elements);
        return PURC_VARIANT_INVALID;
    }

    return elements;
}

