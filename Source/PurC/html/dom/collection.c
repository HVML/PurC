/**
 * @file collection.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of collection container.
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


#include "html/dom/collection.h"
#include "html/dom/interfaces/document.h"


pchtml_dom_collection_t *
pchtml_dom_collection_create(pchtml_dom_document_t *document)
{
    pchtml_dom_collection_t *col;

    col = pchtml_mraw_calloc(document->mraw, sizeof(pchtml_dom_collection_t));
    if (col == NULL) {
        return NULL;
    }

    col->document = document;

    return col;
}

unsigned int
pchtml_dom_collection_init(pchtml_dom_collection_t *col, size_t start_list_size)
{
    if (col == NULL) {
        return PCHTML_STATUS_ERROR_WRONG_ARGS;
    }

    if (col->document == NULL) {
        return PCHTML_STATUS_ERROR_INCOMPLETE_OBJECT;
    }

    return pchtml_array_init(&col->array, start_list_size);
}

pchtml_dom_collection_t *
pchtml_dom_collection_destroy(pchtml_dom_collection_t *col, bool self_destroy)
{
    if (col == NULL) {
        return NULL;
    }

    if (col->array.list != NULL) {
        pchtml_array_destroy(&col->array, false);

        col->array.list = NULL;
    }

    if (self_destroy) {
        if (col->document != NULL) {
            return pchtml_mraw_free(col->document->mraw, col);
        }

        return NULL;
    }

    return col;
}

/*
 * No inline functions for ABI.
 */
pchtml_dom_collection_t *
pchtml_dom_collection_make_noi(pchtml_dom_document_t *document, size_t start_list_size)
{
    return pchtml_dom_collection_make(document, start_list_size);
}

void
pchtml_dom_collection_clean_noi(pchtml_dom_collection_t *col)
{
    pchtml_dom_collection_clean(col);
}

unsigned int
pchtml_dom_collection_append_noi(pchtml_dom_collection_t *col, void *value)
{
    return pchtml_dom_collection_append(col, value);
}

pchtml_dom_element_t *
pchtml_dom_collection_element_noi(pchtml_dom_collection_t *col, size_t idx)
{
    return pchtml_dom_collection_element(col, idx);
}

pchtml_dom_node_t *
pchtml_dom_collection_node_noi(pchtml_dom_collection_t *col, size_t idx)
{
    return pchtml_dom_collection_node(col, idx);
}

size_t
pchtml_dom_collection_length_noi(pchtml_dom_collection_t *col)
{
    return pchtml_dom_collection_length(col);
}
