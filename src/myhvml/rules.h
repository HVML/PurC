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

#ifndef MyHVML_RULES_H
#define MyHVML_RULES_H

#pragma once

#include "myosi.h"
#include "myhvml_internals.h"
#include "tree.h"

#ifdef __cplusplus
extern "C" {
#endif

mystatus_t myhvml_rules_init(myhvml_t* myhvml);
void myhvml_rules_stop_parsing(myhvml_tree_t* tree);
bool myhvml_rules_tree_dispatcher(myhvml_tree_t* tree, myhvml_token_node_t* token);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MyHVML_RULES_H */
