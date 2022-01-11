/**
 * @file html_interface.c
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

#include "private/mraw.h"
#include "html/html_interface.h"
#include "html/interfaces/document.h"


#define PCHTML_HTML_INTERFACE_RES_CONSTRUCTORS
#define PCHTML_HTML_INTERFACE_RES_DESTRUCTOR
#include "html_interface_res.h"


pcdom_interface_t *
pchtml_html_interface_create(pchtml_html_document_t *document, pchtml_tag_id_t tag_id,
                          pchtml_ns_id_t ns)
{
    pcdom_node_t *node;

    if (tag_id >= PCHTML_TAG__LAST_ENTRY) {
        if (ns == PCHTML_NS_HTML) {
            pchtml_html_unknown_element_t *unel;

            unel = pchtml_html_unknown_element_interface_create(document);
            node = pcdom_interface_node(unel);
        }
        else if (ns == PCHTML_NS_SVG) {
            /* TODO: For this need implement SVGElement */
            pcdom_element_t *domel;

            domel = pcdom_element_interface_create(&document->dom_document);
            node = pcdom_interface_node(domel);
        }
        else {
            pcdom_element_t *domel;

            domel = pcdom_element_interface_create(&document->dom_document);
            node = pcdom_interface_node(domel);
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

pcdom_interface_t *
pchtml_html_interface_destroy(pcdom_interface_t *intrfc)
{
    if (intrfc == NULL) {
        return NULL;
    }

    pcdom_node_t *node = intrfc;

    switch (node->type) {
        case PCDOM_NODE_TYPE_TEXT:
        case PCDOM_NODE_TYPE_COMMENT:
        case PCDOM_NODE_TYPE_ELEMENT:
        case PCDOM_NODE_TYPE_DOCUMENT:
        case PCDOM_NODE_TYPE_DOCUMENT_TYPE:
            if (node->local_name >= PCHTML_TAG__LAST_ENTRY) {
                if (node->ns == PCHTML_NS_HTML) {
                    return pchtml_html_unknown_element_interface_destroy(intrfc);
                }
                else if (node->ns == PCHTML_NS_SVG) {
                    /* TODO: For this need implement SVGElement */
                    return pcdom_element_interface_destroy(intrfc);
                }
                else {
                    return pcdom_element_interface_destroy(intrfc);
                }
            }
            else {
                return pchtml_html_interface_res_destructor[node->local_name][node->ns](intrfc);
            }

        case PCDOM_NODE_TYPE_ATTRIBUTE:
            return pcdom_attr_interface_destroy(intrfc);

        case PCDOM_NODE_TYPE_CDATA_SECTION:
            return pcdom_cdata_section_interface_destroy(intrfc);

        case PCDOM_NODE_TYPE_DOCUMENT_FRAGMENT:
            return pcdom_document_fragment_interface_destroy(intrfc);

        case PCDOM_NODE_TYPE_PROCESSING_INSTRUCTION:
            return pcdom_processing_instruction_interface_destroy(intrfc);

        default:
            return NULL;
    }
}
