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

#include "private/ejson.h"
#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"

#define EJSON_MAX_DEPTH              32
#define EJSON_MIN_BUFFER_SIZE        128
#define EJSON_MAX_BUFFER_SIZE        1024 * 1024 * 1024
#define EJSON_END_OF_FILE            0
#define PURC_ENVV_EJSON_LOG_ENABLE  "PURC_EJSON_LOG_ENABLE"

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

#define BEGIN_STATE(state_name)                                             \
    case state_name:                                                        \
    {                                                                       \
        const char *curr_state_name = ""#state_name;                        \
        int curr_state = state_name;                                        \
        UNUSED_PARAM(curr_state_name);                                      \
        UNUSED_PARAM(curr_state);                                           \
        PRINT_STATE(curr_state);

#define END_STATE()                                                         \
        break;                                                              \
    }

#define ADVANCE_TO(new_state)                                               \
    do {                                                                    \
        parser->state = new_state;                                          \
        goto next_input;                                                    \
    } while (false)

#define RECONSUME_IN(new_state)                                             \
    do {                                                                    \
        parser->state = new_state;                                          \
        goto next_state;                                                    \
    } while (false)

#define SET_RETURN_STATE(new_state)                                         \
    do {                                                                    \
        parser->return_state = new_state;                                   \
    } while (false)

#define RETURN_AND_STOP_PARSE()                                             \
    do {                                                                    \
        return -1;                                                          \
    } while (false)

struct pcejson_token {
    uint32_t type;
    struct pcvcm_node *node;
};

struct pcejson_token_stack {
    struct pcutils_stack *stack;
};

enum pcejson_tkz_state {
    EJSON_TKZ_STATE_FIRST = 1000,

    EJSON_TKZ_STATE_DATA = EJSON_TKZ_STATE_FIRST,
    EJSON_TKZ_STATE_FINISHED,
    EJSON_TKZ_STATE_CONTROL,

    EJSON_TKZ_STATE_LAST = EJSON_TKZ_STATE_CONTROL,
};

struct pcejson {
    uint32_t state;
    uint32_t return_state;
    uint32_t depth;
    uint32_t max_depth;
    uint32_t flags;

    struct tkz_uc *curr_uc;
    struct tkz_reader *tkz_reader;
    struct tkz_buffer *temp_buffer;
    struct tkz_buffer *string_buffer;
    struct pcvcm_node *vcm_node;
    struct pcvcm_stack *vcm_stack;
    struct pcutils_stack *ejson_stack;
    struct tkz_sbst *sbst;

    struct pcejson_token_stack *tkz_stack;

    uint32_t prev_separator;
    uint32_t nr_quoted;
    bool enable_log;
};

PCA_EXTERN_C_BEGIN

struct pcejson_token *
pcejson_token_new(uint32_t type);

void
pcejson_token_destroy(struct pcejson_token *token);


struct pcejson_token_stack *
pcejson_token_stack_new();

int
pcejson_token_stack_destroy(struct pcejson_token_stack *stack);

bool
pcejson_token_stack_is_empty(struct pcejson_token_stack *stack);

int
pcejson_token_stack_push_simple(struct pcejson_token_stack *stack,
        uint32_t type);

int
pcejson_token_stack_push(struct pcejson_token_stack *stack,
        struct pcejson_token *token);

struct pcejson_token *
pcejson_token_stack_pop(struct pcejson_token_stack *stack);

struct pcejson_token *
pcejson_token_stack_top(struct pcejson_token_stack *stack);

int
pcejson_token_stack_size(struct pcejson_token_stack *stack);

int
pcejson_token_stack_clear(struct pcejson_token_stack *stack);

PCA_EXTERN_C_END

#endif /* PURC_EJSON_TOKENIZER_H */
