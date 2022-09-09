/**
 * @file vdom-internal.h
 * @author Xu Xiaohong
 * @date 2022/06/11
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

#ifndef PURC_VDOM_VDOM_INTERNAL_H
#define PURC_VDOM_VDOM_INTERNAL_H

/* this feature needs C11 (stdatomic.h) or above */
#if HAVE(STDATOMIC_H)           /* { */
#include <stdatomic.h>
#else                           /* }{ */
#error "Not implemented for this platform."
#endif                          /* } */

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

struct pcvdom_document {
    struct pcvdom_node      node;

    struct pcvdom_doctype   doctype;

    // redundant, for fast access
    struct pcvdom_element  *root;
    struct pcvdom_element  *head;
    struct pcvdom_element  *body;

    struct pcutils_arrlist *bodies;

    atomic_ulong            refc;

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
    enum pchvml_attr_operator       op;

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

    pcutils_array_t        *attr_array;

    unsigned int            self_closing:1;
};

struct pcvdom_content {
    struct pcvdom_node      node;

    struct pcvcm_node      *vcm;
};

struct pcvdom_comment {
    struct pcvdom_node      node;

    char                   *text;
};

#endif // PURC_VDOM_VDOM_INTERNAL_H

