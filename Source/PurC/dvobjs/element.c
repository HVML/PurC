/**
 * @file element.c
 * @author Xu Xiaohong
 * @date 2021/11/18
 * @brief The implementation for element native variant
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

#include "purc-dom.h"

static inline bool
element_eraser(struct pcdvobjs_element *element)
{
    element->elem = NULL;
    free(element);
    return true;
}

purc_variant_t
pcdvobjs_element_prop_getter(pcdom_element_t *element,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(element);
    UNUSED_PARAM(silently);

    if (nr_args < 1) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (argv == NULL || argv[0] == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t pn = argv[0]; // property name
    if (!purc_variant_is_string(pn)) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    PC_ASSERT(0); // Not implemented yet
    return PURC_VARIANT_INVALID;
}

static inline purc_variant_t
prop_getter(void* native_entity, size_t nr_args, purc_variant_t* argv,
        bool silently)
{
    UNUSED_PARAM(silently);
    PC_ASSERT(native_entity);

    struct pcdvobjs_element *element;
    element = (struct pcdvobjs_element*)native_entity;
    PC_ASSERT(element && element->elem);

    return pcdvobjs_element_prop_getter(element->elem, nr_args, argv, silently);
}

purc_variant_t
pcdvobjs_element_attr_getter(pcdom_element_t *element,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(silently);
    if (nr_args < 1) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (argv == NULL || argv[0] == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t an = argv[0]; // attribute name
    if (!purc_variant_is_string(an)) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    const char *name = purc_variant_get_string_const(an);
    int r;
    const char *val;
    size_t len;
    r = pcdom_element_attr(element, name,
            (const unsigned char**)&val, &len);

    if (r) {
        return PURC_VARIANT_INVALID;
    }

    PC_ASSERT(val && val[len]=='\0');

    // FIXME: strdup???
    return purc_variant_make_string_static(val, true);
}

static inline purc_variant_t
attr_getter(void* native_entity, size_t nr_args, purc_variant_t* argv,
        bool silently)
{
    UNUSED_PARAM(silently);
    PC_ASSERT(native_entity);

    struct pcdvobjs_element *element;
    element = (struct pcdvobjs_element*)native_entity;
    PC_ASSERT(element && element->elem);

    return pcdvobjs_element_attr_getter(element->elem, nr_args, argv,
            silently);
}

purc_variant_t
pcdvobjs_element_style_getter(pcdom_element_t *element,
        size_t nr_args, purc_variant_t* argv, bool silently)
{
    UNUSED_PARAM(silently);
    if (nr_args < 1) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (argv == NULL || argv[0] == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t sn = argv[0]; // style name
    if (!purc_variant_is_string(sn)) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    const char *name = purc_variant_get_string_const(sn);
    int r;
    const char *style;
    size_t len;
    r = pcdom_element_style(element, name,
            (const unsigned char**)&style, &len);

    if (r) {
        return PURC_VARIANT_INVALID;
    }

    PC_ASSERT(style && style[len]=='\0');

    // FIXME: strdup???
    return purc_variant_make_string_static(style, true);
}

static inline purc_variant_t
style_getter(void* native_entity, size_t nr_args, purc_variant_t* argv,
        bool silently)
{
    UNUSED_PARAM(silently);
    PC_ASSERT(native_entity);

    struct pcdvobjs_element *element;
    element = (struct pcdvobjs_element*)native_entity;
    PC_ASSERT(element && element->elem);

    return pcdvobjs_element_style_getter(element->elem, nr_args, argv,
            silently);
}

purc_variant_t
pcdvobjs_element_content_getter(pcdom_element_t *element,
        size_t nr_args, purc_variant_t* argv, bool silently)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    int r;
    const char *content;
    size_t len;
    r = pcdom_element_content(element,
            (const unsigned char**)&content, &len);

    if (r)
        return PURC_VARIANT_INVALID;

    // FIXME: strdup???
    return purc_variant_make_string_static(content, true);
}

static inline purc_variant_t
content_getter(void* native_entity, size_t nr_args, purc_variant_t* argv,
        bool silently)
{
    UNUSED_PARAM(silently);
    PC_ASSERT(native_entity);

    struct pcdvobjs_element *element;
    element = (struct pcdvobjs_element*)native_entity;
    PC_ASSERT(element && element->elem);

    return pcdvobjs_element_content_getter(element->elem, nr_args, argv,
            silently);
}

purc_variant_t
pcdvobjs_element_json_content_getter(pcdom_element_t *element,
        size_t nr_args, purc_variant_t* argv, bool silently)
{
    UNUSED_PARAM(element);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);
    PC_ASSERT(0); // Not implemented yet
    return PURC_VARIANT_INVALID;
}

static inline purc_variant_t
json_content_getter(void* native_entity, size_t nr_args, purc_variant_t* argv,
        bool silently)
{
    UNUSED_PARAM(silently);
    PC_ASSERT(native_entity);

    struct pcdvobjs_element *element;
    element = (struct pcdvobjs_element*)native_entity;
    PC_ASSERT(element && element->elem);

    return pcdvobjs_element_json_content_getter(element->elem, nr_args, argv,
            silently);
}

purc_variant_t
pcdvobjs_element_text_content_getter(pcdom_element_t *element,
        size_t nr_args, purc_variant_t* argv, bool silently)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    int r;
    char *text;
    size_t len;
    r = pcdom_element_text_content(element, &text, &len);

    if (r)
        return PURC_VARIANT_INVALID;

    return purc_variant_make_string_reuse_buff(text, len, true);
}

static inline purc_variant_t
text_content_getter(void* native_entity, size_t nr_args, purc_variant_t* argv,
        bool silently)
{
    UNUSED_PARAM(silently);
    PC_ASSERT(native_entity);

    struct pcdvobjs_element *element;
    element = (struct pcdvobjs_element*)native_entity;
    PC_ASSERT(element && element->elem);

    return pcdvobjs_element_text_content_getter(element->elem, nr_args, argv,
            silently);
}

purc_variant_t
pcdvobjs_element_val_getter(pcdom_element_t *element,
        size_t nr_args, purc_variant_t* argv, bool silently)
{
    UNUSED_PARAM(element);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);
    PC_ASSERT(0); // Not implemented yet
    return PURC_VARIANT_INVALID;
}

static inline purc_variant_t
val_getter(void* native_entity, size_t nr_args, purc_variant_t* argv,
        bool silently)
{
    UNUSED_PARAM(silently);
    PC_ASSERT(native_entity);

    struct pcdvobjs_element *element;
    element = (struct pcdvobjs_element*)native_entity;
    PC_ASSERT(element && element->elem);

    return pcdvobjs_element_val_getter(element->elem, nr_args, argv, silently);
}

purc_variant_t
pcdvobjs_element_has_class_getter(pcdom_element_t *element,
        size_t nr_args, purc_variant_t* argv, bool silently)
{
    UNUSED_PARAM(silently);
    if (nr_args < 1) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (argv == NULL || argv[0] == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    purc_variant_t cn = argv[0]; // class name
    if (!purc_variant_is_string(cn)) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    const char *name = purc_variant_get_string_const(cn);
    int r;
    bool has;
    r = pcdom_element_has_class(element, name, &has);
    if (r)
        return PURC_VARIANT_INVALID;

    return purc_variant_make_boolean(has);
}

static inline purc_variant_t
has_class_getter(void* native_entity, size_t nr_args, purc_variant_t* argv,
        bool silently)
{
    UNUSED_PARAM(silently);
    PC_ASSERT(native_entity);

    struct pcdvobjs_element *element;
    element = (struct pcdvobjs_element*)native_entity;
    PC_ASSERT(element && element->elem);

    return pcdvobjs_element_has_class_getter(element->elem, nr_args, argv,
            silently);
}

static struct native_property_cfg configs[] = {
    {"attr", attr_getter, NULL, NULL, NULL},
    {"prop", prop_getter, NULL, NULL, NULL},
    {"style", style_getter, NULL, NULL, NULL},
    {"content", content_getter, NULL, NULL, NULL},
    {"text_content", text_content_getter, NULL, NULL, NULL},
    {"json_content", json_content_getter, NULL, NULL, NULL},
    {"val", val_getter, NULL, NULL, NULL},
    {"has_class", has_class_getter, NULL, NULL, NULL},
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
    PC_ASSERT(native_entity);

    struct pcdvobjs_element *element;
    element = (struct pcdvobjs_element*)native_entity;

    return element_eraser(element);
}

// the callback when the variant was observed (nullable).
static bool
observe(void* native_entity, ...)
{
    UNUSED_PARAM(native_entity);
    PC_ASSERT(0); // Not implemented yet
    return false;
}

purc_variant_t
pcdvobjs_make_element_variant(struct pcdom_element *elem)
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

    struct pcdvobjs_element *element;
    element = (struct pcdvobjs_element*)calloc(1, sizeof(*element));
    if (!element) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = purc_variant_make_native(element, &ops);
    if (v == PURC_VARIANT_INVALID) {
        free(element);
        return PURC_VARIANT_INVALID;
    }

    element->elem = elem;

    return v;
}

struct pcdom_element*
pcdvobjs_get_element_from_variant(purc_variant_t val)
{
    PC_ASSERT(val && purc_variant_is_native(val));
    struct pcdvobjs_element *element;
    element = (struct pcdvobjs_element*)purc_variant_native_get_entity(val);
    PC_ASSERT(element);
    PC_ASSERT(element->elem);
    return element->elem;
}

