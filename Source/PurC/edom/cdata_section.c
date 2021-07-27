/**
 * @file cdata_section.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of cdata section.
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


#include "private/edom/cdata_section.h"
#include "private/edom/document.h"


pchtml_dom_cdata_section_t *
pchtml_dom_cdata_section_interface_create(pchtml_dom_document_t *document)
{
    pchtml_dom_cdata_section_t *element;

    element = pchtml_mraw_calloc(document->mraw,
                                 sizeof(pchtml_dom_cdata_section_t));
    if (element == NULL) {
        return NULL;
    }

    pchtml_dom_node_t *node = pchtml_dom_interface_node(element);

    node->owner_document = document;
    node->type = PCHTML_DOM_NODE_TYPE_CDATA_SECTION;

    return element;
}

pchtml_dom_cdata_section_t *
pchtml_dom_cdata_section_interface_destroy(pchtml_dom_cdata_section_t *cdata_section)
{
    return pchtml_mraw_free(
        pchtml_dom_interface_node(cdata_section)->owner_document->mraw,
        cdata_section);
}
