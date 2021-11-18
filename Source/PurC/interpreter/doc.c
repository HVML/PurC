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

#include "private/debug.h"
#include "private/errors.h"
#include "private/edom.h"
#include "private/avl.h"

#include "purc-variant.h"


static inline bool
doc_eraser(void *native_entity)
{
    PC_ASSERT(native_entity);

    struct pcedom_document *edom_doc;
    edom_doc = (struct pcedom_document*)native_entity;

    pcedom_document_interface_destroy(edom_doc);

    return true;
}

static inline purc_variant_t
make_doc_variant(struct pcedom_document *edom_doc)
{
    static struct purc_native_ops ops = {
        .property_getter          = NULL,
        .property_setter          = NULL,
        .property_eraser          = NULL,
        .property_cleaner         = NULL,
        .cleaner                  = NULL,
        .eraser                   = doc_eraser,
        .observe                  = NULL,
    };

    purc_variant_t v;
    v = purc_variant_make_native(edom_doc, &ops);
    if (v == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    return v;
}

static inline purc_variant_t
doc_get_doctype(purc_variant_t doc)
{
    PC_ASSERT(doc != PURC_VARIANT_INVALID);

    void *native_entity = purc_variant_native_get_entity(doc);
    PC_ASSERT(native_entity);
    struct pcedom_document *edom_doc;
    edom_doc = (struct pcedom_document*)native_entity;

    // TODO: get doctype from edom_doc
    (void)edom_doc;

    const char *s = "doctype:not_implemented_yet";

    purc_variant_t v;
    v = purc_variant_make_string_static(s, false);

    return v;
}

static inline purc_variant_t
doc_get_doctype_system(purc_variant_t doc)
{
    PC_ASSERT(doc != PURC_VARIANT_INVALID);

    void *native_entity = purc_variant_native_get_entity(doc);
    PC_ASSERT(native_entity);
    struct pcedom_document *edom_doc;
    edom_doc = (struct pcedom_document*)native_entity;

    // TODO: get doctype.system from edom_doc
    (void)edom_doc;

    const char *s = "doctype.system:not_implemented yet";
    purc_variant_t v;
    v = purc_variant_make_string_static(s, false);

    return v;
}

static inline purc_variant_t
doc_get_doctype_public(purc_variant_t doc)
{
    PC_ASSERT(doc != PURC_VARIANT_INVALID);

    void *native_entity = purc_variant_native_get_entity(doc);
    PC_ASSERT(native_entity);
    struct pcedom_document *edom_doc;
    edom_doc = (struct pcedom_document*)native_entity;

    // TODO: get doctype.public from edom_doc
    (void)edom_doc;

    const char *s = "doctype.public:not_implemented yet";
    purc_variant_t v;
    v = purc_variant_make_string_static(s, false);

    return v;
}

static inline purc_variant_t
doc_get_doctype_by_sub(purc_variant_t doc, purc_variant_t sub)
{
    PC_ASSERT(sub != PURC_VARIANT_INVALID);

    const char *s = purc_variant_get_string_const(sub);
    if (strcmp(s, "system") == 0) {
        return doc_get_doctype_system(doc);
    }

    if (strcmp(s, "public") == 0) {
        return doc_get_doctype_public(doc);
    }

    pcinst_set_error(PURC_ERROR_NOT_EXISTS);
    return PURC_VARIANT_INVALID;
}

static inline purc_variant_t
do_doctype_getter(purc_variant_t doc, size_t nr_args, purc_variant_t* argv)
{
    PC_ASSERT(doc != PURC_VARIANT_INVALID);

    if (nr_args == 0) {
        if (argv != NULL) {
            pcinst_set_error(PURC_ERROR_WRONG_ARGS);
            return PURC_VARIANT_INVALID;
        }
        return doc_get_doctype(doc);
    }

    if (nr_args == 1) {
        if (argv == NULL || argv[0] == PURC_VARIANT_INVALID) {
            pcinst_set_error(PURC_ERROR_WRONG_ARGS);
            return PURC_VARIANT_INVALID;
        }
        purc_variant_t sub = argv[0];
        return doc_get_doctype_by_sub(doc, sub);
    }

    pcinst_set_error(PURC_ERROR_WRONG_ARGS);
    return PURC_VARIANT_INVALID;
}

static inline purc_variant_t
doctype_getter(void *native_entity, size_t nr_args, purc_variant_t* argv)
{
    PC_ASSERT(native_entity);

    purc_variant_t doc = (purc_variant_t)native_entity;

    return do_doctype_getter(doc, nr_args, argv);
}

static inline purc_nvariant_method
doctype_property_getter(const char* key_name)
{
    PC_ASSERT(key_name);

    if (strcmp(key_name, "__getter") == 0) {
        return doctype_getter;
    }

    return NULL;
}

static inline bool
doctype_eraser(void *native_entity)
{
    PC_ASSERT(native_entity);

    purc_variant_t doc = (purc_variant_t)native_entity;
    PC_ASSERT(doc != PURC_VARIANT_INVALID);

    purc_variant_unref(doc);

    return true;
}

static inline purc_variant_t
make_doctype_variant(purc_variant_t doc)
{
    static struct purc_native_ops ops = {
        .property_getter          = doctype_property_getter,
        .property_setter          = NULL,
        .property_eraser          = NULL,
        .property_cleaner         = NULL,
        .cleaner                  = NULL,
        .eraser                   = doctype_eraser,
        .observe                  = NULL,
    };

    purc_variant_t v;
    v = purc_variant_make_native(doc, &ops);
    if (v == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    purc_variant_ref(doc);

    return v;
}

static inline purc_variant_t
do_doc_getter(purc_variant_t doc, size_t nr_args, purc_variant_t* argv)
{
    if (nr_args != 1 ||
        argv[0] == PURC_VARIANT_INVALID ||
        !purc_variant_is_string(argv[0]))
    {
        pcinst_set_error(PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t k = argv[0];
    const char *name = purc_variant_get_string_const(k);
    PC_ASSERT(name);

    if (strcmp(name, "doctype") == 0) {
        return make_doctype_variant(doc);
    }

    pcinst_set_error(PURC_ERROR_WRONG_ARGS);
    return PURC_VARIANT_INVALID;
}

static inline purc_variant_t
doc_getter(void *native_entity, size_t nr_args, purc_variant_t* argv)
{
    PC_ASSERT(native_entity);

    purc_variant_t doc;
    doc = (purc_variant_t)native_entity;

    return do_doc_getter(doc, nr_args, argv);
}

// query the getter for a specific property.
static inline purc_nvariant_method
property_getter (const char* key_name)
{
    PC_ASSERT(key_name);

    if (strcmp(key_name, "__getter") == 0) {
        return doc_getter;
    }

    return NULL;
}

// the eraser to erase the native entity.
static inline bool
eraser(void* native_entity)
{
    PC_ASSERT(native_entity);

    purc_variant_t doc = (purc_variant_t)native_entity;

    purc_variant_unref(doc);
    return true;
}

// FIXME: where to put the declaration
purc_variant_t
pcintr_create_doc_variant(struct pcedom_document *edom_doc)
{
    PC_ASSERT(edom_doc);

    static struct purc_native_ops ops = {
        .property_getter          = property_getter,
        .property_setter          = NULL,
        .property_eraser          = NULL,
        .property_cleaner         = NULL,
        .cleaner                  = NULL,
        .eraser                   = eraser,
        .observe                  = NULL,
    };

    // NOTE: because `edom_doc` would be shared by
    //       $DOC/$DOC.doctype/$DOC.base/...
    //       we make an internal native variant to wrap `edom_doc`
    purc_variant_t doc = make_doc_variant(edom_doc);
    if (doc == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    // FIXME: we can make an object rather than native for easy process,
    //        but we might suffer from being modified by accidently
    //        calling `purc_variant_object_set...`
    purc_variant_t v;
    v = purc_variant_make_native(doc, &ops);
    if (v == PURC_VARIANT_INVALID) {
        purc_variant_unref(doc);
        return PURC_VARIANT_INVALID;
    }

    return v;
}

