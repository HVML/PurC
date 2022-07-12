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
    return doc->ops->operate_element(doc, elem, op, tag, self_close);
}

void
pcdoc_element_clear(purc_document_t doc, pcdoc_element_t elem)
{
    doc->ops->operate_element(doc, elem, PCDOC_OP_CLEAR, NULL, 0);
}

void
pcdoc_element_remove(purc_document_t doc, pcdoc_element_t elem)
{
    doc->ops->operate_element(doc, elem, PCDOC_OP_ERASE, NULL, 0);
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

pcdoc_node
pcdoc_element_new_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *content, size_t len)
{
    return doc->ops->new_content(doc, elem, op, content, len);
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

pcdoc_node
pcdoc_element_get_child(purc_document_t doc, pcdoc_element_t elem, size_t idx)
{
    return doc->ops->get_child(doc, elem, idx);
}

pcdoc_element_t
pcdoc_node_get_parent(purc_document_t doc, pcdoc_node node)
{
    return doc->ops->get_parent(doc, node);
}

pcdoc_element_t
pcdoc_find_element_in_descendants(purc_document_t doc,
        pcdoc_element_t ancestor, const char *selector)
{
    pcdoc_element_t found;

    if (doc->ops->find_elem) {
        if (ancestor == NULL)
            ancestor = doc->ops->special_elem(doc, PCDOC_SPECIAL_ELEM_ROOT);

        found = doc->ops->find_elem(doc, ancestor, selector);
    }
    else {
        found = NULL;
    }

    return found;
}

static pcdoc_elem_coll_t
element_collection_new(const char *selector)
{
    pcdoc_elem_coll_t coll = calloc(1, sizeof(*coll));
    coll->selector = selector ? strdup(selector) : NULL;
    coll->refc = 1;
    coll->elems = pcutils_arrlist_new_ex(NULL, 4);

    return coll;
}

pcdoc_elem_coll_t
pcdoc_elem_coll_new_from_descendants(purc_document_t doc,
        pcdoc_element_t ancestor, const char *selector)
{
    pcdoc_elem_coll_t coll = element_collection_new(selector);

    if (doc->ops->elem_coll_select) {
        if (ancestor == NULL) {
            ancestor = doc->ops->special_elem(doc,
                    PCDOC_SPECIAL_ELEM_ROOT);
        }

        if (!doc->ops->elem_coll_select(doc, coll, ancestor, selector)) {
            pcdoc_elem_coll_delete(doc, coll);
            coll = NULL;
        }
    }

    return coll;
}

pcdoc_elem_coll_t
pcdoc_elem_coll_filter(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll, const char *selector)
{
    pcdoc_elem_coll_t dst_coll = element_collection_new(selector);

    if (doc->ops->elem_coll_filter) {
        if (!doc->ops->elem_coll_filter(doc, dst_coll,
                elem_coll, selector)) {
            pcdoc_elem_coll_delete(doc, dst_coll);
            dst_coll = NULL;
        }
    }

    return dst_coll;
}

void
pcdoc_elem_coll_delete(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll)
{
    UNUSED_PARAM(doc);

    pcutils_arrlist_free(elem_coll->elems);
    return free(elem_coll);
}

#if 0
static void
element_collection_delete(pcdoc_elem_coll_t coll)
{
    pcutils_arrlist_free(coll->elems);
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

    if (coll->refc <= 1) {
        element_collection_delete(coll);
    }
    else {
        coll->refc--;
    }
}
#endif

