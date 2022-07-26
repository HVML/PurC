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
 *
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#include "purc.h"
#include "config.h"
#include "private/dom.h"

pcdom_collection_t *
pcdom_collection_create(pcdom_document_t *document)
{
    pcdom_collection_t *col;

    col = pcutils_mraw_calloc(document->mraw, sizeof(pcdom_collection_t));
    if (col == NULL) {
        return NULL;
    }

    col->document = document;

    return col;
}

unsigned int
pcdom_collection_init(pcdom_collection_t *col, size_t start_list_size)
{
    if (col == NULL) {
        return PURC_ERROR_INVALID_VALUE;
    }

    if (col->document == NULL) {
        return PURC_ERROR_INCOMPLETE_OBJECT;
    }

    return pcutils_array_init(&col->array, start_list_size);
}

pcdom_collection_t *
pcdom_collection_destroy(pcdom_collection_t *col, bool self_destroy)
{
    if (col == NULL) {
        return NULL;
    }

    if (col->array.list != NULL) {
        pcutils_array_destroy(&col->array, false);

        col->array.list = NULL;
    }

    if (self_destroy) {
        if (col->document != NULL) {
            return pcutils_mraw_free(col->document->mraw, col);
        }

        return NULL;
    }

    return col;
}
