/**
 * @file interface.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of interface operation.
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



#include "html/core/mraw.h"

#include "html/interface.h"
#include "html/interfaces/document.h"

#include "private/edom/interface.h"

#define PCHTML_PARSER_INTERFACE_RES_CONSTRUCTORS
#define PCHTML_PARSER_INTERFACE_RES_DESTRUCTOR
#include "html/interface_res.h"


pcedom_interface_t *
pchtml_html_interface_create(pchtml_html_document_t *document, pchtml_tag_id_t tag_id,
                          pchtml_ns_id_t ns)
{
    pcedom_node_t *node;

    if (tag_id >= PCHTML_TAG__LAST_ENTRY) {
        if (ns == PCHTML_NS_HTML) {
            pchtml_html_unknown_element_t *unel;

            unel = pchtml_html_unknown_element_interface_create(document);
            node = pcedom_interface_node(unel);
        }
        else if (ns == PCHTML_NS_SVG) {
            /* TODO: For this need implement SVGElement */
            pcedom_element_t *domel;

            domel = pcedom_element_interface_create(&document->dom_document);
            node = pcedom_interface_node(domel);
        }
        else {
            pcedom_element_t *domel;

            domel = pcedom_element_interface_create(&document->dom_document);
            node = pcedom_interface_node(domel);
        }
    }
    else {
        node = pchtml_html_interface_res_constructors[tag_id][ns](document);
    }

    if (node == NULL) {
        return NULL;
    }

    node->local_name = tag_id;
    node->ns = ns;

    return node;
}

pcedom_interface_t *
pchtml_html_interface_destroy(pcedom_interface_t *intrfc)
{
    if (intrfc == NULL) {
        return NULL;
    }

    pcedom_node_t *node = intrfc;

    switch (node->type) {
        case PCEDOM_NODE_TYPE_TEXT:
        case PCEDOM_NODE_TYPE_COMMENT:
        case PCEDOM_NODE_TYPE_ELEMENT:
        case PCEDOM_NODE_TYPE_DOCUMENT:
        case PCEDOM_NODE_TYPE_DOCUMENT_TYPE:
            if (node->local_name >= PCHTML_TAG__LAST_ENTRY) {
                if (node->ns == PCHTML_NS_HTML) {
                    return pchtml_html_unknown_element_interface_destroy(intrfc);
                }
                else if (node->ns == PCHTML_NS_SVG) {
                    /* TODO: For this need implement SVGElement */
                    return pcedom_element_interface_destroy(intrfc);
                }
                else {
                    return pcedom_element_interface_destroy(intrfc);
                }
            }
            else {
                return pchtml_html_interface_res_destructor[node->local_name][node->ns](intrfc);
            }

        case PCEDOM_NODE_TYPE_ATTRIBUTE:
            return pcedom_attr_interface_destroy(intrfc);

        case PCEDOM_NODE_TYPE_CDATA_SECTION:
            return pcedom_cdata_section_interface_destroy(intrfc);

        case PCEDOM_NODE_TYPE_DOCUMENT_FRAGMENT:
            return pcedom_document_fragment_interface_destroy(intrfc);

        case PCEDOM_NODE_TYPE_PROCESSING_INSTRUCTION:
            return pcedom_processing_instruction_interface_destroy(intrfc);

        default:
            return NULL;
    }
}
