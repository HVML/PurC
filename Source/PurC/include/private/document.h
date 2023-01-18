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
#include "private/map.h"

struct pcdoc_travel_info {
    pcdoc_node_type_k type;
    bool all;
    size_t nr;
    void *ctxt;
};

struct pcdoc_travel_attrs_info {
    size_t nr;
    void *ctxt;
};

typedef int (*pcdoc_node_cb)(purc_document_t doc, void *node, void *ctxt);

struct purc_document_ops {
    purc_document_t (*create)(const char *content, size_t length);
    void (*destroy)(purc_document_t doc);

    pcdoc_element_t (*operate_element)(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation_k op,
            const char *tag, bool self_close);

    pcdoc_text_node_t (*new_text_content)(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation_k op,
            const char *text, size_t length);

    pcdoc_data_node_t (*new_data_content)(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation_k op,
            purc_variant_t data);

    pcdoc_node (*new_content)(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation_k op,
            const char *content, size_t length);

    int (*set_attribute)(purc_document_t doc,
            pcdoc_element_t elem, pcdoc_operation_k op,
            const char *name, const char *val, size_t len);

    pcdoc_element_t (*special_elem)(purc_document_t doc,
            pcdoc_special_elem_k elem);

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
            pcdoc_element_t elem, pcdoc_node_type_k type, size_t idx);

    int (*get_attribute)(purc_document_t doc, pcdoc_element_t elem,
            const char *name, const char **val, size_t *len);
    int (*get_special_attr)(purc_document_t doc, pcdoc_element_t elem,
            pcdoc_special_attr_k which, const char **val, size_t *len);

    int (*travel_attrs)(purc_document_t doc,
        pcdoc_element_t element, pcdoc_attribute_cb cb,
        struct pcdoc_travel_attrs_info *info);

    pcdoc_attr_t (*first_attr)(purc_document_t doc, pcdoc_element_t elem);
    pcdoc_attr_t (*last_attr)(purc_document_t doc, pcdoc_element_t elem);
    pcdoc_attr_t (*next_attr)(purc_document_t doc, pcdoc_attr_t attr);
    pcdoc_attr_t (*prev_attr)(purc_document_t doc, pcdoc_attr_t attr);

    int (*get_attr_info)(purc_document_t doc, pcdoc_attr_t attr,
        const char **local_name, size_t *local_len,
        const char **qualified_name, size_t *qualified_len,
        const char **value, size_t *value_len);

    int (*get_user_data)(purc_document_t doc, pcdoc_node node,
            void **user_data);
    int (*set_user_data)(purc_document_t doc, pcdoc_node node,
            void *user_data);

    int (*get_text)(purc_document_t doc, pcdoc_text_node_t text_node,
            const char **text, size_t *len);
    int (*get_data)(purc_document_t doc, pcdoc_data_node_t data_node,
        purc_variant_t *data);

    int (*travel)(purc_document_t doc, pcdoc_element_t ancestor,
            pcdoc_node_cb cb, struct pcdoc_travel_info *info);

    int (*serialize)(purc_document_t doc, pcdoc_node node,
            unsigned opts, purc_rwstream_t stm);

    pcdoc_element_t (*find_elem)(purc_document_t doc, pcdoc_element_t scope,
            pcdoc_selector_t selector);

    pcdoc_element_t (*get_elem_by_id)(purc_document_t doc,
            pcdoc_element_t scope, const char *id);

    int (*elem_coll_select)(purc_document_t doc,
            pcdoc_elem_coll_t coll, pcdoc_element_t scope,
            pcdoc_selector_t selector);

    int (*elem_coll_filter)(purc_document_t doc,
            pcdoc_elem_coll_t dst_coll,
            pcdoc_elem_coll_t src_coll, pcdoc_selector_t selector);
};

struct pcdoc_elem_content {
    pcutils_mraw_t     *text;
    pcutils_str_t      *data;
};

struct purc_document {
    purc_document_type_k type;
    pcrdr_msg_data_type def_text_type;

    unsigned need_rdr:1;
    unsigned data_content:1;
    unsigned have_head:1;
    unsigned have_body:1;
    unsigned refc;
    unsigned age;

    pcdoc_element_t root4select;
    struct purc_document_ops *ops;

    void *impl;
};

struct pcdoc_elem_coll {
    purc_document_t  doc;
    pcdoc_element_t  ancestor;
    pcdoc_selector_t selector;

    unsigned    refc;
    unsigned    doc_age;
    size_t      select_begin;
    size_t      nr_elems;

    /* the elements in the collection */
    struct pcutils_arrlist *elems;
};

struct css_element_selector;
struct pcdoc_selector {
    struct css_element_selector *selector;
    char       *id;
    unsigned    refc;
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

