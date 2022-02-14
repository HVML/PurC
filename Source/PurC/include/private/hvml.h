/**
 * @file hvml.h
 * @author Xu Xiaohong
 * @date 2021/08/23
 * @brief The internal interfaces for hvml parser.
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

#ifndef PURC_PRIVATE_HVML_H
#define PURC_PRIVATE_HVML_H

#include "purc-rwstream.h"
#include "private/stack.h"
#include "private/vcm.h"

struct pchvml_uc;
struct pchvml_buffer;
struct pchvml_token;
struct pchvml_rwswrap;
struct pchvml_sbst;

struct pchvml_parser {
    int state;
    int return_state;
    int transit_state;
    struct pchvml_uc* curr_uc;
    struct pchvml_rwswrap* rwswrap;
    struct pchvml_buffer* temp_buffer;
    struct pchvml_buffer* tag_name;
    struct pchvml_buffer* string_buffer;
    struct pchvml_buffer* quoted_buffer;    /* remove */
    struct pchvml_token* token;
    struct pchvml_sbst* sbst;
    struct pcvcm_node* vcm_node;
    struct pcvcm_stack* vcm_stack;
    struct pcutils_stack* ejson_stack;
    uint64_t char_ref_code;
    uint32_t prev_separator;
    uint32_t nr_quoted;
    bool tag_is_operation;
    bool enable_print_log;
};

#define USE_NEW_TOKENIZER

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void pchvml_init_once (void);

struct pchvml_parser* pchvml_create(uint32_t flags, size_t queue_size);

void pchvml_reset(struct pchvml_parser* parser, uint32_t flags,
        size_t queue_size);

void pchvml_destroy(struct pchvml_parser* parser);

struct pchvml_token* pchvml_next_token(struct pchvml_parser* hvml,
                                          purc_rwstream_t rws);

void pchvml_switch_to_ejson_state(struct pchvml_parser* parser);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_HVML_H */

