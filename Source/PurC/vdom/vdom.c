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

static void
_pchvml_dom_node_destroy(pchvml_dom_node_t *node)
{
    if (!node)
        return;

    switch (node->type)
    {
        case PCHVML_DOM_NODE_DOCUMENT:
            {
                pchvml_document_t *doc;
                doc = PCHVML_DOCUMENT_FROM_NODE(node);
                pchvml_document_destroy(doc);
            } break;
        case PCHVML_DOM_NODE_DOCTYPE:
            {
                pchvml_document_doctype_t *doctype;
                doctype = PCHVML_DOCTYPE_FROM_NODE(node);
                pchvml_document_doctype_destroy(doctype);
            } break;
        case PCHVML_DOM_NODE_ELEMENT:
            {
                pchvml_dom_element_t *elem;
                elem = PCHVML_ELEMENT_FROM_NODE(node);
                pchvml_dom_element_destroy(elem);
            } break;
        case PCHVML_DOM_NODE_TAG:
            {
                pchvml_dom_element_tag_t *tag;
                tag = PCHVML_ELEMENT_TAG_FROM_NODE(node);
                pchvml_dom_element_tag_destroy(tag);
            } break;
        case PCHVML_DOM_NODE_ATTR:
            {
                pchvml_dom_element_attr_t *attr;
                attr = PCHVML_ELEMENT_ATTR_FROM_NODE(node);
                pchvml_dom_element_attr_destroy(attr);
            } break;
        case PCHVML_DOM_VDOM_EVAL:
            {
                pchvml_vdom_eval_t *eval;
                eval = PCHVML_VDOM_EVAL_FROM_NODE(node);
                pchvml_vdom_eval_destroy(eval);
            } break;
        default:
            {
                PC_ASSERT(0);
            } break;
    }
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
    pctree_node_destroy(doc->node.node, NULL);
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
    for (size_t i=0; i<doctype->nr_builtins; ++i) {
        char *p = doctype->builtins[i];
        free(p);
    }
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
    pctree_node_destroy(doctype->node.node, NULL);
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

    pchvml_dom_node_t *child;
    child = elem->first_child;
    elem->first_child = NULL;
    elem->last_child  = NULL;
    while (child) {
        struct pctree_node *next;
        next = child->node->next;

        _pchvml_dom_node_destroy(child);

        if (!next)
            break;
        child = (pchvml_dom_node_t*)next->user_data;
    }
}

void
pchvml_dom_element_destroy(pchvml_dom_element_t *elem)
{
    if (!elem)
        return;

    pchvml_dom_element_reset(elem);
    pctree_node_destroy(elem->node.node, NULL);
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

    pchvml_dom_element_attr_t *attr;
    attr = tag->first_attr;
    tag->first_attr = NULL;
    tag->last_attr  = NULL;
    while (attr) {
        struct pctree_node *next;
        next = attr->node.node->next;

        pchvml_dom_element_attr_destroy(attr);

        if (!next)
            break;
        attr = (pchvml_dom_element_attr_t*)next->user_data;
    }
}

void
pchvml_dom_element_tag_destroy(pchvml_dom_element_tag_t *tag)
{
    if (!tag)
        return;

    pchvml_dom_element_tag_reset(tag);
    pctree_node_destroy(tag->node.node, NULL);
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
    pctree_node_destroy(attr->node.node, NULL);
    free(attr);
}

pchvml_vdom_eval_t*
pchvml_vdom_eval_create(void)
{
    pchvml_vdom_eval_t *eval = (pchvml_vdom_eval_t*)calloc(1, sizeof(*eval));
    if (!eval) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    eval->node.type = PCHVML_DOM_VDOM_EVAL;
    eval->node.node = pctree_node_new(&eval->node);
    if (!eval->node.node) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        free(eval);
        return NULL;
    }
    return eval;
}

void
pchvml_vdom_eval_reset(pchvml_vdom_eval_t *eval)
{
    if (!eval)
        return;

    pctree_node_remove(eval->node.node);
}

void
pchvml_vdom_eval_destroy(pchvml_vdom_eval_t *eval)
{
    if (!eval)
        return;

    pchvml_vdom_eval_reset(eval);
    free(eval);
}

int pchvml_document_set_doctype(pchvml_document_t *doc,
        pchvml_document_doctype_t *doctype)
{
    PC_ASSERT(doc->doctype == NULL);
    PC_ASSERT(doctype->node.node->parent == NULL);

    pctree_node_prepend_child(doc->node.node, doctype->node.node);
    doc->doctype = doctype;
    return 0;
}

int pchvml_document_set_root(pchvml_document_t *doc,
        pchvml_dom_element_t *root)
{
    PC_ASSERT(doc->root == NULL);
    PC_ASSERT(root->node.node->parent == NULL);

    pctree_node_append_child(doc->node.node, root->node.node);
    doc->root = root;
    return 0;
}

int pchvml_document_doctype_set_prefix(pchvml_document_doctype_t *doc,
        const char *prefix)
{
    if (doc->prefix &&
        (doc->prefix == prefix || strcasecmp(doc->prefix, prefix)==0))
    {
            return 0;
    }
    char *p = strdup(prefix);
    if (!p) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    free(doc->prefix);
    doc->prefix = p;
    return 0;
}

int pchvml_document_doctype_append_builtin(pchvml_document_doctype_t *doc,
        const char *builtin)
{
    for (size_t i=0; i<doc->nr_builtins; ++i) {
        const char *p = doc->builtins[i];
        if (p==builtin || strcmp(p, builtin)==0)
            return 0;
    }

    char *p = strdup(builtin);
    if (!p) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    if (doc->nr_builtins>=doc->sz_builtins) {
        size_t sz = doc->sz_builtins+30;
        char **ar = (char**)realloc(doc->builtins, sz*sizeof(*ar));
        if (!ar) {
            free(p);
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return -1;
        }
        doc->builtins = ar;
        doc->sz_builtins = sz;
    }

    doc->builtins[doc->nr_builtins++] = p;
    return 0;
}

int pchvml_dom_element_set_tag(pchvml_dom_element_t *elem,
        pchvml_dom_element_tag_t *tag)
{
    PC_ASSERT(elem->tag == NULL);
    PC_ASSERT(tag->node.node->parent == NULL);

    pctree_node_prepend_child(elem->node.node, tag->node.node);
    elem->tag = tag;

    return 0;
}

int pchvml_dom_element_append_attr(pchvml_dom_element_t *elem,
        pchvml_dom_element_attr_t *attr)
{
    PC_ASSERT(attr->node.node->parent == NULL);
    PC_ASSERT(elem->tag);
    PC_ASSERT(0);

    // return _pchvml_dom_element_tag_append_attr(elem->tag, attr);
    // pctree_node_append_child(elem->node.node, attr->node.node);

    return 0;
}

int pchvml_dom_element_append_child(pchvml_dom_element_t *elem,
        pchvml_dom_element_t *child)
{
    PC_ASSERT(child->node.node->parent == NULL);
    PC_ASSERT(elem->tag);

    pctree_node_append_child(elem->node.node, child->node.node);

    if (!elem->first_child)
        elem->first_child = &child->node;
    elem->last_child = &child->node;

    return 0;
}


