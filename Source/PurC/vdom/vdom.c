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
#define PAO(x)     PCHVML_ATTRIBUTE_##x

static void
document_reset(struct pcvdom_document *doc);

static void
document_destroy(struct pcvdom_document *doc);

static struct pcvdom_document*
document_create(void);

static int
document_set_doctype(struct pcvdom_document *doc,
    const char *name, const char *doctype);

static void
element_reset(struct pcvdom_element *elem);

static void
element_destroy(struct pcvdom_element *elem);

static struct pcvdom_element*
element_create(void);

static void
content_reset(struct pcvdom_content *doc);

static void
content_destroy(struct pcvdom_content *doc);

static struct pcvdom_content*
content_create(const char *text);

static void
vdom_node_remove(struct pcvdom_node *node);

static void
comment_reset(struct pcvdom_comment *doc);

static void
comment_destroy(struct pcvdom_comment *doc);

static struct pcvdom_comment*
comment_create(const char *text);

static void
attr_reset(struct pcvdom_attr *doc);

static void
attr_destroy(struct pcvdom_attr *doc);

static struct pcvdom_attr*
attr_create(void);

static void
vdom_node_destroy(struct pcvdom_node *node);

// creating and destroying api
void
pcvdom_document_destroy(struct pcvdom_document *doc)
{
    if (!doc)
        return;

    document_destroy(doc);
}

struct pcvdom_document*
pcvdom_document_create(void)
{
    return document_create();
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

    struct pcvdom_element *elem = element_create();
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
        element_destroy(elem);
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

    struct pcvdom_element *elem = element_create();
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
            element_destroy(elem);
            return NULL;
        }
    }

    return elem;
}

struct pcvdom_content*
pcvdom_content_create(const char *text)
{
    if (!text) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    return content_create(text);
}

struct pcvdom_comment*
pcvdom_comment_create(const char *text)
{
    if (!text) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    return comment_create(text);

}

// for modification operators, such as +=|-=|%=|~=|^=|$=
struct pcvdom_attr*
pcvdom_attr_create(const char *key, enum pchvml_attr_assignment op,
    struct pcvcm_node *vcm)
{
    if (!key) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }
    if (op<PAO(ASSIGNMENT) || op>=PAO(MAX)) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    struct pcvdom_attr *attr = attr_create();
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
            attr_destroy(attr);
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

    attr_destroy(attr);
}

// doc/dom construction api
int
pcvdom_document_set_doctype(struct pcvdom_document *doc,
    const char *name, const char *doctype)
{
    if (!doc || !name || !doctype) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    PC_ASSERT(doc->doctype.name == NULL);

    return document_set_doctype(doc, name, doctype);
}

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

// accessor api
struct pcvdom_node*
pcvdom_node_parent(struct pcvdom_node *node)
{
    if (!node || !node->node.parent) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    return container_of(node->node.parent, struct pcvdom_node, node);
}

struct pcvdom_element*
pcvdom_element_parent(struct pcvdom_element *elem)
{
    if (!elem || !elem->node.node.parent) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    struct pcvdom_node *node;
    node = container_of(elem->node.node.parent, struct pcvdom_node, node);

    return container_of(node, struct pcvdom_element, node);
}

struct pcvdom_element*
pcvdom_content_parent(struct pcvdom_content *content)
{
    if (!content || !content->node.node.parent) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    struct pcvdom_node *node;
    node = container_of(content->node.node.parent, struct pcvdom_node, node);

    return container_of(node, struct pcvdom_element, node);
}

struct pcvdom_element*
pcvdom_comment_parent(struct pcvdom_comment *comment)
{
    if (!comment || !comment->node.node.parent) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    struct pcvdom_node *node;
    node = container_of(comment->node.node.parent, struct pcvdom_node, node);

    return container_of(node, struct pcvdom_element, node);
}

const char*
pcvdom_element_get_tagname(struct pcvdom_element *elem)
{
    if (!elem) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    return elem->tag_name;
}

struct pcvdom_attr*
pcvdom_element_get_attr_c(struct pcvdom_element *elem,
        const char *key)
{
    if (!elem || !key) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    if (!elem->attrs) {
        pcinst_set_error(PURC_ERROR_NO_INSTANCE);
        return NULL;
    }

    pcutils_map_entry *entry;
    entry = pcutils_map_find(elem->attrs, key);

    if (!entry || !entry->val) {
        pcinst_set_error(PURC_ERROR_NOT_EXISTS);
        return NULL;
    }

    return entry->val;
}

// operation api
void
pcvdom_node_remove(struct pcvdom_node *node)
{
    if (!node)
        return;

    vdom_node_remove(node);
}

void
pcvdom_node_destroy(struct pcvdom_node *node)
{
    if (!node)
        return;

    vdom_node_destroy(node);
}



// traverse all vdom_node
struct tree_node_arg {
    struct pcvdom_node       *top;
    void                     *ctx;
    vdom_node_traverse_f      cb;

    int                       abortion;
};

static void
tree_node_cb(struct pctree_node* node,  void* data)
{
    struct tree_node_arg *arg = (struct tree_node_arg*)data;
    if (arg->abortion)
        return;

    struct pcvdom_node *p;
    p = container_of(node, struct pcvdom_node, node);

    int r = arg->cb(arg->top, p, arg->ctx);

    arg->abortion = r;
}

int
pcvdom_node_traverse(struct pcvdom_node *node, void *ctx,
        vdom_node_traverse_f cb)
{
    if (!node || !cb)
        return 0;

    struct tree_node_arg arg = {
        .top        = node,
        .ctx        = ctx,
        .cb         = cb,
        .abortion   = 0,
    };

    pctree_node_pre_order_traversal(&node->node,
        tree_node_cb, &arg);

    return arg.abortion;
}

// traverse all element
struct element_arg {
    struct pcvdom_element    *top;
    void                     *ctx;
    vdom_element_traverse_f   cb;

    int                       abortion;
};

static void
element_cb(struct pctree_node* node,  void* data)
{
    struct element_arg *arg = (struct element_arg*)data;
    if (arg->abortion)
        return;

    struct pcvdom_node *p;
    p = container_of(node, struct pcvdom_node, node);

    if (p->type == VDT(ELEMENT)) {
        struct pcvdom_element *elem;
        elem = container_of(p, struct pcvdom_element, node);
        int r = arg->cb(arg->top, elem, arg->ctx);

        arg->abortion = r;
    }
}

int
pcvdom_element_traverse(struct pcvdom_element *elem, void *ctx,
        vdom_element_traverse_f cb)
{
    if (!elem || !cb)
        return 0;

    struct element_arg arg = {
        .top        = elem,
        .ctx        = ctx,
        .cb         = cb,
        .abortion   = 0,
    };

    pctree_node_pre_order_traversal(&elem->node.node,
        element_cb, &arg);

    return arg.abortion;
}

static inline void
doctype_reset(struct pcvdom_doctype *doctype)
{
    if (doctype->name) {
        free(doctype->name);
        doctype->name = NULL;
    }
    if (doctype->tag_prefix) {
        free(doctype->tag_prefix);
        doctype->tag_prefix = NULL;
    }
    if (doctype->system_info) {
        free(doctype->system_info);
        doctype->system_info = NULL;
    }
}

static void
document_reset(struct pcvdom_document *doc)
{
    int r;

    doctype_reset(&doc->doctype);

    while (doc->node.node.first_child) {
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
document_destroy(struct pcvdom_document *doc)
{
    document_reset(doc);
    PC_ASSERT(doc->node.node.first_child == NULL);
    free(doc);
}

static void*
document_copy_key(const void *key)
{
    return (void*)key;
}

static void
document_free_key(void *key)
{
    UNUSED_PARAM(key);
}

static void*
document_copy_val(const void *val)
{
    purc_variant_t var = (purc_variant_t)val;
    purc_variant_ref(var);
    return var;
}

static int
document_comp_key(const void *key1, const void *key2)
{
    const char *s1 = (const char*)key1;
    const char *s2 = (const char*)key2;

    return strcmp(s1, s2);
}

static void
document_free_val(void *val)
{
    purc_variant_t var = (purc_variant_t)val;
    purc_variant_unref(var);
}

static void
document_remove_child(struct pcvdom_node *me, struct pcvdom_node *child)
{
    struct pcvdom_document *doc;
    doc = container_of(me, struct pcvdom_document, node);

    if (child == &doc->root->node) {
        doc->root = NULL;
    }

    pctree_node_remove(&child->node);
}

static struct pcvdom_document*
document_create(void)
{
    struct pcvdom_document *doc;
    doc = (struct pcvdom_document*)calloc(1, sizeof(*doc));
    if (!doc) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    doc->node.type = VDT(DOCUMENT);
    doc->node.remove_child = document_remove_child;

    doc->variables = pcutils_map_create(document_copy_key, document_free_key,
        document_copy_val, document_free_val,
        document_comp_key, false); // non-thread-safe
    if (!doc->variables) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        document_destroy(doc);
        return NULL;
    }

    return doc;
}

static int
document_set_doctype(struct pcvdom_document *doc,
    const char *name, const char *doctype)
{
    doc->doctype.name = strdup(name);
    if (!doc->doctype.name) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    doc->doctype.system_info = strdup(doctype);
    if (!doc->doctype.system_info) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    return 0;
}

static void
element_reset(struct pcvdom_element *elem)
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
element_destroy(struct pcvdom_element *elem)
{
    element_reset(elem);
    PC_ASSERT(elem->node.node.first_child == NULL);
    free(elem);
}

static void*
element_attr_copy_key(const void *key)
{
    return (void*)key;
}

static void
element_attr_free_key(void *key)
{
    UNUSED_PARAM(key);
}

static void*
element_attr_copy_val(const void *val)
{
    return (void*)val;
}

static int
element_attr_comp_key(const void *key1, const void *key2)
{
    const char *s1 = (const char*)key1;
    const char *s2 = (const char*)key2;

    return strcmp(s1, s2);
}

static void
element_attr_free_val(void *val)
{
    struct pcvdom_attr *attr = (struct pcvdom_attr*)val;
    attr->parent = NULL;
    attr_destroy(attr);
}

static void*
element_copy_key(const void *key)
{
    return (void*)key;
}

static void
element_free_key(void *key)
{
    UNUSED_PARAM(key);
}

static void*
element_copy_val(const void *val)
{
    return (void*)val;
}

static int
element_comp_key(const void *key1, const void *key2)
{
    const char *s1 = (const char*)key1;
    const char *s2 = (const char*)key2;

    return strcmp(s1, s2);
}

static void
element_free_val(void *val)
{
    UNUSED_PARAM(val);    
}

static struct pcvdom_element*
element_create(void)
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

    elem->attrs = pcutils_map_create(element_attr_copy_key, element_attr_free_key,
        element_attr_copy_val, element_attr_free_val,
        element_attr_comp_key, false); // non-thread-safe
    if (!elem->attrs) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        element_destroy(elem);
        return NULL;
    }

    elem->variables = pcutils_map_create(element_copy_key, element_free_key,
        element_copy_val, element_free_val,
        element_comp_key, false); // non-thread-safe
    if (!elem->variables) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        element_destroy(elem);
        return NULL;
    }

    return elem;
}

static void
content_reset(struct pcvdom_content *content)
{
    if (content->text) {
        free(content->text);
        content->text = NULL;
    }
}

static void
content_destroy(struct pcvdom_content *content)
{
    content_reset(content);
    PC_ASSERT(content->node.node.first_child == NULL);
    free(content);
}

static struct pcvdom_content*
content_create(const char *text)
{
    struct pcvdom_content *content;
    content = (struct pcvdom_content*)calloc(1, sizeof(*content));
    if (!content) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    content->node.type = VDT(CONTENT);
    content->node.remove_child = NULL;

    content->text = strdup(text);
    if (!content->text) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        content_destroy(content);
        return NULL;
    }

    return content;
}

static void
comment_reset(struct pcvdom_comment *comment)
{
    if (comment->text) {
        free(comment->text);
        comment->text = NULL;
    }
}

static void
comment_destroy(struct pcvdom_comment *comment)
{
    comment_reset(comment);
    PC_ASSERT(comment->node.node.first_child == NULL);
    free(comment);
}

static struct pcvdom_comment*
comment_create(const char *text)
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
        comment_destroy(comment);
        return NULL;
    }

    return comment;
}

static void
attr_reset(struct pcvdom_attr *attr)
{
    if (attr->pre_defined==NULL) {
        free(attr->key);
    }
    attr->pre_defined = NULL;
    attr->key = NULL;

    pcvcm_node_destroy(attr->val);
    attr->val = NULL;
}

static void
attr_destroy(struct pcvdom_attr *attr)
{
    PC_ASSERT(attr->parent==NULL);
    attr_reset(attr);
    free(attr);
}

static struct pcvdom_attr*
attr_create(void)
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
vdom_node_remove(struct pcvdom_node *node)
{
    struct pcvdom_node *parent = pcvdom_node_parent(node);
    if (!parent)
        return;

    parent->remove_child(parent, node);
}

static void
vdom_node_destroy(struct pcvdom_node *node)
{
    if (!node)
        return;

    switch (node->type)
    {
        case VDT(DOCUMENT):
            {
                struct pcvdom_document *doc;
                doc = container_of(node, struct pcvdom_document, node);
                document_destroy(doc);
            } break;
        case VDT(ELEMENT):
            {
                struct pcvdom_element *elem;
                elem = container_of(node, struct pcvdom_element, node);
                element_destroy(elem);
            } break;
        case VDT(CONTENT):
            {
                struct pcvdom_content *content;
                content = container_of(node, struct pcvdom_content, node);
                content_destroy(content);
            } break;
        case VDT(COMMENT):
            {
                struct pcvdom_comment *comment;
                comment = container_of(node, struct pcvdom_comment, node);
                comment_destroy(comment);
            } break;
        default:
            {
                PC_ASSERT(0);
            } break;
    }
}

