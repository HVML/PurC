/**
 * @file open_elements.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of html open elements.
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
 * This implementation of HTML parser is derived from Lexbor <http://lexbor.com/>.
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#include "html/tree/open_elements.h"
#include "html/tree.h"


void
pchtml_html_tree_open_elements_remove_by_node(pchtml_html_tree_t *tree,
                                           pcedom_node_t *node)
{
    size_t delta;
    void **list = tree->open_elements->list;
    size_t len = tree->open_elements->length;

    while (len != 0) {
        len--;

        if (list[len] == node) {
            delta = tree->open_elements->length - len - 1;

            memmove(list + len, list + len + 1, sizeof(void *) * delta);

            tree->open_elements->length--;

            break;
        }
    }
}

void
pchtml_html_tree_open_elements_pop_until_tag_id(pchtml_html_tree_t *tree,
                                             pchtml_tag_id_t tag_id,
                                             pchtml_ns_id_t ns,
                                             bool exclude)
{
    void **list = tree->open_elements->list;
    pcedom_node_t *node;

    while (tree->open_elements->length != 0) {
        tree->open_elements->length--;

        node = list[ tree->open_elements->length ];

        if (node->local_name == tag_id && node->ns == ns) {
            if (exclude == false) {
                tree->open_elements->length++;
            }

            break;
        }
    }
}

void
pchtml_html_tree_open_elements_pop_until_h123456(pchtml_html_tree_t *tree)
{
    void **list = tree->open_elements->list;
    pcedom_node_t *node;

    while (tree->open_elements->length != 0) {
        tree->open_elements->length--;

        node = list[ tree->open_elements->length ];

        switch (node->local_name) {
            case PCHTML_TAG_H1:
            case PCHTML_TAG_H2:
            case PCHTML_TAG_H3:
            case PCHTML_TAG_H4:
            case PCHTML_TAG_H5:
            case PCHTML_TAG_H6:
                if (node->ns == PCHTML_NS_HTML) {
                    return;
                }

                break;

            default:
                break;
        }
    }
}

void
pchtml_html_tree_open_elements_pop_until_td_th(pchtml_html_tree_t *tree)
{
    void **list = tree->open_elements->list;
    pcedom_node_t *node;

    while (tree->open_elements->length != 0) {
        tree->open_elements->length--;

        node = list[ tree->open_elements->length ];

        switch (node->local_name) {
            case PCHTML_TAG_TD:
            case PCHTML_TAG_TH:
                if (node->ns == PCHTML_NS_HTML) {
                    return;
                }

                break;

            default:
                break;
        }
    }
}

void
pchtml_html_tree_open_elements_pop_until_node(pchtml_html_tree_t *tree,
                                           pcedom_node_t *node,
                                           bool exclude)
{
    void **list = tree->open_elements->list;

    while (tree->open_elements->length != 0) {
        tree->open_elements->length--;

        if (list[ tree->open_elements->length ] == node) {
            if (exclude == false) {
                tree->open_elements->length++;
            }

            break;
        }
    }
}

void
pchtml_html_tree_open_elements_pop_until(pchtml_html_tree_t *tree, size_t idx,
                                      bool exclude)
{
    tree->open_elements->length = idx;

    if (exclude == false) {
        tree->open_elements->length++;
    }
}

bool
pchtml_html_tree_open_elements_find_by_node(pchtml_html_tree_t *tree,
                                         pcedom_node_t *node,
                                         size_t *return_pos)
{
    void **list = tree->open_elements->list;

    for (size_t i = 0; i < tree->open_elements->length; i++) {
        if (list[i] == node) {
            if (return_pos) {
                *return_pos = i;
            }

            return true;
        }
    }

    if (return_pos) {
        *return_pos = 0;
    }

    return false;
}

bool
pchtml_html_tree_open_elements_find_by_node_reverse(pchtml_html_tree_t *tree,
                                                 pcedom_node_t *node,
                                                 size_t *return_pos)
{
    void **list = tree->open_elements->list;
    size_t len = tree->open_elements->length;

    while (len != 0) {
        len--;

        if (list[len] == node) {
            if (return_pos) {
                *return_pos = len;
            }

            return true;
        }
    }

    if (return_pos) {
        *return_pos = 0;
    }

    return false;
}

pcedom_node_t *
pchtml_html_tree_open_elements_find(pchtml_html_tree_t *tree,
                                 pchtml_tag_id_t tag_id, pchtml_ns_id_t ns,
                                 size_t *return_index)
{
    void **list = tree->open_elements->list;
    pcedom_node_t *node;

    for (size_t i = 0; i < tree->open_elements->length; i++) {
        node = list[i];

        if (node->local_name == tag_id && node->ns == ns) {
            if (return_index) {
                *return_index = i;
            }

            return node;
        }
    }

    if (return_index) {
        *return_index = 0;
    }

    return NULL;
}

pcedom_node_t *
pchtml_html_tree_open_elements_find_reverse(pchtml_html_tree_t *tree,
                                         pchtml_tag_id_t tag_id, pchtml_ns_id_t ns,
                                         size_t *return_index)
{
    void **list = tree->open_elements->list;
    size_t len = tree->open_elements->length;

    pcedom_node_t *node;

    while (len != 0) {
        len--;
        node = list[len];

        if (node->local_name == tag_id && node->ns == ns) {
            if (return_index) {
                *return_index = len;
            }

            return node;
        }
    }

    if (return_index) {
        *return_index = 0;
    }

    return NULL;
}
