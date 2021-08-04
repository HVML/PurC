/**
 * @file table_row_element.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of html tr element.
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
 * License Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#include "private/edom.h"
#include "html/interfaces/table_row_element.h"
#include "html/interfaces/document.h"


pchtml_html_table_row_element_t *
pchtml_html_table_row_element_interface_create(pchtml_html_document_t *document)
{
    pchtml_html_table_row_element_t *element;

    element = pchtml_mraw_calloc(document->dom_document.mraw,
                                 sizeof(pchtml_html_table_row_element_t));
    if (element == NULL) {
        return NULL;
    }

    pcedom_node_t *node = pcedom_interface_node(element);

    node->owner_document = pchtml_html_document_original_ref(document);
    node->type = PCEDOM_NODE_TYPE_ELEMENT;

    return element;
}

pchtml_html_table_row_element_t *
pchtml_html_table_row_element_interface_destroy(pchtml_html_table_row_element_t *table_row_element)
{
    return pchtml_mraw_free(
        pcedom_interface_node(table_row_element)->owner_document->mraw,
        table_row_element);
}
