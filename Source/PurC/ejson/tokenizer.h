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
#include "private/tkz-helper.h"

#define USE_NEW_PARSER               1
#define __DEV_EJSON__                0

#define EJSON_MAX_DEPTH              32
#define EJSON_MIN_BUFFER_SIZE        128
#define EJSON_MAX_BUFFER_SIZE        1024 * 1024 * 1024
#define EJSON_END_OF_FILE            0
#define PURC_ENVV_EJSON_LOG_ENABLE  "PURC_EJSON_LOG_ENABLE"

/* EJSON_TOKEN_TYPE */
#define ETT_OBJECT                      '{'         /* { "key":"value" } */
#define ETT_ARRAY                       '['         /* [value, value] */
#define ETT_GET_VARIABLE                '$'         /* $VARIABLE */
#define ETT_GET_ELEMENT                 '.'         /* $VARIABLE.ELEMENT */
#define ETT_GET_ELEMENT_BY_BRACKET      'E'         /* $VARIABLE[ELEMENT] */
#define ETT_CALL_GETTER                 '('         /* $VARIABLE() */
#define ETT_CALL_SETTER                 '<'         /* $VARIABLE(!value) */
#define ETT_PROTECT                     'P'         /* For {{ */
#define ETT_CJSONEE                     'C'         /* CJSONEE */
#define ETT_MULTI_QUOTED_S              'M'         /* multiple quoted string */
#define ETT_MULTI_UNQUOTED_S            'N'         /* multiple unquoted string */
#define ETT_KEY                         'K'         /* object key */
#define ETT_VALUE                       'V'         /* value */
#define ETT_DOUBLE_S                    'D'         /* double quoted string*/
#define ETT_SINGLE_S                    'S'         /* single quoted string*/
#define ETT_UNQUOTED_S                  'U'         /* unquoted string*/
#define ETT_KEYWORD                     'W'         /* keyword : true, false ... */
#define ETT_AND                         '&'         /* CJSONEE OP: && */
#define ETT_OR                          '|'         /* CJSONEE OP: || */
#define ETT_SEMICOLON                   ';'         /* CJSONEE OP: ; */
#define ETT_STRING                      '"'         /* String: temp  */


#if (defined __DEV_EJSON__ && __DEV_EJSON__)
#define PLOG(format, ...)  fprintf(stderr, "#####>"format, ##__VA_ARGS__);
#else
#define PLOG               PC_DEBUG
#endif /* (defined __DEV_EJSON__ && __DEV_EJSON__) */

#define PLINE()            PLOG("%s:%d:%s\n", __FILE__, __LINE__, __func__)

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

#define BEGIN_STATE(state)                                                  \
    case state:                                                             \
    {                                                                       \
        parser->state_name = #state;                                        \
        int curr_state = state;                                             \
        UNUSED_VARIABLE(curr_state);                                        \
        PRINT_STATE(parser);

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

// origin parser begin

#define ejson_stack_is_empty()  pcutils_stack_is_empty(parser->ejson_stack)
#define ejson_stack_top()  pcutils_stack_top(parser->ejson_stack)
#define ejson_stack_pop()  pcutils_stack_pop(parser->ejson_stack)
#define ejson_stack_push(c) pcutils_stack_push(parser->ejson_stack, c)
#define ejson_stack_size() pcutils_stack_size(parser->ejson_stack)
#define ejson_stack_reset() pcutils_stack_clear(parser->ejson_stack)

#define vcm_stack_is_empty() pcvcm_stack_is_empty(parser->vcm_stack)
#define vcm_stack_push(c) pcvcm_stack_push(parser->vcm_stack, c)
#define vcm_stack_pop() pcvcm_stack_pop(parser->vcm_stack)
#define vcm_stack_parent() pcvcm_stack_bottommost(parser->vcm_stack)

#define RESET_STRING_BUFFER()                                               \
    do {                                                                    \
        tkz_buffer_reset(parser->string_buffer);                             \
    } while (false)

#define APPEND_TO_STRING_BUFFER(uc)                                         \
    do {                                                                    \
        tkz_buffer_append(parser->string_buffer, uc);                        \
    } while (false)

#define RESET_QUOTED_COUNTER()                                              \
    do {                                                                    \
        parser->nr_quoted = 0;                                              \
    } while (false)

#define UPDATE_VCM_NODE(node)                                                  \
    do {                                                                    \
        if (node) {                                                         \
            parser->vcm_node = node;                                        \
        }                                                                   \
    } while (false)

#define RESET_VCM_NODE()                                                    \
    do {                                                                    \
        parser->vcm_node = NULL;                                            \
    } while (false)

#define RESTORE_VCM_NODE()                                                  \
    do {                                                                    \
        if (!parser->vcm_node) {                                            \
            parser->vcm_node = pcvcm_stack_pop(parser->vcm_stack);          \
        }                                                                   \
    } while (false)

#define APPEND_CHILD(parent, child)                                         \
    do {                                                                    \
        if (parent && child) {                                              \
            pctree_node_append_child((struct pctree_node*)parent,           \
                (struct pctree_node*)child);                                \
        }                                                                   \
    } while (false)

#define APPEND_AS_VCM_CHILD(node)                                           \
    do {                                                                    \
        if (parser->vcm_node) {                                             \
            pctree_node_append_child((struct pctree_node*)parser->vcm_node, \
                (struct pctree_node*)node);                                 \
        }                                                                   \
        else {                                                              \
            parser->vcm_node = node;                                        \
        }                                                                   \
    } while (false)

#define POP_AS_VCM_PARENT_AND_UPDATE_VCM()                                  \
    do {                                                                    \
        struct pcvcm_node* parent = pcvcm_stack_pop(parser->vcm_stack);     \
        if (parent && pcvcm_node_is_closed(parent)) {                       \
            struct pcvcm_node* gp = pcvcm_stack_pop(parser->vcm_stack);     \
            APPEND_CHILD(gp, parent);                                       \
            parent = gp;                                                    \
        }                                                                   \
        struct pcvcm_node* child = parser->vcm_node;                        \
        APPEND_CHILD(parent, child);                                        \
        UPDATE_VCM_NODE(parent);                                            \
    } while (false)


enum tokenizer_state {
    FIRST_STATE = 0,

    TKZ_STATE_EJSON_DATA = FIRST_STATE,
    TKZ_STATE_EJSON_FINISHED,
    TKZ_STATE_EJSON_CONTROL,
    TKZ_STATE_EJSON_LEFT_BRACE,
    TKZ_STATE_EJSON_RIGHT_BRACE,
    TKZ_STATE_EJSON_LEFT_BRACKET,
    TKZ_STATE_EJSON_RIGHT_BRACKET,
    TKZ_STATE_EJSON_LEFT_PARENTHESIS,
    TKZ_STATE_EJSON_RIGHT_PARENTHESIS,
    TKZ_STATE_EJSON_DOLLAR,
    TKZ_STATE_EJSON_AFTER_VALUE,
    TKZ_STATE_EJSON_BEFORE_NAME,
    TKZ_STATE_EJSON_AFTER_NAME,
    TKZ_STATE_EJSON_NAME_UNQUOTED,
    TKZ_STATE_EJSON_NAME_SINGLE_QUOTED,
    TKZ_STATE_EJSON_NAME_DOUBLE_QUOTED,
    TKZ_STATE_EJSON_VALUE_SINGLE_QUOTED,
    TKZ_STATE_EJSON_VALUE_DOUBLE_QUOTED,
    TKZ_STATE_EJSON_AFTER_VALUE_DOUBLE_QUOTED,
    TKZ_STATE_EJSON_VALUE_TWO_DOUBLE_QUOTED,
    TKZ_STATE_EJSON_VALUE_THREE_DOUBLE_QUOTED,
    TKZ_STATE_EJSON_KEYWORD,
    TKZ_STATE_EJSON_AFTER_KEYWORD,
    TKZ_STATE_EJSON_BYTE_SEQUENCE,
    TKZ_STATE_EJSON_AFTER_BYTE_SEQUENCE,
    TKZ_STATE_EJSON_HEX_BYTE_SEQUENCE,
    TKZ_STATE_EJSON_BINARY_BYTE_SEQUENCE,
    TKZ_STATE_EJSON_BASE64_BYTE_SEQUENCE,
    TKZ_STATE_EJSON_VALUE_NUMBER,
    TKZ_STATE_EJSON_AFTER_VALUE_NUMBER,
    TKZ_STATE_EJSON_VALUE_NUMBER_INTEGER,
    TKZ_STATE_EJSON_VALUE_NUMBER_FRACTION,
    TKZ_STATE_EJSON_VALUE_NUMBER_EXPONENT,
    TKZ_STATE_EJSON_VALUE_NUMBER_EXPONENT_INTEGER,
    TKZ_STATE_EJSON_VALUE_NUMBER_SUFFIX_INTEGER,
    TKZ_STATE_EJSON_VALUE_NUMBER_HEX,
    TKZ_STATE_EJSON_VALUE_NUMBER_HEX_SUFFIX,
    TKZ_STATE_EJSON_AFTER_VALUE_NUMBER_HEX,
    TKZ_STATE_EJSON_VALUE_NUMBER_INFINITY,
    TKZ_STATE_EJSON_VALUE_NAN,
    TKZ_STATE_EJSON_STRING_ESCAPE,
    TKZ_STATE_EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS,
    TKZ_STATE_EJSON_JSONEE_VARIABLE,
    TKZ_STATE_EJSON_JSONEE_FULL_STOP_SIGN,
    TKZ_STATE_EJSON_JSONEE_KEYWORD,
    TKZ_STATE_EJSON_JSONEE_STRING,
    TKZ_STATE_EJSON_AFTER_JSONEE_STRING,
    TKZ_STATE_EJSON_AMPERSAND,
    TKZ_STATE_EJSON_OR_SIGN,
    TKZ_STATE_EJSON_SEMICOLON,
    TKZ_STATE_EJSON_CJSONEE_FINISHED,

    LAST_STATE = TKZ_STATE_EJSON_CJSONEE_FINISHED,
};

// origin parser end

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
    EJSON_TKZ_STATE_SINGLE_QUOTED,
    EJSON_TKZ_STATE_DOUBLE_QUOTED,
    EJSON_TKZ_STATE_UNQUOTED,
    EJSON_TKZ_STATE_LEFT_BRACE,
    EJSON_TKZ_STATE_RIGHT_BRACE,
    EJSON_TKZ_STATE_LEFT_BRACKET,
    EJSON_TKZ_STATE_RIGHT_BRACKET,
    EJSON_TKZ_STATE_LEFT_PARENTHESIS,
    EJSON_TKZ_STATE_RIGHT_PARENTHESIS,
    EJSON_TKZ_STATE_DOLLAR,
    EJSON_TKZ_STATE_AFTER_VALUE,
    EJSON_TKZ_STATE_BEFORE_NAME,
    EJSON_TKZ_STATE_AFTER_NAME,
    EJSON_TKZ_STATE_NAME_UNQUOTED,
    EJSON_TKZ_STATE_NAME_SINGLE_QUOTED,
    EJSON_TKZ_STATE_NAME_DOUBLE_QUOTED,
    EJSON_TKZ_STATE_VALUE_SINGLE_QUOTED,
    EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED,
    EJSON_TKZ_STATE_AFTER_VALUE_DOUBLE_QUOTED,
    EJSON_TKZ_STATE_VALUE_TWO_DOUBLE_QUOTED,
    EJSON_TKZ_STATE_VALUE_THREE_DOUBLE_QUOTED,
    EJSON_TKZ_STATE_KEYWORD,
    EJSON_TKZ_STATE_AFTER_KEYWORD,
    EJSON_TKZ_STATE_BYTE_SEQUENCE,
    EJSON_TKZ_STATE_AFTER_BYTE_SEQUENCE,
    EJSON_TKZ_STATE_HEX_BYTE_SEQUENCE,
    EJSON_TKZ_STATE_BINARY_BYTE_SEQUENCE,
    EJSON_TKZ_STATE_BASE64_BYTE_SEQUENCE,
    EJSON_TKZ_STATE_VALUE_NUMBER,
    EJSON_TKZ_STATE_AFTER_VALUE_NUMBER,
    EJSON_TKZ_STATE_VALUE_NUMBER_INTEGER,
    EJSON_TKZ_STATE_VALUE_NUMBER_FRACTION,
    EJSON_TKZ_STATE_VALUE_NUMBER_EXPONENT,
    EJSON_TKZ_STATE_VALUE_NUMBER_EXPONENT_INTEGER,
    EJSON_TKZ_STATE_VALUE_NUMBER_SUFFIX_INTEGER,
    EJSON_TKZ_STATE_VALUE_NUMBER_HEX,
    EJSON_TKZ_STATE_VALUE_NUMBER_HEX_SUFFIX,
    EJSON_TKZ_STATE_AFTER_VALUE_NUMBER_HEX,
    EJSON_TKZ_STATE_VALUE_NUMBER_INFINITY,
    EJSON_TKZ_STATE_VALUE_NAN,
    EJSON_TKZ_STATE_STRING_ESCAPE,
    EJSON_TKZ_STATE_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS,
    EJSON_TKZ_STATE_AMPERSAND,
    EJSON_TKZ_STATE_OR_SIGN,
    EJSON_TKZ_STATE_SEMICOLON,
    EJSON_TKZ_STATE_CJSONEE_FINISHED,
    EJSON_TKZ_STATE_RAW_STRING,
    EJSON_TKZ_STATE_VARIABLE,
    EJSON_TKZ_STATE_AFTER_VARIABLE,

    EJSON_TKZ_STATE_LAST = EJSON_TKZ_STATE_AFTER_VARIABLE,
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
    const char *state_name;

    uint32_t prev_separator;
    uint32_t nr_quoted;
    bool enable_log;
};

PCA_EXTERN_C_BEGIN

struct pcejson_token *
pcejson_token_new(uint32_t type);

void
pcejson_token_destroy(struct pcejson_token *token);

void
pcejson_token_close(struct pcejson_token *token);

bool
pcejson_token_is_closed(struct pcejson_token *token);


struct pcejson_token_stack *
pcejson_token_stack_new();

int
pcejson_token_stack_destroy(struct pcejson_token_stack *stack);

bool
pcejson_token_stack_is_empty(struct pcejson_token_stack *stack);

struct pcejson_token *
pcejson_token_stack_push(struct pcejson_token_stack *stack, uint32_t type);

struct pcejson_token *
pcejson_token_stack_push_token(struct pcejson_token_stack *stack,
        struct pcejson_token *token);

struct pcejson_token *
pcejson_token_stack_pop(struct pcejson_token_stack *stack);

struct pcejson_token *
pcejson_token_stack_top(struct pcejson_token_stack *stack);

int
pcejson_token_stack_size(struct pcejson_token_stack *stack);

int
pcejson_token_stack_clear(struct pcejson_token_stack *stack);

struct pcejson_token *
pcejson_token_stack_get(struct pcejson_token_stack *stack, int idx);

bool
pcejson_inc_depth (struct pcejson* parser);

void
pcejson_dec_depth (struct pcejson* parser);

struct pcvcm_node *
create_byte_sequenct(struct tkz_buffer *buffer);

int pcejson_parse_o(struct pcvcm_node **vcm_tree,
        struct pcejson **parser_param, purc_rwstream_t rws, uint32_t depth);

int pcejson_parse_n(struct pcvcm_node **vcm_tree,
        struct pcejson **parser_param, purc_rwstream_t rws, uint32_t depth);

PCA_EXTERN_C_END

#endif /* PURC_EJSON_TOKENIZER_H */
