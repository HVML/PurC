/**
 * @file active_formatting.h 
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for active formatting.
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


#ifndef PCHTML_HTML_ACTIVE_FORMATTING_H
#define PCHTML_HTML_ACTIVE_FORMATTING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/array.h"

#include "html/html/tree.h"


pchtml_html_element_t *
pchtml_html_tree_active_formatting_marker(void);

void
pchtml_html_tree_active_formatting_up_to_last_marker(pchtml_html_tree_t *tree);

void
pchtml_html_tree_active_formatting_remove_by_node(pchtml_html_tree_t *tree,
                                               pchtml_dom_node_t *node);

bool
pchtml_html_tree_active_formatting_find_by_node(pchtml_html_tree_t *tree,
                                             pchtml_dom_node_t *node,
                                             size_t *return_pos);

bool
pchtml_html_tree_active_formatting_find_by_node_reverse(pchtml_html_tree_t *tree,
                                                     pchtml_dom_node_t *node,
                                                     size_t *return_pos);

unsigned int
pchtml_html_tree_active_formatting_reconstruct_elements(pchtml_html_tree_t *tree);

pchtml_dom_node_t *
pchtml_html_tree_active_formatting_between_last_marker(pchtml_html_tree_t *tree,
                                                    pchtml_tag_id_t tag_idx,
                                                    size_t *return_idx);

void
pchtml_html_tree_active_formatting_push_with_check_dupl(pchtml_html_tree_t *tree,
                                                     pchtml_dom_node_t *node);


/*
 * Inline functions
 */
static inline pchtml_dom_node_t *
pchtml_html_tree_active_formatting_current_node(pchtml_html_tree_t *tree)
{
    if (tree->active_formatting->length == 0) {
        return NULL;
    }

    return (pchtml_dom_node_t *) tree->active_formatting->list
        [ (tree->active_formatting->length - 1) ];
}

static inline pchtml_dom_node_t *
pchtml_html_tree_active_formatting_first(pchtml_html_tree_t *tree)
{
    return (pchtml_dom_node_t *) pchtml_array_get(tree->active_formatting, 0);
}

static inline pchtml_dom_node_t *
pchtml_html_tree_active_formatting_get(pchtml_html_tree_t *tree, size_t idx)
{
    return (pchtml_dom_node_t *) pchtml_array_get(tree->active_formatting, idx);
}

static inline unsigned int
pchtml_html_tree_active_formatting_push(pchtml_html_tree_t *tree,
                                     pchtml_dom_node_t *node)
{
    return pchtml_array_push(tree->active_formatting, node);
}

static inline pchtml_dom_node_t *
pchtml_html_tree_active_formatting_pop(pchtml_html_tree_t *tree)
{
    return (pchtml_dom_node_t *) pchtml_array_pop(tree->active_formatting);
}

static inline unsigned int
pchtml_html_tree_active_formatting_push_marker(pchtml_html_tree_t *tree)
{
    return pchtml_array_push(tree->active_formatting,
                             pchtml_html_tree_active_formatting_marker());
}

static inline unsigned int
pchtml_html_tree_active_formatting_insert(pchtml_html_tree_t *tree,
                                       pchtml_dom_node_t *node, size_t idx)
{
    return pchtml_array_insert(tree->active_formatting, idx, node);
}

static inline void
pchtml_html_tree_active_formatting_remove(pchtml_html_tree_t *tree, size_t idx)
{
    pchtml_array_delete(tree->active_formatting, idx, 1);
}


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_ACTIVE_FORMATTING_H */

