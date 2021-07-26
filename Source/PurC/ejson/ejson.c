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

static inline bool is_whitespace(wchar_t character)
{
    return character == ' ' || character == '\x0A'
        || character == '\x09' || character == '\x0C';
}

static inline wchar_t to_ascii_lower_unchecked(wchar_t character)
{
        return character | 0x20;
}

static inline bool is_ascii(wchar_t character)
{
    return !(character & ~0x7F);
}

static inline bool is_ascii_lower(wchar_t character)
{
    return character >= 'a' && character <= 'z';
}

static inline bool is_ascii_upper(wchar_t character)
{
     return character >= 'A' && character <= 'Z';
}

static inline bool is_ascii_space(wchar_t character)
{
    return character <= ' ' &&
        (character == ' ' || (character <= 0xD && character >= 0x9));
}

static inline bool is_ascii_digit(wchar_t character)
{
    return character >= '0' && character <= '9';
}

static inline bool is_ascii_binary_digit(wchar_t character)
{
     return character == '0' || character == '1';
}

static inline bool is_ascii_hex_digit(wchar_t character)
{
     return is_ascii_digit(character) ||
         (to_ascii_lower_unchecked(character) >= 'a'
          && to_ascii_lower_unchecked(character) <= 'f');
}

static inline bool is_ascii_octal_digit(wchar_t character)
{
     return character >= '0' && character <= '7';
}

static inline bool is_ascii_alpha(wchar_t character)
{
    return is_ascii_lower(to_ascii_lower_unchecked(character));
}

static inline bool is_ascii_alpha_numeric(wchar_t character)
{
    return is_ascii_digit(character) || is_ascii_alpha(character);
}

static inline bool is_delimiter(wchar_t c)
{
    return is_whitespace(c) || c == '}' || c == ']' || c == ',';
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
    parser->rws2 = purc_rwstream_new_buffer(MIN_EJSON_BUFFER_SIZE,
            MAX_EJSON_BUFFER_SIZE);
    return parser;
}

void pcejson_destroy(struct pcejson* parser)
{
    if (parser) {
        pcejson_stack_destroy(parser->stack);
        purc_rwstream_destroy(parser->rws);
        purc_rwstream_destroy(parser->rws2);
        ejson_free(parser);
    }
}

void pcejson_temp_buffer_reset(struct pcejson* parser)
{
    size_t sz = 0;
    const char* p = purc_rwstream_get_mem_buffer (parser->rws, &sz);
    memset((void*)p, 0, sz);
    purc_rwstream_seek(parser->rws, 0, SEEK_SET);
}

void pcejson_temp_buffer_reset2(struct pcejson* parser)
{
    size_t sz = 0;
    const char* p = purc_rwstream_get_mem_buffer (parser->rws2, &sz);
    memset((void*)p, 0, sz);
    purc_rwstream_seek(parser->rws2, 0, SEEK_SET);
}

char* pcejson_temp_buffer_dup(struct pcejson* parser)
{
    size_t sz = 0;
    const char* p = purc_rwstream_get_mem_buffer (parser->rws, &sz);
    char* dup = (char*)malloc(sz + 1);
    strncpy(dup, p, sz);
    dup[sz] = 0;
    return dup;
}

bool pcejson_temp_buffer_is_empty(struct pcejson* parser)
{
    return (0 == purc_rwstream_tell(parser->rws));
}

ssize_t pcejson_temp_buffer_append(struct pcejson* parser, uint8_t* buf,
        size_t sz)
{
    return purc_rwstream_write (parser->rws, buf, sz);
}

ssize_t pcejson_temp_buffer_append2(struct pcejson* parser, uint8_t* buf,
        size_t sz)
{
    return purc_rwstream_write (parser->rws2, buf, sz);
}

size_t pcejson_temp_buffer_length(struct pcejson* parser)
{
    return purc_rwstream_tell (parser->rws);
}

size_t pcejson_temp_buffer_length2(struct pcejson* parser)
{
    return purc_rwstream_tell (parser->rws2);
}

void pcejson_temp_buffer_clear_head_tail_characters(struct pcejson* parser,
        size_t head, size_t tail)
{
    char* dup = pcejson_temp_buffer_dup(parser);
    pcejson_temp_buffer_reset(parser);
    purc_rwstream_write(parser->rws, dup + head, strlen(dup) - head - tail);
    free(dup);
}

bool pcejson_temp_buffer_equal(struct pcejson* parser, const char* s)
{
    size_t sz = 0;
    const char* p = purc_rwstream_get_mem_buffer (parser->rws, &sz);
    return strcmp(p, s) == 0;
}

bool pcejson_temp_buffer_end_with(struct pcejson* parser, const char* s)
{
    size_t sz = 0;
    const char* p = purc_rwstream_get_mem_buffer (parser->rws, &sz);
    size_t len = pcejson_temp_buffer_length (parser);
    size_t cmp_len = strlen(s);
    return strncmp(p + len - cmp_len, s, cmp_len) == 0;
}

char pcejson_temp_buffer_last_char(struct pcejson* parser) {
    size_t sz = 0;
    const char* p = purc_rwstream_get_mem_buffer (parser->rws, &sz);
    size_t len = pcejson_temp_buffer_length (parser);
    return p[len - 1];
}

void pcejson_reset(struct pcejson* parser, int32_t depth, uint32_t flags)
{
    parser->state = ejson_init_state;
    parser->depth = depth;
    parser->flags = flags;
    pcejson_temp_buffer_reset(parser);
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
struct pcejson_token* pcejson_token_new(enum ejson_token_type type, char* buf)
{
    struct pcejson_token* token = ejson_alloc(sizeof(struct pcejson_token));
    token->type = type;
    token->buf = buf;
    return token;
}

void pcejson_token_destroy(struct pcejson_token* token)
{
    if (token && token->buf) {
        free(token->buf);
    }
    ejson_free(token);
}

#define    END_OF_FILE_MARKER     0

struct pcejson_token* pcejson_next_token(struct pcejson* ejson, purc_rwstream_t rws)
{
    char buf_utf8[8] = {0};
    wchar_t wc = 0;
    int len = 0;

next_input:
    len = purc_rwstream_read_utf8_char (rws, buf_utf8, &wc);
    if (len <= 0) {
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
                    return pcejson_token_new(ejson_token_eof, NULL);
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
                    return pcejson_token_new(ejson_token_eof, NULL);
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
                    pcejson_temp_buffer_reset(ejson);
                    SWITCH_TO(ejson_before_name_state);
                    return pcejson_token_new(ejson_token_start_object, NULL);
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
                    if (pcejson_stack_is_empty(ejson->stack)) {
                        SWITCH_TO(ejson_finished_state);
                    }
                    else {
                        SWITCH_TO(ejson_init_state);
                    }
                    return pcejson_token_new(ejson_token_end_object, NULL);
                }
                else {
                    pcinst_set_error(PCEJSON_UNEXPECTED_RIGHT_BRACE_PARSE_ERROR);
                    return NULL;
                }
            }
            else {
                pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                return NULL;
            }
        END_STATE()

        BEGIN_STATE(ejson_array_state)
            switch (wc) {
                case ' ':
                case '\x0A':
                case '\x09':
                case '\x0C':
                    ADVANCE_TO(ejson_before_value_state);
                    break;
                case '{':
                    pcejson_stack_push (ejson->stack, '[');
                    pcejson_temp_buffer_reset(ejson);
                    return pcejson_token_new(ejson_token_start_array, NULL);
                default:
                    pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                    return NULL;
            }
        END_STATE()

        BEGIN_STATE(ejson_after_array_state)
            if (wc == ']') {
                uint8_t c = pcejson_stack_last(ejson->stack);
                if (c == '[') {
                    pcejson_stack_pop(ejson->stack);
                    if (pcejson_stack_is_empty(ejson->stack)) {
                        SWITCH_TO(ejson_finished_state);
                    }
                    else {
                        SWITCH_TO(ejson_init_state);
                    }
                    return pcejson_token_new(ejson_token_end_array, NULL);
                }
                else {
                    pcinst_set_error(PCEJSON_UNEXPECTED_RIGHT_BRACKET_PARSE_ERROR);
                    return NULL;
                }
            }
            else {
                pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                return NULL;
            }
        END_STATE()

        BEGIN_STATE(ejson_before_name_state)
            if (is_whitespace(wc)) {
                ADVANCE_TO(ejson_before_name_state);
            }
            else if (wc == '"') {
                pcejson_temp_buffer_reset(ejson);
                uint8_t c = pcejson_stack_last(ejson->stack);
                if (c == '{') {
                    pcejson_stack_push (ejson->stack, ':');
                }
                RECONSUME_IN(ejson_name_double_quoted_state);
            }
            else if (wc == '\'') {
                pcejson_temp_buffer_reset(ejson);
                uint8_t c = pcejson_stack_last(ejson->stack);
                if (c == '{') {
                    pcejson_stack_push (ejson->stack, ':');
                }
                RECONSUME_IN(ejson_name_single_quoted_state);
            }
            else if (is_ascii_alpha(wc)) {
                pcejson_temp_buffer_reset(ejson);
                uint8_t c = pcejson_stack_last(ejson->stack);
                if (c == '{') {
                    pcejson_stack_push (ejson->stack, ':');
                }
                RECONSUME_IN(ejson_name_unquoted_state);
            }
            else {
                pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                return NULL;
            }
        END_STATE()

        BEGIN_STATE(ejson_after_name_state)
            switch (wc) {
                case ' ':
                case '\x0A':
                case '\x09':
                case '\x0C':
                    ADVANCE_TO(ejson_after_name_state);
                    break;
                case ':':
                    if (pcejson_temp_buffer_is_empty(ejson)) {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_KEY_NAME_PARSE_ERROR);
                        return NULL;
                    }
                    else {
                        SWITCH_TO(ejson_before_value_state);
                        return pcejson_token_new(ejson_token_key,
                                pcejson_temp_buffer_dup(ejson));
                    }
                default:
                    pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                    return NULL;
            }
        END_STATE()

        BEGIN_STATE(ejson_before_value_state)
            if (is_whitespace(wc)) {
                ADVANCE_TO(ejson_before_value_state);
            }
            else if (wc == '"') {
                pcejson_temp_buffer_reset(ejson);
                RECONSUME_IN(ejson_value_double_quoted_state);
            }
            else if (wc == '\'') {
                pcejson_temp_buffer_reset(ejson);
                RECONSUME_IN(ejson_value_single_quoted_state);
            }
            else if (wc == 'b') {
                pcejson_temp_buffer_reset(ejson);
                RECONSUME_IN(ejson_byte_sequence_state);
            }
            else if (wc == 't' || wc == 'f' || wc == 'n') {
                pcejson_temp_buffer_reset(ejson);
                RECONSUME_IN(ejson_keyword_state);
            }
            else if (is_ascii_digit(wc) || wc == '-') {
                pcejson_temp_buffer_reset(ejson);
                RECONSUME_IN(ejson_value_number_state);
            }
            else if (wc == '{') {
                RECONSUME_IN(ejson_object_state);
            }
            else if (wc == '[') {
                RECONSUME_IN(ejson_array_state);
            }
            else {
                pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                return NULL;
            }
        END_STATE()

        BEGIN_STATE(ejson_after_value_state)
            if (is_whitespace(wc)) {
                ADVANCE_TO(ejson_after_value_state);
            }
            else if (wc == '"' || wc == '\'') {
                return pcejson_token_new(ejson_token_string,
                        pcejson_temp_buffer_dup(ejson));
            }
            else if (wc == '}') {
                pcejson_stack_pop(ejson->stack);
                RECONSUME_IN(ejson_after_object_state);
            }
            else if (wc == ']') {
                pcejson_stack_pop(ejson->stack);
                RECONSUME_IN(ejson_after_array_state);
            }
            else if (wc == ',') {
                pcejson_stack_pop(ejson->stack);
                uint8_t c = pcejson_stack_last(ejson->stack);
                if (c == '{') {
                    SWITCH_TO(ejson_before_name_state);
                    return pcejson_token_new(ejson_token_comma, NULL);
                }
                else if (c == '[') {
                    SWITCH_TO(ejson_before_value_state);
                    return pcejson_token_new(ejson_token_comma, NULL);
                }
                else {
                    pcinst_set_error(PCEJSON_UNEXPECTED_COMMA_PARSE_ERROR);
                    return NULL;
                }
            }
            else {
                pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                return NULL;
            }
        END_STATE()

        BEGIN_STATE(ejson_name_unquoted_state)
            if (is_whitespace(wc) || wc == ':') {
                RECONSUME_IN(ejson_after_name_state);
            }
            else if (is_ascii_alpha(wc) || is_ascii_digit(wc) || wc == '-'
                    || wc == '_') {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                ADVANCE_TO(ejson_name_unquoted_state);
            }
            else {
                pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                return NULL;
            }
        END_STATE()

        BEGIN_STATE(ejson_name_single_quoted_state)
            if (wc == '\'') {
                size_t tmp_buf_len = pcejson_temp_buffer_length(ejson);
                if (tmp_buf_len >= 1) {
                    ADVANCE_TO(ejson_after_name_state);
                }
                else {
                    ADVANCE_TO(ejson_name_single_quoted_state);
                }
            }
            else if (wc == '\\') {
                ejson->return_state = ejson->state;
                ADVANCE_TO(ejson_string_escape_state);
            }
            else if (wc == END_OF_FILE_MARKER) {
                pcinst_set_error(PCEJSON_EOF_IN_STRING_PARSE_ERROR);
                return pcejson_token_new(ejson_token_eof, NULL);
            }
            else {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                ADVANCE_TO(ejson_name_single_quoted_state);
            }
        END_STATE()

        BEGIN_STATE(ejson_name_double_quoted_state)
            if (wc == '"') {
                size_t tmp_buf_len = pcejson_temp_buffer_length(ejson);
                if (tmp_buf_len >= 1) {
                    ADVANCE_TO(ejson_after_name_state);
                }
                else {
                    ADVANCE_TO(ejson_name_double_quoted_state);
                }
            }
            else if (wc == '\\') {
                ejson->return_state = ejson->state;
                ADVANCE_TO(ejson_string_escape_state);
            }
            else if (wc == END_OF_FILE_MARKER) {
                pcinst_set_error(PCEJSON_EOF_IN_STRING_PARSE_ERROR);
                return pcejson_token_new(ejson_token_eof, NULL);
            }
            else {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                ADVANCE_TO(ejson_name_double_quoted_state);
            }
        END_STATE()

        BEGIN_STATE(ejson_value_single_quoted_state)
            if (wc == '\'') {
                size_t tmp_buf_len = pcejson_temp_buffer_length(ejson);
                if (tmp_buf_len >= 1) {
                    RECONSUME_IN(ejson_after_value_state);
                }
                else {
                    ADVANCE_TO(ejson_value_single_quoted_state);
                }
            }
            else if (wc == '\\') {
                ejson->return_state = ejson->state;
                ADVANCE_TO(ejson_string_escape_state);
            }
            else if (wc == END_OF_FILE_MARKER) {
                pcinst_set_error(PCEJSON_EOF_IN_STRING_PARSE_ERROR);
                return pcejson_token_new(ejson_token_eof, NULL);
            }
            else {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                ADVANCE_TO(ejson_value_single_quoted_state);
            }
        END_STATE()

        BEGIN_STATE(ejson_value_double_quoted_state)
            if (wc == '"') {
                if (pcejson_temp_buffer_is_empty(ejson)) {
                    pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                    ADVANCE_TO(ejson_value_double_quoted_state);
                }
                else if (pcejson_temp_buffer_equal(ejson, "\"")) {
                    RECONSUME_IN(ejson_value_two_double_quoted_state);
                }
                else {
                    RECONSUME_IN(ejson_after_value_double_quoted_state);
                }
            }
            else if (wc == '\\') {
                ejson->return_state = ejson->state;
                ADVANCE_TO(ejson_string_escape_state);
            }
            else if (wc == END_OF_FILE_MARKER) {
                pcinst_set_error(PCEJSON_EOF_IN_STRING_PARSE_ERROR);
                return pcejson_token_new(ejson_token_eof, NULL);
            }
            else {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                ADVANCE_TO(ejson_value_double_quoted_state);
            }
        END_STATE()

        BEGIN_STATE(ejson_after_value_double_quoted_state)
            if (wc == '\"') {
                pcejson_temp_buffer_clear_head_tail_characters(ejson, 1, 0);
                RECONSUME_IN(ejson_after_value_state);
            }
            else {
                pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                return NULL;
            }
        END_STATE()

        BEGIN_STATE(ejson_value_two_double_quoted_state)
            if (wc == '"') {
                if (pcejson_temp_buffer_equal(ejson, "\"")) {
                    ADVANCE_TO(ejson_value_two_double_quoted_state);
                }
                else if (pcejson_temp_buffer_equal(ejson, "\"\"")) {
                    RECONSUME_IN(ejson_value_three_double_quoted_state);
                }
            }
            else if (wc == END_OF_FILE_MARKER) {
                pcinst_set_error(PCEJSON_EOF_IN_STRING_PARSE_ERROR);
                return pcejson_token_new(ejson_token_eof, NULL);
            }
            else {
                pcejson_temp_buffer_clear_head_tail_characters(ejson, 1, 1);
                RECONSUME_IN(ejson_after_value_state);
            }
        END_STATE()

        BEGIN_STATE(ejson_value_three_double_quoted_state)
            if (wc == '\"') {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                size_t buf_len = pcejson_temp_buffer_length(ejson);
                if (buf_len >= 6
                        && pcejson_temp_buffer_end_with(ejson, "\"\"\"")) {
                    pcejson_temp_buffer_clear_head_tail_characters(ejson, 1, 1);
                    SWITCH_TO(ejson_after_value_state);
                    return pcejson_token_new(ejson_token_text, NULL);
                }
                else {
                    ADVANCE_TO(ejson_value_three_double_quoted_state);
                }
            }
            else if (wc == END_OF_FILE_MARKER) {
                pcinst_set_error(PCEJSON_EOF_IN_STRING_PARSE_ERROR);
                return pcejson_token_new(ejson_token_eof,
                                pcejson_temp_buffer_dup(ejson));
            }
            else {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                ADVANCE_TO(ejson_value_three_double_quoted_state);
            }
        END_STATE()

        BEGIN_STATE(ejson_keyword_state)
            if (is_delimiter(wc)) {
                RECONSUME_IN(ejson_after_keyword_state);
            }
            switch (wc)
            {
                case 't':
                case 'f':
                case 'n':
                    if (pcejson_temp_buffer_is_empty(ejson)) {
                        pcejson_temp_buffer_append(ejson,
                                (uint8_t*)buf_utf8, len);
                        ADVANCE_TO(ejson_keyword_state);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_KEYWORD_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 'r':
                    if (pcejson_temp_buffer_equal(ejson, "t")) {
                        pcejson_temp_buffer_append(ejson,
                                (uint8_t*)buf_utf8, len);
                        ADVANCE_TO(ejson_keyword_state);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_KEYWORD_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 'u':
                    if (pcejson_temp_buffer_equal(ejson, "tr")
                        || pcejson_temp_buffer_equal(ejson, "n")) {
                        pcejson_temp_buffer_append(ejson,
                                (uint8_t*)buf_utf8, len);
                        ADVANCE_TO(ejson_keyword_state);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_KEYWORD_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 'e':
                    if (pcejson_temp_buffer_equal(ejson, "tru")
                        || pcejson_temp_buffer_equal(ejson, "fals")) {
                        pcejson_temp_buffer_append(ejson,
                                (uint8_t*)buf_utf8, len);
                        ADVANCE_TO(ejson_keyword_state);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_KEYWORD_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 'a':
                    if (pcejson_temp_buffer_equal(ejson, "f")) {
                        pcejson_temp_buffer_append(ejson,
                                (uint8_t*)buf_utf8, len);
                        ADVANCE_TO(ejson_keyword_state);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_KEYWORD_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 'l':
                    if (pcejson_temp_buffer_equal(ejson, "nu")
                        || pcejson_temp_buffer_equal(ejson, "nul")
                        || pcejson_temp_buffer_equal(ejson, "fa")) {
                        pcejson_temp_buffer_append(ejson,
                                (uint8_t*)buf_utf8, len);
                        ADVANCE_TO(ejson_keyword_state);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_KEYWORD_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 's':
                    if (pcejson_temp_buffer_equal(ejson, "fal")) {
                        pcejson_temp_buffer_append(ejson,
                                (uint8_t*)buf_utf8, len);
                        ADVANCE_TO(ejson_keyword_state);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_KEYWORD_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                default:
                    pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                    return NULL;
            }
        END_STATE()

        BEGIN_STATE(ejson_after_keyword_state)
            if (is_delimiter(wc)) {
                if (pcejson_temp_buffer_equal(ejson, "true")
                        || pcejson_temp_buffer_equal(ejson, "false")) {
                    RECONSUME_IN_NEXT(ejson_after_value_state);
                    return pcejson_token_new(ejson_token_boolean,
                                pcejson_temp_buffer_dup(ejson));
                }
                else if (pcejson_temp_buffer_equal(ejson, "null")) {
                    RECONSUME_IN_NEXT(ejson_after_value_state);
                    return pcejson_token_new(ejson_token_null, NULL);
                }
            }
            pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(ejson_byte_sequence_state)
            if (wc == 'b') {
                if (pcejson_temp_buffer_is_empty(ejson)) {
                    pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                    ADVANCE_TO(ejson_byte_sequence_state);
                }
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                ADVANCE_TO(ejson_binary_byte_sequence_state);
            }
            else if (wc == 'x') {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                ADVANCE_TO(ejson_hex_byte_sequence_state);
            }
            else if (wc == '6') {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                ADVANCE_TO(ejson_base64_byte_sequence_state);
            }
            pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(ejson_after_byte_sequence_state)
            if (is_delimiter(wc)) {
                SWITCH_TO(ejson_after_value_state);
                return pcejson_token_new(ejson_token_byte_squence,
                                pcejson_temp_buffer_dup(ejson));
            }
            pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(ejson_hex_byte_sequence_state)
            if (is_delimiter(wc)) {
                RECONSUME_IN(ejson_after_byte_sequence_state);
            }
            else if (is_ascii_digit(wc) || is_ascii_hex_digit(wc)) {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                ADVANCE_TO(ejson_hex_byte_sequence_state);
            }
            pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(ejson_binary_byte_sequence_state)
            if (is_delimiter(wc)) {
                RECONSUME_IN(ejson_after_byte_sequence_state);
            }
            else if (is_ascii_binary_digit(wc)) {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                ADVANCE_TO(ejson_hex_byte_sequence_state);
            }
            else if (wc == '.') {
                ADVANCE_TO(ejson_hex_byte_sequence_state);
            }
            pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(ejson_base64_byte_sequence_state)
            if (is_delimiter(wc)) {
                RECONSUME_IN(ejson_after_byte_sequence_state);
            }
            else if (wc == '=') {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                ADVANCE_TO(ejson_hex_byte_sequence_state);
            }
            else if (is_ascii_digit(wc) || is_ascii_alpha(wc)
                    || wc == '+' || wc == '-') {
                if (!pcejson_temp_buffer_end_with(ejson, "=")) {
                    pcejson_temp_buffer_append(ejson,
                            (uint8_t*)buf_utf8, len);
                    ADVANCE_TO(ejson_hex_byte_sequence_state);
                }
                else {
                    pcinst_set_error(PCEJSON_UNEXPECTED_BASE64_PARSE_ERROR);
                    return NULL;
                }
            }
            pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(ejson_value_number_state)
            if (is_delimiter(wc)) {
                RECONSUME_IN(ejson_after_value_number_state);
            }
            else if (is_ascii_digit(wc)) {
                RECONSUME_IN(ejson_value_number_state);
            }
            else if (wc == '-') {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                RECONSUME_IN(ejson_value_number_state);
            }
            pcinst_set_error(PCEJSON_BAD_JSON_NUMBER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(ejson_after_value_number_state)
            if (is_delimiter(wc)) {
                if (pcejson_temp_buffer_end_with(ejson, "-")
                        || pcejson_temp_buffer_end_with(ejson, "E")
                        || pcejson_temp_buffer_end_with(ejson, "e")) {
                    pcinst_set_error(PCEJSON_BAD_JSON_NUMBER_PARSE_ERROR);
                    return NULL;
                }
                else {
                    SWITCH_TO(ejson_after_value_state);
                    return pcejson_token_new(ejson_token_number,
                                pcejson_temp_buffer_dup(ejson));
                }
            }
        END_STATE()

        BEGIN_STATE(ejson_value_number_integer_state)
            if (is_delimiter(wc)) {
                RECONSUME_IN(ejson_after_value_number_state);
            }
            else if (is_ascii_digit(wc)) {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                ADVANCE_TO(ejson_value_number_integer_state);
            }
            else if (wc == 'E' || wc == 'e') {
                pcejson_temp_buffer_append(ejson, (uint8_t*)"e", 1);
                ADVANCE_TO(ejson_value_number_exponent_state);
            }
            else if (wc == '.') {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                ADVANCE_TO(ejson_value_number_fraction_state);
            }
            else if (wc == 'U' || wc == 'L') {
                RECONSUME_IN(ejson_value_number_suffix_integer_state);
            }
            pcinst_set_error(
                    PCEJSON_UNEXPECTED_JSON_NUMBER_INTEGER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(ejson_value_number_fraction_state)
            if (is_delimiter(wc)) {
                RECONSUME_IN(ejson_after_value_number_state);
            }
            else if (is_ascii_digit(wc)) {
                if (pcejson_temp_buffer_end_with(ejson, "F")) {
                    pcinst_set_error(PCEJSON_BAD_JSON_NUMBER_PARSE_ERROR);
                    return NULL;
                }
                else {
                    pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                    ADVANCE_TO(ejson_value_number_fraction_state);
                }
            }
            else if (wc == 'F') {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                ADVANCE_TO(ejson_value_number_fraction_state);
            }
            else if (wc == 'L') {
                if (pcejson_temp_buffer_end_with(ejson, "F")) {
                    pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                    SWITCH_TO(ejson_after_value_number_state);
                    return pcejson_token_new(ejson_token_long_double_number,
                                pcejson_temp_buffer_dup(ejson));
                }
            }
            else if (wc == 'E' || wc == 'e') {
                if (pcejson_temp_buffer_end_with(ejson, ".")) {
                    pcinst_set_error(
                        PCEJSON_UNEXPECTED_JSON_NUMBER_FRACTION_PARSE_ERROR);
                    return NULL;
                }
                else {
                    pcejson_temp_buffer_append(ejson, (uint8_t*)"e", 1);
                    ADVANCE_TO(ejson_value_number_exponent_state);
                }
            }
            pcinst_set_error(
                    PCEJSON_UNEXPECTED_JSON_NUMBER_FRACTION_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(ejson_value_number_exponent_state)
            if (is_delimiter(wc)) {
                RECONSUME_IN(ejson_after_value_number_state);
            }
            else if (is_ascii_digit(wc)) {
                RECONSUME_IN(ejson_value_number_exponent_integer_state);
            }
            else if (wc == '+' || wc == '-') {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                ADVANCE_TO(ejson_value_number_exponent_integer_state);
            }
            pcinst_set_error(
                    PCEJSON_UNEXPECTED_JSON_NUMBER_EXPONENT_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(ejson_value_number_exponent_integer_state)
            if (is_delimiter(wc)) {
                RECONSUME_IN(ejson_after_value_number_state);
            }
            else if (is_ascii_digit(wc)) {
                if (pcejson_temp_buffer_end_with(ejson, "F")) {
                    pcinst_set_error(PCEJSON_BAD_JSON_NUMBER_PARSE_ERROR);
                    return NULL;
                }
                else {
                    pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                    ADVANCE_TO(ejson_value_number_exponent_integer_state);
                }
            }
            else if (wc == 'F') {
                pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                ADVANCE_TO(ejson_value_number_exponent_integer_state);
            }
            else if (wc == 'L') {
                if (pcejson_temp_buffer_end_with(ejson, "F")) {
                    pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                    SWITCH_TO(ejson_after_value_number_state);
                    return pcejson_token_new(ejson_token_long_double_number,
                                pcejson_temp_buffer_dup(ejson));
                }
            }
            pcinst_set_error(
                    PCEJSON_UNEXPECTED_JSON_NUMBER_EXPONENT_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(ejson_value_number_suffix_integer_state)
            char last_c = pcejson_temp_buffer_last_char(ejson);
            if (is_delimiter(wc)) {
                RECONSUME_IN(ejson_after_value_number_state);
            }
            else if (wc == 'U') {
                if (is_ascii_digit(last_c)) {
                    pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                    ADVANCE_TO(ejson_value_number_suffix_integer_state);
                }
            }
            else if (wc == 'L') {
                if (is_ascii_digit(last_c) || last_c == 'U') {
                    pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                    if (pcejson_temp_buffer_end_with(ejson, "UL")) {
                        SWITCH_TO(ejson_after_value_number_state);
                        return pcejson_token_new(
                                ejson_token_unsigned_long_integer_number,
                                pcejson_temp_buffer_dup(ejson));
                    }
                    else if (pcejson_temp_buffer_end_with(ejson, "L")) {
                        SWITCH_TO(ejson_after_value_number_state);
                        return pcejson_token_new(ejson_token_long_integer_number,
                                    pcejson_temp_buffer_dup(ejson));
                    }
                }
            }
            pcinst_set_error(
                    PCEJSON_UNEXPECTED_JSON_NUMBER_INTEGER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(ejson_string_escape_state)
            switch (wc)
            {
                case '\\':
                case '/':
                case '"':
                case 'b':
                case 'f':
                case 'n':
                case 'r':
                case 't':
                    pcejson_temp_buffer_append(ejson, (uint8_t*)"\\", 1);
                    pcejson_temp_buffer_append(ejson, (uint8_t*)buf_utf8, len);
                    RETURN_TO(ejson->return_state);
                    break;
                case 'u':
                    pcejson_temp_buffer_reset2(ejson);
                    ADVANCE_TO(
                            ejson_string_escape_four_hexadecimal_digits_state);
                    break;
                default:
                    pcinst_set_error(
                         PCEJSON_BAD_JSON_STRING_ESCAPE_ENTITY_PARSE_ERROR);
                    return NULL;
            }
        END_STATE()

        BEGIN_STATE(ejson_string_escape_four_hexadecimal_digits_state)
            if (is_ascii_hex_digit(wc)) {
                pcejson_temp_buffer_append2(ejson, (uint8_t*)buf_utf8, len);
                size_t buf2_len = pcejson_temp_buffer_length2(ejson);
                if (buf2_len == 4) {
                    pcejson_temp_buffer_append(ejson, (uint8_t*)"\\u", 2);
                    purc_rwstream_dump_to_another(ejson->rws2, ejson->rws, 4);
                    RETURN_TO(ejson->return_state);
                }
                ADVANCE_TO(ejson_string_escape_four_hexadecimal_digits_state);
            }
            pcinst_set_error(
                    PCEJSON_BAD_JSON_STRING_ESCAPE_ENTITY_PARSE_ERROR);
            return NULL;
        END_STATE()

        default:
            break;
    }
    return NULL;
}



#define STATE_DESC(state_name)                                 \
    case state_name:                                           \
        return ""#state_name;                                  \

const char* pcejson_ejson_state_desc(enum ejson_state state)
{
    switch (state) {
        STATE_DESC(ejson_init_state)
        STATE_DESC(ejson_finished_state)
        STATE_DESC(ejson_object_state)
        STATE_DESC(ejson_after_object_state)
        STATE_DESC(ejson_array_state)
        STATE_DESC(ejson_after_array_state)
        STATE_DESC(ejson_before_name_state)
        STATE_DESC(ejson_after_name_state)
        STATE_DESC(ejson_before_value_state)
        STATE_DESC(ejson_after_value_state)
        STATE_DESC(ejson_name_unquoted_state)
        STATE_DESC(ejson_name_single_quoted_state)
        STATE_DESC(ejson_name_double_quoted_state)
        STATE_DESC(ejson_value_single_quoted_state)
        STATE_DESC(ejson_value_double_quoted_state)
        STATE_DESC(ejson_after_value_double_quoted_state)
        STATE_DESC(ejson_value_two_double_quoted_state)
        STATE_DESC(ejson_value_three_double_quoted_state)
        STATE_DESC(ejson_keyword_state)
        STATE_DESC(ejson_after_keyword_state)
        STATE_DESC(ejson_byte_sequence_state)
        STATE_DESC(ejson_after_byte_sequence_state)
        STATE_DESC(ejson_hex_byte_sequence_state)
        STATE_DESC(ejson_binary_byte_sequence_state)
        STATE_DESC(ejson_base64_byte_sequence_state)
        STATE_DESC(ejson_value_number_state)
        STATE_DESC(ejson_after_value_number_state)
        STATE_DESC(ejson_value_number_integer_state)
        STATE_DESC(ejson_value_number_fraction_state)
        STATE_DESC(ejson_value_number_exponent_state)
        STATE_DESC(ejson_value_number_exponent_integer_state)
        STATE_DESC(ejson_value_number_suffix_integer_state)
        STATE_DESC(ejson_string_escape_state)
        STATE_DESC(ejson_string_escape_four_hexadecimal_digits_state)
    }
    return NULL;
}
