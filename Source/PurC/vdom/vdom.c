/*
 * @file vdom.c
 * @author Xu Xiaohong
 * @date 2021/08/23
 * @brief The implementation of public part for vdom.
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

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/vdom.h"

void pcvdom_init_once(void)
{
    // initialize others
}

void pcvdom_init_instance(struct pcinst* inst)
{
    UNUSED_PARAM(inst);

    // initialize others
}

void pcvdom_cleanup_instance(struct pcinst* inst)
{
    UNUSED_PARAM(inst);
}

pchvml_document_t*
pchvml_document_create(void)
{
    pchvml_document_t *doc = (pchvml_document_t*)calloc(1, sizeof(*doc));
    if (!doc) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    doc->node.type = PCHVML_DOM_NODE_DOCUMENT;
    doc->node.node = pctree_node_new(&doc->node);
    if (!doc->node.node) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        free(doc);
        return NULL;
    }
    return doc;
}

void
pchvml_document_reset(pchvml_document_t *doc)
{
    if (!doc)
        return;

    pctree_node_remove(doc->node.node);
    pchvml_document_doctype_destroy(doc->doctype);
    doc->doctype = NULL;
    pchvml_dom_element_destroy(doc->root);
    doc->root = NULL;
}

void
pchvml_document_destroy(pchvml_document_t *doc)
{
    if (!doc)
        return;

    pchvml_document_reset(doc);
    free(doc);
}

pchvml_document_doctype_t*
pchvml_document_doctype_create(void)
{
    pchvml_document_doctype_t *doctype;
    doctype = (pchvml_document_doctype_t*)calloc(1, sizeof(*doctype));
    if (!doctype) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    doctype->node.type = PCHVML_DOM_NODE_DOCTYPE;
    doctype->node.node = pctree_node_new(&doctype->node);
    if (!doctype->node.node) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        free(doctype);
        return NULL;
    }

    return doctype;
}

void
pchvml_document_doctype_reset(pchvml_document_doctype_t *doctype)
{
    if (!doctype)
        return;

    pctree_node_remove(doctype->node.node);
    free(doctype->prefix);
    doctype->prefix = NULL;
    doctype->nr_builtins = 0;
}

void
pchvml_document_doctype_destroy(pchvml_document_doctype_t *doctype)
{
    if (!doctype)
        return;

    pchvml_document_doctype_reset(doctype);
    free(doctype->builtins);
    doctype->builtins = NULL;
    doctype->sz_builtins = 0;
    free(doctype);
}

pchvml_dom_element_t*
pchvml_dom_element_create(void)
{
    pchvml_dom_element_t *elem;
    elem = (pchvml_dom_element_t*)calloc(1, sizeof(*elem));
    if (!elem) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    elem->node.type = PCHVML_DOM_NODE_ELEMENT;
    elem->node.node = pctree_node_new(&elem->node);
    if (!elem->node.node) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        free(elem);
        return NULL;
    }

    return elem;
}

void
pchvml_dom_element_reset(pchvml_dom_element_t *elem)
{
    if (!elem)
        return;

    pctree_node_remove(elem->node.node);
    pchvml_dom_element_tag_destroy(elem->tag);
    elem->tag = NULL;

    pchvml_dom_node_t *attr;
    attr = elem->first_attr;
    elem->first_attr = NULL;
    elem->last_attr  = NULL;
    while (attr) {
        pchvml_dom_node_t *next;
        next = (pchvml_dom_node_t*)(attr->node->user_data);

        pchvml_dom_element_attr_t *p;
        p = container_of(attr, pchvml_dom_element_attr_t, node);
        pchvml_dom_element_attr_destroy(p);

        attr = next;
    }

#if 0
    pchvml_dom_node_t *child;
    child = elem->first_child;
    elem->first_child = NULL;
    elem->last_child  = NULL;
    while (child) {
        pchvml_dom_node_t *next;
        next = (pchvml_dom_node_t*)(child->node->user_data);

        pchvml_dom_element_attr_t *p;
        p = container_of(child, pchvml_dom_element_attr_t, node);
        pchvml_dom_element_attr_destroy(p);

        child = next;
    }
#endif // 0 
}

void
pchvml_dom_element_destroy(pchvml_dom_element_t *elem)
{
    if (!elem)
        return;

    pchvml_dom_element_reset(elem);
    free(elem);
}

pchvml_dom_element_tag_t*
pchvml_dom_element_tag_create(void)
{
    pchvml_dom_element_tag_t *tag;
    tag = (pchvml_dom_element_tag_t*)calloc(1, sizeof(*tag));
    if (!tag) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    tag->node.type = PCHVML_DOM_NODE_TAG;
    tag->node.node = pctree_node_new(&tag->node);
    if (!tag->node.node) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        free(tag);
        return NULL;
    }

    return tag;
}

void
pchvml_dom_element_tag_reset(pchvml_dom_element_tag_t *tag)
{
    if (!tag)
        return;

    pctree_node_remove(tag->node.node);
    free(tag->ns);
    tag->ns = NULL;
    free(tag->name);
    tag->name = NULL;
}

void
pchvml_dom_element_tag_destroy(pchvml_dom_element_tag_t *tag)
{
    if (!tag)
        return;

    pchvml_dom_element_tag_reset(tag);
    free(tag);
}

pchvml_dom_element_attr_t*
pchvml_dom_element_attr_create(void)
{
    pchvml_dom_element_attr_t *attr;
    attr = (pchvml_dom_element_attr_t*)calloc(1, sizeof(*attr));
    if (!attr) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    attr->node.type = PCHVML_DOM_NODE_ATTR;
    attr->node.node = pctree_node_new(&attr->node);
    if (!attr->node.node) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        free(attr);
        return NULL;
    }

    return attr;
}

void
pchvml_dom_element_attr_reset(pchvml_dom_element_attr_t *attr)
{
    if (!attr)
        return;

    pctree_node_remove(attr->node.node);
    pchvml_vdom_eval_destroy(attr->key);
    attr->key = NULL;
    pchvml_vdom_eval_destroy(attr->val);
    attr->val = NULL;
}

void
pchvml_dom_element_attr_destroy(pchvml_dom_element_attr_t *attr)
{
    if (!attr)
        return;

    pchvml_dom_element_attr_reset(attr);
    free(attr);
}

void
pchvml_vdom_eval_destroy(pchvml_vdom_eval_t *vdom)
{
    UNUSED_PARAM(vdom);
}

