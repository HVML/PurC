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

#include "hvml-attr.h"

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

#define VDT(x)     PCVDOM_NODE_##x
#define VTT(x)     PCHVML_TAG_##x
#define PAO(x)     PCVDOM_ATTR_OP_##x

static void
_document_reset(struct pcvdom_document *doc);

static void
_document_destroy(struct pcvdom_document *doc);

static struct pcvdom_document*
_document_create(const char *doctype);

static void
_element_reset(struct pcvdom_element *elem);

static void
_element_destroy(struct pcvdom_element *elem);

static struct pcvdom_element*
_element_create(void);

static void
_content_reset(struct pcvdom_content *doc);

static void
_content_destroy(struct pcvdom_content *doc);

static struct pcvdom_content*
_content_create(struct pcvcm_tree *vcm);

static void
_vdom_node_remove(struct pcvdom_node *node);

static void
_comment_reset(struct pcvdom_comment *doc);

static void
_comment_destroy(struct pcvdom_comment *doc);

static struct pcvdom_comment*
_comment_create(const char *text);

static void
_attr_reset(struct pcvdom_attr *doc);

static void
_attr_destroy(struct pcvdom_attr *doc);

static struct pcvdom_attr*
_attr_create(void);

static void
_vdom_node_destroy(struct pcvdom_node *node);

// creating and destroying api
void
pcvdom_document_destroy(struct pcvdom_document *doc)
{
    if (!doc)
        return;

    _document_destroy(doc);
}

struct pcvdom_document*
pcvdom_document_create(const char *doctype)
{
    if (!doctype) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    return _document_create(doctype);
}

struct pcvdom_element*
pcvdom_element_create(pcvdom_tag_id tag)
{
    if (tag < PCHVML_TAG_FIRST_ENTRY ||
        tag >= PCHVML_TAG_LAST_ENTRY)
    {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    struct pcvdom_element *elem = _element_create();
    if (!elem) {
        return NULL;
    }

    const struct pchvml_tag_entry *entry;
    entry = pchvml_tag_static_get_by_id(tag);
    if (entry) {
        elem->tag_id   = entry->id;
        elem->tag_name = (char*)entry->name;
    } else {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        _element_destroy(elem);
        return NULL;
    }

    return elem;
}

struct pcvdom_element*
pcvdom_element_create_c(const char *tag_name)
{
    if (!tag_name) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    struct pcvdom_element *elem = _element_create();
    if (!elem) {
        return NULL;
    }

    const struct pchvml_tag_entry *entry;
    entry = pchvml_tag_static_search(tag_name, strlen(tag_name));
    if (entry) {
        elem->tag_id   = entry->id;
        elem->tag_name = (char*)entry->name;
    } else {
        elem->tag_name = strdup(tag_name);
        if (!elem->tag_name) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            _element_destroy(elem);
            return NULL;
        }
    }

    return elem;
}

struct pcvdom_content*
pcvdom_content_create(struct pcvcm_tree *vcm)
{
    return _content_create(vcm);
}

struct pcvdom_comment*
pcvdom_comment_create(const char *text)
{
    if (!text) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    return _comment_create(text);

}

// key = vcm
// or
// key,    in case when vcm == NULL
struct pcvdom_attr*
pcvdom_attr_create_simple(const char *key, struct pcvcm_tree *vcm)
{
    return pcvdom_attr_create(key, PAO(EQ), vcm);
}


// for modification operators, such as +=|-=|%=|~=|^=|$=
struct pcvdom_attr*
pcvdom_attr_create(const char *key, enum pcvdom_attr_op op,
    struct pcvcm_tree *vcm)
{
    if (!key) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }
    if (op<PAO(EQ) || op>=PAO(MAX)) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    struct pcvdom_attr *attr = _attr_create();
    if (!attr) {
        return NULL;
    }

    attr->op = op;

    attr->pre_defined = pchvml_attr_static_search(key, strlen(key));
    if (attr->pre_defined) {
        attr->key = (char*)attr->pre_defined->name;
    } else {
        attr->key = strdup(key);
        if (!attr->key) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            _attr_destroy(attr);
            return NULL;
        }
    }

    attr->val = vcm;

    return attr;
}

void
pcvdom_attr_destroy(struct pcvdom_attr *attr)
{
    if (!attr)
        return;

    PC_ASSERT(attr->parent==NULL);

    _attr_destroy(attr);
}

// doc/dom construction api
int
pcvdom_document_append_content(struct pcvdom_document *doc,
        struct pcvdom_content *content)
{
    if (!doc || !content || content->node.node.parent) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    bool b = pctree_node_append_child(&doc->node.node, &content->node.node);
    PC_ASSERT(b);

    return 0;
}

int
pcvdom_document_set_root(struct pcvdom_document *doc,
        struct pcvdom_element *root)
{
    if (!doc || !root || root->node.node.parent) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if (doc->root) {
        pcinst_set_error(PURC_ERROR_DUPLICATED);
        return -1;
    }

    bool b = pctree_node_append_child(&doc->node.node, &root->node.node);
    PC_ASSERT(b);

    doc->root = root;

    return 0;
}


int
pcvdom_document_append_comment(struct pcvdom_document *doc,
        struct pcvdom_comment *comment)
{
    if (!doc || !comment || comment->node.node.parent) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    bool b = pctree_node_append_child(&doc->node.node, &comment->node.node);
    PC_ASSERT(b);

    return 0;
}


int
pcvdom_element_append_attr(struct pcvdom_element *elem,
        struct pcvdom_attr *attr)
{
    if (!elem || !attr || attr->parent || !attr->key) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    PC_ASSERT(elem->attrs);

    int r;
    r = pcutils_map_find_replace_or_insert(elem->attrs,
            attr->key, attr, NULL);
    PC_ASSERT(r==0);

    attr->parent = elem;

    return 0;
}

int
pcvdom_element_append_element(struct pcvdom_element *elem,
        struct pcvdom_element *child)
{
    if (!elem || !child || child->node.node.parent) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    bool b = pctree_node_append_child(&elem->node.node, &child->node.node);
    PC_ASSERT(b);

    return 0;
}

int
pcvdom_element_append_content(struct pcvdom_element *elem,
        struct pcvdom_content *child)
{
    if (!elem || !child || child->node.node.parent) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    bool b = pctree_node_append_child(&elem->node.node, &child->node.node);
    PC_ASSERT(b);

    return 0;
}

int
pcvdom_element_append_comment(struct pcvdom_element *elem,
        struct pcvdom_comment *child)
{
    if (!elem || !child || child->node.node.parent) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    bool b = pctree_node_append_child(&elem->node.node, &child->node.node);
    PC_ASSERT(b);

    return 0;
}

// key = vcm
// or
// key,    in case when vcm == NULL
int
pcvdom_element_set_attr_simple(struct pcvdom_element *elem,
        const char *key, struct pcvcm_tree *vcm);

int
// for modification operators, such as +=|-=|%=|~=|^=|$=
pcvdom_element_set_attr(struct pcvdom_element *elem,
        const char *key, enum pcvdom_attr_op, const char *statement);

// accessor api
struct pcvdom_node*
pcvdom_node_parent(struct pcvdom_node *node)
{
    if (!node || !node->node.parent)
        return NULL;

    return container_of(node->node.parent, struct pcvdom_node, node);
}

struct pcvdom_element*
pcvdom_element_parent(struct pcvdom_element *elem)
{
    if (!elem || !elem->node.node.parent)
        return NULL;

    struct pcvdom_node *node;
    node = container_of(elem->node.node.parent, struct pcvdom_node, node);

    return container_of(node, struct pcvdom_element, node);
}

struct pcvdom_element*
pcvdom_content_parent(struct pcvdom_content *content)
{
    if (!content || !content->node.node.parent)
        return NULL;

    struct pcvdom_node *node;
    node = container_of(content->node.node.parent, struct pcvdom_node, node);

    return container_of(node, struct pcvdom_element, node);
}

struct pcvdom_element*
pcvdom_comment_parent(struct pcvdom_comment *comment)
{
    if (!comment || !comment->node.node.parent)
        return NULL;

    struct pcvdom_node *node;
    node = container_of(comment->node.node.parent, struct pcvdom_node, node);

    return container_of(node, struct pcvdom_element, node);
}

const char*
pcvdom_element_get_tagname(struct pcvdom_element *elem);

struct pcvdom_attr*
pcvdom_element_get_attr_c(struct pcvdom_element *elem,
        const char *key);

// operation api
void
pcvdom_node_remove(struct pcvdom_node *node)
{
    if (!node)
        return;

    _vdom_node_remove(node);
}

void
pcvdom_node_destroy(struct pcvdom_node *node)
{
    if (!node)
        return;

    _vdom_node_destroy(node);
}



// traverse all vdom_node
typedef int (*vdom_node_traverse_f)(struct pcvdom_node *top,
    struct pcvdom_node *node, void *ctx);
int pcvdom_node_traverse(struct pcvdom_node *node, void *ctx,
        vdom_node_traverse_f cb);

// traverse all element
typedef int (*vdom_element_traverse_f)(struct pcvdom_element *top,
    struct pcvdom_element *elem, void *ctx);
int pcvdom_element_traverse(struct pcvdom_element *elem, void *ctx,
        vdom_element_traverse_f cb);

static void
_document_reset(struct pcvdom_document *doc)
{
    int r;

    free(doc->doctype);
    doc->doctype = NULL;

    while (doc->node.node.first_child) {
        fprintf(stderr, "=============\n");
        struct pcvdom_node *node;
        node = container_of(doc->node.node.first_child, struct pcvdom_node, node);
        pctree_node_remove(doc->node.node.first_child);
        pcvdom_node_destroy(node);
    }

    if (doc->variables) {
        r = pcutils_map_destroy(doc->variables);
        PC_ASSERT(r==0);
        doc->variables = NULL;
    }
}

static void
_document_destroy(struct pcvdom_document *doc)
{
    _document_reset(doc);
    PC_ASSERT(doc->node.node.first_child == NULL);
    free(doc);
}

static void*
_document_copy_key(const void *key)
{
    return (void*)key;
}

static void
_document_free_key(void *key)
{
    UNUSED_PARAM(key);
}

static void*
_document_copy_val(const void *val)
{
    purc_variant_t var = (purc_variant_t)val;
    purc_variant_ref(var);
    return var;
}

static int
_document_comp_key(const void *key1, const void *key2)
{
    const char *s1 = (const char*)key1;
    const char *s2 = (const char*)key2;

    return strcmp(s1, s2);
}

static void
_document_free_val(void *val)
{
    purc_variant_t var = (purc_variant_t)val;
    purc_variant_unref(var);
}

static void
_document_remove_child(struct pcvdom_node *me, struct pcvdom_node *child)
{
    struct pcvdom_document *doc;
    doc = container_of(me, struct pcvdom_document, node);

    if (child == &doc->root->node) {
        doc->root = NULL;
    }

    pctree_node_remove(&child->node);
}

static struct pcvdom_document*
_document_create(const char *doctype)
{
    struct pcvdom_document *doc;
    doc = (struct pcvdom_document*)calloc(1, sizeof(*doc));
    if (!doc) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    doc->node.type = VDT(DOCUMENT);
    doc->node.remove_child = _document_remove_child;

    doc->doctype = strdup(doctype);
    if (!doc->doctype) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        _document_destroy(doc);
        return NULL;
    }

    doc->variables = pcutils_map_create(_document_copy_key, _document_free_key,
        _document_copy_val, _document_free_val,
        _document_comp_key, false); // non-thread-safe
    if (!doc->variables) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        _document_destroy(doc);
        return NULL;
    }

    return doc;
}

static void
_element_reset(struct pcvdom_element *elem)
{
    int r;

    if (elem->tag_id==VTT(_UNDEF) && elem->tag_name) {
        free(elem->tag_name);
    }
    elem->tag_name = NULL;

    while (elem->node.node.first_child) {
        struct pcvdom_node *node;
        node = container_of(elem->node.node.first_child, struct pcvdom_node, node);
        pctree_node_remove(elem->node.node.first_child);
        pcvdom_node_destroy(node);
    }

    if (elem->attrs) {
        r = pcutils_map_destroy(elem->attrs);
        PC_ASSERT(r==0);
        elem->attrs = NULL;
    }

    if (elem->variables) {
        r = pcutils_map_destroy(elem->variables);
        PC_ASSERT(r==0);
        elem->variables = NULL;
    }
}

static void
_element_destroy(struct pcvdom_element *elem)
{
    _element_reset(elem);
    PC_ASSERT(elem->node.node.first_child == NULL);
    free(elem);
}

static void*
_element_attr_copy_key(const void *key)
{
    return (void*)key;
}

static void
_element_attr_free_key(void *key)
{
    UNUSED_PARAM(key);
}

static void*
_element_attr_copy_val(const void *val)
{
    return (void*)val;
}

static int
_element_attr_comp_key(const void *key1, const void *key2)
{
    const char *s1 = (const char*)key1;
    const char *s2 = (const char*)key2;

    return strcmp(s1, s2);
}

static void
_element_attr_free_val(void *val)
{
    struct pcvdom_attr *attr = (struct pcvdom_attr*)val;
    attr->parent = NULL;
    _attr_destroy(attr);
}

static void*
_element_copy_key(const void *key)
{
    return (void*)key;
}

static void
_element_free_key(void *key)
{
    UNUSED_PARAM(key);
}

static void*
_element_copy_val(const void *val)
{
    return (void*)val;
}

static int
_element_comp_key(const void *key1, const void *key2)
{
    const char *s1 = (const char*)key1;
    const char *s2 = (const char*)key2;

    return strcmp(s1, s2);
}

static void
_element_free_val(void *val)
{
    UNUSED_PARAM(val);    
}

static struct pcvdom_element*
_element_create(void)
{
    struct pcvdom_element *elem;
    elem = (struct pcvdom_element*)calloc(1, sizeof(*elem));
    if (!elem) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    elem->node.type = VDT(ELEMENT);
    elem->node.remove_child = NULL;

    elem->tag_id    = VTT(_UNDEF);

    elem->attrs = pcutils_map_create(_element_attr_copy_key, _element_attr_free_key,
        _element_attr_copy_val, _element_attr_free_val,
        _element_attr_comp_key, false); // non-thread-safe
    if (!elem->attrs) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        _element_destroy(elem);
        return NULL;
    }

    elem->variables = pcutils_map_create(_element_copy_key, _element_free_key,
        _element_copy_val, _element_free_val,
        _element_comp_key, false); // non-thread-safe
    if (!elem->variables) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        _element_destroy(elem);
        return NULL;
    }

    return elem;
}

static void
_content_reset(struct pcvdom_content *content)
{
    if (content->vcm) {
        // destroy vcm
        content->vcm = NULL;
    }
}

static void
_content_destroy(struct pcvdom_content *content)
{
    _content_reset(content);
    PC_ASSERT(content->node.node.first_child == NULL);
    free(content);
}

static struct pcvdom_content*
_content_create(struct pcvcm_tree *vcm)
{
    struct pcvdom_content *content;
    content = (struct pcvdom_content*)calloc(1, sizeof(*content));
    if (!content) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    content->node.type = VDT(CONTENT);
    content->node.remove_child = NULL;

    content->vcm = vcm;

    return content;
}

static void
_comment_reset(struct pcvdom_comment *comment)
{
    if (comment->text) {
        free(comment->text);
        comment->text = NULL;
    }
}

static void
_comment_destroy(struct pcvdom_comment *comment)
{
    _comment_reset(comment);
    PC_ASSERT(comment->node.node.first_child == NULL);
    free(comment);
}

static struct pcvdom_comment*
_comment_create(const char *text)
{
    struct pcvdom_comment *comment;
    comment = (struct pcvdom_comment*)calloc(1, sizeof(*comment));
    if (!comment) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    comment->node.type = VDT(COMMENT);
    comment->node.remove_child = NULL;

    comment->text = strdup(text);
    if (!comment->text) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        _comment_destroy(comment);
        return NULL;
    }

    return comment;
}

static void
_attr_reset(struct pcvdom_attr *attr)
{
    if (attr->pre_defined==NULL) {
        free(attr->key);
    }
    attr->pre_defined = NULL;
    attr->key = NULL;

    // destroy val
    attr->val = NULL;
}

static void
_attr_destroy(struct pcvdom_attr *attr)
{
    PC_ASSERT(attr->parent==NULL);
    _attr_reset(attr);
    free(attr);
}

static struct pcvdom_attr*
_attr_create(void)
{
    struct pcvdom_attr *attr;
    attr = (struct pcvdom_attr*)calloc(1, sizeof(*attr));
    if (!attr) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    return attr;
}

static void
_vdom_node_remove(struct pcvdom_node *node)
{
    struct pcvdom_node *parent = pcvdom_node_parent(node);
    if (!parent)
        return;

    parent->remove_child(parent, node);
}

static void
_vdom_node_destroy(struct pcvdom_node *node)
{
    if (!node)
        return;

    switch (node->type)
    {
        case VDT(DOCUMENT):
            {
                struct pcvdom_document *doc;
                doc = container_of(node, struct pcvdom_document, node);
                _document_destroy(doc);
            } break;
        case VDT(ELEMENT):
            {
                struct pcvdom_element *elem;
                elem = container_of(node, struct pcvdom_element, node);
                _element_destroy(elem);
            } break;
        case VDT(CONTENT):
            {
                struct pcvdom_content *content;
                content = container_of(node, struct pcvdom_content, node);
                _content_destroy(content);
            } break;
        case VDT(COMMENT):
            {
                struct pcvdom_comment *comment;
                comment = container_of(node, struct pcvdom_comment, node);
                _comment_destroy(comment);
            } break;
        default:
            {
                PC_ASSERT(0);
            } break;
    }
}

