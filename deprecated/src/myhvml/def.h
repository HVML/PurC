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

#ifndef MyHVML_DEF_H
#define MyHVML_DEF_H

#pragma once

#define myhvml_parser_skip_whitespace()                                                      \
if(myhvml_whithspace(hvml[hvml_offset], ==, ||)) {                                           \
    while (hvml_offset < hvml_size && (myhvml_whithspace(hvml[hvml_offset], ==, ||))) {      \
        hvml_offset++;                                                                       \
    }                                                                                        \
}

#define myhvml_parser_queue_set_attr(tree, token_node)                              \
    if(token_node->attr_first == NULL) {                                            \
        token_node->attr_first = myhvml_tree_token_attr_current(tree);              \
        token_node->attr_last  = token_node->attr_first;                            \
                                                                                    \
        tree->attr_current = token_node->attr_last;                                 \
        tree->attr_current->next = NULL;                                            \
        tree->attr_current->prev = NULL;                                            \
    }                                                                               \
    else {                                                                          \
        token_node->attr_last->next = myhvml_tree_token_attr_current(tree);         \
        token_node->attr_last->next->prev = token_node->attr_last;                  \
        token_node->attr_last = token_node->attr_last->next;                        \
                                                                                    \
        token_node->attr_last->next = NULL;                                         \
        tree->attr_current = token_node->attr_last;                                 \
    }

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MyHVML_DEF_H */
