/**
 * @file open_elements.h 
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html openning element.
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


#ifndef PCHTML_HTML_OPEN_ELEMENTS_H
#define PCHTML_HTML_OPEN_ELEMENTS_H

#include "config.h"

#include "private/array.h"
#include "html/tree.h"

#ifdef __cplusplus
extern "C" {
#endif

void
pchtml_html_tree_open_elements_remove_by_node(pchtml_html_tree_t *tree,
                    pcdom_node_t *node) WTF_INTERNAL;

void
pchtml_html_tree_open_elements_pop_until_tag_id(pchtml_html_tree_t *tree,
                                             pchtml_tag_id_t tag_id,
                                             pchtml_ns_id_t ns,
                                             bool exclude);

void
pchtml_html_tree_open_elements_pop_until_h123456(pchtml_html_tree_t *tree);

void
pchtml_html_tree_open_elements_pop_until_td_th(pchtml_html_tree_t *tree);

void
pchtml_html_tree_open_elements_pop_until_node(pchtml_html_tree_t *tree,
                                           pcdom_node_t *node,
                                           bool exclude);

void
pchtml_html_tree_open_elements_pop_until(pchtml_html_tree_t *tree, size_t idx,
                                      bool exclude);

bool
pchtml_html_tree_open_elements_find_by_node(pchtml_html_tree_t *tree,
                                         pcdom_node_t *node,
                                         size_t *return_pos);

bool
pchtml_html_tree_open_elements_find_by_node_reverse(pchtml_html_tree_t *tree,
                                                 pcdom_node_t *node,
                                                 size_t *return_pos);

pcdom_node_t *
pchtml_html_tree_open_elements_find(pchtml_html_tree_t *tree,
                                 pchtml_tag_id_t tag_id, pchtml_ns_id_t ns,
                                 size_t *return_index);

pcdom_node_t *
pchtml_html_tree_open_elements_find_reverse(pchtml_html_tree_t *tree,
                                         pchtml_tag_id_t tag_id, pchtml_ns_id_t ns,
                                         size_t *return_index);


/*
 * Inline functions
 */
static inline pcdom_node_t *
pchtml_html_tree_open_elements_first(pchtml_html_tree_t *tree)
{
    return (pcdom_node_t *) pcutils_array_get(tree->open_elements, 0);
}

static inline pcdom_node_t *
pchtml_html_tree_open_elements_get(pchtml_html_tree_t *tree, size_t idx)
{
    return (pcdom_node_t *) pcutils_array_get(tree->open_elements, idx);
}

static inline unsigned int
pchtml_html_tree_open_elements_push(pchtml_html_tree_t *tree, pcdom_node_t *node)
{
    return pcutils_array_push(tree->open_elements, node);
}

static inline pcdom_node_t *
pchtml_html_tree_open_elements_pop(pchtml_html_tree_t *tree)
{
    return (pcdom_node_t *) pcutils_array_pop(tree->open_elements);
}

static inline unsigned int
pchtml_html_tree_open_elements_insert_after(pchtml_html_tree_t *tree, pcdom_node_t *node,
                                    size_t idx)
{
    return pcutils_array_insert(tree->open_elements, (idx + 1), node);
}


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_OPEN_ELEMENTS_H */

