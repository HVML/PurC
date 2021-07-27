/**
 * @file document_type.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of document type.
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


#include "private/edom/document_type.h"
#include "private/edom/document.h"


pcedom_document_type_t *
pcedom_document_type_interface_create(pcedom_document_t *document)
{
    pcedom_document_type_t *element;

    element = pchtml_mraw_calloc(document->mraw,
                                 sizeof(pcedom_document_type_t));
    if (element == NULL) {
        return NULL;
    }

    pcedom_node_t *node = pcedom_interface_node(element);

    node->owner_document = document;
    node->type = PCEDOM_NODE_TYPE_DOCUMENT_TYPE;

    return element;
}

pcedom_document_type_t *
pcedom_document_type_interface_destroy(pcedom_document_type_t *document_type)
{
    return pchtml_mraw_free(
        pcedom_interface_node(document_type)->owner_document->mraw,
        document_type);
}

/*
 * No inline functions for ABI.
 */
const unsigned char *
pcedom_document_type_name_noi(pcedom_document_type_t *doc_type, size_t *len)
{
    return pcedom_document_type_name(doc_type, len);
}

const unsigned char *
pcedom_document_type_public_id_noi(pcedom_document_type_t *doc_type,
                                    size_t *len)
{
    return pcedom_document_type_public_id(doc_type, len);
}

const unsigned char *
pcedom_document_type_system_id_noi(pcedom_document_type_t *doc_type,
                                    size_t *len)
{
    return pcedom_document_type_system_id(doc_type, len);
}
