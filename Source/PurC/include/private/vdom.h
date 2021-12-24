/**
 * @file vdom.h
 * @author Xu Xiaohong
 * @date 2021/08/23
 * @brief The internal interfaces for vdom.
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

#ifndef PURC_PRIVATE_VDOM_H
#define PURC_PRIVATE_VDOM_H

#include "config.h"

#include "purc.h"

#include "purc-macros.h"
#include "purc-errors.h"
#include "purc-variant.h"
#include "private/list.h"
#include "private/tree.h"
#include "private/map.h"
#include "private/vcm.h"
#include "private/var-mgr.h"

#include "hvml-tag.h"

PCA_EXTERN_C_BEGIN

enum pcvdom_nodetype {
    PCVDOM_NODE_DOCUMENT,
    PCVDOM_NODE_ELEMENT,
    PCVDOM_NODE_CONTENT,
    PCVDOM_NODE_COMMENT,
};

enum pchvml_attr_assignment {
    PCHVML_ATTRIBUTE_ASSIGNMENT,             //  =
    PCHVML_ATTRIBUTE_ADDITION_ASSIGNMENT,    // +=
    PCHVML_ATTRIBUTE_SUBTRACTION_ASSIGNMENT, // -=
    PCHVML_ATTRIBUTE_REMAINDER_ASSIGNMENT,   // %=
    PCHVML_ATTRIBUTE_REPLACE_ASSIGNMENT,     // ~=
    PCHVML_ATTRIBUTE_HEAD_ASSIGNMENT,        // ^=
    PCHVML_ATTRIBUTE_TAIL_ASSIGNMENT,        // $=
    PCHVML_ATTRIBUTE_REGEX_ASSIGNMENT,        // /=
    PCHVML_ATTRIBUTE_MAX,
};

#define PCVDOM_NODE_IS_DOCUMENT(_n) \
    (((_n) && (_n)->type==PCVDOM_NODE_DOCUMENT))
#define PCVDOM_NODE_IS_ELEMENT(_n) \
    (((_n) && (_n)->type==PCVDOM_NODE_ELEMENT))
#define PCVDOM_NODE_IS_CONTENT(_n) \
    (((_n) && (_n)->type==PCVDOM_NODE_CONTENT))
#define PCVDOM_NODE_IS_COMMENT(_n) \
    (((_n) && (_n)->type==PCVDOM_NODE_COMMENT))

#define PCVDOM_DOCUMENT_FROM_NODE(_node) \
    (PCVDOM_NODE_IS_DOCUMENT(_node) ? \
        container_of(_node, struct pcvdom_document, node) : NULL)
#define PCVDOM_ELEMENT_FROM_NODE(_node) \
    (PCVDOM_NODE_IS_ELEMENT(_node) ? \
        container_of(_node, struct pcvdom_element, node) : NULL)
#define PCVDOM_CONTENT_FROM_NODE(_node) \
    (PCVDOM_NODE_IS_CONTENT(_node) ? \
        container_of(_node, struct pcvdom_content, node) : NULL)
#define PCVDOM_COMMENT_FROM_NODE(_node) \
    (PCVDOM_NODE_IS_COMMENT(_node) ? \
        container_of(_node, struct pcvdom_comment, node) : NULL)

struct pcvdom_node;
struct pcvdom_document;
struct pcvdom_element;
typedef struct pcvdom_element     pcvdom_element;
typedef struct pcvdom_element    *pcvdom_element_t;
struct pcvdom_content;
struct pcvdom_comment;
typedef enum pchvml_tag_id   pcvdom_tag_id;
struct pcvdom_attr;

struct pcvdom_node {
    struct pctree_node     node;
    enum pcvdom_nodetype   type;
    void (*remove_child)(struct pcvdom_node *me, struct pcvdom_node *child);
};

struct pcvdom_doctype {
    char                   *name;
    char                   *tag_prefix;
    char                   *system_info;
};

struct pcvdom_dvobj_hvml {
    char                   *url;
    unsigned long int      maxIterationCount;
    unsigned long int      maxRecursionDepth;
    struct timespec        timeout;
};

struct pcvdom_document {
    struct pcvdom_node      node;

    struct pcvdom_doctype   doctype;

    // redundant, for fast access
    struct pcvdom_element  *root;
    struct pcvdom_element  *head;
    struct pcvdom_element  *body;

    // document-variables
    // such as `$REQUEST`、`$TIMERS`、`$T` and etc.
    pcvarmgr_list_t         variables;

    // for dvobj hvml
    struct pcvdom_dvobj_hvml dvobj_hvml;

    unsigned int            quirks:1;
};

struct pcvdom_attr {
    struct pcvdom_element    *parent;

    // NOTE for key:
    //   for those pre-defined attrs, static char * in pre_defined
    //   for others, need to be free'd afterwards
    const struct pchvml_attr_entry  *pre_defined;
    char                     *key;

    // operator
    enum pchvml_attr_assignment       op;

    // text/jsonnee/no-value
    struct pcvcm_node        *val;
};

struct pcvdom_element {
    struct pcvdom_node      node;

    // for those non-pre-defined tags(UNDEF)
    // tag_name shall be free'd afterwards in case when tag_id is tag(UNDEF)
    pcvdom_tag_id           tag_id;
    char                   *tag_name;

    // key: char *, the same as struct pcvdom_attr:key
    // val: struct pcvdom_attr*
    struct pcutils_map     *attrs;

    // for those wrapped in `archetype`
    struct pcvcm_node      *vcm_content;

    // FIXME: scoped-variables
    //  for those `defined` in `init`、`bind`、`connect`、`load`、`define`
    pcvarmgr_list_t         variables;
};

struct pcvdom_content {
    struct pcvdom_node      node;

    char                   *text;
};

struct pcvdom_comment {
    struct pcvdom_node      node;

    char                   *text;
};

struct purc_vdom {
    struct pcvdom_document          *document;
};

// creating and destroying api
void
pcvdom_document_destroy(struct pcvdom_document *doc);

struct pcvdom_document*
pcvdom_document_create(void);

struct pcvdom_element*
pcvdom_element_create(pcvdom_tag_id tag);

struct pcvdom_element*
pcvdom_element_create_c(const char *tag_name);

struct pcvdom_content*
pcvdom_content_create(const char *text);

struct pcvdom_comment*
pcvdom_comment_create(const char *text);

// for modification operators, such as +=|-=|%=|~=|^=|$=
struct pcvdom_attr*
pcvdom_attr_create(const char *key, enum pchvml_attr_assignment op,
    struct pcvcm_node *vcm);

// key = vcm
// or
// key,    in case when vcm == NULL
static inline struct pcvdom_attr*
pcvdom_attr_create_simple(const char *key, struct pcvcm_node *vcm)
{
    return pcvdom_attr_create(key, PCHVML_ATTRIBUTE_ASSIGNMENT, vcm);
}

void
pcvdom_attr_destroy(struct pcvdom_attr *attr);

// doc/dom construction api
int
pcvdom_document_set_doctype(struct pcvdom_document *doc,
        const char *name, const char *doctype);

int
pcvdom_document_append_content(struct pcvdom_document *doc,
        struct pcvdom_content *content);

int
pcvdom_document_set_root(struct pcvdom_document *doc,
        struct pcvdom_element *root);

int
pcvdom_document_append_comment(struct pcvdom_document *doc,
        struct pcvdom_comment *comment);

// build-in variable : DOC, TIMERS
bool
pcvdom_document_bind_variable(purc_vdom_t vdom, const char *name,
        purc_variant_t variant);

bool
pcvdom_document_unbind_variable(purc_vdom_t vdom, const char *name);

purc_variant_t
pcvdom_document_get_variable(purc_vdom_t vdom, const char *name);

int
pcvdom_element_append_attr(struct pcvdom_element *elem,
        struct pcvdom_attr *attr);

int
pcvdom_element_append_element(struct pcvdom_element *elem,
        struct pcvdom_element *child);

int
pcvdom_element_append_content(struct pcvdom_element *elem,
        struct pcvdom_content *child);

int
pcvdom_element_append_comment(struct pcvdom_element *elem,
        struct pcvdom_comment *child);

int
pcvdom_element_set_vcm_content(struct pcvdom_element *elem,
        struct pcvcm_node *vcm_content);

// custom variable : init、bind、connect、load、define
bool
pcvdom_element_bind_variable(struct pcvdom_element *elem,
        const char *name, purc_variant_t variant);

bool
pcvdom_element_unbind_variable(struct pcvdom_element *elem,
        const char *name);

purc_variant_t
pcvdom_element_get_variable(struct pcvdom_element *elem,
        const char *name);

// accessor api
struct pcvdom_node*
pcvdom_node_parent(struct pcvdom_node *node);

struct pcvdom_node*
pcvdom_node_first_child(struct pcvdom_node *node);

struct pcvdom_node*
pcvdom_node_last_child(struct pcvdom_node *node);

struct pcvdom_node*
pcvdom_node_next_sibling(struct pcvdom_node *node);

struct pcvdom_node*
pcvdom_node_prev_sibling(struct pcvdom_node *node);

struct pcvdom_element*
pcvdom_element_parent(struct pcvdom_element *elem);

static inline struct pcvdom_element*
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

static inline struct pcvdom_element*
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

static inline struct pcvdom_element*
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

static inline struct pcvdom_element*
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

struct pcvdom_attr*
pcvdom_element_find_attr(struct pcvdom_element *element, const char *key);

struct pcvdom_element*
pcvdom_content_parent(struct pcvdom_content *content);

struct pcvdom_element*
pcvdom_comment_parent(struct pcvdom_comment *comment);

const char*
pcvdom_element_get_tagname(struct pcvdom_element *elem);

struct pcvdom_attr*
pcvdom_element_get_attr_c(struct pcvdom_element *elem,
        const char *key);

// operation api
void pcvdom_node_remove(struct pcvdom_node *node);

void pcvdom_node_destroy(struct pcvdom_node *node);

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

#define pcvdom_document_create_with_doctype(name, doctype) ({     \
    struct pcvdom_document *doc = pcvdom_document_create();       \
    if (doc) {                                                    \
        if (pcvdom_document_set_doctype(doc,name, doctype)) {     \
            pcvdom_document_destroy(doc);                         \
            doc = NULL;                                           \
        }                                                         \
    }                                                             \
    doc; })

purc_variant_t
pcvdom_element_eval_attr_val(pcvdom_element_t element, const char *key);

PCA_EXTERN_C_END

#endif  /* PURC_PRIVATE_VDOM_H */

