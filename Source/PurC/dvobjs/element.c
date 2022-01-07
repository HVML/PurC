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

#include "purc-edom.h"

static inline bool
element_eraser(struct pcintr_element *element)
{
    element->elem = NULL;
    free(element);
    return true;
}

static inline purc_variant_t
element_attr_getter_by_name(pcedom_element_t *element,
        purc_variant_t an)
{
    const char *name = purc_variant_get_string_const(an);
    int r;
    const char *val;
    size_t len;
    r = pcedom_element_attr(element, name,
            (const unsigned char**)&val, &len);

    if (r) {
        return PURC_VARIANT_INVALID;
    }

    PC_ASSERT(val && val[len]=='\0');

    // FIXME: strdup???
    return purc_variant_make_string_static(val, true);
}

static inline purc_variant_t
attr_getter(void* native_entity, size_t nr_args, purc_variant_t* argv)
{
    PC_ASSERT(native_entity);

    struct pcintr_element *element;
    element = (struct pcintr_element*)native_entity;
    PC_ASSERT(element && element->elem);

    if (nr_args == 1) {
        if (argv == NULL || argv[0] == PURC_VARIANT_INVALID) {
            pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
            return PURC_VARIANT_INVALID;
        }
        purc_variant_t an = argv[0]; // attribute name
        if (!purc_variant_is_string(an)) {
            pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
            return PURC_VARIANT_INVALID;
        }
        return element_attr_getter_by_name(element->elem, an);
    }

    pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
    return PURC_VARIANT_INVALID;
}

static struct native_property_cfg configs[] = {
    {"attr", attr_getter, NULL, NULL, NULL},
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

    struct pcintr_element *element;
    element = (struct pcintr_element*)native_entity;

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
pcintr_make_element_variant(struct pcedom_element *elem)
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

    struct pcintr_element *element;
    element = (struct pcintr_element*)calloc(1, sizeof(*element));
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

