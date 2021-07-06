/*
** Copyright (C) 2015-2017 Alexander Borisov
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
** Author: lex.borisov@gmail.com (Alexander Borisov)
*/

#ifndef MyHTML_SERIALIZATION_H
#define MyHTML_SERIALIZATION_H
#pragma once

#include "myosi.h"
#include "mystring.h"
#include "tree.h"

#ifdef __cplusplus
extern "C" {
#endif

// the serialization functions
mystatus_t myhtml_serialization_tree_buffer(myhtml_tree_node_t* scope_node, mycore_string_raw_t* str);
mystatus_t myhtml_serialization_node_buffer(myhtml_tree_node_t* node, mycore_string_raw_t* str);
mystatus_t myhtml_serialization_tree_callback(myhtml_tree_node_t* scope_node, mycore_callback_serialize_f callback, void* ptr);
mystatus_t myhtml_serialization_node_callback(myhtml_tree_node_t* node, mycore_callback_serialize_f callback, void* ptr);

// in version 1.0.3 this is in public api.
// Need to set deprecated?
mystatus_t myhtml_serialization(myhtml_tree_node_t* scope_node, mycore_string_raw_t* str);
mystatus_t myhtml_serialization_node(myhtml_tree_node_t* node, mycore_string_raw_t* str);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MyHTML_SERIALIZATION_H */
