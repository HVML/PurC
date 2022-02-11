/*
 * @file tokenizer.c
 * @author Xue Shuming
 * @date 2022/02/08
 * @brief The implementation of hvml parser.
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

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/dom.h"
#include "private/hvml.h"

#include "hvml-buffer.h"
#include "hvml-rwswrap.h"
#include "hvml-token.h"
#include "hvml-sbst.h"
#include "hvml-attr.h"
#include "hvml-tag.h"

#include <math.h>

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

#include "hvml_err_msgs.inc"

#if HAVE(GLIB)
#define    PCHVML_ALLOC(sz)   g_slice_alloc0(sz)
#define    PCHVML_FREE(p)     g_slice_free1(sizeof(*p), (gpointer)p)
#else
#define    PCHVML_ALLOC(sz)   calloc(1, sz)
#define    PCHVML_FREE(p)     free(p)
#endif

/* Make sure the number of error messages matches the number of error codes */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(msgs,
        PCA_TABLESIZE(hvml_err_msgs) == PCHVML_ERROR_NR);

#undef _COMPILE_TIME_ASSERT

static struct err_msg_seg _hvml_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_FIRST_HVML,
    PURC_ERROR_FIRST_HVML + PCA_TABLESIZE(hvml_err_msgs) - 1,
    hvml_err_msgs
};

void pchvml_init_once(void)
{
    pcinst_register_error_message_segment(&_hvml_err_msgs_seg);
}

struct pchvml_parser* pchvml_create(uint32_t flags, size_t queue_size)
{
    UNUSED_PARAM(flags);
    UNUSED_PARAM(queue_size);

    struct pchvml_parser* parser = (struct pchvml_parser*) PCHVML_ALLOC(
            sizeof(struct pchvml_parser));
    parser->state = PCHVML_DATA_STATE;
    parser->rwswrap = pchvml_rwswrap_new ();
    parser->temp_buffer = pchvml_buffer_new ();
    parser->tag_name = pchvml_buffer_new ();
    parser->string_buffer = pchvml_buffer_new ();
    parser->quoted_buffer = pchvml_buffer_new ();
    parser->vcm_stack = pcvcm_stack_new();
    parser->ejson_stack = pcutils_stack_new(0);
    parser->tag_is_operation = false;
    return parser;
}

void pchvml_reset(struct pchvml_parser* parser, uint32_t flags,
        size_t queue_size)
{
    UNUSED_PARAM(flags);
    UNUSED_PARAM(queue_size);

    parser->state = PCHVML_DATA_STATE;
    pchvml_rwswrap_destroy (parser->rwswrap);
    parser->rwswrap = pchvml_rwswrap_new ();
    pchvml_buffer_reset (parser->temp_buffer);
    pchvml_buffer_reset (parser->tag_name);
    pchvml_buffer_reset (parser->string_buffer);
    pchvml_buffer_reset (parser->quoted_buffer);

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
    parser->vcm_stack = pcvcm_stack_new();
    pcutils_stack_destroy(parser->ejson_stack);
    parser->ejson_stack = pcutils_stack_new(0);
    if (parser->token) {
        pchvml_token_destroy(parser->token);
        parser->token = NULL;
    }
}

void pchvml_destroy(struct pchvml_parser* parser)
{
    if (parser) {
        pchvml_rwswrap_destroy (parser->rwswrap);
        pchvml_buffer_destroy (parser->temp_buffer);
        pchvml_buffer_destroy (parser->tag_name);
        pchvml_buffer_destroy (parser->string_buffer);
        pchvml_buffer_destroy (parser->quoted_buffer);
        if (parser->sbst) {
            pchvml_sbst_destroy(parser->sbst);
        }
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
        if (parser->token) {
            pchvml_token_destroy(parser->token);
        }
        PCHVML_FREE(parser);
    }
}

#define ERROR_NAME(state_name)                                              \
    case state_name:                                                        \
        return ""#state_name;                                               \

const char* pchvml_get_error_name(int err)
{
    switch (err) {
    ERROR_NAME(PCHVML_SUCCESS)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_NULL_CHARACTER)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_QUESTION_MARK_INSTEAD_OF_TAG_NAME)
    ERROR_NAME(PCHVML_ERROR_EOF_BEFORE_TAG_NAME)
    ERROR_NAME(PCHVML_ERROR_MISSING_END_TAG_NAME)
    ERROR_NAME(PCHVML_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME)
    ERROR_NAME(PCHVML_ERROR_EOF_IN_TAG)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_EQUALS_SIGN_BEFORE_ATTRIBUTE_NAME)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_ATTRIBUTE_NAME)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_UNQUOTED_ATTRIBUTE_VALUE)
    ERROR_NAME(PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_ATTRIBUTES)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_SOLIDUS_IN_TAG)
    ERROR_NAME(PCHVML_ERROR_CDATA_IN_HTML_CONTENT)
    ERROR_NAME(PCHVML_ERROR_INCORRECTLY_OPENED_COMMENT)
    ERROR_NAME(PCHVML_ERROR_ABRUPT_CLOSING_OF_EMPTY_COMMENT)
    ERROR_NAME(PCHVML_ERROR_EOF_IN_COMMENT)
    ERROR_NAME(PCHVML_ERROR_EOF_IN_DOCTYPE)
    ERROR_NAME(PCHVML_ERROR_MISSING_WHITESPACE_BEFORE_DOCTYPE_NAME)
    ERROR_NAME(PCHVML_ERROR_MISSING_DOCTYPE_NAME)
    ERROR_NAME(PCHVML_ERROR_INVALID_CHARACTER_SEQUENCE_AFTER_DOCTYPE_NAME)
    ERROR_NAME(PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_PUBLIC_KEYWORD)
    ERROR_NAME(PCHVML_ERROR_MISSING_DOCTYPE_PUBLIC_ID)
    ERROR_NAME(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_ID)
    ERROR_NAME(PCHVML_ERROR_ABRUPT_DOCTYPE_PUBLIC_ID)
    ERROR_NAME(PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_DOCTYPE_PUB_AND_SYS)
    ERROR_NAME(PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_SYSTEM_KEYWORD)
    ERROR_NAME(PCHVML_ERROR_MISSING_DOCTYPE_SYSTEM)
    ERROR_NAME(PCHVML_ERROR_ABRUPT_DOCTYPE_SYSTEM)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_CHARACTER_AFTER_DOCTYPE_SYSTEM)
    ERROR_NAME(PCHVML_ERROR_EOF_IN_CDATA)
    ERROR_NAME(PCHVML_ERROR_UNKNOWN_NAMED_CHARACTER_REFERENCE)
    ERROR_NAME(PCHVML_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_CHARACTER)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_RIGHT_BRACE)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_RIGHT_BRACKET)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_JSON_KEY_NAME)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_COMMA)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD)
    ERROR_NAME(PCHVML_ERROR_UNEXPECTED_BASE64)
    ERROR_NAME(PCHVML_ERROR_BAD_JSON_NUMBER)
    ERROR_NAME(PCHVML_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY)
    ERROR_NAME(PCHVML_ERROR_BAD_JSONEE)
    ERROR_NAME(PCHVML_ERROR_BAD_JSONEE_ESCAPE_ENTITY)
    ERROR_NAME(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME)
    ERROR_NAME(PCHVML_ERROR_EMPTY_JSONEE_NAME)
    ERROR_NAME(PCHVML_ERROR_BAD_JSONEE_NAME)
    ERROR_NAME(PCHVML_ERROR_BAD_JSONEE_KEYWORD)
    ERROR_NAME(PCHVML_ERROR_EMPTY_JSONEE_KEYWORD)
    ERROR_NAME(PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_COMMA)
    ERROR_NAME(PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_PARENTHESIS)
    ERROR_NAME(PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_LEFT_ANGLE_BRACKET)
    ERROR_NAME(PCHVML_ERROR_MISSING_MISSING_ATTRIBUTE_VALUE)
    ERROR_NAME(PCHVML_ERROR_NESTED_COMMENT)
    ERROR_NAME(PCHVML_ERROR_INCORRECTLY_CLOSED_COMMENT)
    ERROR_NAME(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM)
    ERROR_NAME(PCHVML_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE)
    ERROR_NAME(PCHVML_ERROR_CHARACTER_REFERENCE_OUTSIDE_UNICODE_RANGE)
    ERROR_NAME(PCHVML_ERROR_SURROGATE_CHARACTER_REFERENCE)
    ERROR_NAME(PCHVML_ERROR_NONCHARACTER_CHARACTER_REFERENCE)
    ERROR_NAME(PCHVML_ERROR_NULL_CHARACTER_REFERENCE)
    ERROR_NAME(PCHVML_ERROR_CONTROL_CHARACTER_REFERENCE)
    ERROR_NAME(PCHVML_ERROR_INVALID_UTF8_CHARACTER)
    }
    return NULL;
}
