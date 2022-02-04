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
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "private/dom.h"

static pchtml_action_t
pcdom_node_text_content_size(pcdom_node_t *node, void *ctx);

static pchtml_action_t
pcdom_node_text_content_concatenate(pcdom_node_t *node, void *ctx);


pcdom_node_t *
pcdom_node_interface_create(pcdom_document_t *document)
{
    pcdom_node_t *element;

    element = pcutils_mraw_calloc(document->mraw,
                                 sizeof(pcdom_node_t));
    if (element == NULL) {
        return NULL;
    }

    element->owner_document = document;
    element->type = PCDOM_NODE_TYPE_UNDEF;

    return element;
}

pcdom_node_t *
pcdom_node_interface_destroy(pcdom_node_t *node)
{
    return pcutils_mraw_free(node->owner_document->mraw, node);
}

pcdom_node_t *
pcdom_node_destroy(pcdom_node_t *node)
{
    pcdom_node_remove(node);

    return pcdom_document_destroy_interface(node);
}

pcdom_node_t *
pcdom_node_destroy_deep(pcdom_node_t *root)
{
    pcdom_node_t *tmp;
    pcdom_node_t *node = root;

    while (node != NULL) {
        if (node->first_child != NULL) {
            node = node->first_child;
        }
        else {
            while(node != root && node->next == NULL) {
                tmp = node->parent;

                pcdom_node_destroy(node);

                node = tmp;
            }

            if (node == root) {
                pcdom_node_destroy(node);

                break;
            }

            tmp = node->next;

            pcdom_node_destroy(node);

            node = tmp;
        }
    }

    return NULL;
}

const unsigned char *
pcdom_node_name(pcdom_node_t *node, size_t *len)
{
    switch (node->type) {
        case PCDOM_NODE_TYPE_ELEMENT:
            return pcdom_element_tag_name(pcdom_interface_element(node),
                                            len);

        case PCDOM_NODE_TYPE_ATTRIBUTE:
            return pcdom_attr_qualified_name(pcdom_interface_attr(node),
                                               len);

        case PCDOM_NODE_TYPE_TEXT:
            if (len != NULL) {
                *len = sizeof("#text") - 1;
            }

            return (const unsigned char *) "#text";

        case PCDOM_NODE_TYPE_CDATA_SECTION:
            if (len != NULL) {
                *len = sizeof("#cdata-section") - 1;
            }

            return (const unsigned char *) "#cdata-section";

        case PCDOM_NODE_TYPE_PROCESSING_INSTRUCTION:
            return pcdom_processing_instruction_target(pcdom_interface_processing_instruction(node),
                                                         len);

        case PCDOM_NODE_TYPE_COMMENT:
            if (len != NULL) {
                *len = sizeof("#comment") - 1;
            }

            return (const unsigned char *) "#comment";

        case PCDOM_NODE_TYPE_DOCUMENT:
            if (len != NULL) {
                *len = sizeof("#document") - 1;
            }

            return (const unsigned char *) "#document";

        case PCDOM_NODE_TYPE_DOCUMENT_TYPE:
            return pcdom_document_type_name(pcdom_interface_document_type(node),
                                              len);

        case PCDOM_NODE_TYPE_DOCUMENT_FRAGMENT:
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
pcdom_node_insert_child(pcdom_node_t *to, pcdom_node_t *node)
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
pcdom_node_insert_before(pcdom_node_t *to, pcdom_node_t *node)
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
pcdom_node_insert_after(pcdom_node_t *to, pcdom_node_t *node)
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
pcdom_node_remove(pcdom_node_t *node)
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
pcdom_node_replace_all(pcdom_node_t *parent, pcdom_node_t *node)
{
    while (parent->first_child != NULL) {
        pcdom_node_destroy_deep(parent->first_child);
    }

    pcdom_node_insert_child(parent, node);

    return PCHTML_STATUS_OK;
}

void
pcdom_node_simple_walk(pcdom_node_t *root,
                         pcdom_node_simple_walker_f walker_cb, void *ctx)
{
    pchtml_action_t action;
    pcdom_node_t *node = root->first_child;

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
pcdom_node_text_content(pcdom_node_t *node, size_t *len)
{
    unsigned char *text;
    size_t length = 0;

    switch (node->type) {
        case PCDOM_NODE_TYPE_DOCUMENT_FRAGMENT:
        case PCDOM_NODE_TYPE_ELEMENT:
            pcdom_node_simple_walk(node, pcdom_node_text_content_size,
                                     &length);

            text = pcdom_document_create_text(node->owner_document,
                                                (length + 1));
            if (text == NULL) {
                goto failed;
            }

            pcdom_node_simple_walk(node, pcdom_node_text_content_concatenate,
                                     &text);

            text -= length;

            break;

        case PCDOM_NODE_TYPE_ATTRIBUTE: {
            const unsigned char *attr_text;

            attr_text = pcdom_attr_value(pcdom_interface_attr(node), &length);
            if (attr_text == NULL) {
                goto failed;
            }

            text = pcdom_document_create_text(node->owner_document,
                                                (length + 1));
            if (text == NULL) {
                goto failed;
            }

            /* +1 == with null '\0' */
            memcpy(text, attr_text, sizeof(unsigned char) * (length + 1));

            break;
        }

        case PCDOM_NODE_TYPE_TEXT:
        case PCDOM_NODE_TYPE_PROCESSING_INSTRUCTION:
        case PCDOM_NODE_TYPE_COMMENT: {
            pcdom_character_data_t *ch_data;

            ch_data = pcdom_interface_character_data(node);
            length = ch_data->data.length;

            text = pcdom_document_create_text(node->owner_document,
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
pcdom_node_text_content_size(pcdom_node_t *node, void *ctx)
{
    if (node->type == PCDOM_NODE_TYPE_TEXT) {
        *((size_t *) ctx) += pcdom_interface_text(node)->char_data.data.length;
    }

    return PCHTML_ACTION_OK;
}

static pchtml_action_t
pcdom_node_text_content_concatenate(pcdom_node_t *node, void *ctx)
{
    if (node->type != PCDOM_NODE_TYPE_TEXT) {
        return PCHTML_ACTION_OK;
    }

    unsigned char **text = (unsigned char **) ctx;
    pcdom_character_data_t *ch_data = &pcdom_interface_text(node)->char_data;

    memcpy(*text, ch_data->data.data, sizeof(unsigned char) * ch_data->data.length);

    *text = *text + ch_data->data.length;

    return PCHTML_ACTION_OK;
}

unsigned int
pcdom_node_text_content_set(pcdom_node_t *node,
                              const unsigned char *content, size_t len)
{
    unsigned int status;

    switch (node->type) {
        case PCDOM_NODE_TYPE_DOCUMENT_FRAGMENT:
        case PCDOM_NODE_TYPE_ELEMENT: {
            pcdom_text_t *text;

            text = pcdom_document_create_text_node(node->owner_document,
                                                     content, len);
            if (text == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
            }

            status = pcdom_node_replace_all(node, pcdom_interface_node(text));
            if (status != PCHTML_STATUS_OK) {
                pcdom_document_destroy_interface(text);

                return status;
            }

            break;
        }

        case PCDOM_NODE_TYPE_ATTRIBUTE:
            return pcdom_attr_set_existing_value(pcdom_interface_attr(node),
                                                   content, len);

        case PCDOM_NODE_TYPE_TEXT:
        case PCDOM_NODE_TYPE_PROCESSING_INSTRUCTION:
        case PCDOM_NODE_TYPE_COMMENT:
            return pcdom_character_data_replace(pcdom_interface_character_data(node),
                                                  content, len, 0, 0);

        default:
            return PCHTML_STATUS_OK;
    }

    return PCHTML_STATUS_OK;
}

void
pcdom_displace_fragment(pcdom_node_t *parent,
        pcdom_node_t *fragment)
{
    while (parent->first_child)
        pcdom_node_destroy_deep(parent->first_child);

    pcdom_merge_fragment_append(parent, fragment);
}

void
pcdom_merge_fragment_prepend(pcdom_node_t *parent,
        pcdom_node_t *fragment)
{
    while (fragment->last_child != NULL) {
        pcdom_node_t *child;
        child = fragment->last_child;

        pcdom_node_remove(child);
        if (parent->first_child == NULL) {
            pcdom_node_insert_child(parent, child);
        }
        else {
            pcdom_node_insert_before(parent->first_child, child);
        }
    }

    pcdom_node_destroy(fragment);
}

void
pcdom_merge_fragment_append(pcdom_node_t *parent,
        pcdom_node_t *fragment)
{
    while (fragment->first_child != NULL) {
        pcdom_node_t *child;
        child = fragment->first_child;

        pcdom_node_remove(child);
        if (parent->last_child == NULL) {
            pcdom_node_insert_child(parent, child);
        }
        else {
            pcdom_node_insert_after(parent->last_child, child);
        }
    }

    pcdom_node_destroy(fragment);
}

void
pcdom_merge_fragment_insert_before(pcdom_node_t *to,
        pcdom_node_t *fragment)
{
    while (fragment->first_child != NULL) {
        pcdom_node_t *child;
        child = fragment->first_child;

        pcdom_node_remove(child);
        pcdom_node_insert_before(to, child);
    }

    pcdom_node_destroy(fragment);
}

void
pcdom_merge_fragment_insert_after(pcdom_node_t *to,
        pcdom_node_t *fragment)
{
    while (fragment->last_child != NULL) {
        pcdom_node_t *child;
        child = fragment->last_child;

        pcdom_node_remove(child);
        pcdom_node_insert_after(to, child);
    }

    pcdom_node_destroy(fragment);
}
