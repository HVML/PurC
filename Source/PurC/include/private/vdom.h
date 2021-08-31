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

#include "purc-macros.h"
#include "purc-errors.h"
#include "purc-variant.h"
#include "private/list.h"
#include "private/tree.h"
#include "private/map.h"

#include "hvml-tag.h"

PCA_EXTERN_C_BEGIN

enum pcvdom_nodetype {
    PCVDOM_NODE_DOCUMENT,
    PCVDOM_NODE_ELEMENT,
    PCVDOM_NODE_CONTENT,
    PCVDOM_NODE_COMMENT,
};

enum pcvdom_attr_op {
    PCVDOM_ATTR_OP_EQ,                        /*  = */
    PCVDOM_ATTR_OP_ADD,                       /* += */
    PCVDOM_ATTR_OP_DEL,                       /* -= */
    PCVDOM_ATTR_OP_PATTERN_MATCH_REPLACE,     /* %= */
    PCVDOM_ATTR_OP_REGEX_MATCH_REPLACE,       /* ~= */
    PCVDOM_ATTR_OP_PREPEND,                   /* ^= */
    PCVDOM_ATTR_OP_APPEND,                    /* $= */
    PCVDOM_ATTR_OP_MAX
};

#define PCVDOM_NODE_IS_DOCUMENT(_n) \
    (((_n) && (_n)->type==PCVDOM_NODE_DOCUMENT))
#define PCVDOM_NODE_IS_ELEMENT(_n) \
    (((_n) && (_n)->type==PCVDOM_NODE_ELEMENT))
#define PCVDOM_NODE_IS_CONTENT(_n) \
    (((_n) && (_n)->type==PCVDOM_VDOM_CONTENT))
#define PCVDOM_NODE_IS_COMMENT(_n) \
    (((_n) && (_n)->type==PCVDOM_VDOM_COMMENT))
#define PCVDOM_NODE_IS_EXP(_n) \
    (((_n) && (_n)->type==PCVDOM_VDOM_EXP))

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
struct pcvdom_content;
struct pcvdom_comment;
typedef enum pchvml_tag_id   pcvdom_tag_id;
struct pcvdom_attr;

// TODO: replace with vcm.h
struct pcvcm_tree;
typedef struct pcvcm_tree* pcvcm_tree_t;

struct pcvdom_node {
    struct pctree_node     node;
    enum pcvdom_nodetype   type;
    void (*remove_child)(struct pcvdom_node *me, struct pcvdom_node *child);
};

struct pcvdom_document {
    struct pcvdom_node      node;

    // doctype field
    char                   *doctype;

    // redundant, for fast access
    struct pcvdom_element  *root;

    // document-variables
    // such as `$REQUEST`、`$TIMERS`、`$T` and etc.
    pcutils_map            *variables;
};

struct pcvdom_attr {
    struct pcvdom_element    *parent;

    // NOTE for key:
    //   for those pre-defined attrs, static char * in pre_defined
    //   for others, need to be free'd afterwards
    const struct pchvml_attr_entry  *pre_defined;
    char                     *key;

    // operator
    enum pcvdom_attr_op       op;

    // text/jsonnee/no-value
    struct pcvcm_tree        *val;
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

    // FIXME: scoped-variables
    //        for those `defined` in `init`
    pcutils_map            *variables;
};

struct pcvdom_content {
    struct pcvdom_node      node;

    struct pcvcm_tree      *vcm;
};

struct pcvdom_comment {
    struct pcvdom_node      node;

    char                   *text;
};


// creating and destroying api
void
pcvdom_document_destroy(struct pcvdom_document *doc);

struct pcvdom_document*
pcvdom_document_create(const char *doctype);

struct pcvdom_element*
pcvdom_element_create(pcvdom_tag_id tag);

struct pcvdom_element*
pcvdom_element_create_c(const char *tag_name);

struct pcvdom_content*
pcvdom_content_create(struct pcvcm_tree *vcm);

struct pcvdom_comment*
pcvdom_comment_create(const char *text);

// key = vcm
// or
// key,    in case when vcm == NULL
struct pcvdom_attr*
pcvdom_attr_create_simple(const char *key, struct pcvcm_tree *vcm);

// for modification operators, such as +=|-=|%=|~=|^=|$=
struct pcvdom_attr*
pcvdom_attr_create(const char *key, enum pcvdom_attr_op op,
    struct pcvcm_tree *vcm);

void
pcvdom_attr_destroy(struct pcvdom_attr *attr);

// doc/dom construction api
int
pcvdom_document_append_content(struct pcvdom_document *doc,
        struct pcvdom_content *content);

int
pcvdom_document_set_root(struct pcvdom_document *doc,
        struct pcvdom_element *root);

int
pcvdom_document_append_comment(struct pcvdom_document *doc,
        struct pcvdom_comment *comment);

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
pcvdom_node_parent(struct pcvdom_node *node);

struct pcvdom_element*
pcvdom_element_parent(struct pcvdom_element *elem);

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

PCA_EXTERN_C_END

#endif  /* PURC_PRIVATE_VDOM_H */

