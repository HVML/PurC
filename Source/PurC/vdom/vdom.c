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
_document_reset(pcvdom_document_t *doc);
void
_document_destroy(pcvdom_document_t *doc);

void
_doctype_reset(pcvdom_doctype_t *doctype);
void
_doctype_destroy(pcvdom_doctype_t *doctype);

void
_element_reset(pcvdom_element_t *elem);
void
_element_destroy(pcvdom_element_t *elem);

void
_tag_reset(pcvdom_tag_t *tag);
void
_tag_destroy(pcvdom_tag_t *tag);

void
_attr_reset(pcvdom_attr_t *attr);
void
_attr_destroy(pcvdom_attr_t *attr);

void
_exp_reset(pcvdom_exp_t *exp);
void
_exp_destroy(pcvdom_exp_t *exp);

static void
_dom_node_remove(pcvdom_node_t *node)
{
    pcvdom_node_t *parent = pcvdom_node_parent(node);
    if (!parent)
        return;

    parent->remove_child(parent, node);
}

static void
_pcvdom_node_destroy(pcvdom_node_t *node)
{
    if (!node)
        return;

    switch (node->type)
    {
        case PCVDOM_NODE_DOCUMENT:
            {
                pcvdom_document_t *doc;
                doc = PCVDOM_DOCUMENT_FROM_NODE(node);
                _document_destroy(doc);
            } break;
        case PCVDOM_NODE_DOCTYPE:
            {
                pcvdom_doctype_t *doctype;
                doctype = PCVDOM_DOCTYPE_FROM_NODE(node);
                _doctype_destroy(doctype);
            } break;
        case PCVDOM_NODE_ELEMENT:
            {
                pcvdom_element_t *elem;
                elem = PCVDOM_ELEMENT_FROM_NODE(node);
                _element_destroy(elem);
            } break;
        case PCVDOM_NODE_TAG:
            {
                pcvdom_tag_t *tag;
                tag = PCVDOM_TAG_FROM_NODE(node);
                _tag_destroy(tag);
            } break;
        case PCVDOM_NODE_ATTR:
            {
                pcvdom_attr_t *attr;
                attr = PCVDOM_ATTR_FROM_NODE(node);
                _attr_destroy(attr);
            } break;
        case PCVDOM_VDOM_EXP:
            {
                pcvdom_exp_t *exp;
                exp = PCVDOM_EXP_FROM_NODE(node);
                _exp_destroy(exp);
            } break;
        default:
            {
                PC_ASSERT(0);
            } break;
    }
}


static void
_document_remove_child(pcvdom_node_t *me, pcvdom_node_t *child)
{
    pcvdom_document_t *doc;
    doc = container_of(me, pcvdom_document_t, node);
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
_document_reset(pcvdom_document_t *doc)
{
    if (!doc)
        return;

    _doctype_destroy(doc->doctype);
    PC_ASSERT(doc->doctype==NULL);
    _element_destroy(doc->root);
}

void
_document_destroy(pcvdom_document_t *doc)
{
    _document_reset(doc);
    pctree_node_destroy(doc->node.node, NULL);
    free(doc);
}

static void
_doctype_remove_child(pcvdom_node_t *me, pcvdom_node_t *child)
{
    UNUSED_PARAM(me);
    UNUSED_PARAM(child);
}

void
_doctype_reset(pcvdom_doctype_t *doctype)
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
_doctype_destroy(pcvdom_doctype_t *doctype)
{
    if (!doctype)
        return;

    pcvdom_node_remove(&doctype->node);

    _doctype_reset(doctype);
    free(doctype->builtins);
    doctype->builtins = NULL;
    doctype->sz_builtins = 0;
    pctree_node_destroy(doctype->node.node, NULL);
    free(doctype);
}

static void
_element_remove_child(pcvdom_node_t *me, pcvdom_node_t *child)
{
    pcvdom_element_t *elem;
    elem = container_of(me, pcvdom_element_t, node);

    if (child == elem->first_child) {
        struct pctree_node *next = child->node->next;
        elem->first_child = next ? (pcvdom_node_t*)next->user_data : NULL;
    }
    if (child == elem->last_child) {
        struct pctree_node *prev = child->node->prev;
        elem->last_child = prev ? (pcvdom_node_t*)prev->user_data : NULL;
    }

    pctree_node_remove(child->node);
}

void
_element_reset(pcvdom_element_t *elem)
{
    if (!elem)
        return;

    _tag_destroy(elem->tag);

    pcvdom_node_t *child;
    child = elem->first_child;
    while (child) {
        struct pctree_node *next;
        next = child->node->next;

        _pcvdom_node_destroy(child);

        if (!next)
            break;

        child = (pcvdom_node_t*)next->user_data;
    }
}

void
_element_destroy(pcvdom_element_t *elem)
{
    if (!elem)
        return;

    pcvdom_node_remove(&elem->node);

    _element_reset(elem);
    pctree_node_destroy(elem->node.node, NULL);
    free(elem);
}

static void
_tag_remove_child(pcvdom_node_t *me, pcvdom_node_t *child)
{
    pcvdom_tag_t *tag;
    tag = container_of(me, pcvdom_tag_t, node);

    if (child == &tag->first_attr->node) {
        struct pctree_node *next = child->node->next;
        pcvdom_node_t *next_node;
        next_node = next ? (pcvdom_node_t*)next->user_data : NULL;
        tag->first_attr = next_node ?
            container_of(next_node, pcvdom_attr_t, node) :
            NULL;
    }

    if (child == &tag->last_attr->node) {
        struct pctree_node *prev = child->node->prev;
        pcvdom_node_t *prev_node;
        prev_node = prev ? (pcvdom_node_t*)prev->user_data : NULL;
        tag->last_attr = prev_node ?
            container_of(prev_node, pcvdom_attr_t, node) :
            NULL;
    }

    pctree_node_remove(child->node);
}

void
_tag_reset(pcvdom_tag_t *tag)
{
    if (!tag)
        return;

    free(tag->ns);
    tag->ns = NULL;
    free(tag->name);
    tag->name = NULL;

    pcvdom_attr_t *attr;
    attr = tag->first_attr;
    tag->first_attr = NULL;
    tag->last_attr  = NULL;
    while (attr) {
        struct pctree_node *next;
        next = attr->node.node->next;

        _attr_destroy(attr);

        if (!next)
            break;

        attr = (pcvdom_attr_t*)next->user_data;
    }
}

void
_tag_destroy(pcvdom_tag_t *tag)
{
    if (!tag)
        return;

    pcvdom_node_remove(&tag->node);

    _tag_reset(tag);
    pctree_node_destroy(tag->node.node, NULL);
    free(tag);
}

static void
_attr_remove_child(pcvdom_node_t *me, pcvdom_node_t *child)
{
    pcvdom_attr_t *attr;
    attr = container_of(me, pcvdom_attr_t, node);

    if (attr->key && child == &attr->key->node) {
        attr->key = NULL;
    }
    if (attr->val && child == &attr->val->node) {
        attr->val = NULL;
    }

    pctree_node_remove(child->node);
}

void
_attr_reset(pcvdom_attr_t *attr)
{
    if (!attr)
        return;

    _exp_destroy(attr->key);
    attr->key = NULL;
    _exp_destroy(attr->val);
    attr->val = NULL;
}

void
_attr_destroy(pcvdom_attr_t *attr)
{
    if (!attr)
        return;

    pcvdom_node_remove(&attr->node);

    _attr_reset(attr);
    pctree_node_destroy(attr->node.node, NULL);
    free(attr);
}

static void
_exp_remove_child(pcvdom_node_t *me, pcvdom_node_t *child)
{
    UNUSED_PARAM(me);

    pctree_node_remove(child->node);
}

void
_exp_reset(pcvdom_exp_t *exp)
{
    if (!exp)
        return;

    pctree_node_remove(exp->node.node);
    if (exp->result!=PURC_VARIANT_INVALID) {
        purc_variant_unref(exp->result);
        exp->result = PURC_VARIANT_INVALID;
    }
}

void
_exp_destroy(pcvdom_exp_t *exp)
{
    if (!exp)
        return;

    pcvdom_node_remove(&exp->node);

    _exp_reset(exp);
    free(exp);
}

pcvdom_document_t*
pcvdom_document_create(void)
{
    pcvdom_document_t *doc = (pcvdom_document_t*)calloc(1, sizeof(*doc));
    if (!doc) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    doc->node.type = PCVDOM_NODE_DOCUMENT;
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
pcvdom_document_destroy(pcvdom_document_t *doc)
{
    if (!doc)
        return;

    _document_destroy(doc);
}

pcvdom_doctype_t*
pcvdom_doctype_create(void)
{
    pcvdom_doctype_t *doctype;
    doctype = (pcvdom_doctype_t*)calloc(1, sizeof(*doctype));
    if (!doctype) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    doctype->node.type = PCVDOM_NODE_DOCTYPE;
    doctype->node.node = pctree_node_new(&doctype->node);
    if (!doctype->node.node) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        free(doctype);
        return NULL;
    }
    doctype->node.remove_child = _doctype_remove_child;

    return doctype;
}

pcvdom_element_t*
pcvdom_element_create(void)
{
    pcvdom_element_t *elem;
    elem = (pcvdom_element_t*)calloc(1, sizeof(*elem));
    if (!elem) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    elem->node.type = PCVDOM_NODE_ELEMENT;
    elem->node.node = pctree_node_new(&elem->node);
    if (!elem->node.node) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        free(elem);
        return NULL;
    }
    elem->node.remove_child = _element_remove_child;

    return elem;
}

pcvdom_tag_t*
pcvdom_tag_create(void)
{
    pcvdom_tag_t *tag;
    tag = (pcvdom_tag_t*)calloc(1, sizeof(*tag));
    if (!tag) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    tag->node.type = PCVDOM_NODE_TAG;
    tag->node.node = pctree_node_new(&tag->node);
    if (!tag->node.node) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        free(tag);
        return NULL;
    }
    tag->node.remove_child = _tag_remove_child;

    return tag;
}

pcvdom_attr_t*
pcvdom_attr_create(void)
{
    pcvdom_attr_t *attr;
    attr = (pcvdom_attr_t*)calloc(1, sizeof(*attr));
    if (!attr) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    attr->node.type = PCVDOM_NODE_ATTR;
    attr->node.node = pctree_node_new(&attr->node);
    if (!attr->node.node) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        free(attr);
        return NULL;
    }
    attr->node.remove_child = _attr_remove_child;

    return attr;
}

pcvdom_exp_t*
pcvdom_exp_create(void)
{
    pcvdom_exp_t *exp = (pcvdom_exp_t*)calloc(1, sizeof(*exp));
    if (!exp) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    exp->node.type = PCVDOM_VDOM_EXP;
    exp->node.node = pctree_node_new(&exp->node);
    if (!exp->node.node) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        free(exp);
        return NULL;
    }
    exp->node.remove_child = _exp_remove_child;
    exp->result            = PURC_VARIANT_INVALID;
    return exp;
}

int
pcvdom_document_set_doctype(pcvdom_document_t *doc,
        pcvdom_doctype_t *doctype)
{
    PC_ASSERT(doc->doctype == NULL);
    PC_ASSERT(doctype->node.node->parent == NULL);

    pctree_node_prepend_child(doc->node.node, doctype->node.node);
    doc->doctype = doctype;
    return 0;
}

int
pcvdom_document_set_root(pcvdom_document_t *doc,
        pcvdom_element_t *root)
{
    PC_ASSERT(doc->root == NULL);
    PC_ASSERT(root->node.node->parent == NULL);

    pctree_node_append_child(doc->node.node, root->node.node);
    doc->root = root;
    return 0;
}

int
pcvdom_doctype_set_prefix(pcvdom_doctype_t *doc,
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
pcvdom_doctype_append_builtin(pcvdom_doctype_t *doc,
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
        doc->builtins    = ar;
        doc->sz_builtins = sz;
    }

    doc->builtins[doc->nr_builtins++] = p;
    return 0;
}

int
pcvdom_element_set_tag(pcvdom_element_t *elem,
        pcvdom_tag_t *tag)
{
    PC_ASSERT(elem->tag == NULL);
    PC_ASSERT(tag->node.node->parent == NULL);

    pctree_node_prepend_child(elem->node.node, tag->node.node);
    elem->tag = tag;

    return 0;
}

int
pcvdom_element_append_attr(pcvdom_element_t *elem,
        pcvdom_attr_t *attr)
{
    PC_ASSERT(attr->node.node->parent == NULL);
    PC_ASSERT(elem->tag);

    return pcvdom_tag_append_attr(elem->tag, attr);
}

int
pcvdom_element_append_child(pcvdom_element_t *elem,
        pcvdom_element_t *child)
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
pcvdom_tag_set_ns(pcvdom_tag_t *tag,
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
pcvdom_tag_set_name(pcvdom_tag_t *tag,
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
pcvdom_tag_append_attr(pcvdom_tag_t *tag,
        pcvdom_attr_t *attr)
{
    PC_ASSERT(attr->node.node->parent == NULL);

    pctree_node_append_child(tag->node.node, attr->node.node);

    if (!tag->first_attr)
        tag->first_attr = attr;
    tag->last_attr = attr;

    return 0;
}

int
pcvdom_attr_set_key(pcvdom_attr_t *attr,
        pcvdom_exp_t *key)
{
    PC_ASSERT(attr->key == NULL);
    PC_ASSERT(key->node.node->parent == NULL);

    pctree_node_prepend_child(attr->node.node, key->node.node);
    attr->key = key;
    return 0;
}

int
pcvdom_attr_set_val(pcvdom_attr_t *attr,
        pcvdom_exp_t *val)
{
    PC_ASSERT(attr->val == NULL);
    PC_ASSERT(val->node.node->parent == NULL);

    pctree_node_append_child(attr->node.node, val->node.node);
    attr->val = val;
    return 0;
}

pcvdom_node_t* pcvdom_node_parent(pcvdom_node_t *node)
{
    if (!node || !node->node || !node->node->parent)
        return NULL;

    return (pcvdom_node_t*)(node->node->parent->user_data);
}

void
pcvdom_node_remove(pcvdom_node_t *node)
{
    if (!node)
        return;

    _dom_node_remove(node);
}

void
pcvdom_node_destroy(pcvdom_node_t *node)
{
    if (!node)
        return;

    _pcvdom_node_destroy(node);
}

struct pcvdom_node_traverse_s {
    pcvdom_node_t        *top;
    vdom_node_traverse_f  cb;
    void                 *ctx;
    int                   abortion;
};

static void
_pctree_node_cb(struct pctree_node* node,  void* ctx)
{
    struct pcvdom_node_traverse_s *data;
    data = (struct pcvdom_node_traverse_s*)ctx;

    if (data->abortion)
        return;

    pcvdom_node_t *p = (pcvdom_node_t*)(node->user_data);
    data->abortion = data->cb(data->top, p, data->ctx);
}

int
pcvdom_node_traverse(pcvdom_node_t *node, void *ctx,
        vdom_node_traverse_f cb)
{
    if (!node)
        return 0;

    struct pcvdom_node_traverse_s data = {
        .top       = node,
        .cb        = cb,
        .ctx       = ctx,
        .abortion  = 0
    };

    pctree_node_pre_order_traversal(node->node, _pctree_node_cb, &data);

    return data.abortion ? -1 : 0;
}

struct pcvdom_element_traverse_s {
    pcvdom_element_t        *top;
    vdom_element_traverse_f  cb;
    void                 *ctx;
    int                   abortion;
};

static void
_pctree_element_cb(struct pctree_node* node,  void* ctx)
{
    struct pcvdom_element_traverse_s *data;
    data = (struct pcvdom_element_traverse_s*)ctx;

    if (data->abortion)
        return;

    pcvdom_node_t *p = (pcvdom_node_t*)(node->user_data);
    if (p->type == PCVDOM_NODE_ELEMENT) {
        pcvdom_element_t *elem;
        elem = container_of(p, pcvdom_element_t, node);
        data->abortion = data->cb(data->top, elem, data->ctx);
    }
}

int
pcvdom_element_traverse(pcvdom_element_t *elem, void *ctx,
        vdom_element_traverse_f cb)
{
    if (!elem)
        return 0;

    struct pcvdom_element_traverse_s data = {
        .top       = elem,
        .cb        = cb,
        .ctx       = ctx,
        .abortion  = 0
    };

    pctree_node_pre_order_traversal(elem->node.node, _pctree_element_cb, &data);

    return data.abortion ? -1 : 0;
}

