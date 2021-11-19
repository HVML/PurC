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

#include "element.h"

#include "private/debug.h"
#include "private/errors.h"
#include "private/edom.h"
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
doctype_default(struct pcedom_document *doc)
{
    const char *s = "html";

    pcedom_document_type_t *doc_type;
    doc_type = doc->doctype;
    if (doc_type) {
        const unsigned char *doctype;
        size_t len;
        doctype = pcedom_document_type_name(doc_type, &len);
        PC_ASSERT(doctype);
        PC_ASSERT(doctype[len]=='\0');
        s = (const char*)doctype;
    }

    // NOTE: we don't hold ownership
    return purc_variant_make_string_static(s, false);
}

static inline purc_variant_t
doctype_system(struct pcedom_document *doc)
{
    const char *s = "";

    pcedom_document_type_t *doc_type;
    doc_type = doc->doctype;
    if (doc_type) {
        const unsigned char *s_system;
        size_t len;
        s_system = pcedom_document_type_system_id(doc_type, &len);
        PC_ASSERT(s_system);
        PC_ASSERT(s_system[len]=='\0');
        s = (const char*)s_system;
    }

    // NOTE: we don't hold ownership
    return purc_variant_make_string_static(s, false);
}

static inline purc_variant_t
doctype_public(struct pcedom_document *doc)
{
    const char *s = "";

    pcedom_document_type_t *doc_type;
    doc_type = doc->doctype;
    if (doc_type) {
        const unsigned char *s_public;
        size_t len;
        s_public = pcedom_document_type_public_id(doc_type, &len);
        PC_ASSERT(s_public);
        PC_ASSERT(s_public[len]=='\0');
        s = (const char*)s_public;
    }

    // NOTE: we don't hold ownership
    return purc_variant_make_string_static(s, false);
}

static inline purc_variant_t
doctype_getter(purc_variant_t root, size_t nr_args, purc_variant_t * argv)
{
    PC_ASSERT(root != PURC_VARIANT_INVALID);

    purc_variant_t edom;
    edom = purc_variant_object_get_by_ckey(root, "__edom");
    PC_ASSERT(edom != PURC_VARIANT_INVALID);
    PC_ASSERT(!purc_variant_is_ulongint(edom));
    uint64_t u64;
    bool ok;
    ok = purc_variant_cast_to_ulongint(edom, &u64, false);
    PC_ASSERT(ok && u64);
    struct pcedom_document *doc;
    doc = (struct pcedom_document*)u64;

    if (nr_args == 0) {
        if (argv != NULL) {
            pcinst_set_error(PURC_ERROR_WRONG_ARGS);
            return PURC_VARIANT_INVALID;
        }
        return doctype_default(doc);
    }

    if (nr_args == 1) {
        if (argv == NULL || argv[0] == PURC_VARIANT_INVALID) {
            pcinst_set_error(PURC_ERROR_WRONG_ARGS);
            return PURC_VARIANT_INVALID;
        }
        purc_variant_t v = argv[0];
        if (!purc_variant_is_string(v)) {
            pcinst_set_error(PURC_ERROR_WRONG_ARGS);
            return PURC_VARIANT_INVALID;
        }
        const char *name = purc_variant_get_string_const(v);
        if (strcmp(name, "system") == 0) {
            return doctype_system(doc);
        }
        if (strcmp(name, "public") == 0) {
            return doctype_public(doc);
        }
        pcinst_set_error(PURC_ERROR_NOT_EXISTS);
        return PURC_VARIANT_INVALID;
    }

    pcinst_set_error(PURC_ERROR_WRONG_ARGS);
    return PURC_VARIANT_INVALID;
}

static inline purc_variant_t
query(struct pcedom_document *doc, const char *css)
{
    struct pcedom_element **elems    = NULL;
    size_t                  nr_elems = 0;

    // TODO:
    UNUSED_PARAM(doc);
    UNUSED_PARAM(css);

    PC_ASSERT(0); // Not implemented yet

    return pcintr_make_elements(nr_elems, elems);
}

static inline purc_variant_t
query_getter(purc_variant_t root, size_t nr_args, purc_variant_t * argv)
{
    PC_ASSERT(root != PURC_VARIANT_INVALID);

    purc_variant_t edom;
    edom = purc_variant_object_get_by_ckey(root, "__edom");
    PC_ASSERT(edom != PURC_VARIANT_INVALID);
    PC_ASSERT(!purc_variant_is_ulongint(edom));
    uint64_t u64;
    bool ok;
    ok = purc_variant_cast_to_ulongint(edom, &u64, false);
    PC_ASSERT(ok && u64);
    struct pcedom_document *doc;
    doc = (struct pcedom_document*)u64;

    if (nr_args == 1) {
        if (argv == NULL || argv[0] == PURC_VARIANT_INVALID) {
            pcinst_set_error(PURC_ERROR_WRONG_ARGS);
            return PURC_VARIANT_INVALID;
        }
        purc_variant_t v = argv[0];
        if (!purc_variant_is_string(v)) {
            pcinst_set_error(PURC_ERROR_WRONG_ARGS);
            return PURC_VARIANT_INVALID;
        }
        const char *css = purc_variant_get_string_const(v);
        return query(doc, css);
    }

    pcinst_set_error(PURC_ERROR_WRONG_ARGS);
    return PURC_VARIANT_INVALID;
}

static inline bool
add_edom_doc(purc_variant_t doc, struct pcedom_document *edom_doc)
{
    uint64_t u64 = (uint64_t)edom_doc;
    purc_variant_t v;
    v = purc_variant_make_ulongint(u64);
    if (v == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    bool ok = purc_variant_object_set_by_static_ckey(doc, "__edom", v);
    if (!ok) {
        purc_variant_unref(v);
        return false;
    }

    return true;
}

purc_variant_t
pcintr_make_doc_variant(struct pcedom_document *edom_doc)
{
    struct dynamic_args args[] = {
        {"doctype", doctype_getter, NULL},
        {"query",   query_getter,   NULL},
    };

    purc_variant_t v = make_object(PCA_TABLESIZE(args), args);
    if (v == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    bool ok = add_edom_doc(v, edom_doc);
    if (!ok) {
        purc_variant_unref(v);
        return PURC_VARIANT_INVALID;
    }

    return v;
}

