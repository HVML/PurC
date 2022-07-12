/**
 * @file void-document.c
 * @author Vincent Wei
 * @date 2022/07/11
 * @brief The implementation of void document.
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

static purc_document_t create(const char *content, size_t length)
{
    UNUSED_PARAM(content);
    UNUSED_PARAM(length);

    purc_document_t doc = calloc(1, sizeof(*doc));
    doc->data_content = 0;
    doc->have_head = 0;
    doc->have_body = 0;
    doc->ops = &_pcdoc_void_ops;
    doc->impl = NULL;
    return doc;
}

static void destroy(purc_document_t doc)
{
    free (doc);
}

static pcdoc_element_t operate_element(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation op,
            const char *tag, bool self_close)
{
    UNUSED_PARAM(elem);
    UNUSED_PARAM(op);
    UNUSED_PARAM(tag);
    UNUSED_PARAM(self_close);

    return (pcdoc_element_t)doc;
}

static pcdoc_text_node_t new_text_content(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation op,
            const char *content, size_t length)
{
    UNUSED_PARAM(elem);
    UNUSED_PARAM(op);
    UNUSED_PARAM(content);
    UNUSED_PARAM(length);

    return (pcdoc_text_node_t)doc;
}

static pcdoc_node new_content(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation op,
            const char *content, size_t length)
{
    UNUSED_PARAM(elem);
    UNUSED_PARAM(op);
    UNUSED_PARAM(content);
    UNUSED_PARAM(length);

    pcdoc_node node;
    node.type = PCDOC_NODE_ELEMENT;
    node.elem = (pcdoc_element_t)doc;
    return node;
}

static bool set_attribute(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation op,
            const char *name, const char *val, size_t len)
{
    UNUSED_PARAM(doc);
    UNUSED_PARAM(elem);
    UNUSED_PARAM(op);
    UNUSED_PARAM(name);
    UNUSED_PARAM(val);
    UNUSED_PARAM(len);

    return true;
}

static pcdoc_element_t special_elem(purc_document_t doc,
            pcdoc_special_elem elem)
{
    UNUSED_PARAM(elem);

    return (pcdoc_element_t)doc;
}

static pcdoc_element_t get_parent(purc_document_t doc, pcdoc_node node)
{
    UNUSED_PARAM(doc);
    UNUSED_PARAM(node);
    return NULL;
}

static size_t
children_count(purc_document_t doc, pcdoc_element_t elem, pcdoc_node_type type)
{
    UNUSED_PARAM(doc);
    UNUSED_PARAM(elem);
    UNUSED_PARAM(type);

    return 0;
}

static pcdoc_node
get_child(purc_document_t doc, pcdoc_element_t elem,
        pcdoc_node_type type, size_t idx)
{
    UNUSED_PARAM(doc);
    UNUSED_PARAM(elem);
    UNUSED_PARAM(type);
    UNUSED_PARAM(idx);

    pcdoc_node node;
    node.type = PCDOC_NODE_NULL;
    node.elem = (pcdoc_element_t)NULL;
    return node;
}

static bool get_attribute(purc_document_t doc, pcdoc_element_t elem,
            const char *name, const char **val, size_t *len)
{
    UNUSED_PARAM(doc);
    UNUSED_PARAM(elem);
    UNUSED_PARAM(name);

    *val = "";
    if (len) *len = 0;
    return true;
}

static bool get_text(purc_document_t doc, pcdoc_text_node_t text_node,
            const char **text, size_t *len)
{
    UNUSED_PARAM(doc);
    UNUSED_PARAM(text_node);

    *text = "";
    if (len) *len = 0;
    return true;
}

struct purc_document_ops _pcdoc_void_ops = {
    .create = create,
    .destroy = destroy,
    .operate_element = operate_element,
    .new_text_content = new_text_content,
    .new_data_content = NULL,
    .new_content = new_content,
    .set_attribute = set_attribute,
    .special_elem = special_elem,
    .get_parent = get_parent,
    .children_count = children_count,
    .get_child = get_child,
    .get_attribute = get_attribute,
    .get_text = get_text,
    .get_data = NULL,
    .elem_coll_select = NULL,
    .elem_coll_filter = NULL,
};

