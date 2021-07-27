/**
 * @file character_data.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of character data.
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


#include "private/errors.h"

#include "private/edom/character_data.h"
#include "private/edom/document.h"


pchtml_dom_character_data_t *
pchtml_dom_character_data_interface_create(pchtml_dom_document_t *document)
{
    pchtml_dom_character_data_t *element;

    element = pchtml_mraw_calloc(document->mraw,
                                 sizeof(pchtml_dom_character_data_t));
    if (element == NULL) {
        return NULL;
    }

    pchtml_dom_node_t *node = pchtml_dom_interface_node(element);

    node->owner_document = document;
    node->type = PCHTML_DOM_NODE_TYPE_UNDEF;

    return element;
}

pchtml_dom_character_data_t *
pchtml_dom_character_data_interface_destroy(pchtml_dom_character_data_t *character_data)
{
    pchtml_str_destroy(&character_data->data,
                       pchtml_dom_interface_node(character_data)->owner_document->text,
                       false);

    return pchtml_mraw_free(
        pchtml_dom_interface_node(character_data)->owner_document->mraw,
        character_data);
}

/* TODO: oh, need to... https://dom.spec.whatwg.org/#concept-cd-replace */
unsigned int
pchtml_dom_character_data_replace(pchtml_dom_character_data_t *ch_data,
                               const unsigned char *data, size_t len,
                               size_t offset, size_t count)
{
    UNUSED_PARAM(offset);
    UNUSED_PARAM(count);

    if (ch_data->data.data == NULL) {
        pchtml_str_init(&ch_data->data, ch_data->node.owner_document->text, len);
        if (ch_data->data.data == NULL) {
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }
    else if (pchtml_str_size(&ch_data->data) < len) {
        const unsigned char *data;

        data = pchtml_str_realloc(&ch_data->data,
                                  ch_data->node.owner_document->text, (len + 1));
        if (data == NULL) {
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    memcpy(ch_data->data.data, data, sizeof(unsigned char) * len);

    ch_data->data.data[len] = 0x00;
    ch_data->data.length = len;

    return PCHTML_STATUS_OK;
}
