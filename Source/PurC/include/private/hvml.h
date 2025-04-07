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

struct tkz_uc;
struct tkz_buffer;
struct pchvml_token;
struct tkz_reader;
struct tkz_sbst;
struct pcejson;

struct pchvml_parser {
    unsigned int state;
    unsigned int return_state;
    unsigned int transit_state;
    unsigned int last_token_type;

    uint64_t char_ref_code;
    uint32_t prev_separator;
    uint32_t nr_whitespace;
    uint32_t nr_single_quoted;
    uint32_t nr_double_quoted;
    uint32_t ejson_parser_max_depth;
    uint32_t ejson_parser_flags;

    unsigned int tag_is_operation:1;
    unsigned int tag_has_raw_attr:1;
    unsigned int enable_log:1;
    unsigned int is_in_file_header:1;
    unsigned int record_ucs:1;

    struct tkz_uc* curr_uc;
    struct tkz_reader* reader;
    struct tkz_lc*     lc;
    struct tkz_buffer* temp_buffer;
    struct tkz_buffer* tag_name;
    struct tkz_buffer* string_buffer;

    struct tkz_ucs*    temp_ucs;        /* keep ucs for ejson parser */


    struct pchvml_token* token;
    struct tkz_sbst* sbst;
    struct pcutils_stack* ejson_stack;
    struct pchvml_token* start_tag_token;

    struct pcejson *ejson_parser;
};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

struct pchvml_parser* pchvml_create(uint32_t flags, size_t queue_size,
        purc_rwstream_t rws);

void pchvml_reset(struct pchvml_parser* parser, uint32_t flags,
        size_t queue_size);

void pchvml_destroy(struct pchvml_parser* parser);

struct pchvml_token* pchvml_next_token(struct pchvml_parser* hvml);

void pchvml_switch_to_ejson_state(struct pchvml_parser* parser);

int pchvml_parser_get_curr_pos(struct pchvml_parser* parser,
    uint32_t *character, int *line, int *column, int *position);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_HVML_H */

