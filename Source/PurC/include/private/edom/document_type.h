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
 */


#ifndef PCHTML_DOM_DOCUMENT_TYPE_H
#define PCHTML_DOM_DOCUMENT_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/str.h"

#include "private/edom/document.h"
#include "private/edom/node.h"
#include "private/edom/attr.h"
#include "private/edom/document_type.h"


struct pchtml_dom_document_type {
    pchtml_dom_node_t    node;

    pchtml_dom_attr_id_t name;
    pchtml_str_t      public_id;
    pchtml_str_t      system_id;
};


pchtml_dom_document_type_t *
pchtml_dom_document_type_interface_create(
                pchtml_dom_document_t *document) WTF_INTERNAL;

pchtml_dom_document_type_t *
pchtml_dom_document_type_interface_destroy(
                pchtml_dom_document_type_t *document_type) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline const unsigned char *
pchtml_dom_document_type_name(pchtml_dom_document_type_t *doc_type, size_t *len)
{
    const pchtml_dom_attr_data_t *data;

    static const unsigned char pchtml_empty[] = "";

    data = pchtml_dom_attr_data_by_id(doc_type->node.owner_document->attrs,
                                   doc_type->name);
    if (data == NULL || doc_type->name == PCHTML_DOM_ATTR__UNDEF) {
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
pchtml_dom_document_type_public_id(pchtml_dom_document_type_t *doc_type, size_t *len)
{
    if (len != NULL) {
        *len = doc_type->public_id.length;
    }

    return doc_type->public_id.data;
}

static inline const unsigned char *
pchtml_dom_document_type_system_id(pchtml_dom_document_type_t *doc_type, size_t *len)
{
    if (len != NULL) {
        *len = doc_type->system_id.length;
    }

    return doc_type->system_id.data;
}

/*
 * No inline functions for ABI.
 */
const unsigned char *
pchtml_dom_document_type_name_noi(pchtml_dom_document_type_t *doc_type, size_t *len);

const unsigned char *
pchtml_dom_document_type_public_id_noi(pchtml_dom_document_type_t *doc_type,
                                    size_t *len);

const unsigned char *
pchtml_dom_document_type_system_id_noi(pchtml_dom_document_type_t *doc_type,
                                    size_t *len);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_DOM_DOCUMENT_TYPE_H */
