/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML parser
** and interpreter.
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
**
** Author: Vincent Wei <https://github.com/VincentWei>
*/

#include "callback.h"


/* callback functions for tokens */
myhvml_callback_token_f myhvml_callback_before_token_done(myhvml_tree_t *tree)
{
    return tree->callback_before_token;
}

myhvml_callback_token_f myhvml_callback_after_token_done(myhvml_tree_t *tree)
{
    return tree->callback_after_token;
}

void myhvml_callback_before_token_done_set(myhvml_tree_t *tree, myhvml_callback_token_f func, void* ctx)
{
    tree->callback_before_token = func;
    tree->callback_before_token_ctx = ctx;
}

void myhvml_callback_after_token_done_set(myhvml_tree_t *tree, myhvml_callback_token_f func, void* ctx)
{
    tree->callback_after_token = func;
    tree->callback_after_token_ctx = ctx;
}

myhvml_callback_tree_node_f myhvml_callback_tree_node_insert(myhvml_tree_t *tree)
{
    return tree->callback_tree_node_insert;
}

myhvml_callback_tree_node_f myhvml_callback_tree_node_remove(myhvml_tree_t *tree)
{
    return tree->callback_tree_node_remove;
}

void myhvml_callback_tree_node_insert_set(myhvml_tree_t *tree, myhvml_callback_tree_node_f func, void* ctx)
{
    tree->callback_tree_node_insert = func;
    tree->callback_tree_node_insert_ctx = ctx;
}

void myhvml_callback_tree_node_remove_set(myhvml_tree_t *tree, myhvml_callback_tree_node_f func, void* ctx)
{
    tree->callback_tree_node_remove = func;
    tree->callback_tree_node_remove_ctx = ctx;
}


