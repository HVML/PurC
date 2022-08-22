/*
 * @file tokenizer.h
 * @author Xue Shuming
 * @date 2022/08/22
 * @brief The api of ejson/jsonee tokenizer.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an EJSON interpreter.
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

#ifndef PURC_EJSON_TOKENIZER_H
#define PURC_EJSON_TOKENIZER_H

#include "config.h"

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"

#define PRINT_STATE(state_name)                                             \
    if (parser->enable_log) {                                               \
        size_t len;                                                         \
        char *s = pcvcm_node_to_string(parser->vcm_node, &len);             \
        PC_DEBUG(                                                           \
            "in %s|uc=%c|hex=0x%X|stack_is_empty=%d"                        \
            "|stack_top=%c|stack_size=%ld|vcm_node=%s\n",                   \
            curr_state_name, character, character,                          \
            ejson_stack_is_empty(), (char)ejson_stack_top(),                \
            ejson_stack_size(), s);                                         \
        free(s); \
    }

#define SET_ERR(err)    do {                                                \
    if (parser->curr_uc) {                                                  \
        char buf[ERROR_BUF_SIZE+1];                                         \
        snprintf(buf, ERROR_BUF_SIZE,                                       \
                "line=%d, column=%d, character=%c",                         \
                parser->curr_uc->line,                                      \
                parser->curr_uc->column,                                    \
                parser->curr_uc->character);                                \
        if (parser->enable_log) {                                           \
            PC_DEBUG( "%s:%d|%s|%s\n", __FILE__, __LINE__, #err, buf);      \
        }                                                                   \
    }                                                                       \
    tkz_set_error_info(parser->curr_uc, err);                               \
} while (0)

struct pcejson {
    int state;
    int return_state;
    uint32_t depth;
    uint32_t max_depth;
    uint32_t flags;

    struct tkz_uc* curr_uc;
    struct tkz_reader* tkz_reader;
    struct tkz_buffer* temp_buffer;
    struct tkz_buffer* string_buffer;
    struct pcvcm_node* vcm_node;
    struct pcvcm_stack* vcm_stack;
    struct pcutils_stack* ejson_stack;
    struct tkz_sbst* sbst;
    struct pcutils_stack* tkz_stack;
    uint32_t prev_separator;
    uint32_t nr_quoted;
    bool enable_log;
};


PCA_EXTERN_C_BEGIN

PCA_EXTERN_C_END

#endif /* PURC_EJSON_TOKENIZER_H */
