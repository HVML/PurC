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
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "private/dom.h"
#include "dom/shadow_root.h"

pcdom_interface_t *
pcdom_interface_create(pcdom_document_t *document, pchtml_tag_id_t tag_id,
                         pchtml_ns_id_t ns)
{
    pcdom_element_t *domel;

    domel = pcdom_element_interface_create(document);
    if (domel == NULL) {
        return NULL;
    }

    domel->node.local_name = tag_id;
    domel->node.ns = ns;

    return domel;
}

pcdom_interface_t *
pcdom_interface_destroy(pcdom_interface_t *intrfc)
{
    if (intrfc == NULL) {
        return NULL;
    }

    pcdom_node_t *node = intrfc;

    switch (node->type) {
        case PCDOM_NODE_TYPE_ELEMENT:
            return pcdom_element_interface_destroy(intrfc);

        case PCDOM_NODE_TYPE_TEXT:
            return pcdom_text_interface_destroy(intrfc);

        case PCDOM_NODE_TYPE_CDATA_SECTION:
            return pcdom_cdata_section_interface_destroy(intrfc);

        case PCDOM_NODE_TYPE_PROCESSING_INSTRUCTION:
            return pcdom_processing_instruction_interface_destroy(intrfc);

        case PCDOM_NODE_TYPE_COMMENT:
            return pcdom_comment_interface_destroy(intrfc);

        case PCDOM_NODE_TYPE_DOCUMENT:
            return pcdom_document_interface_destroy(intrfc);

        case PCDOM_NODE_TYPE_DOCUMENT_TYPE:
            return pcdom_document_type_interface_destroy(intrfc);

        case PCDOM_NODE_TYPE_DOCUMENT_FRAGMENT:
            return pcdom_document_fragment_interface_destroy(intrfc);

        default:
            return pcutils_mraw_free(node->owner_document->mraw, intrfc);
    }
}
