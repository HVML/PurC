/*
 * @file tokenizer.c
 * @author Xue Shuming
 * @date 2022/08/22
 * @brief The implementation of ejson/jsonee tokenizer.
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
 */

#include "config.h"

#include "tokenizer.h"

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/stack.h"
#include "private/tkz-helper.h"

#include <math.h>

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

#define ERROR_BUF_SIZE  100
#define NR_CONSUMED_LIST_LIMIT   10

#define INVALID_CHARACTER    0xFFFFFFFF

#define PCEJSON_PARSER_BEGIN                                                \
int pcejson_parse_n(struct pcvcm_node **vcm_tree,                           \
        struct pcejson **parser_param,                                      \
        purc_rwstream_t rws,                                                \
        uint32_t depth)                                                     \
{                                                                           \
    if (*parser_param == NULL) {                                            \
        *parser_param = pcejson_create(                                     \
                depth > 0 ? depth : EJSON_MAX_DEPTH, 1);                    \
        if (*parser_param == NULL) {                                        \
            return -1;                                                      \
        }                                                                   \
    }                                                                       \
                                                                            \
    uint32_t character = 0;                                                 \
    struct pcejson* parser = *parser_param;                                 \
    tkz_reader_set_rwstream (parser->tkz_reader, rws);                      \
                                                                            \
next_input:                                                                 \
    parser->curr_uc = tkz_reader_next_char (parser->tkz_reader);            \
    if (!parser->curr_uc) {                                                 \
        return -1;                                                          \
    }                                                                       \
                                                                            \
    character = parser->curr_uc->character;                                 \
    if (character == INVALID_CHARACTER) {                                   \
        SET_ERR(PURC_ERROR_BAD_ENCODING);                                   \
        return -1;                                                          \
    }                                                                       \
                                                                            \
    if (is_separator(character)) {                                          \
        if (parser->prev_separator == ',' && character == ',') {            \
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_COMMA);                        \
            return -1;                                                      \
        }                                                                   \
        parser->prev_separator = character;                                 \
    }                                                                       \
    else if (!is_whitespace(character)) {                                   \
        parser->prev_separator = 0;                                         \
    }                                                                       \
                                                                            \
next_state:                                                                 \
    switch (parser->state) {

#define PCEJSON_PARSER_END                                                  \
    default:                                                                \
        break;                                                              \
    }                                                                       \
    return -1;                                                              \
}


