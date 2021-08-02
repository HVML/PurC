/**
 * @file node.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of dom node.
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

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"

#include "private/edom/node.h"
#include "private/edom/attr.h"
#include "private/edom/document.h"
#include "private/edom/document_type.h"
#include "private/edom/element.h"
#include "private/edom/processing_instruction.h"


static pchtml_action_t
pcedom_node_text_content_size(pcedom_node_t *node, void *ctx);

static pchtml_action_t
pcedom_node_text_content_concatenate(pcedom_node_t *node, void *ctx);


pcedom_node_t *
pcedom_node_interface_create(pcedom_document_t *document)
{
    pcedom_node_t *element;

    element = pchtml_mraw_calloc(document->mraw,
                                 sizeof(pcedom_node_t));
    if (element == NULL) {
        return NULL;
    }

    element->owner_document = document;
    element->type = PCEDOM_NODE_TYPE_UNDEF;

    return element;
}

pcedom_node_t *
pcedom_node_interface_destroy(pcedom_node_t *node)
{
    return pchtml_mraw_free(node->owner_document->mraw, node);
}

pcedom_node_t *
pcedom_node_destroy(pcedom_node_t *node)
{
    pcedom_node_remove(node);

    return pcedom_document_destroy_interface(node);
}

pcedom_node_t *
pcedom_node_destroy_deep(pcedom_node_t *root)
{
    pcedom_node_t *tmp;
    pcedom_node_t *node = root;

    while (node != NULL) {
        if (node->first_child != NULL) {
            node = node->first_child;
        }
        else {
            while(node != root && node->next == NULL) {
                tmp = node->parent;

                pcedom_node_destroy(node);

                node = tmp;
            }

            if (node == root) {
                pcedom_node_destroy(node);

                break;
            }

            tmp = node->next;

            pcedom_node_destroy(node);

            node = tmp;
        }
    }

    return NULL;
}

const unsigned char *
pcedom_node_name(pcedom_node_t *node, size_t *len)
{
    switch (node->type) {
        case PCEDOM_NODE_TYPE_ELEMENT:
            return pcedom_element_tag_name(pcedom_interface_element(node),
                                            len);

        case PCEDOM_NODE_TYPE_ATTRIBUTE:
            return pcedom_attr_qualified_name(pcedom_interface_attr(node),
                                               len);

        case PCEDOM_NODE_TYPE_TEXT:
            if (len != NULL) {
                *len = sizeof("#text") - 1;
            }

            return (const unsigned char *) "#text";

        case PCEDOM_NODE_TYPE_CDATA_SECTION:
            if (len != NULL) {
                *len = sizeof("#cdata-section") - 1;
            }

            return (const unsigned char *) "#cdata-section";

        case PCEDOM_NODE_TYPE_PROCESSING_INSTRUCTION:
            return pcedom_processing_instruction_target(pcedom_interface_processing_instruction(node),
                                                         len);

        case PCEDOM_NODE_TYPE_COMMENT:
            if (len != NULL) {
                *len = sizeof("#comment") - 1;
            }

            return (const unsigned char *) "#comment";

        case PCEDOM_NODE_TYPE_DOCUMENT:
            if (len != NULL) {
                *len = sizeof("#document") - 1;
            }

            return (const unsigned char *) "#document";

        case PCEDOM_NODE_TYPE_DOCUMENT_TYPE:
            return pcedom_document_type_name(pcedom_interface_document_type(node),
                                              len);

        case PCEDOM_NODE_TYPE_DOCUMENT_FRAGMENT:
            if (len != NULL) {
                *len = sizeof("#document-fragment") - 1;
            }

            return (const unsigned char *) "#document-fragment";

        default:
            break;
    }

    if (len != NULL) {
        *len = 0;
    }

    return NULL;
}

void
pcedom_node_insert_child(pcedom_node_t *to, pcedom_node_t *node)
{
    if (to->last_child != NULL) {
        to->last_child->next = node;
    }
    else {
        to->first_child = node;
    }

    node->parent = to;
    node->next = NULL;
    node->prev = to->last_child;

    to->last_child = node;
}

void
pcedom_node_insert_before(pcedom_node_t *to, pcedom_node_t *node)
{
    if (to->prev != NULL) {
        to->prev->next = node;
    }
    else {
        if (to->parent != NULL) {
            to->parent->first_child = node;
        }
    }

    node->parent = to->parent;
    node->next = to;
    node->prev = to->prev;

    to->prev = node;
}

void
pcedom_node_insert_after(pcedom_node_t *to, pcedom_node_t *node)
{
    if (to->next != NULL) {
        to->next->prev = node;
    }
    else {
        if (to->parent != NULL) {
            to->parent->last_child = node;
        }
    }

    node->parent = to->parent;
    node->next = to->next;
    node->prev = to;
    to->next = node;
}

void
pcedom_node_remove(pcedom_node_t *node)
{
    if (node->parent != NULL) {
        if (node->parent->first_child == node) {
            node->parent->first_child = node->next;
        }

        if (node->parent->last_child == node) {
            node->parent->last_child = node->prev;
        }
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    }

    if (node->prev != NULL) {
        node->prev->next = node->next;
    }

    node->parent = NULL;
    node->next = NULL;
    node->prev = NULL;
}

unsigned int
pcedom_node_replace_all(pcedom_node_t *parent, pcedom_node_t *node)
{
    while (parent->first_child != NULL) {
        pcedom_node_destroy_deep(parent->first_child);
    }

    pcedom_node_insert_child(parent, node);

    return PCHTML_STATUS_OK;
}

void
pcedom_node_simple_walk(pcedom_node_t *root,
                         pcedom_node_simple_walker_f walker_cb, void *ctx)
{
    pchtml_action_t action;
    pcedom_node_t *node = root->first_child;

    while (node != NULL) {
        action = walker_cb(node, ctx);
        if (action == PCHTML_ACTION_STOP) {
            return;
        }

        if (node->first_child != NULL && action != PCHTML_ACTION_NEXT) {
            node = node->first_child;
        }
        else {
            while(node != root && node->next == NULL) {
                node = node->parent;
            }

            if (node == root) {
                break;
            }

            node = node->next;
        }
    }
}

unsigned char *
pcedom_node_text_content(pcedom_node_t *node, size_t *len)
{
    unsigned char *text;
    size_t length = 0;

    switch (node->type) {
        case PCEDOM_NODE_TYPE_DOCUMENT_FRAGMENT:
        case PCEDOM_NODE_TYPE_ELEMENT:
            pcedom_node_simple_walk(node, pcedom_node_text_content_size,
                                     &length);

            text = pcedom_document_create_text(node->owner_document,
                                                (length + 1));
            if (text == NULL) {
                goto failed;
            }

            pcedom_node_simple_walk(node, pcedom_node_text_content_concatenate,
                                     &text);

            text -= length;

            break;

        case PCEDOM_NODE_TYPE_ATTRIBUTE: {
            const unsigned char *attr_text;

            attr_text = pcedom_attr_value(pcedom_interface_attr(node), &length);
            if (attr_text == NULL) {
                goto failed;
            }

            text = pcedom_document_create_text(node->owner_document,
                                                (length + 1));
            if (text == NULL) {
                goto failed;
            }

            /* +1 == with null '\0' */
            memcpy(text, attr_text, sizeof(unsigned char) * (length + 1));

            break;
        }

        case PCEDOM_NODE_TYPE_TEXT:
        case PCEDOM_NODE_TYPE_PROCESSING_INSTRUCTION:
        case PCEDOM_NODE_TYPE_COMMENT: {
            pcedom_character_data_t *ch_data;

            ch_data = pcedom_interface_character_data(node);
            length = ch_data->data.length;

            text = pcedom_document_create_text(node->owner_document,
                                                (length + 1));
            if (text == NULL) {
                goto failed;
            }

            /* +1 == with null '\0' */
            memcpy(text, ch_data->data.data, sizeof(unsigned char) * (length + 1));

            break;
        }

        default:
            goto failed;
    }

    if (len != NULL) {
        *len = length;
    }

    text[length] = 0x00;

    return text;

failed:

    if (len != NULL) {
        *len = 0;
    }

    return NULL;
}

static pchtml_action_t
pcedom_node_text_content_size(pcedom_node_t *node, void *ctx)
{
    if (node->type == PCEDOM_NODE_TYPE_TEXT) {
        *((size_t *) ctx) += pcedom_interface_text(node)->char_data.data.length;
    }

    return PCHTML_ACTION_OK;
}

static pchtml_action_t
pcedom_node_text_content_concatenate(pcedom_node_t *node, void *ctx)
{
    if (node->type != PCEDOM_NODE_TYPE_TEXT) {
        return PCHTML_ACTION_OK;
    }

    unsigned char **text = (unsigned char **) ctx;
    pcedom_character_data_t *ch_data = &pcedom_interface_text(node)->char_data;

    memcpy(*text, ch_data->data.data, sizeof(unsigned char) * ch_data->data.length);

    *text = *text + ch_data->data.length;

    return PCHTML_ACTION_OK;
}

unsigned int
pcedom_node_text_content_set(pcedom_node_t *node,
                              const unsigned char *content, size_t len)
{
    unsigned int status;

    switch (node->type) {
        case PCEDOM_NODE_TYPE_DOCUMENT_FRAGMENT:
        case PCEDOM_NODE_TYPE_ELEMENT: {
            pcedom_text_t *text;

            text = pcedom_document_create_text_node(node->owner_document,
                                                     content, len);
            if (text == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
            }

            status = pcedom_node_replace_all(node, pcedom_interface_node(text));
            if (status != PCHTML_STATUS_OK) {
                pcedom_document_destroy_interface(text);

                return status;
            }

            break;
        }

        case PCEDOM_NODE_TYPE_ATTRIBUTE:
            return pcedom_attr_set_existing_value(pcedom_interface_attr(node),
                                                   content, len);

        case PCEDOM_NODE_TYPE_TEXT:
        case PCEDOM_NODE_TYPE_PROCESSING_INSTRUCTION:
        case PCEDOM_NODE_TYPE_COMMENT:
            return pcedom_character_data_replace(pcedom_interface_character_data(node),
                                                  content, len, 0, 0);

        default:
            return PCHTML_STATUS_OK;
    }

    return PCHTML_STATUS_OK;
}

pchtml_tag_id_t
pcedom_node_tag_id_noi(pcedom_node_t *node)
{
    return pcedom_node_tag_id(node);
}

pcedom_node_t *
pcedom_node_next_noi(pcedom_node_t *node)
{
    return pcedom_node_next(node);
}

pcedom_node_t *
pcedom_node_prev_noi(pcedom_node_t *node)
{
    return pcedom_node_prev(node);
}

pcedom_node_t *
pcedom_node_parent_noi(pcedom_node_t *node)
{
    return pcedom_node_parent(node);
}

pcedom_node_t *
pcedom_node_first_child_noi(pcedom_node_t *node)
{
    return pcedom_node_first_child(node);
}

pcedom_node_t *
pcedom_node_last_child_noi(pcedom_node_t *node)
{
    return pcedom_node_last_child(node);
}
