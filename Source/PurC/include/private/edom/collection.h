/**
 * @file collection.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for collection container.
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


#ifndef PCHTML_DOM_COLLECTION_H
#define PCHTML_DOM_COLLECTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "html/core/base.h"
#include "html/core/array.h"

#include "private/edom/interface.h"


typedef struct {
    pchtml_array_t     array;
    pchtml_dom_document_t *document;
}
pchtml_dom_collection_t;


pchtml_dom_collection_t *
pchtml_dom_collection_create(pchtml_dom_document_t *document) WTF_INTERNAL;

unsigned int
pchtml_dom_collection_init(pchtml_dom_collection_t *col, 
                size_t start_list_size) WTF_INTERNAL;

pchtml_dom_collection_t *
pchtml_dom_collection_destroy(pchtml_dom_collection_t *col, 
                bool self_destroy) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline pchtml_dom_collection_t *
pchtml_dom_collection_make(pchtml_dom_document_t *document, size_t start_list_size)
{
    unsigned int status;
    pchtml_dom_collection_t *col;

    col = pchtml_dom_collection_create(document);
    status = pchtml_dom_collection_init(col, start_list_size);

    if(status != PCHTML_STATUS_OK) {
        return pchtml_dom_collection_destroy(col, true);
    }

    return col;
}

static inline void
pchtml_dom_collection_clean(pchtml_dom_collection_t *col)
{
    pchtml_array_clean(&col->array);
}

static inline unsigned int
pchtml_dom_collection_append(pchtml_dom_collection_t *col, void *value)
{
    return pchtml_array_push(&col->array, value);
}

static inline pchtml_dom_element_t *
pchtml_dom_collection_element(pchtml_dom_collection_t *col, size_t idx)
{
    return (pchtml_dom_element_t *) pchtml_array_get(&col->array, idx);
}

static inline pchtml_dom_node_t *
pchtml_dom_collection_node(pchtml_dom_collection_t *col, size_t idx)
{
    return (pchtml_dom_node_t *) pchtml_array_get(&col->array, idx);
}

static inline size_t
pchtml_dom_collection_length(pchtml_dom_collection_t *col)
{
    return pchtml_array_length(&col->array);
}

/*
 * No inline functions for ABI.
 */
pchtml_dom_collection_t *
pchtml_dom_collection_make_noi(pchtml_dom_document_t *document, size_t start_list_size);

void
pchtml_dom_collection_clean_noi(pchtml_dom_collection_t *col);

unsigned int
pchtml_dom_collection_append_noi(pchtml_dom_collection_t *col, void *value);

pchtml_dom_element_t *
pchtml_dom_collection_element_noi(pchtml_dom_collection_t *col, size_t idx);

pchtml_dom_node_t *
pchtml_dom_collection_node_noi(pchtml_dom_collection_t *col, size_t idx);

size_t
pchtml_dom_collection_length_noi(pchtml_dom_collection_t *col);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_DOM_COLLECTION_H */
