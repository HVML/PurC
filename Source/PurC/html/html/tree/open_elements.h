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
 */


#ifndef PCHTML_HTML_OPEN_ELEMENTS_H
#define PCHTML_HTML_OPEN_ELEMENTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/array.h"

#include "html/html/tree.h"


void
pchtml_html_tree_open_elements_remove_by_node(pchtml_html_tree_t *tree,
                    pcedom_node_t *node) WTF_INTERNAL;

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
                                           pcedom_node_t *node,
                                           bool exclude);

void
pchtml_html_tree_open_elements_pop_until(pchtml_html_tree_t *tree, size_t idx,
                                      bool exclude);

bool
pchtml_html_tree_open_elements_find_by_node(pchtml_html_tree_t *tree,
                                         pcedom_node_t *node,
                                         size_t *return_pos);

bool
pchtml_html_tree_open_elements_find_by_node_reverse(pchtml_html_tree_t *tree,
                                                 pcedom_node_t *node,
                                                 size_t *return_pos);

pcedom_node_t *
pchtml_html_tree_open_elements_find(pchtml_html_tree_t *tree,
                                 pchtml_tag_id_t tag_id, pchtml_ns_id_t ns,
                                 size_t *return_index);

pcedom_node_t *
pchtml_html_tree_open_elements_find_reverse(pchtml_html_tree_t *tree,
                                         pchtml_tag_id_t tag_id, pchtml_ns_id_t ns,
                                         size_t *return_index);


/*
 * Inline functions
 */
static inline pcedom_node_t *
pchtml_html_tree_open_elements_first(pchtml_html_tree_t *tree)
{
    return (pcedom_node_t *) pchtml_array_get(tree->open_elements, 0);
}

static inline pcedom_node_t *
pchtml_html_tree_open_elements_get(pchtml_html_tree_t *tree, size_t idx)
{
    return (pcedom_node_t *) pchtml_array_get(tree->open_elements, idx);
}

static inline unsigned int
pchtml_html_tree_open_elements_push(pchtml_html_tree_t *tree, pcedom_node_t *node)
{
    return pchtml_array_push(tree->open_elements, node);
}

static inline pcedom_node_t *
pchtml_html_tree_open_elements_pop(pchtml_html_tree_t *tree)
{
    return (pcedom_node_t *) pchtml_array_pop(tree->open_elements);
}

static inline unsigned int
pchtml_html_tree_open_elements_insert_after(pchtml_html_tree_t *tree, pcedom_node_t *node,
                                    size_t idx)
{
    return pchtml_array_insert(tree->open_elements, (idx + 1), node);
}


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_OPEN_ELEMENTS_H */

