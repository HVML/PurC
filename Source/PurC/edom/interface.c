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
 *
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under Apahce 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#include "private/edom/interface.h"
#include "private/edom/cdata_section.h"
#include "private/edom/character_data.h"
#include "private/edom/comment.h"
#include "private/edom/document.h"
#include "private/edom/document_fragment.h"
#include "private/edom/document_type.h"
#include "private/edom/element.h"
#include "private/edom/event_target.h"
#include "private/edom/node.h"
#include "private/edom/shadow_root.h"
#include "private/edom/text.h"
#include "edom/processing_instruction.h"


pcedom_interface_t *
pcedom_interface_create(pcedom_document_t *document, pchtml_tag_id_t tag_id,
                         pchtml_ns_id_t ns)
{
    pcedom_element_t *domel;

    domel = pcedom_element_interface_create(document);
    if (domel == NULL) {
        return NULL;
    }

    domel->node.local_name = tag_id;
    domel->node.ns = ns;

    return domel;
}

pcedom_interface_t *
pcedom_interface_destroy(pcedom_interface_t *intrfc)
{
    if (intrfc == NULL) {
        return NULL;
    }

    pcedom_node_t *node = intrfc;

    switch (node->type) {
        case PCEDOM_NODE_TYPE_ELEMENT:
            return pcedom_element_interface_destroy(intrfc);

        case PCEDOM_NODE_TYPE_TEXT:
            return pcedom_text_interface_destroy(intrfc);

        case PCEDOM_NODE_TYPE_CDATA_SECTION:
            return pcedom_cdata_section_interface_destroy(intrfc);

        case PCEDOM_NODE_TYPE_PROCESSING_INSTRUCTION:
            return pcedom_processing_instruction_interface_destroy(intrfc);

        case PCEDOM_NODE_TYPE_COMMENT:
            return pcedom_comment_interface_destroy(intrfc);

        case PCEDOM_NODE_TYPE_DOCUMENT:
            return pcedom_document_interface_destroy(intrfc);

        case PCEDOM_NODE_TYPE_DOCUMENT_TYPE:
            return pcedom_document_type_interface_destroy(intrfc);

        case PCEDOM_NODE_TYPE_DOCUMENT_FRAGMENT:
            return pcedom_document_fragment_interface_destroy(intrfc);

        default:
            return pchtml_mraw_free(node->owner_document->mraw, intrfc);
    }
}
