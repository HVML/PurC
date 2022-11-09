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
#define tkz_stack_pop()       pcejson_token_stack_pop(parser->tkz_stack)
#define tkz_stack_push(c)     pcejson_tkz_stack_push(parser, c)
#define tkz_stack_size()      pcejson_token_stack_size(parser->tkz_stack)
#define tkz_stack_reset()     pcejson_token_stack_clear(parser->tkz_stack)
#define tkz_stack_drop_top()                                                \
    do {                                                                    \
        struct pcejson_token *t = tkz_stack_pop();                          \
        pcejson_token_destroy(t);                                           \
    } while (false)

#define tkz_curr_token()      pcejson_token_stack_top(parser->tkz_stack)
#define tkz_prev_token()      tkz_stack_prev_token(parser->tkz_stack)

#define CHECK_FINISHED() do {                                               \
    if (parser->is_finished(parser, character)) {                           \
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
    APPEND_TO_RAW_BUFFER(character);                                        \
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
tkz_stack_prev_token(struct pcejson_token_stack *stack)
{
    size_t nr = pcejson_token_stack_size(stack);
    if (nr > 1) {
        return pcejson_token_stack_get(stack, nr - 2);
    }
    return NULL;
}

static bool
is_get_element(uint32_t type)
{
    return type == ETT_GET_ELEMENT || type == ETT_GET_ELEMENT_BY_BRACKET;
}

static bool
is_parse_finished(struct pcejson *parser, uint32_t character)
{
    if (is_eof(character)
            || parser->is_finished(parser, character)) {
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
        pctree_node_append_child((struct pctree_node*)parent->node,
                (struct pctree_node*)token->node);
        token->node = NULL;
        pcejson_token_destroy(token);
        break;

    case ETT_GET_VARIABLE:
        while (parent && parent->type == ETT_GET_VARIABLE) {
            cr++;
            pctree_node_append_child((struct pctree_node*)parent->node,
                    (struct pctree_node*)token->node);
            token->node = NULL;
            pcejson_token_destroy(token);
            token = NULL;

            pcejson_token_close(parent);

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

    case ETT_GET_ELEMENT:
        pctree_node_append_child((struct pctree_node*)parent->node,
                (struct pctree_node*)token->node);
        token->node = NULL;
        pcejson_token_destroy(token);
        pcejson_token_close(parent);  /* auto close */
        break;

    case ETT_GET_ELEMENT_BY_BRACKET:
        pctree_node_append_child((struct pctree_node*)parent->node,
                (struct pctree_node*)token->node);
        token->node = NULL;
        pcejson_token_destroy(token);
        break;

    case ETT_CALL_GETTER:
    case ETT_CALL_SETTER:
        pctree_node_append_child((struct pctree_node*)parent->node,
                (struct pctree_node*)token->node);
        token->node = NULL;
        pcejson_token_destroy(token);
        break;

    case ETT_PROTECT:
        parent->node = token->node;
        token->node = NULL;
        pcejson_token_destroy(token);
        break;

    case ETT_MULTI_UNQUOTED_S:
        pctree_node_append_child((struct pctree_node*)parent->node,
                (struct pctree_node*)token->node);
        token->node = NULL;
        pcejson_token_destroy(token);
        break;

    case ETT_MULTI_QUOTED_S:
        pctree_node_append_child((struct pctree_node*)parent->node,
                (struct pctree_node*)token->node);
        token->node = NULL;
        pcejson_token_destroy(token);
        break;

    case ETT_CJSONEE:
        pctree_node_append_child((struct pctree_node*)parent->node,
                (struct pctree_node*)token->node);
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

struct pcejson_token *
pcejson_tkz_stack_push(struct pcejson *parser, uint32_t type)
{
    struct pcejson_token *top = NULL;
    switch (type) {
    case ETT_GET_ELEMENT:
    case ETT_GET_ELEMENT_BY_BRACKET:
    case ETT_CALL_GETTER:
    case ETT_CALL_SETTER:
        {
            struct pcejson_token *token = tkz_stack_pop();
            top = pcejson_token_stack_push(parser->tkz_stack, type);
            pctree_node_append_child((struct pctree_node*)top->node,
                    (struct pctree_node*)token->node);
            token->node = NULL;
            pcejson_token_destroy(token);
        }
        break;

    default:
        top = pcejson_token_stack_push(parser->tkz_stack, type);
        break;
    }

    return top;
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

    /* ETT_GET_ELEMENT is auto closed */
    switch (type) {
    case ETT_ARRAY:
    case ETT_TUPLE:
    case ETT_GET_ELEMENT_BY_BRACKET:
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
            pcejson_token_close(token);
            break;
        }
        else if (character == ']' && is_match_right_bracket(token->type)) {
            pcejson_token_close(token);
            break;
        }
        else if (character == ')' && is_match_right_parenthesis(token->type)) {
            pcejson_token_close(token);
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
            pcejson_token_close(token);
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

    for (int i = 0; i < nr; i++) {
        struct pcejson_token *token = pcejson_token_stack_get(
                parser->tkz_stack, i);
        if (!pcejson_token_is_closed(token)) {
            ret = -1;
            goto out;
        }
        pctree_node_append_child((struct pctree_node*)root,
                (struct pctree_node*)token->node);
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
                "in %-60s|uc=%2s|hex=0x%04X"
                "|top=%1c|stack.size=%2ld|stack=%s|node=%s|tmp_buffer=%s|"
                "\n",
                parser->state_name, buf, character,
                type, nr_stack, s_stack, node, tbuf
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
            top = tkz_stack_push(ETT_MULTI_UNQUOTED_S);
            pctree_node_append_child((struct pctree_node*)top->node,
                    (struct pctree_node*)token->node);
            token->node = NULL;
            pcejson_token_destroy(token);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
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
    if (character == '(') {
        RECONSUME_IN(EJSON_TKZ_STATE_LEFT_PARENTHESIS);
    }
    if (character == ')') {
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
        RESET_QUOTED_COUNTER();
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
            pctree_node_append_child((struct pctree_node*)top->node,
                    (struct pctree_node*)node);
        }
        update_tkz_stack(parser);

        RESET_TEMP_BUFFER();
        RESET_QUOTED_COUNTER();
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
        RESET_QUOTED_COUNTER();
        RECONSUME_IN(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
    }
    if (type == ETT_MULTI_QUOTED_S) {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            pctree_node_append_child((struct pctree_node*)top->node,
                    (struct pctree_node*)node);
        }
        pcejson_token_close(top);
        update_tkz_stack(parser);

        RESET_TEMP_BUFFER();
        RESET_QUOTED_COUNTER();
        if (is_parse_finished(parser, character)) {
            RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
        }
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_UNQUOTED)
    if (is_ascii_digit(character) || character == '-') {
        if (!top) {
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
        RESET_TEMP_BUFFER();
        if (top == NULL) {
//            SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
//            RETURN_AND_STOP_PARSE();
            tkz_stack_push(ETT_UNQUOTED_S);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
        }
        uint32_t type = top->type;
        if (type == ETT_GET_VARIABLE || is_get_element(type)
                || type == ETT_CALL_SETTER || type == ETT_CALL_GETTER) {
            tkz_stack_push(ETT_GET_ELEMENT);
            tkz_stack_push(ETT_VALUE);
            ADVANCE_TO(EJSON_TKZ_STATE_VARIABLE);
        }
    }
    if (character == ':') {
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

        if (!pcejson_inc_depth(parser)) {
            SET_ERR(PCEJSON_ERROR_MAX_DEPTH_EXCEEDED);
            return -1;
        }

        tkz_stack_pop();
        pcejson_token_destroy(top);
        top = tkz_stack_top();
        if (top && top->type == ETT_PROTECT) {
            tkz_stack_pop();
            pcejson_token_destroy(top);

            top = tkz_stack_push(ETT_CJSONEE);
            tkz_stack_push(ETT_VALUE);
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
        else {
            top = tkz_stack_push(ETT_OBJECT);
            RECONSUME_IN(EJSON_TKZ_STATE_BEFORE_NAME);
        }
    }
    if (top->type == ETT_PROTECT) {
        tkz_stack_pop();
        pcejson_token_destroy(top);
        top = tkz_stack_push(ETT_OBJECT);
        RECONSUME_IN(EJSON_TKZ_STATE_BEFORE_NAME);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_RIGHT_BRACE)
    if (is_parse_finished(parser, character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
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
        pcejson_dec_depth(parser);

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
            update_tkz_stack_with_level(parser, 1);
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
        if (!pcejson_inc_depth(parser)) {
            SET_ERR(PCEJSON_ERROR_MAX_DEPTH_EXCEEDED);
            return -1;
        }
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

        if (!pcejson_inc_depth(parser)) {
            SET_ERR(PCEJSON_ERROR_MAX_DEPTH_EXCEEDED);
            return -1;
        }
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
            tkz_stack_push(ETT_GET_ELEMENT_BY_BRACKET);
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
        }

        tkz_stack_push(ETT_GET_ELEMENT_BY_BRACKET);
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
        if ((top->type == ETT_GET_ELEMENT_BY_BRACKET
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
        if (top->type == ETT_GET_ELEMENT_BY_BRACKET
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
    update_tkz_stack(parser);
    RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_LEFT_PARENTHESIS)
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '(') {
        RESET_TEMP_BUFFER();
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_LEFT_PARENTHESIS);
    }
    if (character == '!') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_LEFT_PARENTHESIS);
    }
    if (!pcejson_inc_depth(parser)) {
        SET_ERR(PCEJSON_ERROR_MAX_DEPTH_EXCEEDED);
        return -1;
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
        ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE);
    }
    if (character == '}') {
        RECONSUME_IN(EJSON_TKZ_STATE_RIGHT_BRACE);
    }
    if (character == ']') {
        RECONSUME_IN(EJSON_TKZ_STATE_RIGHT_BRACKET);
    }
    if (character == ')') {
        pcejson_dec_depth(parser);
        RECONSUME_IN(EJSON_TKZ_STATE_RIGHT_PARENTHESIS);
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

            struct pcejson_token *token = tkz_stack_push(ETT_STRING);
            token->node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer));
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

            top = tkz_stack_push(ETT_STRING);
            top->node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer));
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

        top = tkz_stack_push(ETT_MULTI_UNQUOTED_S);

        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            pctree_node_append_child((struct pctree_node*)top->node,
                    (struct pctree_node*)node);
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

            top = tkz_stack_push(ETT_STRING);
            top->node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer));
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

            top = tkz_stack_push(ETT_STRING);
            top->node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer));
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            ADVANCE_TO(EJSON_TKZ_STATE_AFTER_NAME);
        }
        else if (nr_buf_chars == 1) {
             /* K */
            tkz_stack_drop_top();
             /* D */
            tkz_stack_drop_top();
            top = tkz_stack_push(ETT_STRING);
            top->node = pcvcm_node_new_string("");
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

        top = tkz_stack_push(ETT_MULTI_QUOTED_S);

        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            pctree_node_append_child((struct pctree_node*)top->node,
                    (struct pctree_node*)node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(EJSON_TKZ_STATE_NAME_DOUBLE_QUOTED);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_SINGLE_QUOTED)
    if (character == '\'') {
        parser->nr_quoted++;
        size_t nr_buf_chars = tkz_buffer_get_size_in_chars(
                parser->temp_buffer);
        if (parser->nr_quoted > 1 || nr_buf_chars >= 1) {
             /* V */
            tkz_stack_drop_top();
             /* S */
            tkz_stack_drop_top();
            top = tkz_stack_push(ETT_STRING);
            top->node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RESET_QUOTED_COUNTER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        else {
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_SINGLE_QUOTED);
        }
    }
    if (character == '\\') {
        SET_RETURN_STATE(curr_state);
        ADVANCE_TO(EJSON_TKZ_STATE_STRING_ESCAPE);
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(EJSON_TKZ_STATE_VALUE_SINGLE_QUOTED);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED)
    if (character == '"') {
        if (parser->nr_quoted == 0) {
            parser->nr_quoted++;
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
        }
        else if (parser->nr_quoted == 1) {
            RECONSUME_IN(EJSON_TKZ_STATE_VALUE_TWO_DOUBLE_QUOTED);
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
            top = tkz_stack_push(ETT_MULTI_QUOTED_S);
        }
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            if (tkz_buffer_end_with(parser->temp_buffer, "{", 1)) {
                tkz_reader_reconsume_last_char(parser->tkz_reader);
                tkz_reader_reconsume_last_char(parser->tkz_reader);
                DELETE_FROM_RAW_BUFFER(2);
                tkz_buffer_delete_tail_chars(parser->temp_buffer, 1);
                if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                    struct pcvcm_node *node = pcvcm_node_new_string(
                            tkz_buffer_get_bytes(parser->temp_buffer)
                            );
                    pctree_node_append_child((struct pctree_node*)top->node,
                            (struct pctree_node*)node);
                    RESET_TEMP_BUFFER();
                }
            }
            else if (tkz_buffer_end_with(parser->temp_buffer, "{{", 2)) {
                tkz_reader_reconsume_last_char(parser->tkz_reader);
                tkz_reader_reconsume_last_char(parser->tkz_reader);
                tkz_reader_reconsume_last_char(parser->tkz_reader);
                DELETE_FROM_RAW_BUFFER(3);
                tkz_buffer_delete_tail_chars(parser->temp_buffer, 2);
                if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                    struct pcvcm_node *node = pcvcm_node_new_string(
                            tkz_buffer_get_bytes(parser->temp_buffer)
                            );
                    pctree_node_append_child((struct pctree_node*)top->node,
                            (struct pctree_node*)node);
                    RESET_TEMP_BUFFER();
                }
            }
            else {
                tkz_reader_reconsume_last_char(parser->tkz_reader);
                DELETE_FROM_RAW_BUFFER(1);
                struct pcvcm_node *node = pcvcm_node_new_string(
                        tkz_buffer_get_bytes(parser->temp_buffer)
                        );
                pctree_node_append_child((struct pctree_node*)top->node,
                            (struct pctree_node*)node);
                RESET_TEMP_BUFFER();
            }
            RESET_TEMP_BUFFER();
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
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
        top = tkz_stack_push(ETT_STRING);
        top->node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer)
                );
        update_tkz_stack(parser);
        RESET_TEMP_BUFFER();
        RESET_QUOTED_COUNTER();
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_TWO_DOUBLE_QUOTED)
    if (character == '"') {
        if (parser->nr_quoted == 1) {
            parser->nr_quoted++;
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_TWO_DOUBLE_QUOTED);
        }
        else if (parser->nr_quoted == 2) {
            RECONSUME_IN(EJSON_TKZ_STATE_VALUE_THREE_DOUBLE_QUOTED);
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
    }
    else if (top->type == ETT_MULTI_QUOTED_S) {
        struct pcvcm_node *node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer)
                );
        pctree_node_append_child((struct pctree_node*)top->node,
                (struct pctree_node*)node);
        pcejson_token_close(top);
    }
    RESET_TEMP_BUFFER();
    RESET_QUOTED_COUNTER();
    if (is_parse_finished(parser, character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
    }
    RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_THREE_DOUBLE_QUOTED)
    if (character == '\"') {
        parser->nr_quoted++;
        if (parser->nr_quoted > 3) {
            APPEND_TO_TEMP_BUFFER(character);
        }
        if (parser->nr_quoted >= 6
                && tkz_buffer_end_with(parser->temp_buffer,
                    "\"\"\"", 3)) {
            tkz_buffer_delete_tail_chars(parser->temp_buffer, 3);
            /* V */
            tkz_stack_drop_top();
            /* D */
            tkz_stack_drop_top();
            top = tkz_stack_push(ETT_STRING);
            top->node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RESET_QUOTED_COUNTER();
            ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_THREE_DOUBLE_QUOTED);
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(EJSON_TKZ_STATE_VALUE_THREE_DOUBLE_QUOTED);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_KEYWORD)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ','
            || character == ')') {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_KEYWORD);
    }
    if (character == '$' && (parser->flags & PCEJSON_FLAG_GET_VARIABLE)) {
        /* unquoted */
        top = tkz_stack_push(ETT_MULTI_UNQUOTED_S);

        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            pctree_node_append_child((struct pctree_node*)top->node,
                    (struct pctree_node*)node);
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
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        if (tkz_buffer_equal_to(parser->temp_buffer, "false",
                    5)) {
            top->node = pcvcm_node_new_boolean(false);
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        if (tkz_buffer_equal_to(parser->temp_buffer, "null", 4)) {
            top->node = pcvcm_node_new_null();
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        if (tkz_buffer_equal_to(parser->temp_buffer, "undefined", 9)) {
            top->node = pcvcm_node_new_undefined();
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        RESET_TEMP_BUFFER();
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
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
        top = tkz_stack_push(ETT_MULTI_UNQUOTED_S);
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            pctree_node_append_child((struct pctree_node*)top->node,
                    (struct pctree_node*)node);
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
        top = tkz_stack_push(ETT_MULTI_UNQUOTED_S);
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            pctree_node_append_child((struct pctree_node*)top->node,
                    (struct pctree_node*)node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    SET_ERR(PCEJSON_ERROR_BAD_JSON_NUMBER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || is_parse_finished(parser, character)) {
        if (tkz_buffer_end_with(parser->temp_buffer, "-", 1)
            || tkz_buffer_end_with(parser->temp_buffer, "E", 1)
            || tkz_buffer_end_with(parser->temp_buffer, "e", 1)) {
            SET_ERR(PCEJSON_ERROR_BAD_JSON_NUMBER);
            RETURN_AND_STOP_PARSE();
        }
        double d = strtod(
                tkz_buffer_get_bytes(parser->temp_buffer), NULL);
        top->node = pcvcm_node_new_number(d);
        update_tkz_stack(parser);
        RESET_TEMP_BUFFER();
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_NUMBER_INTEGER)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }
    if (is_ascii_digit(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_INTEGER);
    }
    if (character == 'x') {
        if(tkz_buffer_equal_to(parser->temp_buffer, "0", 1)) {
            RESET_TEMP_BUFFER();
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_HEX);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'E' || character == 'e') {
        APPEND_TO_TEMP_BUFFER('e');
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_EXPONENT);
    }
    if (character == '.' || character == 'F') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_FRACTION);
    }
    if (character == 'U' || character == 'L') {
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
    if (parser->is_finished(parser, character)) {
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

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_NUMBER_FRACTION)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || is_eof(character)) {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }

    if (is_ascii_digit(character)) {
        if (tkz_buffer_end_with(parser->temp_buffer, "F", 1)) {
            SET_ERR(PCEJSON_ERROR_BAD_JSON_NUMBER);
            RETURN_AND_STOP_PARSE();
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_FRACTION);
    }
    if (character == 'F') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_FRACTION);
    }
    if (character == 'L') {
        if (tkz_buffer_end_with(parser->temp_buffer, "F", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            long double ld = strtold (
                    tkz_buffer_get_bytes(parser->temp_buffer), NULL);
            top->node = pcvcm_node_new_longdouble(ld);
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE);
        }
    }
    if (character == 'E' || character == 'e') {
        if (tkz_buffer_end_with(parser->temp_buffer, ".", 1)) {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION);
            RETURN_AND_STOP_PARSE();
        }
        APPEND_TO_TEMP_BUFFER('e');
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_EXPONENT);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_NUMBER_EXPONENT)
    if (is_whitespace(character) || character == '}'
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
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }
    if (is_ascii_digit(character)) {
        if (tkz_buffer_end_with(parser->temp_buffer, "F", 1)) {
            SET_ERR(PCEJSON_ERROR_BAD_JSON_NUMBER);
            RETURN_AND_STOP_PARSE();
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_EXPONENT_INTEGER);
    }
    if (character == 'F') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_EXPONENT_INTEGER);
    }
    if (character == 'L') {
        if (tkz_buffer_end_with(parser->temp_buffer, "F", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            long double ld = strtold (
                    tkz_buffer_get_bytes(parser->temp_buffer), NULL);
            top->node = pcvcm_node_new_longdouble(ld);
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
        }
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
    if (character == 'U') {
        if (is_ascii_digit(last_c)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_SUFFIX_INTEGER);
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
                top->node = pcvcm_node_new_ulongint(u64);
                update_tkz_stack(parser);
                RESET_TEMP_BUFFER();
                ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE);
            }
            else if (tkz_buffer_end_with(parser->temp_buffer,
                        "L", 1)) {
                int64_t i64 = strtoll (
                    tkz_buffer_get_bytes(parser->temp_buffer),
                    NULL, 10);
                top->node = pcvcm_node_new_longint(i64);
                update_tkz_stack(parser);
                RESET_TEMP_BUFFER();
                ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE);
            }
        }
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_NUMBER_HEX)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER_HEX);
    }
    if (is_ascii_hex_digit(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_HEX);
    }
    if (character == 'U' || character == 'L') {
        RECONSUME_IN(EJSON_TKZ_STATE_VALUE_NUMBER_HEX_SUFFIX);
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_VALUE_NUMBER_HEX_SUFFIX)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER_HEX);
    }
    uint32_t last_c = tkz_buffer_get_last_char(parser->temp_buffer);
    if (character == 'U') {
        if (is_ascii_hex_digit(last_c)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NUMBER_HEX_SUFFIX);
        }
    }
    if (character == 'L') {
        if (is_ascii_hex_digit(last_c) || last_c == 'U') {
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
        if (tkz_buffer_end_with(parser->temp_buffer, "U", 1)
                || tkz_buffer_end_with(parser->temp_buffer, "UL", 2)
                ) {
            uint64_t u64 = strtoull (bytes, NULL, 16);
            top->node = pcvcm_node_new_ulongint(u64);
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        else {
            int64_t i64 = strtoll (bytes, NULL, 16);
            top->node = pcvcm_node_new_longint(i64);
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
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
        }
        if (tkz_buffer_equal_to(parser->temp_buffer,
                "Infinity", 8)) {
            double d = INFINITY;
            top->node = pcvcm_node_new_number(d);
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
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
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (is_eof(character)) {
        ADVANCE_TO(EJSON_TKZ_STATE_AFTER_VALUE_NUMBER);
    }
    if (parser->is_finished(parser, character)) {
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
            update_tkz_stack(parser);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VALUE);
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
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (character == 'a') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "N", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(EJSON_TKZ_STATE_VALUE_NAN);
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
            APPEND_BYTES_TO_TEMP_BUFFER("\\u", 2);
            APPEND_BUFFER_TO_TEMP_BUFFER(parser->string_buffer);
            RESET_STRING_BUFFER();
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
    tkz_reader_reconsume_last_char(parser->tkz_reader);
    tkz_reader_reconsume_last_char(parser->tkz_reader);
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
            top = tkz_stack_push(ETT_MULTI_UNQUOTED_S);
            pctree_node_append_child((struct pctree_node*)top->node,
                    (struct pctree_node*)token->node);
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
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
        ADVANCE_TO(EJSON_TKZ_STATE_CJSONEE_FINISHED);
    }
    if (tkz_buffer_equal_to(parser->temp_buffer, "}}", 2)) {
        update_tkz_stack(parser);
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
                    pctree_node_append_child((struct pctree_node*)top->node,
                            (struct pctree_node*)node);
                }
                else {
                    top->node = pcvcm_node_new_string(
                            tkz_buffer_get_bytes(parser->temp_buffer)
                            );
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
                    pctree_node_append_child((struct pctree_node*)top->node,
                            (struct pctree_node*)node);
                }
                else {
                    top->node = pcvcm_node_new_string(
                            tkz_buffer_get_bytes(parser->temp_buffer)
                            );
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
            top = tkz_stack_push(ETT_MULTI_UNQUOTED_S);
        }

        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node *node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            pctree_node_append_child((struct pctree_node*)top->node,
                    (struct pctree_node*)node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    if (character == '\\') {
        SET_RETURN_STATE(curr_state);
        ADVANCE_TO(EJSON_TKZ_STATE_STRING_ESCAPE);
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
            if (prev && (prev->type == ETT_GET_ELEMENT)) {
                SET_ERR(PCEJSON_ERROR_BAD_JSONEE_KEYWORD);
                RETURN_AND_STOP_PARSE();
            }
        }
        top->node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer)
                );
        RESET_TEMP_BUFFER();
    }
    RECONSUME_IN(EJSON_TKZ_STATE_AFTER_VARIABLE);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_AFTER_VARIABLE)
    if (character == '}') {
        update_tkz_stack(parser);
        top = tkz_stack_top();
        if (top->type == ETT_GET_ELEMENT ||
                top->type == ETT_GET_ELEMENT_BY_BRACKET ||
                top->type == ETT_GET_VARIABLE) {
            update_tkz_stack(parser);
        }
        top = tkz_stack_top();
        if (top->type == ETT_MULTI_QUOTED_S ||
                top->type == ETT_MULTI_UNQUOTED_S) {
            pcejson_token_close(top);
            update_tkz_stack(parser);
        }
        RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
    }
    else if (character == '.' || character == '(' || character == '[') {
        update_tkz_stack_with_level(parser, 1);
    }
    else if (character == '$' && (parser->flags & PCEJSON_FLAG_GET_VARIABLE)) {
        top = tkz_stack_top();
        if (top->type == ETT_VALUE && top->node) {
            struct pcejson_token *token = tkz_stack_pop();

            top = tkz_stack_top();
            if (top->type == ETT_GET_ELEMENT ||
                    top->type == ETT_GET_ELEMENT_BY_BRACKET ||
                    top->type == ETT_GET_VARIABLE) {
                pctree_node_append_child((struct pctree_node*)top->node,
                        (struct pctree_node*)token->node);
                token->node = NULL;
                pcejson_token_destroy(token);
                pcejson_token_close(top);
                update_tkz_stack(parser);
            }
            else {
                if ((top->type != ETT_MULTI_QUOTED_S) &&
                        (top->type != ETT_MULTI_UNQUOTED_S)) {
                    top = tkz_stack_push(ETT_MULTI_UNQUOTED_S);
                }
                pctree_node_append_child((struct pctree_node*)top->node,
                        (struct pctree_node*)token->node);
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
                    pcejson_token_close(token);
                    struct pcejson_token *token = tkz_stack_pop();
                    top = tkz_stack_push(ETT_MULTI_UNQUOTED_S);

                    pctree_node_append_child((struct pctree_node*)top->node,
                            (struct pctree_node*)token->node);
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
        if (top->type == ETT_GET_ELEMENT ||
                top->type == ETT_GET_ELEMENT_BY_BRACKET ||
                top->type == ETT_GET_VARIABLE) {
            update_tkz_stack(parser);
        }
        top = tkz_stack_top();
        if (top->type == ETT_MULTI_QUOTED_S) {
            pcejson_token_close(top);
            update_tkz_stack(parser);
            if (is_parse_finished(parser, character)) {
                ADVANCE_TO(EJSON_TKZ_STATE_FINISHED);
            }
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
        if (top->type == ETT_MULTI_UNQUOTED_S) {
            //FIXME:
            struct pcvcm_node *node = pcvcm_node_new_string("\"");
            pctree_node_append_child((struct pctree_node*)top->node,
                    (struct pctree_node*)node);
            update_tkz_stack(parser);
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
    }
    else {
        update_tkz_stack(parser);
        top = tkz_stack_top();
        if (top->type == ETT_GET_ELEMENT ||
                top->type == ETT_GET_ELEMENT_BY_BRACKET ||
                top->type == ETT_GET_VARIABLE) {
            update_tkz_stack(parser);
        }
        top = tkz_stack_top();
        if (is_parse_finished(parser, character)) {
            if (top->type == ETT_MULTI_UNQUOTED_S) {
                pcejson_token_close(top);
            }
            update_tkz_stack(parser);
            RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
        }
        top = tkz_stack_top();
        if (top->type == ETT_MULTI_QUOTED_S) {
            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_VALUE_DOUBLE_QUOTED);
        }
        if (top->type == ETT_MULTI_UNQUOTED_S) {
            RESET_TEMP_BUFFER();
            tkz_stack_push(ETT_VALUE);
            RECONSUME_IN(EJSON_TKZ_STATE_RAW_STRING);
        }
        if (is_parse_finished(parser, character)) {
            RECONSUME_IN(EJSON_TKZ_STATE_FINISHED);
        }
        size_t nr = tkz_stack_size();
        if (nr == 1 && pcejson_token_is_closed(top)) {
            struct pcejson_token *token = tkz_stack_pop();
            top = tkz_stack_push(ETT_MULTI_UNQUOTED_S);

            pctree_node_append_child((struct pctree_node*)top->node,
                    (struct pctree_node*)token->node);
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
            || parser->is_finished(parser, character)) {
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
        pctree_node_append_child((struct pctree_node*)top->node,
                (struct pctree_node*)node);
        RESET_TEMP_BUFFER();
        ADVANCE_TO(EJSON_TKZ_STATE_BACKQUOTE_CONTENT);
    }

    if (character == '`' || is_eof(character)
            || parser->is_finished(parser, character)) {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            const char *buf = tkz_buffer_get_bytes(parser->temp_buffer);
            purc_atom_t t = purc_atom_try_string_ex(ATOM_BUCKET_EXCEPT, buf);
            if (t == 0) {
                SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
                RETURN_AND_STOP_PARSE();
            }
            struct pcvcm_node *node = pcvcm_node_new_ulongint(t);
            pctree_node_append_child((struct pctree_node*)top->node,
                    (struct pctree_node*)node);
            RESET_TEMP_BUFFER();
        }
        pcejson_token_close(top);
        update_tkz_stack(parser);
        ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
    }

    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(EJSON_TKZ_STATE_BACKQUOTE_CONTENT);
END_STATE()

PCEJSON_PARSER_END

