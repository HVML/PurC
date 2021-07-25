/**
 * @file processing_instruction.c 
 * @author
 * @date 2021/07/02
 * @brief The complementation of processing instruction.
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


#include "html/dom/interfaces/processing_instruction.h"
#include "html/dom/interfaces/document.h"


pchtml_dom_processing_instruction_t *
pchtml_dom_processing_instruction_interface_create(pchtml_dom_document_t *document)
{
    pchtml_dom_processing_instruction_t *element;

    element = pchtml_mraw_calloc(document->mraw,
                                 sizeof(pchtml_dom_processing_instruction_t));
    if (element == NULL) {
        return NULL;
    }

    pchtml_dom_node_t *node = pchtml_dom_interface_node(element);

    node->owner_document = document;
    node->type = PCHTML_DOM_NODE_TYPE_PROCESSING_INSTRUCTION;

    return element;
}

pchtml_dom_processing_instruction_t *
pchtml_dom_processing_instruction_interface_destroy(pchtml_dom_processing_instruction_t *processing_instruction)
{
    return pchtml_mraw_free(
        pchtml_dom_interface_node(processing_instruction)->owner_document->mraw,
        processing_instruction);
}

/*
 * No inline functions for ABI.
 */
const unsigned char *
pchtml_dom_processing_instruction_target_noi(pchtml_dom_processing_instruction_t *pi,
                                          size_t *len)
{
    return pchtml_dom_processing_instruction_target(pi, len);
}
