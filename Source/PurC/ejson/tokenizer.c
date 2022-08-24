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

#define tkz_stack_is_empty()  pcejson_token_stack_is_empty(parser->tkz_stack)
#define tkz_stack_top()  pcejson_token_stack_top(parser->tkz_stack)
#define tkz_stack_pop()  pcejson_token_stack_pop(parser->tkz_stack)
#define tkz_stack_push(c, node) \
    pcejson_token_stack_push_simple(parser->tkz_stack, c, node)
#define tkz_stack_size() pcejson_token_stack_size(parser->tkz_stack)
#define tkz_stack_reset() pcejson_token_stack_clear(parser->tkz_stack)

#define tkz_current()  pcejson_token_stack_top(parser->tkz_stack)

#define PRINT_STATE(state_name)

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

static int
update_tkz_stack(struct pcejson *parser)
{
    int ret = 0;
    int nr = tkz_stack_size();
    if (nr <= 1) {
        goto out;
    }

    struct pcejson_token *token = tkz_stack_pop();
    if (!pcvcm_node_is_closed(token->node)) {
        goto out;
    }

    struct pcejson_token *parent = tkz_stack_top();
    switch (parent->type) {
    case '{':
    case '[':
        pctree_node_append_child((struct pctree_node*)parent->node,
                (struct pctree_node*)token->node);
        pcvcm_node_set_closed(parent->node, true);
        token->node = NULL;
        pcejson_token_destroy(token);
        break;
    case '$':
        while (parent && parent->type == '$') {
            pctree_node_append_child((struct pctree_node*)parent->node,
                    (struct pctree_node*)token->node);
            pcvcm_node_set_closed(parent->node, true);
            token->node = NULL;
            pcejson_token_destroy(token);
            token = tkz_stack_pop();
            parent = tkz_stack_top();
        }
        break;
    case '.':
    case '(':
    case '<':
        pctree_node_append_child((struct pctree_node*)parent->node,
                (struct pctree_node*)token->node);
        pcvcm_node_set_closed(parent->node, true);
        token->node = NULL;
        pcejson_token_destroy(token);
        break;
    }

out:
    return ret;
}

static int
build_jsonee(struct pcejson *parser)
{
    struct pcejson_token *token = NULL;
    struct pcejson_token *parent = NULL;
    int nr = tkz_stack_size();
    if (nr == 0) {
        goto out;
    }
    else if (nr == 1) {
        token = tkz_stack_pop();
        parser->vcm_node = token->node;
        token->node = NULL;
        pcejson_token_destroy(token);
        goto out;
    }

    update_tkz_stack(parser);

    token = tkz_stack_pop();
    parent = tkz_stack_pop();
    while (parent) {
        struct pcvcm_node *p_node = parent->node;
        if (pcvcm_node_is_closed(p_node)) {
            break;
        }
    }

out:
    return 0;
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
    return ret;
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_CONTROL)
    if (is_whitespace(character)) {
        ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
    }
    if (character == '{') {
        RECONSUME_IN(EJSON_TKZ_STATE_LEFT_BRACE);
    }
    if (character == '}') {
        RECONSUME_IN(EJSON_TKZ_STATE_RIGHT_BRACE);
    }
    if (character == '[') {
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
    if (character == '$') {
        RECONSUME_IN(EJSON_TKZ_STATE_DOLLAR);
    }
    if (character == '&') {
        RECONSUME_IN(EJSON_TKZ_STATE_AMPERSAND);
    }
    if (character == '|') {
        RECONSUME_IN(EJSON_TKZ_STATE_OR_SIGN);
    }
    if (character == ';') {
        RECONSUME_IN(EJSON_TKZ_STATE_SEMICOLON);
    }
    if (character == '\'') {
        RECONSUME_IN(EJSON_TKZ_STATE_SINGLE_QUOTED);
    }
    if (character == '"') {
        RECONSUME_IN(EJSON_TKZ_STATE_DOUBLE_QUOTED);
    }
    RECONSUME_IN(EJSON_TKZ_STATE_UNQUOTED);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_AFTER_TOKEN)
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_LEFT_BRACE)
    if (character == '{') {
        tkz_stack_push('P', NULL);
        ADVANCE_TO(EJSON_TKZ_STATE_LEFT_BRACE);
    }
    if (character == '$') {
        RECONSUME_IN(EJSON_TKZ_STATE_DOLLAR);
    }
    struct pcejson_token *top = tkz_stack_top();
    if (is_whitespace(character)) {
        if (top->type != 'P') {
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
        if (top && top->type == 'P') {
            tkz_stack_pop();
            pcejson_token_destroy(top);

            top = tkz_stack_push('C', pcvcm_node_new_cjsonee());
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
        else {
            top = tkz_stack_push('{', pcvcm_node_new_object(0, NULL));
            RECONSUME_IN(EJSON_TKZ_STATE_BEFORE_NAME);
        }
    }
    tkz_stack_pop();
    pcejson_token_destroy(top);
    top = tkz_stack_push('{', pcvcm_node_new_object(0, NULL));
    RECONSUME_IN(EJSON_TKZ_STATE_BEFORE_NAME);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_RIGHT_BRACE)
    struct pcejson_token *top = tkz_stack_top();
    if (!top) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    uint32_t uc = top->type;
    if (character == '}') {
        pcejson_dec_depth(parser);
        if (uc == 'C') {
            RESET_TEMP_BUFFER();
            RECONSUME_IN(EJSON_TKZ_STATE_CJSONEE_FINISHED);
        }
        if (uc == '{') {
            tkz_stack_pop();
            pcejson_token_destroy(top);

            update_tkz_stack(parser);
            ADVANCE_TO(EJSON_TKZ_STATE_AFTER_TOKEN);
        }
        else if (uc == 'P') {
            tkz_stack_pop();
            pcejson_token_destroy(top);
            if (parser->vcm_node->extra & EXTRA_PROTECT_FLAG) {
                parser->vcm_node->extra &= EXTRA_SUGAR_FLAG;
            }
            else {
                parser->vcm_node->extra &= EXTRA_PROTECT_FLAG;
            }

            update_tkz_stack(parser);
            ADVANCE_TO(EJSON_TKZ_STATE_RIGHT_BRACE);
        }
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_LEFT_BRACKET)
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }

    if (character == '[') {
        struct pcejson_token *top = tkz_stack_top();
        if (!pcejson_inc_depth(parser)) {
            SET_ERR(PCEJSON_ERROR_MAX_DEPTH_EXCEEDED);
            return -1;
        }
        if (top == NULL) {
            top = tkz_stack_push('[', pcvcm_node_new_array(0, NULL));
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
        uint32_t uc = top->type;
        if (uc == '(' || uc == '<' || uc == '[' || uc == ':' || uc == '"') {
            top = tkz_stack_push('[', pcvcm_node_new_array(0, NULL));
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }

        update_tkz_stack(parser);
        top = tkz_stack_push('.', pcvcm_node_new_get_element(NULL, NULL));
        ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
    }

    SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_RIGHT_BRACKET)
    if (is_whitespace(character)) {
        ADVANCE_TO(EJSON_TKZ_STATE_RIGHT_BRACKET);
    }
    if (is_eof(character)) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_EOF);
        RETURN_AND_STOP_PARSE();
    }
    if (character == ']') {
        struct pcejson_token *top = tkz_stack_top();
        if (top == NULL) {
            SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
        uint32_t uc = top->type;
        if (uc == '.' || uc == '[') {
            pcvcm_node_set_closed(top->node, true);
            update_tkz_stack(parser);
            ADVANCE_TO(EJSON_TKZ_STATE_AFTER_TOKEN);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_RIGHT_BRACKET);
        RETURN_AND_STOP_PARSE();
    }
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
    update_tkz_stack(parser);
    if (tkz_buffer_equal_to(parser->temp_buffer, "(!", 2)) {
        tkz_stack_push('<', pcvcm_node_new_call_setter(NULL, 0, NULL));
    }
    else if (tkz_buffer_equal_to(parser->temp_buffer, "(", 1)) {
        tkz_stack_push('(', pcvcm_node_new_call_getter(NULL, 0, NULL));
    }
    RECONSUME_IN(EJSON_TKZ_STATE_CONTROL);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_RIGHT_PARENTHESIS)
    struct pcejson_token *top = tkz_stack_top();
    if (top == NULL) {
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == ')') {
        uint32_t uc = top->type;

        if (uc == '(' || uc == '<') {
            pcvcm_node_set_closed(top->node, true);
            update_tkz_stack(parser);
            ADVANCE_TO(EJSON_TKZ_STATE_CONTROL);
        }
        SET_ERR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
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
        tkz_stack_push('$', pcvcm_node_new_get_variable(NULL));
        ADVANCE_TO(EJSON_TKZ_STATE_DOLLAR);
    }
    if (character == '{') {
        tkz_stack_push('P', NULL);
        RESET_TEMP_BUFFER();
        ADVANCE_TO(EJSON_TKZ_STATE_JSONEE_VARIABLE);
    }
    RESET_TEMP_BUFFER();
    RECONSUME_IN(EJSON_TKZ_STATE_JSONEE_VARIABLE);
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_AMPERSAND)
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_OR_SIGN)
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_SEMICOLON)
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_SINGLE_QUOTED)
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_DOUBLE_QUOTED)
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_UNQUOTED)
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_BEFORE_NAME)
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_JSONEE_VARIABLE)
END_STATE()

BEGIN_STATE(EJSON_TKZ_STATE_CJSONEE_FINISHED)
END_STATE()

PCEJSON_PARSER_END

