/*
** Copyright (C) 2021 FMSoft (https://www.fmsoft.cn)
** Copyright (C) 2015-2017 Alexander Borisov
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
** Author: Vincent Wei (https://github.com/VincentWei)
** Author: lex.borisov@gmail.com (Alexander Borisov)
*/

#ifndef MyHTML_CALLBACK_H
#define MyHTML_CALLBACK_H
#pragma once

#ifdef __cplusplus
extern "C" {
#endif
    
#include "myosi.h"
#include "tree.h"

/* callback functions */
myhtml_callback_token_f myhtml_callback_before_token_done(myhtml_tree_t *tree);
myhtml_callback_token_f myhtml_callback_after_token_done(myhtml_tree_t *tree);
void myhtml_callback_before_token_done_set(myhtml_tree_t *tree, myhtml_callback_token_f func, void* ctx);
void myhtml_callback_after_token_done_set(myhtml_tree_t *tree, myhtml_callback_token_f func, void* ctx);

myhtml_callback_tree_node_f myhtml_callback_tree_node_insert(myhtml_tree_t *tree);
myhtml_callback_tree_node_f myhtml_callback_tree_node_remove(myhtml_tree_t *tree);
void myhtml_callback_tree_node_insert_set(myhtml_tree_t *tree, myhtml_callback_tree_node_f func, void* ctx);
void myhtml_callback_tree_node_remove_set(myhtml_tree_t *tree, myhtml_callback_tree_node_f func, void* ctx);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MyHTML_CALLBACK_H */
