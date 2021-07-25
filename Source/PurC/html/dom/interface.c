/**
 * @file interface.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of dom interface.
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


#include "html/dom/interface.h"
#include "html/dom/interfaces/cdata_section.h"
#include "html/dom/interfaces/character_data.h"
#include "html/dom/interfaces/comment.h"
#include "html/dom/interfaces/document.h"
#include "html/dom/interfaces/document_fragment.h"
#include "html/dom/interfaces/document_type.h"
#include "html/dom/interfaces/element.h"
#include "html/dom/interfaces/event_target.h"
#include "html/dom/interfaces/node.h"
#include "html/dom/interfaces/processing_instruction.h"
#include "html/dom/interfaces/shadow_root.h"
#include "html/dom/interfaces/text.h"


pchtml_dom_interface_t *
pchtml_dom_interface_create(pchtml_dom_document_t *document, pchtml_tag_id_t tag_id,
                         pchtml_ns_id_t ns)
{
    pchtml_dom_element_t *domel;

    domel = pchtml_dom_element_interface_create(document);
    if (domel == NULL) {
        return NULL;
    }

    domel->node.local_name = tag_id;
    domel->node.ns = ns;

    return domel;
}

pchtml_dom_interface_t *
pchtml_dom_interface_destroy(pchtml_dom_interface_t *intrfc)
{
    if (intrfc == NULL) {
        return NULL;
    }

    pchtml_dom_node_t *node = intrfc;

    switch (node->type) {
        case PCHTML_DOM_NODE_TYPE_ELEMENT:
            return pchtml_dom_element_interface_destroy(intrfc);

        case PCHTML_DOM_NODE_TYPE_TEXT:
            return pchtml_dom_text_interface_destroy(intrfc);

        case PCHTML_DOM_NODE_TYPE_CDATA_SECTION:
            return pchtml_dom_cdata_section_interface_destroy(intrfc);

        case PCHTML_DOM_NODE_TYPE_PROCESSING_INSTRUCTION:
            return pchtml_dom_processing_instruction_interface_destroy(intrfc);

        case PCHTML_DOM_NODE_TYPE_COMMENT:
            return pchtml_dom_comment_interface_destroy(intrfc);

        case PCHTML_DOM_NODE_TYPE_DOCUMENT:
            return pchtml_dom_document_interface_destroy(intrfc);

        case PCHTML_DOM_NODE_TYPE_DOCUMENT_TYPE:
            return pchtml_dom_document_type_interface_destroy(intrfc);

        case PCHTML_DOM_NODE_TYPE_DOCUMENT_FRAGMENT:
            return pchtml_dom_document_fragment_interface_destroy(intrfc);

        default:
            return pchtml_mraw_free(node->owner_document->mraw, intrfc);
    }
}
