/**
 * @file text.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of text content.
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



#include "html/dom/interfaces/text.h"
#include "html/dom/interfaces/document.h"


pchtml_dom_text_t *
pchtml_dom_text_interface_create(pchtml_dom_document_t *document)
{
    pchtml_dom_text_t *element;

    element = pchtml_mraw_calloc(document->mraw,
                                 sizeof(pchtml_dom_text_t));
    if (element == NULL) {
        return NULL;
    }

    pchtml_dom_node_t *node = pchtml_dom_interface_node(element);

    node->owner_document = document;
    node->type = PCHTML_DOM_NODE_TYPE_TEXT;

    return element;
}

pchtml_dom_text_t *
pchtml_dom_text_interface_destroy(pchtml_dom_text_t *text)
{
    pchtml_str_destroy(&text->char_data.data,
                       pchtml_dom_interface_node(text)->owner_document->text, false);

    return pchtml_mraw_free(
        pchtml_dom_interface_node(text)->owner_document->mraw,
        text);
}
