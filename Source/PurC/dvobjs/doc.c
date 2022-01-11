/*
 * @file doc.c
 * @author Xu Xiaohong
 * @date 2021/11/17
 * @brief The implementation for DOC native variant
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

#include "internal.h"

#include "private/debug.h"
#include "private/errors.h"
#include "private/dom.h"
#include "private/avl.h"

#include "purc-variant.h"

struct dynamic_args {
    const char              *name;
    purc_dvariant_method     getter;
    purc_dvariant_method     setter;
};

static inline bool
set_object_by(purc_variant_t obj, struct dynamic_args *arg)
{
    purc_variant_t dynamic;
    dynamic = purc_variant_make_dynamic(arg->getter, arg->setter);
    if (dynamic == PURC_VARIANT_INVALID)
        return false;

    bool ok = purc_variant_object_set_by_static_ckey(obj, arg->name, dynamic);
    if (ok)
        return true;

    purc_variant_unref(dynamic);
    return false;
}

static inline purc_variant_t
make_object(size_t nr_args, struct dynamic_args *args)
{
    purc_variant_t obj;
    obj = purc_variant_make_object_by_static_ckey(0,
            NULL, PURC_VARIANT_INVALID);

    if (obj == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    for (size_t i=0; i<nr_args; ++i) {
        struct dynamic_args *arg = args + i;
        if (!set_object_by(obj, arg)) {
            purc_variant_unref(obj);
            return false;
        }
    }

    return obj;
}

static inline purc_variant_t
doctype_default(struct pcdom_document *doc)
{
    const char *s = "html";

    pcdom_document_type_t *doc_type;
    doc_type = doc->doctype;
    if (doc_type) {
        const unsigned char *doctype;
        size_t len;
        doctype = pcdom_document_type_name(doc_type, &len);
        PC_ASSERT(doctype);
        PC_ASSERT(doctype[len]=='\0');
        s = (const char*)doctype;
    }

    // NOTE: we don't hold ownership
    return purc_variant_make_string_static(s, false);
}

static inline purc_variant_t
doctype_system(struct pcdom_document *doc)
{
    const char *s = "";

    pcdom_document_type_t *doc_type;
    doc_type = doc->doctype;
    if (doc_type) {
        const unsigned char *s_system;
        size_t len;
        s_system = pcdom_document_type_system_id(doc_type, &len);
        PC_ASSERT(s_system);
        PC_ASSERT(s_system[len]=='\0');
        s = (const char*)s_system;
    }

    // NOTE: we don't hold ownership
    return purc_variant_make_string_static(s, false);
}

static inline purc_variant_t
doctype_public(struct pcdom_document *doc)
{
    const char *s = "";

    pcdom_document_type_t *doc_type;
    doc_type = doc->doctype;
    if (doc_type) {
        const unsigned char *s_public;
        size_t len;
        s_public = pcdom_document_type_public_id(doc_type, &len);
        PC_ASSERT(s_public);
        PC_ASSERT(s_public[len]=='\0');
        s = (const char*)s_public;
    }

    // NOTE: we don't hold ownership
    return purc_variant_make_string_static(s, false);
}

static inline purc_variant_t
doctype_getter(void *entity,
        size_t nr_args, purc_variant_t * argv)
{
    PC_ASSERT(entity);
    struct pcdom_document *doc = (struct pcdom_document*)entity;

    if (nr_args == 0) {
        return purc_variant_make_string_static("html", false);
    }

    if (argv == NULL || argv[0] == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    purc_variant_t v = argv[0];
    if (!purc_variant_is_string(v)) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    const char *s = purc_variant_get_string_const(v);
    PC_ASSERT(s);
    if (strcmp(s, "system") == 0) {
        return doctype_system(doc);
    }
    if (strcmp(s, "public") == 0) {
        return doctype_public(doc);
    }

    pcinst_set_error(PURC_ERROR_NOT_EXISTS);
    return PURC_VARIANT_INVALID;
}

static inline purc_variant_t
query(struct pcdom_document *doc, const char *css)
{
    PC_ASSERT(doc);
    PC_ASSERT(css);

    pcdom_element_t *root;
    root = doc->element;
    PC_ASSERT(root);

    return pcdvobjs_query_elements(root, css);
}

static inline purc_variant_t
query_getter(void *entity,
        size_t nr_args, purc_variant_t * argv)
{
    PC_ASSERT(entity);
    struct pcdom_document *doc = (struct pcdom_document*)entity;

    if (nr_args > 0) {
        if (argv == NULL || argv[0] == PURC_VARIANT_INVALID) {
            pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
            return PURC_VARIANT_INVALID;
        }
        purc_variant_t v = argv[0];
        if (!purc_variant_is_string(v)) {
            pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
            return PURC_VARIANT_INVALID;
        }
        const char *css = purc_variant_get_string_const(v);
        PC_ASSERT(css);
        return query(doc, css);
    }

    pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
    return PURC_VARIANT_INVALID;
}

static struct native_property_cfg configs[] = {
    {"doctype", doctype_getter, NULL, NULL, NULL},
    {"query", query_getter, NULL, NULL, NULL},
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

purc_variant_t
pcdvobjs_make_doc_variant(struct pcdom_document *doc)
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

    PC_ASSERT(doc);

    return purc_variant_make_native(doc, &ops);
}

