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

struct pcintr_doc {
    struct pcedom_document          *edom_doc;
};

static inline void
doc_clean(struct pcintr_doc *doc)
{
    if (!doc)
        return;

    if (doc->edom_doc) {
        pcedom_document_interface_destroy(doc->edom_doc);
        doc->edom_doc = NULL;
    }
}

static purc_variant_t
do_doctype(struct pcintr_doc *doc, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(doc);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    PC_ASSERT(0); // Not implemented yet!!!
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
do_base(struct pcintr_doc *doc, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(doc);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    PC_ASSERT(0); // Not implemented yet!!!
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
do_query(struct pcintr_doc *doc, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(doc);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    PC_ASSERT(0); // Not implemented yet!!!
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
doctype(void* native_entity, size_t nr_args, purc_variant_t* argv)
{
    PC_ASSERT(native_entify);

    struct pcintr_doc *doc = (struct pcintr_doc*)native_entity;

    return do_doctype(doc, nr_args, argv);
}

static purc_variant_t
base(void* native_entity, size_t nr_args, purc_variant_t* argv)
{
    PC_ASSERT(native_entify);

    struct pcintr_doc *doc = (struct pcintr_doc*)native_entity;

    return do_base(doc, nr_args, argv);
}

static purc_variant_t
query(void* native_entity, size_t nr_args, purc_variant_t* argv)
{
    PC_ASSERT(native_entify);

    struct pcintr_doc *doc = (struct pcintr_doc*)native_entity;

    return do_query(doc, nr_args, argv);
}

// query the getter for a specific property.
static inline purc_nvariant_method
property_getter (const char* key_name)
{
    PC_ASSERT(key_name);

    if (strcmp(key_name, "doctype") == 0) {
        return doctype;
    }

    if (strcmp(key_name, "base") == 0) {
        return base;
    }

    if (strcmp(key_name, "query") == 0) {
        return query;
    }

    return NULL;
}

// the cleaner to clear the content of the native entity.
static inline bool
cleaner(void* native_entity)
{
    PC_ASSERT(native_entity);

    struct pcintr_doc *doc = (struct pcintr_doc*)native_entity;
    doc_clean(doc);

    return true;
}

// the eraser to erase the native entity.
static inline bool
eraser(void* native_entity)
{
    PC_ASSERT(native_entity);

    struct pcintr_doc *doc = (struct pcintr_doc*)native_entity;
    doc_clean(doc);

    free(doc);

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
        .cleaner                  = cleaner,
        .eraser                   = eraser,
        .observe                  = NULL,
    };

    struct pcintr_doc *doc;
    doc = (struct pcintr_doc*)calloc(1, sizeof(*doc));
    if (!doc) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v;
    v = purc_variant_make_native(doc, &ops);
    if (v == PURC_VARIANT_INVALID) {
        doc_clean(doc);
        free(doc);
        return PURC_VARIANT_INVALID;
    }

    doc->edom_doc = edom_doc;

    return v;
}


