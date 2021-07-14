/*
 * @file tree.c
 * @author XueShuming
 * @date 2021/07/02
 * @brief The API for tree.
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

#include "private/tree.h"
#include "private/errors.h"
#include "config.h"

bool purc_tree_node_append_child (purc_tree_node_t parent,
        purc_tree_node_t node)
{
    UNUSED_PARAM(parent);
    UNUSED_PARAM(node);
    return false;
}

bool purc_tree_node_prepend_child (purc_tree_node_t parent,
        purc_tree_node_t node)
{
    UNUSED_PARAM(parent);
    UNUSED_PARAM(node);
    return false;
}

bool purc_tree_node_insert_before (purc_tree_node_t current,
        purc_tree_node_t node)
{
    UNUSED_PARAM(current);
    UNUSED_PARAM(node);
    return false;
}

bool purc_tree_node_insert_after (purc_tree_node_t current,
        purc_tree_node_t node)
{
    UNUSED_PARAM(current);
    UNUSED_PARAM(node);
    return false;
}

purc_tree_node_t purc_tree_node_parent (purc_tree_node_t node)
{
    UNUSED_PARAM(node);
    return NULL;
}

purc_tree_node_t purc_tree_node_child (purc_tree_node_t node)
{
    UNUSED_PARAM(node);
    return NULL;
}

purc_tree_node_t purc_tree_node_last_child (purc_tree_node_t node)
{
    UNUSED_PARAM(node);
    return NULL;
}

purc_tree_node_t purc_tree_node_next (purc_tree_node_t node)
{
    UNUSED_PARAM(node);
    return NULL;
}

purc_tree_node_t purc_tree_node_prev (purc_tree_node_t node)
{
    UNUSED_PARAM(node);
    return NULL;
}

size_t purc_tree_node_children_number (purc_tree_node_t node)
{
    UNUSED_PARAM(node);
    return 0;
}

unsigned int purc_tree_node_type (purc_tree_node_t node)
{
    UNUSED_PARAM(node);
    return 0;
}

void purc_tree_node_children_for_each (purc_tree_node_t node,
        purc_tree_node_for_each_fn* func, void* data)
{
    UNUSED_PARAM(node);
    UNUSED_PARAM(func);
    UNUSED_PARAM(data);
}

void purc_tree_node_traverse (purc_tree_node_t node,
        purc_tree_node_for_each_fn* func, void* data)
{
    UNUSED_PARAM(node);
    UNUSED_PARAM(func);
    UNUSED_PARAM(data);
}
