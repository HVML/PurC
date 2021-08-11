/*
 * @file parser.h
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The header file of parameter lparser.
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
 *
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef _DVOJBS_INTERNAL_H_
#define _DVOJBS_INTERNAL_H_

#include "config.h"
#include "private/debug.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

enum node_type {
    NODE_TYPE_TOKEN,
    NODE_TYPE_ADD,
    NODE_TYPE_SUB,
    NODE_TYPE_MULTIPLE,
    NODE_TYPE_DIV,
    NODE_TYPE_MOD,
    NODE_TYPE_AND,
    NODE_TYPE_OR,
    NODE_TYPE_ONLY,
} ;

struct pcdvobjs_node {
    enum node_type type;
    unsigned char * text;
    struct pcdvobjs_node * first_child;
    struct pcdvobjs_node * next;
    // function
} pcdvobjs_node;
typedef struct pcdvobjs_node * pcdvobjs_node_t;

pcdvobjs_node_t pcdvobjs_parse (const unsigned char * data)  WTF_INTERNAL;
bool destroy_tree (pcdvobjs_node_t root)  WTF_INTERNAL;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  // _DVOJBS_INTERNAL_H_
