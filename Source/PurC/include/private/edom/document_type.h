/**
 * @file document_type.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for document type.
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
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under Apahce 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#ifndef PCEDOM_DOCUMENT_TYPE_H
#define PCEDOM_DOCUMENT_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/str.h"

#include "private/edom/document.h"
#include "private/edom/node.h"
#include "private/edom/attr.h"
#include "private/edom/document_type.h"


struct pcedom_document_type {
    pcedom_node_t    node;

    pcedom_attr_id_t name;
    pchtml_str_t      public_id;
    pchtml_str_t      system_id;
};


pcedom_document_type_t *
pcedom_document_type_interface_create(
                pcedom_document_t *document) WTF_INTERNAL;

pcedom_document_type_t *
pcedom_document_type_interface_destroy(
                pcedom_document_type_t *document_type) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline const unsigned char *
pcedom_document_type_name(pcedom_document_type_t *doc_type, size_t *len)
{
    const pcedom_attr_data_t *data;

    static const unsigned char pchtml_empty[] = "";

    data = pcedom_attr_data_by_id(doc_type->node.owner_document->attrs,
                                   doc_type->name);
    if (data == NULL || doc_type->name == PCEDOM_ATTR__UNDEF) {
        if (len != NULL) {
            *len = 0;
        }

        return pchtml_empty;
    }

    if (len != NULL) {
        *len = data->entry.length;
    }

    return pchtml_hash_entry_str(&data->entry);
}

static inline const unsigned char *
pcedom_document_type_public_id(pcedom_document_type_t *doc_type, size_t *len)
{
    if (len != NULL) {
        *len = doc_type->public_id.length;
    }

    return doc_type->public_id.data;
}

static inline const unsigned char *
pcedom_document_type_system_id(pcedom_document_type_t *doc_type, size_t *len)
{
    if (len != NULL) {
        *len = doc_type->system_id.length;
    }

    return doc_type->system_id.data;
}


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCEDOM_DOCUMENT_TYPE_H */
