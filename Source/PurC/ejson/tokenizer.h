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

#define RESET_TEMP_BUFFER()                                                 \
    do {                                                                    \
        tkz_buffer_reset(parser->temp_buffer);                              \
    } while (false)

#define APPEND_TO_TEMP_BUFFER(c)                                            \
    do {                                                                    \
        tkz_buffer_append(parser->temp_buffer, c);                          \
    } while (false)

#define APPEND_BYTES_TO_TEMP_BUFFER(bytes, nr_bytes)                        \
    do {                                                                    \
        tkz_buffer_append_bytes(parser->temp_buffer, bytes, nr_bytes);      \
    } while (false)

#define APPEND_BUFFER_TO_TEMP_BUFFER(buffer)                                \
    do {                                                                    \
        tkz_buffer_append_another(parser->temp_buffer, buffer);             \
    } while (false)

#define IS_TEMP_BUFFER_EMPTY()                                              \
        tkz_buffer_is_empty(parser->temp_buffer)


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
    EJSON_TKZ_STATE_AFTER_TOKEN,
    EJSON_TKZ_STATE_LEFT_BRACE,
    EJSON_TKZ_STATE_RIGHT_BRACE,
    EJSON_TKZ_STATE_LEFT_BRACKET,
    EJSON_TKZ_STATE_RIGHT_BRACKET,
    EJSON_TKZ_STATE_LEFT_PARENTHESIS,
    EJSON_TKZ_STATE_RIGHT_PARENTHESIS,
    EJSON_TKZ_STATE_DOLLAR,
    EJSON_TKZ_STATE_AMPERSAND,
    EJSON_TKZ_STATE_OR_SIGN,
    EJSON_TKZ_STATE_SEMICOLON,
    EJSON_TKZ_STATE_SINGLE_QUOTED,
    EJSON_TKZ_STATE_DOUBLE_QUOTED,
    EJSON_TKZ_STATE_UNQUOTED,
    EJSON_TKZ_STATE_BEFORE_NAME,

    EJSON_TKZ_STATE_CJSONEE_FINISHED,

    EJSON_TKZ_STATE_LAST = EJSON_TKZ_STATE_CJSONEE_FINISHED,
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

struct pcejson_token *
pcejson_token_stack_push_simple(struct pcejson_token_stack *stack,
        uint32_t type);

struct pcejson_token *
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

static inline
bool pcejson_inc_depth (struct pcejson* parser)
{
    parser->depth++;
    return parser->depth <= parser->max_depth;
}

static inline
void pcejson_dec_depth (struct pcejson* parser)
{
    if (parser->depth > 0) {
        parser->depth--;
    }
}


PCA_EXTERN_C_END

#endif /* PURC_EJSON_TOKENIZER_H */
