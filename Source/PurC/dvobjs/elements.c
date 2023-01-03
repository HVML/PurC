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

#include "purc-errors.h"

#include "private/dvobjs.h"
#include "private/stringbuilder.h"

#include "internal.h"

static int
elements_init(struct pcdvobjs_elements *elements)
{
    PC_ASSERT(elements);

    elements->elements = pcutils_array_create();
    if (!elements->elements)
        return -1;

    return 0;
}


static void
elements_release(struct pcdvobjs_elements *elements)
{
    if (!elements) {
        return;
    }

    if (elements->elements) {
        pcutils_array_destroy(elements->elements, true);
        elements->elements = NULL;
    }
    if (elements->css) {
        free(elements->css);
        elements->css = NULL;
    }
}

static void
elements_destroy(struct pcdvobjs_elements *elements)
{
    if (elements) {
        elements_release(elements);
        free(elements);
    }
}

static purc_variant_t
count_getter(void *entity,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    PC_ASSERT(entity);
    struct pcdvobjs_elements *elements= (struct pcdvobjs_elements*)entity;
    pcutils_array_t *arr = elements->elements;
    PC_ASSERT(arr);
    size_t len = pcutils_array_length(arr);
    return purc_variant_make_ulongint(len);
}

static purc_variant_t
at_getter(void *entity,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(call_flags);
    PC_ASSERT(entity);
    if (nr_args == 0 || argv[0] == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    bool ok;
    uint64_t uidx;
    bool force = true;

    ok = purc_variant_cast_to_ulongint(argv[0], &uidx, force);
    if (!ok) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    struct pcdvobjs_elements *elements= (struct pcdvobjs_elements*)entity;
    pcutils_array_t *arr = elements->elements;
    PC_ASSERT(arr);
    size_t len = pcutils_array_length(arr);
    if (uidx >= len) {
        purc_set_error(PURC_ERROR_OVERFLOW);
        return PURC_VARIANT_INVALID;
    }

    pcdoc_element_t elem;
    elem = (pcdoc_element_t)pcutils_array_get(elements->elements, uidx);
    PC_ASSERT(elem);

    return pcdvobjs_make_elements(elements->doc, elem);
}

static purc_variant_t
attr_getter(void* native_entity, size_t nr_args, purc_variant_t* argv,
        unsigned call_flags)
{
    PC_ASSERT(native_entity);
    UNUSED_PARAM(call_flags);

    struct pcdvobjs_elements *elements;
    elements = (struct pcdvobjs_elements*)native_entity;
    PC_ASSERT(elements && elements->elements);

    size_t nr = pcutils_array_length(elements->elements);
    if (nr == 0)
        return PURC_VARIANT_INVALID;

    pcdoc_element_t elem;
    elem = (pcdoc_element_t)pcutils_array_get(elements->elements, 0);
    PC_ASSERT(elem);

    return pcdvobjs_element_attr_getter(elements->doc, elem,
            nr_args, argv, (call_flags & PCVRT_CALL_FLAG_SILENTLY));
}

static purc_variant_t
content_getter(void* native_entity, size_t nr_args, purc_variant_t* argv,
        unsigned call_flags)
{
    UNUSED_PARAM(call_flags);
    PC_ASSERT(native_entity);

    struct pcdvobjs_elements *elements;
    elements = (struct pcdvobjs_elements*)native_entity;
    PC_ASSERT(elements && elements->elements);

    size_t nr = pcutils_array_length(elements->elements);
    if (nr == 0)
        return PURC_VARIANT_INVALID;

    pcdoc_element_t elem;
    elem = (pcdoc_element_t)pcutils_array_get(elements->elements, 0);
    PC_ASSERT(elem);

    return pcdvobjs_element_content_getter(elements->doc, elem,
            nr_args, argv, (call_flags & PCVRT_CALL_FLAG_SILENTLY));
}

static purc_variant_t
text_content_getter(void* native_entity, size_t nr_args, purc_variant_t* argv,
        unsigned call_flags)
{
    UNUSED_PARAM(call_flags);
    PC_ASSERT(native_entity);

    struct pcdvobjs_elements *elements;
    elements = (struct pcdvobjs_elements*)native_entity;
    PC_ASSERT(elements && elements->elements);

    size_t nr = pcutils_array_length(elements->elements);
    if (nr == 0)
        return PURC_VARIANT_INVALID;

    pcdoc_element_t elem;
    elem = (pcdoc_element_t)pcutils_array_get(elements->elements, 0);
    PC_ASSERT(elem);

    return pcdvobjs_element_text_content_getter(elements->doc, elem,
            nr_args, argv, (call_flags & PCVRT_CALL_FLAG_SILENTLY));
}

static purc_variant_t
json_content_getter(void* native_entity, size_t nr_args, purc_variant_t* argv,
        unsigned call_flags)
{
    UNUSED_PARAM(call_flags);
    PC_ASSERT(native_entity);

    struct pcdvobjs_elements *elements;
    elements = (struct pcdvobjs_elements*)native_entity;
    PC_ASSERT(elements && elements->elements);

    size_t nr = pcutils_array_length(elements->elements);
    if (nr == 0)
        return PURC_VARIANT_INVALID;

    pcdoc_element_t elem;
    elem = (pcdoc_element_t)pcutils_array_get(elements->elements, 0);
    PC_ASSERT(elem);

    return pcdvobjs_element_json_content_getter(elements->doc, elem,
            nr_args, argv, (call_flags & PCVRT_CALL_FLAG_SILENTLY));
}

static purc_variant_t
has_class_getter(void* native_entity, size_t nr_args, purc_variant_t* argv,
        unsigned call_flags)
{
    UNUSED_PARAM(call_flags);
    PC_ASSERT(native_entity);

    struct pcdvobjs_elements *elements;
    elements = (struct pcdvobjs_elements*)native_entity;
    PC_ASSERT(elements && elements->elements);

    size_t nr = pcutils_array_length(elements->elements);
    if (nr == 0)
        return purc_variant_make_boolean(false);

    for (size_t i=0; i<nr; ++i) {
        pcdoc_element_t elem;
        elem = (pcdoc_element_t)pcutils_array_get(elements->elements, i);
        PC_ASSERT(elem);

        purc_variant_t v = pcdvobjs_element_has_class_getter(elements->doc,
                elem, nr_args, argv, (call_flags & PCVRT_CALL_FLAG_SILENTLY));
        if (v == PURC_VARIANT_INVALID)
            continue;
        PC_ASSERT(purc_variant_is_boolean(v));
        bool has = purc_variant_booleanize(v);
        if (has)
            return v;
        purc_variant_unref(v);
    }

    return purc_variant_make_boolean(false);
}

#define IS_ELEMENTS         "is_elements"

static purc_variant_t
is_elements(void* native_entity, size_t nr_args, purc_variant_t* argv,
        unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    return purc_variant_make_boolean(true);
}

static struct native_property_cfg configs[] = {
    {"count", count_getter, NULL, NULL, NULL},
    {"at", at_getter, NULL, NULL, NULL},
    {"attr", attr_getter, NULL, NULL, NULL},
    // VW {"prop", prop_getter, NULL, NULL, NULL},
    // VW {"style", style_getter, NULL, NULL, NULL},
    {"content", content_getter, NULL, NULL, NULL},
    {"text_content", text_content_getter, NULL, NULL, NULL},
    {"json_content", json_content_getter, NULL, NULL, NULL},
    // VW {"val", val_getter, NULL, NULL, NULL},
    {"has_class", has_class_getter, NULL, NULL, NULL},
    {IS_ELEMENTS, is_elements, NULL, NULL, NULL},
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
property_getter(void* entity, const char* key_name)
{
    UNUSED_PARAM(entity);
    PC_ASSERT(key_name);
    struct native_property_cfg *cfg = property_cfg_by_name(key_name);
    if (cfg)
        return cfg->property_getter;
    return NULL;
}

// query the setter for a specific property.
static purc_nvariant_method
property_setter(void* entity, const char* key_name)
{
    UNUSED_PARAM(entity);
    PC_ASSERT(key_name);
    struct native_property_cfg *cfg = property_cfg_by_name(key_name);
    if (cfg)
        return cfg->property_setter;
    return NULL;
}

// query the eraser for a specific property.
static purc_nvariant_method
property_eraser(void* entity, const char* key_name)
{
    UNUSED_PARAM(entity);
    PC_ASSERT(key_name);
    struct native_property_cfg *cfg = property_cfg_by_name(key_name);
    if (cfg)
        return cfg->property_eraser;
    return NULL;
}

// query the cleaner for a specific property.
static purc_nvariant_method
property_cleaner(void* entity, const char* key_name)
{
    UNUSED_PARAM(entity);
    PC_ASSERT(key_name);
    struct native_property_cfg *cfg = property_cfg_by_name(key_name);
    if (cfg)
        return cfg->property_cleaner;
    return NULL;
}

static purc_variant_t
cleaner(void *native_entity, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(call_flags);
    struct pcdvobjs_elements *elements;
    elements = (struct pcdvobjs_elements*)native_entity;
    pcutils_array_t *arr = elements->elements;
    PC_ASSERT(arr);

    pcdoc_element_t elem = NULL;
    size_t len = pcutils_array_length(arr);
    for (size_t i = 0; i < len; i++) {
        elem = (pcdoc_element_t)pcutils_array_get(elements->elements, i);
        if (!elem) {
            continue;
        }

        pcdoc_element_clear(elements->doc, elem);
#if 0 // VW
        pcdom_node_t *node = pcdom_interface_node(elem);
        while(node->first_child) {
            pcdom_node_t *child = node->first_child;
            pcdom_node_remove(child);
        }
#endif
    }

    return purc_variant_make_boolean(true);
}

static purc_variant_t
eraser(void* native_entity, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(call_flags);
    struct pcdvobjs_elements *elements;
    elements = (struct pcdvobjs_elements*)native_entity;
    pcutils_array_t *arr = elements->elements;
    PC_ASSERT(arr);

    pcdoc_element_t elem = NULL;
    size_t len = pcutils_array_length(arr);
    size_t nr_erase = 0;
    for (size_t i = 0; i < len; i++) {
        elem = (pcdoc_element_t)pcutils_array_get(elements->elements, i);
        pcdoc_element_erase(elements->doc, elem);
        nr_erase++;
    }

    return purc_variant_make_ulongint(nr_erase);
}

static bool
did_matched(void* native_entity, purc_variant_t val)
{
    bool ret = false;
    void *comp = NULL;
    struct pcdvobjs_elements *elements = (struct pcdvobjs_elements*)native_entity;

    if (!purc_variant_is_native(val) && !purc_variant_is_string(val)) {
        goto out;
    }

    if (purc_variant_is_string(val)) {
        const char *s = purc_variant_get_string_const(val);
        if (elements->css && strcmp(elements->css, s) == 0) {
            ret = true;
            goto out;
        }
        else if (s[0] == '#'){
            purc_variant_t v = pcdvobjs_elements_by_css(elements->doc, s);
            if (v) {
                comp =  pcdvobjs_get_element_from_elements(v, 0);
                purc_variant_unref(v);
            }
        }
    }
    else if (purc_variant_is_native(val)) {
        comp = purc_variant_native_get_entity(val);
    }

    if (comp) {
        purc_variant_t v = pcdvobjs_elements_by_css(elements->doc, elements->css);
        void *entity = purc_variant_native_get_entity(v);
        struct pcdvobjs_elements *elems = (struct pcdvobjs_elements*)entity;

        pcutils_array_t *arr = elems->elements;
        PC_ASSERT(arr);

        pcdoc_element_t elem = NULL;
        size_t len = pcutils_array_length(arr);
        for (size_t i = 0; i < len; i++) {
            elem = (pcdoc_element_t)pcutils_array_get(elems->elements, i);
            if (elem == comp) {
                ret = true;
                break;
            }
        }
        purc_variant_unref(v);
    }

out:
    return ret;
}

static bool
on_observe(void* native_entity,
        const char *event_name, const char *event_subname)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(event_name);
    UNUSED_PARAM(event_subname);
    return true;
}


// the callback to release the native entity.
static void
on_release(void* native_entity)
{
    UNUSED_PARAM(native_entity);

    PC_ASSERT(native_entity);
    struct pcdvobjs_elements *elements;
    elements = (struct pcdvobjs_elements*)native_entity;
    elements_destroy(elements);
}

static purc_variant_t
make_elements(void)
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

        .on_observe                = on_observe,
        .on_release                = on_release,
    };

    struct pcdvobjs_elements *elements;
    elements = (struct pcdvobjs_elements*)calloc(1, sizeof(*elements));
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
add_element(struct pcdvobjs_elements *elements, pcdoc_element_t elem)
{
    PC_ASSERT(elements);
    PC_ASSERT(elements->elements);
    PC_ASSERT(elem);
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

struct visit_args {
    struct pcdvobjs_elements *elements;
    const char               *css;
};

static bool
match_by_class(purc_document_t doc, pcdoc_element_t element,
        struct visit_args *args)
{
    bool found = false;
    pcdoc_element_has_class(doc, element, args->css + 1, &found);
    return found;
}

static bool
match_by_id(purc_document_t doc, pcdoc_element_t element,
        struct visit_args *args)
{
    const char *s;
    size_t len;
    s = pcdoc_element_id(doc, element, &len);

    if (s && s[len]=='\0' &&
            strncmp(s, args->css + 1, len)==0 &&
            args->css[1+len]=='\0')
    {
        return true;
    }

    return false;
}

static int
visit_element(purc_document_t doc, pcdoc_element_t element, void *ud)
{
    struct visit_args *args = (struct visit_args*)ud;

    if (args->css[0] == '.') {
        if (!match_by_class(doc, element, args))
            return 0;
    }
    else if (args->css[0] == '#') {
        if (!match_by_id(doc, element, args))
            return 0;
    }

    if (!add_element(args->elements, element))
        return -1;

    return 0;
}

purc_variant_t
pcdvobjs_query_elements(purc_document_t doc, pcdoc_element_t root,
        const char *css)
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

    purc_variant_t elements = make_elements();
    if (elements == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    PC_ASSERT(purc_variant_is_type(elements, PURC_VARIANT_TYPE_NATIVE));
    void *entity = purc_variant_native_get_entity(elements);
    PC_ASSERT(entity);

    struct pcdvobjs_elements* elems = (struct pcdvobjs_elements*)entity;
    elems->doc = doc;
    elems->css = strdup(css);
    if (elems->css == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        purc_variant_unref(elements);
        return PURC_VARIANT_INVALID;
    }

    struct visit_args args;
    args.elements = (struct pcdvobjs_elements*)entity;
    args.css      = css;

    int r = pcdoc_travel_descendant_elements(doc, root,
            visit_element, &args, NULL);
    if (r) {
        purc_variant_unref(elements);
        return PURC_VARIANT_INVALID;
    }

    return elements;
}

bool
pcdvobjs_is_elements(purc_variant_t v)
{
    if (!purc_variant_is_native(v)) {
        return false;
    }
    void *entity = purc_variant_native_get_entity(v);
    struct purc_native_ops *ops = purc_variant_native_get_ops(v);
    purc_nvariant_method m = ops->property_getter(entity, IS_ELEMENTS);
    if (m) {
        return true;
    }
    return false;
}

purc_variant_t
pcdvobjs_make_elements(purc_document_t doc, pcdoc_element_t element)
{
    PC_ASSERT(element);

    purc_variant_t elements = make_elements();
    if (elements == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    PC_ASSERT(purc_variant_is_type(elements, PURC_VARIANT_TYPE_NATIVE));
    void *entity = purc_variant_native_get_entity(elements);
    PC_ASSERT(entity);

    struct pcdvobjs_elements *elems;
    elems = (struct pcdvobjs_elements*)entity;
    elems->doc = doc;

    if (!add_element(elems, element)) {
        purc_variant_unref(elements);
        return PURC_VARIANT_INVALID;
    }

    return elements;
}

purc_variant_t
pcdvobjs_elements_by_css(purc_document_t doc, const char *css)
{
    return pcdvobjs_query_elements(doc, NULL, css);
}

pcdoc_element_t
pcdvobjs_get_element_from_elements(purc_variant_t elems, size_t idx)
{
    PC_ASSERT(elems);

    PC_ASSERT(purc_variant_is_type(elems, PURC_VARIANT_TYPE_NATIVE));
    void *entity = purc_variant_native_get_entity(elems);
    PC_ASSERT(entity);

    struct pcdvobjs_elements *elements;
    elements = (struct pcdvobjs_elements*)entity;

    pcutils_array_t *arr = elements->elements;
    PC_ASSERT(arr);

    pcdoc_element_t elem;
    elem = (pcdoc_element_t)pcutils_array_get(elements->elements, idx);

    return elem;
}

#if 0 // VW
typedef int (*traverse_cb)(struct pcdom_element *element, void *ud);

static inline int
traverse_elements(struct pcdom_element *root, traverse_cb cb, void *ud)
{
    PC_ASSERT(root);

    int r = cb(root, ud);
    if (r)
        return -1;

    pcdom_node_t *node = &root->node;

    pcdom_node_t *child = node->first_child;
    for (; child; child = child->next) {
        if (child->type != PCDOM_NODE_TYPE_ELEMENT)
            continue;
        struct pcdom_element *element;
        element = container_of(child, struct pcdom_element, node);
        r = traverse_elements(element, cb, ud);
        if (r)
            return -1;
    }

    return 0;
}
#endif

#if 0 // VW: deprecated
static inline purc_variant_t
prop_getter(void* native_entity, size_t nr_args, purc_variant_t* argv,
        unsigned call_flags)
{
    UNUSED_PARAM(call_flags);
    PC_ASSERT(native_entity);

    struct pcdvobjs_elements *elements;
    elements = (struct pcdvobjs_elements*)native_entity;
    PC_ASSERT(elements && elements->elements);

    size_t nr = pcutils_array_length(elements->elements);
    if (nr == 0)
        return PURC_VARIANT_INVALID;

    pcdoc_element_t elem;
    elem = (pcdoc_element_t)pcutils_array_get(elements->elements, 0);
    PC_ASSERT(elem);

    return pcdvobjs_element_prop_getter(elem, nr_args, argv,
            (call_flags & PCVRT_CALL_FLAG_SILENTLY));
}

static inline purc_variant_t
style_getter(void* native_entity, size_t nr_args, purc_variant_t* argv,
        unsigned call_flags)
{
    UNUSED_PARAM(call_flags);
    PC_ASSERT(native_entity);

    struct pcdvobjs_elements *elements;
    elements = (struct pcdvobjs_elements*)native_entity;
    PC_ASSERT(elements && elements->elements);

    size_t nr = pcutils_array_length(elements->elements);
    if (nr == 0)
        return PURC_VARIANT_INVALID;

    pcdoc_element_t elem;
    elem = (pcdoc_element_t)pcutils_array_get(elements->elements, 0);
    PC_ASSERT(elem);

    return pcdvobjs_element_style_getter(elem, nr_args, argv,
            (call_flags & PCVRT_CALL_FLAG_SILENTLY));
}

static inline purc_variant_t
val_getter(void* native_entity, size_t nr_args, purc_variant_t* argv,
        unsigned call_flags)
{
    UNUSED_PARAM(call_flags);
    PC_ASSERT(native_entity);

    struct pcdvobjs_elements *elements;
    elements = (struct pcdvobjs_elements*)native_entity;
    PC_ASSERT(elements && elements->elements);

    size_t nr = pcutils_array_length(elements->elements);
    if (nr == 0)
        return PURC_VARIANT_INVALID;

    pcdoc_element_t elem;
    elem = (pcdoc_element_t)pcutils_array_get(elements->elements, 0);
    PC_ASSERT(elem);

    return pcdvobjs_element_val_getter(elem, nr_args, argv,
            (call_flags & PCVRT_CALL_FLAG_SILENTLY));
}
#endif // VW: deprecated

