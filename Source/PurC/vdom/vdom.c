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
#include "private/stringbuilder.h"

#include "hvml-attr.h"

#include "vdom-internal.h"

#include <math.h>
#include <regex.h>

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
content_create(struct pcvcm_node *vcm_content);

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

struct pcvdom_document*
pcvdom_document_ref(struct pcvdom_document *doc)
{
    assert(doc);

    atomic_fetch_add(&doc->refc, 1);
    return doc;
}

void
pcvdom_document_unref(struct pcvdom_document *doc)
{
    assert(doc);

    unsigned long refc = atomic_fetch_sub(&doc->refc, 1);
    if (refc <= 2) {
        document_destroy(doc);
    }
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
pcvdom_content_create(struct pcvcm_node *vcm_content)
{
    if (!vcm_content) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    return content_create(vcm_content);
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
pcvdom_attr_create(const char *key, enum pchvml_attr_operator op,
    struct pcvcm_node *vcm)
{
    if (!key) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }
    if (op<PAO(OPERATOR) || op>=PAO(MAX)) {
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

struct pcvdom_element*
pcvdom_document_get_root(struct pcvdom_document *doc)
{
    PC_ASSERT(doc);
    return doc->root;
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
    r = pcutils_array_push(elem->attrs, attr);
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

int
pcvdom_element_set_vcm_content(struct pcvdom_element *elem,
        struct pcvcm_node *vcm_content)
{
    int ret = -1;
    bool b = false;
    struct pcvdom_content *content = NULL;
    enum pcvcm_node_type type;

    if (!elem || !vcm_content) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    type = vcm_content->type;
    if (type == PCVCM_NODE_TYPE_STRING) {
        struct pcvdom_node *last_child = pcvdom_node_last_child(&elem->node);
        struct pcvdom_content *last_content = PCVDOM_CONTENT_FROM_NODE(last_child);
        if (!last_child || !last_content) {
            goto normal;
        }

        struct pcvcm_node *last_vcm = last_content->vcm;
        if (last_vcm->type == PCVCM_NODE_TYPE_STRING) {
            struct pcvcm_node *cs = pcvcm_node_new_concat_string(0, NULL);
            if (!cs) {
                goto out;
            }
            b = pcvcm_node_append_child(cs, last_vcm);
            if (!b) {
                pcvcm_node_destroy(cs);
                goto out;
            }

            b = pcvcm_node_append_child(cs, vcm_content);
            if (!b) {
                pcvcm_node_destroy(cs);
                goto out;
            }

            last_content->vcm = cs;
            ret = 0;
            goto out;
        }
        else if (last_vcm->type == PCVCM_NODE_TYPE_FUNC_CONCAT_STRING) {
            if (pcvcm_node_append_child(last_vcm, vcm_content)) {
                ret = 0;
            }
            goto out;
        }
    }

normal:
    content = content_create(vcm_content);
    if (!content) {
        goto out;
    }

    if (pctree_node_append_child(&elem->node.node, &content->node.node)) {
        ret = 0;
    }

out:
    return ret;
}

// accessor api
struct pcvdom_node*
pcvdom_node_parent(struct pcvdom_node *node)
{
    if (!node || !node->node.parent) {
        return NULL;
    }

    return container_of(node->node.parent, struct pcvdom_node, node);
}

struct pcvdom_node*
pcvdom_node_first_child(struct pcvdom_node *node)
{
    if (!node || !node->node.first_child) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    return container_of(node->node.first_child, struct pcvdom_node, node);
}

struct pcvdom_node*
pcvdom_node_last_child(struct pcvdom_node *node)
{
    if (!node || !node->node.last_child) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    return container_of(node->node.last_child, struct pcvdom_node, node);
}

struct pcvdom_node*
pcvdom_node_next_sibling(struct pcvdom_node *node)
{
    if (!node || !node->node.next) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    return container_of(node->node.next, struct pcvdom_node, node);
}

struct pcvdom_node*
pcvdom_node_prev_sibling(struct pcvdom_node *node)
{
    if (!node || !node->node.prev) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    return container_of(node->node.prev, struct pcvdom_node, node);
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

    struct pcvdom_attr* attr = pcvdom_element_find_attr(elem, key);

    if (!attr) {
        pcinst_set_error(PURC_ERROR_NOT_EXISTS);
        return NULL;
    }

    return attr;
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

    PC_ASSERT(node->type != VDT(DOCUMENT));

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

struct serialize_data {
    struct pcvdom_node                 *top;
    int                                 is_doc;
    enum pcvdom_util_node_serialize_opt opt;
    size_t                              level;
    pcvdom_util_node_serialize_cb       cb;
    void                               *ctxt;
};

static void
document_serialize(struct pcvdom_document *doc, int level, int push,
        struct serialize_data *ud)
{
    UNUSED_PARAM(doc);
    UNUSED_PARAM(ud);
    if (ud->opt & PCVDOM_UTIL_NODE_SERIALIZE_INDENT) {
        for (int i=0; i<level; ++i) {
            ud->cb("  ", 2, ud->ctxt);
        }
    }
    if (!push)
        return;

    ud->cb("<!DOCTYPE", 9, ud->ctxt);
    struct pcvdom_doctype  *doctype = &doc->doctype;
    const char *name = doctype->name;
    const char *system_info = doctype->system_info;
    if (!name)
        name = "html";
    if (!system_info)
        system_info = "";

    ud->cb(" ", 1, ud->ctxt);
    ud->cb(name, strlen(name), ud->ctxt);
    ud->cb(" ", 1, ud->ctxt);

    ud->cb("SYSTEM \"", 8, ud->ctxt);
    ud->cb(system_info, strlen(system_info), ud->ctxt);
    ud->cb("\"", 1, ud->ctxt);

    ud->cb(">", 1, ud->ctxt);
}

static int
attr_serialize(void *key, void *val, void *ctxt, bool is_operation)
{
    const char *sk = (const char*)key;
    struct pcvdom_attr *attr = (struct pcvdom_attr*)val;
    struct serialize_data *ud = (struct serialize_data*)ctxt;
    PC_ASSERT(sk == attr->key);
    enum pchvml_attr_operator  op  = attr->op;
    struct pcvcm_node         *v = attr->val;

    ud->cb(" ", 1, ud->ctxt);
    ud->cb(sk, strlen(sk), ud->ctxt);
    if (!v) {
        PC_ASSERT(op == PCHVML_ATTRIBUTE_OPERATOR);
        return 0;
    }

    switch (op) {
        case PCHVML_ATTRIBUTE_OPERATOR:
            if (is_operation) {
                ud->cb(" ", 1, ud->ctxt);
            }
            else {
                ud->cb("=", 1, ud->ctxt);
            }
            break;
        case PCHVML_ATTRIBUTE_ADDITION_OPERATOR:
            ud->cb("+=", 2, ud->ctxt);
            break;
        case PCHVML_ATTRIBUTE_SUBTRACTION_OPERATOR:
            ud->cb("-=", 2, ud->ctxt);
            break;
        case PCHVML_ATTRIBUTE_ASTERISK_OPERATOR:
            ud->cb("*=", 2, ud->ctxt);
            break;
        case PCHVML_ATTRIBUTE_REGEX_OPERATOR:
            ud->cb("/=", 2, ud->ctxt);
            break;
        case PCHVML_ATTRIBUTE_PRECISE_OPERATOR:
            ud->cb("%=", 2, ud->ctxt);
            break;
        case PCHVML_ATTRIBUTE_REPLACE_OPERATOR:
            ud->cb("~=", 2, ud->ctxt);
            break;
        case PCHVML_ATTRIBUTE_HEAD_OPERATOR:
            ud->cb("^=", 2, ud->ctxt);
            break;
        case PCHVML_ATTRIBUTE_TAIL_OPERATOR:
            ud->cb("$=", 2, ud->ctxt);
            break;
        default:
            PC_ASSERT(0);
            break;
    }

    size_t len;
    char *s = pcvcm_node_serialize(v, &len);
    if (!s) {
        ud->cb("{{OOM}}", 7, ud->ctxt);
        return 0;
    }

    ud->cb(s, len, ud->ctxt);
    free(s);

    return 0;
}

static void
element_serialize(struct pcvdom_element *element, int level, int push,
        struct serialize_data *ud)
{
    UNUSED_PARAM(level);
    UNUSED_PARAM(push);
    UNUSED_PARAM(ud);

    if (ud->opt & PCVDOM_UTIL_NODE_SERIALIZE_INDENT) {
        ud->cb("\n", 1, ud->ctxt);

        for (int i=0; i<level; ++i) {
            ud->cb("  ", 2, ud->ctxt);
        }
    }

    char *tag_name = element->tag_name;
    bool self_closing = element->self_closing;
    bool is_operation = pcvdom_element_is_hvml_operation(element);

    if (push) {
        ud->cb("<", 1, ud->ctxt);
        ud->cb(tag_name, strlen(tag_name), ud->ctxt);

        size_t nr = pcutils_array_length(element->attrs);
        for (size_t i = 0; i < nr; i++) {
            struct pcvdom_attr *attr = pcutils_array_get(element->attrs, i);
            attr_serialize(attr->key, attr, ud, is_operation);
        }

        if (self_closing) {
            ud->cb("/", 1, ud->ctxt);
        }
        ud->cb(">", 1, ud->ctxt);
    }
    else if (!self_closing) {
        ud->cb("</", 2, ud->ctxt);
        ud->cb(tag_name, strlen(tag_name), ud->ctxt);
        ud->cb(">", 1, ud->ctxt);
    }
}

static void
content_serialize(struct pcvdom_content *content, int level, int push,
        struct serialize_data *ud)
{
    UNUSED_PARAM(content);
    UNUSED_PARAM(level);
    UNUSED_PARAM(push);
    UNUSED_PARAM(ud);

    if (!push) {
        return;
    }

    struct pcvcm_node *vcm = content->vcm;
    if (!vcm) {
        goto out;
    }

    size_t len;
    char *s = pcvcm_node_serialize(vcm, &len);
    if (s) {
        ud->cb(s, len, ud->ctxt);
        free(s);
    }

out:
    return;
}

static void
comment_serialize(struct pcvdom_comment *comment, int level, int push,
        struct serialize_data *ud)
{
    UNUSED_PARAM(comment);
    UNUSED_PARAM(level);
    UNUSED_PARAM(push);
    UNUSED_PARAM(ud);
}

static int
vdom_node_serialize(struct pcvdom_node *node, int level,
        int push, struct serialize_data *ud)
{
    switch (node->type)
    {
        case VDT(DOCUMENT):
            {
                struct pcvdom_document *doc;
                doc = container_of(node, struct pcvdom_document, node);
                document_serialize(doc, level, push, ud);
            } break;
        case VDT(ELEMENT):
            {
                struct pcvdom_element *elem;
                elem = container_of(node, struct pcvdom_element, node);
                element_serialize(elem, level, push, ud);
            } break;
        case VDT(CONTENT):
            {
                struct pcvdom_content *content;
                content = container_of(node, struct pcvdom_content, node);
                content_serialize(content, level, push, ud);
            } break;
        case VDT(COMMENT):
            {
                struct pcvdom_comment *comment;
                comment = container_of(node, struct pcvdom_comment, node);
                comment_serialize(comment, level, push, ud);
            } break;
        default:
            {
                PC_ASSERT(0);
            } break;
    }

    return 0;
}

static int
node_serialize(struct pctree_node *node, int level,
        int push, void *ctxt)
{
    struct serialize_data *ud = (struct serialize_data*)ctxt;
    if (ud->is_doc && node != &ud->top->node) {
        --level;
    }
    struct pcvdom_node *vdom_node;
    vdom_node = container_of(node, struct pcvdom_node, node);
    return vdom_node_serialize(vdom_node, level, push, ud);
}

void
pcvdom_util_node_serialize_ex(struct pcvdom_node *node,
        enum pcvdom_util_node_serialize_opt opt, bool serialize_children,
        pcvdom_util_node_serialize_cb cb, void *ctxt)
{
    struct serialize_data ud = {
        .top        = node,
        .is_doc     = node->type == PCVDOM_NODE_DOCUMENT,
        .opt        = opt,
        .cb         = cb,
        .ctxt       = ctxt,
    };

    if (serialize_children) {
        pctree_node_walk(&node->node, 0, node_serialize, &ud);
    }
    else {
        node_serialize(&node->node, 0, 1, &ud);
//        node_serialize(&node->node, 0, 0, &ud);
    }
    cb("\n", 1, ctxt);

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
    doctype_reset(&doc->doctype);

    pcutils_arrlist_free(doc->bodies);
    doc->bodies = NULL;

    while (doc->node.node.first_child) {
        struct pcvdom_node *node;
        node = container_of(doc->node.node.first_child, struct pcvdom_node, node);
        pctree_node_remove(doc->node.node.first_child);
        pcvdom_node_destroy(node);
    }
}

static void
document_destroy(struct pcvdom_document *doc)
{
    document_reset(doc);
    PC_ASSERT(doc->node.node.first_child == NULL);
    free(doc);
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

    doc->bodies = pcutils_arrlist_new_ex(NULL, 4);
    if (!doc->bodies) {
        free(doc);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    doc->node.type = VDT(DOCUMENT);
    doc->node.remove_child = document_remove_child;

    doc->refc = 1;

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
        size_t nr = pcutils_array_length(elem->attrs);
        for (size_t i = 0; i < nr; i++) {
            struct pcvdom_attr *attr = pcutils_array_get(elem->attrs, i);
            attr->parent = NULL;
            attr_destroy(attr);
        }
        pcutils_array_destroy(elem->attrs, true);
        elem->attrs = NULL;
    }
}

static void
element_destroy(struct pcvdom_element *elem)
{
    element_reset(elem);
    PC_ASSERT(elem->node.node.first_child == NULL);
    free(elem);
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

    elem->attrs = pcutils_array_create();
    if (!elem->attrs) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        element_destroy(elem);
        return NULL;
    }

    // FIXME:
    // if (pcintr_get_stack() == NULL)
    //     return elem;

    return elem;
}

static void
content_reset(struct pcvdom_content *content)
{
    if (content->vcm) {
        pcvcm_node_destroy(content->vcm);
        content->vcm= NULL;
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
content_create(struct pcvcm_node *vcm_content)
{
    struct pcvdom_content *content;
    content = (struct pcvdom_content*)calloc(1, sizeof(*content));
    if (!content) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    content->node.type = VDT(CONTENT);
    content->node.remove_child = NULL;

    content->vcm = vcm_content;

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

    if (parent->remove_child)
        parent->remove_child(parent, node);
    else
        pctree_node_remove(&node->node);
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

static inline enum pchvml_tag_category
pcvdom_element_categories(struct pcvdom_element *element)
{
    PC_ASSERT(element);
    enum pchvml_tag_id tag_id = element->tag_id;

    const struct pchvml_tag_entry *entry;
    entry = pchvml_tag_static_get_by_id(tag_id);

    if (entry == NULL) {
        return PCHVML_TAGCAT__UNDEF;
    }

    return entry->cats;
}

bool
pcvdom_element_is_foreign(struct pcvdom_element *element)
{
    enum pchvml_tag_category cats;
    cats = pcvdom_element_categories(element);
    return cats & PCHVML_TAGCAT_FOREIGN;
}

bool
pcvdom_element_is_hvml_native(struct pcvdom_element *element)
{
    enum pchvml_tag_category cats;
    cats = pcvdom_element_categories(element);
    return cats & (PCHVML_TAGCAT_TEMPLATE | PCHVML_TAGCAT_VERB);
}

bool
pcvdom_element_is_hvml_operation(struct pcvdom_element *element)
{
    enum pchvml_tag_category cats;
    cats = pcvdom_element_categories(element);
    return cats & PCHVML_TAGCAT_VERB;
}

struct pcvdom_attr*
pcvdom_element_find_attr(struct pcvdom_element *element, const char *key)
{
    struct pcvdom_attr *attr = NULL;
    if (PCVDOM_NODE_IS_DOCUMENT(&element->node)) {
        goto out;
    }

    size_t nr = pcutils_array_length(element->attrs);
    for (size_t i = 0; i < nr; i++) {
        struct pcvdom_attr *v = pcutils_array_get(element->attrs, i);
        if (strcmp(v->key, key) == 0) {
            attr = v;
            break;
        }
    }

out:
    return attr;
}

purc_variant_t
pcvdom_element_eval_attr_val(pcintr_stack_t stack, pcvdom_element_t element,
        const char *key)
{
    struct pcvdom_attr *attr;
    attr = pcvdom_element_find_attr(element, key);
    if (!attr)
        return purc_variant_make_undefined();

    struct pcvcm_node           *val = attr->val;

    purc_variant_t v;
    v = pcvcm_eval(val, stack, pcvdom_element_is_silently(element));
#if 0 // VW
    PC_ASSERT(v != PURC_VARIANT_INVALID);

    enum pchvml_attr_operator  op  = attr->op;
    UNUSED_PARAM(op);
    PC_ASSERT(0); // FIXME: how to use op????
#endif

    return v;
}

#define SILENTLY_ATTR_NAME          "silently"
#define SILENTLY_ATTR_FULL_NAME     "hvml:silently"
#define MUST_YIELD_ATTR_NAME        "must-yield"
#define MUST_YIELD_ATTR_FULL_NAME   "hvml:must-yield"

bool
pcvdom_element_is_silently(struct pcvdom_element *element)
{
    return pcvdom_element_find_attr(element, SILENTLY_ATTR_NAME) ||
        pcvdom_element_find_attr(element, SILENTLY_ATTR_FULL_NAME);
}

bool
pcvdom_element_is_must_yield(struct pcvdom_element *element)
{
    return pcvdom_element_find_attr(element, MUST_YIELD_ATTR_NAME) ||
        pcvdom_element_find_attr(element, MUST_YIELD_ATTR_FULL_NAME);
}

static double
_round(double x, int p)
{
    if (x != 0.0) {
        double c = pow((double)10.0, p);
        double l = ((floor((fabs(x) * c) + 0.5)) / c);
        double r = (x / fabs(x));
        return l * r;
    }
    return 0.0;
}

static purc_variant_t
tokenwised_eval_attr_num(enum pchvml_attr_operator op,
        purc_variant_t ll, purc_variant_t rr)
{
    double ld = purc_variant_numerify(ll);
    double rd = purc_variant_numerify(rr);

    switch (op) {
        case PCHVML_ATTRIBUTE_OPERATOR:
            return purc_variant_ref(rr);

        case PCHVML_ATTRIBUTE_ADDITION_OPERATOR:
            return purc_variant_make_number(ld + rd);

        case PCHVML_ATTRIBUTE_SUBTRACTION_OPERATOR:
            return purc_variant_make_number(ld - rd);

        case PCHVML_ATTRIBUTE_ASTERISK_OPERATOR:
            return purc_variant_make_number(ld * rd);

        case PCHVML_ATTRIBUTE_REGEX_OPERATOR:
            switch (rr->type) {
                case PURC_VARIANT_TYPE_ULONGINT:
                    if (rr->u64 == 0) {
                        purc_set_error(PURC_ERROR_DIVBYZERO);
                        return PURC_VARIANT_INVALID;
                    }
                    break;
                case PURC_VARIANT_TYPE_LONGINT:
                    if (rr->u64 == 0) {
                        purc_set_error(PURC_ERROR_DIVBYZERO);
                        return PURC_VARIANT_INVALID;
                    }
                    break;
                default:
                    break;
            }
            return purc_variant_make_number(ld / rd);

        case PCHVML_ATTRIBUTE_PRECISE_OPERATOR:
            {
                // FIXME:
                uint64_t l = (uint64_t)fabs(ld);
                uint64_t r = (uint64_t)fabs(rd);
                if (r == 0) {
                    return purc_variant_make_number(0);
                }
                return purc_variant_make_number(l % r);
            }
            break;

        case PCHVML_ATTRIBUTE_REPLACE_OPERATOR:
            {
                if (rd <= 0) {
                    return purc_variant_make_number(round(ld));
                }
                double e = _round(ld, rd);
                char buf[64];
                long long f = (long long)round(ld);
                int n = snprintf(buf, sizeof(buf), "%lld", f);
                char *ret = gcvt(e, n + fabs(rd), buf);
                if (ret) {
                    return purc_variant_make_string(ret, false);
                }
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                return PURC_VARIANT_INVALID;
            }
            break;

        case PCHVML_ATTRIBUTE_HEAD_OPERATOR:
            return purc_variant_make_number(pow(ld, rd));

        case PCHVML_ATTRIBUTE_TAIL_OPERATOR:
            if (rd != 0.0) {
                return purc_variant_make_number(ld / rd);
            }
            purc_set_error(PURC_ERROR_DIVBYZERO);
            return PURC_VARIANT_INVALID;

        default:
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return PURC_VARIANT_INVALID;
    }

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
tokenwised_eval_attr_str_add(purc_variant_t ll, purc_variant_t rr)
{
    const char *tokens = purc_variant_get_string_const(ll);
    PC_ASSERT(tokens);
    const char *_new = purc_variant_get_string_const(rr);
    PC_ASSERT(_new);
    const size_t len = strlen(_new);

    struct pcutils_token_iterator it;
    it = pcutils_token_it_begin(tokens, tokens + strlen(tokens), NULL);
    struct pcutils_token *token;
    for (token = pcutils_token_it_value(&it);
        token;
        token = pcutils_token_it_next(&it))
    {
        if ((size_t)(token->end - token->start) != len)
            continue;

        if (strncmp(token->start, _new, len))
            continue;

        break;
    }
    pcutils_token_it_end(&it);

    if (token)
        return purc_variant_ref(ll);

    const size_t sz = strlen(tokens) + 1 + len;
    char *p = malloc(sz + 1);
    if (!p) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }
    strcpy(p, tokens);
    strcat(p, " ");
    strcat(p, _new);

    bool check_encoding = true;
    return purc_variant_make_string_reuse_buff(p, sz + 1, check_encoding);
}

static purc_variant_t
str_to_variant_and_reset(struct pcutils_string *str)
{
    purc_variant_t v;
    bool check_encoding = true;
    if (str->abuf == str->buf) {
        v = purc_variant_make_string(str->buf, check_encoding);
    }
    else {
        v = purc_variant_make_string_reuse_buff(str->abuf,
                str->curr - str->abuf, check_encoding);
        if (v != PURC_VARIANT_INVALID) {
            // NOTE: no need to reset pcutils_string
            return v;
        }
    }

    pcutils_string_reset(str);
    return v;
}

static purc_variant_t
tokenwised_eval_attr_str_sub(purc_variant_t ll, purc_variant_t rr)
{
    const char *tokens = purc_variant_get_string_const(ll);
    PC_ASSERT(tokens);
    const char *_new = purc_variant_get_string_const(rr);
    PC_ASSERT(_new);
    const size_t len = strlen(_new);

    struct pcutils_string str;
    pcutils_string_init(&str, 128);

    struct pcutils_token_iterator it;
    it = pcutils_token_it_begin(tokens, tokens + strlen(tokens), NULL);
    size_t i = 0;
    struct pcutils_token *token;
    for (token = pcutils_token_it_value(&it);
        token;
        token = pcutils_token_it_next(&it))
    {
        if ((size_t)(token->end - token->start) == len &&
                strncmp(token->start, _new, len) == 0)
        {
            continue;
        }

        if (i && pcutils_string_append_chunk(&str, " ", 1)) {
            pcutils_string_reset(&str);
            return PURC_VARIANT_INVALID;
        }

        i++;
        if (pcutils_string_append_chunk(&str,
                    token->start, token->end - token->start))
        {
            pcutils_string_reset(&str);
            return PURC_VARIANT_INVALID;
        }
    }
    pcutils_token_it_end(&it);

    return str_to_variant_and_reset(&str);
}

static purc_variant_t
tokenwised_eval_attr_str_append_or_prepend(purc_variant_t ll,
        purc_variant_t rr)
{
    const char *pattern = purc_variant_get_string_const(rr);
    PC_ASSERT(pattern);

    bool append = true;
    if (*pattern == '^') {
        append = false;
        ++pattern;
        if (!*pattern) {
            return purc_variant_ref(ll);
        }
    }
    const size_t len = strlen(pattern);

    const char *tokens = purc_variant_get_string_const(ll);
    PC_ASSERT(tokens);

    size_t sz = 0;
    struct pcutils_token *token;
    struct pcutils_token_iterator it;
    it = pcutils_token_it_begin(tokens, tokens + strlen(tokens), NULL);
    for (token = pcutils_token_it_value(&it);
        token;
        token = pcutils_token_it_next(&it))
    {
        sz += token->end - token->start + len + 1;
    }
    pcutils_token_it_end(&it);

    struct pcutils_string str;
    pcutils_string_init(&str, sz + 1);

    int r = 0;

    size_t i = 0;
    it = pcutils_token_it_begin(tokens, tokens + strlen(tokens), NULL);
    for (token = pcutils_token_it_value(&it);
        token;
        token = pcutils_token_it_next(&it), ++i)
    {
        if (i) {
            r = pcutils_string_append_chunk(&str, " ", 1);
            if (r)
                break;
        }
        if (append) {
            r = pcutils_string_append_chunk(&str,
                    token->start, token->end - token->start);
            if (r)
                break;

            r = pcutils_string_append_chunk(&str, pattern, len);
            if (r)
                break;
        }
        else {
            r = pcutils_string_append_chunk(&str, pattern, len);
            if (r)
                break;

            r = pcutils_string_append_chunk(&str,
                    token->start, token->end - token->start);
            if (r)
                break;
        }
    }
    pcutils_token_it_end(&it);

    if (r) {
        pcutils_string_reset(&str);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    return str_to_variant_and_reset(&str);
}

static int
split_pattern_replace(const char *tokens,
        const char **pattern, size_t *nr1,
        const char **replace, size_t *nr2)
{
    struct pcutils_token *token;
    struct pcutils_token_iterator it;
    it = pcutils_token_it_begin(tokens, tokens + strlen(tokens), NULL);
    do {
        token = pcutils_token_it_value(&it);
        if (!token)
            break;
        *pattern = token->start;
        *nr1 = token->end - token->start;

        token = pcutils_token_it_next(&it);
        if (!token)
            break;

        size_t sz = strlen(token->start);
        *replace = pcutils_trim_blanks(token->start, &sz);
        *nr2 = sz;

        pcutils_token_it_end(&it);
        return 0;
    } while (0);
    pcutils_token_it_end(&it);

    return -1;
}

static int
split_re_replace(const char *tokens, regex_t *re,
        const char **replace, size_t *nr)
{
    int r;

    struct pcutils_string pattern;
    pcutils_string_init(&pattern, strlen(tokens) + 1);

    const char *p = tokens;
    while (*p && purc_isspace(*p))
        ++p;

    if (*p != '/') {
        purc_set_error(PURC_ERROR_INVALID_OPERAND);
        return -1;
    }

    r = 0;
    for (++p; 1; ++p) {
        char c = *p;
        if (!c) {
            purc_set_error(PURC_ERROR_INVALID_OPERAND);
            r = -1;
            break;
        }

        if (c == '/')
            break;

        if (c == '\\') {
            ++p;
            c = *p;
            if (!c) {
                purc_set_error(PURC_ERROR_INVALID_OPERAND);
                r = -1;
                break;
            }
        }

        r = pcutils_string_append_chunk(&pattern, &c, 1);
        if (r) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            break;
        }
    }

    if (r) {
        pcutils_string_reset(&pattern);
        return -1;
    }

    r = regcomp(re, pattern.abuf, REG_EXTENDED|REG_NOSUB);
    pcutils_string_reset(&pattern);
    if (r) {
        purc_set_error(PURC_ERROR_INVALID_OPERAND);
        return -1;
    }

    ++p;
    *replace = p;
    *nr = strlen(p);

    return 0;
}

static purc_variant_t
tokenwised_eval_attr_str_regex_re_replace(purc_variant_t ll,
        regex_t *re, const char *replace, size_t nr)
{
    int r;

    const char *tokens = purc_variant_get_string_const(ll);
    PC_ASSERT(tokens);
    const size_t tokens_len = strlen(tokens);
    const size_t chunk_size = tokens_len < 128 ? 128 : tokens_len + 1;

    struct pcutils_string str;
    pcutils_string_init(&str, chunk_size);

    struct pcutils_string buf;
    pcutils_string_init(&buf, chunk_size);

    size_t idx = 0;
    struct pcutils_token *token;
    struct pcutils_token_iterator it;
    it = pcutils_token_it_begin(tokens, tokens + strlen(tokens), NULL);
    for (token = pcutils_token_it_value(&it);
        token;
        token = pcutils_token_it_next(&it), ++idx)
    {
        pcutils_string_clear(&buf);

        r = pcutils_string_append_chunk(&buf,
                token->start, token->end - token->start);
        if (r)
            break;

        const char *p;
        size_t n;
        r = regexec(re, buf.abuf, 0, NULL, 0);
        if (r) {
            r = 0;
            p = token->start;
            n = token->end - token->start;
        }
        else {
            p = replace;
            n = nr;
        }

        if (idx)
            r = pcutils_string_append_chunk(&str, " ", 1);

        if (r == 0) {
            r = pcutils_string_append_chunk(&str, p, n);
        }

        if (r)
            break;
    }
    pcutils_token_it_end(&it);

    pcutils_string_reset(&buf);

    if (r) {
        pcutils_string_reset(&str);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    return str_to_variant_and_reset(&str);
}

static purc_variant_t
tokenwised_eval_attr_str_regex_replace(purc_variant_t ll,
        purc_variant_t rr)
{
    int r;

    const char *s = purc_variant_get_string_const(rr);
    PC_ASSERT(s);

    regex_t re;
    const char *replace;
    size_t nr;
    r = split_re_replace(s, &re, &replace, &nr);
    if (r)
        return PURC_VARIANT_INVALID;

    purc_variant_t v;
    v = tokenwised_eval_attr_str_regex_re_replace(ll, &re, replace, nr);
    regfree(&re);

    return v;
}

static purc_variant_t
tokenwised_eval_attr_str_regex_pattern_replace(purc_variant_t ll,
        const char *pattern, size_t sz, const char *replace, size_t nr)
{
    int r = 0;

    const char *tokens = purc_variant_get_string_const(ll);
    PC_ASSERT(tokens);
    const size_t tokens_len = strlen(tokens);
    const size_t chunk_size = tokens_len < 128 ? 128 : tokens_len + 1;

    struct pcutils_string str;
    pcutils_string_init(&str, chunk_size);

    size_t idx = 0;
    struct pcutils_token *token;
    struct pcutils_token_iterator it;
    it = pcutils_token_it_begin(tokens, tokens + strlen(tokens), NULL);
    for (token = pcutils_token_it_value(&it);
        token;
        token = pcutils_token_it_next(&it), ++idx)
    {
        const char *p;
        size_t n;

        if (strncmp(token->start, pattern, sz) == 0 &&
                (size_t)(token->end - token->start) == sz)
        {
            p = replace;
            n = nr;
        }
        else
        {
            p = token->start;
            n = token->end - token->start;
        }

        if (idx)
            r = pcutils_string_append_chunk(&str, " ", 1);

        if (r == 0) {
            r = pcutils_string_append_chunk(&str, p, n);
        }

        if (r)
            break;
    }
    pcutils_token_it_end(&it);

    if (r) {
        pcutils_string_reset(&str);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    return str_to_variant_and_reset(&str);
}

static purc_variant_t
tokenwised_eval_attr_str_replace(purc_variant_t ll,
        purc_variant_t rr)
{
    int r;

    const char *s = purc_variant_get_string_const(rr);
    PC_ASSERT(s);

    const char *pattern;
    size_t sz;
    const char *replace;
    size_t nr;
    r = split_pattern_replace(s, &pattern, &sz, &replace, &nr);
    if (r)
        return PURC_VARIANT_INVALID;

    purc_variant_t v;
    v = tokenwised_eval_attr_str_regex_pattern_replace(ll,
            pattern, sz, replace, nr);
    return v;
}

static purc_variant_t
tokenwised_eval_attr_str_wildcard_wildcard_replace(purc_variant_t ll,
        struct pcutils_wildcard *wildcard, const char *replace, size_t nr)
{
    int r = 0;

    const char *tokens = purc_variant_get_string_const(ll);
    PC_ASSERT(tokens);
    const size_t tokens_len = strlen(tokens);
    const size_t chunk_size = tokens_len < 128 ? 128 : tokens_len + 1;

    struct pcutils_string str;
    pcutils_string_init(&str, chunk_size);

    size_t idx = 0;
    struct pcutils_token *token;
    struct pcutils_token_iterator it;
    it = pcutils_token_it_begin(tokens, tokens + strlen(tokens), NULL);
    for (token = pcutils_token_it_value(&it);
        token;
        token = pcutils_token_it_next(&it), ++idx)
    {
        bool matched;
        r = pcutils_wildcard_match(wildcard,
                token->start, token->end - token->start, &matched);
        if (r)
            break;

        const char *p;
        size_t n;

        if (matched) {
            p = replace;
            n = nr;
        }
        else {
            p = token->start;
            n = token->end - token->start;
        }

        if (idx)
            r = pcutils_string_append_chunk(&str, " ", 1);

        if (r == 0) {
            r = pcutils_string_append_chunk(&str, p, n);
        }

        if (r)
            break;
    }
    pcutils_token_it_end(&it);

    if (r) {
        pcutils_string_reset(&str);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    return str_to_variant_and_reset(&str);
}

static purc_variant_t
tokenwised_eval_attr_str_wildcard_replace(purc_variant_t ll,
        purc_variant_t rr)
{
    int r;

    const char *s = purc_variant_get_string_const(rr);
    PC_ASSERT(s);

    const char *pattern;
    size_t sz;
    const char *replace;
    size_t nr;
    r = split_pattern_replace(s, &pattern, &sz, &replace, &nr);
    if (r)
        return PURC_VARIANT_INVALID;

    char *t = strndup(pattern, sz);
    if (!t) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    struct pcutils_wildcard *wildcard;
    wildcard = pcutils_wildcard_create(t, sz);
    free(t);
    t = NULL;
    if (!wildcard) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v;
    v = tokenwised_eval_attr_str_wildcard_wildcard_replace(ll,
            wildcard, replace, nr);
    pcutils_wildcard_destroy(wildcard);

    return v;
}

static purc_variant_t
tokenwised_eval_attr_str_prepend_or_append(purc_variant_t ll,
        purc_variant_t rr, bool append)
{
    int r;

    const char *s = purc_variant_get_string_const(rr);
    PC_ASSERT(s);
    const size_t len = strlen(s);

    const char *tokens = purc_variant_get_string_const(ll);
    PC_ASSERT(tokens);

    struct pcutils_string str;
    pcutils_string_init(&str, 128);

    size_t idx = 0;
    struct pcutils_token *token;
    struct pcutils_token_iterator it;
    it = pcutils_token_it_begin(tokens, tokens + strlen(tokens), NULL);
    for (token = pcutils_token_it_value(&it);
        token;
        token = pcutils_token_it_next(&it), ++idx)
    {
        if (idx) {
            r = pcutils_string_append_chunk(&str, " ", 1);
            if (r)
                break;
        }

        if (append) {
            r = pcutils_string_append_chunk(&str,
                    token->start, token->end - token->start);
            if (r)
                break;

            r = pcutils_string_append_chunk(&str, s, len);
        }
        else {
            r = pcutils_string_append_chunk(&str, s, len);
            if (r)
                break;

            r = pcutils_string_append_chunk(&str,
                    token->start, token->end - token->start);
        }

        if (r)
            break;
    }
    pcutils_token_it_end(&it);

    return str_to_variant_and_reset(&str);
}

static purc_variant_t
tokenwised_eval_attr_str(enum pchvml_attr_operator op,
        purc_variant_t ll, purc_variant_t rr)
{
    switch (op) {
        case PCHVML_ATTRIBUTE_OPERATOR:
            // =
            return purc_variant_ref(rr);

        case PCHVML_ATTRIBUTE_ADDITION_OPERATOR:
            // +=
            return tokenwised_eval_attr_str_add(ll, rr);

        case PCHVML_ATTRIBUTE_SUBTRACTION_OPERATOR:
            // -=
            return tokenwised_eval_attr_str_sub(ll, rr);

        case PCHVML_ATTRIBUTE_ASTERISK_OPERATOR:
            // *=
            return tokenwised_eval_attr_str_append_or_prepend(ll, rr);

        case PCHVML_ATTRIBUTE_REGEX_OPERATOR:
            // /=
            return tokenwised_eval_attr_str_regex_replace(ll, rr);

        case PCHVML_ATTRIBUTE_PRECISE_OPERATOR:
            // %=
            return tokenwised_eval_attr_str_replace(ll, rr);

        case PCHVML_ATTRIBUTE_REPLACE_OPERATOR:
            // ~=
            return tokenwised_eval_attr_str_wildcard_replace(ll, rr);

        case PCHVML_ATTRIBUTE_HEAD_OPERATOR:
            // ^=
            return tokenwised_eval_attr_str_prepend_or_append(ll, rr, false);

        case PCHVML_ATTRIBUTE_TAIL_OPERATOR:
            // $=
            return tokenwised_eval_attr_str_prepend_or_append(ll, rr, true);

        default:
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return PURC_VARIANT_INVALID;
    }
}

purc_variant_t
pcvdom_tokenwised_eval_attr(enum pchvml_attr_operator op,
        purc_variant_t ll, purc_variant_t rr)
{
    if (ll == PURC_VARIANT_INVALID || rr == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if (pcvariant_is_of_number(rr)) {
        return tokenwised_eval_attr_num(op, ll, rr);
    }
    else if (purc_variant_is_string(rr)) {
        return tokenwised_eval_attr_str(op, ll, rr);
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }
}

struct pcvdom_element*
pcvdom_element_first_child_element(struct pcvdom_element *elem)
{
    if (!elem)
        return NULL;
    struct pcvdom_node *node = pcvdom_node_first_child(&elem->node);

    while (node && !PCVDOM_NODE_IS_ELEMENT(node)) {
        node = pcvdom_node_next_sibling(node);
    }

    if (!node)
        return NULL;

    return container_of(node, struct pcvdom_element, node);
}

struct pcvdom_element*
pcvdom_element_last_child_element(struct pcvdom_element *elem)
{
    if (!elem)
        return NULL;
    struct pcvdom_node *node = pcvdom_node_last_child(&elem->node);

    while (node && !PCVDOM_NODE_IS_ELEMENT(node)) {
        node = pcvdom_node_prev_sibling(node);
    }

    if (!node)
        return NULL;

    return container_of(node, struct pcvdom_element, node);
}

struct pcvdom_element*
pcvdom_element_next_sibling_element(struct pcvdom_element *elem)
{
    if (!elem)
        return NULL;
    struct pcvdom_node *node = pcvdom_node_next_sibling(&elem->node);

    while (node && !PCVDOM_NODE_IS_ELEMENT(node)) {
        node = pcvdom_node_next_sibling(node);
    }

    if (!node)
        return NULL;

    return container_of(node, struct pcvdom_element, node);
}

struct pcvdom_element*
pcvdom_element_prev_sibling_element(struct pcvdom_element *elem)
{
    if (!elem)
        return NULL;
    struct pcvdom_node *node = pcvdom_node_prev_sibling(&elem->node);

    while (node && !PCVDOM_NODE_IS_ELEMENT(node)) {
        node = pcvdom_node_prev_sibling(node);
    }

    if (!node)
        return NULL;

    return container_of(node, struct pcvdom_element, node);
}

void
pcvdom_util_node_serialize(struct pcvdom_node *node,
        pcvdom_util_node_serialize_cb cb, void *ctxt)
{
    enum pcvdom_util_node_serialize_opt opt;
    opt = PCVDOM_UTIL_NODE_SERIALIZE_INDENT;
    pcvdom_util_node_serialize_ex(node, opt, true, cb, ctxt);
}

void
pcvdom_util_node_serialize_alone(struct pcvdom_node *node,
        pcvdom_util_node_serialize_cb cb, void *ctxt)
{
    enum pcvdom_util_node_serialize_opt opt;
    opt = PCVDOM_UTIL_NODE_SERIALIZE__UNDEF;
    pcvdom_util_node_serialize_ex(node, opt, false, cb, ctxt);
}

int
pcvdom_util_fprintf(const char *buf, size_t len, void *ctxt)
{
    UNUSED_PARAM(ctxt);
    PC_WARN("%.*s", (int)len, buf);
    return 0;
}

struct pcvdom_attr*
pcvdom_attr_create_simple(const char *key, struct pcvcm_node *vcm)
{
    return pcvdom_attr_create(key, PCHVML_ATTRIBUTE_OPERATOR, vcm);
}

struct pcvdom_node*
pcvdom_node_from_document(struct pcvdom_document *doc)
{
    PC_ASSERT(doc);
    return &doc->node;
}

struct pcvdom_node*
pcvdom_node_from_element(struct pcvdom_element *elem)
{
    PC_ASSERT(elem);
    return &elem->node;
}

struct pcvdom_node*
pcvdom_node_from_content(struct pcvdom_content *content)
{
    PC_ASSERT(content);
    return &content->node;
}

struct pcvdom_node*
pcvdom_node_from_comment(struct pcvdom_comment *comment)
{
    PC_ASSERT(comment);
    return &comment->node;
}

struct pcvdom_document*
pcvdom_document_from_node(struct pcvdom_node *node)
{
    while (node) {
        struct pcvdom_node *parent;
        parent = pcvdom_node_parent(node);
        if (parent) {
            node = parent;
            continue;
        }
        PC_ASSERT(node->type == VDT(DOCUMENT));
        return container_of(node, struct pcvdom_document, node);
    }

    return NULL;
}

