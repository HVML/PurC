/**
 * @file document.h
 * @date 2022/07/11
 * @brief The internal interfaces for DOCUMENT module.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * Authors:
 *  Vincent Wei (<https://github.com/VincentWei>), 2022
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
 */

#ifndef PURC_PRIVATE_DOCUMENT_H
#define PURC_PRIVATE_DOCUMENT_H

#include "config.h"

#include "purc-document.h"
#include "purc-pcrdr.h"
#include "purc-utils.h"

#include "private/debug.h"
#include "private/errors.h"

struct pcdoc_travel_info {
    pcdoc_node_type type;
    size_t nr;
    void *ctxt;
};

typedef int (*pcdoc_node_cb)(purc_document_t doc, void *node, void *ctxt);

struct purc_document_ops {
    purc_document_t (*create)(const char *content, size_t length);
    void (*destroy)(purc_document_t doc);

    pcdoc_element_t (*operate_element)(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation op,
            const char *tag, bool self_close);

    pcdoc_text_node_t (*new_text_content)(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation op,
            const char *text, size_t length);

    pcdoc_data_node_t (*new_data_content)(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation op,
            purc_variant_t data);

    pcdoc_node (*new_content)(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation op,
            const char *content, size_t length);

    int (*set_attribute)(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation op,
            const char *name, const char *val, size_t len);

    pcdoc_element_t (*special_elem)(purc_document_t doc,
            pcdoc_special_elem elem);

    int (*get_tag_name)(purc_document_t doc, pcdoc_element_t elem,
            const char **local_name, size_t *local_len,
            const char **prefix, size_t *prefix_len,
            const char **ns_name, size_t *ns_len);

    pcdoc_node (*first_child)(purc_document_t doc, pcdoc_element_t elem);
    pcdoc_node (*last_child)(purc_document_t doc, pcdoc_element_t elem);
    pcdoc_node (*next_sibling)(purc_document_t doc, pcdoc_node node);
    pcdoc_node (*prev_sibling)(purc_document_t doc, pcdoc_node node);

    pcdoc_element_t (*get_parent)(purc_document_t doc, pcdoc_node node);

    // nullable
    int (*children_count)(purc_document_t doc, pcdoc_element_t elem,
            size_t *nrs);
    // null if `children_count` is null
    pcdoc_node (*get_child)(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_node_type type, size_t idx);

    int (*get_attribute)(purc_document_t doc, pcdoc_element_t elem,
            const char *name, const char **val, size_t *len);
    int (*get_special_attr)(purc_document_t doc, pcdoc_element_t elem,
            pcdoc_special_attr which, const char **val, size_t *len);

    int (*get_text)(purc_document_t doc, pcdoc_text_node_t text_node,
            const char **text, size_t *len);
    int (*get_data)(purc_document_t doc, pcdoc_data_node_t data_node,
        purc_variant_t *data);

    int (*travel)(purc_document_t doc, pcdoc_element_t ancestor,
            pcdoc_node_cb cb, struct pcdoc_travel_info *info);

    int (*serialize)(purc_document_t doc, pcdoc_node node,
            unsigned opts, purc_rwstream_t stm);

    pcdoc_element_t (*find_elem)(purc_document_t doc, pcdoc_element_t scope,
            const char *selector);

    int (*elem_coll_select)(purc_document_t doc,
            pcdoc_elem_coll_t coll, pcdoc_element_t scope,
            const char *selector);

    int (*elem_coll_filter)(purc_document_t doc,
            pcdoc_elem_coll_t dst_coll,
            pcdoc_elem_coll_t src_coll, const char *selector);
};

struct purc_document {
    purc_document_type type;
    pcrdr_msg_data_type def_text_type;

    unsigned need_rdr:1;
    unsigned data_content:1;
    unsigned have_head:1;
    unsigned have_body:1;
    unsigned refc;

    struct purc_document_ops *ops;

    void *impl;
};

struct pcdoc_elem_coll {
    /* the CSS selector */
    char       *selector;
    unsigned    refc;

    /* the elements in the collection */
    struct pcutils_arrlist *elems;
};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

extern struct purc_document_ops _pcdoc_void_ops WTF_INTERNAL;
extern struct purc_document_ops _pcdoc_plain_ops WTF_INTERNAL;
extern struct purc_document_ops _pcdoc_html_ops WTF_INTERNAL;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_DOCUMENT_H */

