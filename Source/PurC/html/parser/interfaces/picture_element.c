/**
 * @file picture_element.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of html picture element.
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


#include "html/parser/interfaces/picture_element.h"
#include "html/parser/interfaces/document.h"


pchtml_html_picture_element_t *
pchtml_html_picture_element_interface_create(pchtml_html_document_t *document)
{
    pchtml_html_picture_element_t *element;

    element = pchtml_mraw_calloc(document->dom_document.mraw,
                                 sizeof(pchtml_html_picture_element_t));
    if (element == NULL) {
        return NULL;
    }

    pcedom_node_t *node = pcedom_interface_node(element);

    node->owner_document = pchtml_html_document_original_ref(document);
    node->type = PCEDOM_NODE_TYPE_ELEMENT;

    return element;
}

pchtml_html_picture_element_t *
pchtml_html_picture_element_interface_destroy(pchtml_html_picture_element_t *picture_element)
{
    return pchtml_mraw_free(
        pcedom_interface_node(picture_element)->owner_document->mraw,
        picture_element);
}
