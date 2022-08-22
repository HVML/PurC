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

#define RESET_TEMP_BUFFER()                                                 \
    do {                                                                    \
        tkz_buffer_reset(parser->temp_buffer);                               \
    } while (false)

#define APPEND_TO_TEMP_BUFFER(c)                                            \
    do {                                                                    \
        tkz_buffer_append(parser->temp_buffer, c);                           \
    } while (false)

#define APPEND_BYTES_TO_TEMP_BUFFER(bytes, nr_bytes)                        \
    do {                                                                    \
        tkz_buffer_append_bytes(parser->temp_buffer, bytes, nr_bytes);       \
    } while (false)

#define APPEND_BUFFER_TO_TEMP_BUFFER(buffer)                                \
    do {                                                                    \
        tkz_buffer_append_another(parser->temp_buffer, buffer);              \
    } while (false)

#define IS_TEMP_BUFFER_EMPTY()                                              \
        tkz_buffer_is_empty(parser->temp_buffer)

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
            pc_free(token);
        }
    }
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

int
pcejson_token_stack_push_simple(struct pcejson_token_stack *stack,
        uint32_t type)
{

    struct pcejson_token *token = pcejson_token_new(type);
    if (token) {
        return pcejson_token_stack_push(stack, token);
    }
    return -1;
}

int
pcejson_token_stack_push(struct pcejson_token_stack *stack,
        struct pcejson_token *token)
{
    pcutils_stack_push(stack->stack, (uintptr_t)token);
    return 0;
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
    parser->tkz_stack = pcejson_token_stack_new();
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
        pcejson_token_stack_destroy(parser->tkz_stack);
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
    pcejson_token_stack_destroy(parser->tkz_stack);
    parser->ejson_stack = pcutils_stack_new(0);
    parser->tkz_stack = pcejson_token_stack_new();
    parser->prev_separator = 0;
    parser->nr_quoted = 0;
}

static inline UNUSED_FUNCTION
bool pcejson_inc_depth (struct pcejson* parser)
{
    parser->depth++;
    return parser->depth <= parser->max_depth;
}

static inline UNUSED_FUNCTION
void pcejson_dec_depth (struct pcejson* parser)
{
    if (parser->depth > 0) {
        parser->depth--;
    }
}

static UNUSED_FUNCTION
struct pcvcm_node *create_byte_sequenct(struct tkz_buffer *buffer)
{
    UNUSED_PARAM(buffer);
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

#define PCEJSON_PARSER_BEGIN                                                \
int pcejson_parse(struct pcvcm_node **vcm_tree,                             \
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
    tkz_reader_set_rwstream (parser->tkz_reader, rws);                            \
                                                                            \
next_input:                                                                 \
    parser->curr_uc = tkz_reader_next_char (parser->tkz_reader);                  \
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

#if 1
PCEJSON_PARSER_BEGIN

BEGIN_STATE(TKZ_STATE_EJSON_DATA)
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    if (is_whitespace (character) || character == 0xFEFF) {
        ADVANCE_TO(TKZ_STATE_EJSON_DATA);
    }
    RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_FINISHED)
    if (!is_eof(character) && !is_whitespace(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    while (!vcm_stack_is_empty()) {
        ejson_stack_pop();
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
    }
    if (is_eof(character)  && !ejson_stack_is_empty()) {
        uint32_t uc = ejson_stack_top();
        if (uc == '{' || uc == '[' || uc == '(' || uc == ':') {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
            return -1;
        }
    }
    ejson_stack_reset();
    *vcm_tree = parser->vcm_node;
    parser->vcm_node = NULL;
    return 0;
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_CONTROL)
    uint32_t uc = ejson_stack_top();
    if (is_whitespace(character)) {
        if (ejson_stack_is_empty()) {
            RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
        }
        if (uc == '"' || uc == '\'' || uc == 'U') {
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_JSONEE_STRING);
        }
        ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
    }
    if (character == '{') {
        RECONSUME_IN(TKZ_STATE_EJSON_LEFT_BRACE);
    }
    if (character == '}') {
        if ((parser->vcm_node->type == PCVCM_NODE_TYPE_FUNC_CONCAT_STRING)
                && (uc == '"' || uc == '\'' || uc == 'U')) {
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_JSONEE_STRING);
        }
        if (parser->vcm_node && pcvcm_node_is_closed(parser->vcm_node)) {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_RIGHT_BRACE);
    }
    if (character == '[') {
        RECONSUME_IN(TKZ_STATE_EJSON_LEFT_BRACKET);
    }
    if (character == ']') {
        if (parser->vcm_node != NULL && parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_CONCAT_STRING
                && (uc == '"' || uc == '\'' || uc == 'U')) {
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_JSONEE_STRING);
        }
        RECONSUME_IN(TKZ_STATE_EJSON_RIGHT_BRACKET);
    }
#if 0
    if (character == '<' || character == '>') {
        RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
    }
#endif
    if (character == '/') {
        if (ejson_stack_is_empty() && parser->vcm_node) {
            RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
        }
    }
    if (character == '(') {
        ADVANCE_TO(TKZ_STATE_EJSON_LEFT_PARENTHESIS);
    }
    if (character == ')') {
        if (ejson_stack_is_empty() && parser->vcm_node) {
            RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
        }
        if (uc == '"' || uc == '\'' || uc == 'U') {
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_JSONEE_STRING);
        }
        pcejson_dec_depth(parser);
        RECONSUME_IN(TKZ_STATE_EJSON_RIGHT_PARENTHESIS);
        //ADVANCE_TO(TKZ_STATE_EJSON_RIGHT_PARENTHESIS);
    }
    if (character == '$') {
        RECONSUME_IN(TKZ_STATE_EJSON_DOLLAR);
    }
    if (character == '"') {
        if (ejson_stack_is_empty() && parser->vcm_node) {
            RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
        }
        if (uc == '"') {
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_JSONEE_STRING);
        }
        else {
            RESET_TEMP_BUFFER();
            RESET_QUOTED_COUNTER();
            RECONSUME_IN(TKZ_STATE_EJSON_VALUE_DOUBLE_QUOTED);
        }
    }
    if (character == '\'') {
        RESET_TEMP_BUFFER();
        RESET_QUOTED_COUNTER();
        RECONSUME_IN(TKZ_STATE_EJSON_VALUE_SINGLE_QUOTED);
    }
    if (character == 'b') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(TKZ_STATE_EJSON_BYTE_SEQUENCE);
    }
    if (character == 't' || character == 'f' || character == 'n'
            || character == 'u') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(TKZ_STATE_EJSON_KEYWORD);
    }
    if (character == 'I') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(TKZ_STATE_EJSON_VALUE_NUMBER_INFINITY);
    }
    if (character == 'N') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(TKZ_STATE_EJSON_VALUE_NAN);
    }
    if (is_ascii_digit(character) || character == '-') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(TKZ_STATE_EJSON_VALUE_NUMBER);
    }
    if (is_eof(character)) {
        if (parser->vcm_node) {
            RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    if (character == ',') {
        if (uc == '{') {
            ejson_stack_pop();
            ADVANCE_TO(TKZ_STATE_EJSON_BEFORE_NAME);
        }
        if (uc == '[' || uc == '(' || uc == '<') {
            if (parser->vcm_node && pcvcm_node_is_closed(parser->vcm_node)) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
        }
        if (uc == ':') {
            ejson_stack_pop();
            if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                struct pcvcm_node *node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer));
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
            }
            if (parser->vcm_node &&
                    parser->vcm_node->type != PCVCM_NODE_TYPE_OBJECT) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            ADVANCE_TO(TKZ_STATE_EJSON_BEFORE_NAME);
        }
        if (uc == '"') {
            RECONSUME_IN(TKZ_STATE_EJSON_JSONEE_STRING);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '.') {
        RECONSUME_IN(TKZ_STATE_EJSON_JSONEE_FULL_STOP_SIGN);
    }
    if (uc == '[') {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '&') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(TKZ_STATE_EJSON_AMPERSAND);
    }
    if (character == '|') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(TKZ_STATE_EJSON_OR_SIGN);
    }
    if (character == ';') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(TKZ_STATE_EJSON_SEMICOLON);
    }
    if (parser->vcm_node != NULL && (parser->vcm_node->type ==
            PCVCM_NODE_TYPE_FUNC_GET_VARIABLE ||
            parser->vcm_node->type ==
            PCVCM_NODE_TYPE_FUNC_GET_ELEMENT)) {
        size_t n = pctree_node_children_number(
                (struct pctree_node*)parser->vcm_node);
        if (n < 2) {
            RECONSUME_IN(TKZ_STATE_EJSON_JSONEE_VARIABLE);
        }
        else {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
    }
    if (ejson_stack_is_empty() && parser->vcm_node) {
        RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
    }
    RECONSUME_IN(TKZ_STATE_EJSON_JSONEE_STRING);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_DOLLAR)
    if (is_whitespace(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '$') {
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        ejson_stack_push('$');
        struct pcvcm_node *snode = pcvcm_node_new_get_variable(NULL);
        UPDATE_VCM_NODE(snode);
        ADVANCE_TO(TKZ_STATE_EJSON_DOLLAR);
    }
    if (character == '{') {
        ejson_stack_push('P');
        RESET_TEMP_BUFFER();
        ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_VARIABLE);
    }
    RESET_TEMP_BUFFER();
    RECONSUME_IN(TKZ_STATE_EJSON_JSONEE_VARIABLE);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_JSONEE_FULL_STOP_SIGN)
    if (parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_GET_VARIABLE ||
                parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_GET_ELEMENT) {
        ejson_stack_push('.');
        struct pcvcm_node *node = pcvcm_node_new_get_element(NULL,
                NULL);
        APPEND_CHILD(node, parser->vcm_node);
        UPDATE_VCM_NODE(node);
        ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_KEYWORD);
    }
    else if (parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_CALL_GETTER ||
                parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_CALL_SETTER) {
        ejson_stack_push('.');
        struct pcvcm_node *node = pcvcm_node_new_get_element(NULL,
                NULL);
        APPEND_CHILD(node, parser->vcm_node);
        UPDATE_VCM_NODE(node);
        ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_KEYWORD);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_LEFT_BRACE)
    if (character == '{') {
        ejson_stack_push('P');
        ADVANCE_TO(TKZ_STATE_EJSON_LEFT_BRACE);
    }
    if (character == '$') {
        RECONSUME_IN(TKZ_STATE_EJSON_DOLLAR);
    }
    uint32_t uc = ejson_stack_top();
    if (is_whitespace(character)) {
        if (uc == 'P') {
            ejson_stack_pop();
            uc = ejson_stack_top();
            if (uc == 'P') {
                ejson_stack_pop();
                ejson_stack_push('C');
                if (parser->vcm_node) {
                    vcm_stack_push(parser->vcm_node);
                }
                struct pcvcm_node *node = pcvcm_node_new_cjsonee();
                UPDATE_VCM_NODE(node);
                ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
            }
            else {
                if (!pcejson_inc_depth(parser)) {
                    SET_ERR(PCEJSON_ERROR_MAX_DEPTH_EXCEEDED);
                    return -1;
                }
                ejson_stack_push('{');
                if (parser->vcm_node) {
                    vcm_stack_push(parser->vcm_node);
                }
                struct pcvcm_node *node = pcvcm_node_new_object(0, NULL);
                UPDATE_VCM_NODE(node);
                RECONSUME_IN(TKZ_STATE_EJSON_BEFORE_NAME);
            }
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (uc == 'P') {
        ejson_stack_pop();
        ejson_stack_push('{');
        if (!pcejson_inc_depth(parser)) {
            SET_ERR(PCEJSON_ERROR_MAX_DEPTH_EXCEEDED);
            return -1;
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        struct pcvcm_node *node = pcvcm_node_new_object(0, NULL);
        UPDATE_VCM_NODE(node);
        RECONSUME_IN(TKZ_STATE_EJSON_BEFORE_NAME);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_RIGHT_BRACE)
    if (is_eof(character)) {
        if (parser->vcm_node) {
            RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    uint32_t uc = ejson_stack_top();
    if (character == '}') {
        if (uc == 'C') {
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_CJSONEE_FINISHED);
        }
        if (uc == ':') {
            ejson_stack_pop();
            uc = ejson_stack_top();
        }
        if (uc == '{') {
            ejson_stack_pop();
            pcejson_dec_depth(parser);
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            if (ejson_stack_is_empty()) {
                ADVANCE_TO(TKZ_STATE_EJSON_FINISHED);
            }
            ADVANCE_TO(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        else if (uc == 'P') {
            ejson_stack_pop();
            if (parser->vcm_node->extra & EXTRA_PROTECT_FLAG) {
                parser->vcm_node->extra &= EXTRA_SUGAR_FLAG;
            }
            else {
                parser->vcm_node->extra &= EXTRA_PROTECT_FLAG;
            }
            // FIXME : <update from="assets/{$SYS.locale}.json" />
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            if (ejson_stack_is_empty()) {
                ADVANCE_TO(TKZ_STATE_EJSON_FINISHED);
            }
            ADVANCE_TO(TKZ_STATE_EJSON_RIGHT_BRACE);
        }
        else if (uc == '(' || uc == '<' || uc == '"') {
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_RIGHT_BRACE);
        RETURN_AND_STOP_PARSE();
    }
    if (uc == '"') {
        RECONSUME_IN(TKZ_STATE_EJSON_JSONEE_STRING);
    }
    if (is_whitespace(character)) {
        uint32_t uc = ejson_stack_top();
        while (uc == '$') {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            uc = ejson_stack_top();
        }
        if (ejson_stack_is_empty()) {
            ADVANCE_TO(TKZ_STATE_EJSON_FINISHED);
        }
        if (uc == 'U') {
            RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
        }
        ADVANCE_TO(TKZ_STATE_EJSON_RIGHT_BRACE);
    }
    if (character == ':') {
        if (uc == '{') {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            vcm_stack_push(parser->vcm_node);
            RESET_VCM_NODE();
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
        }
        if (uc == 'P') {
            ejson_stack_pop();
            ejson_stack_push('{');
            struct pcvcm_node *node = pcvcm_node_new_object(0, NULL);
            APPEND_CHILD(node, parser->vcm_node);
            vcm_stack_push(node);
            RESET_VCM_NODE();
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
#if 0
    if (character == '.' && uc == '$') {
        ejson_stack_pop();
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
    }
#endif
    RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_LEFT_BRACKET)
    if (character == '[') {
        if (parser->vcm_node && ejson_stack_is_empty()) {
            ejson_stack_push('.');
            struct pcvcm_node *node = pcvcm_node_new_get_element(NULL,
                    NULL);
            APPEND_CHILD(node, parser->vcm_node);
            UPDATE_VCM_NODE(node);
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
        }
        if (parser->vcm_node && (parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_GET_VARIABLE ||
                parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_GET_ELEMENT
                )) {
            ejson_stack_push('.');
            struct pcvcm_node *node = pcvcm_node_new_get_element(NULL,
                    NULL);
            APPEND_CHILD(node, parser->vcm_node);
            UPDATE_VCM_NODE(node);
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
        }
        uint32_t uc = ejson_stack_top();
        if (uc != '('  && uc != '<' && parser->vcm_node && (
                parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_CALL_GETTER ||
                parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_CALL_SETTER
                    )) {
            ejson_stack_push('.');
            struct pcvcm_node *node = pcvcm_node_new_get_element(NULL,
                    NULL);
            APPEND_CHILD(node, parser->vcm_node);
            UPDATE_VCM_NODE(node);
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
        }
        if (uc == '(' || uc == '<' || uc == '[' || uc == ':' || uc == 0
                || uc == '"') {
            ejson_stack_push('[');
            if (!pcejson_inc_depth(parser)) {
                SET_ERR(PCEJSON_ERROR_MAX_DEPTH_EXCEEDED);
                return -1;
            }
            if (parser->vcm_node) {
                vcm_stack_push(parser->vcm_node);
            }
            struct pcvcm_node *node = pcvcm_node_new_array(0, NULL);
            UPDATE_VCM_NODE(node);
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_RIGHT_BRACKET)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_EJSON_RIGHT_BRACKET);
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    uint32_t uc = ejson_stack_top();
    if (character == ']') {
        if (uc == '.') {
            ejson_stack_pop();
            uc = ejson_stack_top();
            if (uc == '"' || uc == 'U') {
                struct pcvcm_node *parent = vcm_stack_parent();
                if (parent &&
                        parent->type == PCVCM_NODE_TYPE_FUNC_GET_ELEMENT) {
                    POP_AS_VCM_PARENT_AND_UPDATE_VCM();
                }
                ADVANCE_TO(TKZ_STATE_EJSON_AFTER_VALUE);
            }
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            ADVANCE_TO(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        if (uc == '[') {
            ejson_stack_pop();
            pcejson_dec_depth(parser);
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            struct pcvcm_node *parent = (struct pcvcm_node*)
                pctree_node_parent((struct pctree_node*)parser->vcm_node);
            if (parent) {
                UPDATE_VCM_NODE(parent);
            }
#if 0
            if (ejson_stack_is_empty()) {
                ADVANCE_TO(TKZ_STATE_EJSON_FINISHED);
            }
#endif
            ADVANCE_TO(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        if (uc == '"') {
            RECONSUME_IN(TKZ_STATE_EJSON_JSONEE_STRING);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_RIGHT_BRACKET);
        RETURN_AND_STOP_PARSE();
    }
    if (ejson_stack_is_empty()
            || uc == '(' || uc == '<') {
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_LEFT_PARENTHESIS)
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '!') {
        if (parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_GET_VARIABLE ||
                parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_GET_ELEMENT) {
            struct pcvcm_node *node = pcvcm_node_new_call_setter(NULL,
                    0, NULL);
            APPEND_CHILD(node, parser->vcm_node);
            UPDATE_VCM_NODE(node);
            ejson_stack_push('<');
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (parser->vcm_node->type ==
            PCVCM_NODE_TYPE_FUNC_GET_VARIABLE ||
            parser->vcm_node->type ==
            PCVCM_NODE_TYPE_FUNC_GET_ELEMENT) {
        if (!pcejson_inc_depth(parser)) {
            SET_ERR(PCEJSON_ERROR_MAX_DEPTH_EXCEEDED);
            return -1;
        }
        struct pcvcm_node *node = pcvcm_node_new_call_getter(NULL,
                0, NULL);
        APPEND_CHILD(node, parser->vcm_node);
        UPDATE_VCM_NODE(node);
        ejson_stack_push('(');
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    if (ejson_stack_is_empty()) {
        RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_RIGHT_PARENTHESIS)
#if 0
    uint32_t uc = ejson_stack_top();
    if (character == '.' || character == '[') {
        if (uc == '(' || uc == '<') {
            ejson_stack_pop();
#if 1
            struct pcvcm_node *parent = vcm_stack_parent();
            if (!vcm_stack_is_empty() && (
                        parent->type == PCVCM_NODE_TYPE_FUNC_CALL_GETTER ||
                        parent->type == PCVCM_NODE_TYPE_FUNC_CALL_SETTER
                    )) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
#endif
            RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
        }
        if (ejson_stack_is_empty()) {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    else {
        if (uc == '(' || uc == '<') {
            ejson_stack_pop();
            if (!vcm_stack_is_empty()) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
        }
        if (ejson_stack_is_empty()) {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
#else
    uint32_t uc = ejson_stack_top();
    if (uc == '(' || uc == '<') {
        ejson_stack_pop();

        PRINT_VCM_NODE(parser->vcm_node);

        if (parser->vcm_node->type == PCVCM_NODE_TYPE_FUNC_CALL_GETTER
                || parser->vcm_node->type == PCVCM_NODE_TYPE_FUNC_CALL_SETTER) {
            if (!pcvcm_node_is_closed(parser->vcm_node)) {
                pcvcm_node_set_closed(parser->vcm_node, true);
                PRINT_VCM_NODE(parser->vcm_node);
                ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
            }
        }

        if (!vcm_stack_is_empty()) {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            pcvcm_node_set_closed(parser->vcm_node, true);
        PRINT_VCM_NODE(parser->vcm_node);
        }
        ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
    }
    if (ejson_stack_is_empty()) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
#endif
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_AFTER_VALUE)
    uint32_t uc = ejson_stack_top();
    if (is_whitespace(character)) {
        if (ejson_stack_is_empty() || uc  == 'U' || uc == '"' || uc == 'T') {
            RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
        }
        ADVANCE_TO(TKZ_STATE_EJSON_AFTER_VALUE);
    }
    if (is_eof(character) && ejson_stack_is_empty()) {
        RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
    }
    if (character == '"' || character == '\'') {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer));
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        if (uc == '"' || uc == '\'') {
            ejson_stack_pop();
            if (ejson_stack_is_empty()) {
                ADVANCE_TO(TKZ_STATE_EJSON_FINISHED);
            }
        }
        ADVANCE_TO(TKZ_STATE_EJSON_AFTER_VALUE);
    }
    if (character == '}') {
        RECONSUME_IN(TKZ_STATE_EJSON_RIGHT_BRACE);
    }
    if (character == ']') {
        RECONSUME_IN(TKZ_STATE_EJSON_RIGHT_BRACKET);
    }
    if (character == ')') {
        pcejson_dec_depth(parser);
        //ADVANCE_TO(TKZ_STATE_EJSON_RIGHT_PARENTHESIS);
        RECONSUME_IN(TKZ_STATE_EJSON_RIGHT_PARENTHESIS);
    }
    if (character == ',') {
        if (uc == '{') {
            ejson_stack_pop();
            ADVANCE_TO(TKZ_STATE_EJSON_BEFORE_NAME);
        }
        if (uc == '[') {
            if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                struct pcvcm_node *node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer));
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
            }
            if (parser->vcm_node &&
                    parser->vcm_node->type != PCVCM_NODE_TYPE_ARRAY) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
        }
        if (uc == '(' || uc == '<') {
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
        }
        if (uc == ':') {
            ejson_stack_pop();
            if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                struct pcvcm_node *node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer));
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
            }
            if (parser->vcm_node &&
                    parser->vcm_node->type != PCVCM_NODE_TYPE_OBJECT) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            ADVANCE_TO(TKZ_STATE_EJSON_BEFORE_NAME);
        }
        // FIXME
        if (ejson_stack_is_empty() && parser->vcm_node) {
            parser->prev_separator = 0;
            RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '<' || character == '.') {
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    if (character == ';' || character == '|' || character == '&') {
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    if (uc == '"' || uc  == 'U') {
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_BEFORE_NAME)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_EJSON_BEFORE_NAME);
    }
    uint32_t uc = ejson_stack_top();
    if (character == '"') {
        RESET_TEMP_BUFFER();
        RESET_STRING_BUFFER();
        if (uc == '{') {
            ejson_stack_push(':');
        }
        RECONSUME_IN(TKZ_STATE_EJSON_NAME_DOUBLE_QUOTED);
    }
    if (character == '\'') {
        RESET_TEMP_BUFFER();
        if (uc == '{') {
            ejson_stack_push(':');
        }
        RECONSUME_IN(TKZ_STATE_EJSON_NAME_SINGLE_QUOTED);
    }
    if (character == '}') {
        RECONSUME_IN(TKZ_STATE_EJSON_RIGHT_BRACE);
    }
    if (character == '$') {
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    if (is_ascii_alpha(character) || character == '_') {
        RESET_TEMP_BUFFER();
        if (uc == '{') {
            ejson_stack_push(':');
        }
        RECONSUME_IN(TKZ_STATE_EJSON_NAME_UNQUOTED);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_AFTER_NAME)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_EJSON_AFTER_NAME);
    }
    if (character == ':') {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer));
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_NAME_UNQUOTED)
    if (is_whitespace(character) || character == ':') {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_NAME);
    }
    if (is_ascii_alpha(character) || is_ascii_digit(character)
            || character == '-' || character == '_') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_NAME_UNQUOTED);
    }
    if (character == '$') {
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        struct pcvcm_node *snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('U');
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_NAME_SINGLE_QUOTED)
    if (character == '\'') {
        size_t nr_buf_chars = tkz_buffer_get_size_in_chars(
                parser->temp_buffer);
        if (nr_buf_chars >= 1) {
            ADVANCE_TO(TKZ_STATE_EJSON_AFTER_NAME);
        }
        else {
            ADVANCE_TO(TKZ_STATE_EJSON_NAME_SINGLE_QUOTED);
        }
    }
    if (character == '\\') {
        SET_RETURN_STATE(TKZ_STATE_EJSON_NAME_SINGLE_QUOTED);
        ADVANCE_TO(TKZ_STATE_EJSON_STRING_ESCAPE);
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(TKZ_STATE_EJSON_NAME_SINGLE_QUOTED);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_NAME_DOUBLE_QUOTED)
    if (character == '"') {
        size_t nr_buf_chars = tkz_buffer_get_size_in_chars(
                parser->temp_buffer);
        if (nr_buf_chars > 1) {
            tkz_buffer_delete_head_chars (parser->temp_buffer, 1);
            ADVANCE_TO(TKZ_STATE_EJSON_AFTER_NAME);
        }
        else if (nr_buf_chars == 1) {
            RESET_TEMP_BUFFER();
            RESTORE_VCM_NODE();
            struct pcvcm_node *node = pcvcm_node_new_string ("");
            APPEND_AS_VCM_CHILD(node);
            ADVANCE_TO(TKZ_STATE_EJSON_AFTER_NAME);
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_NAME_DOUBLE_QUOTED);
    }
    if (character == '\\') {
        SET_RETURN_STATE(curr_state);
        ADVANCE_TO(TKZ_STATE_EJSON_STRING_ESCAPE);
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '$') {
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        struct pcvcm_node *snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('"');
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(TKZ_STATE_EJSON_NAME_DOUBLE_QUOTED);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_SINGLE_QUOTED)
    if (character == '\'') {
        parser->nr_quoted++;
        size_t nr_buf_chars = tkz_buffer_get_size_in_chars(
                parser->temp_buffer);
        if (parser->nr_quoted > 1 || nr_buf_chars >= 1) {
            RESTORE_VCM_NODE();
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RESET_QUOTED_COUNTER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        else {
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_SINGLE_QUOTED);
        }
    }
    if (character == '\\') {
        SET_RETURN_STATE(curr_state);
        ADVANCE_TO(TKZ_STATE_EJSON_STRING_ESCAPE);
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(TKZ_STATE_EJSON_VALUE_SINGLE_QUOTED);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_DOUBLE_QUOTED)
    if (character == '"') {
        if (parser->nr_quoted == 0) {
            parser->nr_quoted++;
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_DOUBLE_QUOTED);
        }
        else if (parser->nr_quoted == 1) {
            RECONSUME_IN(TKZ_STATE_EJSON_VALUE_TWO_DOUBLE_QUOTED);
        }
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE_DOUBLE_QUOTED);
    }
    if (character == '\\') {
        SET_RETURN_STATE(curr_state);
        ADVANCE_TO(TKZ_STATE_EJSON_STRING_ESCAPE);
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '$') {
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        struct pcvcm_node *snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('"');
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            if (tkz_buffer_end_with(parser->temp_buffer, "{", 1)) {
                tkz_reader_reconsume_last_char(parser->tkz_reader);
                tkz_reader_reconsume_last_char(parser->tkz_reader);
                tkz_buffer_delete_tail_chars(parser->temp_buffer, 1);
                if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                    struct pcvcm_node *node = pcvcm_node_new_string(
                            tkz_buffer_get_bytes(parser->temp_buffer)
                            );
                    APPEND_AS_VCM_CHILD(node);
                    RESET_TEMP_BUFFER();
                }
            }
            else if (tkz_buffer_end_with(parser->temp_buffer, "{{", 2)) {
                tkz_reader_reconsume_last_char(parser->tkz_reader);
                tkz_reader_reconsume_last_char(parser->tkz_reader);
                tkz_reader_reconsume_last_char(parser->tkz_reader);
                tkz_buffer_delete_tail_chars(parser->temp_buffer, 2);
                if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                    struct pcvcm_node *node = pcvcm_node_new_string(
                            tkz_buffer_get_bytes(parser->temp_buffer)
                            );
                    APPEND_AS_VCM_CHILD(node);
                    RESET_TEMP_BUFFER();
                }
            }
            else {
                tkz_reader_reconsume_last_char(parser->tkz_reader);
                struct pcvcm_node *node = pcvcm_node_new_string(
                        tkz_buffer_get_bytes(parser->temp_buffer)
                        );
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
            }
            RESET_TEMP_BUFFER();
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(TKZ_STATE_EJSON_VALUE_DOUBLE_QUOTED);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_AFTER_VALUE_DOUBLE_QUOTED)
    if (character == '\"') {
        RESET_QUOTED_COUNTER();
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_TWO_DOUBLE_QUOTED)
    if (character == '"') {
        if (parser->nr_quoted == 1) {
            parser->nr_quoted++;
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_TWO_DOUBLE_QUOTED);
        }
        else if (parser->nr_quoted == 2) {
            RECONSUME_IN(TKZ_STATE_EJSON_VALUE_THREE_DOUBLE_QUOTED);
        }
    }
    RESTORE_VCM_NODE();
    struct pcvcm_node *node = pcvcm_node_new_string(
            tkz_buffer_get_bytes(parser->temp_buffer)
            );
    APPEND_AS_VCM_CHILD(node);
    RESET_TEMP_BUFFER();
    RESET_QUOTED_COUNTER();
    RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_THREE_DOUBLE_QUOTED)
    if (character == '\"') {
        parser->nr_quoted++;
        if (parser->nr_quoted > 3) {
            APPEND_TO_TEMP_BUFFER(character);
        }
        if (parser->nr_quoted >= 6
                && tkz_buffer_end_with(parser->temp_buffer,
                    "\"\"\"", 3)) {
            RESTORE_VCM_NODE();
            tkz_buffer_delete_tail_chars(parser->temp_buffer, 3);
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RESET_QUOTED_COUNTER();
            ADVANCE_TO(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        ADVANCE_TO(TKZ_STATE_EJSON_VALUE_THREE_DOUBLE_QUOTED);
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(TKZ_STATE_EJSON_VALUE_THREE_DOUBLE_QUOTED);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_KEYWORD)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ','
            || character == ')') {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_KEYWORD);
    }
    if (character == '$') {
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        struct pcvcm_node *snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('U');
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    if (parser->sbst == NULL) {
        parser->sbst = tkz_sbst_new_ejson_keywords();
    }
    bool ret = tkz_sbst_advance_ex(parser->sbst, character, true);
    if (!ret) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_KEYWORD);
        tkz_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        RETURN_AND_STOP_PARSE();
    }

    const char* value = tkz_sbst_get_match(parser->sbst);
    if (value == NULL) {
        ADVANCE_TO(TKZ_STATE_EJSON_KEYWORD);
    }
    else {
        APPEND_BYTES_TO_TEMP_BUFFER(value, strlen(value));
        tkz_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        ADVANCE_TO(TKZ_STATE_EJSON_AFTER_KEYWORD);
    }
    if (is_eof(character)) {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_KEYWORD);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_AFTER_KEYWORD)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ','
            || character == ')' || character == ';' || character == '&'
            || character == '|' || is_eof(character)) {
        if (tkz_buffer_equal_to(parser->temp_buffer, "true", 4)) {
            RESTORE_VCM_NODE();
            struct pcvcm_node *node = pcvcm_node_new_boolean(true);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        if (tkz_buffer_equal_to(parser->temp_buffer, "false",
                    5)) {
            RESTORE_VCM_NODE();
            struct pcvcm_node *node = pcvcm_node_new_boolean(false);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        if (tkz_buffer_equal_to(parser->temp_buffer, "null", 4)) {
            struct pcvcm_node *node = pcvcm_node_new_null();
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        if (tkz_buffer_equal_to(parser->temp_buffer, "undefined", 9)) {
            struct pcvcm_node *node = pcvcm_node_new_undefined();
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        RESET_TEMP_BUFFER();
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    RESET_TEMP_BUFFER();
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_BYTE_SEQUENCE)
    if (character == 'b') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_BYTE_SEQUENCE);
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_BINARY_BYTE_SEQUENCE);
    }
    if (character == 'x') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_HEX_BYTE_SEQUENCE);
    }
    if (character == '6') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_BASE64_BYTE_SEQUENCE);
    }
    if (character == '$') {
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        struct pcvcm_node *snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('U');
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_AFTER_BYTE_SEQUENCE)
    if (is_eof(character) || is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        struct pcvcm_node *node = create_byte_sequenct(parser->temp_buffer);
        if (node == NULL) {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
        RESTORE_VCM_NODE();
        APPEND_AS_VCM_CHILD(node);
        RESET_TEMP_BUFFER();
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_HEX_BYTE_SEQUENCE)
    if (is_eof(character) || is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_BYTE_SEQUENCE);
    }
    else if (is_ascii_digit(character)
            || is_ascii_hex_digit(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_HEX_BYTE_SEQUENCE);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_BINARY_BYTE_SEQUENCE)
    if (is_eof(character) || is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_BYTE_SEQUENCE);
    }
    else if (is_ascii_binary_digit(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_BINARY_BYTE_SEQUENCE);
    }
    if (character == '.') {
        ADVANCE_TO(TKZ_STATE_EJSON_BINARY_BYTE_SEQUENCE);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_BASE64_BYTE_SEQUENCE)
    if (is_eof(character) || is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_BYTE_SEQUENCE);
    }
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_BASE64_BYTE_SEQUENCE);
    }
    if (is_ascii_digit(character) || is_ascii_alpha(character)
            || character == '+' || character == '-' || character == '/') {
        if (!tkz_buffer_end_with(parser->temp_buffer, "=", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_BASE64_BYTE_SEQUENCE);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_BASE64);
        RETURN_AND_STOP_PARSE();
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER);
    }
    if (is_ascii_digit(character)) {
        RECONSUME_IN(TKZ_STATE_EJSON_VALUE_NUMBER_INTEGER);
    }
    if (character == '-') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_INTEGER);
    }
    if (character == '$') {
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        struct pcvcm_node *snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('U');
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    SET_ERR(PCEJSON_ERROR_BAD_JSON_NUMBER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || is_eof(character)) {
        if (tkz_buffer_end_with(parser->temp_buffer, "-", 1)
            || tkz_buffer_end_with(parser->temp_buffer, "E", 1)
            || tkz_buffer_end_with(parser->temp_buffer, "e", 1)) {
            SET_ERR(PCEJSON_ERROR_BAD_JSON_NUMBER);
            RETURN_AND_STOP_PARSE();
        }
        double d = strtod(
                tkz_buffer_get_bytes(parser->temp_buffer), NULL);
        RESTORE_VCM_NODE();
        struct pcvcm_node *node = pcvcm_node_new_number(d);
        APPEND_AS_VCM_CHILD(node);
        RESET_TEMP_BUFFER();
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER_INTEGER)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER);
    }
    if (is_ascii_digit(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_INTEGER);
    }
    if (character == 'x') {
        if(tkz_buffer_equal_to(parser->temp_buffer, "0", 1)) {
            RESET_TEMP_BUFFER();
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_HEX);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'E' || character == 'e') {
        APPEND_TO_TEMP_BUFFER('e');
        ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_EXPONENT);
    }
    if (character == '.' || character == 'F') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_FRACTION);
    }
    if (character == 'U' || character == 'L') {
        RECONSUME_IN(TKZ_STATE_EJSON_VALUE_NUMBER_SUFFIX_INTEGER);
    }
    if (character == 'I' && (
                tkz_buffer_is_empty(parser->temp_buffer) ||
                tkz_buffer_equal_to(parser->temp_buffer, "-", 1)
                )) {
        RECONSUME_IN(TKZ_STATE_EJSON_VALUE_NUMBER_INFINITY);
    }
    if (is_eof(character)) {
        ADVANCE_TO(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER_FRACTION)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || is_eof(character)) {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER);
    }

    if (is_ascii_digit(character)) {
        if (tkz_buffer_end_with(parser->temp_buffer, "F", 1)) {
            SET_ERR(PCEJSON_ERROR_BAD_JSON_NUMBER);
            RETURN_AND_STOP_PARSE();
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_FRACTION);
    }
    if (character == 'F') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_FRACTION);
    }
    if (character == 'L') {
        if (tkz_buffer_end_with(parser->temp_buffer, "F", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            long double ld = strtold (
                    tkz_buffer_get_bytes(parser->temp_buffer), NULL);
            RESTORE_VCM_NODE();
            struct pcvcm_node *node = pcvcm_node_new_longdouble(ld);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            ADVANCE_TO(TKZ_STATE_EJSON_AFTER_VALUE);
        }
    }
    if (character == 'E' || character == 'e') {
        if (tkz_buffer_end_with(parser->temp_buffer, ".", 1)) {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION);
            RETURN_AND_STOP_PARSE();
        }
        APPEND_TO_TEMP_BUFFER('e');
        ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_EXPONENT);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER_EXPONENT)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER);
    }
    if (is_ascii_digit(character)) {
        RECONSUME_IN(TKZ_STATE_EJSON_VALUE_NUMBER_EXPONENT_INTEGER);
    }
    if (character == '+' || character == '-') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_EXPONENT_INTEGER);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER_EXPONENT_INTEGER)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER);
    }
    if (is_ascii_digit(character)) {
        if (tkz_buffer_end_with(parser->temp_buffer, "F", 1)) {
            SET_ERR(PCEJSON_ERROR_BAD_JSON_NUMBER);
            RETURN_AND_STOP_PARSE();
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_EXPONENT_INTEGER);
    }
    if (character == 'F') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_EXPONENT_INTEGER);
    }
    if (character == 'L') {
        if (tkz_buffer_end_with(parser->temp_buffer, "F", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            long double ld = strtold (
                    tkz_buffer_get_bytes(parser->temp_buffer), NULL);
            RESTORE_VCM_NODE();
            struct pcvcm_node *node = pcvcm_node_new_longdouble(ld);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            ADVANCE_TO(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER_SUFFIX_INTEGER)
    uint32_t last_c = tkz_buffer_get_last_char(
            parser->temp_buffer);
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER);
    }
    if (character == 'U') {
        if (is_ascii_digit(last_c)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_SUFFIX_INTEGER);
        }
    }
    if (character == 'L') {
        if (is_ascii_digit(last_c) || last_c == 'U') {
            APPEND_TO_TEMP_BUFFER(character);
            if (tkz_buffer_end_with(parser->temp_buffer, "UL", 2)
                    ) {
                uint64_t u64 = strtoull (
                    tkz_buffer_get_bytes(parser->temp_buffer),
                    NULL, 10);
                RESTORE_VCM_NODE();
                struct pcvcm_node *node = pcvcm_node_new_ulongint(u64);
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
                ADVANCE_TO(TKZ_STATE_EJSON_AFTER_VALUE);
            }
            else if (tkz_buffer_end_with(parser->temp_buffer,
                        "L", 1)) {
                int64_t i64 = strtoll (
                    tkz_buffer_get_bytes(parser->temp_buffer),
                    NULL, 10);
                RESTORE_VCM_NODE();
                struct pcvcm_node *node = pcvcm_node_new_longint(i64);
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
                ADVANCE_TO(TKZ_STATE_EJSON_AFTER_VALUE);
            }
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER_HEX)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER_HEX);
    }
    if (is_ascii_hex_digit(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_HEX);
    }
    if (character == 'U' || character == 'L') {
        RECONSUME_IN(TKZ_STATE_EJSON_VALUE_NUMBER_HEX_SUFFIX);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER_HEX_SUFFIX)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER_HEX);
    }
    uint32_t last_c = tkz_buffer_get_last_char(parser->temp_buffer);
    if (character == 'U') {
        if (is_ascii_hex_digit(last_c)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_HEX_SUFFIX);
        }
    }
    if (character == 'L') {
        if (is_ascii_hex_digit(last_c) || last_c == 'U') {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_HEX_SUFFIX);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER_HEX)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || is_eof(character)) {
        const char *bytes = tkz_buffer_get_bytes(parser->temp_buffer);
        if (tkz_buffer_end_with(parser->temp_buffer, "U", 1)
                || tkz_buffer_end_with(parser->temp_buffer, "UL", 2)
                ) {
            uint64_t u64 = strtoull (bytes, NULL, 16);
            RESTORE_VCM_NODE();
            struct pcvcm_node *node = pcvcm_node_new_ulongint(u64);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        else {
            int64_t i64 = strtoll (bytes, NULL, 16);
            RESTORE_VCM_NODE();
            struct pcvcm_node *node = pcvcm_node_new_longint(i64);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER_INFINITY)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        if (tkz_buffer_equal_to(parser->temp_buffer,
                    "-Infinity", 9)) {
            double d = -INFINITY;
            RESTORE_VCM_NODE();
            struct pcvcm_node *node = pcvcm_node_new_number(d);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        if (tkz_buffer_equal_to(parser->temp_buffer,
                "Infinity", 8)) {
            double d = INFINITY;
            RESTORE_VCM_NODE();
            struct pcvcm_node *node = pcvcm_node_new_number(d);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'I') {
        if (tkz_buffer_is_empty(parser->temp_buffer)
            || tkz_buffer_equal_to(parser->temp_buffer, "-", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_INFINITY);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (character == 'n') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "I", 1)
          || tkz_buffer_equal_to(parser->temp_buffer, "-I", 2)
          || tkz_buffer_equal_to(parser->temp_buffer, "Infi", 4)
          || tkz_buffer_equal_to(parser->temp_buffer, "-Infi", 5)
            ) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_INFINITY);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (character == 'f') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "In", 2)
            || tkz_buffer_equal_to (parser->temp_buffer, "-In", 3)
                ) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_INFINITY);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (character == 'i') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "Inf", 3)
         || tkz_buffer_equal_to(parser->temp_buffer, "-Inf", 4)
         || tkz_buffer_equal_to(parser->temp_buffer, "Infin", 5)
         || tkz_buffer_equal_to(parser->temp_buffer, "-Infin", 6)
         ) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_INFINITY);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (character == 't') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "Infini", 6)
            || tkz_buffer_equal_to (parser->temp_buffer,
                "-Infini", 7)
                ) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_INFINITY);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (character == 'y') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "Infinit", 7)
           || tkz_buffer_equal_to (parser->temp_buffer,
               "-Infinit", 8)
                ) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_INFINITY);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NAN)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "NaN", 3)) {
            double d = NAN;
            RESTORE_VCM_NODE();
            struct pcvcm_node *node = pcvcm_node_new_number(d);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'N') {
        if (tkz_buffer_is_empty(parser->temp_buffer)
          || tkz_buffer_equal_to(parser->temp_buffer, "Na", 2)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NAN);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (character == 'a') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "N", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NAN);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_STRING_ESCAPE)
    switch (character)
    {
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
            APPEND_TO_TEMP_BUFFER('\\');
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(parser->return_state);
            break;
        case '$':
        case '{':
        case '}':
        case '<':
        case '>':
        case '/':
        case '\\':
        case '"':
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(parser->return_state);
            break;
        case 'u':
            RESET_STRING_BUFFER();
            ADVANCE_TO(
              TKZ_STATE_EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS);
            break;
        default:
            SET_ERR(PCEJSON_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY);
            RETURN_AND_STOP_PARSE();
    }
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS)
    if (is_ascii_hex_digit(character)) {
        APPEND_TO_STRING_BUFFER(character);
        size_t nr_chars = tkz_buffer_get_size_in_chars(
                parser->string_buffer);
        if (nr_chars == 4) {
            APPEND_BYTES_TO_TEMP_BUFFER("\\u", 2);
            APPEND_BUFFER_TO_TEMP_BUFFER(parser->string_buffer);
            RESET_STRING_BUFFER();
            ADVANCE_TO(parser->return_state);
        }
        ADVANCE_TO(
            TKZ_STATE_EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS);
    }
    SET_ERR(PCEJSON_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_JSONEE_VARIABLE)
    if (character == '"') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            RECONSUME_IN(TKZ_STATE_EJSON_VALUE_DOUBLE_QUOTED);
        }
    }
    if (character == '\'') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            RESET_QUOTED_COUNTER();
            RECONSUME_IN(TKZ_STATE_EJSON_VALUE_SINGLE_QUOTED);
        }
    }
    if (character == '$') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }

        uint32_t uc = ejson_stack_top();
        if (uc == 'P') {
            struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                    NULL);
            UPDATE_VCM_NODE(snode);
            struct pcvcm_node* node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        else {
            parser->vcm_node = pcvcm_node_new_string(
                       tkz_buffer_get_bytes(parser->temp_buffer));
            RESET_TEMP_BUFFER();
        }

        uc = ejson_stack_top();
        while (uc == '$') {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            uc = ejson_stack_top();
        }
        if (uc == '(' || uc == '<' || uc == '.' || uc == '"') {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    if (character == '_' || is_ascii_digit(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_VARIABLE);
    }
    if (is_ascii_alpha(character) || character == '-') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_VARIABLE);
    }
    if (is_whitespace(character) || character == '}' || character == '"'
            || character == ']' || character == ')' || character == ';'
            || character == '&' || character == '|') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCEJSON_ERROR_BAD_JSONEE_VARIABLE_NAME);
            RETURN_AND_STOP_PARSE();
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   tkz_buffer_get_bytes(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        uint32_t uc = ejson_stack_top();
        while (uc == '$') {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            uc = ejson_stack_top();
        }
        if (uc == '(' || uc == '<' || uc == '.' || uc == '"' || uc == 'T') {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    if (character == ',') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCEJSON_ERROR_BAD_JSONEE_VARIABLE_NAME);
            RETURN_AND_STOP_PARSE();
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   tkz_buffer_get_bytes(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        uint32_t uc = ejson_stack_top();
        while (uc == '$') {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            uc = ejson_stack_top();
        }
        if (uc == '(' || uc == '<') {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
    }
    if (character == ':') {
        if (tkz_buffer_is_empty(parser->temp_buffer)
            || tkz_buffer_is_int(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_VARIABLE);
        }
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_VARIABLE);
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   tkz_buffer_get_bytes(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        uint32_t uc = ejson_stack_top();
        while (uc == '$') {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            uc = ejson_stack_top();
        }
        if (uc == '(' || uc == '<' || uc == '{') {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        if (uc == 'P') {
            ejson_stack_pop();
            ejson_stack_push('{');
            ejson_stack_push(':');
            struct pcvcm_node *node = pcvcm_node_new_object(0, NULL);
            APPEND_CHILD(node, parser->vcm_node);
            UPDATE_VCM_NODE(node);
        }
        else if (uc == '{') {
            ejson_stack_push(':');
        }
        if (ejson_stack_is_empty()) {
            RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
        }
        ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
    }
    if (is_context_variable(character)) {
        if (tkz_buffer_is_empty(parser->temp_buffer)
            || tkz_buffer_is_int(parser->temp_buffer)
            || tkz_buffer_start_with(parser->temp_buffer, "#", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_VARIABLE);
        }
    }
    if (character == '#') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_VARIABLE);
        }
    }
    if (character == '[' || character == '(') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCEJSON_ERROR_BAD_JSONEE_VARIABLE_NAME);
            RETURN_AND_STOP_PARSE();
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   tkz_buffer_get_bytes(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        uint32_t uc = ejson_stack_top();
        if (uc == '$') {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    if (character == '<' || character == '>') {
        // FIXME
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   tkz_buffer_get_bytes(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        uint32_t uc = ejson_stack_top();
        if (uc == '$') {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    if (character == '.') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCEJSON_ERROR_BAD_JSONEE_VARIABLE_NAME);
            RETURN_AND_STOP_PARSE();
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   tkz_buffer_get_bytes(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        uint32_t uc = ejson_stack_top();
        if (uc == '$') {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_JSONEE_FULL_STOP_SIGN);
    }
    if (character == '=') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_VARIABLE);
        }
    }
    SET_ERR(PCEJSON_ERROR_BAD_JSONEE_VARIABLE_NAME);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_JSONEE_KEYWORD)
    if (is_ascii_digit(character)) {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCEJSON_ERROR_BAD_JSONEE_KEYWORD);
            RETURN_AND_STOP_PARSE();
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_KEYWORD);
    }
    if (is_ascii_alpha(character) || character == '_' ||
            character == '-') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_KEYWORD);
    }
    if (is_delimiter(character) || character == '"') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCEJSON_ERROR_BAD_JSONEE_KEYWORD);
            RETURN_AND_STOP_PARSE();
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   tkz_buffer_get_bytes(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        ejson_stack_pop();
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    if (character == ',') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCEJSON_ERROR_BAD_JSONEE_KEYWORD);
            RETURN_AND_STOP_PARSE();
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   tkz_buffer_get_bytes(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        ejson_stack_pop();
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        uint32_t uc = ejson_stack_top();
        if (uc == '(' || uc == '<') {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
    }
    if (character == '.') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCEJSON_ERROR_BAD_JSONEE_KEYWORD);
            RETURN_AND_STOP_PARSE();
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   tkz_buffer_get_bytes(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        ejson_stack_pop();
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        RECONSUME_IN(TKZ_STATE_EJSON_JSONEE_FULL_STOP_SIGN);
    }
    SET_ERR(PCEJSON_ERROR_BAD_JSONEE_KEYWORD);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_JSONEE_STRING)
    uint32_t uc = ejson_stack_top();
    if (is_whitespace(character)) {
        if (uc == 'U') {
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_JSONEE_STRING);
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_STRING);
    }
    if (character == '$') {
    //    if (uc != 'U' && uc != '"') {
            if (parser->vcm_node) {
                vcm_stack_push(parser->vcm_node);
            }
            struct pcvcm_node *snode = pcvcm_node_new_concat_string(0,
                    NULL);
            UPDATE_VCM_NODE(snode);
            ejson_stack_push('"');
            if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                struct pcvcm_node *node = pcvcm_node_new_string(
                   tkz_buffer_get_bytes(parser->temp_buffer));
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
      //          ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_STRING);
            }
     //   }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    if (character == '\\') {
        SET_RETURN_STATE(curr_state);
        ADVANCE_TO(TKZ_STATE_EJSON_STRING_ESCAPE);
    }
    if (character == '"') {
        if (parser->vcm_node) {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_JSONEE_STRING);
    }
    if (is_eof(character)) {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            if (tkz_buffer_end_with(parser->temp_buffer, "\n", 1)) {
                tkz_buffer_delete_tail_chars(parser->temp_buffer, 1);
            }
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer));
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    if (character == ':' && uc == ':') {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RESET_TEMP_BUFFER();
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_STRING);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_AFTER_JSONEE_STRING)
    uint32_t uc = ejson_stack_top();
    if (is_whitespace(character)) {
        if (parser->vcm_node &&
                parser->vcm_node->type != PCVCM_NODE_TYPE_FUNC_CONCAT_STRING) {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        if (uc == 'U') {
            ejson_stack_pop();
            if (!ejson_stack_is_empty()) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
        }
        RECONSUME_IN(TKZ_STATE_EJSON_JSONEE_STRING);
    }
    if (character == '"') {
        if (uc == 'U') {
            SET_ERR(PCEJSON_ERROR_BAD_JSONEE_NAME);
            RETURN_AND_STOP_PARSE();
        }
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        ejson_stack_pop();
        if (!ejson_stack_is_empty()) {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
    }
    if (character == '}' || character == ']' || character == ')') {
        if (uc == '"') {
            RECONSUME_IN(TKZ_STATE_EJSON_JSONEE_STRING);
        }
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        ejson_stack_pop();
        if (!ejson_stack_is_empty()) {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
    }
    SET_ERR(PCEJSON_ERROR_BAD_JSONEE_NAME);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_AMPERSAND)
    if (character == '&') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_AMPERSAND);
    }
    {
        if (tkz_buffer_equal_to(parser->temp_buffer, "&&", 2)) {
            uint32_t uc = ejson_stack_top();
            while (uc != 0 && uc != 'C') {
                ejson_stack_pop();
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
                uc = ejson_stack_top();
            }
            if (parser->vcm_node &&
                    parser->vcm_node->type != PCVCM_NODE_TYPE_CJSONEE) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            if (parser->vcm_node &&
                    parser->vcm_node->type == PCVCM_NODE_TYPE_CJSONEE) {
                struct pcvcm_node *node = pcvcm_node_new_cjsonee_op_and();
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
                RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
            }
            RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_OR_SIGN)
    if (character == '|') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_OR_SIGN);
    }
    {
        if (tkz_buffer_equal_to(parser->temp_buffer, "||", 2)) {
            uint32_t uc = ejson_stack_top();
            while (uc != 0 && uc != 'C') {
                ejson_stack_pop();
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
                uc = ejson_stack_top();
            }
            if (parser->vcm_node &&
                    parser->vcm_node->type != PCVCM_NODE_TYPE_CJSONEE) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            if (parser->vcm_node &&
                    parser->vcm_node->type == PCVCM_NODE_TYPE_CJSONEE) {
                struct pcvcm_node *node = pcvcm_node_new_cjsonee_op_or();
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
                RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
            }
            RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_SEMICOLON)
    if (character == ';') {
        uint32_t uc = ejson_stack_top();
        while (uc != 0 && uc != 'C') {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            uc = ejson_stack_top();
        }
        if (parser->vcm_node &&
                parser->vcm_node->type != PCVCM_NODE_TYPE_CJSONEE) {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        if (parser->vcm_node &&
                parser->vcm_node->type == PCVCM_NODE_TYPE_CJSONEE) {
            struct pcvcm_node *node = pcvcm_node_new_cjsonee_op_semicolon();
            APPEND_AS_VCM_CHILD(node);
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
        }
        RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
    }
    RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_CJSONEE_FINISHED)
    if (character == '}') {
        APPEND_TO_TEMP_BUFFER(character);
        if (tkz_buffer_equal_to(parser->temp_buffer, "}}", 2)) {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
        }
        ADVANCE_TO(TKZ_STATE_EJSON_CJSONEE_FINISHED);
    }
    if (tkz_buffer_equal_to(parser->temp_buffer, "}}", 2)) {
        ejson_stack_pop();
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()


PCEJSON_PARSER_END
#endif

