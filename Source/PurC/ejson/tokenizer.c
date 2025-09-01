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

#include "private/dvobjs.h"
#include "tokenizer.h"

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/stack.h"
#include "private/tkz-helper.h"
#include "private/atom-buckets.h"
#include "private/vcm.h"
#include "purc-errors.h"

#include <math.h>

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

#define ERROR_BUF_SIZE          100
#define NR_CONSUMED_LIST_LIMIT  10

#define INVALID_CHARACTER       0xFFFFFFFF

#define tkz_stack_is_empty()  pcejson_token_stack_is_empty(parser->tkz_stack)
#define tkz_stack_top()       pcejson_token_stack_top(parser->tkz_stack)
#define tkz_stack_pop()       pcejson_tkz_stack_pop(parser)
#define tkz_stack_size()      pcejson_token_stack_size(parser->tkz_stack)
#define tkz_stack_reset()     pcejson_token_stack_clear(parser->tkz_stack)
#define tkz_stack_drop_top()                                                \
    do {                                                                    \
        struct pcejson_token *t = tkz_stack_pop();                          \
        pcejson_token_destroy(t);                                           \
    } while (false)

#define tkz_curr_token()      pcejson_token_stack_top(parser->tkz_stack)
#define tkz_prev_token()      tkz_stack_prev_token(parser->tkz_stack)
#define tkz_get_token(pos)    tkz_stack_get_token(parser->tkz_stack, pos)

#define tkz_stack_push(c) do {                                              \
    if (!pcejson_tkz_stack_push(parser, c, -1)) {                           \
        return -1;                                                          \
    }                                                                       \
} while(false)

#define tkz_stack_push_ex(c, pos) do {                                      \
    if (!pcejson_tkz_stack_push(parser, c, pos)) {                          \
        return -1;                                                          \
    }                                                                       \
} while(false)

#define CHECK_FINISHED() do {                                               \
    if (is_finished_by_callback(parser, character)) {                       \
        update_tkz_stack(parser);                                           \
        RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);                             \
    }                                                                       \
    if ((parser->flags & PCEJSON_FLAG_MULTI_JSONEE) == 0) {                 \
        if (1 == tkz_stack_size() && pcejson_token_is_closed(top)) {        \
            RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);                         \
        }                                                                   \
    }                                                                       \
} while(false)

#define PRINT_STATE(parser) print_parser_state(parser)

#define PCEJSON_PARSER_BEGIN                                                \
int pcejson_parse_full(struct pcvcm_node **vcm_tree,                        \
        struct pcejson **parser_param,                                      \
        struct tkz_reader *reader,                                          \
        uint32_t depth,                                                     \
        pcejson_parse_is_finished_fn is_finished)                           \
{                                                                           \
    if (*parser_param == NULL) {                                            \
        *parser_param = pcejson_create(                                     \
                depth > 0 ? depth : EJSON_MAX_DEPTH, PCEJSON_FLAG_ALL);     \
        (*parser_param)->state = EJSON_TKZ_STATE_DATA;                      \
        if (*parser_param == NULL) {                                        \
            return -1;                                                      \
        }                                                                   \
    }                                                                       \
                                                                            \
    struct pcejson_token *top = NULL;                                       \
                                                                            \
    uint32_t character = 0;                                                 \
    struct pcejson* parser = *parser_param;                                 \
    parser->tkz_reader = reader;                                            \
    parser->is_finished = is_finished;                                      \
    START_RECORD_UCS();                                                     \
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
    if (is_separator(character) && (                                        \
           (parser->state != EJSON_TKZ_STATE_VALUE_TRIPLE_DOUBLE_QUOTED)     \
        && (parser->state != EJSON_TKZ_STATE_VALUE_SINGLE_QUOTED)           \
        && (parser->state != EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED)           \
                                                                     )) {   \
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
    APPEND_TO_RAW_BUFFER(character);                                        \
                                                                            \
    if (parser->record_ucs) {                                               \
        APPEND_TO_TEMP_UCS(*parser->curr_uc);                               \
    }                                                                       \
                                                                            \
next_state:                                                                 \
    top = tkz_stack_top();                                                  \
    switch (parser->state) {

#define PCEJSON_PARSER_END                                                  \
    default:                                                                \
        break;                                                              \
    }                                                                       \
    return -1;                                                              \
}

static const uint32_t numeric_char_ref_extension_array[32] = {
    0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, // 80-87
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F, // 88-8F
    0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, // 90-97
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178, // 98-9F
};

struct pcejson_token *
tkz_stack_get_token(struct pcejson_token_stack *stack, int pos)
{
    assert(pos >= 0);
    size_t nr = pcejson_token_stack_size(stack);
    int idx = nr - pos - 1;
    if (idx >= 0) {
        return pcejson_token_stack_get(stack, idx);
    }
    return NULL;
}

struct pcejson_token *
tkz_stack_prev_token(struct pcejson_token_stack *stack)
{
    return tkz_stack_get_token(stack, 1);
}

static bool
is_get_element(uint32_t type)
{
    return type == ETT_GET_MEMBER || type == ETT_GET_MEMBER_BY_BRACKET;
}

static bool
is_finished_by_callback(struct pcejson *parser, uint32_t character)
{
    bool ret = parser->is_finished(parser, character);
    if (ret) {
        parser->finished_by_callback = true;
    }
    /* keep state */
    return ret;
}

static bool
is_parse_finished(struct pcejson *parser, uint32_t character)
{
    if (is_eof(character)
            || is_finished_by_callback(parser, character)) {
        return true;
    }
    if ((parser->flags & PCEJSON_FLAG_MULTI_JSONEE) == 0) {
        struct pcejson_token *curr = tkz_curr_token();
        if (1 == tkz_stack_size() && pcejson_token_is_closed(curr)) {
            return true;
        }
    }

    return false;
}

static void
close_token(struct pcejson *parser, struct pcejson_token *token)
{
    (void) parser;
    pcejson_token_close(token);
}

#if 0
static bool
is_op_expr(struct pcejson_token *token)
{
    return token && (token->type == ETT_OP_EXPR);
}
#endif

static bool
is_op_expr_in_func(struct pcejson_token *token)
{
    return token && (token->type == ETT_OP_EXPR_IN_FUNC);
}

static bool
is_any_op_expr(struct pcejson_token *token)
{
    return token &&
           (token->type == ETT_OP_EXPR || token->type == ETT_OP_EXPR_IN_FUNC);
}

static struct pcejson_token *pcejson_tkz_stack_pop(struct pcejson *parser);
static int
update_tkz_stack_with_level(struct pcejson *parser, int level)
{
    int ret = 0;
    int nr = tkz_stack_size();
    if (nr <= 1) {
        goto out;
    }

    struct pcejson_token *token = NULL;
    struct pcejson_token *parent = NULL;
    int cr;

again:
    cr = 0;
    token = tkz_stack_top();
    if (!pcejson_token_is_closed(token)) {
        goto out;
    }

    token = tkz_stack_pop();
    parent = tkz_stack_top();
    if (parent == NULL || pcejson_token_is_closed(parent)) {
        pcejson_token_stack_push_token(parser->tkz_stack, token);
        goto out;
    }
    switch (parent->type) {
    case ETT_VALUE:
        parent->node = token->node;
        token->node = NULL;
        pcejson_token_destroy(token);
        goto again;
        break;

    case ETT_OBJECT:
    case ETT_ARRAY:
    case ETT_TUPLE:
        pcvcm_node_append_child(parent->node, token->node);
        token->node = NULL;
        pcejson_token_destroy(token);
        break;

    case ETT_GET_VARIABLE:
        while (parent && parent->type == ETT_GET_VARIABLE) {
            cr++;
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);
            token = NULL;

            close_token(parser, parent);

            size_t nr = tkz_stack_size();
            if (nr == 1 || cr >= level) {
                break;
            }
            token = tkz_stack_pop();
            parent = tkz_stack_top();
        }
        if (token && token->type == ETT_GET_VARIABLE) {
            pcejson_token_stack_push_token(parser->tkz_stack, token);
            goto again;
        }
        break;

    case ETT_GET_MEMBER:
        pcvcm_node_append_child(parent->node, token->node);
        token->node = NULL;
        pcejson_token_destroy(token);
        close_token(parser, parent);  /* auto close */
        break;

    case ETT_GET_MEMBER_BY_BRACKET:
        pcvcm_node_append_child(parent->node, token->node);
        token->node = NULL;
        pcejson_token_destroy(token);
        break;

    case ETT_CALL_GETTER:
    case ETT_CALL_SETTER:
        pcvcm_node_append_child(parent->node, token->node);
        token->node = NULL;
        pcejson_token_destroy(token);
        break;

    case ETT_PROTECT:
        parent->node = token->node;
        token->node = NULL;
        pcejson_token_destroy(token);
        break;

    case ETT_MULTI_UNQUOTED_S:
        pcvcm_node_append_child(parent->node, token->node);
        token->node = NULL;
        pcejson_token_destroy(token);
        break;

    case ETT_MULTI_QUOTED_S:
        pcvcm_node_append_child(parent->node, token->node);
        token->node = NULL;
        pcejson_token_destroy(token);
        break;

    case ETT_CJSONEE:
        pcvcm_node_append_child(parent->node, token->node);
        token->node = NULL;
        pcejson_token_destroy(token);
        break;

    case ETT_TRIPLE_DOUBLE_QUOTED:
        pcvcm_node_append_child(parent->node, token->node);
        token->node = NULL;
        pcejson_token_destroy(token);
        break;

    default:
        pcejson_token_stack_push_token(parser->tkz_stack, token);
        break;
    }

out:
    return ret;
}

static int
update_tkz_stack(struct pcejson *parser)
{
    return update_tkz_stack_with_level(parser, tkz_stack_size());
}

static struct pcejson_token *
token_stack_push(struct pcejson *parser, uint32_t type, int32_t pos)
{
    struct pcejson_token_stack *stack = parser->tkz_stack;
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
        /* { */
        token->node->position = tkz_ucs_find_reverse(parser->temp_ucs, '{');
        break;

    case ETT_ARRAY:
        token->node = pcvcm_node_new_array(0, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        /* [ */
        token->node->position = tkz_ucs_find_reverse(parser->temp_ucs, '[');
        break;

    case ETT_TUPLE:
        token->node = pcvcm_node_new_tuple(0, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        /* [! */
        token->node->position = tkz_ucs_find_reverse(parser->temp_ucs, '[');
        break;

    case ETT_CALL_GETTER:
        token->node = pcvcm_node_new_call_getter(NULL, 0, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        /* ( */
        token->node->position = tkz_ucs_find_reverse(parser->temp_ucs, '(');
        break;

    case ETT_CALL_SETTER:
        token->node = pcvcm_node_new_call_setter(NULL, 0, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        /* !( */
        token->node->position = tkz_ucs_find_reverse(parser->temp_ucs, '(');
        break;

    case ETT_GET_VARIABLE:
        token->node = pcvcm_node_new_get_variable(NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        /* $ */
        token->node->position = tkz_ucs_find_reverse(parser->temp_ucs, '$');
        break;

    case ETT_GET_MEMBER:
        token->node = pcvcm_node_new_get_element(NULL, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        /* . */
        token->node->position = tkz_ucs_find_reverse(parser->temp_ucs, '.');
        break;

    case ETT_GET_MEMBER_BY_BRACKET:
        token->node = pcvcm_node_new_get_element(NULL, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        /* [ */
        token->node->position = tkz_ucs_find_reverse(parser->temp_ucs, '[');
        break;

    case ETT_CJSONEE:
        token->node = pcvcm_node_new_cjsonee();
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        /* {{ */
        token->node->position = tkz_ucs_find_reverse(parser->temp_ucs, '{');
        if (token->node->position > 0) {
            token->node->position--;
        }
        break;

    case ETT_STRING:
        break;

    case ETT_MULTI_QUOTED_S:       /* multiple double quoted */
        token->node = pcvcm_node_new_concat_string(0, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        /* " */
        token->node->position = tkz_ucs_find_reverse(parser->temp_ucs, '"');
        break;

    case ETT_MULTI_UNQUOTED_S:       /* multiple unquoted*/
        token->node = pcvcm_node_new_concat_string(0, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        token->node->position = -1;
        break;

    case ETT_TRIPLE_DOUBLE_QUOTED:
        token->node = pcvcm_node_new_concat_string(0, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        /* """ */
        token->node->position = tkz_ucs_find_reverse(parser->temp_ucs, '"');
        if (token->node->position > 2) {
            token->node->position = token->node->position - 2;
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
        /* && */
        token->node->position = tkz_ucs_find_reverse(parser->temp_ucs, '&');
        if (token->node->position > 0) {
            token->node->position--;
        }
        break;

    case ETT_OR:
        token->node = pcvcm_node_new_cjsonee_op_or();
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        /* || */
        token->node->position = tkz_ucs_find_reverse(parser->temp_ucs, '|');
        if (token->node->position > 0) {
            token->node->position--;
        }
        break;

    case ETT_SEMICOLON:
        token->node = pcvcm_node_new_cjsonee_op_semicolon();
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        /* ; */
        token->node->position = tkz_ucs_find_reverse(parser->temp_ucs, ';');
        break;

    case ETT_BACKQUOTE:
        token->node = pcvcm_node_new_constant(0, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        /* ` */
        token->node->position = tkz_ucs_find_reverse(parser->temp_ucs, '`');
        break;

    case ETT_OP_EXPR:
        token->node = pcvcm_node_new_operator_expression(0, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        /* ( */
        token->node->position = tkz_ucs_find_reverse(parser->temp_ucs, '(');
        break;

    case ETT_OP_EXPR_IN_FUNC:
        token->node = pcvcm_node_new_operator_expression(0, NULL);
        if (!token->node) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
        token->node->position = pos;
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

static bool need_update_depth(uint32_t type)
{
    switch (type) {
    case ETT_GET_MEMBER:
    case ETT_GET_MEMBER_BY_BRACKET:
    case ETT_CALL_GETTER:
    case ETT_CALL_SETTER:
    case ETT_OBJECT:
    case ETT_ARRAY:
    case ETT_TUPLE:
        return true;

    default:
        return false;
    }
}

struct pcejson_token *
pcejson_tkz_stack_push(struct pcejson *parser, uint32_t type, int pos)
{
    if (need_update_depth(type)) {
        if (!pcejson_inc_depth(parser)) {
            SET_ERR(PCEJSON_ERROR_MAX_EMBEDDED_LEVELS);
            return NULL;
        }
    }

    struct pcejson_token *top = NULL;
    switch (type) {
    case ETT_GET_MEMBER:
    case ETT_GET_MEMBER_BY_BRACKET:
    case ETT_CALL_GETTER:
    case ETT_CALL_SETTER:
        {
            struct pcejson_token *token = tkz_stack_pop();
            top = token_stack_push(parser, type, pos);
            pcvcm_node_append_child(top->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);
        }
        break;

    default:
        top = token_stack_push(parser, type, pos);
        break;
    }

    return top;
}

struct pcejson_token *
pcejson_tkz_stack_pop(struct pcejson *parser)
{
    struct pcejson_token *token = (struct pcejson_token*)pcutils_stack_pop(
            parser->tkz_stack->stack);

    if (token && need_update_depth(token->type)) {
        pcejson_dec_depth(parser);
    }

    return token;
}

static inline bool
is_match_right_brace(uint32_t type)
{
    switch (type) {
    case ETT_OBJECT:
    case ETT_PROTECT:
    case ETT_CJSONEE:
        return true;
    }
    return false;
}

static inline bool
is_match_right_bracket(uint32_t type)
{

    /* ETT_GET_MEMBER is auto closed */
    switch (type) {
    case ETT_ARRAY:
    case ETT_TUPLE:
    case ETT_GET_MEMBER_BY_BRACKET:
        return true;
    }
    return false;
}

static inline bool
is_match_right_parenthesis(uint32_t type)
{
    switch (type) {
    case ETT_CALL_GETTER:
    case ETT_CALL_SETTER:
        return true;
    }
    return false;
}

static int
back_container_top(struct pcejson *parser)
{
    if (parser->enable_log) {
        PLOG("try to back_container size=%d|\n", tkz_stack_size());
    }
    struct pcejson_token *token = tkz_stack_top();
    while (token) {
        int nr = tkz_stack_size();
        if (parser->enable_log) {
            PLOG("token->type=%c|closed=%d\n", token->type,
                    pcejson_token_is_closed(token));
        }

        if (is_match_right_brace(token->type)) {
            break;
        }
        else if (is_match_right_bracket(token->type)) {
            break;
        }
        else if (is_match_right_parenthesis(token->type)) {
            break;
        }

        if (nr == 1) {
            break;
        }

        if (pcejson_token_is_closed(token)) {
            update_tkz_stack(parser);
            token = tkz_stack_top();
            continue;
        }
        break;
    }
    if (parser->enable_log) {
        PLOG("end to back_container size=%d\n", tkz_stack_size());
    }
    return 0;
}

static int
close_container(struct pcejson *parser, uint32_t character)
{
    if (parser->enable_log) {
        PLOG("try to close_container size=%d|type=%c\n", tkz_stack_size(),
                character);
    }
    struct pcejson_token *token = tkz_stack_top();
    while (token) {
        int nr = tkz_stack_size();
        if (parser->enable_log) {
            PLOG("token->type=%c|closed=%d\n", token->type,
                    pcejson_token_is_closed(token));
        }

        if (character == '}' && is_match_right_brace(token->type)) {
            close_token(parser, token);
            break;
        }
        else if (character == ']' && is_match_right_bracket(token->type)) {
            close_token(parser, token);
            break;
        }
        else if (character == ')' && is_match_right_parenthesis(token->type)) {
            close_token(parser, token);
            break;
        }

        if (nr == 1) {
            break;
        }

        if (pcejson_token_is_closed(token)) {
            update_tkz_stack(parser);
            if (token == tkz_stack_top()) {
                break;
            }
            token = tkz_stack_top();
            continue;
        }

        if (token->node == NULL) {
            tkz_stack_drop_top();
            token = tkz_stack_top();
            continue;
        }
        break;
    }
    if (parser->enable_log) {
        PLOG("end to close_container size=%d|type=%c\n", tkz_stack_size(),
                character);
    }
    return 0;

}

static struct pcvcm_node *
update_result(struct pcvcm_node *node)
{
    struct pcvcm_node *result = node;
    if (node->type == PCVCM_NODE_TYPE_FUNC_CONCAT_STRING) {
        size_t nr = pcvcm_node_children_count(node);
        if (nr == 1) {
            result = pcvcm_node_first_child(node);
            pcvcm_node_remove_child(node, result);
            pcvcm_node_destroy(node);
        }
    }
    return result;
}


int
build_jsonee(struct pcejson *parser)
{
    int ret = 0;
    struct pcvcm_node *root = NULL;
    int nr = tkz_stack_size();
    update_tkz_stack(parser);
    nr = tkz_stack_size();

    if (nr == 0) {
        goto out;
    }
    else if (nr == 1) {
        struct pcejson_token *token = tkz_stack_top();
        if (pcejson_token_is_closed(token)) {
            parser->vcm_node = update_result(token->node);
            token->node = NULL;
            tkz_stack_drop_top();
        }
        else if (token->type == ETT_MULTI_UNQUOTED_S ||
                token->type == ETT_MULTI_QUOTED_S) {
            close_token(parser, token);
            parser->vcm_node = update_result(token->node);
            token->node = NULL;
            tkz_stack_drop_top();
        }
        goto out;
    }

    root = pcvcm_node_new_concat_string(0, NULL);
    if (!root) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        ret = -1;
        goto out;
    }
    root->position = 0;

    for (int i = 0; i < nr; i++) {
        struct pcejson_token *token = pcejson_token_stack_get(
                parser->tkz_stack, i);
        if (!pcejson_token_is_closed(token)) {
            ret = -1;
            goto out;
        }
        pcvcm_node_append_child(root, token->node);
        token->node = NULL;
    }
    parser->vcm_node = root;
    root = NULL;

out:
    if (root) {
        pcvcm_node_destroy(root);
    }
    return ret;
}

static inline void
print_parser_state(struct pcejson *parser)
{
    if (!parser->enable_log) {
        return;
    }

    char buf[8] = {0};
    char *s_stack;
    struct tkz_uc *uc = parser->curr_uc;
    uint32_t character = uc->character;
    struct pcejson_token *top = tkz_stack_top();
    uint32_t type = top ? top->type : 0x20;
    struct pcvcm_node *vcm_node = top ? top->node : NULL;
    char *node = NULL;

    uc_to_utf8(character, buf);

    size_t nr_stack = tkz_stack_size();
    s_stack = (char*)malloc(nr_stack + 1);

    for (size_t i = 0; i < nr_stack; i++) {
        struct pcejson_token * token = pcejson_token_stack_get(
                parser->tkz_stack, i);
        s_stack[i] = (char)token->type;
    }
    s_stack[nr_stack] = 0;

    size_t len;
    node = pcvcm_node_to_string(vcm_node, &len);

    const char *tbuf = tkz_buffer_get_bytes(parser->temp_buffer);
    if (parser->enable_log) {
        PLOG(
                "in %-60s|uc=%2s|hex=0x%04X|utf8=%s"
                "|top=%1c|stack.size=%2ld|stack=%s|node=%s|tmp_buffer=%s|"
                "line=%d|column=%d\n",
                parser->state_name, buf, character, uc->utf8_buf,
                type, nr_stack, s_stack, node, tbuf, uc->line, uc->column
            );
    }
    free(s_stack);
    free(node);
}

PCEJSON_PARSER_BEGIN

BEGIN_STATE(EJSON_TKZ_STATE_DATA)
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    if (is_whitespace (character) || character == 0xFEFF) {
        ADVANCE_TO(EJSON_TKZ_STATE_DATA);
    }
    RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_FINISHED)
    int ret = build_jsonee(parser);
    if (ret == 0) {
        if (!(parser->flags & PCEJSON_FLAG_KEEP_LAST_CHAR)) {
            tkz_ucs_delete_tail(parser->temp_ucs, 1);
        }
        tkz_ucs_trim_tail(parser->temp_ucs);
        if (parser->vcm_node) {
            if (!parser->vcm_node->ucs) {
                parser->vcm_node->ucs = tkz_ucs_new();
            }
            tkz_ucs_move(parser->vcm_node->ucs, parser->temp_ucs);
            tkz_ucs_renumber(parser->vcm_node->ucs);
        }
        *vcm_tree = parser->vcm_node;
        parser->vcm_node = NULL;
    }
    if (*vcm_tree == NULL) {
        if (is_eof(character)) {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        }
        else {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        }
        ret = -1;
    }
    DELETE_FROM_RAW_BUFFER(1);
    return ret;
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_CONTROL)
    if (is_eof(character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
    }
    if (top && top->type == ETT_TRIPLE_DOUBLE_QUOTED) {
        tkz_stack_push(ETT_VALUE);
        RESET_TEMP_BUFFER();
        RESET_STRING_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_VALUE_TRIPLE_DOUBLE_QUOTED);
    }
    if (is_whitespace(character)) {
        if (!top) {
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
        if (pcejson_token_is_closed(top)) {
            if (1 == tkz_stack_size() &&
                    is_parse_finished(parser, character)) {
                RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
            }
            struct pcejson_token *token = tkz_stack_pop();
            top = tkz_stack_top();
            if (top) {
                if (top->type == ETT_MULTI_UNQUOTED_S) {
                    pcvcm_node_append_child(top->node, token->node);
                    token->node = NULL;
                    pcejson_token_destroy(token);
                    RESET_TEMP_BUFFER();
                    tkz_stack_push(ETT_VALUE);
                    RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
                }
                else if (top->type == ETT_MULTI_QUOTED_S) {
                    pcvcm_node_append_child(top->node, token->node);
                    token->node = NULL;
                    pcejson_token_destroy(token);
                    RESET_TEMP_BUFFER();
                    tkz_stack_push(ETT_VALUE);
                    RECONSUME_IN(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
                }
                else if (is_any_op_expr(top)) {
                    pcvcm_node_append_child(top->node, token->node);
                    token->node = NULL;
                    pcejson_token_destroy(token);
                } else if (top->type == ETT_OP_COND_THEN ||
                           top->type == ETT_OP_COND_ELSE) {
                    top->node = token->node;
                    token->node = NULL;
                    pcejson_token_destroy(token);
                }
                else if (top->type == ETT_OP_COMMA) {
                    pcvcm_node_append_child(top->node, token->node);
                    token->node = NULL;
                    pcejson_token_destroy(token);
                }
            }
            else {
                tkz_stack_push(ETT_MULTI_UNQUOTED_S);
                top = tkz_stack_top();
                pcvcm_node_append_child(top->node, token->node);
                token->node = NULL;
                pcejson_token_destroy(token);
                RESET_TEMP_BUFFER();
                tkz_stack_push(ETT_VALUE);
                RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
            }
        }
        else {
            if (top->type == ETT_MULTI_UNQUOTED_S) {
                RESET_TEMP_BUFFER();
                tkz_stack_push(ETT_VALUE);
                RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
            }
            else if (top->type == ETT_MULTI_QUOTED_S) {
                RESET_TEMP_BUFFER();
                tkz_stack_push(ETT_VALUE);
                RECONSUME_IN(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
            }
        }
        ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
    }
    if (character == '{') {
        RECONSUME_IN(EJSON_TKZ_STATE_LEFT_BRACE);
    }
    if (character == '}') {
        RECONSUME_IN(EJSON_TKZ_STATE_RIGHT_BRACE);
    }
    if (character == '[') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_LEFT_BRACKET);
    }
    if (character == ']') {
        RECONSUME_IN(EJSON_TKZ_STATE_RIGHT_BRACKET);
    }
    if (is_operator_sign(character)) {
        if (is_any_op_expr(top)) {
            RECONSUME_IN(EJSON_TKZ_STATE_OP_SIGN);
        }
        struct pcejson_token *prev = tkz_prev_token();
        if (is_any_op_expr(prev)) {
            RECONSUME_IN(EJSON_TKZ_STATE_OP_SIGN);
        }
        if (top &&
            ((top->type == ETT_CALL_GETTER || top->type == ETT_CALL_SETTER) &&
             !top->node->is_closed) &&
            (character != ',')) {
            if (character != '(') {
                CHECK_FINISHED();
            }
            if (character == ')') {
                struct pcvcm_node *last = pcvcm_node_last_child(top->node);
                if (last && last->type == PCVCM_NODE_TYPE_OPERATOR_EXPRESSION
                        && !last->is_closed) {
                    RECONSUME_IN(EJSON_TKZ_STATE_OP_SIGN);
                }
                else {
                    RECONSUME_IN(EJSON_TKZ_STATE_RIGHT_PARENTHESIS);
                }
            }
            RECONSUME_IN(EJSON_TKZ_STATE_OP_EXPR_IN_FUNC);
        }

        if (character == '+' && prev &&
            (prev->type == ETT_CALL_GETTER || prev->type == ETT_CALL_SETTER) &&
            top && top->type == ETT_VALUE && top->node == NULL) {
            CHECK_FINISHED();
            RECONSUME_IN(EJSON_TKZ_STATE_OP_EXPR_IN_FUNC);
        }
    }
    if (character == '!') {
        RECONSUME_IN(EJSON_TKA_STATE_EXCLAMATION_MARK);
    }
    if (character == '(') {
        if (!top) {
            RECONSUME_IN(EJSON_TKZ_STATE_OP_EXPR);
        }
        RECONSUME_IN(EJSON_TKZ_STATE_LEFT_PARENTHESIS);
    }
    if (character == ')') {
        struct pcejson_token *prev = tkz_prev_token();
        if (top && top->type == ETT_VALUE && prev &&
            prev->type == ETT_OP_COND_ELSE) {
            RECONSUME_IN(EJSON_TKZ_STATE_OP_CONDITIONAL);
        }
        if (top && top->type == ETT_VALUE && prev &&
            prev->type == ETT_OP_COMMA) {
            RECONSUME_IN(EJSON_TKZ_STATE_OP_COMMA);
        }
        RECONSUME_IN(EJSON_TKZ_STATE_RIGHT_PARENTHESIS);
    }
    if (character == '$' && (parser->flags & PCEJSON_FLAG_GET_VARIABLE)) {
        RECONSUME_IN(EJSON_TKZ_STATE_DOLLAR);
    }
    if (character == '&') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_AMPERSAND);
    }
    if (character == '|') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OR_SIGN);
    }
    if (character == ';') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_SEMICOLON);
    }
    if (character == '\'') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_SINGLE_QUOTED);
    }
    if (character == '"') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_DOUBLE_QUOTED);
    }
    if (character == '#' && !tkz_stack_is_empty()) {
        SET_RETURN_STATE(EJSON_TKZ_STATE_CONTROL);
        ADVANCE_TO(EJSON_TKZ_STATE_LINE_COMMENT);
    }
    if (character == '`') {
        RECONSUME_IN(EJSON_TKZ_STATE_BACKQUOTE);
    }
//    CHECK_FINISHED();
    RECONSUME_IN(EJSON_TKZ_STATE_UNQUOTED);
END_STATE()


BEGIN_STATE(EJSON_TKZ_STATE_SINGLE_QUOTED)
    uint32_t type = top ? top->type : 0;
    if (type == 0 || type == ETT_VALUE) {
        tkz_stack_push(ETT_SINGLE_S);
        tkz_stack_push(ETT_VALUE);
        RESET_SINGLE_QUOTED_COUNTER();
        RECONSUME_IN(EJSON_TKZ_STATE_VALUE_SINGLE_QUOTED);
    }
    if (type == ETT_MULTI_QUOTED_S || type == ETT_MULTI_UNQUOTED_S) {
        tkz_stack_push(ETT_VALUE);
        RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
#if 0
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            pcvcm_node_append_child(top->node, node);
        }
        update_tkz_stack(parser);

        RESET_TEMP_BUFFER();
        RESET_SINGLE_QUOTED_COUNTER();
        if (is_parse_finished(parser, character)) {
            RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
        }
        tkz_stack_push(ETT_SINGLE_S);
        tkz_stack_push(ETT_VALUE);
        RECONSUME_IN(EJSON_TKZ_STATE_VALUE_SINGLE_QUOTED);
#endif
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_DOUBLE_QUOTED)
    uint32_t type = top ? top->type : 0;
    if (type == 0 || type == ETT_VALUE) {
        tkz_stack_push(ETT_DOUBLE_S);
        tkz_stack_push(ETT_VALUE);
        RESET_TEMP_BUFFER();
        RESET_DOUBLE_QUOTED_COUNTER();
        RECONSUME_IN(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
    }
    if (type == ETT_MULTI_QUOTED_S) {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            node->quoted_type = PCVCM_NODE_QUOTED_TYPE_DOUBLE;
            node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            pcvcm_node_append_child(top->node, node);
        }
        close_token(parser, top);
        update_tkz_stack(parser);

        RESET_TEMP_BUFFER();
        RESET_DOUBLE_QUOTED_COUNTER();
        if (is_parse_finished(parser, character)) {
            RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
        }
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
    }
    if (top->type == ETT_MULTI_UNQUOTED_S) {
        RESET_TEMP_BUFFER();
        tkz_stack_push(ETT_VALUE);
        RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_UNQUOTED)
    if (is_ascii_digit(character) || character == '-') {
        if (top) {
            if (top->type == ETT_MULTI_UNQUOTED_S) {
                tkz_stack_push(ETT_VALUE);
                RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
            }
            else if (top->type == ETT_MULTI_QUOTED_S) {
                tkz_stack_push(ETT_VALUE);
                RECONSUME_IN(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
            }
        }
        else {
            tkz_stack_push(ETT_VALUE);
        }
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_VALUE_NUMBER);
    }
    if (character == 'I') {
        if (!top) {
            tkz_stack_push(ETT_VALUE);
        }
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_VALUE_NUMBER_INFINITY);
    }
    if (character == 'N') {
        if (!top) {
            tkz_stack_push(ETT_VALUE);
        }
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_VALUE_NAN);
    }
    if (character == 'b') {
        if (!top) {
            tkz_stack_push(ETT_VALUE);
        }
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_BYTE_SEQUENCE);
    }
    if (character == 't' || character == 'f' || character == 'n'
            || character == 'u') {
        if (!top) {
            tkz_stack_push(ETT_VALUE);
        }
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_KEYWORD);
    }
    if (character == ',') {
        if (top == NULL) {
            if (tkz_buffer_is_empty(parser->temp_buffer)) {
                tkz_stack_push(ETT_UNQUOTED_S);
                tkz_stack_push(ETT_VALUE);
                RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
            }
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
        back_container_top(parser);
        top = tkz_stack_top();
        if (pcejson_token_is_closed(top)) {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
        uint32_t type = top->type;
        switch (type) {
        case ETT_OBJECT:
            ADVANCE_TO(EJSON_TKZ_STATE_BEFORE_NAME);
            break;
        case ETT_ARRAY:
        case ETT_TUPLE:
            tkz_stack_push(ETT_VALUE);
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
            break;
        case ETT_CALL_GETTER:
        case ETT_CALL_SETTER:
            tkz_stack_push(ETT_VALUE);
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
            break;
        case ETT_MULTI_UNQUOTED_S:
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
            break;
        case ETT_MULTI_QUOTED_S:
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
            break;
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_COMMA);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '.') {
        if (top == NULL) {
//            SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
//            RETURN_AND_STOP_PARSE();
            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_UNQUOTED_S);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
        }
        uint32_t type = top->type;
        if (type == ETT_GET_VARIABLE || is_get_element(type)
                || type == ETT_CALL_SETTER || type == ETT_CALL_GETTER) {
            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_GET_MEMBER);
            tkz_stack_push(ETT_VALUE);
            ADVANCE_TO(EJSON_TKZ_STATE_VARIABLE);
        }
        if (type == ETT_VALUE) {
            struct pcejson_token *prev = tkz_prev_token();
            if (prev && prev->type == ETT_GET_MEMBER) {
                tkz_stack_pop();
                pcejson_token_destroy(top);
                top = tkz_stack_pop();


                struct pcvcm_node *child = pcvcm_node_first_child(top->node);
                pcvcm_node_remove_child(top->node, child);
                pcejson_token_destroy(top);

                APPEND_TO_TEMP_BUFFER(character);
                APPEND_TO_TEMP_BUFFER(character);

                top = tkz_stack_top();
                if (top == NULL) {
                    tkz_stack_push(ETT_UNQUOTED_S);
                    top = tkz_stack_top();
                    pcvcm_node_append_child(top->node, child);
                    tkz_stack_push(ETT_VALUE);
                    ADVANCE_TO(EJSON_TKZ_STATE_RAW_STRING);
                }
                else if (top->type == ETT_MULTI_UNQUOTED_S) {
                    pcvcm_node_append_child(top->node, child);
                    tkz_stack_push(ETT_VALUE);
                    ADVANCE_TO(EJSON_TKZ_STATE_RAW_STRING);
                }
                else if (top->type == ETT_MULTI_QUOTED_S) {
                    pcvcm_node_append_child(top->node, child);
                    tkz_stack_push(ETT_VALUE);
                    ADVANCE_TO(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
                }

            }
        }
    }
    if (character == ':') {
        if (top == NULL) {
            tkz_stack_push(ETT_UNQUOTED_S);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
        }
        if (top->type == ETT_OBJECT) {
            tkz_stack_push(ETT_VALUE);
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
        if (top->type == ETT_PROTECT) {
            top = tkz_stack_pop();
            tkz_stack_push(ETT_OBJECT);
            if (top->node) {
                pcejson_token_stack_push_token(parser->tkz_stack, top);
                update_tkz_stack(parser);
            }
            else {
                pcejson_token_destroy(top);
            }

            tkz_stack_push(ETT_VALUE);
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    CHECK_FINISHED();
    if (top == NULL) {
        tkz_stack_push(ETT_UNQUOTED_S);
        tkz_stack_push(ETT_VALUE);
        RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
    }
    if (top->type == ETT_MULTI_UNQUOTED_S) {
        tkz_stack_push(ETT_VALUE);
        RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
    }
    if (top->type == ETT_MULTI_QUOTED_S) {
        tkz_stack_push(ETT_VALUE);
        RECONSUME_IN(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
    }
    if (top->type == ETT_PROTECT) {
        if (pcejson_token_is_closed(top)) {
            if (1 == tkz_stack_size() &&
                    is_parse_finished(parser, character)) {
                RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
            }
            struct pcejson_token *token = tkz_stack_pop();
            top = tkz_stack_top();
            if (top) {
                if (top->type == ETT_MULTI_UNQUOTED_S) {
                    pcvcm_node_append_child(top->node, token->node);
                    token->node = NULL;
                    pcejson_token_destroy(token);
                    RESET_TEMP_BUFFER();
                    tkz_stack_push(ETT_VALUE);
                    RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
                }
                else if (top->type == ETT_MULTI_QUOTED_S) {
                    pcvcm_node_append_child(top->node, token->node);
                    token->node = NULL;
                    pcejson_token_destroy(token);
                    RESET_TEMP_BUFFER();
                    tkz_stack_push(ETT_VALUE);
                    RECONSUME_IN(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
                }
            }
            else {
                tkz_stack_push(ETT_MULTI_UNQUOTED_S);
                top = tkz_stack_top();
                pcvcm_node_append_child(top->node, token->node);
                token->node = NULL;
                pcejson_token_destroy(token);
                RESET_TEMP_BUFFER();
                tkz_stack_push(ETT_VALUE);
                RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
            }
            if (token) {
                pcejson_token_destroy(token);
            }
        }
    }

    uint32_t type = top->type;
    size_t nr = tkz_stack_size();
    if (type == ETT_VALUE &&  nr > 1) {
        tkz_stack_drop_top();
        top = tkz_stack_top();
        if (is_get_element(top->type)) {
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_VARIABLE);
        }
    }
    if (parser->hvml_double_quoted_attr_value) {
        if (!top) {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
        if (pcejson_token_is_closed(top)) {
            if (1 == tkz_stack_size() &&
                    is_parse_finished(parser, character)) {
                RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
            }
            struct pcejson_token *token = tkz_stack_pop();
            top = tkz_stack_top();
            if (top) {
                if (top->type == ETT_MULTI_UNQUOTED_S) {
                    pcvcm_node_append_child(top->node, token->node);
                    token->node = NULL;
                    pcejson_token_destroy(token);
                    RESET_TEMP_BUFFER();
                    tkz_stack_push(ETT_VALUE);
                    RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
                }
                else if (top->type == ETT_MULTI_QUOTED_S) {
                    pcvcm_node_append_child(top->node, token->node);
                    token->node = NULL;
                    pcejson_token_destroy(token);
                    RESET_TEMP_BUFFER();
                    tkz_stack_push(ETT_VALUE);
                    RECONSUME_IN(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
                }
            }
            else {
                tkz_stack_push(ETT_MULTI_UNQUOTED_S);
                top = tkz_stack_top();
                pcvcm_node_append_child(top->node, token->node);
                token->node = NULL;
                pcejson_token_destroy(token);
                RESET_TEMP_BUFFER();
                tkz_stack_push(ETT_VALUE);
                RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
            }
        }
        else {
            if (top->type == ETT_MULTI_UNQUOTED_S) {
                RESET_TEMP_BUFFER();
                tkz_stack_push(ETT_VALUE);
                RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
            }
            else if (top->type == ETT_MULTI_QUOTED_S) {
                RESET_TEMP_BUFFER();
                tkz_stack_push(ETT_VALUE);
                RECONSUME_IN(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
            }
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_LEFT_BRACE)
    if (character == '{') {
        tkz_stack_push(ETT_PROTECT);
        ADVANCE_TO(EJSON_TKZ_STATE_LEFT_BRACE);
    }
    if (character == '$' && (parser->flags & PCEJSON_FLAG_GET_VARIABLE)) {
        RECONSUME_IN(EJSON_TKZ_STATE_DOLLAR);
    }
    if (is_whitespace(character)) {
        if (top->type != ETT_PROTECT) {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }

        tkz_stack_pop();
        pcejson_token_destroy(top);
        top = tkz_stack_top();
        if (top && top->type == ETT_PROTECT) {
            tkz_stack_pop();
            pcejson_token_destroy(top);

            tkz_stack_push(ETT_CJSONEE);
            top = tkz_stack_top();
            tkz_stack_push(ETT_VALUE);
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
        else {
            tkz_stack_push(ETT_OBJECT);
            top = tkz_stack_top();
            RECONSUME_IN(EJSON_TKZ_STATE_BEFORE_NAME);
        }
    }
    if (top->type == ETT_PROTECT) {
        tkz_stack_pop();
        pcejson_token_destroy(top);
        tkz_stack_push(ETT_OBJECT);
        top = tkz_stack_top();
        RECONSUME_IN(EJSON_TKZ_STATE_BEFORE_NAME);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_RIGHT_BRACE)
    if (is_parse_finished(parser, character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
    }
    if (is_whitespace(character)) {
        update_tkz_stack(parser);
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    if (character == '}') {
        if (top == NULL) {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_RIGHT_BRACE);
            RETURN_AND_STOP_PARSE();
        }
        if ((top->type == ETT_CJSONEE || top->type == ETT_OBJECT)
                && pcejson_token_is_closed(top)) {
            update_tkz_stack(parser);

            struct pcejson_token *token = tkz_stack_top();
            if (token == top) {
                /* not paired match */
                SET_ERR(PCEJSON_ERROR_UNEXPECTED_RIGHT_BRACE);
                RETURN_AND_STOP_PARSE();
            }
        }
        close_container(parser, character);

        top = tkz_stack_top();
        uint32_t type = top ? top->type : 0;
        if (type == ETT_CJSONEE) {
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_CJSONEE_FINISHED);
        }
        if (type == ETT_OBJECT) {
            ADVANCE_TO(EJSON_TKZ_STATE_RIGHT_BRACE);
        }
        else if (type == ETT_PROTECT) {
            //update_tkz_stack_with_level(parser, 1);
            top = tkz_stack_top();
            if (top->node->extra & EXTRA_PROTECT_FLAG) {
                top->node->extra &= EXTRA_SUGAR_FLAG;
            }
            else {
                top->node->extra &= EXTRA_PROTECT_FLAG;
            }
            ADVANCE_TO(EJSON_TKZ_STATE_RIGHT_BRACE);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_RIGHT_BRACE);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '.' || character == '[') {
        if ((top->type == ETT_VALUE || top->type == ETT_PROTECT) && top->node) {
            if (top->node->type == PCVCM_NODE_TYPE_FUNC_GET_VARIABLE) {
                top->type = ETT_GET_VARIABLE;
            }
            else if (top->node->type == PCVCM_NODE_TYPE_FUNC_GET_MEMBER) {
                top->type = ETT_GET_MEMBER;
            }
            else if (top->node->type == PCVCM_NODE_TYPE_STRING) {
                top->type = ETT_VALUE;
                update_tkz_stack_with_level(parser, 1);
            }
        }
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    update_tkz_stack(parser);
    RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_LEFT_BRACKET)
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }


    if (character == '[') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_LEFT_BRACKET);
        }
    }
    else if (character == '!') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_LEFT_BRACKET);
    }

    if (tkz_buffer_equal_to(parser->temp_buffer, "[!", 2)) {
        RESET_TEMP_BUFFER();

        if (top == NULL) {
            tkz_stack_push(ETT_TUPLE);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        uint32_t type = top->type;
        // FIXME:
        if (type == ETT_OBJECT || type == ETT_ARRAY || type == ETT_TUPLE
                || type == ETT_STRING || type == ETT_VALUE) {
            tkz_stack_push(ETT_TUPLE);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    else if (tkz_buffer_equal_to(parser->temp_buffer, "[", 1)) {
        RESET_TEMP_BUFFER();

        if (top == NULL) {
            tkz_stack_push(ETT_ARRAY);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        uint32_t type = top->type;
        // FIXME:
        if (type == ETT_OBJECT || type == ETT_ARRAY || type == ETT_TUPLE
                || type == ETT_STRING || type == ETT_VALUE) {
            tkz_stack_push(ETT_ARRAY);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }

        if (type == ETT_GET_VARIABLE || is_get_element(type)) {
            tkz_stack_push(ETT_GET_MEMBER_BY_BRACKET);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }

        tkz_stack_push(ETT_GET_MEMBER_BY_BRACKET);
        tkz_stack_push(ETT_VALUE);
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }

    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_RIGHT_BRACKET)
    if (character == ']') {
        if (top == NULL) {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_RIGHT_BRACKET);
            RETURN_AND_STOP_PARSE();
        }
        if ((top->type == ETT_GET_MEMBER_BY_BRACKET
                    || top->type == ETT_ARRAY || top->type == ETT_TUPLE)
                && pcejson_token_is_closed(top)) {
            update_tkz_stack(parser);

            struct pcejson_token *token = tkz_stack_top();
            if (token == top) {
                /* not paired match */
                SET_ERR(PCEJSON_ERROR_UNEXPECTED_RIGHT_BRACKET);
                RETURN_AND_STOP_PARSE();
            }
        }

        close_container(parser, character);
        top = tkz_stack_top();
        if (top->type == ETT_GET_MEMBER_BY_BRACKET
                || top->type == ETT_ARRAY
                || top->type == ETT_TUPLE) {
            ADVANCE_TO(EJSON_TKZ_STATE_RIGHT_BRACKET);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_RIGHT_BRACKET);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '[' || character == '.') {
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    if (is_parse_finished(parser, character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
    }
    update_tkz_stack(parser);
    RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
END_STATE()

BEGIN_STATE(EJSON_TKA_STATE_EXCLAMATION_MARK)
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '!') {
        RESET_TEMP_BUFFER();
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKA_STATE_EXCLAMATION_MARK);
    }
    if (character == '(') {
        top = tkz_stack_top();
        if (top && (top->node == NULL) && (top->type == ETT_VALUE)) {
            struct pcejson_token *prev = tkz_prev_token();
            if (prev && prev->type == ETT_GET_VARIABLE) {
                SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
                RETURN_AND_STOP_PARSE();
            }
        }
        update_tkz_stack_with_level(parser, 1);

        tkz_stack_push(ETT_CALL_SETTER);
        tkz_stack_push(ETT_VALUE);
        ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
    }
    {
        update_tkz_stack(parser);
        top = tkz_stack_top();
        if (top->type == ETT_TRIPLE_DOUBLE_QUOTED) {
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_VALUE_TRIPLE_DOUBLE_QUOTED);
        }
        if (top->type == ETT_GET_MEMBER ||
                top->type == ETT_GET_MEMBER_BY_BRACKET ||
                top->type == ETT_GET_VARIABLE) {
            update_tkz_stack(parser);
        }
        top = tkz_stack_top();
        if (top->type == ETT_MULTI_QUOTED_S) {
//            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
        }
        top = tkz_stack_top();
        if (top && top->type == ETT_TRIPLE_DOUBLE_QUOTED) {
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (is_parse_finished(parser, character)) {
            if (top->type == ETT_MULTI_UNQUOTED_S) {
                close_token(parser, top);
            }
            update_tkz_stack(parser);
            RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
        }
        top = tkz_stack_top();
        if (top->type == ETT_MULTI_UNQUOTED_S) {
            //RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
        }
        if (is_parse_finished(parser, character)) {
            RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
        }
        if (top->type == ETT_TRIPLE_DOUBLE_QUOTED) {
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_VALUE_TRIPLE_DOUBLE_QUOTED);
        }
        size_t nr = tkz_stack_size();
        if (nr == 1 && pcejson_token_is_closed(top)) {
            struct pcejson_token *token = tkz_stack_pop();
            tkz_stack_push(ETT_MULTI_UNQUOTED_S);
            top = tkz_stack_top();

            pcvcm_node_append_child(top->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);
            //RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
        }
    }
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_LEFT_PARENTHESIS)
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '(') {
        if (top && top->type == ETT_VALUE) {
            struct pcejson_token *prev = tkz_prev_token();
            if (prev->type == ETT_CALL_GETTER || prev->type == ETT_CALL_SETTER) {
                tkz_stack_drop_top();
                RECONSUME_IN(EJSON_TKZ_STATE_OP_SIGN);
            }
        }

        if (tkz_buffer_equal_to(parser->temp_buffer, "!", 1)) {
            tkz_stack_push(ETT_CALL_SETTER);
            tkz_stack_push(ETT_VALUE);
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
        RESET_TEMP_BUFFER();
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_LEFT_PARENTHESIS);
    }
    if (character == '!') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_LEFT_PARENTHESIS);
    }

    if (tkz_buffer_equal_to(parser->temp_buffer, "(!", 2)) {
        tkz_stack_push(ETT_CALL_SETTER);
        tkz_stack_push(ETT_VALUE);
    }
    else if (tkz_buffer_equal_to(parser->temp_buffer, "(", 1)) {
        tkz_stack_push(ETT_CALL_GETTER);
        tkz_stack_push(ETT_VALUE);
    }
    RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_RIGHT_PARENTHESIS)
    if (top == NULL) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (is_finished_by_callback(parser, character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    if (character == ')') {
        if ((top->type == ETT_CALL_SETTER || top->type == ETT_CALL_GETTER)
                && pcejson_token_is_closed(top)) {
            update_tkz_stack(parser);

            struct pcejson_token *token = tkz_stack_top();
            if (token == top) {
                /* not paired match */
                SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
                RETURN_AND_STOP_PARSE();
            }
        }

        close_container(parser, character);
        top = tkz_stack_top();
        if (top->type == ETT_CALL_SETTER || top->type == ETT_CALL_GETTER) {
            ADVANCE_TO(EJSON_TKZ_STATE_RIGHT_PARENTHESIS);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '[' || character == '.') {
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    if (character == '!') {
        RECONSUME_IN(EJSON_TKA_STATE_EXCLAMATION_MARK);
    }
    if (character == '(') {
        RECONSUME_IN(EJSON_TKZ_STATE_LEFT_PARENTHESIS);
    }
    update_tkz_stack(parser);
    RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
END_STATE()


BEGIN_STATE(EJSON_TKZ_STATE_DOLLAR)
    if (is_whitespace(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '$') {
        tkz_stack_push(ETT_GET_VARIABLE);
        ADVANCE_TO(EJSON_TKZ_STATE_DOLLAR);
    }
    if (character == '{') {
        tkz_stack_push(ETT_PROTECT);
        RESET_TEMP_BUFFER();
        tkz_stack_push(ETT_VALUE);
        ADVANCE_TO(EJSON_TKZ_STATE_VARIABLE);
    }
    RESET_TEMP_BUFFER();
    tkz_stack_push(ETT_VALUE);
    RECONSUME_IN(EJSON_TKZ_STATE_VARIABLE);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_AFTER_VALUE)
    uint32_t type = top->type;
    if (is_parse_finished(parser, character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
    }
    if (is_whitespace(character)) {
        if (type == ETT_UNQUOTED_S || type == ETT_STRING) {
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (type == ETT_VALUE) {
            struct pcejson_token *prev = tkz_prev_token();
            if (!prev) {
                RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
            }
        }
        ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE);
    }
    if (character == '"' || character == '\'') {
        update_tkz_stack(parser);
        ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
    }
    if (character == '}') {
        RECONSUME_IN(EJSON_TKZ_STATE_RIGHT_BRACE);
    }
    if (character == ']') {
        RECONSUME_IN(EJSON_TKZ_STATE_RIGHT_BRACKET);
    }
    if (character == ')') {
        struct pcejson_token *prev = tkz_prev_token();
        if (is_any_op_expr(prev)) {
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_OP_EXPR);
        }
        if (top && top->type == ETT_VALUE && prev &&
            prev->type == ETT_OP_COND_ELSE) {
            RECONSUME_IN(EJSON_TKZ_STATE_OP_CONDITIONAL);
        }
        if (top && top->type == ETT_VALUE && prev &&
            prev->type == ETT_OP_COMMA) {
            RECONSUME_IN(EJSON_TKZ_STATE_OP_COMMA);
        }
        if (top && (top->type != ETT_VALUE)  && top->node->is_closed && prev &&
            (prev->type == ETT_VALUE)) {
            struct pcejson_token *token = tkz_get_token(2);
            if (is_any_op_expr(token)) {
                update_tkz_stack(parser);
                RECONSUME_IN(EJSON_TKZ_STATE_OP_SIGN);
            }
        }
        RECONSUME_IN(EJSON_TKZ_STATE_RIGHT_PARENTHESIS);
    }
    if (is_operator_sign(character)) {
        struct pcejson_token *prev = tkz_prev_token();
        if (is_any_op_expr(prev)) {
            RECONSUME_IN(EJSON_TKZ_STATE_OP_SIGN);
        }

        if (character == ':' && top && top->type == ETT_VALUE && prev &&
            prev->type == ETT_OP_COND_THEN) {
            RECONSUME_IN(EJSON_TKZ_STATE_OP_CONDITIONAL);
        }

        if (top && top->type == ETT_VALUE && prev &&
            prev->type == ETT_OP_COND_ELSE) {
            RECONSUME_IN(EJSON_TKZ_STATE_OP_CONDITIONAL);
        }
        if (top && top->type == ETT_VALUE && prev &&
            prev->type == ETT_OP_COMMA) {
            RECONSUME_IN(EJSON_TKZ_STATE_OP_COMMA);
        }
        if (top &&
            (top->type == ETT_CALL_GETTER || top->type == ETT_CALL_SETTER)
            && (character != ',')) {
            RECONSUME_IN(EJSON_TKZ_STATE_OP_EXPR_IN_FUNC);
        }
        if (top && (top->type != ETT_VALUE)  && top->node->is_closed && prev &&
            (prev->type == ETT_VALUE)) {
            struct pcejson_token *token = tkz_get_token(2);
            if (is_any_op_expr(token)) {
                update_tkz_stack(parser);
                RECONSUME_IN(EJSON_TKZ_STATE_OP_SIGN);
            }
        }
    }
    if (character == ',') {
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    if (character == '<' || character == '.') {
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    if (character == ';' || character == '|' || character == '&') {
        top = tkz_stack_top();
        if (top->type != ETT_CJSONEE) {
            update_tkz_stack(parser);
        }
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    if (character == '#') {
        SET_RETURN_STATE(EJSON_TKZ_STATE_AFTER_VALUE);
        ADVANCE_TO(EJSON_TKZ_STATE_LINE_COMMENT);
    }
    if (type == ETT_STRING || type == ETT_UNQUOTED_S) {
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_BEFORE_NAME)
    if (is_whitespace(character)) {
        ADVANCE_TO(EJSON_TKZ_STATE_BEFORE_NAME);
    }
    uint32_t type = top->type;
    if (character == '"') {
        RESET_TEMP_BUFFER();
        RESET_STRING_BUFFER();
        if (type == ETT_OBJECT) {
            tkz_stack_push(ETT_KEY);
            tkz_stack_push(ETT_DOUBLE_S);
        }
        RECONSUME_IN(EJSON_TKZ_STATE_NAME_DOUBLE_QUOTED);
    }
    if (character == '\'') {
        RESET_TEMP_BUFFER();
        if (type == ETT_OBJECT) {
            tkz_stack_push(ETT_KEY);
            tkz_stack_push(ETT_SINGLE_S);
        }
        RECONSUME_IN(EJSON_TKZ_STATE_NAME_SINGLE_QUOTED);
    }
    if (character == '}') {
        RECONSUME_IN(EJSON_TKZ_STATE_RIGHT_BRACE);
    }
    if (character == '$') {
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    if (character == '#') {
        SET_RETURN_STATE(EJSON_TKZ_STATE_BEFORE_NAME);
        ADVANCE_TO(EJSON_TKZ_STATE_LINE_COMMENT);
    }
    if (is_ascii_alpha(character) || character == '_') {
        RESET_TEMP_BUFFER();
        if (type == ETT_OBJECT) {
            tkz_stack_push(ETT_KEY);
            tkz_stack_push(ETT_UNQUOTED_S);
        }
        RECONSUME_IN(EJSON_TKZ_STATE_NAME_UNQUOTED);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_AFTER_NAME)
    if (is_whitespace(character)) {
        ADVANCE_TO(EJSON_TKZ_STATE_AFTER_NAME);
    }
    if (character == ':') {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            tkz_stack_pop();
            pcejson_token_destroy(top);

            tkz_stack_push(ETT_STRING);
            struct pcejson_token *token = tkz_stack_top();
            token->node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer));
            top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
        }

        tkz_stack_push(ETT_VALUE);
        ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
    }
    if (character == '#') {
        SET_RETURN_STATE(EJSON_TKZ_STATE_AFTER_NAME);
        ADVANCE_TO(EJSON_TKZ_STATE_LINE_COMMENT);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_NAME_UNQUOTED)
    if (is_whitespace(character) || character == ':') {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
             /* ETT_KEY: K */
            tkz_stack_drop_top();
             /* ETT_UNQUOTED_S: U */
            tkz_stack_drop_top();

            tkz_stack_push(ETT_STRING);
            top = tkz_stack_top();
            top->node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer));
            top->node->quoted_type = PCVCM_NODE_QUOTED_TYPE_NONE;
            top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;

            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_NAME);
    }
    if (is_ascii_alpha(character) || is_ascii_digit(character)
            || character == '-' || character == '_') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_NAME_UNQUOTED);
    }
    if (character == '$') {
         /* U */
        tkz_stack_drop_top();

        tkz_stack_push(ETT_MULTI_UNQUOTED_S);
        top = tkz_stack_top();

        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            node->quoted_type = PCVCM_NODE_QUOTED_TYPE_NONE;
            node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            pcvcm_node_append_child(top->node, node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_NAME_SINGLE_QUOTED)
    if (character == '\'') {
        size_t nr_buf_chars = tkz_buffer_get_size_in_chars(
                parser->temp_buffer);
        if (nr_buf_chars >= 1) {
             /* K */
            tkz_stack_drop_top();

             /* S */
            tkz_stack_drop_top();

            tkz_stack_push(ETT_STRING);
            top = tkz_stack_top();
            top->node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer));
            top->node->quoted_type = PCVCM_NODE_QUOTED_TYPE_SINGLE;
            top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();

            ADVANCE_TO(EJSON_TKZ_STATE_AFTER_NAME);
        }
        else {
            ADVANCE_TO(EJSON_TKZ_STATE_NAME_SINGLE_QUOTED);
        }
    }
    if (character == '\\') {
        SET_RETURN_STATE(EJSON_TKZ_STATE_NAME_SINGLE_QUOTED);
        ADVANCE_TO(EJSON_TKZ_STATE_STRING_ESCAPE);
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(EJSON_TKZ_STATE_NAME_SINGLE_QUOTED);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_NAME_DOUBLE_QUOTED)
    if (character == '"') {
        size_t nr_buf_chars = tkz_buffer_get_size_in_chars(
                parser->temp_buffer);
        if (nr_buf_chars > 1) {
            tkz_buffer_delete_head_chars (parser->temp_buffer, 1);
             /* K */
            tkz_stack_drop_top();

             /* D */
            tkz_stack_drop_top();

            tkz_stack_push(ETT_STRING);
            top = tkz_stack_top();
            top->node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer));
            top->node->quoted_type = PCVCM_NODE_QUOTED_TYPE_DOUBLE;
            top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            ADVANCE_TO(EJSON_TKZ_STATE_AFTER_NAME);
        }
        else if (nr_buf_chars == 1) {
             /* K */
            tkz_stack_drop_top();
             /* D */
            tkz_stack_drop_top();
            tkz_stack_push(ETT_STRING);
            top = tkz_stack_top();
            top->node = pcvcm_node_new_string("");
            top->node->position = parser->temp_ucs->nr_ucs - nr_buf_chars -1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();

            ADVANCE_TO(EJSON_TKZ_STATE_AFTER_NAME);
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_NAME_DOUBLE_QUOTED);
    }
    if (character == '\\') {
        SET_RETURN_STATE(curr_state);
        ADVANCE_TO(EJSON_TKZ_STATE_STRING_ESCAPE);
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '$') {
         /* D */
        tkz_stack_drop_top();

        tkz_stack_push(ETT_MULTI_QUOTED_S);
        top = tkz_stack_top();

        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            node->quoted_type = PCVCM_NODE_QUOTED_TYPE_DOUBLE;
            node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            pcvcm_node_append_child(top->node, node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(EJSON_TKZ_STATE_NAME_DOUBLE_QUOTED);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_SINGLE_QUOTED)
    if (character == '\'') {
        parser->nr_single_quoted++;
        size_t nr_buf_chars = tkz_buffer_get_size_in_chars(
                parser->temp_buffer);
        if (nr_buf_chars >= 1) {
             /* V */
            tkz_stack_drop_top();
             /* S */
            tkz_stack_drop_top();
            tkz_stack_push(ETT_STRING);
            top = tkz_stack_top();
            top->node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            top->node->quoted_type = PCVCM_NODE_QUOTED_TYPE_SINGLE;
            top->node->position = parser->temp_ucs->nr_ucs - nr_buf_chars -1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RESET_SINGLE_QUOTED_COUNTER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        else if (parser->nr_single_quoted == 3) {
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_TRIPLE_SINGLE_QUOTED);
        }
        else {
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_SINGLE_QUOTED);
        }
    }
    if (parser->nr_single_quoted == 2) {
        /* V */
        tkz_stack_drop_top();
        /* S */
        tkz_stack_drop_top();
        tkz_stack_push(ETT_STRING);
        top = tkz_stack_top();
        top->node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer)
                );
        top->node->quoted_type = PCVCM_NODE_QUOTED_TYPE_SINGLE;
        top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
        update_tkz_stack(parser);
        RESET_TEMP_BUFFER();
        RESET_SINGLE_QUOTED_COUNTER();
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
    }
    if (character == '\\') {
        SET_RETURN_STATE(curr_state);
        ADVANCE_TO(EJSON_TKZ_STATE_STRING_ESCAPE);
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    if (is_c0(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_UNESCAPED_CONTROL_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(EJSON_TKZ_STATE_VALUE_SINGLE_QUOTED);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_TRIPLE_SINGLE_QUOTED)
    if (character == '\'') {
        parser->nr_single_quoted++;
        if (parser->nr_single_quoted > 3) {
            APPEND_TO_TEMP_BUFFER(character);
        }
        if (parser->nr_single_quoted >= 6
                && tkz_buffer_end_with(parser->temp_buffer,
                    "\'\'\'", 3)) {
            tkz_buffer_delete_tail_chars(parser->temp_buffer, 3);
            /* V */
            tkz_stack_drop_top();
            /* D */
            tkz_stack_drop_top();
            tkz_stack_push(ETT_STRING);
            top = tkz_stack_top();
            top->node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            top->node->quoted_type = PCVCM_NODE_QUOTED_TYPE_SINGLE;
            top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RESET_SINGLE_QUOTED_COUNTER();
            ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_TRIPLE_SINGLE_QUOTED);
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(EJSON_TKZ_STATE_VALUE_TRIPLE_SINGLE_QUOTED);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED)
    if (character == '"') {
        if (parser->nr_double_quoted == 0) {
            parser->nr_double_quoted++;
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
        }
        else if (parser->nr_double_quoted == 1) {
            RECONSUME_IN(EJSON_TKZ_STATE_VALUE_DOUBLE_DOUBLE_QUOTED);
        }
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_DOUBLE_QUOTED);
    }
    if (character == '\\') {
        SET_RETURN_STATE(curr_state);
        ADVANCE_TO(EJSON_TKZ_STATE_STRING_ESCAPE);
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '{') {
        uint32_t last_c = tkz_buffer_get_last_char(parser->temp_buffer);
        if (last_c != '{') {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
        }

        /* {{ as CHEE */
        tkz_buffer_delete_tail_chars(parser->temp_buffer, 1);

         /* ETT_VALUE */
        tkz_stack_drop_top();
        top = tkz_stack_top();
        if (top->type == ETT_DOUBLE_S) {
            tkz_stack_drop_top();
            tkz_stack_push(ETT_MULTI_QUOTED_S);
            top = tkz_stack_top();
        }
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            node->quoted_type = PCVCM_NODE_QUOTED_TYPE_DOUBLE;
            node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            pcvcm_node_append_child(top->node, node);
            RESET_TEMP_BUFFER();
        }

        RECONSUME_LAST_CHAR();
        RECONSUME_LAST_CHAR();
        ADVANCE_TO(EJSON_TKZ_STATE_DATA);
    }
    if (character == '$') {
        if ((parser->flags & PCEJSON_FLAG_GET_VARIABLE) == 0) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
        }
         /* ETT_VALUE */
        tkz_stack_drop_top();
        top = tkz_stack_top();
        if (top->type == ETT_DOUBLE_S) {
            tkz_stack_drop_top();
            tkz_stack_push(ETT_MULTI_QUOTED_S);
            top = tkz_stack_top();
        }
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            if (tkz_buffer_end_with(parser->temp_buffer, "{", 1)) {
                RECONSUME_LAST_CHAR();
                RECONSUME_LAST_CHAR();
                DELETE_FROM_RAW_BUFFER(2);
                tkz_buffer_delete_tail_chars(parser->temp_buffer, 1);
                if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                    struct pcvcm_node *node = pcvcm_node_new_string(
                            tkz_buffer_get_bytes(parser->temp_buffer)
                            );
                    node->quoted_type = PCVCM_NODE_QUOTED_TYPE_DOUBLE;
                    node->position = parser->temp_ucs->nr_ucs -
                        tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
                    pcvcm_node_append_child(top->node, node);
                    RESET_TEMP_BUFFER();
                }
            }
            else if (tkz_buffer_end_with(parser->temp_buffer, "{{", 2)) {
                RECONSUME_LAST_CHAR();
                RECONSUME_LAST_CHAR();
                RECONSUME_LAST_CHAR();
                DELETE_FROM_RAW_BUFFER(3);
                tkz_buffer_delete_tail_chars(parser->temp_buffer, 2);
                if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                    struct pcvcm_node *node = pcvcm_node_new_string(
                            tkz_buffer_get_bytes(parser->temp_buffer)
                            );
                    node->quoted_type = PCVCM_NODE_QUOTED_TYPE_DOUBLE;
                    node->position = parser->temp_ucs->nr_ucs -
                        tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
                    pcvcm_node_append_child(top->node, node);
                    RESET_TEMP_BUFFER();
                }
            }
            else {
                RECONSUME_LAST_CHAR();
                DELETE_FROM_RAW_BUFFER(1);
                struct pcvcm_node *node = pcvcm_node_new_string(
                        tkz_buffer_get_bytes(parser->temp_buffer)
                        );
                node->quoted_type = PCVCM_NODE_QUOTED_TYPE_DOUBLE;
                node->position = parser->temp_ucs->nr_ucs -
                    tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
                pcvcm_node_append_child(top->node, node);
                RESET_TEMP_BUFFER();
            }
            RESET_TEMP_BUFFER();
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    if (is_c0(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_UNESCAPED_CONTROL_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_AFTER_VALUE_DOUBLE_QUOTED)
    if (character == '\"') {
         /* V */
        tkz_stack_drop_top();
         /* D */
        tkz_stack_drop_top();
        tkz_stack_push(ETT_STRING);
        top = tkz_stack_top();
        top->node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer)
                );
        top->node->quoted_type = PCVCM_NODE_QUOTED_TYPE_DOUBLE;
        top->node->position = parser->temp_ucs->nr_ucs -
            tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
        update_tkz_stack(parser);
        RESET_TEMP_BUFFER();
        RESET_DOUBLE_QUOTED_COUNTER();
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_DOUBLE_DOUBLE_QUOTED)
    if (character == '"') {
        if (parser->nr_double_quoted == 1) {
            parser->nr_double_quoted++;
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_DOUBLE_DOUBLE_QUOTED);
        }
        else if (parser->nr_double_quoted == 2) {
            /* V */
            tkz_stack_drop_top();
            /* D */
            tkz_stack_drop_top();

            tkz_stack_push(ETT_TRIPLE_DOUBLE_QUOTED);
            tkz_stack_push(ETT_VALUE);
            RESET_STRING_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_VALUE_TRIPLE_DOUBLE_QUOTED);
        }
    }
    if (top->type == ETT_VALUE) {
        tkz_stack_drop_top();
        top = tkz_stack_top();
    }
    if (top->type == ETT_DOUBLE_S) {
        top->node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer)
                );
        top->node->position = parser->temp_ucs->nr_ucs -
            tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
    }
    else if (top->type == ETT_MULTI_QUOTED_S) {
        struct pcvcm_node *node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer)
                );
        node->position = parser->temp_ucs->nr_ucs -
            tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
        pcvcm_node_append_child(top->node, node);
        close_token(parser, top);
    }
    RESET_TEMP_BUFFER();
    RESET_DOUBLE_QUOTED_COUNTER();
    if (is_parse_finished(parser, character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
    }
    RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_TRIPLE_DOUBLE_QUOTED)
    if (character == '\"') {
        parser->nr_double_quoted++;
        if (parser->nr_double_quoted > 3) {
            APPEND_TO_TEMP_BUFFER(character);
        }
        if (parser->nr_double_quoted >= 6
                && tkz_buffer_end_with(parser->temp_buffer,
                    "\"\"\"", 3)) {
            /* V */
            tkz_stack_drop_top();

            tkz_buffer_delete_tail_chars(parser->temp_buffer, 3);
            tkz_stack_push(ETT_STRING);
            top = tkz_stack_top();
            top->node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            top->node->quoted_type = PCVCM_NODE_QUOTED_TYPE_DOUBLE;
            top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);

            // concat string
            top = tkz_stack_top();
            close_token(parser, top);
            update_tkz_stack(parser);

            RESET_STRING_BUFFER();
            RESET_TEMP_BUFFER();
            RESET_DOUBLE_QUOTED_COUNTER();
            ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_TRIPLE_DOUBLE_QUOTED);
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '$' && (parser->flags & PCEJSON_FLAG_GET_VARIABLE)) {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            if (tkz_buffer_end_with(parser->temp_buffer, "{", 1)) {
                RECONSUME_LAST_CHAR();
                RECONSUME_LAST_CHAR();
                DELETE_FROM_RAW_BUFFER(2);
                tkz_buffer_delete_tail_chars(parser->temp_buffer, 1);
            }
            else if (tkz_buffer_end_with(parser->temp_buffer, "{{", 2)) {
                RECONSUME_LAST_CHAR();
                RECONSUME_LAST_CHAR();
                RECONSUME_LAST_CHAR();
                DELETE_FROM_RAW_BUFFER(3);
                tkz_buffer_delete_tail_chars(parser->temp_buffer, 2);
            }
            else if (!tkz_buffer_is_empty(parser->string_buffer)) {
                size_t sz = 1 + tkz_buffer_get_size_in_chars(parser->string_buffer);
                for (size_t i = 0; i < sz; i++) {
                    RECONSUME_LAST_CHAR();
                }
                DELETE_FROM_RAW_BUFFER(sz);
                tkz_buffer_delete_tail_chars(parser->temp_buffer, sz - 1);
            }
            else {
                RECONSUME_LAST_CHAR();
                DELETE_FROM_RAW_BUFFER(1);
            }
            if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                tkz_stack_push(ETT_STRING);
                top = tkz_stack_top();
                top->node = pcvcm_node_new_string(
                        tkz_buffer_get_bytes(parser->temp_buffer)
                        );
                top->node->quoted_type = PCVCM_NODE_QUOTED_TYPE_DOUBLE;
                top->node->position = parser->temp_ucs->nr_ucs -
                    tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
                update_tkz_stack(parser);
                RESET_TEMP_BUFFER();
            }
        }
        tkz_stack_push(ETT_VALUE);
        RESET_STRING_BUFFER();
        ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
    }
    if (character == '{') {
        APPEND_TO_STRING_BUFFER(character);
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_TRIPLE_DOUBLE_QUOTED);
    }
    if (is_whitespace(character)
            && !tkz_buffer_is_empty(parser->string_buffer)) {
        APPEND_TO_STRING_BUFFER(character);
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_TRIPLE_DOUBLE_QUOTED);
    }
    RESET_STRING_BUFFER();
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(EJSON_TKZ_STATE_VALUE_TRIPLE_DOUBLE_QUOTED);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_KEYWORD)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ','
            || character == ')' || is_parse_finished(parser,character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_KEYWORD);
    }
    if (character == '$' && (parser->flags & PCEJSON_FLAG_GET_VARIABLE)) {
        /* unquoted */
        tkz_stack_push(ETT_MULTI_UNQUOTED_S);
        top = tkz_stack_top();

        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            pcvcm_node_append_child(top->node, node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    if (parser->sbst == NULL) {
        parser->sbst = tkz_sbst_new_ejson_keywords();
    }
    APPEND_TO_TEMP_BUFFER(character);
    bool ret = tkz_sbst_advance_ex(parser->sbst, character, true);
    if (!ret) {
        tkz_sbst_destroy(parser->sbst);
        parser->sbst = NULL;

        tkz_stack_drop_top();
        top = tkz_stack_top();
        if (top == NULL) {
            tkz_stack_push(ETT_UNQUOTED_S);
            tkz_stack_push(ETT_VALUE);
            ADVANCE_TO(EJSON_TKZ_STATE_RAW_STRING);
        }

        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_KEYWORD);
        RETURN_AND_STOP_PARSE();
    }

    const char* value = tkz_sbst_get_match(parser->sbst);
    if (value == NULL) {
        ADVANCE_TO(EJSON_TKZ_STATE_KEYWORD);
    }
    else {
        RESET_TEMP_BUFFER();
        APPEND_BYTES_TO_TEMP_BUFFER(value, strlen(value));
        tkz_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        ADVANCE_TO(EJSON_TKZ_STATE_AFTER_KEYWORD);
    }
    if (is_parse_finished(parser,character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_KEYWORD);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_AFTER_KEYWORD)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ','
            || character == ')' || character == ';' || character == '&'
            || character == '|' || is_eof(character)) {
        if (tkz_buffer_equal_to(parser->temp_buffer, "true", 4)) {
            top->node = pcvcm_node_new_boolean(true);
            top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        if (tkz_buffer_equal_to(parser->temp_buffer, "false",
                    5)) {
           top->node = pcvcm_node_new_boolean(false);
            top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        if (tkz_buffer_equal_to(parser->temp_buffer, "null", 4)) {
            top->node = pcvcm_node_new_null();
            top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        if (tkz_buffer_equal_to(parser->temp_buffer, "undefined", 9)) {
            top->node = pcvcm_node_new_undefined();
            top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        struct pcejson_token *prev = tkz_prev_token();
        if (prev == NULL) {
            tkz_stack_push(ETT_UNQUOTED_S);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
        }
        RESET_TEMP_BUFFER();
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    struct pcejson_token *prev = tkz_prev_token();
    if (prev == NULL) {
        tkz_stack_push(ETT_UNQUOTED_S);
        tkz_stack_push(ETT_VALUE);
        RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
    }
    RESET_TEMP_BUFFER();
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_BYTE_SEQUENCE)
    if (character == 'b') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_BYTE_SEQUENCE);
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_BINARY_BYTE_SEQUENCE);
    }
    if (character == 'x') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_HEX_BYTE_SEQUENCE);
    }
    if (character == '6') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_BASE64_BYTE_SEQUENCE);
    }
    if (character == '$' && (parser->flags & PCEJSON_FLAG_GET_VARIABLE)) {
        /* unquoted */
        tkz_stack_push(ETT_MULTI_UNQUOTED_S);
        top = tkz_stack_top();
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            pcvcm_node_append_child(top->node, node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    if (top == NULL || top->type == ETT_VALUE) {
        tkz_stack_push(ETT_UNQUOTED_S);
        tkz_stack_push(ETT_VALUE);
        RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_AFTER_BYTE_SEQUENCE)
    if (is_parse_finished(parser, character) || is_whitespace(character)
            || character == '}' || character == ']' || character == ','
            || character == ')') {
        struct pcvcm_node *node = create_byte_sequenct(parser->temp_buffer);
        if (node == NULL) {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
        top->node = node;
        update_tkz_stack(parser);
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_HEX_BYTE_SEQUENCE)
    if (is_parse_finished(parser, character) || is_whitespace(character)
            || character == '}' || character == ']' || character == ','
            || character == ')') {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_BYTE_SEQUENCE);
    }
    else if (is_ascii_digit(character)
            || is_ascii_hex_digit(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_HEX_BYTE_SEQUENCE);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_BINARY_BYTE_SEQUENCE)
    if (is_parse_finished(parser, character) || is_whitespace(character)
            || character == '}' || character == ']' || character == ','
            || character == ')') {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_BYTE_SEQUENCE);
    }
    else if (is_ascii_binary_digit(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_BINARY_BYTE_SEQUENCE);
    }
    if (character == '.') {
        ADVANCE_TO(EJSON_TKZ_STATE_BINARY_BYTE_SEQUENCE);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_BASE64_BYTE_SEQUENCE)
    if (is_parse_finished(parser, character) || is_whitespace(character)
            || character == '}' || character == ']' || character == ','
            || character == ')') {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_BYTE_SEQUENCE);
    }
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_BASE64_BYTE_SEQUENCE);
    }
    if (is_ascii_digit(character) || is_ascii_alpha(character)
            || character == '+' || character == '-' || character == '/') {
        if (!tkz_buffer_end_with(parser->temp_buffer, "=", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_BASE64_BYTE_SEQUENCE);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_BASE64);
        RETURN_AND_STOP_PARSE();
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_NUMBER)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }
    if (is_ascii_digit(character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_VALUE_NUMBER_INTEGER);
    }
    if (character == '-') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_INTEGER);
    }
    if (character == '$' && (parser->flags & PCEJSON_FLAG_GET_VARIABLE)) {
        /* unquoted */
        tkz_stack_push(ETT_MULTI_UNQUOTED_S);
        top = tkz_stack_top();
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            pcvcm_node_append_child(top->node, node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    SET_ERR(PCEJSON_ERROR_BAD_JSON_NUMBER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER)
    struct pcejson_token *prev = tkz_prev_token();
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || is_parse_finished(parser, character) ||
            (is_any_op_expr(prev) && is_operator_sign(character))) {

        if (tkz_buffer_end_with(parser->temp_buffer, "-", 1)
            || tkz_buffer_end_with(parser->temp_buffer, "E", 1)
            || tkz_buffer_end_with(parser->temp_buffer, "e", 1)) {
            SET_ERR(PCEJSON_ERROR_BAD_JSON_NUMBER);
            RETURN_AND_STOP_PARSE();
        }
        if (tkz_buffer_end_with_ci(parser->temp_buffer, "U", 1)) {
            const char *buf = tkz_buffer_get_bytes(parser->temp_buffer);
            size_t nr_buf =
                tkz_buffer_get_size_in_bytes(parser->temp_buffer) - 1;

            bool is_decimal = (buf[0] != '0') || (nr_buf == 1);
            if (!is_decimal && nr_buf > 1) {
                for (size_t i = 1; i < nr_buf; i++) {
                    if (!is_ascii_octal_digit(buf[i])) {
                        is_decimal = true;
                        break;
                    }
                }
            }

            int base = is_decimal ? 10 : 8;
            uint64_t u64 =
                strtoull(tkz_buffer_get_bytes(parser->temp_buffer), NULL, base);
            top->node = pcvcm_node_new_ulongint(u64);
            top->node->position =
                parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        } else {
            const char *buf = tkz_buffer_get_bytes(parser->temp_buffer);
            size_t nr_buf = tkz_buffer_get_size_in_bytes(parser->temp_buffer);
            bool is_not_octal = (buf[0] != '0') || (nr_buf == 1);
            if (!is_not_octal) {
                for (size_t i = 1; i < nr_buf; i++) {
                    if (!is_ascii_octal_digit(buf[i])) {
                        is_not_octal = true;
                        break;
                    }
                }
            }

            if (is_not_octal) {
                double d = strtod(tkz_buffer_get_bytes(parser->temp_buffer), NULL);
                top->node = pcvcm_node_new_number(d);
                top->node->position =
                    parser->temp_ucs->nr_ucs -
                    tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
                update_tkz_stack(parser);
                RESET_TEMP_BUFFER();
                RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
            }
            else {
                uint64_t u64 = strtoull(
                    tkz_buffer_get_bytes(parser->temp_buffer), NULL, 8);
                top->node = pcvcm_node_new_longint(u64);
                top->node->position =
                    parser->temp_ucs->nr_ucs -
                    tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
                update_tkz_stack(parser);
                RESET_TEMP_BUFFER();
                RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
            }
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_NUMBER_INTEGER)
    if (is_whitespace(character) || is_eof(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }
    if (is_ascii_digit(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_INTEGER);
    }
    if (is_alpha_equal_ci(character, 'x')) {
        if(tkz_buffer_equal_to(parser->temp_buffer, "0", 1)) {
            RESET_TEMP_BUFFER();
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_HEX);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
        RETURN_AND_STOP_PARSE();
    }
    if (is_alpha_equal_ci(character,  'E')) {
        APPEND_TO_TEMP_BUFFER('e');
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_EXPONENT);
    }
    if (character == '.' || is_alpha_equal_ci(character, 'F')) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_FRACTION);
    }
    if (is_alpha_equal_ci(character, 'U') ||
        is_alpha_equal_ci(character, 'L') ||
        is_alpha_equal_ci(character, 'N')) {
        RECONSUME_IN(EJSON_TKZ_STATE_VALUE_NUMBER_SUFFIX_INTEGER);
    }
    if (character == 'I' && (
                tkz_buffer_is_empty(parser->temp_buffer) ||
                tkz_buffer_equal_to(parser->temp_buffer, "-", 1)
                )) {
        RECONSUME_IN(EJSON_TKZ_STATE_VALUE_NUMBER_INFINITY);
    }
    if (is_eof(character)) {
        ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }
    if (is_finished_by_callback(parser, character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }

    struct pcejson_token *prev = tkz_prev_token();
    if (prev == NULL) {
        tkz_stack_push(ETT_UNQUOTED_S);
        tkz_stack_push(ETT_VALUE);
        RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
    }

    if (tkz_buffer_equal_to(parser->temp_buffer, "-", 1)) {
        if (!is_any_op_expr(prev)) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            tkz_stack_push(ETT_OP_EXPR_IN_FUNC);
            struct pcejson_token *top = tkz_stack_top();

            struct pcvcm_node *sign = pcvcm_node_new_op_unary_minus();

            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    if (is_any_op_expr(prev) && is_operator_sign(character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }

    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_NUMBER_FRACTION)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || is_eof(character) || is_finished_by_callback(parser, character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }

    if (is_ascii_digit(character)) {
        if (tkz_buffer_end_with_ci(parser->temp_buffer, "F", 1)) {
            SET_ERR(PCEJSON_ERROR_BAD_JSON_NUMBER);
            RETURN_AND_STOP_PARSE();
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_FRACTION);
    }
    if (is_alpha_equal_ci(character, 'F')) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_FRACTION);
    }
    if (is_alpha_equal_ci(character, 'L')) {
        if (tkz_buffer_end_with_ci(parser->temp_buffer, "F", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            long double ld = strtold (
                    tkz_buffer_get_bytes(parser->temp_buffer), NULL);
            top->node = pcvcm_node_new_longdouble(ld);
            top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE);
        }
    }
    if (is_alpha_equal_ci(character, 'E')) {
        if (tkz_buffer_end_with(parser->temp_buffer, ".", 1)) {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION);
            RETURN_AND_STOP_PARSE();
        }
        APPEND_TO_TEMP_BUFFER('e');
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_EXPONENT);
    }
    if (is_eof(character)) {
        ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }
    if (is_finished_by_callback(parser, character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }

    struct pcejson_token *prev = tkz_prev_token();
    if (prev == NULL) {
        tkz_stack_push(ETT_UNQUOTED_S);
        tkz_stack_push(ETT_VALUE);
        RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_NUMBER_EXPONENT)
    if (is_whitespace(character) || is_eof(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }
    if (is_ascii_digit(character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_VALUE_NUMBER_EXPONENT_INTEGER);
    }
    if (character == '+' || character == '-') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_EXPONENT_INTEGER);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_NUMBER_EXPONENT_INTEGER)
    if (is_whitespace(character) || is_eof(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }
    if (is_ascii_digit(character)) {
        if (tkz_buffer_end_with_ci(parser->temp_buffer, "F", 1)) {
            SET_ERR(PCEJSON_ERROR_BAD_JSON_NUMBER);
            RETURN_AND_STOP_PARSE();
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_EXPONENT_INTEGER);
    }
    if (is_alpha_equal_ci(character, 'F')) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_EXPONENT_INTEGER);
    }
    if (is_alpha_equal_ci(character,  'L')) {
        if (tkz_buffer_end_with_ci(parser->temp_buffer, "F", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            long double ld = strtold (
                    tkz_buffer_get_bytes(parser->temp_buffer), NULL);
            top->node = pcvcm_node_new_longdouble(ld);
            top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
        }
    }
    if (is_eof(character)) {
        ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }
    if (is_finished_by_callback(parser, character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }

    struct pcejson_token *prev = tkz_prev_token();
    if (prev == NULL) {
        tkz_stack_push(ETT_UNQUOTED_S);
        tkz_stack_push(ETT_VALUE);
        RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_NUMBER_SUFFIX_INTEGER)
    uint32_t last_c = tkz_buffer_get_last_char(
            parser->temp_buffer);
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }
    if (is_alpha_equal_ci(character, 'U')) {
        if (is_ascii_digit(last_c)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_SUFFIX_INTEGER);
        }
    }
    if (is_alpha_equal_ci(character, 'L')) {
        if (is_ascii_digit(last_c) || is_alpha_equal_ci(last_c, 'U')) {
            APPEND_TO_TEMP_BUFFER(character);
            if (tkz_buffer_end_with_ci(parser->temp_buffer, "UL", 2)) {
                const char *buf = tkz_buffer_get_bytes(parser->temp_buffer);
                size_t nr_buf = tkz_buffer_get_size_in_bytes(parser->temp_buffer) - 2;

                bool is_decimal = (buf[0] != '0') || (nr_buf == 1);
                if (!is_decimal && nr_buf > 1) {
                    for (size_t i = 1; i < nr_buf; i++) {
                        if (!is_ascii_octal_digit(buf[i])) {
                            is_decimal = true;
                            break;
                        }
                    }
                }

                int base = is_decimal ? 10 : 8;
                uint64_t u64 = strtoull (
                    tkz_buffer_get_bytes(parser->temp_buffer),
                    NULL, base);
                top->node = pcvcm_node_new_ulongint(u64);
                top->node->position = parser->temp_ucs->nr_ucs -
                    tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
                update_tkz_stack(parser);
                RESET_TEMP_BUFFER();
                ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE);
            } else if (tkz_buffer_end_with_ci(parser->temp_buffer, "L", 1)) {
                const char *buf = tkz_buffer_get_bytes(parser->temp_buffer);
                size_t nr_buf = tkz_buffer_get_size_in_bytes(parser->temp_buffer) - 1;

                bool is_decimal = (buf[0] != '0') || (nr_buf == 1);
                if (!is_decimal && nr_buf > 1) {
                    for (size_t i = 1; i < nr_buf; i++) {
                        if (!is_ascii_octal_digit(buf[i])) {
                            is_decimal = true;
                            break;
                        }
                    }
                }

                int base = is_decimal ? 10 : 8;
                int64_t i64 = strtoll (
                    tkz_buffer_get_bytes(parser->temp_buffer),
                    NULL, base);
                top->node = pcvcm_node_new_longint(i64);
                top->node->position = parser->temp_ucs->nr_ucs -
                    tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
                update_tkz_stack(parser);
                RESET_TEMP_BUFFER();
                ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE);
            }
        }
    }
    if (is_alpha_equal_ci(character, 'N')) {
        if (is_ascii_digit(last_c)) {
            const char *buf = tkz_buffer_get_bytes(parser->temp_buffer);
            size_t nr_buf = tkz_buffer_get_size_in_bytes(parser->temp_buffer);

            bool is_decimal = (buf[0] != '0') || (nr_buf == 1);
            if (!is_decimal && nr_buf > 1) {
                for (size_t i = 1; i < nr_buf; i++) {
                    if (!is_ascii_octal_digit(buf[i])) {
                        is_decimal = true;
                        break;
                    }
                }
            }

            int base = is_decimal ? 10 : 8;
            top->node = pcvcm_node_new_bigint(
                tkz_buffer_get_bytes(parser->temp_buffer), base);
            top->node->position =
                parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE);
        }
    }
    if (is_eof(character)) {
        ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }
    if (is_finished_by_callback(parser, character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }

    struct pcejson_token *prev = tkz_prev_token();
    if (prev == NULL) {
        tkz_stack_push(ETT_UNQUOTED_S);
        tkz_stack_push(ETT_VALUE);
        RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_NUMBER_HEX)
    if (is_whitespace(character) || is_eof(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER_HEX);
    }
    if (is_ascii_hex_digit(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_HEX);
    }
    if (is_alpha_equal_ci(character, 'U') ||
        is_alpha_equal_ci(character, 'L') ||
        is_alpha_equal_ci(character, 'N')) {
        RECONSUME_IN(EJSON_TKZ_STATE_VALUE_NUMBER_HEX_SUFFIX);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_NUMBER_HEX_SUFFIX)
    if (is_whitespace(character) || is_eof(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER_HEX);
    }
    uint32_t last_c = tkz_buffer_get_last_char(parser->temp_buffer);
    if (is_alpha_equal_ci(character, 'U')) {
        if (is_ascii_hex_digit(last_c)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_HEX_SUFFIX);
        }
    }
    if (is_alpha_equal_ci(character, 'L')) {
        if (is_ascii_hex_digit(last_c) || is_alpha_equal_ci(last_c, 'U')) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_HEX_SUFFIX);
        }
    }
    if (is_alpha_equal_ci(character, 'N')) {
        if (is_ascii_hex_digit(last_c)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_HEX_SUFFIX);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER_HEX)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || is_eof(character)) {
        const char *bytes = tkz_buffer_get_bytes(parser->temp_buffer);
        if (tkz_buffer_end_with_ci(parser->temp_buffer, "U", 1) ||
            tkz_buffer_end_with_ci(parser->temp_buffer, "UL", 2)) {
            uint64_t u64 = strtoull (bytes, NULL, 16);
            top->node = pcvcm_node_new_ulongint(u64);
            top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        } else if (tkz_buffer_end_with_ci(parser->temp_buffer, "N", 1)) {
            tkz_buffer_delete_tail_chars(parser->temp_buffer, 1);
            top->node = pcvcm_node_new_bigint(
                tkz_buffer_get_bytes(parser->temp_buffer), 16);
            top->node->position =
                parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        } else {
            int64_t i64 = strtoll (bytes, NULL, 16);
            top->node = pcvcm_node_new_longint(i64);
            top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_NUMBER_INFINITY)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        if (tkz_buffer_equal_to(parser->temp_buffer,
                    "-Infinity", 9)) {
            double d = -INFINITY;
            top->node = pcvcm_node_new_number(d);
            top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        if (tkz_buffer_equal_to(parser->temp_buffer,
                "Infinity", 8)) {
            double d = INFINITY;
            top->node = pcvcm_node_new_number(d);
            top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        if (is_whitespace(character) &&
                (top == NULL || top->type == ETT_VALUE)) {
            tkz_stack_push(ETT_UNQUOTED_S);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'I') {
        if (tkz_buffer_is_empty(parser->temp_buffer)
            || tkz_buffer_equal_to(parser->temp_buffer, "-", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_INFINITY);
        }

        if (top == NULL || top->type == ETT_VALUE) {
            tkz_stack_push(ETT_UNQUOTED_S);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
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
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_INFINITY);
        }

        if (top == NULL || top->type == ETT_VALUE) {
            tkz_stack_push(ETT_UNQUOTED_S);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (character == 'f') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "In", 2)
            || tkz_buffer_equal_to (parser->temp_buffer, "-In", 3)
                ) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_INFINITY);
        }

        if (top == NULL || top->type == ETT_VALUE) {
            tkz_stack_push(ETT_UNQUOTED_S);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
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
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_INFINITY);
        }

        if (top == NULL || top->type == ETT_VALUE) {
            tkz_stack_push(ETT_UNQUOTED_S);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
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
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_INFINITY);
        }

        if (top == NULL || top->type == ETT_VALUE) {
            tkz_stack_push(ETT_UNQUOTED_S);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
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
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_INFINITY);
        }

        if (top == NULL || top->type == ETT_VALUE) {
            tkz_stack_push(ETT_UNQUOTED_S);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (is_eof(character)) {
        ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }
    if (is_finished_by_callback(parser, character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }

    struct pcejson_token *prev = tkz_prev_token();
    if (prev == NULL) {
        tkz_stack_push(ETT_UNQUOTED_S);
        tkz_stack_push(ETT_VALUE);
        RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_NAN)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "NaN", 3)) {
            double d = NAN;
            top->node = pcvcm_node_new_number(d);
            top->node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        if (is_whitespace(character) &&
                (top == NULL || top->type == ETT_VALUE)) {
            tkz_stack_push(ETT_UNQUOTED_S);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'N') {
        if (tkz_buffer_is_empty(parser->temp_buffer)
          || tkz_buffer_equal_to(parser->temp_buffer, "Na", 2)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NAN);
        }
        if (top == NULL || top->type == ETT_VALUE) {
            tkz_stack_push(ETT_UNQUOTED_S);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (character == 'a') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "N", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NAN);
        }
        if (top == NULL || top->type == ETT_VALUE) {
            tkz_stack_push(ETT_UNQUOTED_S);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }
    if (top == NULL || top->type == ETT_VALUE) {
        tkz_stack_push(ETT_UNQUOTED_S);
        tkz_stack_push(ETT_VALUE);
        RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
    }

    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_STRING_ESCAPE)
    switch (character)
    {
        case 'b':
            APPEND_TO_TEMP_BUFFER('\b');
            ADVANCE_TO(parser->return_state);
            break;
        case 'v':
            APPEND_TO_TEMP_BUFFER('\v');
            ADVANCE_TO(parser->return_state);
        case 'f':
            APPEND_TO_TEMP_BUFFER('\f');
            ADVANCE_TO(parser->return_state);
            break;
        case 'n':
            APPEND_TO_TEMP_BUFFER('\n');
            ADVANCE_TO(parser->return_state);
            break;
        case 'r':
            APPEND_TO_TEMP_BUFFER('\r');
            ADVANCE_TO(parser->return_state);
            break;
        case 't':
            APPEND_TO_TEMP_BUFFER('\t');
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
        case '\'':
        case '.':
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(parser->return_state);
            break;
        case 'u':
            RESET_STRING_BUFFER();
            ADVANCE_TO(
              EJSON_TKZ_STATE_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS);
            break;
        default:
            SET_ERR(PCEJSON_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY);
            RETURN_AND_STOP_PARSE();
    }
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS)
    if (is_ascii_hex_digit(character)) {
        APPEND_TO_STRING_BUFFER(character);
        size_t nr_chars = tkz_buffer_get_size_in_chars(
                parser->string_buffer);
        if (nr_chars == 4) {
            uint64_t uc = 0;
            const char *bytes = tkz_buffer_get_bytes(parser->string_buffer);
            for (size_t i = 0; i < nr_chars; i++) {
                if (is_ascii_digit(bytes[i])) {
                    uc *= 16;
                    uc += bytes[i] - 0x30;
                }
                else if (is_ascii_upper_hex_digit(bytes[i])) {
                    uc *= 16;
                    uc += bytes[i] - 0x37;
                }
                else if (is_ascii_lower_hex_digit(bytes[i])) {
                    uc *= 16;
                    uc += bytes[i] - 0x57;
                }
            }

            RESET_STRING_BUFFER();
            if ((uc & 0xFFFFF800) == 0xD800) {
                SET_ERR(PCEJSON_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY);
                RETURN_AND_STOP_PARSE();
            }

            APPEND_TO_TEMP_BUFFER(uc);
            ADVANCE_TO(parser->return_state);
        }
        ADVANCE_TO(
            EJSON_TKZ_STATE_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS);
    }
    SET_ERR(PCEJSON_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_AMPERSAND)
    if (character == '&') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_AMPERSAND);
    }
    if (tkz_buffer_equal_to(parser->temp_buffer, "&&", 2)) {
        while (top && top->type != ETT_CJSONEE) {
            if (top->type == ETT_VALUE && top->node == NULL) {
                tkz_stack_drop_top();
            }
            else {
                update_tkz_stack(parser);
            }
            top = tkz_stack_top();
        }

        if (top && top->type == ETT_CJSONEE) {
            tkz_stack_push(ETT_AND);
            update_tkz_stack(parser);
            tkz_stack_push(ETT_VALUE);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    RECONSUME_LAST_CHAR();
    RECONSUME_LAST_CHAR();
    DELETE_FROM_RAW_BUFFER(2);
    tkz_stack_push(ETT_UNQUOTED_S);
    tkz_stack_push(ETT_VALUE);
    SET_RETURN_STATE(EJSON_TKZ_STATE_RAW_STRING);
    ADVANCE_TO(EJSON_TKZ_STATE_CHARACTER_REFERENCE);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OR_SIGN)
    if (character == '|') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OR_SIGN);
    }
    if (tkz_buffer_equal_to(parser->temp_buffer, "||", 2)) {
        while (top && top->type != ETT_CJSONEE) {
            if (top->type == ETT_VALUE && top->node == NULL) {
                tkz_stack_drop_top();
            }
            else {
                update_tkz_stack(parser);
            }
            top = tkz_stack_top();
        }

        if (top && top->type == ETT_CJSONEE) {
            tkz_stack_push(ETT_OR);
            update_tkz_stack(parser);
            tkz_stack_push(ETT_VALUE);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_SEMICOLON)
    if (character == ';') {
        top = tkz_stack_top();
        if (top == NULL) {
            tkz_stack_push(ETT_UNQUOTED_S);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
        }

        while (top->type != ETT_CJSONEE) {
            if (top->type == ETT_VALUE && top->node == NULL) {
                tkz_stack_drop_top();
            }
            else {
                update_tkz_stack(parser);
            }
            top = tkz_stack_top();
            if (1 == tkz_stack_size()) {
                break;
            }
        }
        if (top->type == ETT_CJSONEE) {
            tkz_stack_push(ETT_SEMICOLON);
            update_tkz_stack(parser);
            tkz_stack_push(ETT_VALUE);
            RESET_TEMP_BUFFER();
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
        else {
            struct pcejson_token *token = tkz_stack_pop();
            tkz_stack_push(ETT_MULTI_UNQUOTED_S);
            top = tkz_stack_top();
            pcvcm_node_append_child(top->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
        }
    }
    RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_CJSONEE_FINISHED)
    if (character == '}') {
        APPEND_TO_TEMP_BUFFER(character);
        if (tkz_buffer_equal_to(parser->temp_buffer, "}}", 2)) {
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
        ADVANCE_TO(EJSON_TKZ_STATE_CJSONEE_FINISHED);
    }
    if (tkz_buffer_equal_to(parser->temp_buffer, "}}", 2)) {
        update_tkz_stack(parser);
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_RAW_STRING)
    if (is_parse_finished(parser, character)) {
        if (top && top->type == ETT_VALUE) {
            tkz_stack_drop_top();
        }
        top = tkz_stack_top();
        if (!tkz_buffer_is_empty(parser->temp_buffer)
                && !tkz_buffer_is_whitespace(parser->temp_buffer)) {
            // FIXME: remove 0x0a of the file
            if (tkz_buffer_end_with(parser->temp_buffer, "\n", 1)) {
                tkz_buffer_delete_tail_chars(parser->temp_buffer, 1);
            }
            if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                if (top->node) {
                    struct pcvcm_node *node = pcvcm_node_new_string(
                            tkz_buffer_get_bytes(parser->temp_buffer)
                            );
                    node->position = parser->temp_ucs->nr_ucs -
                        tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
                    pcvcm_node_append_child(top->node, node);
                }
                else {
                    top->node = pcvcm_node_new_string(
                            tkz_buffer_get_bytes(parser->temp_buffer)
                            );
                    top->node->position = parser->temp_ucs->nr_ucs -
                        tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
                }
                RESET_TEMP_BUFFER();
                update_tkz_stack(parser);
            }
        }
        RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
    }
    if (is_whitespace(character)) {
        if ((parser->flags & PCEJSON_FLAG_MULTI_JSONEE) != 0) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_RAW_STRING);
        }
        if (top && top->type == ETT_VALUE) {
            tkz_stack_drop_top();
        }
        top = tkz_stack_top();
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            // FIXME: remove 0x0a of the file
            if (tkz_buffer_end_with(parser->temp_buffer, "\n", 1)) {
                tkz_buffer_delete_tail_chars(parser->temp_buffer, 1);
            }
            if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                if (top->node) {
                    struct pcvcm_node *node = pcvcm_node_new_string(
                            tkz_buffer_get_bytes(parser->temp_buffer)
                            );
                    node->position = parser->temp_ucs->nr_ucs -
                        tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
                    pcvcm_node_append_child(top->node, node);
                }
                else {
                    top->node = pcvcm_node_new_string(
                            tkz_buffer_get_bytes(parser->temp_buffer)
                            );
                    top->node->position = parser->temp_ucs->nr_ucs -
                        tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
                }
                RESET_TEMP_BUFFER();
                update_tkz_stack(parser);
            }
        }
        RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
    }
    if (character == '&') {
        SET_RETURN_STATE(EJSON_TKZ_STATE_RAW_STRING);
        ADVANCE_TO(EJSON_TKZ_STATE_CHARACTER_REFERENCE);
    }
    if ((character == '$' && (parser->flags & PCEJSON_FLAG_GET_VARIABLE))
            || character == '{') {
        if (top->type == ETT_VALUE) {
            tkz_stack_drop_top();
            top = tkz_stack_top();
        }
        if (top->type == ETT_UNQUOTED_S) {
             /* U */
            tkz_stack_drop_top();
            tkz_stack_push(ETT_MULTI_UNQUOTED_S);
            top = tkz_stack_top();
        }

        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            pcvcm_node_append_child(top->node, node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    if (character == '\\') {
        SET_RETURN_STATE(curr_state);
        ADVANCE_TO(EJSON_TKZ_STATE_STRING_ESCAPE);
    }
    if (character == '"') {
        if (top->type == ETT_MULTI_QUOTED_S) {
            close_token(parser, top);
            update_tkz_stack(parser);
            if (is_parse_finished(parser, character)) {
                ADVANCE_TO(EJSON_TKZ_STATE_FINISHED);
            }
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(EJSON_TKZ_STATE_RAW_STRING);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VARIABLE)
    if (character == '_' || is_ascii_alpha(character)
            || is_unihan(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VARIABLE);
    }
    if (is_ascii_digit(character)) {
        // FIXME:
#if 0
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
#endif
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VARIABLE);
    }
    if (is_context_variable(character)) {
        if (tkz_buffer_is_empty(parser->temp_buffer)
            || tkz_buffer_is_int(parser->temp_buffer)
            || tkz_buffer_start_with(parser->temp_buffer, "#", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_VARIABLE);
        }
    }
    if (character == '#') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_VARIABLE);
        }
    }
    if (!tkz_buffer_is_empty(parser->temp_buffer)) {
        if (tkz_buffer_is_int(parser->temp_buffer)) {
            struct pcejson_token *prev = tkz_prev_token();
            if (prev && (prev->type == ETT_GET_MEMBER)) {
                SET_ERR(PCEJSON_ERROR_BAD_JSONEE_KEYWORD);
                RETURN_AND_STOP_PARSE();
            }
        }
        top->node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer)
                );
        top->node->position = parser->temp_ucs->nr_ucs -
            tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
        RESET_TEMP_BUFFER();
    }
    RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VARIABLE);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_AFTER_VARIABLE)
    if (character == '}') {
        update_tkz_stack(parser);
        top = tkz_stack_top();
        if (top->type == ETT_GET_MEMBER ||
                top->type == ETT_GET_MEMBER_BY_BRACKET ||
                top->type == ETT_GET_VARIABLE) {
            update_tkz_stack(parser);
        }
        top = tkz_stack_top();
        if (top->type == ETT_MULTI_QUOTED_S ||
                top->type == ETT_MULTI_UNQUOTED_S) {
            close_token(parser, top);
            update_tkz_stack(parser);
        }
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    else if (character == '!') {
        RESET_TEMP_BUFFER();
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKA_STATE_EXCLAMATION_MARK);
    }
    else if (character == '.' || character == '(' || character == '[') {
        top = tkz_stack_top();
        if (top && (top->node == NULL) && (top->type == ETT_VALUE)) {
            struct pcejson_token *prev = tkz_prev_token();
            if (prev && prev->type == ETT_GET_VARIABLE) {
                SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
                RETURN_AND_STOP_PARSE();
            }
        }
        update_tkz_stack_with_level(parser, 1);
    }
    else if (character == '$' && (parser->flags & PCEJSON_FLAG_GET_VARIABLE)) {
        top = tkz_stack_top();
        if (top->type == ETT_VALUE && top->node) {
            struct pcejson_token *token = tkz_stack_pop();

            top = tkz_stack_top();
            if (top->type == ETT_GET_MEMBER ||
                    top->type == ETT_GET_MEMBER_BY_BRACKET ||
                    top->type == ETT_GET_VARIABLE) {
                pcvcm_node_append_child(top->node, token->node);
                token->node = NULL;
                pcejson_token_destroy(token);
                close_token(parser, top);
                update_tkz_stack(parser);
            }
            else {
                if ((top->type != ETT_MULTI_QUOTED_S) &&
                        (top->type != ETT_MULTI_UNQUOTED_S)) {
                    tkz_stack_push(ETT_MULTI_UNQUOTED_S);
                    top = tkz_stack_top();
                }
                pcvcm_node_append_child(top->node, token->node);
                token->node = NULL;
                pcejson_token_destroy(token);
            }
        }
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    else if (character == ',') {
        update_tkz_stack(parser);
        struct pcejson_token *token = tkz_stack_top();
        while (token) {
            if (token->type == ETT_CALL_SETTER || token->type == ETT_OBJECT ||
                    token->type == ETT_CALL_GETTER || token->type == ETT_ARRAY
                    || token->type == ETT_TUPLE) {
                break;
            }
            size_t nr = tkz_stack_size();
            if (nr == 1) {
                if (token->type != ETT_MULTI_UNQUOTED_S
                        && token->type != ETT_MULTI_QUOTED_S) {
                    close_token(parser, token);
                    struct pcejson_token *token = tkz_stack_pop();
                    tkz_stack_push(ETT_MULTI_UNQUOTED_S);
                    top = tkz_stack_top();

                    pcvcm_node_append_child(top->node, token->node);
                    token->node = NULL;
                    pcejson_token_destroy(token);
                }
                RESET_TEMP_BUFFER();
                RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
            }
            else if (token->type == ETT_MULTI_UNQUOTED_S
                        || token->type == ETT_MULTI_QUOTED_S) {
                RESET_TEMP_BUFFER();
                RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
            }
            update_tkz_stack(parser);
            top = tkz_stack_top();

            struct pcejson_token *prev = tkz_prev_token();
            if (top && top->type == ETT_VALUE && prev &&
                (prev->type == ETT_OP_COMMA ||
                 prev->type == ETT_OP_EXPR_IN_FUNC)) {
                RECONSUME_IN(EJSON_TKZ_STATE_OP_COMMA);
            }
            if (top && top->type == ETT_VALUE && prev && prev->type == ETT_OP_EXPR){
                size_t nr = pcvcm_node_children_count(prev->node);
                if (nr == 0) {
                    RECONSUME_IN(EJSON_TKZ_STATE_OP_COMMA);
                }
                struct pcvcm_node *last = pcvcm_node_last_child(prev->node);
                if (last->type == PCVCM_NODE_TYPE_OP_LP) {
                    RECONSUME_IN(EJSON_TKZ_STATE_OP_COMMA);
                }
            }
            if (top == token) {
                SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
                RETURN_AND_STOP_PARSE();
            }
            token = top;
        }
    }
    else if (character == '"') {
        update_tkz_stack(parser);

        struct pcejson_token *prev = tkz_prev_token();
        if (prev && prev->type == ETT_DOUBLE_S) {
            top = tkz_stack_pop();
            prev->node = top->node;
            top->node = NULL;
            pcejson_token_destroy(top);
            update_tkz_stack(parser);
            if (is_parse_finished(parser, character)) {
                ADVANCE_TO(EJSON_TKZ_STATE_FINISHED);
            }
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }

        top = tkz_stack_top();
        if (top->type == ETT_GET_MEMBER ||
                top->type == ETT_GET_MEMBER_BY_BRACKET ||
                top->type == ETT_GET_VARIABLE) {
            update_tkz_stack(parser);
        }
        top = tkz_stack_top();
        if (top->type == ETT_MULTI_QUOTED_S) {
            close_token(parser, top);
            update_tkz_stack(parser);
            if (is_parse_finished(parser, character)) {
                ADVANCE_TO(EJSON_TKZ_STATE_FINISHED);
            }
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
        if (top->type == ETT_MULTI_UNQUOTED_S) {
            //FIXME:
            struct pcvcm_node *node = pcvcm_node_new_string("\"");
            node->position = parser->temp_ucs->nr_ucs - 1;
            pcvcm_node_append_child(top->node, node);
            update_tkz_stack(parser);
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
    }
    else {
        update_tkz_stack(parser);
        top = tkz_stack_top();
        if (top->type == ETT_TRIPLE_DOUBLE_QUOTED) {
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_VALUE_TRIPLE_DOUBLE_QUOTED);
        }
        if (top->type == ETT_GET_MEMBER ||
                top->type == ETT_GET_MEMBER_BY_BRACKET ||
                top->type == ETT_GET_VARIABLE) {
            update_tkz_stack(parser);
        }
        top = tkz_stack_top();
        if (top->type == ETT_MULTI_QUOTED_S) {
            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
        }
        top = tkz_stack_top();
        if (top && top->type == ETT_TRIPLE_DOUBLE_QUOTED) {
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (is_parse_finished(parser, character)) {
            if (top->type == ETT_MULTI_UNQUOTED_S) {
                close_token(parser, top);
            }
            update_tkz_stack(parser);
            RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
        }
        top = tkz_stack_top();
        if (top->type == ETT_MULTI_UNQUOTED_S) {
            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
        }
        if (is_parse_finished(parser, character)) {
            RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
        }
        if (top->type == ETT_TRIPLE_DOUBLE_QUOTED) {
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_VALUE_TRIPLE_DOUBLE_QUOTED);
        }
        size_t nr = tkz_stack_size();
        if (nr == 1 && pcejson_token_is_closed(top)) {
            struct pcejson_token *token = tkz_stack_pop();
            tkz_stack_push(ETT_MULTI_UNQUOTED_S);
            top = tkz_stack_top();

            pcvcm_node_append_child(top->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
        }
    }
    RESET_TEMP_BUFFER();
    RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
END_STATE()


BEGIN_STATE(EJSON_TKZ_STATE_CHARACTER_REFERENCE)
    RESET_STRING_BUFFER();
    APPEND_TO_STRING_BUFFER('&');
    if (is_ascii_alpha_numeric(character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_NAMED_CHARACTER_REFERENCE);
    }
    if (character == '#') {
        APPEND_TO_STRING_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_NUMERIC_CHARACTER_REFERENCE);
    }
    APPEND_BUFFER_TO_TEMP_BUFFER(parser->string_buffer);
    RESET_STRING_BUFFER();
    RECONSUME_IN(parser->return_state);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_NAMED_CHARACTER_REFERENCE)
    if (parser->sbst == NULL) {
        parser->sbst = tkz_sbst_new_char_ref();
    }
    bool ret = tkz_sbst_advance(parser->sbst, character);
    if (!ret) {
        struct pcutils_arrlist* ucs = tkz_sbst_get_buffered_ucs(
                parser->sbst);
        size_t length = pcutils_arrlist_length(ucs);
        for (size_t i = 0; i < length; i++) {
            uint32_t uc = (uint32_t)(uintptr_t) pcutils_arrlist_get_idx(
                    ucs, i);
            APPEND_TO_STRING_BUFFER(uc);
        }
        tkz_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        APPEND_BUFFER_TO_TEMP_BUFFER(parser->string_buffer);
        RESET_STRING_BUFFER();
        ADVANCE_TO(EJSON_TKZ_STATE_AMBIGUOUS_AMPERSAND);
    }

    const char* value = tkz_sbst_get_match(parser->sbst);
    if (value == NULL) {
        ADVANCE_TO(EJSON_TKZ_STATE_NAMED_CHARACTER_REFERENCE);
    }
    if (character != ';') {
        ADVANCE_TO(EJSON_TKZ_STATE_NAMED_CHARACTER_REFERENCE);
    }
    APPEND_BYTES_TO_TEMP_BUFFER(value, strlen(value));
    RESET_STRING_BUFFER();

    tkz_sbst_destroy(parser->sbst);
    parser->sbst = NULL;
    ADVANCE_TO(parser->return_state);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_AMBIGUOUS_AMPERSAND)
    if (is_ascii_alpha_numeric(character)) {
        RECONSUME_IN(parser->return_state);
    }
    if (character == ';') {
        SET_ERR(PCHVML_ERROR_UNKNOWN_NAMED_CHARACTER_REFERENCE);
        RETURN_AND_STOP_PARSE();
    }
    RECONSUME_IN(parser->return_state);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_NUMERIC_CHARACTER_REFERENCE)
    parser->char_ref_code = 0;
    if (character == 'x' || character == 'X') {
        APPEND_TO_STRING_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_HEXADECIMAL_CHARACTER_REFERENCE_START);
    }
    RECONSUME_IN(EJSON_TKZ_STATE_DECIMAL_CHARACTER_REFERENCE_START);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_HEXADECIMAL_CHARACTER_REFERENCE_START)
    if (is_ascii_hex_digit(character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_HEXADECIMAL_CHARACTER_REFERENCE);
    }
    SET_ERR(
        PCHVML_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_DECIMAL_CHARACTER_REFERENCE_START)
    if (is_ascii_digit(character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_DECIMAL_CHARACTER_REFERENCE);
    }
    SET_ERR(
        PCHVML_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_HEXADECIMAL_CHARACTER_REFERENCE)
    if (is_ascii_digit(character)) {
        parser->char_ref_code *= 16;
        parser->char_ref_code += character - 0x30;
        ADVANCE_TO(EJSON_TKZ_STATE_HEXADECIMAL_CHARACTER_REFERENCE);
    }
    if (is_ascii_upper_hex_digit(character)) {
        parser->char_ref_code *= 16;
        parser->char_ref_code += character - 0x37;
        ADVANCE_TO(EJSON_TKZ_STATE_HEXADECIMAL_CHARACTER_REFERENCE);
    }
    if (is_ascii_lower_hex_digit(character)) {
        parser->char_ref_code *= 16;
        parser->char_ref_code += character - 0x57;
        ADVANCE_TO(EJSON_TKZ_STATE_HEXADECIMAL_CHARACTER_REFERENCE);
    }
    if (character == ';') {
        ADVANCE_TO(EJSON_TKZ_STATE_NUMERIC_CHARACTER_REFERENCE_END);
    }
    SET_ERR(PCHVML_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_DECIMAL_CHARACTER_REFERENCE)
    if (is_ascii_digit(character)) {
        parser->char_ref_code *= 10;
        parser->char_ref_code += character - 0x30;
        ADVANCE_TO(EJSON_TKZ_STATE_DECIMAL_CHARACTER_REFERENCE);
    }
    if (character == ';') {
        ADVANCE_TO(EJSON_TKZ_STATE_NUMERIC_CHARACTER_REFERENCE_END);
    }
    SET_ERR(PCHVML_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_NUMERIC_CHARACTER_REFERENCE_END)
    uint32_t uc = parser->char_ref_code;
    if (uc == 0x00) {
        SET_ERR(PCHVML_ERROR_NULL_CHARACTER_REFERENCE);
        parser->char_ref_code = 0xFFFD;
        RETURN_AND_STOP_PARSE();
    }
    if (uc > 0x10FFFF) {
        SET_ERR(PCHVML_ERROR_CHARACTER_REFERENCE_OUTSIDE_UNICODE_RANGE);
        parser->char_ref_code = 0xFFFD;
        RETURN_AND_STOP_PARSE();
    }
    if ((uc & 0xFFFFF800) == 0xD800) {
        SET_ERR(PCHVML_ERROR_SURROGATE_CHARACTER_REFERENCE);
        RETURN_AND_STOP_PARSE();
    }
    if (uc >= 0xFDD0 && (uc <= 0xFDEF || (uc&0xFFFE) == 0xFFFE) &&
            uc <= 0x10FFFF) {
        SET_ERR(PCHVML_ERROR_NONCHARACTER_CHARACTER_REFERENCE);
        RETURN_AND_STOP_PARSE();
    }
    if (uc <= 0x1F &&
            !(uc == 0x09 || uc == 0x0A || uc == 0x0C)){
        SET_ERR(PCHVML_ERROR_CONTROL_CHARACTER_REFERENCE);
        RETURN_AND_STOP_PARSE();
    }
    if (uc >= 0x7F && uc <= 0x9F) {
        SET_ERR(PCHVML_ERROR_CONTROL_CHARACTER_REFERENCE);
        if (uc >= 0x80) {
            parser->char_ref_code =
                numeric_char_ref_extension_array[uc - 0x80];
        }
        RETURN_AND_STOP_PARSE();
    }
    uc = parser->char_ref_code;
    APPEND_TO_TEMP_BUFFER(uc);
    RESET_STRING_BUFFER();
    RECONSUME_IN(parser->return_state);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_LINE_COMMENT)
    if (character == '\n' || is_eof(character)
            || is_finished_by_callback(parser, character)) {
        ADVANCE_TO(parser->return_state);
    }
    ADVANCE_TO(EJSON_TKZ_STATE_LINE_COMMENT);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_BACKQUOTE)
    tkz_stack_push(ETT_BACKQUOTE);
    RESET_TEMP_BUFFER();
    ADVANCE_TO(EJSON_TKZ_STATE_BACKQUOTE_CONTENT);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_BACKQUOTE_CONTENT)
    if (is_whitespace(character)) {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            ADVANCE_TO(EJSON_TKZ_STATE_BACKQUOTE_CONTENT);
        }
        const char *buf = tkz_buffer_get_bytes(parser->temp_buffer);
        purc_atom_t t = purc_atom_try_string_ex(ATOM_BUCKET_EXCEPT, buf);
        if (t == 0) {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
        struct pcvcm_node *node = pcvcm_node_new_ulongint(t);
        node->position = parser->temp_ucs->nr_ucs -
            tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
        pcvcm_node_append_child(top->node, node);
        RESET_TEMP_BUFFER();
        ADVANCE_TO(EJSON_TKZ_STATE_BACKQUOTE_CONTENT);
    }

    if (character == '`' || is_eof(character)
            || is_finished_by_callback(parser, character)) {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            const char *buf = tkz_buffer_get_bytes(parser->temp_buffer);
            purc_atom_t t = purc_atom_try_string_ex(ATOM_BUCKET_EXCEPT, buf);
            if (t == 0) {
                SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
                RETURN_AND_STOP_PARSE();
            }
            struct pcvcm_node *node = pcvcm_node_new_ulongint(t);
            node->position = parser->temp_ucs->nr_ucs -
                tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
            pcvcm_node_append_child(top->node, node);
            RESET_TEMP_BUFFER();
        }
        close_token(parser, top);
        update_tkz_stack(parser);
        ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
    }

    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(EJSON_TKZ_STATE_BACKQUOTE_CONTENT);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_PARAM_STRING)
    if ((character == '"' || character == '\'')
            && tkz_buffer_is_empty(parser->temp_buffer) ) {
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    if (character == '(') {
        if (!top) {
            RECONSUME_IN(EJSON_TKZ_STATE_OP_EXPR);
        }
    }
    if (character == '}' || character == '[' || character == ']'
            || character == '(' || character == ')') {
        RESET_TEMP_BUFFER();
        tkz_stack_push(ETT_UNQUOTED_S);
        tkz_stack_push(ETT_VALUE);
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_RAW_STRING);
    }
    if (character == '$' && (parser->flags & PCEJSON_FLAG_GET_VARIABLE)) {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            if (tkz_buffer_end_with(parser->temp_buffer, "{", 1)) {
                RECONSUME_LAST_CHAR();
                RECONSUME_LAST_CHAR();
                DELETE_FROM_RAW_BUFFER(2);
                tkz_buffer_delete_tail_chars(parser->temp_buffer, 1);
            }
            else if (tkz_buffer_end_with(parser->temp_buffer, "{{", 2)) {
                RECONSUME_LAST_CHAR();
                RECONSUME_LAST_CHAR();
                RECONSUME_LAST_CHAR();
                DELETE_FROM_RAW_BUFFER(3);
                tkz_buffer_delete_tail_chars(parser->temp_buffer, 2);
            }
            else if (!tkz_buffer_is_empty(parser->string_buffer)) {
                size_t sz = 1 + tkz_buffer_get_size_in_chars(parser->string_buffer);
                for (size_t i = 0; i < sz; i++) {
                    RECONSUME_LAST_CHAR();
                }
                DELETE_FROM_RAW_BUFFER(sz);
                tkz_buffer_delete_tail_chars(parser->temp_buffer, sz - 1);
            }
            else {
                RECONSUME_LAST_CHAR();
                DELETE_FROM_RAW_BUFFER(1);
            }
            if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                tkz_stack_push(ETT_STRING);
                top = tkz_stack_top();
                top->node = pcvcm_node_new_string(
                        tkz_buffer_get_bytes(parser->temp_buffer)
                        );
                top->node->position = parser->temp_ucs->nr_ucs -
                    tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
                update_tkz_stack(parser);
                RESET_TEMP_BUFFER();
            }
        }
        else {
            RECONSUME_LAST_CHAR();
        }
        RESET_STRING_BUFFER();
        RESET_TEMP_BUFFER();
        ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
    }
    if (character == '{') {
        APPEND_TO_STRING_BUFFER(character);
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_PARAM_STRING);
    }
    if (is_whitespace(character)
            && !tkz_buffer_is_empty(parser->string_buffer)) {
        APPEND_TO_STRING_BUFFER(character);
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_PARAM_STRING);
    }
    RESET_STRING_BUFFER();
    if (!tkz_buffer_is_empty(parser->temp_buffer)) {
        tkz_stack_push(ETT_UNQUOTED_S);
        tkz_stack_push(ETT_VALUE);
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_RAW_STRING);
    }
    RECONSUME_IN(EJSON_TKZ_STATE_UNQUOTED);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_ATTR_VALUE)
    if (is_whitespace(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_ATTR_VALUE);
    }
    if (is_eof(character)) {
        tkz_stack_push(ETT_STRING);
        top = tkz_stack_top();
        top->node = pcvcm_node_new_string(
            tkz_buffer_get_bytes(parser->temp_buffer));
        top->node->quoted_type = PCVCM_NODE_QUOTED_TYPE_DOUBLE;
        top->node->position = parser->temp_ucs->nr_ucs -
            tkz_buffer_get_size_in_chars(parser->temp_buffer) - 1;
    }
    RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_EXPR)
    if (is_whitespace(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_EXPR);
    }

    if (character == '(') {
        if (top) {
            if (is_any_op_expr(top)) {
                struct pcvcm_node *sign = pcvcm_node_new_op_lp();
                pcvcm_node_append_child(top->node, sign);
                tkz_stack_push(ETT_VALUE);
                ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
            }

            struct pcejson_token *prev = tkz_prev_token();
            if (is_any_op_expr(prev)) {
                struct pcejson_token *token = tkz_stack_pop();
                pcvcm_node_append_child(prev->node, token->node);
                token->node = NULL;
                pcejson_token_destroy(token);

                struct pcvcm_node *sign = pcvcm_node_new_op_lp();
                pcvcm_node_append_child(prev->node, sign);
                tkz_stack_push(ETT_VALUE);
                ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
            }
        }
        tkz_stack_push(ETT_OP_EXPR);
        tkz_stack_push(ETT_VALUE);
        ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
    }

END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_AFTER_OP_EXPR)
    if (character == ')') {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *prev = tkz_prev_token();
            if (is_any_op_expr(prev)) {

                struct pcejson_token *token = tkz_stack_pop();
                struct pcejson_token *parent = tkz_stack_top();
                pcvcm_node_append_child(parent->node, token->node);
                token->node = NULL;
                pcejson_token_destroy(token);

                bool found_lp = false;
                struct pcvcm_node *last = pcvcm_node_last_child(parent->node);
                int found_rp = 0;
                while(last) {
                    if (last->type == PCVCM_NODE_TYPE_OP_RP) {
                        found_rp++;
                    }

                    if (last->type == PCVCM_NODE_TYPE_OP_LP) {
                        if (found_rp) {
                            found_rp--;
                            last = pcvcm_node_prev_child(last);
                            continue;
                        }
                        found_lp = true;
                        break;
                    }

                    last = pcvcm_node_prev_child(last);
                }
                bool closed = false;
                if (found_lp) {
                    struct pcvcm_node *sign = pcvcm_node_new_op_rp();
                    pcvcm_node_append_child(prev->node, sign);
                    tkz_stack_push(ETT_VALUE);
                }
                else {
                    close_token(parser, parent);
                    closed = true;
                }

                top = tkz_stack_top();
                if (closed && is_op_expr_in_func(top)) {
                    struct pcejson_token *token = tkz_stack_pop();

                    top = tkz_stack_top();
                    if (top->type == ETT_VALUE && top->node == NULL) {
                        tkz_stack_drop_top();
                    }
                    top = tkz_stack_top();

                    pcvcm_node_append_child(top->node, token->node);
                    token->node = NULL;
                    pcejson_token_destroy(token);

                    RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
                }

                struct pcejson_token *prev = tkz_prev_token();
                if (closed && prev && prev->node) {
                    struct pcejson_token *token = tkz_stack_pop();
                    top = tkz_stack_top();
                    pcvcm_node_append_child(top->node, token->node);
                    token->node = NULL;
                    pcejson_token_destroy(token);
                }
                ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
            }
        }
        if (is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            {
                bool found_lp = false;
                struct pcvcm_node *last = pcvcm_node_last_child(top->node);
                int found_rp = 0;
                while(last) {
                    if (last->type == PCVCM_NODE_TYPE_OP_RP) {
                        found_rp++;
                    }

                    if (last->type == PCVCM_NODE_TYPE_OP_LP) {
                        if (found_rp) {
                            found_rp--;
                            last = pcvcm_node_prev_child(last);
                            continue;
                        }
                        found_lp = true;
                        break;
                    }

                    last = pcvcm_node_prev_child(last);
                }
                if (found_lp) {
                    struct pcvcm_node *sign = pcvcm_node_new_op_rp();
                    pcvcm_node_append_child(top->node, sign);
                    tkz_stack_push(ETT_VALUE);
                    ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
                }
            }

            close_token(parser, top);
            if (is_op_expr_in_func(top)) {
                struct pcejson_token *token = tkz_stack_pop();
                top = tkz_stack_top();
                pcvcm_node_append_child(top->node, token->node);
                token->node = NULL;
                pcejson_token_destroy(token);

                ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
            }
            struct pcejson_token *prev = tkz_prev_token();
            if (prev && prev->node) {
                struct pcejson_token *token = tkz_stack_pop();
                top = tkz_stack_top();
                pcvcm_node_append_child(top->node, token->node);
                token->node = NULL;
                pcejson_token_destroy(token);
            }
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_EXPR_IN_FUNC)
    struct pcvcm_node *last = NULL;
    if (top && (top->type == ETT_CALL_GETTER || top->type == ETT_CALL_SETTER)) {
        last = pcvcm_node_last_child(top->node);
    }

    tkz_stack_push(ETT_OP_EXPR_IN_FUNC);
    if (last) {
        pcvcm_node_remove_child(top->node, last);

        top = tkz_stack_top();
        pcvcm_node_append_child(top->node, last);
    }

    tkz_stack_push(ETT_VALUE);
    RECONSUME_IN(EJSON_TKZ_STATE_OP_SIGN);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_SIGN)
    if (character == '(') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_EXPR);
    }
    if (character == ')') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_OP_EXPR);
    }
    if (character == '+') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_PLUS);
    }
    if (character == '-') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_MINUS);
    }
    if (character == '*') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_MUL);
    }
    if (character == '/') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_DIV);
    }
    if (character == '%') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_MOD);
    }
    if (character == '=') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_EQUAL);
    }
    if (character == '!') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_NOT_EQUAL);
    }
    if (character == '>') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_GREATER);
    }
    if (character == '<') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_LESS);
    }
    if (character == '&') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_BITWISE_AND);
    }
    if (character == '|') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_BITWISE_OR);
    }
    if (character == '~') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_BITWISE_INVERT);
    }
    if (character == '^') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_BITWISE_XOR);
    }
    if (character == '?' || character == ':') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_CONDITIONAL);
    }
    if (character == ',') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_COMMA);
    }
    if (character == 'a') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_AND);
    }
    if (character == 'o') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_OR);
    }
    if (character == 'n') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_NOT);
    }
    if (character == 'i') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_OP_IN);
    }
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_PLUS)
    if (character == '+') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_PLUS);
    }
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_PLUS);
    }

    if (tkz_buffer_end_with(parser->temp_buffer, "+=", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_plus_assign();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_plus_assign();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    } else if (tkz_buffer_end_with(parser->temp_buffer, "++", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_increment();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_increment();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    } else {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = NULL;
            struct pcvcm_node *last = pcvcm_node_last_child(parent->node);
            if (!last || last->type == PCVCM_NODE_TYPE_OP_LP) {
                sign = pcvcm_node_new_op_unary_plus();
            }
            else {
                sign = pcvcm_node_new_op_add();
            }
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_add();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_MINUS)
    if (character == '-') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_MINUS);
    }
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_MINUS);
    }

    if (tkz_buffer_end_with(parser->temp_buffer, "-=", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign =
                pcvcm_node_new_op_minus_assign();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
            pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign =
                pcvcm_node_new_op_minus_assign();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    } else if (tkz_buffer_end_with(parser->temp_buffer, "--", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_decrement();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_decrement();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    } else {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = NULL;
            struct pcvcm_node *last = pcvcm_node_last_child(parent->node);
            if (!last || last->type == PCVCM_NODE_TYPE_OP_LP) {
                sign = pcvcm_node_new_op_unary_minus();
            }
            else {
                sign = pcvcm_node_new_op_sub();
            }

            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_sub();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_MUL)
    if (character == '*') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_MUL);
    }
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_MUL);
    }
    if (tkz_buffer_end_with(parser->temp_buffer, "**=", 3)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_power_assign();
            pcvcm_node_append_child(parent->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_power_assign();
            pcvcm_node_append_child(top->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    } else if (tkz_buffer_end_with(parser->temp_buffer, "*=", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_multiply_assign();
            pcvcm_node_append_child(parent->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_multiply_assign();
            pcvcm_node_append_child(top->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    } else if (tkz_buffer_end_with(parser->temp_buffer, "**", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_power();
            pcvcm_node_append_child(parent->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_power();
            pcvcm_node_append_child(top->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    } else {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_mul();
            pcvcm_node_append_child(parent->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_mul();
            pcvcm_node_append_child(top->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_DIV)
    if (character == '/') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_DIV);
    }
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_DIV);
    }
    if (tkz_buffer_end_with(parser->temp_buffer, "//=", 3)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_floor_div_assign();
            pcvcm_node_append_child(parent->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_floor_div_assign();
            pcvcm_node_append_child(top->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    else if (tkz_buffer_end_with(parser->temp_buffer, "/=", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_divide_assign();
            pcvcm_node_append_child(parent->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_divide_assign();
            pcvcm_node_append_child(top->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    else if (tkz_buffer_end_with(parser->temp_buffer, "//", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_floor_div();
            pcvcm_node_append_child(parent->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_floor_div();
            pcvcm_node_append_child(top->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    else {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_true_div();
            pcvcm_node_append_child(parent->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_true_div();
            pcvcm_node_append_child(top->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }

    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_MOD)
    if (character == '%') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_MOD);
    }
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_MOD);
    }
    if (tkz_buffer_end_with(parser->temp_buffer, "%=", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign =
                pcvcm_node_new_op_modulo_assign();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign =
                pcvcm_node_new_op_modulo_assign();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    else {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_modulo();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_modulo();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_EQUAL)
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_EQUAL);
    }
    if (tkz_buffer_end_with(parser->temp_buffer, "==", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_equal();
            pcvcm_node_append_child(parent->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_assign();
            pcvcm_node_append_child(top->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    else {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_assign();
            pcvcm_node_append_child(parent->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_assign();
            pcvcm_node_append_child(top->node, sign);

            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_NOT_EQUAL)
    if (character == '!') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_NOT_EQUAL);
    }
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_NOT_EQUAL);
    }

    if (tkz_buffer_end_with(parser->temp_buffer, "!=", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_not_equal();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_not_equal();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_GREATER)
    if (character == '>') {
        if (tkz_buffer_end_with(parser->temp_buffer, ">", 1)) {
            RECONSUME_IN(EJSON_TKZ_STATE_OP_BITWISE_RIGHT_SHIFT);
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_GREATER);
    }
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_GREATER);
    }
    if (tkz_buffer_end_with(parser->temp_buffer, ">=", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_greater_equal();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_greater_equal();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    else {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_greater();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_greater();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_LESS)
    if (character == '<') {
        if (tkz_buffer_end_with(parser->temp_buffer, "<", 1)) {
            RECONSUME_IN(EJSON_TKZ_STATE_OP_BITWISE_LEFT_SHIFT);
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_LESS);
    }
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_LESS);
    }

    if (tkz_buffer_end_with(parser->temp_buffer, "<=", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_less_equal();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_less_equal();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    else {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_less();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_less();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_BITWISE_AND)
    if (character == '&') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_BITWISE_AND);
    }
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_BITWISE_AND);
    }
    if (tkz_buffer_end_with(parser->temp_buffer, "&=", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign =
                pcvcm_node_new_op_bitwise_and_assign();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign =
                pcvcm_node_new_op_bitwise_and_assign();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    else {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_bitwise_and();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_bitwise_and();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_BITWISE_OR)
    if (character == '|') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_BITWISE_OR);
    }
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_BITWISE_OR);
    }
    if (tkz_buffer_end_with(parser->temp_buffer, "!=", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign =
                pcvcm_node_new_op_bitwise_or_assign();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign =
                pcvcm_node_new_op_bitwise_or_assign();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    else {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_bitwise_or();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_bitwise_or();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_BITWISE_INVERT)
    if (character == '~') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_BITWISE_INVERT);
    }
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_BITWISE_INVERT);
    }
    if (tkz_buffer_end_with(parser->temp_buffer, "~=", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign =
                pcvcm_node_new_op_bitwise_invert_assign();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign =
                pcvcm_node_new_op_bitwise_invert_assign();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    else {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_bitwise_invert();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_bitwise_invert();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_BITWISE_XOR)
    if (character == '^') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_BITWISE_XOR);
    }
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_BITWISE_XOR);
    }
    if (tkz_buffer_end_with(parser->temp_buffer, "^=", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_bitwise_xor_assign();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_bitwise_xor_assign();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    else {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_bitwise_xor();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_bitwise_xor();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_BITWISE_LEFT_SHIFT)
    if (character == '<') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_BITWISE_LEFT_SHIFT);
    }
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_BITWISE_LEFT_SHIFT);
    }
    if (tkz_buffer_end_with(parser->temp_buffer, "<<=", 3)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_left_shift_assign();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_left_shift_assign();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    else {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_left_shift();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_left_shift();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_BITWISE_RIGHT_SHIFT)
    if (character == '>') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_BITWISE_RIGHT_SHIFT);
    }
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_OP_BITWISE_RIGHT_SHIFT);
    }
    if (tkz_buffer_end_with(parser->temp_buffer, ">>=", 3)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign =
                pcvcm_node_new_op_right_shift_assign();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign =
                pcvcm_node_new_op_right_shift_assign();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    else {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_right_shift();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_right_shift();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_CONDITIONAL)
    if (character == '?') {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);
        }

        top = tkz_stack_top();
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *last = pcvcm_node_last_child(top->node);
            if (last) {
                pcvcm_node_remove_child(top->node, last);
                struct pcvcm_node *sign =
                    pcvcm_node_new_op_conditional();
                pcvcm_node_append_child(sign, last);
                pcvcm_node_append_child(top->node, sign);

                tkz_stack_push(ETT_OP_COND_THEN);
                tkz_stack_push(ETT_VALUE);
                ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
            }
        }
    }
    if (character == ':') {
        struct pcejson_token *token = tkz_stack_pop();

        if (token->type == ETT_VALUE) {
            /* ETT_OP_COND_THEN: H */
            tkz_stack_drop_top();
        }

        top = tkz_stack_top();
        assert(is_any_op_expr(top));

        struct pcvcm_node *last = pcvcm_node_last_child(top->node);
        assert(last && last->type == PCVCM_NODE_TYPE_OP_CONDITIONAL);

        pcvcm_node_append_child(last, token->node);

        token->node = NULL;
        pcejson_token_destroy(token);

        tkz_stack_push(ETT_OP_COND_ELSE);
        tkz_stack_push(ETT_VALUE);
        ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
    }

    if (top && top->type == ETT_VALUE) {
        struct pcejson_token *prev = tkz_prev_token();
        if (prev && prev->type == ETT_OP_COND_ELSE) {
            struct pcejson_token *token = tkz_stack_pop();

            if (token->type == ETT_VALUE) {
                /* ETT_OP_COND_THEN: H */
                tkz_stack_drop_top();
            }

            top = tkz_stack_top();
            assert(is_any_op_expr(top));

            struct pcvcm_node *last = pcvcm_node_last_child(top->node);
            assert(last && last->type == PCVCM_NODE_TYPE_OP_CONDITIONAL);

            pcvcm_node_append_child(last, token->node);

            token->node = NULL;
            pcejson_token_destroy(token);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }

    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_COMMA)
    if (character == ',') {
        if (top && top->type == ETT_VALUE) {
            struct pcejson_token *prev = tkz_prev_token();
            assert(prev);

            if (prev->type == ETT_OP_COMMA) {
                struct pcejson_token *token = tkz_stack_pop();
                pcvcm_node_append_child(prev->node, token->node);
                token->node = NULL;
                pcejson_token_destroy(token);

                tkz_stack_push(ETT_VALUE);
                ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
            }
            else if (is_any_op_expr(prev)) {
                struct pcejson_token *token = tkz_stack_pop();

                if (prev->type == ETT_OP_EXPR_IN_FUNC) {
                    bool found_lp = false;
                    struct pcvcm_node *last = pcvcm_node_last_child(prev->node);
                    int found_rp = 0;
                    while(last) {
                        if (last->type == PCVCM_NODE_TYPE_OP_RP) {
                            found_rp++;
                        }

                        if (last->type == PCVCM_NODE_TYPE_OP_LP) {
                            if (found_rp) {
                                found_rp--;
                                last = pcvcm_node_prev_child(last);
                                continue;
                            }
                            found_lp = true;
                            break;
                        }

                        last = pcvcm_node_prev_child(last);
                    }

                    if (!found_lp) {
                        pcvcm_node_append_child(prev->node, token->node);
                        token->node = NULL;
                        pcejson_token_destroy(token);
                        close_token(parser, prev);

                        struct pcejson_token *token = tkz_stack_pop();
                        top = tkz_stack_top();
                        if (top->type == ETT_VALUE && top->node == NULL) {
                            tkz_stack_drop_top();
                            top = tkz_stack_top();
                        }

                        pcvcm_node_append_child(top->node, token->node);
                        token->node = NULL;
                        pcejson_token_destroy(token);

                        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
                    }
                }

                struct pcvcm_node *sign =
                    pcvcm_node_new_op_comma();
                pcvcm_node_append_child(sign, token->node);
                token->node = NULL;
                pcejson_token_destroy(token);


                tkz_stack_push(ETT_OP_COMMA);
                struct pcejson_token *top = tkz_stack_top();
                top->node = sign;

                tkz_stack_push(ETT_VALUE);
                ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
            }
        }
    }
    struct pcejson_token *prev = tkz_prev_token();
    if (prev && prev->type == ETT_OP_COMMA) {
        struct pcejson_token *token = tkz_stack_pop();
        pcvcm_node_append_child(prev->node, token->node);
        token->node = NULL;
        pcejson_token_destroy(token);

        struct pcejson_token *comma = tkz_stack_pop();
        struct pcejson_token *top = tkz_stack_top();
        pcvcm_node_append_child(top->node, comma->node);
        comma->node = NULL;
        pcejson_token_destroy(comma);

        tkz_stack_push(ETT_VALUE);
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_AND)
    if (character == 'a') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_OP_AND);
        }

        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'n') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "a", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_OP_AND);
        }

        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'd') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "an", 2)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_OP_AND);
        }

        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (is_whitespace(character) &&
        tkz_buffer_equal_to(parser->temp_buffer, "and", 3)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_logical_and();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_logical_and();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_OR)
    if (character == 'o') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_OP_OR);
        }

        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'r') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "o", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_OP_OR);
        }

        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }

    if (is_whitespace(character) &&
        tkz_buffer_equal_to(parser->temp_buffer, "or", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_logical_or();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_logical_or();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }

    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_NOT)
    if (character == 'n') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_OP_NOT);
        }
        else if (tkz_buffer_equal_to(parser->temp_buffer, "not i", 5)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_OP_NOT);
        }

        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'o') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "n", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_OP_NOT);
        }

        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 't') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "no", 2)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_OP_NOT);
        }

        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (is_whitespace(character)) {
        if (tkz_buffer_equal_to(parser->temp_buffer, "not in", 6)) {
            if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
                struct pcejson_token *token = tkz_stack_pop();
                struct pcejson_token *parent = tkz_stack_top();
                pcvcm_node_append_child(parent->node, token->node);
                token->node = NULL;
                pcejson_token_destroy(token);

                struct pcvcm_node *sign = pcvcm_node_new_op_not_in();
                pcvcm_node_append_child(parent->node, sign);

                tkz_stack_push(ETT_VALUE);
                ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
            }
            if (top && is_any_op_expr(top) &&
                    pcvcm_node_children_count(top->node) > 0) {
                struct pcvcm_node *sign = pcvcm_node_new_op_not_in();
                pcvcm_node_append_child(top->node, sign);

                tkz_stack_push(ETT_VALUE);
                ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
            }
        }
        else if (tkz_buffer_equal_to(parser->temp_buffer, "not", 3)) {
            APPEND_TO_TEMP_BUFFER(' ');
            ADVANCE_TO(EJSON_TKZ_STATE_OP_NOT);
        }
        else if (tkz_buffer_equal_to(parser->temp_buffer, "not ", 4)) {
            ADVANCE_TO(EJSON_TKZ_STATE_OP_NOT);
        }
        else {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
    }
    if (character == 'i') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "not ", 4)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_OP_NOT);
        }
    }
    if (tkz_buffer_equal_to(parser->temp_buffer, "not ", 4)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_logical_not();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_logical_not();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OP_IN)
    if (character == 'i') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_OP_IN);
        }

        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'n') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "i", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_OP_IN);
        }

        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }

    if (is_whitespace(character) &&
        tkz_buffer_equal_to(parser->temp_buffer, "in", 2)) {
        if (top && (!is_any_op_expr(top)) && tkz_stack_size() > 0) {
            struct pcejson_token *token = tkz_stack_pop();
            struct pcejson_token *parent = tkz_stack_top();
            pcvcm_node_append_child(parent->node, token->node);
            token->node = NULL;
            pcejson_token_destroy(token);

            struct pcvcm_node *sign = pcvcm_node_new_op_in();
            pcvcm_node_append_child(parent->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
        if (top && is_any_op_expr(top) &&
                   pcvcm_node_children_count(top->node) > 0) {
            struct pcvcm_node *sign = pcvcm_node_new_op_in();
            pcvcm_node_append_child(top->node, sign);

            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }
    }

    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()


PCEJSON_PARSER_END

