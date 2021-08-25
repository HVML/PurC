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

#include "purc-errors.h"
#include "purc-variant.h"
#include "private/list.h"
#include "private/tree.h"
#include "private/map.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define PURC_ERROR_VDOM PURC_ERROR_FIRST_VDOM

typedef enum {
    PCHVML_STATUS_OK                       = PURC_ERROR_OK,
    PCHVML_STATUS_ERROR                    = PURC_ERROR_UNKNOWN,
    PCHVML_STATUS_ERROR_MEMORY_ALLOCATION  = PURC_ERROR_OUT_OF_MEMORY,
    PCHVML_STATUS_ERROR_OBJECT_IS_NULL     = PURC_ERROR_NULL_OBJECT,
    PCHVML_STATUS_ERROR_SMALL_BUFFER       = PURC_ERROR_TOO_SMALL_BUFF,
    PCHVML_STATUS_ERROR_TOO_SMALL_SIZE     = PURC_ERROR_TOO_SMALL_SIZE,
    PCHVML_STATUS_ERROR_INCOMPLETE_OBJECT  = PURC_ERROR_INCOMPLETE_OBJECT,
    PCHVML_STATUS_ERROR_NO_FREE_SLOT       = PURC_ERROR_NO_FREE_SLOT,
    PCHVML_STATUS_ERROR_NOT_EXISTS         = PURC_ERROR_NOT_EXISTS,
    PCHVML_STATUS_ERROR_WRONG_ARGS         = PURC_ERROR_WRONG_ARGS,
    PCHVML_STATUS_ERROR_WRONG_STAGE        = PURC_ERROR_WRONG_STAGE,
    PCHVML_STATUS_ERROR_UNEXPECTED_RESULT  = PURC_ERROR_UNEXPECTED_RESULT,
    PCHVML_STATUS_ERROR_UNEXPECTED_DATA    = PURC_ERROR_UNEXPECTED_DATA,
    PCHVML_STATUS_ERROR_OVERFLOW           = PURC_ERROR_OVERFLOW,
    PCHVML_STATUS_CONTINUE                 = PURC_ERROR_FIRST_HVML,
    PCHVML_STATUS_SMALL_BUFFER,
    PCHVML_STATUS_ABORTED,
    PCHVML_STATUS_STOPPED,
    PCHVML_STATUS_NEXT,
    PCHVML_STATUS_STOP,
} pchvml_status_t;

enum pcvdom_node_type {
    PCVDOM_NODE_DOCUMENT,
    PCVDOM_NODE_ELEMENT,
    PCVDOM_VDOM_EXP,
};

#define PCVDOM_NODE_IS_DOCUMENT(_n) \
    (((_n) && (_n)->type==PCVDOM_NODE_DOCUMENT))
#define PCVDOM_NODE_IS_ELEMENT(_n) \
    (((_n) && (_n)->type==PCVDOM_NODE_ELEMENT))
#define PCVDOM_NODE_IS_EXP(_n) \
    (((_n) && (_n)->type==PCVDOM_VDOM_EXP))

#define PCVDOM_DOCUMENT_FROM_NODE(_node) \
    (PCVDOM_NODE_IS_DOCUMENT(_node) ? \
        container_of(_node, pcvdom_document_t, node) : NULL)
#define PCVDOM_ELEMENT_FROM_NODE(_node) \
    (PCVDOM_NODE_IS_ELEMENT(_node) ? \
        container_of(_node, pcvdom_element_t, node) : NULL)
#define PCVDOM_EXP_FROM_NODE(_node) \
    (PCVDOM_NODE_IS_EXP(_node) ? \
        container_of(_node, pcvdom_exp_t, node) : NULL)

enum pcvdom_attr_key_type {
    PCVDOM_ATTR_KEY_TEXT,
    PCVDOM_ATTR_KEY_JSONEE,
};

enum pcvdom_attr_val_type {
    PCVDOM_ATTR_VAL_JSONEE,
    PCVDOM_ATTR_VAL_EJSON,
};

enum pcvdom_exp_anchor_type {
    PCVDOM_EXP_ANCHOR_ATTR_KEY   = 0x01,
    PCVDOM_EXP_ANCHOR_ATTR_VAL   = 0x02,
    PCVDOM_EXP_ANCHOR_ELEM       = 0x10,
};

#define PCVDOM_EXP_PARENT_IS_ATTR(t) ((t) & 0x03)

struct pcvdom_node;
typedef struct pcvdom_node pcvdom_node_t;

struct pcvdom_document;
typedef struct pcvdom_document pcvdom_document_t;

struct pcvdom_doctype;
typedef struct pcvdom_doctype pcvdom_doctype_t;

struct pcvdom_element;
typedef struct pcvdom_element pcvdom_element_t;

struct pcvdom_tag;
typedef struct pcvdom_tag pcvdom_tag_t;

struct pcvdom_attr;
typedef struct pcvdom_attr pcvdom_attr_t;

struct pcvdom_exp;
typedef struct pcvdom_exp pcvdom_exp_t;

struct pcvdom_node {
    struct pctree_node         *node;
    enum pcvdom_node_type       type;
    void (*remove_child)(pcvdom_node_t *me, pcvdom_node_t *child);
};

struct pcvdom_doctype {
    pcvdom_document_t      *parent;

    // optimize later
    char                   *prefix;
    char                  **builtins;
    size_t                  nr_builtins;
    size_t                  sz_builtins;
};

struct pcvdom_document {
    pcvdom_node_t           node;

    pcvdom_doctype_t        doctype;
    pcvdom_element_t       *root; // <hvml>

    // document-variables
    // such as `$_REQUEST`、`$TIMERS`、`$T` and etc.
    pcutils_map            *variables;
};

struct pcvdom_tag {
    pcvdom_element_t       *parent;

    // optimize later with tag_id
    char                   *ns;    // namespace prefix
    char                   *name;  // local name, lower space
};

struct pcvdom_exp {
    enum pcvdom_exp_anchor_type  at;
    union {
        pcvdom_attr_t        *attr;
        pcvdom_element_t     *elem;
    } parent;
    // to do
};

struct pcvdom_attr {
    struct list_head        lh;

    pcvdom_element_t       *parent;

    // text/jsonee
    enum pcvdom_attr_key_type    kt;
    union {
        char               *str;
        pcvdom_exp_t       *exp;
    } key;

    // jsonee/ejson
    enum pcvdom_attr_val_type    vt;
    union {
        pcvdom_exp_t       *exp;
        // for ejson type of value, which means it's `constant`
        // as compared to above `exp`
    } val;
};

struct pcvdom_element {
    pcvdom_node_t           node;

    pcvdom_tag_t            tag;

    // pcvdom_attr_t*
    struct list_head        attrs;

    // element/text content/ejson
    pcvdom_node_t          *first_child;
    pcvdom_node_t          *last_child;

    // FIXME: scoped-variables
    //        for those `defined` in `init`, `set`
    pcutils_map            *variables;
};


// creating and destroying api
void
pcvdom_document_destroy(pcvdom_document_t *doc);

pcvdom_document_t*
pcvdom_document_create(void);

pcvdom_element_t*
pcvdom_element_create(void);

pcvdom_attr_t*
pcvdom_attr_create(void);

pcvdom_exp_t*
pcvdom_exp_create(void);

// doc/dom construction api
int
pcvdom_document_set_root(pcvdom_document_t *doc,
        pcvdom_element_t *root);

int
pcvdom_doctype_set_prefix(pcvdom_doctype_t *doc,
        const char *prefix);
int
pcvdom_doctype_append_builtin(pcvdom_doctype_t *doc,
        const char *builtin);

int
pcvdom_element_append_attr(pcvdom_element_t *elem,
        pcvdom_attr_t *attr);
int
pcvdom_element_append_child(pcvdom_element_t *elem,
        pcvdom_element_t *child);

int
pcvdom_tag_set_ns(pcvdom_tag_t *tag,
        const char *ns);
int
pcvdom_tag_set_name(pcvdom_tag_t *tag,
        const char *name);

int
pcvdom_attr_set_key_c(pcvdom_attr_t *attr,
        const char *key);
int
pcvdom_attr_set_key(pcvdom_attr_t *attr,
        pcvdom_exp_t *key);

int
pcvdom_attr_set_val(pcvdom_attr_t *attr,
        pcvdom_exp_t *val);

// accessor api
pcvdom_node_t*
pcvdom_node_parent(pcvdom_node_t *node);

// operation api
void pcvdom_node_remove(pcvdom_node_t *node);
void pcvdom_node_destroy(pcvdom_node_t *node);

// traverse all vdom_node
typedef int (*vdom_node_traverse_f)(pcvdom_node_t *top,
    pcvdom_node_t *node, void *ctx);
int pcvdom_node_traverse(pcvdom_node_t *node, void *ctx,
        vdom_node_traverse_f cb);

// traverse all element
typedef int (*vdom_element_traverse_f)(pcvdom_element_t *top,
    pcvdom_element_t *elem, void *ctx);
int pcvdom_element_traverse(pcvdom_element_t *elem, void *ctx,
        vdom_element_traverse_f cb);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_VDOM_H */

