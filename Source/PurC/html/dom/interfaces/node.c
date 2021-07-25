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
 */


#include "html/dom/interfaces/node.h"
#include "html/dom/interfaces/attr.h"
#include "html/dom/interfaces/document.h"
#include "html/dom/interfaces/document_type.h"
#include "html/dom/interfaces/element.h"
#include "html/dom/interfaces/processing_instruction.h"


static pchtml_action_t
pchtml_dom_node_text_content_size(pchtml_dom_node_t *node, void *ctx);

static pchtml_action_t
pchtml_dom_node_text_content_concatenate(pchtml_dom_node_t *node, void *ctx);


pchtml_dom_node_t *
pchtml_dom_node_interface_create(pchtml_dom_document_t *document)
{
    pchtml_dom_node_t *element;

    element = pchtml_mraw_calloc(document->mraw,
                                 sizeof(pchtml_dom_node_t));
    if (element == NULL) {
        return NULL;
    }

    element->owner_document = document;
    element->type = PCHTML_DOM_NODE_TYPE_UNDEF;

    return element;
}

pchtml_dom_node_t *
pchtml_dom_node_interface_destroy(pchtml_dom_node_t *node)
{
    return pchtml_mraw_free(node->owner_document->mraw, node);
}

pchtml_dom_node_t *
pchtml_dom_node_destroy(pchtml_dom_node_t *node)
{
    pchtml_dom_node_remove(node);

    return pchtml_dom_document_destroy_interface(node);
}

pchtml_dom_node_t *
pchtml_dom_node_destroy_deep(pchtml_dom_node_t *root)
{
    pchtml_dom_node_t *tmp;
    pchtml_dom_node_t *node = root;

    while (node != NULL) {
        if (node->first_child != NULL) {
            node = node->first_child;
        }
        else {
            while(node != root && node->next == NULL) {
                tmp = node->parent;

                pchtml_dom_node_destroy(node);

                node = tmp;
            }

            if (node == root) {
                pchtml_dom_node_destroy(node);

                break;
            }

            tmp = node->next;

            pchtml_dom_node_destroy(node);

            node = tmp;
        }
    }

    return NULL;
}

const unsigned char *
pchtml_dom_node_name(pchtml_dom_node_t *node, size_t *len)
{
    switch (node->type) {
        case PCHTML_DOM_NODE_TYPE_ELEMENT:
            return pchtml_dom_element_tag_name(pchtml_dom_interface_element(node),
                                            len);

        case PCHTML_DOM_NODE_TYPE_ATTRIBUTE:
            return pchtml_dom_attr_qualified_name(pchtml_dom_interface_attr(node),
                                               len);

        case PCHTML_DOM_NODE_TYPE_TEXT:
            if (len != NULL) {
                *len = sizeof("#text") - 1;
            }

            return (const unsigned char *) "#text";

        case PCHTML_DOM_NODE_TYPE_CDATA_SECTION:
            if (len != NULL) {
                *len = sizeof("#cdata-section") - 1;
            }

            return (const unsigned char *) "#cdata-section";

        case PCHTML_DOM_NODE_TYPE_PROCESSING_INSTRUCTION:
            return pchtml_dom_processing_instruction_target(pchtml_dom_interface_processing_instruction(node),
                                                         len);

        case PCHTML_DOM_NODE_TYPE_COMMENT:
            if (len != NULL) {
                *len = sizeof("#comment") - 1;
            }

            return (const unsigned char *) "#comment";

        case PCHTML_DOM_NODE_TYPE_DOCUMENT:
            if (len != NULL) {
                *len = sizeof("#document") - 1;
            }

            return (const unsigned char *) "#document";

        case PCHTML_DOM_NODE_TYPE_DOCUMENT_TYPE:
            return pchtml_dom_document_type_name(pchtml_dom_interface_document_type(node),
                                              len);

        case PCHTML_DOM_NODE_TYPE_DOCUMENT_FRAGMENT:
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
pchtml_dom_node_insert_child(pchtml_dom_node_t *to, pchtml_dom_node_t *node)
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
pchtml_dom_node_insert_before(pchtml_dom_node_t *to, pchtml_dom_node_t *node)
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
pchtml_dom_node_insert_after(pchtml_dom_node_t *to, pchtml_dom_node_t *node)
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
pchtml_dom_node_remove(pchtml_dom_node_t *node)
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
pchtml_dom_node_replace_all(pchtml_dom_node_t *parent, pchtml_dom_node_t *node)
{
    while (parent->first_child != NULL) {
        pchtml_dom_node_destroy_deep(parent->first_child);
    }

    pchtml_dom_node_insert_child(parent, node);

    return PCHTML_STATUS_OK;
}

void
pchtml_dom_node_simple_walk(pchtml_dom_node_t *root,
                         pchtml_dom_node_simple_walker_f walker_cb, void *ctx)
{
    pchtml_action_t action;
    pchtml_dom_node_t *node = root->first_child;

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
pchtml_dom_node_text_content(pchtml_dom_node_t *node, size_t *len)
{
    unsigned char *text;
    size_t length = 0;

    switch (node->type) {
        case PCHTML_DOM_NODE_TYPE_DOCUMENT_FRAGMENT:
        case PCHTML_DOM_NODE_TYPE_ELEMENT:
            pchtml_dom_node_simple_walk(node, pchtml_dom_node_text_content_size,
                                     &length);

            text = pchtml_dom_document_create_text(node->owner_document,
                                                (length + 1));
            if (text == NULL) {
                goto failed;
            }

            pchtml_dom_node_simple_walk(node, pchtml_dom_node_text_content_concatenate,
                                     &text);

            text -= length;

            break;

        case PCHTML_DOM_NODE_TYPE_ATTRIBUTE: {
            const unsigned char *attr_text;

            attr_text = pchtml_dom_attr_value(pchtml_dom_interface_attr(node), &length);
            if (attr_text == NULL) {
                goto failed;
            }

            text = pchtml_dom_document_create_text(node->owner_document,
                                                (length + 1));
            if (text == NULL) {
                goto failed;
            }

            /* +1 == with null '\0' */
            memcpy(text, attr_text, sizeof(unsigned char) * (length + 1));

            break;
        }

        case PCHTML_DOM_NODE_TYPE_TEXT:
        case PCHTML_DOM_NODE_TYPE_PROCESSING_INSTRUCTION:
        case PCHTML_DOM_NODE_TYPE_COMMENT: {
            pchtml_dom_character_data_t *ch_data;

            ch_data = pchtml_dom_interface_character_data(node);
            length = ch_data->data.length;

            text = pchtml_dom_document_create_text(node->owner_document,
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
pchtml_dom_node_text_content_size(pchtml_dom_node_t *node, void *ctx)
{
    if (node->type == PCHTML_DOM_NODE_TYPE_TEXT) {
        *((size_t *) ctx) += pchtml_dom_interface_text(node)->char_data.data.length;
    }

    return PCHTML_ACTION_OK;
}

static pchtml_action_t
pchtml_dom_node_text_content_concatenate(pchtml_dom_node_t *node, void *ctx)
{
    if (node->type != PCHTML_DOM_NODE_TYPE_TEXT) {
        return PCHTML_ACTION_OK;
    }

    unsigned char **text = (unsigned char **) ctx;
    pchtml_dom_character_data_t *ch_data = &pchtml_dom_interface_text(node)->char_data;

    memcpy(*text, ch_data->data.data, sizeof(unsigned char) * ch_data->data.length);

    *text = *text + ch_data->data.length;

    return PCHTML_ACTION_OK;
}

unsigned int
pchtml_dom_node_text_content_set(pchtml_dom_node_t *node,
                              const unsigned char *content, size_t len)
{
    unsigned int status;

    switch (node->type) {
        case PCHTML_DOM_NODE_TYPE_DOCUMENT_FRAGMENT:
        case PCHTML_DOM_NODE_TYPE_ELEMENT: {
            pchtml_dom_text_t *text;

            text = pchtml_dom_document_create_text_node(node->owner_document,
                                                     content, len);
            if (text == NULL) {
                return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
            }

            status = pchtml_dom_node_replace_all(node, pchtml_dom_interface_node(text));
            if (status != PCHTML_STATUS_OK) {
                pchtml_dom_document_destroy_interface(text);

                return status;
            }

            break;
        }

        case PCHTML_DOM_NODE_TYPE_ATTRIBUTE:
            return pchtml_dom_attr_set_existing_value(pchtml_dom_interface_attr(node),
                                                   content, len);

        case PCHTML_DOM_NODE_TYPE_TEXT:
        case PCHTML_DOM_NODE_TYPE_PROCESSING_INSTRUCTION:
        case PCHTML_DOM_NODE_TYPE_COMMENT:
            return pchtml_dom_character_data_replace(pchtml_dom_interface_character_data(node),
                                                  content, len, 0, 0);

        default:
            return PCHTML_STATUS_OK;
    }

    return PCHTML_STATUS_OK;
}

pchtml_tag_id_t
pchtml_dom_node_tag_id_noi(pchtml_dom_node_t *node)
{
    return pchtml_dom_node_tag_id(node);
}

pchtml_dom_node_t *
pchtml_dom_node_next_noi(pchtml_dom_node_t *node)
{
    return pchtml_dom_node_next(node);
}

pchtml_dom_node_t *
pchtml_dom_node_prev_noi(pchtml_dom_node_t *node)
{
    return pchtml_dom_node_prev(node);
}

pchtml_dom_node_t *
pchtml_dom_node_parent_noi(pchtml_dom_node_t *node)
{
    return pchtml_dom_node_parent(node);
}

pchtml_dom_node_t *
pchtml_dom_node_first_child_noi(pchtml_dom_node_t *node)
{
    return pchtml_dom_node_first_child(node);
}

pchtml_dom_node_t *
pchtml_dom_node_last_child_noi(pchtml_dom_node_t *node)
{
    return pchtml_dom_node_last_child(node);
}
