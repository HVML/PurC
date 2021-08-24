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


void
_document_reset(pchvml_document_t *doc);
void
_document_destroy(pchvml_document_t *doc);

void
_doctype_reset(pchvml_document_doctype_t *doctype);
void
_doctype_destroy(pchvml_document_doctype_t *doctype);

void
_element_reset(pchvml_dom_element_t *elem);
void
_element_destroy(pchvml_dom_element_t *elem);

void
_tag_reset(pchvml_dom_element_tag_t *tag);
void
_tag_destroy(pchvml_dom_element_tag_t *tag);

void
_attr_reset(pchvml_dom_element_attr_t *attr);
void
_attr_destroy(pchvml_dom_element_attr_t *attr);

void
_eval_reset(pchvml_vdom_eval_t *eval);
void
_eval_destroy(pchvml_vdom_eval_t *eval);

static void
_dom_node_remove(pchvml_dom_node_t *node)
{
    pchvml_dom_node_t *parent = pchvml_dom_node_parent(node);
    if (!parent)
        return;

    parent->remove_child(parent, node);
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
                _document_destroy(doc);
            } break;
        case PCHVML_DOM_NODE_DOCTYPE:
            {
                pchvml_document_doctype_t *doctype;
                doctype = PCHVML_DOCTYPE_FROM_NODE(node);
                _doctype_destroy(doctype);
            } break;
        case PCHVML_DOM_NODE_ELEMENT:
            {
                pchvml_dom_element_t *elem;
                elem = PCHVML_ELEMENT_FROM_NODE(node);
                _element_destroy(elem);
            } break;
        case PCHVML_DOM_NODE_TAG:
            {
                pchvml_dom_element_tag_t *tag;
                tag = PCHVML_ELEMENT_TAG_FROM_NODE(node);
                _tag_destroy(tag);
            } break;
        case PCHVML_DOM_NODE_ATTR:
            {
                pchvml_dom_element_attr_t *attr;
                attr = PCHVML_ELEMENT_ATTR_FROM_NODE(node);
                _attr_destroy(attr);
            } break;
        case PCHVML_DOM_VDOM_EVAL:
            {
                pchvml_vdom_eval_t *eval;
                eval = PCHVML_VDOM_EVAL_FROM_NODE(node);
                _eval_destroy(eval);
            } break;
        default:
            {
                PC_ASSERT(0);
            } break;
    }
}


static void
_document_remove_child(pchvml_dom_node_t *me, pchvml_dom_node_t *child)
{
    pchvml_document_t *doc;
    doc = container_of(me, pchvml_document_t, node);
    PC_ASSERT(&doc->node == me);
    PC_ASSERT(&doc->node == me->node->user_data);

    if (doc->doctype && child == &doc->doctype->node) {
        doc->doctype = NULL;
    }
    if (doc->root && child == &doc->root->node) {
        doc->root = NULL;
    }

    pctree_node_remove(child->node);
}

void
_document_reset(pchvml_document_t *doc)
{
    if (!doc)
        return;

    _doctype_destroy(doc->doctype);
    PC_ASSERT(doc->doctype==NULL);
    _element_destroy(doc->root);
}

void
_document_destroy(pchvml_document_t *doc)
{
    _document_reset(doc);
    pctree_node_destroy(doc->node.node, NULL);
    free(doc);
}

static void
_doctype_remove_child(pchvml_dom_node_t *me, pchvml_dom_node_t *child)
{
    UNUSED_PARAM(me);
    UNUSED_PARAM(child);
}

void
_doctype_reset(pchvml_document_doctype_t *doctype)
{
    if (!doctype)
        return;

    free(doctype->prefix);
    doctype->prefix = NULL;
    for (size_t i=0; i<doctype->nr_builtins; ++i) {
        char *p = doctype->builtins[i];
        free(p);
    }
    doctype->nr_builtins = 0;
}

void
_doctype_destroy(pchvml_document_doctype_t *doctype)
{
    if (!doctype)
        return;

    pchvml_dom_node_remove(&doctype->node);

    _doctype_reset(doctype);
    free(doctype->builtins);
    doctype->builtins = NULL;
    doctype->sz_builtins = 0;
    pctree_node_destroy(doctype->node.node, NULL);
    free(doctype);
}

static void
_element_remove_child(pchvml_dom_node_t *me, pchvml_dom_node_t *child)
{
    pchvml_dom_element_t *elem;
    elem = container_of(me, pchvml_dom_element_t, node);

    if (child == elem->first_child) {
        struct pctree_node *next = child->node->next;
        elem->first_child = next ? (pchvml_dom_node_t*)next->user_data : NULL;
    }
    if (child == elem->last_child) {
        struct pctree_node *prev = child->node->prev;
        elem->last_child = prev ? (pchvml_dom_node_t*)prev->user_data : NULL;
    }

    pctree_node_remove(child->node);
}

void
_element_reset(pchvml_dom_element_t *elem)
{
    if (!elem)
        return;

    _tag_destroy(elem->tag);

    pchvml_dom_node_t *child;
    child = elem->first_child;
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
_element_destroy(pchvml_dom_element_t *elem)
{
    if (!elem)
        return;

    pchvml_dom_node_remove(&elem->node);

    _element_reset(elem);
    pctree_node_destroy(elem->node.node, NULL);
    free(elem);
}

static void
_tag_remove_child(pchvml_dom_node_t *me, pchvml_dom_node_t *child)
{
    pchvml_dom_element_tag_t *tag;
    tag = container_of(me, pchvml_dom_element_tag_t, node);

    if (child == &tag->first_attr->node) {
        struct pctree_node *next = child->node->next;
        pchvml_dom_node_t *next_node;
        next_node = next ? (pchvml_dom_node_t*)next->user_data : NULL;
        tag->first_attr = next_node ?
            container_of(next_node, pchvml_dom_element_attr_t, node) :
            NULL;
    }

    if (child == &tag->last_attr->node) {
        struct pctree_node *prev = child->node->prev;
        pchvml_dom_node_t *prev_node;
        prev_node = prev ? (pchvml_dom_node_t*)prev->user_data : NULL;
        tag->last_attr = prev_node ?
            container_of(prev_node, pchvml_dom_element_attr_t, node) :
            NULL;
    }

    pctree_node_remove(child->node);
}

void
_tag_reset(pchvml_dom_element_tag_t *tag)
{
    if (!tag)
        return;

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

        _attr_destroy(attr);

        if (!next)
            break;

        attr = (pchvml_dom_element_attr_t*)next->user_data;
    }
}

void
_tag_destroy(pchvml_dom_element_tag_t *tag)
{
    if (!tag)
        return;

    pchvml_dom_node_remove(&tag->node);

    _tag_reset(tag);
    pctree_node_destroy(tag->node.node, NULL);
    free(tag);
}

static void
_attr_remove_child(pchvml_dom_node_t *me, pchvml_dom_node_t *child)
{
    pchvml_dom_element_attr_t *attr;
    attr = container_of(me, pchvml_dom_element_attr_t, node);

    if (attr->key && child == &attr->key->node) {
        attr->key = NULL;
    }
    if (attr->val && child == &attr->val->node) {
        attr->val = NULL;
    }

    pctree_node_remove(child->node);
}

void
_attr_reset(pchvml_dom_element_attr_t *attr)
{
    if (!attr)
        return;

    _eval_destroy(attr->key);
    attr->key = NULL;
    _eval_destroy(attr->val);
    attr->val = NULL;
}

void
_attr_destroy(pchvml_dom_element_attr_t *attr)
{
    if (!attr)
        return;

    pchvml_dom_node_remove(&attr->node);

    _attr_reset(attr);
    pctree_node_destroy(attr->node.node, NULL);
    free(attr);
}

static void
_eval_remove_child(pchvml_dom_node_t *me, pchvml_dom_node_t *child)
{
    UNUSED_PARAM(me);

    pctree_node_remove(child->node);
}

void
_eval_reset(pchvml_vdom_eval_t *eval)
{
    if (!eval)
        return;

    pctree_node_remove(eval->node.node);
    if (eval->result!=PURC_VARIANT_INVALID) {
        purc_variant_unref(eval->result);
        eval->result = PURC_VARIANT_INVALID;
    }
}

void
_eval_destroy(pchvml_vdom_eval_t *eval)
{
    if (!eval)
        return;

    pchvml_dom_node_remove(&eval->node);

    _eval_reset(eval);
    free(eval);
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
    doc->node.remove_child = _document_remove_child;
    return doc;
}

void
pchvml_document_destroy(pchvml_document_t *doc)
{
    if (!doc)
        return;

    _document_destroy(doc);
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
    doctype->node.remove_child = _doctype_remove_child;

    return doctype;
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
    elem->node.remove_child = _element_remove_child;

    return elem;
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
    tag->node.remove_child = _tag_remove_child;

    return tag;
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
    attr->node.remove_child = _attr_remove_child;

    return attr;
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
    eval->node.remove_child = _eval_remove_child;
    eval->result            = PURC_VARIANT_INVALID;
    return eval;
}

int
pchvml_document_set_doctype(pchvml_document_t *doc,
        pchvml_document_doctype_t *doctype)
{
    PC_ASSERT(doc->doctype == NULL);
    PC_ASSERT(doctype->node.node->parent == NULL);

    pctree_node_prepend_child(doc->node.node, doctype->node.node);
    doc->doctype = doctype;
    return 0;
}

int
pchvml_document_set_root(pchvml_document_t *doc,
        pchvml_dom_element_t *root)
{
    PC_ASSERT(doc->root == NULL);
    PC_ASSERT(root->node.node->parent == NULL);

    pctree_node_append_child(doc->node.node, root->node.node);
    doc->root = root;
    return 0;
}

int
pchvml_document_doctype_set_prefix(pchvml_document_doctype_t *doc,
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

int
pchvml_document_doctype_append_builtin(pchvml_document_doctype_t *doc,
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

int
pchvml_dom_element_set_tag(pchvml_dom_element_t *elem,
        pchvml_dom_element_tag_t *tag)
{
    PC_ASSERT(elem->tag == NULL);
    PC_ASSERT(tag->node.node->parent == NULL);

    pctree_node_prepend_child(elem->node.node, tag->node.node);
    elem->tag = tag;

    return 0;
}

int
pchvml_dom_element_append_attr(pchvml_dom_element_t *elem,
        pchvml_dom_element_attr_t *attr)
{
    PC_ASSERT(attr->node.node->parent == NULL);
    PC_ASSERT(elem->tag);

    return pchvml_dom_element_tag_append_attr(elem->tag, attr);
}

int
pchvml_dom_element_append_child(pchvml_dom_element_t *elem,
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

int
pchvml_dom_element_tag_set_ns(pchvml_dom_element_tag_t *tag,
        const char *ns)
{
    if (tag->ns &&
        (tag->ns == ns || strcasecmp(tag->ns, ns)==0))
    {
            return 0;
    }
    char *p = strdup(ns);
    if (!p) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    free(tag->ns);
    tag->ns = p;
    return 0;
}

int
pchvml_dom_element_tag_set_name(pchvml_dom_element_tag_t *tag,
        const char *name)
{
    if (tag->name &&
        (tag->name == name || strcasecmp(tag->name, name)==0))
    {
            return 0;
    }
    char *p = strdup(name);
    if (!p) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    free(tag->name);
    tag->name = p;
    return 0;
}

int
pchvml_dom_element_tag_append_attr(pchvml_dom_element_tag_t *tag,
        pchvml_dom_element_attr_t *attr)
{
    PC_ASSERT(attr->node.node->parent == NULL);

    pctree_node_append_child(tag->node.node, attr->node.node);

    if (!tag->first_attr)
        tag->first_attr = attr;
    tag->last_attr = attr;

    return 0;
}

int
pchvml_dom_element_attr_set_key(pchvml_dom_element_attr_t *attr,
        pchvml_vdom_eval_t *key)
{
    PC_ASSERT(attr->key == NULL);
    PC_ASSERT(key->node.node->parent == NULL);

    pctree_node_prepend_child(attr->node.node, key->node.node);
    attr->key = key;
    return 0;
}

int
pchvml_dom_element_attr_set_val(pchvml_dom_element_attr_t *attr,
        pchvml_vdom_eval_t *val)
{
    PC_ASSERT(attr->val == NULL);
    PC_ASSERT(val->node.node->parent == NULL);

    pctree_node_append_child(attr->node.node, val->node.node);
    attr->val = val;
    return 0;
}

pchvml_dom_node_t* pchvml_dom_node_parent(pchvml_dom_node_t *node)
{
    if (!node || !node->node || !node->node->parent)
        return NULL;

    return (pchvml_dom_node_t*)(node->node->parent->user_data);
}

void
pchvml_dom_node_remove(pchvml_dom_node_t *node)
{
    if (!node)
        return;

    _dom_node_remove(node);
}

void
pchvml_dom_node_destroy(pchvml_dom_node_t *node)
{
    if (!node)
        return;

    _pchvml_dom_node_destroy(node);
}

