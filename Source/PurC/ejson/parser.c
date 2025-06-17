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
    parser->temp_ucs = tkz_ucs_new();
    parser->tkz_stack = pcejson_token_stack_new();
    parser->prev_separator = 0;
    parser->nr_single_quoted = 0;
    parser->nr_double_quoted = 0;
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
        tkz_ucs_destroy(parser->temp_ucs);
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
    tkz_ucs_reset(parser->temp_ucs);

    pcejson_token_stack_destroy(parser->tkz_stack);
    parser->tkz_stack = pcejson_token_stack_new();
    parser->prev_separator = 0;
    parser->nr_single_quoted = 0;
    parser->nr_double_quoted = 0;
    parser->finished_by_callback = 0;
    parser->record_ucs = 0;
    parser->hvml_double_quoted_attr_value = 0;
}

void pcejson_update_state_to_parse_double_quoted_attr_value(struct pcejson* parser)
{
    parser->state = EJSON_TKZ_STATE_ATTR_VALUE;
    parser->hvml_double_quoted_attr_value = 1;
}

static bool
is_finished_default(struct pcejson *parser, uint32_t character)
{
    UNUSED_PARAM(parser);
    UNUSED_PARAM(character);
    return false;
}

#define EJSON_PARSER_LC_SIZE         3

int pcejson_parse_ex (struct pcvcm_node** vcm_tree, struct pcejson** parser_param,
                   purc_rwstream_t rws, uint32_t depth,
                   pcejson_parse_is_finished_fn is_finished)
{
    int ret;
    struct tkz_reader *reader = tkz_reader_new();
    if (!reader) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        ret = -1;
        goto out;
    }
    struct tkz_lc *lc = tkz_lc_new(EJSON_PARSER_LC_SIZE);
    if (!lc) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        ret = -1;
        goto out_clear_reader;
    }

    tkz_reader_set_data_source_rws(reader, rws);
    tkz_reader_set_lc(reader, lc);
    ret = pcejson_parse_full(vcm_tree, parser_param, reader, depth,
            is_finished);
    tkz_lc_destroy(lc);

out_clear_reader:
    tkz_reader_destroy(reader);

out:
    return ret;
}

int pcejson_parse (struct pcvcm_node** vcm_tree, struct pcejson** parser,
                   purc_rwstream_t rws, uint32_t depth)
{
    return pcejson_parse_ex(vcm_tree, parser, rws, depth, is_finished_default);
}

int pcejson_set_state(struct pcejson *parser, int state)
{
    if (parser) {
        parser->state = state;
    }
    return 0;
}

int pcejson_set_state_param_string(struct pcejson *parser)
{
    return pcejson_set_state(parser, EJSON_TKZ_STATE_PARAM_STRING);
}

bool
pcejson_is_finished_stream(struct pcejson *parser, uint32_t character)
{
    (void) character;
    if (1 == pcejson_token_stack_size(parser->tkz_stack)) {
        struct pcejson_token *top = pcejson_token_stack_top(parser->tkz_stack);
        bool is_closed = pcejson_token_is_closed(top);
        bool finished = is_closed;
        return finished;
    }
    return false;
}
