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
 *
 * This implementation of HTML parser is derived from Lexbor <http://lexbor.com/>.
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#ifndef PCEDOM_COLLECTION_H
#define PCEDOM_COLLECTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "html/core/base.h"

#include "private/array.h"
#include "private/edom/interface.h"


typedef struct {
    pcutils_array_t     array;
    pcedom_document_t *document;
}
pcedom_collection_t;


pcedom_collection_t *
pcedom_collection_create(pcedom_document_t *document) WTF_INTERNAL;

unsigned int
pcedom_collection_init(pcedom_collection_t *col, 
                size_t start_list_size) WTF_INTERNAL;

pcedom_collection_t *
pcedom_collection_destroy(pcedom_collection_t *col, 
                bool self_destroy) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline pcedom_collection_t *
pcedom_collection_make(pcedom_document_t *document, size_t start_list_size)
{
    unsigned int status;
    pcedom_collection_t *col;

    col = pcedom_collection_create(document);
    status = pcedom_collection_init(col, start_list_size);

    if(status != PCHTML_STATUS_OK) {
        return pcedom_collection_destroy(col, true);
    }

    return col;
}

static inline void
pcedom_collection_clean(pcedom_collection_t *col)
{
    pcutils_array_clean(&col->array);
}

static inline unsigned int
pcedom_collection_append(pcedom_collection_t *col, void *value)
{
    return pcutils_array_push(&col->array, value);
}

static inline pcedom_element_t *
pcedom_collection_element(pcedom_collection_t *col, size_t idx)
{
    return (pcedom_element_t *) pcutils_array_get(&col->array, idx);
}

static inline pcedom_node_t *
pcedom_collection_node(pcedom_collection_t *col, size_t idx)
{
    return (pcedom_node_t *) pcutils_array_get(&col->array, idx);
}

static inline size_t
pcedom_collection_length(pcedom_collection_t *col)
{
    return pcutils_array_length(&col->array);
}

/*
 * No inline functions for ABI.
 */
pcedom_collection_t *
pcedom_collection_make_noi(pcedom_document_t *document, size_t start_list_size);

void
pcedom_collection_clean_noi(pcedom_collection_t *col);

unsigned int
pcedom_collection_append_noi(pcedom_collection_t *col, void *value);

pcedom_element_t *
pcedom_collection_element_noi(pcedom_collection_t *col, size_t idx);

pcedom_node_t *
pcedom_collection_node_noi(pcedom_collection_t *col, size_t idx);

size_t
pcedom_collection_length_noi(pcedom_collection_t *col);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCEDOM_COLLECTION_H */
