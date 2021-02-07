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
** Author: VincentWei <https://github.com/VincentWei>
*/

#ifndef MyHVML_MYHVML_H
#define MyHVML_MYHVML_H

#pragma once

#include "myosi.h"

#include "mycore/utils/mctree.h"
#include "mycore/utils/mcobject_async.h"
#include "mycore/mythread.h"
#include "mycore/incoming.h"
#include "mycore/charef.h"
#include "myencoding/encoding.h"
#include "tree.h"
#include "tag.h"
#include "def.h"
#include "parser.h"
#include "tokenizer.h"
#include "rules.h"
#include "token.h"
#include "callback.h"

#define mh_queue_current() tree->queue
#define myhvml_tokenizer_state_set(tree) myhvml_tree_set(tree, state)

#define mh_queue_get(idx, attr) myhvml->queue->nodes[idx].attr

// space, tab, LF, FF, CR
#define myhvml_whithspace(onechar, action, logic)    \
    onechar action ' '  logic                        \
    onechar action '\t' logic                        \
    onechar action '\n' logic                        \
    onechar action '\f' logic                        \
    onechar action '\r'

#define myhvml_ascii_char_cmp(onechar)      \
    ((onechar >= 'a' && onechar <= 'z') ||  \
    (onechar >= 'A' && onechar <= 'Z'))

#define myhvml_ascii_char_unless_cmp(onechar)         \
    ((onechar < 'a' || onechar > 'z') &&              \
    (onechar < 'A' || onechar > 'Z'))

struct myhvml {
    mythread_t* thread_stream;
    mythread_t* thread_batch;
    mythread_t* thread_list[3];
    size_t      thread_total;
    
    myhvml_tokenizer_state_f* parse_state_func;
    myhvml_insertion_f* insertion_func;
    
    enum myhvml_options opt;
    myhvml_tree_node_t *marker;
};

#ifdef __cplusplus
extern "C" {
#endif

bool myhvml_is_hvml_node(myhvml_tree_node_t *node, myhvml_tag_id_t tag_id);

mystatus_t myhvml_queue_add(myhvml_tree_t *tree, size_t begin, myhvml_token_node_t* token);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
