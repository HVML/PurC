/**
 * @file document.c
 * @author Vincent Wei
 * @date 2022/07/11
 * @brief The implementation of target document.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

#include "purc-document.h"
#include "purc-errors.h"

#include "private/document.h"

static struct purc_document_ops *doc_ops[] = {
    &_pcdoc_void_ops,
    NULL, // &_pcdoc_plain_ops,
    NULL, // &_pcdoc_html_ops,
    NULL,
    NULL,
};

/* Make sure the number of doc_ops matches the number of target doc types */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(ops,
        PCA_TABLESIZE(doc_ops) == PCDOC_NR_TYPES);

#undef _COMPILE_TIME_ASSERT

purc_document_t
purc_document_new(purc_document_type type)
{
    struct purc_document_ops *ops = doc_ops[type];
    if (ops == NULL) {
        purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
        return NULL;
    }

    return ops->create(NULL, 0);
}


purc_document_t
purc_document_load(purc_document_type type, const char *content, size_t len)
{
    struct purc_document_ops *ops = doc_ops[type];
    if (ops == NULL) {
        purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
        return NULL;
    }

    return ops->create(content, len);
}

void
purc_document_delete(purc_document_t doc)
{
    doc->ops->destroy(doc);
}


pcdoc_element_t
purc_document_special_elem(purc_document_t doc, pcdoc_special_elem elem)
{
    return doc->ops->special_elem(doc, elem);
}

pcdoc_element_t
pcdoc_element_new_element(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *tag, bool self_close)
{
    return doc->ops->new_element(doc, elem, op, tag, self_close);
}

pcdoc_text_node_t
pcdoc_element_new_text_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *text, size_t len)
{
    return doc->ops->new_text_content(doc, elem, op, text, len);
}

pcdoc_data_node_t
pcdoc_element_set_data_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        purc_variant_t data)
{
    if (doc->ops->new_data_content)
        return doc->ops->new_data_content(doc, elem, op, data);

    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

pcdoc_node_t
pcdoc_element_new_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *content, size_t len, pcdoc_node_type *type)
{
    return doc->ops->new_content(doc, elem, op, content, len, type);
}

bool
pcdoc_element_set_attribute(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *name, const char *val, size_t len)
{
    return doc->ops->set_attribute(doc, elem, op, name, val, len);
}

bool
pcdoc_element_get_attribute(purc_document_t doc, pcdoc_element_t elem,
        const char *name, const char **val, size_t *len)
{
    return doc->ops->get_attribute(doc, elem, name, val, len);
}

bool
pcdoc_text_content_get_text(purc_document_t doc, pcdoc_text_node_t text_node,
        const char **text, size_t *len)
{
    return doc->ops->get_text(doc, text_node, text, len);
}

bool
pcdoc_data_content_get_data(purc_document_t doc, pcdoc_data_node_t data_node,
        purc_variant_t *data)
{
    return doc->ops->get_data(doc, data_node, data);
}

size_t
pcdoc_element_children_count(purc_document_t doc, pcdoc_element_t elem)
{
    return doc->ops->children_count(doc, elem);
}

pcdoc_node_t
pcdoc_element_get_child(purc_document_t doc, pcdoc_element_t elem,
        size_t idx, pcdoc_node_type *type)
{
    return doc->ops->get_child(doc, elem, idx, type);
}

pcdoc_element_t
pcdoc_node_get_parent(purc_document_t doc, pcdoc_node_t node)
{
    return doc->ops->get_parent(doc, node);
}

static pcdoc_elem_coll_t
element_collection_new(purc_document_t doc, bool scope_or_coll,
        const char *selector)
{
    pcdoc_elem_coll_t coll = calloc(1, sizeof(*coll));
    coll->selector = selector ? strdup(selector) : NULL;
    coll->doc_age = doc->age;
    coll->refc = 1;
    coll->scope_or_coll = scope_or_coll;

    coll->sa_elems = pcutils_sorted_array_create(SAFLAG_DEFAULT,
            0, NULL, NULL);

    return coll;
}

static void
element_collection_delete(pcdoc_elem_coll_t coll)
{
    pcutils_sorted_array_destroy(coll->sa_elems);
    return free(coll);
}

static pcdoc_elem_coll_t
element_collection_ref(purc_document_t doc, pcdoc_elem_coll_t coll)
{
    UNUSED_PARAM(doc);

    coll->refc++;
    return coll;
}

static void
element_collection_unref(purc_document_t doc, pcdoc_elem_coll_t coll)
{
    UNUSED_PARAM(doc);

    if (!coll->scope_or_coll) {
        element_collection_unref(doc, coll->super_coll);
    }

    if (coll->refc <= 1) {
        element_collection_delete(coll);
    }
    else {
        coll->refc--;
    }
}

pcdoc_elem_coll_t
pcdoc_elem_coll_new_from_document(purc_document_t doc,
        const char *css_selector)
{
    pcdoc_elem_coll_t coll = element_collection_new(doc, true, css_selector);

    if (doc->ops->elem_coll_select) {
        coll->scope_elem = doc->ops->special_elem(doc,
                PCDOC_SPECIAL_ELEM_ROOT);

        if (!doc->ops->elem_coll_select(doc,
                    coll, coll->scope_elem, css_selector)) {
            element_collection_delete(coll);
            coll = NULL;
        }
    }

    return coll;
}

pcdoc_elem_coll_t
pcdoc_elem_coll_new_from_descendants(purc_document_t doc,
        pcdoc_element_t ancestor, const char *css_selector)
{
    pcdoc_elem_coll_t coll = element_collection_new(doc, true, css_selector);

    if (doc->ops->elem_coll_select) {
        coll->scope_elem = ancestor;

        if (!doc->ops->elem_coll_select(doc,
                    coll, coll->scope_elem, css_selector)) {
            element_collection_delete(coll);
            coll = NULL;
        }
    }

    return coll;
}

pcdoc_elem_coll_t
pcdoc_elem_coll_filter(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll, const char *css_selector)
{
    pcdoc_elem_coll_t dst_coll =
        element_collection_new(doc, false, css_selector);

    if (doc->ops->elem_coll_filter) {
        dst_coll->super_coll = element_collection_ref(doc, elem_coll);
        if (!doc->ops->elem_coll_filter(doc, dst_coll,
                elem_coll, css_selector)) {
            element_collection_delete(dst_coll);
            dst_coll = NULL;
        }
    }

    return dst_coll;
}

void
pcdoc_elem_coll_delete(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll)
{
    element_collection_unref(doc, elem_coll);
}

