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

void pctree_node_children_for_each (pctree_node_t node,
        pctree_node_for_each_fn* func, void* data)
{
    node = node->first_child;
    while (node) {
        pctree_node_t current = node;
        node = current->next;
        func (current, data);
    }
}

void pctree_node_traverse (pctree_node_t node,
        pctree_node_for_each_fn* func, void* data)
{
    func (node, data);
    node = node->first_child;
    while (node) {
        pctree_node_t current = node;
        node = current->next;
        pctree_node_traverse (current, func, data);
    }
}
