/**
 * @file active_formatting.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of html formatting.
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

#include "html/tree/active_formatting.h"
#include "html/tree/open_elements.h"
#include "html/interfaces/element.h"


static pchtml_html_element_t pchtml_html_tree_active_formatting_marker_static;

static pcdom_node_t *pchtml_html_tree_active_formatting_marker_node_static =
    (pcdom_node_t *) &pchtml_html_tree_active_formatting_marker_static;


pchtml_html_element_t *
pchtml_html_tree_active_formatting_marker(void)
{
    return &pchtml_html_tree_active_formatting_marker_static;
}

void
pchtml_html_tree_active_formatting_up_to_last_marker(pchtml_html_tree_t *tree)
{
    void **list = tree->active_formatting->list;

    while (tree->active_formatting->length != 0) {
        tree->active_formatting->length--;

        if (list[tree->active_formatting->length]
            == &pchtml_html_tree_active_formatting_marker_static)
        {
            break;
        }
    }
}

void
pchtml_html_tree_active_formatting_remove_by_node(pchtml_html_tree_t *tree,
                                               pcdom_node_t *node)
{
    size_t delta;
    void **list = tree->active_formatting->list;
    size_t idx = tree->active_formatting->length;

    while (idx != 0) {
        idx--;

        if (list[idx] == node) {
            delta = tree->active_formatting->length - idx - 1;

            memmove(list + idx, list + idx + 1, sizeof(void *) * delta);

            tree->active_formatting->length--;

            break;
        }
    }
}

bool
pchtml_html_tree_active_formatting_find_by_node(pchtml_html_tree_t *tree,
                                             pcdom_node_t *node,
                                             size_t *return_pos)
{
    void **list = tree->active_formatting->list;

    for (size_t i = 0; i < tree->active_formatting->length; i++) {
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
pchtml_html_tree_active_formatting_find_by_node_reverse(pchtml_html_tree_t *tree,
                                                     pcdom_node_t *node,
                                                     size_t *return_pos)
{
    void **list = tree->active_formatting->list;
    size_t len = tree->active_formatting->length;

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

unsigned int
pchtml_html_tree_active_formatting_reconstruct_elements(pchtml_html_tree_t *tree)
{
    /* Step 1 */
    if (tree->active_formatting->length == 0) {
        return PCHTML_STATUS_OK;
    }

    pcutils_array_t *af = tree->active_formatting;
    void **list = af->list;

    /* Step 2-3 */
    size_t af_idx = af->length - 1;

    if(list[af_idx] == &pchtml_html_tree_active_formatting_marker_static
       || pchtml_html_tree_open_elements_find_by_node_reverse(tree, list[af_idx],
                                                           NULL))
    {
        return PCHTML_STATUS_OK;
    }

    /*
     * Step 4-6
     * Rewind
     */
    while (af_idx != 0) {
        af_idx--;

        if(list[af_idx] == &pchtml_html_tree_active_formatting_marker_static ||
           pchtml_html_tree_open_elements_find_by_node_reverse(tree, list[af_idx],
                                                            NULL))
        {
            /* Step 7 */
            af_idx++;

            break;
        }
    }

    /*
     * Step 8-10
     * Create
     */
    pcdom_node_t *node;
    pchtml_html_element_t *element;
    pchtml_html_token_t fake_token = {0};

    while (af_idx < af->length) {
        node = list[af_idx];

        fake_token.tag_id = node->local_name;
        fake_token.base_element = node;

        element = pchtml_html_tree_insert_html_element(tree, &fake_token);
        if (element == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        /* Step 9 */
        list[af_idx] = pcdom_interface_node(element);

        /* Step 10 */
        af_idx++;
    }

    return PCHTML_STATUS_OK;
}

pcdom_node_t *
pchtml_html_tree_active_formatting_between_last_marker(pchtml_html_tree_t *tree,
                                                    pchtml_tag_id_t tag_idx,
                                                    size_t *return_idx)
{
    pcdom_node_t **list = (pcdom_node_t **) tree->active_formatting->list;
    size_t idx = tree->active_formatting->length;

    while (idx) {
        idx--;

        if (list[idx] == pchtml_html_tree_active_formatting_marker_node_static) {
            return NULL;
        }

        if (list[idx]->local_name == tag_idx && list[idx]->ns == PCHTML_NS_HTML) {
            if (return_idx) {
                *return_idx = idx;
            }

            return list[idx];
        }
    }

    return NULL;
}

void
pchtml_html_tree_active_formatting_push_with_check_dupl(pchtml_html_tree_t *tree,
                                                     pcdom_node_t *node)
{
    pcdom_node_t **list = (pcdom_node_t **) tree->active_formatting->list;
    size_t idx = tree->active_formatting->length;
    size_t earliest_idx = (idx ? (idx - 1) : 0);
    size_t count = 0;

    while (idx) {
        idx--;

        if (list[idx] == pchtml_html_tree_active_formatting_marker_node_static) {
            break;
        }

        if(list[idx]->local_name == node->local_name && list[idx]->ns == node->ns
            && pcdom_element_compare(pcdom_interface_element(list[idx]),
                                       pcdom_interface_element(node)))
        {
            count++;
            earliest_idx = idx;
        }
    }

    if(count >= 3) {
        pchtml_html_tree_active_formatting_remove(tree, earliest_idx);
    }

    pchtml_html_tree_active_formatting_push(tree, node);
}
