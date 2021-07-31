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

#include <math.h>

#if HAVE(GLIB)
#include <gmodule.h>
#endif

#define MIN_EJSON_BUFFER_SIZE 128
#define MAX_EJSON_BUFFER_SIZE 1024 * 1024 * 1024

#if HAVE(GLIB)
#define    ejson_alloc(sz)   g_slice_alloc0(sz)
#define    ejson_free(p)     g_slice_free1(sizeof(*p), (gpointer)p)
#else
#define    ejson_alloc(sz)   calloc(1, sz)
#define    ejson_free(p)     free(p)
#endif

static const char* ejson_err_msgs[] = {
    /* PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR */
    "pcejson unexpected character parse error",
    /* PCEJSON_UNEXPECTED_NULL_CHARACTER_PARSE_ERROR */
    "pcejson unexpected null character parse error",
    /* PCEJSON_UNEXPECTED_JSON_NUMBER_EXPONENT_PARSE_ERROR */
    "pcejson unexpected json number exponent parse error",
    /* PCEJSON_UNEXPECTED_JSON_NUMBER_FRACTION_PARSE_ERROR */
    "pcejson unexpected json number fraction parse error",
    /* PCEJSON_UNEXPECTED_JSON_NUMBER_INTEGER_PARSE_ERROR */
    "pcejson unexpected json number integer parse error",
    /* PCEJSON_UNEXPECTED_JSON_NUMBER_PARSE_ERROR */
    "pcejson unexpected json number parse error",
    /* PCEJSON_UNEXPECTED_RIGHT_BRACE_PARSE_ERROR */
    "pcejson unexpected right brace parse error",
    /* PCEJSON_UNEXPECTED_RIGHT_BRACKET_PARSE_ERROR */
    "pcejson unexpected right bracket parse error",
    /* PCEJSON_UNEXPECTED_JSON_KEY_NAME_PARSE_ERROR */
    "pcejson unexpected json key name parse error",
    /* PCEJSON_UNEXPECTED_COMMA_PARSE_ERROR */
    "pcejson unexpected comma parse error",
    /* PCEJSON_UNEXPECTED_JSON_KEYWORD_PARSE_ERROR */
    "pcejson unexpected json keyword parse error",
    /* PCEJSON_UNEXPECTED_BASE64_PARSE_ERROR */
    "pcejson unexpected base64 parse error",
    /* PCEJSON_BAD_JSON_NUMBER_PARSE_ERROR */
    "pcejson bad json number parse error",
    /* PCEJSON_BAD_JSON_PARSE_ERROR */
    "pcejson bad json parse error",
    /* PCEJSON_BAD_JSON_STRING_ESCAPE_ENTITY_PARSE_ERROR */
    "pcejson bad json string escape entity parse error",
    /* PCEJSON_EOF_IN_STRING_PARSE_ERROR */
    "pcejson eof in string parse error",
};

static struct err_msg_seg _ejson_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_FIRST_EJSON,
    PURC_ERROR_FIRST_EJSON + PCA_TABLESIZE(ejson_err_msgs) - 1,
    ejson_err_msgs
};

void pcejson_init_once (void)
{
    pcinst_register_error_message_segment(&_ejson_err_msgs_seg);
}

static inline bool is_whitespace (wchar_t character)
{
    return character == ' ' || character == '\x0A'
        || character == '\x09' || character == '\x0C';
}

static inline wchar_t to_ascii_lower_unchecked (wchar_t character)
{
        return character | 0x20;
}

static inline bool is_ascii (wchar_t character)
{
    return !(character & ~0x7F);
}

static inline bool is_ascii_lower (wchar_t character)
{
    return character >= 'a' && character <= 'z';
}

static inline bool is_ascii_upper (wchar_t character)
{
     return character >= 'A' && character <= 'Z';
}

static inline bool is_ascii_space (wchar_t character)
{
    return character <= ' ' &&
        (character == ' ' || (character <= 0xD && character >= 0x9));
}

static inline bool is_ascii_digit (wchar_t character)
{
    return character >= '0' && character <= '9';
}

static inline bool is_ascii_binary_digit (wchar_t character)
{
     return character == '0' || character == '1';
}

static inline bool is_ascii_hex_digit (wchar_t character)
{
     return is_ascii_digit(character) ||
         (to_ascii_lower_unchecked(character) >= 'a'
          && to_ascii_lower_unchecked(character) <= 'f');
}

static inline bool is_ascii_octal_digit (wchar_t character)
{
     return character >= '0' && character <= '7';
}

static inline bool is_ascii_alpha (wchar_t character)
{
    return is_ascii_lower(to_ascii_lower_unchecked(character));
}

static inline bool is_ascii_alpha_numeric (wchar_t character)
{
    return is_ascii_digit(character) || is_ascii_alpha(character);
}

static inline bool is_delimiter (wchar_t c)
{
    return is_whitespace(c) || c == '}' || c == ']' || c == ',';
}

struct pcejson* pcejson_create (int32_t depth, uint32_t flags)
{
    struct pcejson* parser = (struct pcejson*) ejson_alloc (
            sizeof(struct pcejson));
    parser->state = EJSON_INIT_STATE;
    parser->depth = depth;
    parser->flags = flags;
    parser->stack = pcutils_stack_new(2 * depth);
    parser->vcm_stack = pcutils_stack_new(0);
    parser->tmp_buff = purc_rwstream_new_buffer(MIN_EJSON_BUFFER_SIZE,
            MAX_EJSON_BUFFER_SIZE);
    parser->tmp_buff2 = purc_rwstream_new_buffer(MIN_EJSON_BUFFER_SIZE,
            MAX_EJSON_BUFFER_SIZE);
    return parser;
}

void pcejson_destroy (struct pcejson* parser)
{
    if (parser) {
        pcutils_stack_destroy(parser->stack);
        pcutils_stack_destroy(parser->vcm_stack);
        purc_rwstream_destroy(parser->tmp_buff);
        purc_rwstream_destroy(parser->tmp_buff2);
        ejson_free(parser);
    }
}

void pcejson_tmp_buff_reset (purc_rwstream_t rws)
{
    size_t sz = 0;
    const char* p = purc_rwstream_get_mem_buffer (rws, &sz);
    memset((void*)p, 0, sz);
    purc_rwstream_seek(rws, 0, SEEK_SET);
}

bool pcejson_tmp_buff_is_empty (purc_rwstream_t rws)
{
    return (0 == purc_rwstream_tell(rws));
}

ssize_t pcejson_tmp_buff_append (purc_rwstream_t rws, uint8_t* buf,
        size_t sz)
{
    return purc_rwstream_write (rws, buf, sz);
}

size_t pcejson_tmp_buff_length (purc_rwstream_t rws)
{
    return purc_rwstream_tell (rws);
}

const char* pcejson_tmp_buf_get_buf (purc_rwstream_t rws, size_t* sz)
{
    *sz = purc_rwstream_tell (rws);
    size_t sz_mem = 0;
    return purc_rwstream_get_mem_buffer (rws, &sz_mem);
}

void pcejson_tmp_buff_remove_first_last (purc_rwstream_t rws,
        size_t first, size_t last)
{
    size_t length = 0;
    const char* p = pcejson_tmp_buf_get_buf(rws, &length);
    char* dup = (char*) malloc (length + 1);
    memcpy(dup, p, length);
    pcejson_tmp_buff_reset (rws);
    purc_rwstream_write(rws, dup + first, length - first - last);
    free(dup);
}

bool pcejson_tmp_buff_equal (purc_rwstream_t rws, const char* s)
{
    size_t length = 0;
    const char* p = pcejson_tmp_buf_get_buf(rws, &length);
    return (length == strlen(s) && memcmp(p, s, strlen(s)) == 0);
}

bool pcejson_tmp_buff_end_with (purc_rwstream_t rws, const char* s)
{
    size_t sz = 0;
    const char* p = purc_rwstream_get_mem_buffer (rws, &sz);
    size_t len = pcejson_tmp_buff_length  (rws);
    size_t cmp_len = strlen(s);
    return memcmp(p + len - cmp_len, s, cmp_len) == 0;
}

char pcejson_tmp_buff_last_char (purc_rwstream_t rws) {
    size_t sz = 0;
    const char* p = purc_rwstream_get_mem_buffer (rws, &sz);
    size_t len = pcejson_tmp_buff_length  (rws);
    return p[len - 1];
}

void pcejson_reset (struct pcejson* parser, int32_t depth, uint32_t flags)
{
    parser->state = EJSON_INIT_STATE;
    parser->depth = depth;
    parser->flags = flags;
    pcejson_tmp_buff_reset (parser->tmp_buff);
    pcejson_tmp_buff_reset (parser->tmp_buff2);
}

struct pcvcm_node* pcejson_token_to_pcvcm_node (
        struct pcutils_stack* node_stack,struct pcejson_token* token)
{
    struct pcvcm_node* node = NULL;
    union pcvcm_node_data data;
    data.sz_ptr[0] = token->sz_ptr[0];
    data.sz_ptr[1] = token->sz_ptr[1];
    switch (token->type)
    {
        case EJSON_TOKEN_START_OBJECT:
            node = pcvcm_node_new (PCVCM_NODE_TYPE_OBJECT, data);
            break;

        case EJSON_TOKEN_END_OBJECT:
            pcutils_stack_pop (node_stack);
            break;

        case EJSON_TOKEN_START_ARRAY:
            node = pcvcm_node_new (PCVCM_NODE_TYPE_ARRAY, data);
            break;

        case EJSON_TOKEN_END_ARRAY:
            pcutils_stack_pop (node_stack);
            break;

        case EJSON_TOKEN_KEY:
            node = pcvcm_node_new (PCVCM_NODE_TYPE_KEY, data);
            break;

        case EJSON_TOKEN_STRING:
            node = pcvcm_node_new (PCVCM_NODE_TYPE_STRING, data);
            break;

        case EJSON_TOKEN_NULL:
            node = pcvcm_node_new (PCVCM_NODE_TYPE_NULL, data);
            break;

        case EJSON_TOKEN_BOOLEAN:
            node = pcvcm_node_new (PCVCM_NODE_TYPE_BOOLEAN, data);
            break;

        case EJSON_TOKEN_NUMBER:
            node = pcvcm_node_new (PCVCM_NODE_TYPE_NUMBER, data);
            break;

        case EJSON_TOKEN_LONG_INT:
            node = pcvcm_node_new (PCVCM_NODE_TYPE_LONG_INT, data);
            break;

        case EJSON_TOKEN_ULONG_INT:
            node = pcvcm_node_new (PCVCM_NODE_TYPE_ULONG_INT, data);
            break;

        case EJSON_TOKEN_LONG_DOUBLE:
            node = pcvcm_node_new (PCVCM_NODE_TYPE_LONG_DOUBLE, data);
            break;

        case EJSON_TOKEN_TEXT:
            node = pcvcm_node_new (PCVCM_NODE_TYPE_STRING, data);
            break;

        case EJSON_TOKEN_BYTE_SQUENCE:
            node = pcvcm_node_new (PCVCM_NODE_TYPE_BYTE_SEQUENCE, data);
            break;

        default:
            break;
    }
    return node;
}

int pcejson_parse (struct pcvcm_node** vcm_tree, struct pcejson** parser,
        purc_rwstream_t rws)
{
    if (*parser == NULL) {
        *parser = pcejson_create (10, 1);
    }

    struct pcutils_stack* node_stack = (*parser)->vcm_stack;

    struct pcejson_token* token = pcejson_next_token(*parser, rws);
    while (token) {
        struct pcvcm_node* node = pcejson_token_to_pcvcm_node (node_stack,
                token);
        if (node) {
            if (*vcm_tree == NULL) {
                *vcm_tree = node;
            }
            struct pcvcm_node* parent =
                (struct pcvcm_node*) pcutils_stack_top(node_stack);
            if (parent && parent != node) {
                pctree_node_append_child (pcvcm_node_to_pctree_node(parent),
                        pcvcm_node_to_pctree_node(node));
            }
            if (node->type == PCVCM_NODE_TYPE_OBJECT
                    || node->type == PCVCM_NODE_TYPE_ARRAY) {
                pcutils_stack_push (node_stack, (uintptr_t) node);
            }
        }
        token = pcejson_next_token(*parser, rws);
    }

    return 0;
}

// eJSON tokenizer
struct pcejson_token* pcejson_token_new (enum ejson_token_type type,
        const uint8_t* bytes, size_t nr_bytes)
{
    struct pcejson_token* token = ejson_alloc(sizeof (struct pcejson_token));
    token->type = type;
    switch (type)
    {
        case EJSON_TOKEN_BOOLEAN:
            token->b = (bytes[0] == 't');
            break;

        case EJSON_TOKEN_NUMBER:
            token->d = strtod ((const char*)bytes, NULL);
            break;

        case EJSON_TOKEN_LONG_INT:
            token->i64 = strtoll ((const char*)bytes, NULL, 10);
            break;

        case EJSON_TOKEN_ULONG_INT:
            token->u64 = strtoull ((const char*)bytes, NULL, 10);
            break;

        case EJSON_TOKEN_LONG_DOUBLE:
            token->ld = strtold ((const char*)bytes, NULL);
            break;

        case EJSON_TOKEN_INFINITY:
//            token->d = (bytes[0] == '-') ? -DBL_MAX : DBL_MAX;
            token->d = (bytes[0] == '-') ? -INFINITY : INFINITY;
            break;

        case EJSON_TOKEN_NAN:
            token->d = NAN;
            break;

        case EJSON_TOKEN_KEY:
        case EJSON_TOKEN_STRING:
        case EJSON_TOKEN_TEXT:
        case EJSON_TOKEN_BYTE_SQUENCE:
            {
                uint8_t* buf = (uint8_t*) malloc (nr_bytes + 1);
                memcpy(buf, bytes, nr_bytes);
                buf[nr_bytes] = 0;
                token->sz_ptr[0] = nr_bytes;
                token->sz_ptr[1] = (uintptr_t) buf;
            }
            break;

        default:
            break;
    }
    return token;
}

struct pcejson_token* pcejson_token_new_from_tmp_buf (
        enum ejson_token_type type, purc_rwstream_t rws)
{
    const uint8_t* bytes = NULL;
    size_t nr_bytes = 0;

    switch (type)
    {
        case EJSON_TOKEN_BOOLEAN:
        case EJSON_TOKEN_NUMBER:
        case EJSON_TOKEN_LONG_INT:
        case EJSON_TOKEN_ULONG_INT:
        case EJSON_TOKEN_LONG_DOUBLE:
        case EJSON_TOKEN_KEY:
        case EJSON_TOKEN_STRING:
        case EJSON_TOKEN_TEXT:
        case EJSON_TOKEN_BYTE_SQUENCE:
        case EJSON_TOKEN_INFINITY:
            bytes = (const uint8_t*) pcejson_tmp_buf_get_buf(rws, &nr_bytes);
            break;

        default:
            break;
    }
    return pcejson_token_new (type, bytes, nr_bytes);
}

void pcejson_token_destroy (struct pcejson_token* token)
{
    if (token) {
        switch (token->type)
        {
            case EJSON_TOKEN_KEY:
            case EJSON_TOKEN_STRING:
            case EJSON_TOKEN_TEXT:
            case EJSON_TOKEN_BYTE_SQUENCE:
                if (token->sz_ptr[1]) {
                    free((void*)token->sz_ptr[1]);
                }
                break;

            default:
                break;
        }
        ejson_free(token);
    }
}

#define    END_OF_FILE_MARKER     0


struct pcejson_token* pcejson_next_token (struct pcejson* ejson,
        purc_rwstream_t rws)
{

next_input:
    if (!ejson->need_reconsume) {
        ejson->c_len = purc_rwstream_read_utf8_char (rws,
                ejson->c, &ejson->wc);
        if (ejson->c_len <= 0) {
            return NULL;
        }
    }

    ejson->need_reconsume = false;

next_state:
    switch (ejson->state) {

        BEGIN_STATE(EJSON_INIT_STATE)
            switch (ejson->wc) {
                case ' ':
                case '\x0A':
                case '\x09':
                case '\x0C':
                    ADVANCE_TO(EJSON_INIT_STATE);
                    break;
                case '{':
                    RECONSUME_IN(EJSON_OBJECT_STATE);
                    break;
                case '[':
                    RECONSUME_IN(EJSON_OBJECT_STATE);
                    break;
                case END_OF_FILE_MARKER:
                    return pcejson_token_new (EJSON_TOKEN_EOF, NULL, 0);
                default:
                    pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                    return NULL;
            }
        END_STATE()

        BEGIN_STATE(EJSON_FINISHED_STATE)
            switch (ejson->wc) {
                case ' ':
                case '\x0A':
                case '\x09':
                case '\x0C':
                    ADVANCE_TO(EJSON_FINISHED_STATE);
                    break;
                case END_OF_FILE_MARKER:
                    return pcejson_token_new (EJSON_TOKEN_EOF, NULL, 0);
                default:
                    pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                    return NULL;
            }
        END_STATE()

        BEGIN_STATE(EJSON_OBJECT_STATE)
            switch (ejson->wc) {
                case ' ':
                case '\x0A':
                case '\x09':
                case '\x0C':
                    ADVANCE_TO(EJSON_BEFORE_NAME_STATE);
                    break;
                case '{':
                    pcutils_stack_push (ejson->stack, '{');
                    pcejson_tmp_buff_reset (ejson->tmp_buff);
                    SWITCH_TO(EJSON_BEFORE_NAME_STATE);
                    return pcejson_token_new (EJSON_TOKEN_START_OBJECT,
                            NULL, 0);
                default:
                    pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                    return NULL;
            }
        END_STATE()

        BEGIN_STATE(EJSON_AFTER_OBJECT_STATE)
            if (ejson->wc == '}') {
                uint8_t c = pcutils_stack_top(ejson->stack);
                if (c == '{') {
                    pcutils_stack_pop(ejson->stack);
                    if (pcutils_stack_is_empty(ejson->stack)) {
                        SWITCH_TO(EJSON_FINISHED_STATE);
                    }
                    else {
                        SWITCH_TO(EJSON_AFTER_VALUE_STATE);
                    }
                    return pcejson_token_new (EJSON_TOKEN_END_OBJECT,
                            NULL, 0);
                }
                else {
                    pcinst_set_error(
                            PCEJSON_UNEXPECTED_RIGHT_BRACE_PARSE_ERROR);
                    return NULL;
                }
            }
            else {
                pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                return NULL;
            }
        END_STATE()

        BEGIN_STATE(EJSON_ARRAY_STATE)
            switch (ejson->wc) {
                case ' ':
                case '\x0A':
                case '\x09':
                case '\x0C':
                    ADVANCE_TO(EJSON_BEFORE_VALUE_STATE);
                    break;
                case '[':
                    pcutils_stack_push (ejson->stack, '[');
                    pcejson_tmp_buff_reset (ejson->tmp_buff);
                    SWITCH_TO(EJSON_BEFORE_VALUE_STATE);
                    return pcejson_token_new (EJSON_TOKEN_START_ARRAY, NULL, 0);
                default:
                    pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                    return NULL;
            }
        END_STATE()

        BEGIN_STATE(EJSON_AFTER_ARRAY_STATE)
            if (ejson->wc == ']') {
                uint8_t c = pcutils_stack_top(ejson->stack);
                if (c == '[') {
                    pcutils_stack_pop(ejson->stack);
                    if (pcutils_stack_is_empty(ejson->stack)) {
                        SWITCH_TO(EJSON_FINISHED_STATE);
                    }
                    else {
                        SWITCH_TO(EJSON_AFTER_VALUE_STATE);
                    }
                    return pcejson_token_new (EJSON_TOKEN_END_ARRAY, NULL, 0);
                }
                else {
                    pcinst_set_error(
                            PCEJSON_UNEXPECTED_RIGHT_BRACKET_PARSE_ERROR);
                    return NULL;
                }
            }
            else {
                pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                return NULL;
            }
        END_STATE()

        BEGIN_STATE(EJSON_BEFORE_NAME_STATE)
            if (is_whitespace(ejson->wc)) {
                ADVANCE_TO(EJSON_BEFORE_NAME_STATE);
            }
            else if (ejson->wc == '"') {
                pcejson_tmp_buff_reset (ejson->tmp_buff);
                uint8_t c = pcutils_stack_top(ejson->stack);
                if (c == '{') {
                    pcutils_stack_push (ejson->stack, ':');
                }
                RECONSUME_IN(EJSON_NAME_DOUBLE_QUOTED_STATE);
            }
            else if (ejson->wc == '\'') {
                pcejson_tmp_buff_reset (ejson->tmp_buff);
                uint8_t c = pcutils_stack_top(ejson->stack);
                if (c == '{') {
                    pcutils_stack_push (ejson->stack, ':');
                }
                RECONSUME_IN(EJSON_NAME_SINGLE_QUOTED_STATE);
            }
            else if (is_ascii_alpha(ejson->wc)) {
                pcejson_tmp_buff_reset (ejson->tmp_buff);
                uint8_t c = pcutils_stack_top(ejson->stack);
                if (c == '{') {
                    pcutils_stack_push (ejson->stack, ':');
                }
                RECONSUME_IN(EJSON_NAME_UNQUOTED_STATE);
            }
            else if (ejson->wc == '}') {
                RECONSUME_IN(EJSON_AFTER_OBJECT_STATE);
            }
            else {
                pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                return NULL;
            }
        END_STATE()

        BEGIN_STATE(EJSON_AFTER_NAME_STATE)
            switch (ejson->wc) {
                case ' ':
                case '\x0A':
                case '\x09':
                case '\x0C':
                    ADVANCE_TO(EJSON_AFTER_NAME_STATE);
                    break;
                case ':':
                    if (pcejson_tmp_buff_is_empty(ejson->tmp_buff)) {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_KEY_NAME_PARSE_ERROR);
                        return NULL;
                    }
                    else {
                        SWITCH_TO(EJSON_BEFORE_VALUE_STATE);
                        return pcejson_token_new_from_tmp_buf (EJSON_TOKEN_KEY,
                                ejson->tmp_buff);
                    }
                default:
                    pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                    return NULL;
            }
        END_STATE()

        BEGIN_STATE(EJSON_BEFORE_VALUE_STATE)
            if (is_whitespace(ejson->wc)) {
                ADVANCE_TO(EJSON_BEFORE_VALUE_STATE);
            }
            else if (ejson->wc == '"') {
                pcejson_tmp_buff_reset (ejson->tmp_buff);
                RECONSUME_IN(EJSON_VALUE_DOUBLE_QUOTED_STATE);
            }
            else if (ejson->wc == '\'') {
                pcejson_tmp_buff_reset (ejson->tmp_buff);
                RECONSUME_IN(EJSON_VALUE_SINGLE_QUOTED_STATE);
            }
            else if (ejson->wc == 'b') {
                pcejson_tmp_buff_reset (ejson->tmp_buff);
                RECONSUME_IN(EJSON_BYTE_SEQUENCE_STATE);
            }
            else if (ejson->wc == 't' || ejson->wc == 'f' || ejson->wc == 'n') {
                pcejson_tmp_buff_reset (ejson->tmp_buff);
                RECONSUME_IN(EJSON_KEYWORD_STATE);
            }
            else if (is_ascii_digit(ejson->wc) || ejson->wc == '-') {
                pcejson_tmp_buff_reset (ejson->tmp_buff);
                RECONSUME_IN(EJSON_VALUE_NUMBER_STATE);
            }
            else if (ejson->wc == '{') {
                RECONSUME_IN(EJSON_OBJECT_STATE);
            }
            else if (ejson->wc == '[') {
                RECONSUME_IN(EJSON_ARRAY_STATE);
            }
            else if (ejson->wc == ']') {
                RECONSUME_IN(EJSON_AFTER_ARRAY_STATE);
            }
            else if (ejson->wc == 'I') {
                pcejson_tmp_buff_reset (ejson->tmp_buff);
                RECONSUME_IN(EJSON_VALUE_NUMBER_INFINITY_STATE);
            }
            else if (ejson->wc == 'N') {
                pcejson_tmp_buff_reset (ejson->tmp_buff);
                RECONSUME_IN(EJSON_VALUE_NAN_STATE);
            }
            else {
                pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                return NULL;
            }
        END_STATE()

        BEGIN_STATE(EJSON_AFTER_VALUE_STATE)
            if (is_whitespace(ejson->wc)) {
                ADVANCE_TO(EJSON_AFTER_VALUE_STATE);
            }
            else if (ejson->wc == '"' || ejson->wc == '\'') {
                return pcejson_token_new_from_tmp_buf (EJSON_TOKEN_STRING,
                        ejson->tmp_buff);
            }
            else if (ejson->wc == '}') {
                pcutils_stack_pop(ejson->stack);
                RECONSUME_IN(EJSON_AFTER_OBJECT_STATE);
            }
            else if (ejson->wc == ']') {
                RECONSUME_IN(EJSON_AFTER_ARRAY_STATE);
            }
            else if (ejson->wc == ',') {
                uint8_t c = pcutils_stack_top(ejson->stack);
                if (c == '{') {
                    SWITCH_TO(EJSON_BEFORE_NAME_STATE);
                    return pcejson_token_new (EJSON_TOKEN_COMMA, NULL, 0);
                }
                else if (c == '[') {
                    SWITCH_TO(EJSON_BEFORE_VALUE_STATE);
                    return pcejson_token_new (EJSON_TOKEN_COMMA, NULL, 0);
                }
                else if (c == ':') {
                    pcutils_stack_pop(ejson->stack);
                    SWITCH_TO(EJSON_BEFORE_NAME_STATE);
                    return pcejson_token_new (EJSON_TOKEN_COMMA, NULL, 0);
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

        BEGIN_STATE(EJSON_NAME_UNQUOTED_STATE)
            if (is_whitespace(ejson->wc) || ejson->wc == ':') {
                RECONSUME_IN(EJSON_AFTER_NAME_STATE);
            }
            else if (is_ascii_alpha(ejson->wc) || is_ascii_digit(ejson->wc)
                    || ejson->wc == '-' || ejson->wc == '_') {
                pcejson_tmp_buff_append (ejson->tmp_buff, (uint8_t*)ejson->c,
                        ejson->c_len);
                ADVANCE_TO(EJSON_NAME_UNQUOTED_STATE);
            }
            else {
                pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                return NULL;
            }
        END_STATE()

        BEGIN_STATE(EJSON_NAME_SINGLE_QUOTED_STATE)
            if (ejson->wc == '\'') {
                size_t tmp_buf_len = pcejson_tmp_buff_length (ejson->tmp_buff);
                if (tmp_buf_len >= 1) {
                    ADVANCE_TO(EJSON_AFTER_NAME_STATE);
                }
                else {
                    ADVANCE_TO(EJSON_NAME_SINGLE_QUOTED_STATE);
                }
            }
            else if (ejson->wc == '\\') {
                ejson->return_state = ejson->state;
                ADVANCE_TO(EJSON_STRING_ESCAPE_STATE);
            }
            else if (ejson->wc == END_OF_FILE_MARKER) {
                pcinst_set_error(PCEJSON_EOF_IN_STRING_PARSE_ERROR);
                return pcejson_token_new (EJSON_TOKEN_EOF, NULL, 0);
            }
            else {
                pcejson_tmp_buff_append (ejson->tmp_buff, (uint8_t*)ejson->c,
                        ejson->c_len);
                ADVANCE_TO(EJSON_NAME_SINGLE_QUOTED_STATE);
            }
        END_STATE()

        BEGIN_STATE(EJSON_NAME_DOUBLE_QUOTED_STATE)
            if (ejson->wc == '"') {
                size_t tmp_buf_len = pcejson_tmp_buff_length (ejson->tmp_buff);
                if (tmp_buf_len >= 1) {
                    ADVANCE_TO(EJSON_AFTER_NAME_STATE);
                }
                else {
                    ADVANCE_TO(EJSON_NAME_DOUBLE_QUOTED_STATE);
                }
            }
            else if (ejson->wc == '\\') {
                ejson->return_state = ejson->state;
                ADVANCE_TO(EJSON_STRING_ESCAPE_STATE);
            }
            else if (ejson->wc == END_OF_FILE_MARKER) {
                pcinst_set_error(PCEJSON_EOF_IN_STRING_PARSE_ERROR);
                return pcejson_token_new (EJSON_TOKEN_EOF, NULL, 0);
            }
            else {
                pcejson_tmp_buff_append (ejson->tmp_buff, (uint8_t*)ejson->c,
                        ejson->c_len);
                ADVANCE_TO(EJSON_NAME_DOUBLE_QUOTED_STATE);
            }
        END_STATE()

        BEGIN_STATE(EJSON_VALUE_SINGLE_QUOTED_STATE)
            if (ejson->wc == '\'') {
                size_t tmp_buf_len = pcejson_tmp_buff_length (ejson->tmp_buff);
                if (tmp_buf_len >= 1) {
                    RECONSUME_IN(EJSON_AFTER_VALUE_STATE);
                }
                else {
                    ADVANCE_TO(EJSON_VALUE_SINGLE_QUOTED_STATE);
                }
            }
            else if (ejson->wc == '\\') {
                ejson->return_state = ejson->state;
                ADVANCE_TO(EJSON_STRING_ESCAPE_STATE);
            }
            else if (ejson->wc == END_OF_FILE_MARKER) {
                pcinst_set_error(PCEJSON_EOF_IN_STRING_PARSE_ERROR);
                return pcejson_token_new (EJSON_TOKEN_EOF, NULL, 0);
            }
            else {
                pcejson_tmp_buff_append (ejson->tmp_buff, (uint8_t*)ejson->c,
                        ejson->c_len);
                ADVANCE_TO(EJSON_VALUE_SINGLE_QUOTED_STATE);
            }
        END_STATE()

        BEGIN_STATE(EJSON_VALUE_DOUBLE_QUOTED_STATE)
            if (ejson->wc == '"') {
                if (pcejson_tmp_buff_is_empty(ejson->tmp_buff)) {
                    pcejson_tmp_buff_append (ejson->tmp_buff,
                            (uint8_t*)ejson->c, ejson->c_len);
                    ADVANCE_TO(EJSON_VALUE_DOUBLE_QUOTED_STATE);
                }
                else if (pcejson_tmp_buff_equal(ejson->tmp_buff, "\"")) {
                    RECONSUME_IN(EJSON_VALUE_TWO_DOUBLE_QUOTED_STATE);
                }
                else {
                    RECONSUME_IN(EJSON_AFTER_VALUE_DOUBLE_QUOTED_STATE);
                }
            }
            else if (ejson->wc == '\\') {
                ejson->return_state = ejson->state;
                ADVANCE_TO(EJSON_STRING_ESCAPE_STATE);
            }
            else if (ejson->wc == END_OF_FILE_MARKER) {
                pcinst_set_error(PCEJSON_EOF_IN_STRING_PARSE_ERROR);
                return pcejson_token_new (EJSON_TOKEN_EOF, NULL, 0);
            }
            else {
                pcejson_tmp_buff_append (ejson->tmp_buff,
                        (uint8_t*)ejson->c, ejson->c_len);
                ADVANCE_TO(EJSON_VALUE_DOUBLE_QUOTED_STATE);
            }
        END_STATE()

        BEGIN_STATE(EJSON_AFTER_VALUE_DOUBLE_QUOTED_STATE)
            if (ejson->wc == '\"') {
                pcejson_tmp_buff_remove_first_last (ejson->tmp_buff, 1, 0);
                RECONSUME_IN(EJSON_AFTER_VALUE_STATE);
            }
            else {
                pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
                return NULL;
            }
        END_STATE()

        BEGIN_STATE(EJSON_VALUE_TWO_DOUBLE_QUOTED_STATE)
            if (ejson->wc == '"') {
                if (pcejson_tmp_buff_equal(ejson->tmp_buff, "\"")) {
                    pcejson_tmp_buff_append (ejson->tmp_buff,
                            (uint8_t*)ejson->c, ejson->c_len);
                    ADVANCE_TO(EJSON_VALUE_TWO_DOUBLE_QUOTED_STATE);
                }
                else if (pcejson_tmp_buff_equal(ejson->tmp_buff, "\"\"")) {
                    RECONSUME_IN(EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE);
                }
            }
            else if (ejson->wc == END_OF_FILE_MARKER) {
                pcinst_set_error(PCEJSON_EOF_IN_STRING_PARSE_ERROR);
                return pcejson_token_new (EJSON_TOKEN_EOF, NULL, 0);
            }
            else {
                pcejson_tmp_buff_remove_first_last (ejson->tmp_buff, 1, 1);
                RECONSUME_IN(EJSON_AFTER_VALUE_STATE);
            }
        END_STATE()

        BEGIN_STATE(EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE)
            if (ejson->wc == '\"') {
                pcejson_tmp_buff_append (ejson->tmp_buff,
                        (uint8_t*)ejson->c, ejson->c_len);
                size_t buf_len = pcejson_tmp_buff_length (ejson->tmp_buff);
                if (buf_len >= 6
                        && pcejson_tmp_buff_end_with (ejson->tmp_buff,
                            "\"\"\"")) {
                    pcejson_tmp_buff_remove_first_last (ejson->tmp_buff, 3, 3);
                    SWITCH_TO(EJSON_AFTER_VALUE_STATE);
                    return pcejson_token_new_from_tmp_buf (EJSON_TOKEN_TEXT,
                                ejson->tmp_buff);
                }
                else {
                    ADVANCE_TO(EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE);
                }
            }
            else if (ejson->wc == END_OF_FILE_MARKER) {
                pcinst_set_error(PCEJSON_EOF_IN_STRING_PARSE_ERROR);
                return pcejson_token_new_from_tmp_buf (EJSON_TOKEN_EOF,
                                ejson->tmp_buff);
            }
            else {
                pcejson_tmp_buff_append (ejson->tmp_buff,
                        (uint8_t*)ejson->c, ejson->c_len);
                ADVANCE_TO(EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE);
            }
        END_STATE()

        BEGIN_STATE(EJSON_KEYWORD_STATE)
            if (is_delimiter(ejson->wc)) {
                RECONSUME_IN(EJSON_AFTER_KEYWORD_STATE);
            }
            switch (ejson->wc)
            {
                case 't':
                case 'f':
                case 'n':
                    if (pcejson_tmp_buff_is_empty(ejson->tmp_buff)) {
                        pcejson_tmp_buff_append (ejson->tmp_buff,
                                (uint8_t*)ejson->c, ejson->c_len);
                        ADVANCE_TO(EJSON_KEYWORD_STATE);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_KEYWORD_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 'r':
                    if (pcejson_tmp_buff_equal(ejson->tmp_buff, "t")) {
                        pcejson_tmp_buff_append (ejson->tmp_buff,
                                (uint8_t*)ejson->c, ejson->c_len);
                        ADVANCE_TO(EJSON_KEYWORD_STATE);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_KEYWORD_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 'u':
                    if (pcejson_tmp_buff_equal(ejson->tmp_buff, "tr")
                        || pcejson_tmp_buff_equal (ejson->tmp_buff, "n")) {
                        pcejson_tmp_buff_append (ejson->tmp_buff,
                                (uint8_t*)ejson->c, ejson->c_len);
                        ADVANCE_TO(EJSON_KEYWORD_STATE);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_KEYWORD_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 'e':
                    if (pcejson_tmp_buff_equal(ejson->tmp_buff, "tru")
                        || pcejson_tmp_buff_equal (ejson->tmp_buff, "fals")) {
                        pcejson_tmp_buff_append (ejson->tmp_buff,
                                (uint8_t*)ejson->c, ejson->c_len);
                        ADVANCE_TO(EJSON_KEYWORD_STATE);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_KEYWORD_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 'a':
                    if (pcejson_tmp_buff_equal(ejson->tmp_buff, "f")) {
                        pcejson_tmp_buff_append (ejson->tmp_buff,
                                (uint8_t*)ejson->c, ejson->c_len);
                        ADVANCE_TO(EJSON_KEYWORD_STATE);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_KEYWORD_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 'l':
                    if (pcejson_tmp_buff_equal(ejson->tmp_buff, "nu")
                        || pcejson_tmp_buff_equal (ejson->tmp_buff, "nul")
                        || pcejson_tmp_buff_equal (ejson->tmp_buff, "fa")) {
                        pcejson_tmp_buff_append (ejson->tmp_buff,
                                (uint8_t*)ejson->c, ejson->c_len);
                        ADVANCE_TO(EJSON_KEYWORD_STATE);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_KEYWORD_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 's':
                    if (pcejson_tmp_buff_equal(ejson->tmp_buff, "fal")) {
                        pcejson_tmp_buff_append (ejson->tmp_buff,
                                (uint8_t*)ejson->c, ejson->c_len);
                        ADVANCE_TO(EJSON_KEYWORD_STATE);
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

        BEGIN_STATE(EJSON_AFTER_KEYWORD_STATE)
            if (is_delimiter(ejson->wc)) {
                if (pcejson_tmp_buff_equal(ejson->tmp_buff, "true")
                        || pcejson_tmp_buff_equal (ejson->tmp_buff, "false")) {
                    RECONSUME_IN_NEXT(EJSON_AFTER_VALUE_STATE);
                    return pcejson_token_new_from_tmp_buf (EJSON_TOKEN_BOOLEAN,
                                ejson->tmp_buff);
                }
                else if (pcejson_tmp_buff_equal(ejson->tmp_buff, "null")) {
                    RECONSUME_IN_NEXT(EJSON_AFTER_VALUE_STATE);
                    return pcejson_token_new (EJSON_TOKEN_NULL, NULL, 0);
                }
            }
            pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(EJSON_BYTE_SEQUENCE_STATE)
            if (ejson->wc == 'b') {
                if (pcejson_tmp_buff_is_empty(ejson->tmp_buff)) {
                    pcejson_tmp_buff_append (ejson->tmp_buff,
                            (uint8_t*)ejson->c, ejson->c_len);
                    ADVANCE_TO(EJSON_BYTE_SEQUENCE_STATE);
                }
                pcejson_tmp_buff_append (ejson->tmp_buff, (uint8_t*)ejson->c,
                        ejson->c_len);
                ADVANCE_TO(EJSON_BINARY_BYTE_SEQUENCE_STATE);
            }
            else if (ejson->wc == 'x') {
                pcejson_tmp_buff_append (ejson->tmp_buff, (uint8_t*)ejson->c,
                        ejson->c_len);
                ADVANCE_TO(EJSON_HEX_BYTE_SEQUENCE_STATE);
            }
            else if (ejson->wc == '6') {
                pcejson_tmp_buff_append (ejson->tmp_buff, (uint8_t*)ejson->c,
                        ejson->c_len);
                ADVANCE_TO(EJSON_BASE64_BYTE_SEQUENCE_STATE);
            }
            pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(EJSON_AFTER_BYTE_SEQUENCE_STATE)
            if (is_delimiter(ejson->wc)) {
                RECONSUME_IN_NEXT(EJSON_AFTER_VALUE_STATE);
                return pcejson_token_new_from_tmp_buf (EJSON_TOKEN_BYTE_SQUENCE,
                                ejson->tmp_buff);
            }
            pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(EJSON_HEX_BYTE_SEQUENCE_STATE)
            if (is_delimiter(ejson->wc)) {
                RECONSUME_IN(EJSON_AFTER_BYTE_SEQUENCE_STATE);
            }
            else if (is_ascii_digit(ejson->wc)
                    || is_ascii_hex_digit(ejson->wc)) {
                pcejson_tmp_buff_append (ejson->tmp_buff, (uint8_t*)ejson->c,
                        ejson->c_len);
                ADVANCE_TO(EJSON_HEX_BYTE_SEQUENCE_STATE);
            }
            pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(EJSON_BINARY_BYTE_SEQUENCE_STATE)
            if (is_delimiter(ejson->wc)) {
                RECONSUME_IN(EJSON_AFTER_BYTE_SEQUENCE_STATE);
            }
            else if (is_ascii_binary_digit(ejson->wc)) {
                pcejson_tmp_buff_append (ejson->tmp_buff, (uint8_t*)ejson->c,
                        ejson->c_len);
                ADVANCE_TO(EJSON_BINARY_BYTE_SEQUENCE_STATE);
            }
            else if (ejson->wc == '.') {
                ADVANCE_TO(EJSON_BINARY_BYTE_SEQUENCE_STATE);
            }
            pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(EJSON_BASE64_BYTE_SEQUENCE_STATE)
            if (is_delimiter(ejson->wc)) {
                RECONSUME_IN(EJSON_AFTER_BYTE_SEQUENCE_STATE);
            }
            else if (ejson->wc == '=') {
                pcejson_tmp_buff_append (ejson->tmp_buff, (uint8_t*)ejson->c,
                        ejson->c_len);
                ADVANCE_TO(EJSON_BASE64_BYTE_SEQUENCE_STATE);
            }
            else if (is_ascii_digit(ejson->wc) || is_ascii_alpha(ejson->wc)
                    || ejson->wc == '+' || ejson->wc == '-') {
                if (!pcejson_tmp_buff_end_with(ejson->tmp_buff, "=")) {
                    pcejson_tmp_buff_append (ejson->tmp_buff,
                            (uint8_t*)ejson->c, ejson->c_len);
                    ADVANCE_TO(EJSON_BASE64_BYTE_SEQUENCE_STATE);
                }
                else {
                    pcinst_set_error(PCEJSON_UNEXPECTED_BASE64_PARSE_ERROR);
                    return NULL;
                }
            }
            pcinst_set_error(PCEJSON_UNEXPECTED_CHARACTER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(EJSON_VALUE_NUMBER_STATE)
            if (is_delimiter(ejson->wc)) {
                RECONSUME_IN(EJSON_AFTER_VALUE_NUMBER_STATE);
            }
            else if (is_ascii_digit(ejson->wc)) {
                RECONSUME_IN(EJSON_VALUE_NUMBER_INTEGER_STATE);
            }
            else if (ejson->wc == '-') {
                pcejson_tmp_buff_append(ejson->tmp_buff, (uint8_t*)ejson->c,
                        ejson->c_len);
                ADVANCE_TO(EJSON_VALUE_NUMBER_INTEGER_STATE);
            }
            pcinst_set_error(PCEJSON_BAD_JSON_NUMBER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(EJSON_AFTER_VALUE_NUMBER_STATE)
            if (is_delimiter(ejson->wc)) {
                if (pcejson_tmp_buff_end_with(ejson->tmp_buff, "-")
                        || pcejson_tmp_buff_end_with (ejson->tmp_buff, "E")
                        || pcejson_tmp_buff_end_with (ejson->tmp_buff, "e")) {
                    pcinst_set_error(PCEJSON_BAD_JSON_NUMBER_PARSE_ERROR);
                    return NULL;
                }
                else {
                    RECONSUME_IN_NEXT(EJSON_AFTER_VALUE_STATE);
                    return pcejson_token_new_from_tmp_buf (EJSON_TOKEN_NUMBER,
                                ejson->tmp_buff);
                }
            }
        END_STATE()

        BEGIN_STATE(EJSON_VALUE_NUMBER_INTEGER_STATE)
            if (is_delimiter(ejson->wc)) {
                RECONSUME_IN(EJSON_AFTER_VALUE_NUMBER_STATE);
            }
            else if (is_ascii_digit(ejson->wc)) {
                pcejson_tmp_buff_append(ejson->tmp_buff,  (uint8_t*)ejson->c,
                        ejson->c_len);
                ADVANCE_TO(EJSON_VALUE_NUMBER_INTEGER_STATE);
            }
            else if (ejson->wc == 'E' || ejson->wc == 'e') {
                pcejson_tmp_buff_append(ejson->tmp_buff,  (uint8_t*)"e", 1);
                ADVANCE_TO(EJSON_VALUE_NUMBER_EXPONENT_STATE);
            }
            else if (ejson->wc == '.' || ejson->wc == 'F') {
                pcejson_tmp_buff_append(ejson->tmp_buff, (uint8_t*)ejson->c,
                        ejson->c_len);
                ADVANCE_TO(EJSON_VALUE_NUMBER_FRACTION_STATE);
            }
            else if (ejson->wc == 'U' || ejson->wc == 'L') {
                RECONSUME_IN(EJSON_VALUE_NUMBER_SUFFIX_INTEGER_STATE);
            }
            else if (ejson->wc == 'I' && (
                        pcejson_tmp_buff_is_empty(ejson->tmp_buff) ||
                        pcejson_tmp_buff_equal(ejson->tmp_buff, "-"))) {
                RECONSUME_IN(EJSON_VALUE_NUMBER_INFINITY_STATE);
            }
            pcinst_set_error(
                    PCEJSON_UNEXPECTED_JSON_NUMBER_INTEGER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(EJSON_VALUE_NUMBER_FRACTION_STATE)
            if (is_delimiter(ejson->wc)) {
                RECONSUME_IN(EJSON_AFTER_VALUE_NUMBER_STATE);
            }
            else if (is_ascii_digit(ejson->wc)) {
                if (pcejson_tmp_buff_end_with(ejson->tmp_buff, "F")) {
                    pcinst_set_error(PCEJSON_BAD_JSON_NUMBER_PARSE_ERROR);
                    return NULL;
                }
                else {
                    pcejson_tmp_buff_append(ejson->tmp_buff,
                            (uint8_t*)ejson->c, ejson->c_len);
                    ADVANCE_TO(EJSON_VALUE_NUMBER_FRACTION_STATE);
                }
            }
            else if (ejson->wc == 'F') {
                pcejson_tmp_buff_append(ejson->tmp_buff, (uint8_t*)ejson->c,
                        ejson->c_len);
                ADVANCE_TO(EJSON_VALUE_NUMBER_FRACTION_STATE);
            }
            else if (ejson->wc == 'L') {
                if (pcejson_tmp_buff_end_with(ejson->tmp_buff, "F")) {
                    pcejson_tmp_buff_append(ejson->tmp_buff,
                            (uint8_t*)ejson->c, ejson->c_len);
                    SWITCH_TO(EJSON_AFTER_VALUE_STATE);
                    return pcejson_token_new_from_tmp_buf (
                            EJSON_TOKEN_LONG_DOUBLE, ejson->tmp_buff);
                }
            }
            else if (ejson->wc == 'E' || ejson->wc == 'e') {
                if (pcejson_tmp_buff_end_with(ejson->tmp_buff, ".")) {
                    pcinst_set_error(
                        PCEJSON_UNEXPECTED_JSON_NUMBER_FRACTION_PARSE_ERROR);
                    return NULL;
                }
                else {
                    pcejson_tmp_buff_append(ejson->tmp_buff,  (uint8_t*)"e", 1);
                    ADVANCE_TO(EJSON_VALUE_NUMBER_EXPONENT_STATE);
                }
            }
            pcinst_set_error(
                    PCEJSON_UNEXPECTED_JSON_NUMBER_FRACTION_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(EJSON_VALUE_NUMBER_EXPONENT_STATE)
            if (is_delimiter(ejson->wc)) {
                RECONSUME_IN(EJSON_AFTER_VALUE_NUMBER_STATE);
            }
            else if (is_ascii_digit(ejson->wc)) {
                RECONSUME_IN(EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE);
            }
            else if (ejson->wc == '+' || ejson->wc == '-') {
                pcejson_tmp_buff_append(ejson->tmp_buff, (uint8_t*)ejson->c,
                        ejson->c_len);
                ADVANCE_TO(EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE);
            }
            pcinst_set_error(
                    PCEJSON_UNEXPECTED_JSON_NUMBER_EXPONENT_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE)
            if (is_delimiter(ejson->wc)) {
                RECONSUME_IN(EJSON_AFTER_VALUE_NUMBER_STATE);
            }
            else if (is_ascii_digit(ejson->wc)) {
                if (pcejson_tmp_buff_end_with(ejson->tmp_buff, "F")) {
                    pcinst_set_error(PCEJSON_BAD_JSON_NUMBER_PARSE_ERROR);
                    return NULL;
                }
                else {
                    pcejson_tmp_buff_append(ejson->tmp_buff, (uint8_t*)ejson->c,
                            ejson->c_len);
                    ADVANCE_TO(EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE);
                }
            }
            else if (ejson->wc == 'F') {
                pcejson_tmp_buff_append(ejson->tmp_buff, (uint8_t*)ejson->c,
                        ejson->c_len);
                ADVANCE_TO(EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE);
            }
            else if (ejson->wc == 'L') {
                if (pcejson_tmp_buff_end_with(ejson->tmp_buff, "F")) {
                    pcejson_tmp_buff_append(ejson->tmp_buff, (uint8_t*)ejson->c,
                            ejson->c_len);
                    SWITCH_TO(EJSON_AFTER_VALUE_NUMBER_STATE);
                    return pcejson_token_new_from_tmp_buf (
                            EJSON_TOKEN_LONG_DOUBLE, ejson->tmp_buff);
                }
            }
            pcinst_set_error(
                    PCEJSON_UNEXPECTED_JSON_NUMBER_EXPONENT_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(EJSON_VALUE_NUMBER_SUFFIX_INTEGER_STATE)
            char last_c = pcejson_tmp_buff_last_char (ejson->tmp_buff);
            if (is_delimiter(ejson->wc)) {
                RECONSUME_IN(EJSON_AFTER_VALUE_NUMBER_STATE);
            }
            else if (ejson->wc == 'U') {
                if (is_ascii_digit(last_c)) {
                    pcejson_tmp_buff_append(ejson->tmp_buff,
                            (uint8_t*)ejson->c, ejson->c_len);
                    ADVANCE_TO(EJSON_VALUE_NUMBER_SUFFIX_INTEGER_STATE);
                }
            }
            else if (ejson->wc == 'L') {
                if (is_ascii_digit(last_c) || last_c == 'U') {
                    pcejson_tmp_buff_append(ejson->tmp_buff, (uint8_t*)ejson->c,
                            ejson->c_len);
                    if (pcejson_tmp_buff_end_with(ejson->tmp_buff, "UL")) {
                        SWITCH_TO(EJSON_AFTER_VALUE_STATE);
                        return pcejson_token_new_from_tmp_buf (
                                EJSON_TOKEN_ULONG_INT,
                                ejson->tmp_buff);
                    }
                    else if (pcejson_tmp_buff_end_with(ejson->tmp_buff, "L")) {
                        SWITCH_TO(EJSON_AFTER_VALUE_STATE);
                        return pcejson_token_new_from_tmp_buf (
                                EJSON_TOKEN_LONG_INT, ejson->tmp_buff);
                    }
                }
            }
            pcinst_set_error(
                    PCEJSON_UNEXPECTED_JSON_NUMBER_INTEGER_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(EJSON_STRING_ESCAPE_STATE)
            switch (ejson->wc)
            {
                case '\\':
                case '/':
                case '"':
                case 'b':
                case 'f':
                case 'n':
                case 'r':
                case 't':
                    pcejson_tmp_buff_append(ejson->tmp_buff, (uint8_t*)"\\", 1);
                    pcejson_tmp_buff_append(ejson->tmp_buff, (uint8_t*)ejson->c,
                            ejson->c_len);
                    ADVANCE_TO(ejson->return_state);
                    break;
                case 'u':
                    pcejson_tmp_buff_reset (ejson->tmp_buff2);
                    ADVANCE_TO(
                            EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS_STATE);
                    break;
                default:
                    pcinst_set_error(
                         PCEJSON_BAD_JSON_STRING_ESCAPE_ENTITY_PARSE_ERROR);
                    return NULL;
            }
        END_STATE()

        BEGIN_STATE(EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS_STATE)
            if (is_ascii_hex_digit(ejson->wc)) {
                pcejson_tmp_buff_append(ejson->tmp_buff2, (uint8_t*)ejson->c,
                        ejson->c_len);
                size_t buf2_len = pcejson_tmp_buff_length (ejson->tmp_buff2);
                if (buf2_len == 4) {
                    pcejson_tmp_buff_append(ejson->tmp_buff, (uint8_t*)"\\u", 2);
                    purc_rwstream_seek(ejson->tmp_buff2, 0, SEEK_SET);
                    purc_rwstream_dump_to_another(ejson->tmp_buff2,
                            ejson->tmp_buff, 4);
                    ADVANCE_TO(ejson->return_state);
                }
                ADVANCE_TO(EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS_STATE);
            }
            pcinst_set_error(
                    PCEJSON_BAD_JSON_STRING_ESCAPE_ENTITY_PARSE_ERROR);
            return NULL;
        END_STATE()

        BEGIN_STATE(EJSON_VALUE_NUMBER_INFINITY_STATE)
            if (is_delimiter(ejson->wc)) {
                if (pcejson_tmp_buff_equal(ejson->tmp_buff, "-Infinity")
                    || pcejson_tmp_buff_equal(ejson->tmp_buff, "Infinity")) {
                    RECONSUME_IN_NEXT(EJSON_AFTER_VALUE_STATE);
                    return pcejson_token_new_from_tmp_buf (
                            EJSON_TOKEN_INFINITY, ejson->tmp_buff);
                }
                pcinst_set_error(PCEJSON_UNEXPECTED_JSON_NUMBER_PARSE_ERROR);
                return NULL;
            }
            switch (ejson->wc)
            {
                case 'I':
                    if (pcejson_tmp_buff_is_empty(ejson->tmp_buff)
                        || pcejson_tmp_buff_equal(ejson->tmp_buff, "-")) {
                        pcejson_tmp_buff_append (ejson->tmp_buff,
                                (uint8_t*)ejson->c, ejson->c_len);
                        ADVANCE_TO(EJSON_VALUE_NUMBER_INFINITY_STATE);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_NUMBER_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 'n':
                    if (pcejson_tmp_buff_equal(ejson->tmp_buff, "I")
                        || pcejson_tmp_buff_equal (ejson->tmp_buff, "-I")
                        || pcejson_tmp_buff_equal (ejson->tmp_buff, "Infi")
                        || pcejson_tmp_buff_equal (ejson->tmp_buff, "-Infi")
                        ) {
                        pcejson_tmp_buff_append (ejson->tmp_buff,
                                (uint8_t*)ejson->c, ejson->c_len);
                        ADVANCE_TO(EJSON_VALUE_NUMBER_INFINITY_STATE);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_NUMBER_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 'f':
                    if (pcejson_tmp_buff_equal(ejson->tmp_buff, "In")
                        || pcejson_tmp_buff_equal (ejson->tmp_buff, "-In")
                            ) {
                        pcejson_tmp_buff_append (ejson->tmp_buff,
                                (uint8_t*)ejson->c, ejson->c_len);
                        ADVANCE_TO(EJSON_VALUE_NUMBER_INFINITY_STATE);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_NUMBER_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 'i':
                    if (pcejson_tmp_buff_equal(ejson->tmp_buff, "Inf")
                        || pcejson_tmp_buff_equal (ejson->tmp_buff, "-Inf")
                        || pcejson_tmp_buff_equal (ejson->tmp_buff, "Infin")
                        || pcejson_tmp_buff_equal (ejson->tmp_buff, "-Infin")) {
                        pcejson_tmp_buff_append (ejson->tmp_buff,
                                (uint8_t*)ejson->c, ejson->c_len);
                        ADVANCE_TO(EJSON_VALUE_NUMBER_INFINITY_STATE);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_NUMBER_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 't':
                    if (pcejson_tmp_buff_equal(ejson->tmp_buff, "Infini")
                        || pcejson_tmp_buff_equal (ejson->tmp_buff, "-Infini")
                            ) {
                        pcejson_tmp_buff_append (ejson->tmp_buff,
                                (uint8_t*)ejson->c, ejson->c_len);
                        ADVANCE_TO(EJSON_VALUE_NUMBER_INFINITY_STATE);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_NUMBER_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 'y':
                    if (pcejson_tmp_buff_equal(ejson->tmp_buff, "Infinit")
                        || pcejson_tmp_buff_equal (ejson->tmp_buff, "-Infinit")
                            ) {
                        pcejson_tmp_buff_append (ejson->tmp_buff,
                                (uint8_t*)ejson->c, ejson->c_len);
                        ADVANCE_TO(EJSON_VALUE_NUMBER_INFINITY_STATE);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_NUMBER_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                default:
                    pcinst_set_error(
                            PCEJSON_UNEXPECTED_JSON_NUMBER_PARSE_ERROR);
                    return NULL;
            }
        END_STATE()

        BEGIN_STATE(EJSON_VALUE_NAN_STATE)
            if (is_delimiter(ejson->wc)) {
                if (pcejson_tmp_buff_equal(ejson->tmp_buff, "NaN")) {
                    RECONSUME_IN_NEXT(EJSON_AFTER_VALUE_STATE);
                    return pcejson_token_new (EJSON_TOKEN_NAN, NULL, 0);
                }
                pcinst_set_error(PCEJSON_UNEXPECTED_JSON_NUMBER_PARSE_ERROR);
                return NULL;
            }
            switch (ejson->wc)
            {
                case 'N':
                    if (pcejson_tmp_buff_is_empty(ejson->tmp_buff)
                        || pcejson_tmp_buff_equal(ejson->tmp_buff, "Na")) {
                        pcejson_tmp_buff_append (ejson->tmp_buff,
                                (uint8_t*)ejson->c, ejson->c_len);
                        ADVANCE_TO(EJSON_VALUE_NAN_STATE);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_NUMBER_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                case 'a':
                    if (pcejson_tmp_buff_equal(ejson->tmp_buff, "N")) {
                        pcejson_tmp_buff_append (ejson->tmp_buff,
                                (uint8_t*)ejson->c, ejson->c_len);
                        ADVANCE_TO(EJSON_VALUE_NAN_STATE);
                    }
                    else {
                        pcinst_set_error(
                                PCEJSON_UNEXPECTED_JSON_NUMBER_PARSE_ERROR);
                        return NULL;
                    }
                    break;

                default:
                    pcinst_set_error(
                            PCEJSON_UNEXPECTED_JSON_NUMBER_PARSE_ERROR);
                    return NULL;
            }
        END_STATE()

        default:
            break;
    }
    return NULL;
}



#define STATE_DESC(state_name)                                 \
    case state_name:                                           \
        return ""#state_name;                                  \

const char* pcejson_ejson_state_desc (enum ejson_state state)
{
    switch (state) {
        STATE_DESC(EJSON_INIT_STATE)
        STATE_DESC(EJSON_FINISHED_STATE)
        STATE_DESC(EJSON_OBJECT_STATE)
        STATE_DESC(EJSON_AFTER_OBJECT_STATE)
        STATE_DESC(EJSON_ARRAY_STATE)
        STATE_DESC(EJSON_AFTER_ARRAY_STATE)
        STATE_DESC(EJSON_BEFORE_NAME_STATE)
        STATE_DESC(EJSON_AFTER_NAME_STATE)
        STATE_DESC(EJSON_BEFORE_VALUE_STATE)
        STATE_DESC(EJSON_AFTER_VALUE_STATE)
        STATE_DESC(EJSON_NAME_UNQUOTED_STATE)
        STATE_DESC(EJSON_NAME_SINGLE_QUOTED_STATE)
        STATE_DESC(EJSON_NAME_DOUBLE_QUOTED_STATE)
        STATE_DESC(EJSON_VALUE_SINGLE_QUOTED_STATE)
        STATE_DESC(EJSON_VALUE_DOUBLE_QUOTED_STATE)
        STATE_DESC(EJSON_AFTER_VALUE_DOUBLE_QUOTED_STATE)
        STATE_DESC(EJSON_VALUE_TWO_DOUBLE_QUOTED_STATE)
        STATE_DESC(EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE)
        STATE_DESC(EJSON_KEYWORD_STATE)
        STATE_DESC(EJSON_AFTER_KEYWORD_STATE)
        STATE_DESC(EJSON_BYTE_SEQUENCE_STATE)
        STATE_DESC(EJSON_AFTER_BYTE_SEQUENCE_STATE)
        STATE_DESC(EJSON_HEX_BYTE_SEQUENCE_STATE)
        STATE_DESC(EJSON_BINARY_BYTE_SEQUENCE_STATE)
        STATE_DESC(EJSON_BASE64_BYTE_SEQUENCE_STATE)
        STATE_DESC(EJSON_VALUE_NUMBER_STATE)
        STATE_DESC(EJSON_AFTER_VALUE_NUMBER_STATE)
        STATE_DESC(EJSON_VALUE_NUMBER_INTEGER_STATE)
        STATE_DESC(EJSON_VALUE_NUMBER_FRACTION_STATE)
        STATE_DESC(EJSON_VALUE_NUMBER_EXPONENT_STATE)
        STATE_DESC(EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE)
        STATE_DESC(EJSON_VALUE_NUMBER_SUFFIX_INTEGER_STATE)
        STATE_DESC(EJSON_VALUE_NUMBER_INFINITY_STATE)
        STATE_DESC(EJSON_VALUE_NAN_STATE)
        STATE_DESC(EJSON_STRING_ESCAPE_STATE)
        STATE_DESC(EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS_STATE)
    }
    return NULL;
}
