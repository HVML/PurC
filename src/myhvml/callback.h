/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of Purring Cat 2, a HVML parser and interpreter.
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

#ifndef MyHVML_CALLBACK_H
#define MyHVML_CALLBACK_H

#pragma once

#include "myosi.h"
#include "tree.h"

#ifdef __cplusplus
extern "C" {
#endif
    
/* callback functions */
myhvml_callback_token_f myhvml_callback_before_token_done(myhvml_tree_t *tree);
myhvml_callback_token_f myhvml_callback_after_token_done(myhvml_tree_t *tree);
void myhvml_callback_before_token_done_set(myhvml_tree_t *tree, myhvml_callback_token_f func, void* ctx);
void myhvml_callback_after_token_done_set(myhvml_tree_t *tree, myhvml_callback_token_f func, void* ctx);

myhvml_callback_tree_node_f myhvml_callback_tree_node_insert(myhvml_tree_t *tree);
myhvml_callback_tree_node_f myhvml_callback_tree_node_remove(myhvml_tree_t *tree);
void myhvml_callback_tree_node_insert_set(myhvml_tree_t *tree, myhvml_callback_tree_node_f func, void* ctx);
void myhvml_callback_tree_node_remove_set(myhvml_tree_t *tree, myhvml_callback_tree_node_f func, void* ctx);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MyHVML_CALLBACK_H */
