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
#include "private/list.h"
#include "private/tree.h"

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

enum pchvml_dom_node_type {
    PCHVML_DOM_NODE_DOCUMENT,
    PCHVML_DOM_NODE_DOCTYPE,
    PCHVML_DOM_NODE_ELEMENT,
    PCHVML_DOM_NODE_TAG,
    PCHVML_DOM_NODE_ATTR,
    PCHVML_DOM_VDOM_EVAL,
};

struct pchvml_dom_node;
typedef struct pchvml_dom_node pchvml_dom_node_t;

struct pchvml_document;
typedef struct pchvml_document pchvml_document_t;

struct pchvml_document_doctype;
typedef struct pchvml_document_doctype pchvml_document_doctype_t;

struct pchvml_dom_element;
typedef struct pchvml_dom_element pchvml_dom_element_t;

struct pchvml_dom_element_tag;
typedef struct pchvml_dom_element_tag pchvml_dom_element_tag_t;

struct pchvml_dom_element_attr;
typedef struct pchvml_dom_element_attr pchvml_dom_element_attr_t;

struct pchvml_vdom_eval;
typedef struct pchvml_vdom_eval pchvml_vdom_eval_t;

// creating and destroying api
pchvml_document_t*
pchvml_document_create(void);

void
pchvml_document_destroy(pchvml_document_t *doc);

pchvml_document_doctype_t*
pchvml_document_doctype_create(void);

pchvml_dom_element_t*
pchvml_dom_element_create(void);

pchvml_dom_element_tag_t*
pchvml_dom_element_tag_create(void);

pchvml_dom_element_attr_t*
pchvml_dom_element_attr_create(void);

pchvml_vdom_eval_t*
pchvml_vdom_eval_create(void);


// doc/dom construction api
int pchvml_document_set_doctype(pchvml_document_t *doc,
        pchvml_document_doctype_t *doctype);

int pchvml_document_set_root(pchvml_document_t *doc,
        pchvml_dom_element_t *root);

int pchvml_document_doctype_set_prefix(pchvml_document_doctype_t *doc,
        const char *prefix);

int pchvml_document_doctype_append_builtin(pchvml_document_doctype_t *doc,
        const char *builtin);

int pchvml_dom_element_set_tag(pchvml_dom_element_t *elem,
        pchvml_dom_element_tag_t *tag);

int pchvml_dom_element_append_attr(pchvml_dom_element_t *elem,
        pchvml_dom_element_attr_t *attr);

int pchvml_dom_element_append_child(pchvml_dom_element_t *elem,
        pchvml_dom_element_t *child);

int pchvml_dom_element_tag_set_ns(pchvml_dom_element_tag_t *tag,
        const char *ns);

int pchvml_dom_element_tag_set_name(pchvml_dom_element_tag_t *tag,
        const char *name);

int pchvml_dom_element_tag_append_attr(pchvml_dom_element_tag_t *tag,
        pchvml_dom_element_attr_t *attr);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_VDOM_H */

