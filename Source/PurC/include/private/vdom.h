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

enum pchvml_attr_operator {
    PCHVML_ATTRIBUTE_OPERATOR,                //  =
    PCHVML_ATTRIBUTE_ADDITION_OPERATOR,       // +=
    PCHVML_ATTRIBUTE_SUBTRACTION_OPERATOR,    // -=
    PCHVML_ATTRIBUTE_ASTERISK_OPERATOR,       // *=
    PCHVML_ATTRIBUTE_REGEX_OPERATOR,          // /=
    PCHVML_ATTRIBUTE_PRECISE_OPERATOR,        // %=
    PCHVML_ATTRIBUTE_REPLACE_OPERATOR,        // ~=
    PCHVML_ATTRIBUTE_HEAD_OPERATOR,           // ^=
    PCHVML_ATTRIBUTE_TAIL_OPERATOR,           // $=
    PCHVML_ATTRIBUTE_MAX,
};

struct pcvdom_node;
struct pcvdom_document;
struct pcvdom_element;
typedef struct pcvdom_element     pcvdom_element;
typedef struct pcvdom_element    *pcvdom_element_t;
struct pcvdom_content;
struct pcvdom_comment;
typedef enum pchvml_tag_id   pcvdom_tag_id;
struct pcvdom_attr;

struct pcintr_stack;

static inline struct pcvdom_node *
pcvdom_doc_cast_to_node(struct pcvdom_document *doc)
{
    return (struct pcvdom_node *)doc;
}

static inline struct pcvdom_node *
pcvdom_ele_cast_to_node(struct pcvdom_element *doc)
{
    return (struct pcvdom_node *)doc;
}

struct pcvdom_document*
pcvdom_document_ref(struct pcvdom_document *doc);

void
pcvdom_document_unref(struct pcvdom_document *doc);

struct pcvdom_document*
pcvdom_document_create(void);

struct pcvdom_element*
pcvdom_element_create(pcvdom_tag_id tag);

struct pcvdom_element*
pcvdom_element_create_c(const char *tag_name);

struct pcvdom_content*
pcvdom_content_create(struct pcvcm_node *vcm_content);

struct pcvdom_comment*
pcvdom_comment_create(const char *text);

// for modification operators, such as +=|-=|%=|~=|^=|$=
struct pcvdom_attr*
pcvdom_attr_create(const char *key, enum pchvml_attr_operator op,
    struct pcvcm_node *vcm);

// key = vcm
// or
// key,    in case when vcm == NULL
struct pcvdom_attr*
pcvdom_attr_create_simple(const char *key, struct pcvcm_node *vcm);

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

struct pcvdom_element*
pcvdom_document_get_root(struct pcvdom_document *doc);

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

int
pcvdom_element_set_vcm_content(struct pcvdom_element *elem,
        struct pcvcm_node *vcm_content);

// accessor api
struct pcvdom_node*
pcvdom_node_from_document(struct pcvdom_document *doc);

struct pcvdom_node*
pcvdom_node_from_element(struct pcvdom_element *elem);

struct pcvdom_node*
pcvdom_node_from_content(struct pcvdom_content *content);

struct pcvdom_node*
pcvdom_node_from_comment(struct pcvdom_comment *comment);

struct pcvdom_document*
pcvdom_document_from_node(struct pcvdom_node *node);

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

struct pcvdom_element*
pcvdom_element_first_child_element(struct pcvdom_element *elem);

struct pcvdom_element*
pcvdom_element_last_child_element(struct pcvdom_element *elem);

struct pcvdom_element*
pcvdom_element_next_sibling_element(struct pcvdom_element *elem);

struct pcvdom_element*
pcvdom_element_prev_sibling_element(struct pcvdom_element *elem);

bool
pcvdom_element_is_foreign(struct pcvdom_element *element);

bool
pcvdom_element_is_hvml_native(struct pcvdom_element *element);

bool
pcvdom_element_is_hvml_operation(struct pcvdom_element *element);

struct pcvdom_attr*
pcvdom_element_find_attr(struct pcvdom_element *element, const char *key);

bool
pcvdom_element_is_silently(struct pcvdom_element *element);

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
        if (pcvdom_document_set_doctype(doc, name, doctype)) {    \
            pcvdom_document_unref(doc);                           \
            doc = NULL;                                           \
        }                                                         \
    }                                                             \
    doc; })

purc_variant_t
pcvdom_element_eval_attr_val(struct pcintr_stack* stack,
        pcvdom_element_t element, const char *key);

struct pcvdom_pos {
    uint32_t        c;
    int             line;
    int             col;
    int             pos;
};

struct pcvdom_document*
pcvdom_util_document_from_stream(purc_rwstream_t in,
        struct pcvdom_pos *pos);

struct pcvdom_document*
pcvdom_util_document_from_buf(const unsigned char *buf, size_t len,
        struct pcvdom_pos *pos);

struct pcvdom_element*
pcvdom_util_document_parse_fragment(purc_rwstream_t in,
        struct pcvdom_pos *pos);

struct pcvdom_element*
pcvdom_util_document_parse_fragment_buf(const unsigned char *buf, size_t len,
        struct pcvdom_pos *pos);

enum pcvdom_util_node_serialize_opt {
    PCVDOM_UTIL_NODE_SERIALIZE__UNDEF,
    PCVDOM_UTIL_NODE_SERIALIZE_INDENT,
};

typedef int
(*pcvdom_util_node_serialize_cb)(const char *buf, size_t len, void *ctxt);

void
pcvdom_util_node_serialize_ex(struct pcvdom_node *node,
        enum pcvdom_util_node_serialize_opt opt,
        pcvdom_util_node_serialize_cb cb, void *ctxt);

void
pcvdom_util_node_serialize(struct pcvdom_node *node,
        pcvdom_util_node_serialize_cb cb, void *ctxt);

int
pcvdom_util_fprintf(const char *buf, size_t len, void *ctxt);

purc_variant_t
pcvdom_tokenwised_eval_attr(enum pchvml_attr_operator op,
        purc_variant_t l, purc_variant_t r);

#define PRINT_VDOM_NODE(_node)      \
    pcvdom_util_node_serialize(_node, pcvdom_util_fprintf, NULL)

PCA_EXTERN_C_END

#endif  /* PURC_PRIVATE_VDOM_H */
