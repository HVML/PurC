/*
 * @file ejson.c
 * @author XueShuming
 * @date 2021/07/19
 * @brief The impl for eJSON parser
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
 */

#include "private/ejson.h"
#include "private/errors.h"
#include "purc-utils.h"
#include "config.h"

#if HAVE(GLIB)
#include <gmodule.h>
#endif

#define MIN_STACK_CAPACITY 32
#define MIN_EJSON_BUFFER_SIZE 128
#define MAX_EJSON_BUFFER_SIZE 1024 * 1024 * 1024

#if HAVE(GLIB)
#define    ejson_alloc(sz)   g_slice_alloc0(sz)
#define    ejson_free(p)     g_slice_free1(sizeof(*p), (gpointer)p)
#else
#define    ejson_alloc(sz)   calloc(1, sz)
#define    ejson_free(p)     free(p)
#endif

static size_t get_stack_size(size_t sz_stack) {
    size_t stack = pcutils_get_next_fibonacci_number(sz_stack);
    return stack < MIN_STACK_CAPACITY ? MIN_STACK_CAPACITY : stack;
}

struct pcejson_stack* pcejson_stack_new(size_t sz_init)
{
    struct pcejson_stack* stack = (struct pcejson_stack*) ejson_alloc(
            sizeof(struct pcejson_stack));
    sz_init = get_stack_size(sz_init);
    stack->buf = (uint8_t*) calloc (1, sz_init);
    stack->last = -1;
    stack->capacity = sz_init;
    return stack;
}

bool pcejson_stack_is_empty(struct pcejson_stack* stack)
{
    return stack->last == -1;
}

void pcejson_stack_push(struct pcejson_stack* stack, uint8_t c)
{
    if (stack->last == (int32_t)(stack->capacity - 1))
    {
        size_t sz = get_stack_size(stack->capacity);
        uint8_t* newbuf = (uint8_t*) realloc(stack->buf, sz);
        if (newbuf == NULL)
        {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return;
        }
        stack->capacity = sz;
    }
    stack->buf[++stack->last] = c;
}

uint8_t pcejson_stack_pop(struct pcejson_stack* stack)
{
    if (pcejson_stack_is_empty(stack))
    {
        return -1;
    }
    return stack->buf[stack->last--];
}

uint8_t pcejson_stack_first(struct pcejson_stack* stack)
{
    if (pcejson_stack_is_empty(stack))
    {
        return -1;
    }
    return stack->buf[0];
}

uint8_t pcejson_stack_last(struct pcejson_stack* stack)
{
    if (pcejson_stack_is_empty(stack)) {
        return -1;
    }
    return stack->buf[stack->last];
}

void pcejson_stack_destroy(struct pcejson_stack* stack)
{
    if (stack) {
        free(stack->buf);
        stack->buf = NULL;
        stack->last = -1;
        stack->capacity = 0;
        ejson_free(stack);
    }
}

struct pcejson* pcejson_create(int32_t depth, uint32_t flags)
{
    struct pcejson* parser = (struct pcejson*)ejson_alloc(sizeof(struct pcejson));
    parser->state = ejson_init_state;
    parser->depth = depth;
    parser->flags = flags;
    parser->stack = pcejson_stack_new(2 * depth);
    parser->rws = purc_rwstream_new_buffer(MIN_EJSON_BUFFER_SIZE,
            MAX_EJSON_BUFFER_SIZE);
    return parser;
}

void pcejson_destroy(struct pcejson* parser)
{
    if (parser) {
        pcejson_stack_destroy(parser->stack);
        purc_rwstream_destroy(parser->rws);
        ejson_free(parser);
    }
}

void pcejson_reset_temp_buffer(struct pcejson* parser)
{
    size_t sz = 0;
    const char* p = purc_rwstream_get_mem_buffer (parser->rws, &sz);
    memset((void*)p, 0, sz);
    purc_rwstream_seek(parser->rws, 0, SEEK_SET);
}

void pcejson_reset(struct pcejson* parser, int32_t depth, uint32_t flags)
{
    parser->state = ejson_init_state;
    parser->depth = depth;
    parser->flags = flags;
    pcejson_reset_temp_buffer(parser);
}

// TODO
int pcejson_parse(pcvcm_tree_t vcm_tree, purc_rwstream_t rwstream)
{
    UNUSED_PARAM(vcm_tree);
    UNUSED_PARAM(rwstream);
    pcinst_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return -1;
}

// eJSON tokenizer
struct pcejson_token* pcejson_token_new(enum ejson_token_type type,
        size_t sz_min, size_t sz_max)
{
    struct pcejson_token* token = ejson_alloc(sizeof(struct pcejson_token));
    token->type = type;
    token->rws = purc_rwstream_new_buffer(sz_min, sz_max);
    return token;
}

void pcejson_token_destroy(struct pcejson_token* token)
{
    if (token && token->rws) {
        purc_rwstream_destroy(token->rws);
    }
    ejson_free(token);
}

#define    END_OF_FILE_MARKER     0

struct pcejson_token* pcejson_next_token(struct pcejson* ejson, purc_rwstream_t rws)
{
    char buf_utf8[8] = {0};
    wchar_t wc = 0;

    int ret = purc_rwstream_read_utf8_char (rws, buf_utf8, &wc);
    if (ret <= 0) {
        return NULL;
    }

    switch (ejson->state) {

        BEGIN_STATE(ejson_init_state)
            switch (wc) {
                case ' ':
                case '\x0A':
                case '\x09':
                case '\x0C':
                    ADVANCE_TO(ejson_init_state);
                    break;
                case '{':
                    RECONSUME_IN(ejson_object_state);
                    break;
                case '[':
                    RECONSUME_IN(ejson_object_state);
                    break;
                case END_OF_FILE_MARKER:
                    return pcejson_token_new(ejson_token_eof, 0, 0);
                default:
                    pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                    return NULL;
            }
        END_STATE()

        BEGIN_STATE(ejson_finished_state)
            switch (wc) {
                case ' ':
                case '\x0A':
                case '\x09':
                case '\x0C':
                    ADVANCE_TO(ejson_finished_state);
                    break;
                case END_OF_FILE_MARKER:
                    return pcejson_token_new(ejson_token_eof, 0, 0);
                default:
                    pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                    return NULL;
            }
        END_STATE()

        BEGIN_STATE(ejson_object_state)
            switch (wc) {
                case ' ':
                case '\x0A':
                case '\x09':
                case '\x0C':
                    ADVANCE_TO(ejson_before_name_state);
                    break;
                case '{':
                    pcejson_stack_push (ejson->stack, '{');
                    pcejson_reset_temp_buffer(ejson);
                    return pcejson_token_new(ejson_token_start_object, 0, 0);
                default:
                    pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                    return NULL;
            }
        END_STATE()

        BEGIN_STATE(ejson_after_object_state)
            if (wc == '}') {
                uint8_t c = pcejson_stack_last(ejson->stack);
                if (c == '{') {
                    pcejson_stack_pop(ejson->stack);
                    // TODO: define macro
                    if (pcejson_stack_is_empty(ejson->stack)) {
                        ejson->state = ejson_finished_state;
                    }
                    else {
                        ejson->state = ejson_init_state;
                    }
                    return pcejson_token_new(ejson_token_end_object, 0, 0);
                }
            }
            else {
                pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                return NULL;
            }
        END_STATE()

        BEGIN_STATE(ejson_array_state)
        END_STATE()

        BEGIN_STATE(ejson_after_array_state)
        END_STATE()

        BEGIN_STATE(ejson_before_name_state)
        END_STATE()

        BEGIN_STATE(ejson_after_name_state)
        END_STATE()

        BEGIN_STATE(ejson_before_value_state)
        END_STATE()

        BEGIN_STATE(ejson_after_value_state)
        END_STATE()

        BEGIN_STATE(ejson_name_unquoted_state)
        END_STATE()

        BEGIN_STATE(ejson_after_name_unquoted_state)
        END_STATE()

        BEGIN_STATE(ejson_name_single_quoted_state)
        END_STATE()

        BEGIN_STATE(ejson_after_name_single_quoted_state)
        END_STATE()

        BEGIN_STATE(ejson_name_double_quoted_state)
        END_STATE()

        BEGIN_STATE(ejson_after_name_double_quoted_state)
        END_STATE()

        BEGIN_STATE(ejson_value_single_quoted_state)
        END_STATE()

        BEGIN_STATE(ejson_after_value_single_quoted_state)
        END_STATE()

        BEGIN_STATE(ejson_value_double_quoted_state)
        END_STATE()

        BEGIN_STATE(ejson_after_value_double_quoted_state)
        END_STATE()

        BEGIN_STATE(ejson_value_two_double_quoted_state)
        END_STATE()

        BEGIN_STATE(ejson_after_value_two_double_quoted_state)
        END_STATE()

        BEGIN_STATE(ejson_value_three_double_quoted_state)
        END_STATE()

        BEGIN_STATE(ejson_after_value_three_double_quoted_state)
        END_STATE()

        BEGIN_STATE(ejson_keyword_state)
        END_STATE()

        BEGIN_STATE(ejson_after_keyword_state)
        END_STATE()

        BEGIN_STATE(ejson_byte_sequence_state)
        END_STATE()

        BEGIN_STATE(ejson_after_byte_sequence_state)
        END_STATE()

        BEGIN_STATE(ejson_hex_byte_sequence_state)
        END_STATE()

        BEGIN_STATE(ejson_binary_byte_sequence_state)
        END_STATE()

        BEGIN_STATE(ejson_base64_byte_sequence_state)
        END_STATE()

        BEGIN_STATE(ejson_value_number_state)
        END_STATE()

        BEGIN_STATE(ejson_after_value_number_state)
        END_STATE()

        BEGIN_STATE(ejson_value_number_integer_state)
        END_STATE()

        BEGIN_STATE(ejson_value_number_fraction_state)
        END_STATE()

        BEGIN_STATE(ejson_after_value_number_fraction_state)
        END_STATE()

        BEGIN_STATE(ejson_value_number_exponent_state)
        END_STATE()

        BEGIN_STATE(ejson_value_number_exponent_integer_state)
        END_STATE()

        BEGIN_STATE(ejson_after_value_number_exponent_integer_state)
        END_STATE()

        BEGIN_STATE(ejson_value_number_suffix_integer_state)
        END_STATE()

        BEGIN_STATE(ejson_after_value_number_suffix_integer_state)
        END_STATE()

        BEGIN_STATE(ejson_string_escape_state)
        END_STATE()

        BEGIN_STATE(ejson_string_escape_four_hexadecimal_digits_state)
        END_STATE()

        default:
            break;
    }
    UNUSED_LABEL(ejson_init_state);
    UNUSED_LABEL(ejson_finished_state);
    UNUSED_LABEL(ejson_object_state);
    UNUSED_LABEL(ejson_after_object_state);
    UNUSED_LABEL(ejson_array_state);
    UNUSED_LABEL(ejson_after_array_state);
    UNUSED_LABEL(ejson_before_name_state);
    UNUSED_LABEL(ejson_after_name_state);
    UNUSED_LABEL(ejson_before_value_state);
    UNUSED_LABEL(ejson_after_value_state);
    UNUSED_LABEL(ejson_name_unquoted_state);
    UNUSED_LABEL(ejson_after_name_unquoted_state);
    UNUSED_LABEL(ejson_name_single_quoted_state);
    UNUSED_LABEL(ejson_after_name_single_quoted_state);
    UNUSED_LABEL(ejson_name_double_quoted_state);
    UNUSED_LABEL(ejson_after_name_double_quoted_state);
    UNUSED_LABEL(ejson_value_single_quoted_state);
    UNUSED_LABEL(ejson_after_value_single_quoted_state);
    UNUSED_LABEL(ejson_value_double_quoted_state);
    UNUSED_LABEL(ejson_after_value_double_quoted_state);
    UNUSED_LABEL(ejson_value_two_double_quoted_state);
    UNUSED_LABEL(ejson_after_value_two_double_quoted_state);
    UNUSED_LABEL(ejson_value_three_double_quoted_state);
    UNUSED_LABEL(ejson_after_value_three_double_quoted_state);
    UNUSED_LABEL(ejson_keyword_state);
    UNUSED_LABEL(ejson_after_keyword_state);
    UNUSED_LABEL(ejson_byte_sequence_state);
    UNUSED_LABEL(ejson_after_byte_sequence_state);
    UNUSED_LABEL(ejson_hex_byte_sequence_state);
    UNUSED_LABEL(ejson_binary_byte_sequence_state);
    UNUSED_LABEL(ejson_base64_byte_sequence_state);
    UNUSED_LABEL(ejson_value_number_state);
    UNUSED_LABEL(ejson_after_value_number_state);
    UNUSED_LABEL(ejson_value_number_integer_state);
    UNUSED_LABEL(ejson_value_number_fraction_state);
    UNUSED_LABEL(ejson_after_value_number_fraction_state);
    UNUSED_LABEL(ejson_value_number_exponent_state);
    UNUSED_LABEL(ejson_value_number_exponent_integer_state);
    UNUSED_LABEL(ejson_after_value_number_exponent_integer_state);
    UNUSED_LABEL(ejson_value_number_suffix_integer_state);
    UNUSED_LABEL(ejson_after_value_number_suffix_integer_state);
    UNUSED_LABEL(ejson_string_escape_state);
    UNUSED_LABEL(ejson_string_escape_four_hexadecimal_digits_state);
    return NULL;
}
