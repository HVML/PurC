/*
 * @file parser.c
 * @author Xue Shuming
 * @date 2022/02/24
 * @brief The implementation of ejson parser.
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

#if HAVE(GLIB)
#define    pc_alloc(sz)   g_slice_alloc0(sz)
#define    pc_free(p)     g_slice_free1(sizeof(*p), (gpointer)p)
#else
#define    pc_alloc(sz)   calloc(1, sz)
#define    pc_free(p)     free(p)
#endif

#define PRINT_STATE(state_name)                                             \
    if (parser->enable_log) {                                               \
        size_t len;                                                         \
        char *s = pcvcm_node_to_string(parser->vcm_node, &len);             \
        PC_DEBUG(                                                           \
            "in %s|uc=%c|hex=0x%X|stack_is_empty=%d"                        \
            "|stack_top=%c|stack_size=%ld|vcm_node=%s\n",                   \
            curr_state_name, character, character,                          \
            ejson_stack_is_empty(), (char)ejson_stack_top(),                \
            (long int)ejson_stack_size(), s);                               \
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

#define RESET_STRING_BUFFER()                                               \
    do {                                                                    \
        tkz_buffer_reset(parser->string_buffer);                            \
    } while (false)

#define APPEND_TO_STRING_BUFFER(uc)                                         \
    do {                                                                    \
        tkz_buffer_append(parser->string_buffer, uc);                       \
    } while (false)

#define RESET_QUOTED_COUNTER()                                              \
    do {                                                                    \
        parser->nr_quoted = 0;                                              \
    } while (false)

#define UPDATE_VCM_NODE(node)                                               \
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

#define print_uc_list(uc_list, tag)                                         \
    do {                                                                    \
        PC_DEBUG( "begin print %s list\n|", tag);                           \
        struct list_head *p, *n;                                            \
        list_for_each_safe(p, n, uc_list) {                                 \
            struct tkz_uc *puc = list_entry(p, struct tkz_uc, list);        \
            PC_DEBUG( "%c", puc->character);                                \
        }                                                                   \
        PC_DEBUG( "|\nend print %s list\n", tag);                           \
    } while(0)

#define PRINT_CONSUMED_LIST(wrap)    \
        print_uc_list(&wrap->consumed_list, "consumed")

#define PRINT_RECONSUM_LIST(wrap)    \
        print_uc_list(&wrap->reconsume_list, "reconsume")

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
    uint32_t prev_separator;
    uint32_t nr_quoted;
    bool enable_log;
};

#define EJSON_MAX_DEPTH         32
#define EJSON_MIN_BUFFER_SIZE   128
#define EJSON_MAX_BUFFER_SIZE   1024 * 1024 * 1024
#define EJSON_END_OF_FILE       0
#define PURC_ENVV_EJSON_LOG_ENABLE  "PURC_EJSON_LOG_ENABLE"

struct pcejson *pcejson_create(uint32_t depth, uint32_t flags)
{
    struct pcejson* parser = (struct pcejson*) pc_alloc(
            sizeof(struct pcejson));
    if (!parser) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    parser->state = 0;
    parser->max_depth = depth;
    parser->depth = 0;
    parser->flags = flags;

    parser->curr_uc = NULL;
    parser->tkz_reader = tkz_reader_new();
    parser->temp_buffer = tkz_buffer_new();
    parser->string_buffer = tkz_buffer_new();
    parser->vcm_stack = pcvcm_stack_new();
    parser->ejson_stack = pcutils_stack_new(0);
    parser->prev_separator = 0;
    parser->nr_quoted = 0;

    const char *env_value = getenv(PURC_ENVV_EJSON_LOG_ENABLE);
    parser->enable_log = ((env_value != NULL) &&
            (*env_value == '1' || pcutils_strcasecmp(env_value, "true") == 0));

    return parser;
}

void pcejson_destroy(struct pcejson *parser)
{
    if (parser) {
        tkz_reader_destroy(parser->tkz_reader);
        tkz_buffer_destroy(parser->temp_buffer);
        tkz_buffer_destroy(parser->string_buffer);
        struct pcvcm_node* n = parser->vcm_node;
        parser->vcm_node = NULL;
        while (!pcvcm_stack_is_empty(parser->vcm_stack)) {
            struct pcvcm_node* node = pcvcm_stack_pop(parser->vcm_stack);
            pctree_node_append_child(
                    (struct pctree_node*)node, (struct pctree_node*)n);
            n = node;
        }
        pcvcm_node_destroy(n);
        pcvcm_stack_destroy(parser->vcm_stack);
        pcutils_stack_destroy(parser->ejson_stack);
        tkz_sbst_destroy(parser->sbst);
        pc_free(parser);
    }
}

void pcejson_reset(struct pcejson *parser, uint32_t depth, uint32_t flags)
{
    parser->state = 0;
    parser->max_depth = depth;
    parser->depth = 0;
    parser->flags = flags;

    tkz_reader_destroy(parser->tkz_reader);
    parser->tkz_reader = tkz_reader_new();

    tkz_buffer_reset(parser->temp_buffer);
    tkz_buffer_reset(parser->string_buffer);

    struct pcvcm_node* n = parser->vcm_node;
    parser->vcm_node = NULL;
    while (!pcvcm_stack_is_empty(parser->vcm_stack)) {
        struct pcvcm_node *node = pcvcm_stack_pop(parser->vcm_stack);
        pctree_node_append_child(
                (struct pctree_node *)node, (struct pctree_node *)n);
        n = node;
    }
    pcvcm_node_destroy(n);
    pcvcm_stack_destroy(parser->vcm_stack);
    parser->vcm_stack = pcvcm_stack_new();
    pcutils_stack_destroy(parser->ejson_stack);
    parser->ejson_stack = pcutils_stack_new(0);
    parser->prev_separator = 0;
    parser->nr_quoted = 0;
}

static inline UNUSED_FUNCTION
bool pcejson_inc_depth (struct pcejson* parser)
{
    parser->depth++;
    return parser->depth <= parser->max_depth;
}

void pcejson_dec_depth (struct pcejson* parser)
{
    if (parser->depth > 0) {
        parser->depth--;
    }
}

struct pcvcm_node *
create_byte_sequenct(struct tkz_buffer *buffer)
{
    size_t nr_bytes = tkz_buffer_get_size_in_bytes(buffer);
    const char *bytes = tkz_buffer_get_bytes(buffer);
    if (bytes[1] == 'x') {
        return pcvcm_node_new_byte_sequence_from_bx(bytes + 2, nr_bytes - 2);
    }
    else if (bytes[1] == 'b') {
        return pcvcm_node_new_byte_sequence_from_bb(bytes + 2, nr_bytes - 2);
    }
    else if (bytes[1] == '6') {
        return pcvcm_node_new_byte_sequence_from_b64(bytes + 3, nr_bytes - 3);
    }
    return NULL;
}

struct pcejson_token *
pcejson_token_new(uint32_t type)
{
    struct pcejson_token *token = (struct pcejson_token*) pc_alloc(
            sizeof(struct pcejson_token));
    if (!token) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }
    token->type = type;

out:
    return token;
}

void
pcejson_token_destroy(struct pcejson_token *token)
{
    if (token) {
        if (token->node) {
            pcvcm_node_destroy(token->node);
        }
        pc_free(token);
    }
}

void
pcejson_token_close(struct pcejson_token *token)
{
    if (token) {
        if (token->node) {
            pcvcm_node_set_closed(token->node, true);
        }
    }
}

bool
pcejson_token_is_closed(struct pcejson_token *token)
{
    return (token && token->node) ? pcvcm_node_is_closed(token->node) : false;
}

struct pcejson_token_stack *
pcejson_token_stack_new()
{
    struct pcejson_token_stack *stack = (struct pcejson_token_stack*)pc_alloc(
            sizeof(struct pcejson_token_stack));
    if (!stack) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    stack->stack = pcutils_stack_new(0);
    if (!stack->stack) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    goto out;

failed:
    if (stack) {
        pc_free(stack);
        stack = NULL;
    }

out:
    return stack;
}

int
pcejson_token_stack_destroy(struct pcejson_token_stack *stack)
{
    if (stack) {
        struct pcejson_token *token = pcejson_token_stack_pop(stack);
        while(token) {
            pcejson_token_destroy(token);
            token = pcejson_token_stack_pop(stack);
        }
        pcutils_stack_destroy(stack->stack);
        pc_free(stack);
    }
    return 0;
}

bool
pcejson_token_stack_is_empty(struct pcejson_token_stack *stack)
{
    return pcutils_stack_is_empty(stack->stack);
}

struct pcejson_token *
pcejson_token_stack_push(struct pcejson_token_stack *stack, uint32_t type)
{
    struct pcejson_token *token = pcejson_token_new(type);
    if (!token) {
        goto failed;
    }

    switch (type) {
    case ETT_PROTECT:
        token->node = NULL;
        break;

    case ETT_OBJECT:
        token->node = pcvcm_node_new_object(0, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        break;

    case ETT_ARRAY:
        token->node = pcvcm_node_new_array(0, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        break;

    case ETT_CALL_GETTER:
        token->node = pcvcm_node_new_call_getter(NULL, 0, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        break;

    case ETT_CALL_SETTER:
        token->node = pcvcm_node_new_call_setter(NULL, 0, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        break;

    case ETT_GET_VARIABLE:
        token->node = pcvcm_node_new_get_variable(NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        break;

    case ETT_GET_ELEMENT:
    case ETT_GET_ELEMENT_BY_BRACKET:
        token->node = pcvcm_node_new_get_element(NULL, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        break;

    case ETT_CJSONEE:
        token->node = pcvcm_node_new_cjsonee(NULL, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        break;

    case ETT_STRING:
        break;

    case ETT_MULTI_QUOTED_S:       /* multiple double quoted */
    case ETT_MULTI_UNQUOTED_S:       /* multiple unquoted*/
        token->node = pcvcm_node_new_concat_string(0, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        break;

    case ETT_KEY:           /* json object key */
        break;

    case ETT_VALUE:           /* json object value */
        break;

    case ETT_DOUBLE_S:           /* double quoted string */
        break;

    case ETT_SINGLE_S:           /* single quoted string */
        break;

    case ETT_UNQUOTED_S:           /* unquoted string */
        break;

    case ETT_KEYWORD:           /* keywords true, false, null */
        break;

    case ETT_AND:
        token->node = pcvcm_node_new_cjsonee_op_and();
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        break;

    case ETT_OR:
        token->node = pcvcm_node_new_cjsonee_op_or();
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        break;

    case ETT_SEMICOLON:
        token->node = pcvcm_node_new_cjsonee_op_semicolon();
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        break;
    }

    pcutils_stack_push(stack->stack, (uintptr_t)token);
    return token;

failed:
    if (token) {
        pcejson_token_destroy(token);
    }
    return token;
}

struct pcejson_token *
pcejson_token_stack_push_token(struct pcejson_token_stack *stack,
        struct pcejson_token *token)
{
    if (token) {
        pcutils_stack_push(stack->stack, (uintptr_t)token);
    }
    return token;
}

struct pcejson_token *
pcejson_token_stack_pop(struct pcejson_token_stack *stack)
{
    return (struct pcejson_token*)pcutils_stack_pop(stack->stack);
}

struct pcejson_token *
pcejson_token_stack_top(struct pcejson_token_stack *stack)
{
    return (struct pcejson_token*)pcutils_stack_top(stack->stack);
}

int
pcejson_token_stack_size(struct pcejson_token_stack *stack)
{
    return pcutils_stack_size(stack->stack);
}

int
pcejson_token_stack_clear(struct pcejson_token_stack *stack)
{
    struct pcejson_token *token = pcejson_token_stack_pop(stack);
    while(token) {
        pcejson_token_destroy(token);
        token = pcejson_token_stack_pop(stack);
    }
    return 0;
}

struct pcejson_token *
pcejson_token_stack_get(struct pcejson_token_stack *stack, int idx)
{
    return (struct pcejson_token*)pcutils_stack_get(stack->stack, idx);
}

struct pcejson *pcejson_create(uint32_t depth, uint32_t flags)
{
    struct pcejson* parser = (struct pcejson*) pc_alloc(
            sizeof(struct pcejson));
    if (!parser) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    parser->state =  EJSON_TKZ_STATE_DATA;
    parser->max_depth = depth;
    parser->depth = 0;
    parser->flags = flags;

    parser->curr_uc = NULL;
    parser->tkz_reader = NULL;
    parser->temp_buffer = tkz_buffer_new();
    parser->string_buffer = tkz_buffer_new();
    parser->tkz_stack = pcejson_token_stack_new();
    parser->prev_separator = 0;
    parser->nr_quoted = 0;
    parser->raw_buffer = tkz_buffer_new();

    const char *env_value = getenv(PURC_ENVV_EJSON_LOG_ENABLE);
    parser->enable_log = ((env_value != NULL) &&
            (*env_value == '1' || pcutils_strcasecmp(env_value, "true") == 0));

    return parser;
}

void pcejson_destroy(struct pcejson *parser)
{
    if (parser) {
        tkz_buffer_destroy(parser->temp_buffer);
        tkz_buffer_destroy(parser->string_buffer);
        pcejson_token_stack_destroy(parser->tkz_stack);
        tkz_sbst_destroy(parser->sbst);
        tkz_buffer_destroy(parser->raw_buffer);
        pc_free(parser);
    }
}

void pcejson_reset(struct pcejson *parser, uint32_t depth, uint32_t flags)
{
    parser->state =  EJSON_TKZ_STATE_DATA;
    parser->max_depth = depth;
    parser->depth = 0;
    parser->flags = flags;
    parser->tkz_reader = NULL;

    tkz_buffer_reset(parser->temp_buffer);
    tkz_buffer_reset(parser->string_buffer);
    tkz_buffer_reset(parser->raw_buffer);

    pcejson_token_stack_destroy(parser->tkz_stack);
    parser->tkz_stack = pcejson_token_stack_new();
    parser->prev_separator = 0;
    parser->nr_quoted = 0;
}

static bool
is_finished_default(struct pcejson *parser, uint32_t character)
{
    UNUSED_PARAM(parser);
    UNUSED_PARAM(character);
    return false;
}

int pcejson_parse(struct pcvcm_node **vcm_tree,
        struct pcejson **parser_param, purc_rwstream_t rws, uint32_t depth)
{
    int ret;
    struct tkz_reader *reader = tkz_reader_new();
    if (!reader) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        ret = -1;
        goto out;
    }
    tkz_reader_set_rwstream(reader, rws);
    ret = pcejson_parse_full(vcm_tree, parser_param, reader, depth,
            is_finished_default);
    tkz_reader_destroy(reader);
out:
    return ret;
}



