/*
 * @file tokenizer.c
 * @author Xue Shuming
 * @date 2022/02/08
 * @brief The implementation of hvml tokenizer.
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
#include "private/dom.h"
#include "private/hvml.h"

#include "hvml-buffer.h"
#include "hvml-rwswrap.h"
#include "hvml-token.h"
#include "hvml-sbst.h"
#include "hvml-attr.h"
#include "hvml-tag.h"

#ifdef USE_NEW_TOKENIZER

#include <math.h>

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

#define PCHVML_NEXT_TOKEN_BEGIN                                         \
struct pchvml_token* pchvml_next_token(struct pchvml_parser* parser,    \
                                          purc_rwstream_t rws)          \
{                                                                       \
    struct pchvml_uc* hvml_uc = NULL;                                   \
    uint32_t character = 0;                                             \
    if (parser->token) {                                                \
        struct pchvml_token* token = parser->token;                     \
        parser->token = NULL;                                           \
        return token;                                                   \
    }                                                                   \
                                                                        \
    pchvml_rwswrap_set_rwstream (parser->rwswrap, rws);                 \
                                                                        \
next_input:                                                             \
    hvml_uc = pchvml_rwswrap_next_char (parser->rwswrap);               \
    if (!hvml_uc) {                                                     \
        return NULL;                                                    \
    }                                                                   \
                                                                        \
    character = hvml_uc->character;                                     \
    if (character == PCHVML_INVALID_CHARACTER) {                        \
        SET_ERR(PCHVML_ERROR_INVALID_UTF8_CHARACTER);                   \
        return NULL;                                                    \
    }                                                                   \
                                                                        \
    if (is_separator(character)) {                                      \
        if (parser->prev_separator == ',' && character == ',') {        \
            SET_ERR(PCHVML_ERROR_UNEXPECTED_COMMA);                     \
            return NULL;                                                \
        }                                                               \
        parser->prev_separator = character;                             \
    }                                                                   \
    else if (!is_whitespace(character)) {                               \
        parser->prev_separator = 0;                                     \
    }                                                                   \
                                                                        \
next_state:                                                             \
    switch (parser->state) {


#define PCHVML_NEXT_TOKEN_END                                           \
    default:                                                            \
        break;                                                          \
    }                                                                   \
    return NULL;                                                        \
}

#define TEMP_BUFFER_TO_VCM_NODE()                                       \
        pchvml_buffer_to_vcm_node(parser->temp_buffer)

#define HVML_DEBUG_PRINT

#ifdef HVML_DEBUG_PRINT
#define PRINT_STATE(state_name)                                             \
    fprintf(stderr, \
            "in %s|uc=%c|hex=0x%X|stack_is_empty=%d|stack_top=%c|vcm_node->type=%d\n",                              \
            pchvml_get_state_name(state_name), character, character,     \
            ejson_stack_is_empty(), (char)ejson_stack_top(),                \
            (parser->vcm_node != NULL ? (int)parser->vcm_node->type : -1));
#define SET_ERR(err)    do {                                                \
    fprintf(stderr, "error %s:%d %s\n", __FILE__, __LINE__,                 \
            pchvml_get_error_name(err));                                    \
    pcinst_set_error (err);                                                 \
} while (0)
#else
#define PRINT_STATE(state_name)
#define SET_ERR(err)    pcinst_set_error(err)
#endif

#define ejson_stack_is_empty()  pcutils_stack_is_empty(parser->ejson_stack)
#define ejson_stack_top()  pcutils_stack_top(parser->ejson_stack)
#define ejson_stack_pop()  pcutils_stack_pop(parser->ejson_stack)
#define ejson_stack_push(c) pcutils_stack_push(parser->ejson_stack, c)

#define vcm_stack_is_empty() pcvcm_stack_is_empty(parser->vcm_stack)
#define vcm_stack_push(c) pcvcm_stack_push(parser->vcm_stack, c)
#define vcm_stack_pop() pcvcm_stack_pop(parser->vcm_stack)

#define BEGIN_STATE(state_name)                                             \
    case state_name:                                                        \
    {                                                                       \
        enum pchvml_state curr_state = state_name;                          \
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

#define SET_TRANSIT_STATE(new_state)                                        \
    do {                                                                    \
        parser->transit_state = new_state;                                  \
    } while (false)

#define CHECK_TEMPLATE_TAG_AND_SWITCH_STATE(token)                          \
    do {                                                                    \
        const char* name = pchvml_token_get_name(token);                    \
        if (pchvml_token_is_type(token, PCHVML_TOKEN_START_TAG) &&          \
                pchvml_parser_is_template_tag(name)) {                      \
            parser->state = HVML_EJSON_DATA_STATE;                        \
        }                                                                   \
    } while (false)

#define RETURN_AND_SWITCH_TO(next_state)                                    \
    do {                                                                    \
        parser->state = next_state;                                         \
        pchvml_parser_save_tag_name(parser);                                \
        pchvml_token_done(parser->token);                                   \
        struct pchvml_token* token = parser->token;                         \
        parser->token = NULL;                                               \
        CHECK_TEMPLATE_TAG_AND_SWITCH_STATE(token);                         \
        return token;                                                       \
    } while (false)

#define RETURN_AND_RECONSUME_IN(next_state)                                 \
    do {                                                                    \
        parser->state = next_state;                                         \
        pchvml_parser_save_tag_name(parser);                                \
        pchvml_token_done(parser->token);                                   \
        struct pchvml_token* token = parser->token;                         \
        parser->token = NULL;                                               \
        return token;                                                       \
    } while (false)

#define RETURN_CURRENT_TOKEN()                                              \
    do {                                                                    \
        pchvml_token_done(parser->token);                                   \
        struct pchvml_token* token = parser->token;                         \
        parser->token = NULL;                                               \
        return token;                                                       \
    } while (false)

#define RETURN_NEW_EOF_TOKEN()                                              \
    do {                                                                    \
        if (parser->token) {                                                \
            struct pchvml_token* token = parser->token;                     \
            parser->token = pchvml_token_new_eof();                         \
            return token;                                                   \
        }                                                                   \
        return pchvml_token_new_eof();                                      \
    } while (false)

#define RETURN_MULTIPLE_AND_SWITCH_TO(token, next_token, next_state)        \
    do {                                                                    \
        parser->state = next_state;                                         \
        pchvml_token_done(token);                                           \
        pchvml_token_done(next_token);                                      \
        parser->token = next_token;                                         \
        return token;                                                       \
    } while (false)

#define RETURN_AND_STOP_PARSE()                                             \
    do {                                                                    \
        return NULL;                                                        \
    } while (false)

#define RESET_TEMP_BUFFER()                                                 \
    do {                                                                    \
        pchvml_buffer_reset(parser->temp_buffer);                           \
    } while (false)

#define APPEND_TO_TEMP_BUFFER(c)                                            \
    do {                                                                    \
        pchvml_buffer_append(parser->temp_buffer, c);                       \
    } while (false)

#define APPEND_BYTES_TO_TEMP_BUFFER(bytes, nr_bytes)                        \
    do {                                                                    \
        pchvml_buffer_append_bytes(parser->temp_buffer, bytes, nr_bytes);   \
    } while (false)

#define APPEND_BUFFER_TO_TEMP_BUFFER(buffer)                                \
    do {                                                                    \
        pchvml_buffer_append_another(parser->temp_buffer, buffer);          \
    } while (false)

#define IS_TEMP_BUFFER_EMPTY()                                              \
        pchvml_buffer_is_empty(parser->temp_buffer)

#define RESET_STRING_BUFFER()                                               \
    do {                                                                    \
        pchvml_buffer_reset(parser->string_buffer);                         \
    } while (false)

#define APPEND_TO_STRING_BUFFER(uc)                                         \
    do {                                                                    \
        pchvml_buffer_append(parser->string_buffer, uc);                    \
    } while (false)

#define RESET_QUOTED_BUFFER()                                               \
    do {                                                                    \
        pchvml_buffer_reset(parser->quoted_buffer);                         \
    } while (false)

#define APPEND_TO_QUOTED_BUFFER(uc)                                         \
    do {                                                                    \
        pchvml_buffer_append(parser->quoted_buffer, uc);                    \
    } while (false)

#define APPEND_TO_TOKEN_NAME(uc)                                            \
    do {                                                                    \
        pchvml_token_append_to_name(parser->token, uc);                     \
    } while (false)

// TODO
#define APPEND_TO_TOKEN_TEXT(uc)                                            \
    do {                                                                    \
        if (parser->token == NULL) {                                        \
            parser->token = pchvml_token_new (PCHVML_TOKEN_CHARACTER);      \
        }                                                                   \
        pchvml_token_append_to_text(parser->token, uc);                     \
    } while (false)

// TODO
#define APPEND_BYTES_TO_TOKEN_TEXT(c, nr_c)                                 \
    do {                                                                    \
        pchvml_token_append_bytes_to_text(parser->token, c, nr_c);          \
    } while (false)

// TODO
#define APPEND_TEMP_BUFFER_TO_TOKEN_TEXT()                                  \
    do {                                                                    \
        const char* c = pchvml_buffer_get_buffer(parser->temp_buffer);      \
        size_t nr_c = pchvml_buffer_get_size_in_bytes(                      \
                parser->temp_buffer);                                       \
        pchvml_token_append_bytes_to_text(parser->token, c, nr_c);          \
        pchvml_buffer_reset(parser->temp_buffer);                           \
    } while (false)

#define APPEND_TO_TOKEN_PUBLIC_ID(uc)                                       \
    do {                                                                    \
        pchvml_token_append_to_public_identifier(parser->token, uc);        \
    } while (false)

#define RESET_TOKEN_PUBLIC_ID()                                             \
    do {                                                                    \
        pchvml_token_reset_public_identifier(parser->token);                \
    } while (false)

#define APPEND_TO_TOKEN_SYSTEM_INFO(uc)                                     \
    do {                                                                    \
        pchvml_token_append_to_system_information(parser->token, uc);       \
    } while (false)

#define RESET_TOKEN_SYSTEM_INFO()                                           \
    do {                                                                    \
        pchvml_token_reset_system_information(parser->token);               \
    } while (false)

#define BEGIN_TOKEN_ATTR()                                                  \
    do {                                                                    \
        pchvml_token_begin_attr(parser->token);                             \
    } while (false)

#define END_TOKEN_ATTR()                                                    \
    do {                                                                    \
        pchvml_token_end_attr(parser->token);                               \
    } while (false)

// TODO
#define APPEND_TO_TOKEN_ATTR_VALUE(uc)                                      \
    do {                                                                    \
        pchvml_token_append_to_attr_value(parser->token, uc);               \
    } while (false)

// TODO:
#define APPEND_BUFFER_TO_TOKEN_ATTR_VALUE(buffer)                           \
    do {                                                                    \
        const char* c = pchvml_buffer_get_buffer(buffer);                   \
        size_t nr_c = pchvml_buffer_get_size_in_bytes(buffer);              \
        pchvml_token_append_bytes_to_attr_value(parser->token, c, nr_c);    \
    } while (false)

#define APPEND_TO_TOKEN_ATTR_NAME(c)                                        \
    do {                                                                    \
        pchvml_token_append_to_attr_name(parser->token, c);                 \
    } while (false)

#define APPEND_TEMP_BUFFER_TO_TOKEN_ATTR_NAME()                             \
    do {                                                                    \
        const char* c = pchvml_buffer_get_buffer(parser->temp_buffer);      \
        size_t nr_c = pchvml_buffer_get_size_in_bytes(                      \
                parser->temp_buffer);                                       \
        pchvml_token_append_bytes_to_attr_name(parser->token, c, nr_c);     \
        pchvml_buffer_reset(parser->temp_buffer);                           \
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
        struct pcvcm_node* child = parser->vcm_node;                        \
        APPEND_CHILD(parent, child);                                        \
        UPDATE_VCM_NODE(parent);                                            \
    } while (false)

static const uint32_t numeric_char_ref_extension_array[32] = {
    0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, // 80-87
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F, // 88-8F
    0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, // 90-97
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178, // 98-9F
};

static UNUSED_FUNCTION
bool pchvml_parser_is_json_content_tag(const char* name)
{
    if (!name) {
        return false;
    }
    return (strcmp(name, "init") == 0 || strcmp(name, "archedata") == 0);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_operation_tag(const char* name)
{
    if (!name) {
        return false;
    }
    const struct pchvml_tag_entry* entry = pchvml_tag_static_search(name,
            strlen(name));
    return (entry &&
            (entry->cats & (PCHVML_TAGCAT_TEMPLATE | PCHVML_TAGCAT_VERB)));
}

static UNUSED_FUNCTION
void pchvml_parser_save_tag_name(struct pchvml_parser* parser)
{
    if (pchvml_token_is_type (parser->token, PCHVML_TOKEN_START_TAG)) {
        const char* name = pchvml_token_get_name(parser->token);
        parser->tag_is_operation = pchvml_parser_is_operation_tag(name);
        pchvml_buffer_reset(parser->tag_name);
        pchvml_buffer_append_bytes(parser->tag_name,
                name, strlen(name));
    }
    else {
        pchvml_buffer_reset(parser->tag_name);
        parser->tag_is_operation = false;
    }
}

static UNUSED_FUNCTION
bool pchvml_parser_is_appropriate_end_tag(struct pchvml_parser* parser)
{
    const char* name = pchvml_token_get_name(parser->token);
    return pchvml_buffer_equal_to (parser->tag_name, name,
            strlen(name));
}

static UNUSED_FUNCTION
bool pchvml_parser_is_appropriate_tag_name(struct pchvml_parser* parser,
        const char* name)
{
    return pchvml_buffer_equal_to (parser->tag_name, name,
            strlen(name));
}

static UNUSED_FUNCTION
bool pchvml_parser_is_operation_tag_token (struct pchvml_token* token)
{
    const char* name = pchvml_token_get_name(token);
    return pchvml_parser_is_operation_tag(name);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_json_content_tag_token (struct pchvml_token* token)
{
    const char* name = pchvml_token_get_name(token);
    return pchvml_parser_is_json_content_tag(name);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_ordinary_attribute (struct pchvml_token_attr* attr)
{
    const char* name = pchvml_token_attr_get_name(attr);
    const struct pchvml_attr_entry* entry =pchvml_attr_static_search(name,
            strlen(name));
    return (entry && entry->type == PCHVML_ATTR_TYPE_ORDINARY);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_preposition_attribute (
        struct pchvml_token_attr* attr)
{
    const char* name = pchvml_token_attr_get_name(attr);
    const struct pchvml_attr_entry* entry =pchvml_attr_static_search(name,
            strlen(name));
    return (entry && entry->type == PCHVML_ATTR_TYPE_PREP);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_template_tag (const char* name)
{
    const struct pchvml_tag_entry* entry = pchvml_tag_static_search(name,
            strlen(name));
    bool ret = (entry && (entry->id == PCHVML_TAG_ARCHETYPE
                || entry->id == PCHVML_TAG_ERROR
                || entry->id == PCHVML_TAG_EXCEPT));
    return ret;
}

static UNUSED_FUNCTION
bool pchvml_parser_is_in_template (struct pchvml_parser* parser)
{
    const char* name = pchvml_buffer_get_buffer(parser->tag_name);
    return pchvml_parser_is_template_tag(name);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_in_json_content_tag (struct pchvml_parser* parser)
{
    const char* name = pchvml_buffer_get_buffer(parser->tag_name);
    return pchvml_parser_is_json_content_tag(name);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_handle_as_jsonee(struct pchvml_token* token, uint32_t uc)
{
    if (!(uc == '[' || uc == '{' || uc == '$')) {
        return false;
    }

    struct pchvml_token_attr* attr = pchvml_token_get_curr_attr(token);
    const char* name = pchvml_token_attr_get_name(attr);
    if (pchvml_parser_is_operation_tag_token(token)
            && (strcmp(name, "on") == 0 || strcmp(name, "with") == 0)) {
        return true;
    }
    const char* token_name = pchvml_token_get_name(token);
    if (strcmp(name, "via") == 0 && (
                strcmp(token_name, "choose") == 0 ||
                strcmp(token_name, "iterate") == 0 ||
                strcmp(token_name, "reduce") == 0 ||
                strcmp(token_name, "update") == 0)) {
        return true;
    }
    return false;
}

static UNUSED_FUNCTION
struct pcvcm_node* pchvml_parser_new_byte_sequence (struct pchvml_parser* hvml,
    struct pchvml_buffer* buffer)
{
    UNUSED_PARAM(hvml);
    UNUSED_PARAM(buffer);
    size_t nr_bytes = pchvml_buffer_get_size_in_bytes(buffer);
    const char* bytes = pchvml_buffer_get_buffer(buffer);
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

static UNUSED_FUNCTION
struct pcvcm_node* pchvml_buffer_to_vcm_node(struct pchvml_buffer* buffer)
{
    return buffer ? pcvcm_node_new_string(
                    pchvml_buffer_get_buffer(buffer)) : NULL;
}

static UNUSED_FUNCTION
bool pchvml_parser_is_in_attribute (struct pchvml_parser* parser)
{
    return parser->token && pchvml_token_is_in_attr(parser->token);
}


PCHVML_NEXT_TOKEN_BEGIN


BEGIN_STATE(HVML_DATA_STATE)
    if (character == '&') {
        SET_RETURN_STATE(HVML_DATA_STATE);
        ADVANCE_TO(HVML_CHARACTER_REFERENCE_STATE);
    }
    if (character == '<') {
        if (parser->token) {
            RETURN_AND_SWITCH_TO(HVML_TAG_OPEN_STATE);
        }
        ADVANCE_TO(HVML_TAG_OPEN_STATE);
    }
    if (is_eof(character)) {
        RETURN_NEW_EOF_TOKEN();
    }
    RESET_TEMP_BUFFER();
    RECONSUME_IN(HVML_TAG_CONTENT_STATE);
END_STATE()

BEGIN_STATE(HVML_TAG_OPEN_STATE)
    if (character == '!') {
        ADVANCE_TO(HVML_MARKUP_DECLARATION_OPEN_STATE);
    }
    if (character == '/') {
        ADVANCE_TO(HVML_END_TAG_OPEN_STATE);
    }
    if (is_ascii_alpha(character)) {
        parser->token = pchvml_token_new_start_tag ();
        RECONSUME_IN(HVML_TAG_NAME_STATE);
    }
    if (character == '?') {
        SET_ERR(PCHVML_ERROR_UNEXPECTED_QUESTION_MARK_INSTEAD_OF_TAG_NAME);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_BEFORE_TAG_NAME);
        RETURN_AND_STOP_PARSE();
    }
    SET_ERR(PCHVML_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_END_TAG_OPEN_STATE)
    if (is_ascii_alpha(character)) {
        parser->token = pchvml_token_new_end_tag();
        RECONSUME_IN(HVML_TAG_NAME_STATE);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_MISSING_END_TAG_NAME);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_BEFORE_TAG_NAME);
        RETURN_AND_STOP_PARSE();
    }
    SET_ERR(PCHVML_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_TAG_CONTENT_STATE)
    if (is_eof(character)) {
        RETURN_NEW_EOF_TOKEN();
    }
    if (is_whitespace(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_TAG_CONTENT_STATE);
    }
    if (!IS_TEMP_BUFFER_EMPTY()) {
        struct pcvcm_node* node = TEMP_BUFFER_TO_VCM_NODE();
        if (!node) {
            RETURN_AND_STOP_PARSE();
        }
        RESET_TEMP_BUFFER();
        parser->token = pchvml_token_new_vcm(parser->vcm_node);
        RETURN_CURRENT_TOKEN();
    }
    if (pchvml_parser_is_in_json_content_tag(parser)) {
        RECONSUME_IN(HVML_JSONTEXT_CONTENT_STATE);
    }
    RECONSUME_IN(HVML_TEXT_CONTENT_STATE);
END_STATE()

BEGIN_STATE(HVML_TAG_NAME_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_BEFORE_ATTRIBUTE_NAME_STATE);
    }
    if (character == '/') {
        ADVANCE_TO(HVML_SELF_CLOSING_START_TAG_STATE);
    }
    if (character == '>') {
        RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TOKEN_NAME(character);
    ADVANCE_TO(HVML_TAG_NAME_STATE);
END_STATE()

BEGIN_STATE(HVML_BEFORE_ATTRIBUTE_NAME_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_BEFORE_ATTRIBUTE_NAME_STATE);
    }
    if (character == '/' || character == '>') {
        RECONSUME_IN(HVML_AFTER_ATTRIBUTE_NAME_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '=') {
        SET_ERR(PCHVML_ERROR_UNEXPECTED_EQUALS_SIGN_BEFORE_ATTRIBUTE_NAME);
        RETURN_AND_STOP_PARSE();
    }
    BEGIN_TOKEN_ATTR();
    RECONSUME_IN(HVML_ATTRIBUTE_NAME_STATE);
END_STATE()

BEGIN_STATE(HVML_ATTRIBUTE_NAME_STATE)
    if (is_whitespace(character) || character == '>') {
        RECONSUME_IN(HVML_AFTER_ATTRIBUTE_NAME_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '=') {
        ADVANCE_TO(HVML_BEFORE_ATTRIBUTE_VALUE_STATE);
    }
    if (character == '"' || character == '\'' || character == '<') {
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_ATTRIBUTE_NAME);
        RETURN_AND_STOP_PARSE();
    }
    if (is_attribute_value_operator(character)
            && pchvml_parser_is_operation_tag_token(parser->token)) {
        RESET_TEMP_BUFFER();
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(
        HVML_SPECIAL_ATTRIBUTE_OPERATOR_IN_ATTRIBUTE_NAME_STATE);
    }
    if (character == '/') {
        RECONSUME_IN(HVML_AFTER_ATTRIBUTE_NAME_STATE);
    }
    APPEND_TO_TOKEN_ATTR_NAME(character);
    ADVANCE_TO(HVML_ATTRIBUTE_NAME_STATE);
END_STATE()

BEGIN_STATE(HVML_AFTER_ATTRIBUTE_NAME_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_AFTER_ATTRIBUTE_NAME_STATE);
    }
    if (character == '=') {
        ADVANCE_TO(HVML_BEFORE_ATTRIBUTE_VALUE_STATE);
    }
    if (character == '>') {
        END_TOKEN_ATTR();
        RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_NEW_EOF_TOKEN();
    }
    if (is_attribute_value_operator(character)
            && pchvml_parser_is_operation_tag_token(parser->token)) {
        RESET_TEMP_BUFFER();
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(
        HVML_SPECIAL_ATTRIBUTE_OPERATOR_AFTER_ATTRIBUTE_NAME_STATE
        );
    }
    if (pchvml_parser_is_operation_tag_token(parser->token)
        && pchvml_parser_is_preposition_attribute(
                pchvml_token_get_curr_attr(parser->token))) {
        RECONSUME_IN(HVML_BEFORE_ATTRIBUTE_VALUE_STATE);
    }
    if (character == '/') {
        END_TOKEN_ATTR();
        ADVANCE_TO(HVML_SELF_CLOSING_START_TAG_STATE);
    }
    END_TOKEN_ATTR();
    BEGIN_TOKEN_ATTR();
    RECONSUME_IN(HVML_ATTRIBUTE_NAME_STATE);
END_STATE()

BEGIN_STATE(HVML_BEFORE_ATTRIBUTE_VALUE_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_BEFORE_ATTRIBUTE_VALUE_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_NEW_EOF_TOKEN();
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_MISSING_MISSING_ATTRIBUTE_VALUE);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '"') {
        ADVANCE_TO(HVML_JSONEE_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE);
    }
    if (character == '\'') {
        ADVANCE_TO(HVML_JSONEE_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE);
    }
    RECONSUME_IN(HVML_JSONEE_ATTRIBUTE_VALUE_UNQUOTED_STATE);
END_STATE()

BEGIN_STATE(HVML_AFTER_ATTRIBUTE_VALUE_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_BEFORE_ATTRIBUTE_NAME_STATE);
    }
    if (character == '/') {
        ADVANCE_TO(HVML_SELF_CLOSING_START_TAG_STATE);
    }
    if (character == '>') {
        RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_NEW_EOF_TOKEN();
    }
    SET_ERR(PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_ATTRIBUTES);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_SELF_CLOSING_START_TAG_STATE)
    if (character == '>') {
        pchvml_token_set_self_closing(parser->token, true);
        RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_NEW_EOF_TOKEN();
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_SOLIDUS_IN_TAG);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_MARKUP_DECLARATION_OPEN_STATE)
    if (parser->sbst == NULL) {
        parser->sbst = pchvml_sbst_new_markup_declaration_open_state();
    }
    bool ret = pchvml_sbst_advance_ex(parser->sbst, character, false);
    if (!ret) {
        SET_ERR(PCHVML_ERROR_INCORRECTLY_OPENED_COMMENT);
        pchvml_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        RETURN_AND_STOP_PARSE();
    }

    const char* value = pchvml_sbst_get_match(parser->sbst);
    if (value == NULL) {
        ADVANCE_TO(HVML_MARKUP_DECLARATION_OPEN_STATE);
    }

    if (strcmp(value, "--") == 0) {
        parser->token = pchvml_token_new_comment();
        pchvml_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        ADVANCE_TO(HVML_COMMENT_START_STATE);
    }
    if (strcmp(value, "DOCTYPE") == 0) {
        pchvml_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        ADVANCE_TO(HVML_DOCTYPE_STATE);
    }
    if (strcmp(value, "[CDATA[") == 0) {
        pchvml_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        ADVANCE_TO(HVML_CDATA_SECTION_STATE);
    }
    SET_ERR(PCHVML_ERROR_INCORRECTLY_OPENED_COMMENT);
    pchvml_sbst_destroy(parser->sbst);
    parser->sbst = NULL;
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_COMMENT_START_STATE)
    if (character == '-') {
        ADVANCE_TO(HVML_COMMENT_START_DASH_STATE);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_ABRUPT_CLOSING_OF_EMPTY_COMMENT);
        RETURN_AND_STOP_PARSE();
    }
    RECONSUME_IN(HVML_COMMENT_STATE);
END_STATE()

BEGIN_STATE(HVML_COMMENT_START_DASH_STATE)
    if (character == '-') {
        ADVANCE_TO(HVML_COMMENT_END_STATE);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_ABRUPT_CLOSING_OF_EMPTY_COMMENT);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_COMMENT);
        RETURN_NEW_EOF_TOKEN();
    }
    APPEND_TO_TOKEN_TEXT('-');
    RECONSUME_IN(HVML_COMMENT_STATE);
END_STATE()

BEGIN_STATE(HVML_COMMENT_STATE)
    if (character == '<') {
        APPEND_TO_TOKEN_TEXT(character);
        ADVANCE_TO(HVML_COMMENT_LESS_THAN_SIGN_STATE);
    }
    if (character == '-') {
        ADVANCE_TO(HVML_COMMENT_END_DASH_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_COMMENT);
        RETURN_NEW_EOF_TOKEN();
    }
    APPEND_TO_TOKEN_TEXT(character);
    ADVANCE_TO(HVML_COMMENT_STATE);
END_STATE()

BEGIN_STATE(HVML_COMMENT_LESS_THAN_SIGN_STATE)
    if (character == '!') {
        APPEND_TO_TOKEN_TEXT(character);
        ADVANCE_TO(HVML_COMMENT_LESS_THAN_SIGN_BANG_STATE);
    }
    if (character == '<') {
        APPEND_TO_TOKEN_TEXT(character);
        ADVANCE_TO(HVML_COMMENT_LESS_THAN_SIGN_STATE);
    }
    RECONSUME_IN(HVML_COMMENT_STATE);
END_STATE()

BEGIN_STATE(HVML_COMMENT_LESS_THAN_SIGN_BANG_STATE)
    if (character == '-') {
        ADVANCE_TO(HVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_STATE);
    }
    RECONSUME_IN(HVML_COMMENT_STATE);
END_STATE()

BEGIN_STATE(HVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_STATE)
    if (character == '-') {
        ADVANCE_TO(HVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_DASH_STATE);
    }
    RECONSUME_IN(HVML_COMMENT_END_DASH_STATE);
END_STATE()

BEGIN_STATE(HVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_DASH_STATE)
    if (character == '>' || is_eof(character)) {
        RECONSUME_IN(HVML_COMMENT_END_STATE);
    }
    SET_ERR(PCHVML_ERROR_NESTED_COMMENT);
    RECONSUME_IN(HVML_COMMENT_END_STATE);
END_STATE()

BEGIN_STATE(HVML_COMMENT_END_DASH_STATE)
    if (character == '-') {
        ADVANCE_TO(HVML_COMMENT_END_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_COMMENT);
        RETURN_NEW_EOF_TOKEN();
    }
    APPEND_TO_TOKEN_TEXT('-');
    RECONSUME_IN(HVML_COMMENT_STATE);
END_STATE()

BEGIN_STATE(HVML_COMMENT_END_STATE)
    if (character == '>') {
        const char* text = pchvml_token_get_text(parser->token);
        if (!text) {
            APPEND_BYTES_TO_TOKEN_TEXT(NULL, 0);
        }
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    if (character == '!') {
        ADVANCE_TO(HVML_COMMENT_END_BANG_STATE);
    }
    if (character == '-') {
        APPEND_TO_TOKEN_TEXT('-');
        ADVANCE_TO(HVML_COMMENT_END_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_COMMENT);
        RETURN_NEW_EOF_TOKEN();
    }
    APPEND_TO_TOKEN_TEXT('-');
    APPEND_TO_TOKEN_TEXT('-');
    RECONSUME_IN(HVML_COMMENT_STATE);
END_STATE()

BEGIN_STATE(HVML_COMMENT_END_BANG_STATE)
    if (character == '-') {
        APPEND_TO_TOKEN_TEXT('-');
        APPEND_TO_TOKEN_TEXT('-');
        APPEND_TO_TOKEN_TEXT('!');
        ADVANCE_TO(HVML_COMMENT_END_DASH_STATE);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_INCORRECTLY_CLOSED_COMMENT);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_COMMENT);
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    APPEND_TO_TOKEN_TEXT('-');
    APPEND_TO_TOKEN_TEXT('-');
    APPEND_TO_TOKEN_TEXT('!');
    RECONSUME_IN(HVML_COMMENT_STATE);
END_STATE()

BEGIN_STATE(HVML_DOCTYPE_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_BEFORE_DOCTYPE_NAME_STATE);
    }
    if (character == '>') {
        RECONSUME_IN(HVML_BEFORE_DOCTYPE_NAME_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        parser->token = pchvml_token_new_doctype();
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    SET_ERR(PCHVML_ERROR_MISSING_WHITESPACE_BEFORE_DOCTYPE_NAME);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_BEFORE_DOCTYPE_NAME_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_BEFORE_DOCTYPE_NAME_STATE);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_MISSING_DOCTYPE_NAME);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        parser->token = pchvml_token_new_doctype();
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    parser->token = pchvml_token_new_doctype();
    APPEND_TO_TOKEN_NAME(character);
    ADVANCE_TO(HVML_DOCTYPE_NAME_STATE);
END_STATE()

BEGIN_STATE(HVML_DOCTYPE_NAME_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_AFTER_DOCTYPE_NAME_STATE);
    }
    if (character == '>') {
        RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    APPEND_TO_TOKEN_NAME(character);
    ADVANCE_TO(HVML_DOCTYPE_NAME_STATE);
END_STATE()

BEGIN_STATE(HVML_AFTER_DOCTYPE_NAME_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_AFTER_DOCTYPE_NAME_STATE);
    }
    if (character == '>') {
        RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    if (parser->sbst == NULL) {
        parser->sbst = pchvml_sbst_new_after_doctype_name_state();
    }
    bool ret = pchvml_sbst_advance_ex(parser->sbst, character, true);
    if (!ret) {
        SET_ERR(PCHVML_ERROR_INVALID_CHARACTER_SEQUENCE_AFTER_DOCTYPE_NAME);
        pchvml_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        RETURN_AND_STOP_PARSE();
    }

    const char* value = pchvml_sbst_get_match(parser->sbst);
    if (value == NULL) {
        ADVANCE_TO(HVML_AFTER_DOCTYPE_NAME_STATE);
    }

    if (strcmp(value, "PUBLIC") == 0) {
        pchvml_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        ADVANCE_TO(HVML_AFTER_DOCTYPE_PUBLIC_KEYWORD_STATE);
    }
    if (strcmp(value, "SYSTEM") == 0) {
        pchvml_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        ADVANCE_TO(HVML_AFTER_DOCTYPE_SYSTEM_KEYWORD_STATE);
    }
    SET_ERR(PCHVML_ERROR_INVALID_CHARACTER_SEQUENCE_AFTER_DOCTYPE_NAME);
    pchvml_sbst_destroy(parser->sbst);
    parser->sbst = NULL;
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_AFTER_DOCTYPE_PUBLIC_KEYWORD_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_BEFORE_DOCTYPE_PUBLIC_ID_STATE);
    }
    if (character == '"') {
        SET_ERR(
          PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_PUBLIC_KEYWORD);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '\'') {
        SET_ERR(
          PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_PUBLIC_KEYWORD);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_MISSING_DOCTYPE_PUBLIC_ID);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    SET_ERR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_ID);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_BEFORE_DOCTYPE_PUBLIC_ID_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_BEFORE_DOCTYPE_PUBLIC_ID_STATE);
    }
    if (character == '"') {
        RESET_TOKEN_PUBLIC_ID();
        ADVANCE_TO(HVML_DOCTYPE_PUBLIC_ID_DOUBLE_QUOTED_STATE);
    }
    if (character == '\'') {
        RESET_TOKEN_PUBLIC_ID();
        ADVANCE_TO(HVML_DOCTYPE_PUBLIC_ID_SINGLE_QUOTED_STATE);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_MISSING_DOCTYPE_PUBLIC_ID);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    SET_ERR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_ID);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_DOCTYPE_PUBLIC_ID_DOUBLE_QUOTED_STATE)
    if (character == '"') {
        ADVANCE_TO(HVML_AFTER_DOCTYPE_PUBLIC_ID_STATE);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_ABRUPT_DOCTYPE_PUBLIC_ID);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    APPEND_TO_TOKEN_PUBLIC_ID(character);
    ADVANCE_TO(HVML_DOCTYPE_PUBLIC_ID_DOUBLE_QUOTED_STATE);
END_STATE()

BEGIN_STATE(HVML_DOCTYPE_PUBLIC_ID_SINGLE_QUOTED_STATE)
    if (character == '\'') {
        ADVANCE_TO(HVML_AFTER_DOCTYPE_PUBLIC_ID_STATE);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_ABRUPT_DOCTYPE_PUBLIC_ID);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    APPEND_TO_TOKEN_PUBLIC_ID(character);
    ADVANCE_TO(HVML_DOCTYPE_PUBLIC_ID_SINGLE_QUOTED_STATE);
END_STATE()

BEGIN_STATE(HVML_AFTER_DOCTYPE_PUBLIC_ID_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_BETWEEN_DOCTYPE_PUBLIC_ID_AND_SYSTEM_INFO_STATE);
    }
    if (character == '>') {
        RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
    }
    if (character == '"') {
        SET_ERR(
          PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_DOCTYPE_PUB_AND_SYS);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '\'') {
        SET_ERR(
          PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_DOCTYPE_PUB_AND_SYS);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    SET_ERR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_BETWEEN_DOCTYPE_PUBLIC_ID_AND_SYSTEM_INFO_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_BETWEEN_DOCTYPE_PUBLIC_ID_AND_SYSTEM_INFO_STATE);
    }
    if (character == '>') {
        RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
    }
    if (character == '"') {
        RESET_TOKEN_SYSTEM_INFO();
        ADVANCE_TO(HVML_DOCTYPE_SYSTEM_DOUBLE_QUOTED_STATE);
    }
    if (character == '\'') {
        RESET_TOKEN_SYSTEM_INFO();
        ADVANCE_TO(HVML_DOCTYPE_SYSTEM_SINGLE_QUOTED_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    RECONSUME_IN(HVML_AFTER_DOCTYPE_NAME_STATE);
    //SET_ERR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM);
    //RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_AFTER_DOCTYPE_SYSTEM_KEYWORD_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_BEFORE_DOCTYPE_SYSTEM_STATE);
    }
    if (character == '"') {
        SET_ERR(
          PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_SYSTEM_KEYWORD);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '\'') {
        SET_ERR(
            PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_SYSTEM_KEYWORD);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_MISSING_DOCTYPE_SYSTEM);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    SET_ERR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_BEFORE_DOCTYPE_SYSTEM_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_BEFORE_DOCTYPE_SYSTEM_STATE);
    }
    if (character == '"') {
        RESET_TOKEN_SYSTEM_INFO();
        ADVANCE_TO(HVML_DOCTYPE_SYSTEM_DOUBLE_QUOTED_STATE);
    }
    if (character == '\'') {
        RESET_TOKEN_SYSTEM_INFO();
        ADVANCE_TO(HVML_DOCTYPE_SYSTEM_SINGLE_QUOTED_STATE);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_MISSING_DOCTYPE_SYSTEM);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    SET_ERR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_DOCTYPE_SYSTEM_DOUBLE_QUOTED_STATE)
    if (character == '"') {
        ADVANCE_TO(HVML_AFTER_DOCTYPE_SYSTEM_STATE);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_ABRUPT_DOCTYPE_SYSTEM);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    APPEND_TO_TOKEN_SYSTEM_INFO(character);
    ADVANCE_TO(HVML_DOCTYPE_SYSTEM_DOUBLE_QUOTED_STATE);
END_STATE()

BEGIN_STATE(HVML_DOCTYPE_SYSTEM_SINGLE_QUOTED_STATE)
    if (character == '\'') {
        ADVANCE_TO(HVML_AFTER_DOCTYPE_SYSTEM_STATE);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_ABRUPT_DOCTYPE_SYSTEM);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    APPEND_TO_TOKEN_SYSTEM_INFO(character);
    ADVANCE_TO(HVML_DOCTYPE_SYSTEM_SINGLE_QUOTED_STATE);
END_STATE()

BEGIN_STATE(HVML_AFTER_DOCTYPE_SYSTEM_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_AFTER_DOCTYPE_SYSTEM_STATE);
    }
    if (character == '>') {
        RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER_AFTER_DOCTYPE_SYSTEM);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_BOGUS_DOCTYPE_STATE)
    if (character == '>') {
        RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
    }
    if (is_eof(character)) {
        RETURN_AND_RECONSUME_IN(HVML_DATA_STATE);
    }
    ADVANCE_TO(HVML_BOGUS_DOCTYPE_STATE);
END_STATE()

BEGIN_STATE(HVML_CDATA_SECTION_STATE)
    if (character == ']') {
        ADVANCE_TO(HVML_CDATA_SECTION_BRACKET_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_CDATA);
        RECONSUME_IN(HVML_DATA_STATE);
    }
    APPEND_TO_TOKEN_TEXT(character);
    ADVANCE_TO(HVML_CDATA_SECTION_STATE);
END_STATE()

BEGIN_STATE(HVML_CDATA_SECTION_BRACKET_STATE)
    if (character == ']') {
        ADVANCE_TO(HVML_CDATA_SECTION_END_STATE);
    }
    APPEND_TO_TOKEN_TEXT(']');
    RECONSUME_IN(HVML_CDATA_SECTION_STATE);
END_STATE()

BEGIN_STATE(HVML_CDATA_SECTION_END_STATE)
    if (character == ']') {
        APPEND_TO_TOKEN_TEXT(']');
        ADVANCE_TO(HVML_CDATA_SECTION_END_STATE);
    }
    if (character == '>') {
        ADVANCE_TO(HVML_DATA_STATE);
    }
    APPEND_TO_TOKEN_TEXT(']');
    APPEND_TO_TOKEN_TEXT(']');
    RECONSUME_IN(HVML_CDATA_SECTION_STATE);
END_STATE()

BEGIN_STATE(HVML_CHARACTER_REFERENCE_STATE)
    RESET_TEMP_BUFFER();
    APPEND_TO_TEMP_BUFFER('&');
    if (is_ascii_alpha_numeric(character)) {
        RECONSUME_IN(HVML_NAMED_CHARACTER_REFERENCE_STATE);
    }
    if (character == '#') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_NUMERIC_CHARACTER_REFERENCE_STATE);
    }
    // FIXME: character reference in attribute value
    APPEND_TEMP_BUFFER_TO_TOKEN_TEXT();
    RESET_TEMP_BUFFER();
    RECONSUME_IN(parser->return_state);
END_STATE()

BEGIN_STATE(HVML_NAMED_CHARACTER_REFERENCE_STATE)
    if (parser->sbst == NULL) {
        parser->sbst = pchvml_sbst_new_char_ref();
    }
    bool ret = pchvml_sbst_advance(parser->sbst, character);
    if (!ret) {
        struct pcutils_arrlist* ucs = pchvml_sbst_get_buffered_ucs(
                parser->sbst);
        size_t length = pcutils_arrlist_length(ucs);
        for (size_t i = 0; i < length; i++) {
            uint32_t uc = (uint32_t)(uintptr_t) pcutils_arrlist_get_idx(
                    ucs, i);
            APPEND_TO_TEMP_BUFFER(uc);
        }
        pchvml_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        APPEND_TEMP_BUFFER_TO_TOKEN_TEXT();
        RESET_TEMP_BUFFER();
        ADVANCE_TO(HVML_AMBIGUOUS_AMPERSAND_STATE);
    }

    const char* value = pchvml_sbst_get_match(parser->sbst);
    if (value == NULL) {
        ADVANCE_TO(HVML_NAMED_CHARACTER_REFERENCE_STATE);
    }
    if (character != ';') {
        ADVANCE_TO(HVML_NAMED_CHARACTER_REFERENCE_STATE);
    }
    RESET_TEMP_BUFFER();
    APPEND_BYTES_TO_TOKEN_TEXT(value, strlen(value));

    pchvml_sbst_destroy(parser->sbst);
    parser->sbst = NULL;
    ADVANCE_TO(parser->return_state);
END_STATE()

BEGIN_STATE(HVML_AMBIGUOUS_AMPERSAND_STATE)
    if (is_ascii_alpha_numeric(character)) {
        if (pchvml_parser_is_in_attribute(parser)) {
            APPEND_TO_TOKEN_ATTR_VALUE(character);
            ADVANCE_TO(HVML_AMBIGUOUS_AMPERSAND_STATE);
        }
        else {
            RECONSUME_IN(parser->return_state);
        }
    }
    if (character == ';') {
        SET_ERR(PCHVML_ERROR_UNKNOWN_NAMED_CHARACTER_REFERENCE);
        RETURN_AND_STOP_PARSE();
    }
    RECONSUME_IN(parser->return_state);
END_STATE()

BEGIN_STATE(HVML_NUMERIC_CHARACTER_REFERENCE_STATE)
    parser->char_ref_code = 0;
    if (character == 'x' || character == 'X') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_HEXADECIMAL_CHARACTER_REFERENCE_START_STATE);
    }
    RECONSUME_IN(HVML_DECIMAL_CHARACTER_REFERENCE_START_STATE);
END_STATE()

BEGIN_STATE(HVML_HEXADECIMAL_CHARACTER_REFERENCE_START_STATE)
    if (is_ascii_hex_digit(character)) {
        RECONSUME_IN(HVML_HEXADECIMAL_CHARACTER_REFERENCE_STATE);
    }
    SET_ERR(
        PCHVML_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_DECIMAL_CHARACTER_REFERENCE_START_STATE)
    if (is_ascii_digit(character)) {
        RECONSUME_IN(HVML_DECIMAL_CHARACTER_REFERENCE_STATE);
    }
    SET_ERR(
        PCHVML_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_HEXADECIMAL_CHARACTER_REFERENCE_STATE)
    if (is_ascii_digit(character)) {
        parser->char_ref_code *= 16;
        parser->char_ref_code += character - 0x30;
    }
    if (is_ascii_upper_hex_digit(character)) {
        parser->char_ref_code *= 16;
        parser->char_ref_code += character - 0x37;
    }
    if (is_ascii_lower_hex_digit(character)) {
        parser->char_ref_code *= 16;
        parser->char_ref_code += character - 0x57;
    }
    if (character == ';') {
        ADVANCE_TO(HVML_NUMERIC_CHARACTER_REFERENCE_END_STATE);
    }
    SET_ERR(PCHVML_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_DECIMAL_CHARACTER_REFERENCE_STATE)
    if (is_ascii_digit(character)) {
        parser->char_ref_code *= 10;
        parser->char_ref_code += character - 0x30;
        ADVANCE_TO(HVML_DECIMAL_CHARACTER_REFERENCE_STATE);
    }
    if (character == ';') {
        ADVANCE_TO(HVML_NUMERIC_CHARACTER_REFERENCE_END_STATE);
    }
    SET_ERR(PCHVML_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_NUMERIC_CHARACTER_REFERENCE_END_STATE)
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
    RESET_TEMP_BUFFER();
    uc = parser->char_ref_code;
    APPEND_TO_TOKEN_TEXT(uc);
    RECONSUME_IN(parser->return_state);
END_STATE()

BEGIN_STATE(HVML_SPECIAL_ATTRIBUTE_OPERATOR_IN_ATTRIBUTE_NAME_STATE)
    if (character == '=') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            pchvml_token_set_assignment_to_attr(
                    parser->token,
                    PCHVML_ATTRIBUTE_OPERATOR);
        }
        else {
            uint32_t op = pchvml_buffer_get_last_char(
                    parser->temp_buffer);
            switch (op) {
                case '+':
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_ADDITION_OPERATOR);
                    break;
                case '-':
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_SUBTRACTION_OPERATOR);
                    break;
                case '*':
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_ASTERISK_OPERATOR);
                    break;
                case '/':
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_REGEX_OPERATOR);
                    break;
                case '%':
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_REMAINDER_OPERATOR);
                    break;
                case '~':
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_REPLACE_OPERATOR);
                    break;
                case '^':
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_HEAD_OPERATOR);
                    break;
                case '$':
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_TAIL_OPERATOR);
                    break;
                default:
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_OPERATOR);
                    break;
            }
        }
        ADVANCE_TO(HVML_BEFORE_ATTRIBUTE_VALUE_STATE);
    }
    if (character == '>'
        &&  pchvml_buffer_equal_to(parser->temp_buffer, "/", 1)) {
        END_TOKEN_ATTR();
        RECONSUME_IN(HVML_SELF_CLOSING_START_TAG_STATE);
    }
    APPEND_TEMP_BUFFER_TO_TOKEN_ATTR_NAME();
    RECONSUME_IN(HVML_ATTRIBUTE_NAME_STATE);
END_STATE()

BEGIN_STATE(HVML_SPECIAL_ATTRIBUTE_OPERATOR_AFTER_ATTRIBUTE_NAME_STATE)
    if (character == '=') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            pchvml_token_set_assignment_to_attr(
                    parser->token,
                    PCHVML_ATTRIBUTE_OPERATOR);
        }
        else {
            uint32_t op = pchvml_buffer_get_last_char(
                    parser->temp_buffer);
            switch (op) {
                case '+':
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_ADDITION_OPERATOR);
                    break;
                case '-':
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_SUBTRACTION_OPERATOR);
                    break;
                case '*':
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_ASTERISK_OPERATOR);
                    break;
                case '/':
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_REGEX_OPERATOR);
                    break;
                case '%':
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_REMAINDER_OPERATOR);
                    break;
                case '~':
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_REPLACE_OPERATOR);
                    break;
                case '^':
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_HEAD_OPERATOR);
                    break;
                case '$':
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_TAIL_OPERATOR);
                    break;
                default:
                    pchvml_token_set_assignment_to_attr(
                            parser->token,
                            PCHVML_ATTRIBUTE_OPERATOR);
                    break;
            }
        }
        ADVANCE_TO(HVML_BEFORE_ATTRIBUTE_VALUE_STATE);
    }
    if (pchvml_buffer_equal_to(parser->temp_buffer, "$", 1)) {
        pchvml_rwswrap_reconsume_last_char(parser->rwswrap);
        pchvml_rwswrap_reconsume_last_char(parser->rwswrap);
        ADVANCE_TO(HVML_BEFORE_ATTRIBUTE_VALUE_STATE);
    }
    if (character == '>'
        &&  pchvml_buffer_equal_to(parser->temp_buffer, "/", 1)) {
        END_TOKEN_ATTR();
        RECONSUME_IN(HVML_SELF_CLOSING_START_TAG_STATE);
    }
    BEGIN_TOKEN_ATTR();
    APPEND_TEMP_BUFFER_TO_TOKEN_ATTR_NAME();
    RECONSUME_IN(HVML_ATTRIBUTE_NAME_STATE);
END_STATE()

BEGIN_STATE(HVML_TEXT_CONTENT_STATE)
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_CDATA);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '&') {
        SET_RETURN_STATE(HVML_TEXT_CONTENT_STATE);
        ADVANCE_TO(HVML_CHARACTER_REFERENCE_STATE);
    }
    if (character == '$') {
        if (!IS_TEMP_BUFFER_EMPTY()) {
            struct pcvcm_node* node = TEMP_BUFFER_TO_VCM_NODE();
            if (!node) {
                RETURN_AND_STOP_PARSE();
            }
            RESET_TEMP_BUFFER();
            parser->token = pchvml_token_new_vcm(parser->vcm_node);
            RETURN_CURRENT_TOKEN();
        }
        RESET_VCM_NODE();
        SET_TRANSIT_STATE(HVML_TEXT_CONTENT_STATE);
        RECONSUME_IN(HVML_EJSON_DATA_STATE);
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(HVML_TEXT_CONTENT_STATE);
END_STATE()

BEGIN_STATE(HVML_JSONTEXT_CONTENT_STATE)
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_CDATA);
        RETURN_AND_STOP_PARSE();
    }
    SET_TRANSIT_STATE(HVML_TEXT_CONTENT_STATE);
    RECONSUME_IN(HVML_EJSON_DATA_STATE);
END_STATE()

BEGIN_STATE(HVML_JSONEE_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE)
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_CDATA);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '"') {
        END_TOKEN_ATTR();
        ADVANCE_TO(HVML_AFTER_ATTRIBUTE_VALUE_STATE);
    }
    if (character == '&') {
        SET_RETURN_STATE(HVML_JSONEE_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE);
        ADVANCE_TO(HVML_CHARACTER_REFERENCE_STATE);
    }
    if (character == '$' || character == '{' || character == '[') {
        bool handle = pchvml_parser_is_handle_as_jsonee(parser->token,
                character);
        bool buffer_is_white = pchvml_buffer_is_whitespace(
                parser->string_buffer);
        if (handle && buffer_is_white) {
            ejson_stack_push('"');
            RESET_TEMP_BUFFER();
            RECONSUME_IN(HVML_EJSON_DATA_STATE);
        }

        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('"');
        if (!pchvml_buffer_is_empty(parser->string_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
                    pchvml_buffer_get_buffer(parser->string_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(HVML_EJSON_DATA_STATE);
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(HVML_JSONEE_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE);
END_STATE()

BEGIN_STATE(HVML_JSONEE_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE)
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '\'') {
        END_TOKEN_ATTR();
        ADVANCE_TO(HVML_AFTER_ATTRIBUTE_VALUE_STATE);
    }
    if (character == '&') {
        SET_RETURN_STATE(HVML_JSONEE_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE);
        ADVANCE_TO(HVML_CHARACTER_REFERENCE_STATE);
    }
    APPEND_TO_TOKEN_ATTR_VALUE(character);
    ADVANCE_TO(HVML_JSONEE_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE);
END_STATE()

BEGIN_STATE(HVML_JSONEE_ATTRIBUTE_VALUE_UNQUOTED_STATE)
    if (is_whitespace(character)) {
        if (!pchvml_buffer_is_empty(parser->string_buffer)) {
            APPEND_BUFFER_TO_TOKEN_ATTR_VALUE(parser->string_buffer);
            RESET_STRING_BUFFER();
        }
        END_TOKEN_ATTR();
        ADVANCE_TO(HVML_BEFORE_ATTRIBUTE_NAME_STATE);
    }
    if (character == '&') {
        SET_RETURN_STATE(HVML_JSONEE_ATTRIBUTE_VALUE_UNQUOTED_STATE);
        ADVANCE_TO(HVML_CHARACTER_REFERENCE_STATE);
    }
    if (character == '>') {
        if (!pchvml_buffer_is_empty(parser->string_buffer)) {
            APPEND_BUFFER_TO_TOKEN_ATTR_VALUE(parser->string_buffer);
            RESET_STRING_BUFFER();
        }
        END_TOKEN_ATTR();
        RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
    }
    if (is_eof(character)) {
        if (!pchvml_buffer_is_empty(parser->string_buffer)) {
            APPEND_BUFFER_TO_TOKEN_ATTR_VALUE(parser->string_buffer);
            RESET_STRING_BUFFER();
        }
        END_TOKEN_ATTR();
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RECONSUME_IN(HVML_DATA_STATE);
    }
    if (character == '$' || character == '{' || character == '[') {
        bool handle = pchvml_parser_is_handle_as_jsonee(parser->token,
                character);
        bool buffer_is_white = pchvml_buffer_is_whitespace(
                parser->string_buffer);
        if (handle && buffer_is_white) {
            ejson_stack_push('U');
            RESET_STRING_BUFFER();
            RECONSUME_IN(HVML_EJSON_DATA_STATE);
        }

        ejson_stack_push('U');
        if (!pchvml_buffer_is_empty(parser->string_buffer)) {
            if (parser->vcm_node) {
                vcm_stack_push(parser->vcm_node);
            }
            struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                    NULL);
            UPDATE_VCM_NODE(snode);
            struct pcvcm_node* node = pcvcm_node_new_string(
                    pchvml_buffer_get_buffer(parser->string_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_STRING_BUFFER();
        }
        RECONSUME_IN(HVML_EJSON_DATA_STATE);
    }
    if (character == '"' || character == '\'' || character == '<'
            || character == '=' || character == '`') {
        SET_ERR(
            PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_UNQUOTED_ATTRIBUTE_VALUE);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_STRING_BUFFER(character);
    ADVANCE_TO(HVML_JSONEE_ATTRIBUTE_VALUE_UNQUOTED_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_DATA_STATE)
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (pchvml_parser_is_in_template(parser)) {
        struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('T');
        RESET_TEMP_BUFFER();
        RESET_STRING_BUFFER();
        RECONSUME_IN(HVML_EJSON_TEMPLATE_DATA_STATE);
    }
    if (is_whitespace (character) || character == 0xFEFF) {
        ADVANCE_TO(HVML_EJSON_DATA_STATE);
    }
    RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_FINISHED_STATE)
    while (!vcm_stack_is_empty()) {
        ejson_stack_pop();
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
    }
    if (!ejson_stack_is_empty()) {
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (parser->transit_state == HVML_TEXT_CONTENT_STATE ||
        parser->transit_state == HVML_JSONTEXT_CONTENT_STATE) {
        parser->token = pchvml_token_new_vcm(parser->vcm_node);
        parser->vcm_node = NULL;
        RESET_VCM_NODE();
        RETURN_AND_SWITCH_TO(HVML_DATA_STATE);
    }
    pchvml_token_append_vcm_to_attr(parser->token, parser->vcm_node);
    END_TOKEN_ATTR();
    RESET_VCM_NODE();
    RECONSUME_IN(HVML_AFTER_ATTRIBUTE_VALUE_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_CONTROL_STATE)
    uint32_t uc = ejson_stack_top();
    if (is_whitespace(character)) {
        if (ejson_stack_is_empty()) {
            RECONSUME_IN(HVML_EJSON_FINISHED_STATE);
        }
        if (uc == '"' || uc == '\'' || uc == 'U') {
            RECONSUME_IN(HVML_EJSON_AFTER_JSONEE_STRING_STATE);
        }
        if (uc == 'T') {
            if (parser->vcm_node->type !=
                    PCVCM_NODE_TYPE_FUNC_CONCAT_STRING) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            RECONSUME_IN(HVML_EJSON_TEMPLATE_DATA_STATE);
        }
        ADVANCE_TO(HVML_EJSON_CONTROL_STATE);
    }
    if (character == '{') {
        RECONSUME_IN(HVML_EJSON_LEFT_BRACE_STATE);
    }
    if (character == '}') {
        if ((parser->vcm_node->type == PCVCM_NODE_TYPE_FUNC_CONCAT_STRING)
                && (uc == '"' || uc == '\'' || uc == 'U')) {
            RECONSUME_IN(HVML_EJSON_AFTER_JSONEE_STRING_STATE);
        }
        RECONSUME_IN(HVML_EJSON_RIGHT_BRACE_STATE);
    }
    if (character == '[') {
        RECONSUME_IN(HVML_EJSON_LEFT_BRACKET_STATE);
    }
    if (character == ']') {
        if (parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_CONCAT_STRING
                && (uc == '"' || uc == '\'' || uc == 'U')) {
            RECONSUME_IN(HVML_EJSON_AFTER_JSONEE_STRING_STATE);
        }
        RECONSUME_IN(HVML_EJSON_RIGHT_BRACKET_STATE);
    }
    if (character == '<' || character == '>') {
        if (pchvml_parser_is_in_template(parser)) {
            if (parser->vcm_node->type !=
                    PCVCM_NODE_TYPE_FUNC_CONCAT_STRING) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            RECONSUME_IN(HVML_EJSON_TEMPLATE_DATA_STATE);
        }
        RECONSUME_IN(HVML_EJSON_FINISHED_STATE);
    }
    if (character == '(') {
        ADVANCE_TO(HVML_EJSON_LEFT_PARENTHESIS_STATE);
    }
    if (character == ')') {
        if (uc == '"' || uc == '\'' || uc == 'U') {
            RECONSUME_IN(HVML_EJSON_AFTER_JSONEE_STRING_STATE);
        }
        ADVANCE_TO(HVML_EJSON_RIGHT_PARENTHESIS_STATE);
    }
    if (character == '$') {
        RECONSUME_IN(HVML_EJSON_DOLLAR_STATE);
    }
    if (character == '"') {
        if (uc == '"') {
            RECONSUME_IN(HVML_EJSON_AFTER_JSONEE_STRING_STATE);
        }
        else if (uc == 'T') {
            if (parser->vcm_node->type !=
                    PCVCM_NODE_TYPE_FUNC_CONCAT_STRING) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            RECONSUME_IN(HVML_EJSON_TEMPLATE_DATA_STATE);
        }
        else {
            RESET_TEMP_BUFFER();
            RESET_QUOTED_BUFFER();
            RECONSUME_IN(HVML_EJSON_VALUE_DOUBLE_QUOTED_STATE);
        }
    }
    if (character == '\'') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(HVML_EJSON_VALUE_SINGLE_QUOTED_STATE);
    }
    if (character == 'b') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(HVML_EJSON_BYTE_SEQUENCE_STATE);
    }
    if (character == 't' || character == 'f' || character == 'n') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(HVML_EJSON_KEYWORD_STATE);
    }
    if (character == 'I') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(HVML_EJSON_VALUE_NUMBER_INFINITY_STATE);
    }
    if (character == 'N') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(HVML_EJSON_VALUE_NAN_STATE);
    }
    if (is_ascii_digit(character) || character == '-') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(HVML_EJSON_VALUE_NUMBER_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == ',') {
        if (uc == '{') {
            ejson_stack_pop();
            ADVANCE_TO(HVML_EJSON_BEFORE_NAME_STATE);
        }
        if (uc == '[' || uc == '(' || uc == '<') {
            ADVANCE_TO(HVML_EJSON_CONTROL_STATE);
        }
        if (uc == ':') {
            ejson_stack_pop();
            if (!pchvml_buffer_is_empty(parser->temp_buffer)) {
                struct pcvcm_node* node = pcvcm_node_new_string(
                pchvml_buffer_get_buffer(parser->temp_buffer));
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
            }
            if (parser->vcm_node &&
                    parser->vcm_node->type != PCVCM_NODE_TYPE_OBJECT) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            ADVANCE_TO(HVML_EJSON_BEFORE_NAME_STATE);
        }
        if (uc == '"') {
            RECONSUME_IN(HVML_EJSON_JSONEE_STRING_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '.') {
        RECONSUME_IN(HVML_EJSON_JSONEE_FULL_STOP_SIGN_STATE);
    }
    if (uc == '[') {
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (parser->vcm_node->type ==
            PCVCM_NODE_TYPE_FUNC_GET_VARIABLE ||
            parser->vcm_node->type ==
            PCVCM_NODE_TYPE_FUNC_GET_ELEMENT) {
        size_t n = pctree_node_children_number(
                (struct pctree_node*)parser->vcm_node);
        if (n < 2) {
            RECONSUME_IN(HVML_EJSON_JSONEE_VARIABLE_STATE);
        }
        else {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
    }
    RECONSUME_IN(HVML_EJSON_JSONEE_STRING_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_DOLLAR_STATE)
    if (is_whitespace(character)) {
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '$') {
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        ejson_stack_push('$');
        struct pcvcm_node* snode = pcvcm_node_new_get_variable(NULL);
        UPDATE_VCM_NODE(snode);
        ADVANCE_TO(HVML_EJSON_DOLLAR_STATE);
    }
    if (character == '{') {
        ejson_stack_push('P');
        RESET_TEMP_BUFFER();
        ADVANCE_TO(HVML_EJSON_JSONEE_VARIABLE_STATE);
    }
    RESET_TEMP_BUFFER();
    RECONSUME_IN(HVML_EJSON_JSONEE_VARIABLE_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_JSONEE_FULL_STOP_SIGN_STATE)
    if (character == '.' &&
        (parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_GET_VARIABLE ||
                parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_GET_ELEMENT ||
                parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_CALL_GETTER ||
                parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_CALL_SETTER
                )) {
        ejson_stack_push('.');
        struct pcvcm_node* node = pcvcm_node_new_get_element(NULL,
                NULL);
        APPEND_CHILD(node, parser->vcm_node);
        UPDATE_VCM_NODE(node);
        ADVANCE_TO(HVML_EJSON_JSONEE_KEYWORD_STATE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_LEFT_BRACE_STATE)
    if (character == '{') {
        ejson_stack_push('P');
        ADVANCE_TO(HVML_EJSON_LEFT_BRACE_STATE);
    }
    if (character == '$') {
        RECONSUME_IN(HVML_EJSON_DOLLAR_STATE);
    }
    uint32_t uc = ejson_stack_top();
    if (uc == 'P') {
        ejson_stack_pop();
        ejson_stack_push('{');
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        struct pcvcm_node* node = pcvcm_node_new_object(0, NULL);
        UPDATE_VCM_NODE(node);
        RECONSUME_IN(HVML_EJSON_BEFORE_NAME_STATE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_RIGHT_BRACE_STATE)
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    uint32_t uc = ejson_stack_top();
    if (character == '}') {
        if (uc == ':') {
            ejson_stack_pop();
            uc = ejson_stack_top();
        }
        if (uc == '{') {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            if (ejson_stack_is_empty()) {
                ADVANCE_TO(HVML_EJSON_FINISHED_STATE);
            }
            ADVANCE_TO(HVML_EJSON_AFTER_VALUE_STATE);
        }
        else if (uc == 'P') {
            ejson_stack_pop();
            if (parser->vcm_node->extra & EXTRA_PROTECT_FLAG) {
                parser->vcm_node->extra &= EXTRA_SUGAR_FLAG;
            }
            else {
                parser->vcm_node->extra &= EXTRA_PROTECT_FLAG;
            }
            // FIXME : <update from="assets/{$SYSTEM.locale}.json" />
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            if (ejson_stack_is_empty()) {
                ADVANCE_TO(HVML_EJSON_FINISHED_STATE);
            }
            ADVANCE_TO(HVML_EJSON_RIGHT_BRACE_STATE);
        }
        else if (uc == '(' || uc == '<' || uc == '"') {
            ADVANCE_TO(HVML_EJSON_CONTROL_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_RIGHT_BRACE);
        RETURN_AND_STOP_PARSE();
    }
    if (uc == '"') {
        RECONSUME_IN(HVML_EJSON_JSONEE_STRING_STATE);
    }
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_EJSON_RIGHT_BRACE_STATE);
    }
    if (character == ':') {
        if (uc == '{') {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            vcm_stack_push(parser->vcm_node);
            RESET_VCM_NODE();
            ADVANCE_TO(HVML_EJSON_CONTROL_STATE);
        }
        if (uc == 'P') {
            ejson_stack_pop();
            ejson_stack_push('{');
            struct pcvcm_node* node = pcvcm_node_new_object(0, NULL);
            APPEND_CHILD(node, parser->vcm_node);
            vcm_stack_push(node);
            RESET_VCM_NODE();
            ADVANCE_TO(HVML_EJSON_CONTROL_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '.' && uc == '$') {
        ejson_stack_pop();
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
    }
    RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_LEFT_BRACKET_STATE)
    if (character == '[') {
        if (parser->vcm_node && ejson_stack_is_empty()) {
            ejson_stack_push('[');
            struct pcvcm_node* node = pcvcm_node_new_get_element(NULL,
                    NULL);
            APPEND_CHILD(node, parser->vcm_node);
            UPDATE_VCM_NODE(node);
            ADVANCE_TO(HVML_EJSON_CONTROL_STATE);
        }
        if (parser->vcm_node && (parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_GET_VARIABLE ||
                parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_GET_ELEMENT)) {
            ejson_stack_push('.');
            struct pcvcm_node* node = pcvcm_node_new_get_element(NULL,
                    NULL);
            APPEND_CHILD(node, parser->vcm_node);
            UPDATE_VCM_NODE(node);
            ADVANCE_TO(HVML_EJSON_CONTROL_STATE);
        }
        uint32_t uc = ejson_stack_top();
        if (uc == '(' || uc == '<' || uc == '[' || uc == ':' || uc == 0
                || uc == '"') {
            ejson_stack_push('[');
            if (parser->vcm_node) {
                vcm_stack_push(parser->vcm_node);
            }
            struct pcvcm_node* node = pcvcm_node_new_array(0, NULL);
            UPDATE_VCM_NODE(node);
            ADVANCE_TO(HVML_EJSON_CONTROL_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_RIGHT_BRACKET_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_EJSON_RIGHT_BRACKET_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    uint32_t uc = ejson_stack_top();
    if (character == ']') {
        if (uc == '.') {
            ejson_stack_pop();
            uc = ejson_stack_top();
            if (uc == '"' || uc == 'U') {
                ADVANCE_TO(HVML_EJSON_AFTER_VALUE_STATE);
            }
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            ADVANCE_TO(HVML_EJSON_AFTER_VALUE_STATE);
        }
        if (uc == '[') {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            struct pcvcm_node* parent = (struct pcvcm_node*)
                pctree_node_parent((struct pctree_node*)parser->vcm_node);
            if (parent) {
                UPDATE_VCM_NODE(parent);
            }
            if (ejson_stack_is_empty()) {
                ADVANCE_TO(HVML_EJSON_FINISHED_STATE);
            }
            ADVANCE_TO(HVML_EJSON_AFTER_VALUE_STATE);
        }
        if (uc == '"') {
            RECONSUME_IN(HVML_EJSON_JSONEE_STRING_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_RIGHT_BRACKET);
        RETURN_AND_STOP_PARSE();
    }
    if (ejson_stack_is_empty()
            || uc == '(' || uc == '<') {
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    ADVANCE_TO(HVML_EJSON_CONTROL_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_LEFT_PARENTHESIS_STATE)
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '!') {
        if (parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_GET_VARIABLE ||
                parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_GET_ELEMENT) {
            struct pcvcm_node* node = pcvcm_node_new_call_setter(NULL,
                    0, NULL);
            APPEND_CHILD(node, parser->vcm_node);
            UPDATE_VCM_NODE(node);
            ejson_stack_push('<');
            ADVANCE_TO(HVML_EJSON_CONTROL_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (parser->vcm_node->type ==
            PCVCM_NODE_TYPE_FUNC_GET_VARIABLE ||
            parser->vcm_node->type ==
            PCVCM_NODE_TYPE_FUNC_GET_ELEMENT) {
        struct pcvcm_node* node = pcvcm_node_new_call_getter(NULL,
                0, NULL);
        APPEND_CHILD(node, parser->vcm_node);
        UPDATE_VCM_NODE(node);
        ejson_stack_push('(');
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    if (ejson_stack_is_empty()) {
        RECONSUME_IN(HVML_EJSON_FINISHED_STATE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_RIGHT_PARENTHESIS_STATE)
    uint32_t uc = ejson_stack_top();
    if (character == '.') {
        if (uc == '(' || uc == '<') {
            ejson_stack_pop();
            RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
        }
        if (ejson_stack_is_empty()) {
            SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    else {
        if (uc == '(' || uc == '<') {
            ejson_stack_pop();
            if (!vcm_stack_is_empty()) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
        }
        if (ejson_stack_is_empty()) {
            SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
END_STATE()

BEGIN_STATE(HVML_EJSON_AFTER_VALUE_STATE)
    uint32_t uc = ejson_stack_top();
    if (is_whitespace(character)) {
        if (uc  == 'U' || uc == '"' || uc == 'T') {
            RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
        }
        ADVANCE_TO(HVML_EJSON_AFTER_VALUE_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '"' || character == '\'') {
        if (!pchvml_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
                    pchvml_buffer_get_buffer(parser->temp_buffer));
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        if (uc == '"' || uc == '\'') {
            ejson_stack_pop();
            if (ejson_stack_is_empty()) {
                ADVANCE_TO(HVML_EJSON_FINISHED_STATE);
            }
        }
        ADVANCE_TO(HVML_EJSON_AFTER_VALUE_STATE);
    }
    if (character == '}') {
        RECONSUME_IN(HVML_EJSON_RIGHT_BRACE_STATE);
    }
    if (character == ']') {
        RECONSUME_IN(HVML_EJSON_RIGHT_BRACKET_STATE);
    }
    if (character == ')') {
        ADVANCE_TO(HVML_EJSON_RIGHT_PARENTHESIS_STATE);
    }
    if (character == ',') {
        if (uc == '{') {
            ejson_stack_pop();
            ADVANCE_TO(HVML_EJSON_BEFORE_NAME_STATE);
        }
        if (uc == '[') {
            if (!pchvml_buffer_is_empty(parser->temp_buffer)) {
                struct pcvcm_node* node = pcvcm_node_new_string(
                pchvml_buffer_get_buffer(parser->temp_buffer));
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
            }
            if (parser->vcm_node &&
                    parser->vcm_node->type != PCVCM_NODE_TYPE_ARRAY) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            ADVANCE_TO(HVML_EJSON_CONTROL_STATE);
        }
        if (uc == '(' || uc == '<') {
            ADVANCE_TO(HVML_EJSON_CONTROL_STATE);
        }
        if (uc == ':') {
            ejson_stack_pop();
            if (!pchvml_buffer_is_empty(parser->temp_buffer)) {
                struct pcvcm_node* node = pcvcm_node_new_string(
                pchvml_buffer_get_buffer(parser->temp_buffer));
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
            }
            if (parser->vcm_node &&
                    parser->vcm_node->type != PCVCM_NODE_TYPE_OBJECT) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            ADVANCE_TO(HVML_EJSON_BEFORE_NAME_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '<' || character == '.') {
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    if (uc == '"' || uc  == 'U') {
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_BEFORE_NAME_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_EJSON_BEFORE_NAME_STATE);
    }
    uint32_t uc = ejson_stack_top();
    if (character == '"') {
        RESET_TEMP_BUFFER();
        RESET_STRING_BUFFER();
        if (uc == '{') {
            ejson_stack_push(':');
        }
        RECONSUME_IN(HVML_EJSON_NAME_DOUBLE_QUOTED_STATE);
    }
    if (character == '\'') {
        RESET_TEMP_BUFFER();
        if (uc == '{') {
            ejson_stack_push(':');
        }
        RECONSUME_IN(HVML_EJSON_NAME_SINGLE_QUOTED_STATE);
    }
    if (character == '}') {
        RECONSUME_IN(HVML_EJSON_RIGHT_BRACE_STATE);
    }
    if (character == '$') {
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    if (is_ascii_alpha(character)) {
        RESET_TEMP_BUFFER();
        if (uc == '{') {
            ejson_stack_push(':');
        }
        RECONSUME_IN(HVML_EJSON_NAME_UNQUOTED_STATE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_AFTER_NAME_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(HVML_EJSON_AFTER_NAME_STATE);
    }
    if (character == ':') {
        if (!pchvml_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
                pchvml_buffer_get_buffer(parser->temp_buffer));
            APPEND_AS_VCM_CHILD(node);
        }
        ADVANCE_TO(HVML_EJSON_CONTROL_STATE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_NAME_UNQUOTED_STATE)
    if (is_whitespace(character) || character == ':') {
        RECONSUME_IN(HVML_EJSON_AFTER_NAME_STATE);
    }
    if (is_ascii_alpha(character) || is_ascii_digit(character)
            || character == '-' || character == '_') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_NAME_UNQUOTED_STATE);
    }
    if (character == '$') {
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('U');
        if (!pchvml_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
                    pchvml_buffer_get_buffer(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_NAME_SINGLE_QUOTED_STATE)
    if (character == '\'') {
        size_t nr_buf_chars = pchvml_buffer_get_size_in_chars(
                parser->temp_buffer);
        if (nr_buf_chars >= 1) {
            ADVANCE_TO(HVML_EJSON_AFTER_NAME_STATE);
        }
        else {
            ADVANCE_TO(HVML_EJSON_NAME_SINGLE_QUOTED_STATE);
        }
    }
    if (character == '\\') {
        SET_RETURN_STATE(HVML_EJSON_NAME_SINGLE_QUOTED_STATE);
        ADVANCE_TO(HVML_EJSON_STRING_ESCAPE_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(HVML_EJSON_NAME_SINGLE_QUOTED_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_NAME_DOUBLE_QUOTED_STATE)
    if (character == '"') {
        size_t nr_buf_chars = pchvml_buffer_get_size_in_chars(
                parser->temp_buffer);
        if (nr_buf_chars > 1) {
            pchvml_buffer_delete_head_chars (parser->temp_buffer, 1);
            ADVANCE_TO(HVML_EJSON_AFTER_NAME_STATE);
        }
        else if (nr_buf_chars == 1) {
            RESET_TEMP_BUFFER();
            RESTORE_VCM_NODE();
            struct pcvcm_node* node = pcvcm_node_new_string ("");
            APPEND_AS_VCM_CHILD(node);
            ADVANCE_TO(HVML_EJSON_AFTER_NAME_STATE);
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_NAME_DOUBLE_QUOTED_STATE);
    }
    if (character == '\\') {
        SET_RETURN_STATE(curr_state);
        ADVANCE_TO(HVML_EJSON_STRING_ESCAPE_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '$') {
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('"');
        if (!pchvml_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
                    pchvml_buffer_get_buffer(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(HVML_EJSON_NAME_DOUBLE_QUOTED_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_VALUE_SINGLE_QUOTED_STATE)
    if (character == '\'') {
        size_t nr_buf_chars = pchvml_buffer_get_size_in_chars(
                parser->temp_buffer);
        if (nr_buf_chars >= 1) {
            RECONSUME_IN(HVML_EJSON_AFTER_VALUE_STATE);
        }
        else {
            ADVANCE_TO(HVML_EJSON_VALUE_SINGLE_QUOTED_STATE);
        }
    }
    if (character == '\\') {
        SET_RETURN_STATE(curr_state);
        ADVANCE_TO(HVML_EJSON_STRING_ESCAPE_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(HVML_EJSON_VALUE_SINGLE_QUOTED_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_VALUE_DOUBLE_QUOTED_STATE)
    if (character == '"') {
        if (pchvml_buffer_is_empty(parser->quoted_buffer)) {
            APPEND_TO_QUOTED_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_VALUE_DOUBLE_QUOTED_STATE);
        }
        else if (pchvml_buffer_equal_to(parser->quoted_buffer, "\"",
                    1)) {
            RECONSUME_IN(HVML_EJSON_VALUE_TWO_DOUBLE_QUOTED_STATE);
        }
        RECONSUME_IN(HVML_EJSON_AFTER_VALUE_DOUBLE_QUOTED_STATE);
    }
    if (character == '\\') {
        SET_RETURN_STATE(curr_state);
        ADVANCE_TO(HVML_EJSON_STRING_ESCAPE_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '$') {
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('"');
        if (!pchvml_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
                    pchvml_buffer_get_buffer(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(HVML_EJSON_VALUE_DOUBLE_QUOTED_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_AFTER_VALUE_DOUBLE_QUOTED_STATE)
    if (character == '\"') {
        RESET_QUOTED_BUFFER();
        RECONSUME_IN(HVML_EJSON_AFTER_VALUE_STATE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_VALUE_TWO_DOUBLE_QUOTED_STATE)
    if (character == '"') {
        if (pchvml_buffer_equal_to(parser->quoted_buffer, "\"", 1)) {
            APPEND_TO_QUOTED_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_VALUE_TWO_DOUBLE_QUOTED_STATE);
        }
        if (pchvml_buffer_equal_to(parser->quoted_buffer, "\"\"", 2)) {
            RECONSUME_IN(HVML_EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE);
        }
    }
    RESTORE_VCM_NODE();
    struct pcvcm_node* node = pcvcm_node_new_string(
            pchvml_buffer_get_buffer(parser->temp_buffer)
            );
    APPEND_AS_VCM_CHILD(node);
    RESET_TEMP_BUFFER();
    RESET_QUOTED_BUFFER();
    RECONSUME_IN(HVML_EJSON_AFTER_VALUE_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE)
    if (character == '\"') {
        APPEND_TO_QUOTED_BUFFER(character);
        size_t buf_len = pchvml_buffer_get_size_in_chars(
                parser->quoted_buffer);
        if (buf_len > 3) {
            APPEND_TO_TEMP_BUFFER(character);
        }
        if (buf_len >= 6
                && pchvml_buffer_end_with(parser->quoted_buffer,
                    "\"\"\"", 3)) {
            RESTORE_VCM_NODE();
            pchvml_buffer_delete_tail_chars(parser->temp_buffer, 3);
            struct pcvcm_node* node = pcvcm_node_new_string(
                    pchvml_buffer_get_buffer(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RESET_QUOTED_BUFFER();
            ADVANCE_TO(HVML_EJSON_AFTER_VALUE_STATE);
        }
        ADVANCE_TO(HVML_EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TEMP_BUFFER(character);
    APPEND_TO_QUOTED_BUFFER(character);
    ADVANCE_TO(HVML_EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_KEYWORD_STATE)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ','
            || character == ')') {
        RECONSUME_IN(HVML_EJSON_AFTER_KEYWORD_STATE);
    }
    if (character == '$') {
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('U');
        if (!pchvml_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
                    pchvml_buffer_get_buffer(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    if (character == 't' || character == 'f' || character == 'n') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_KEYWORD_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'r') {
        if (pchvml_buffer_equal_to(parser->temp_buffer, "t", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_KEYWORD_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'u') {
        if (pchvml_buffer_equal_to(parser->temp_buffer, "tr", 2)
           || pchvml_buffer_equal_to(parser->temp_buffer, "n", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_KEYWORD_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'e') {
        if (pchvml_buffer_equal_to(parser->temp_buffer, "tru", 3)
           || pchvml_buffer_equal_to(parser->temp_buffer, "fals", 4)
           ) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_KEYWORD_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'a') {
        if (pchvml_buffer_equal_to(parser->temp_buffer, "f", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_KEYWORD_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'l') {
        if (pchvml_buffer_equal_to(parser->temp_buffer, "nu", 2)
         || pchvml_buffer_equal_to(parser->temp_buffer, "nul", 3)
         || pchvml_buffer_equal_to(parser->temp_buffer, "fa", 2)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_KEYWORD_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 's') {
        if (pchvml_buffer_equal_to(parser->temp_buffer, "fal", 3)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_KEYWORD_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD);
        RETURN_AND_STOP_PARSE();
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_AFTER_KEYWORD_STATE)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ','
            || character == ')') {
        if (pchvml_buffer_equal_to(parser->temp_buffer, "true", 4)) {
            RESTORE_VCM_NODE();
            struct pcvcm_node* node = pcvcm_node_new_boolean(true);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(HVML_EJSON_AFTER_VALUE_STATE);
        }
        if (pchvml_buffer_equal_to(parser->temp_buffer, "false",
                    5)) {
            RESTORE_VCM_NODE();
            struct pcvcm_node* node = pcvcm_node_new_boolean(false);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(HVML_EJSON_AFTER_VALUE_STATE);
        }
        if (pchvml_buffer_equal_to(parser->temp_buffer, "null", 4)) {
            struct pcvcm_node* node = pcvcm_node_new_null();
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(HVML_EJSON_AFTER_VALUE_STATE);
        }
        RESET_TEMP_BUFFER();
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    RESET_TEMP_BUFFER();
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_BYTE_SEQUENCE_STATE)
    if (character == 'b') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_BYTE_SEQUENCE_STATE);
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_BINARY_BYTE_SEQUENCE_STATE);
    }
    if (character == 'x') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_HEX_BYTE_SEQUENCE_STATE);
    }
    if (character == '6') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_BASE64_BYTE_SEQUENCE_STATE);
    }
    if (character == '$') {
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('U');
        if (!pchvml_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
                    pchvml_buffer_get_buffer(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_AFTER_BYTE_SEQUENCE_STATE)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        struct pcvcm_node* node = pchvml_parser_new_byte_sequence(
                parser, parser->temp_buffer);
        if (node == NULL) {
            SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
        RESTORE_VCM_NODE();
        APPEND_AS_VCM_CHILD(node);
        RESET_TEMP_BUFFER();
        RECONSUME_IN(HVML_EJSON_AFTER_VALUE_STATE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_HEX_BYTE_SEQUENCE_STATE)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(HVML_EJSON_AFTER_BYTE_SEQUENCE_STATE);
    }
    else if (is_ascii_digit(character)
            || is_ascii_hex_digit(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_HEX_BYTE_SEQUENCE_STATE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_BINARY_BYTE_SEQUENCE_STATE)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(HVML_EJSON_AFTER_BYTE_SEQUENCE_STATE);
    }
    else if (is_ascii_binary_digit(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_BINARY_BYTE_SEQUENCE_STATE);
    }
    if (character == '.') {
        ADVANCE_TO(HVML_EJSON_BINARY_BYTE_SEQUENCE_STATE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_BASE64_BYTE_SEQUENCE_STATE)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(HVML_EJSON_AFTER_BYTE_SEQUENCE_STATE);
    }
    if (character == '=') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_BASE64_BYTE_SEQUENCE_STATE);
    }
    if (is_ascii_digit(character) || is_ascii_alpha(character)
            || character == '+' || character == '-') {
        if (!pchvml_buffer_end_with(parser->temp_buffer, "=", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_BASE64_BYTE_SEQUENCE_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_BASE64);
        RETURN_AND_STOP_PARSE();
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_VALUE_NUMBER_STATE)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(HVML_EJSON_AFTER_VALUE_NUMBER_STATE);
    }
    if (is_ascii_digit(character)) {
        RECONSUME_IN(HVML_EJSON_VALUE_NUMBER_INTEGER_STATE);
    }
    if (character == '-') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_VALUE_NUMBER_INTEGER_STATE);
    }
    if (character == '$') {
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('U');
        if (!pchvml_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
                    pchvml_buffer_get_buffer(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    SET_ERR(PCHVML_ERROR_BAD_JSON_NUMBER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_AFTER_VALUE_NUMBER_STATE)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        if (pchvml_buffer_end_with(parser->temp_buffer, "-", 1)
            || pchvml_buffer_end_with(parser->temp_buffer, "E", 1)
            || pchvml_buffer_end_with(parser->temp_buffer, "e", 1)) {
            SET_ERR(PCHVML_ERROR_BAD_JSON_NUMBER);
            RETURN_AND_STOP_PARSE();
        }
        double d = strtod(
                pchvml_buffer_get_buffer(parser->temp_buffer), NULL);
        RESTORE_VCM_NODE();
        struct pcvcm_node* node = pcvcm_node_new_number(d);
        APPEND_AS_VCM_CHILD(node);
        RESET_TEMP_BUFFER();
        RECONSUME_IN(HVML_EJSON_AFTER_VALUE_STATE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_VALUE_NUMBER_INTEGER_STATE)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(HVML_EJSON_AFTER_VALUE_NUMBER_STATE);
    }
    if (is_ascii_digit(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_VALUE_NUMBER_INTEGER_STATE);
    }
    if (character == 'E' || character == 'e') {
        APPEND_TO_TEMP_BUFFER('e');
        ADVANCE_TO(HVML_EJSON_VALUE_NUMBER_EXPONENT_STATE);
    }
    if (character == '.' || character == 'F') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_VALUE_NUMBER_FRACTION_STATE);
    }
    if (character == 'U' || character == 'L') {
        RECONSUME_IN(HVML_EJSON_VALUE_NUMBER_SUFFIX_INTEGER_STATE);
    }
    if (character == 'I' && (
                pchvml_buffer_is_empty(parser->temp_buffer) ||
                pchvml_buffer_equal_to(parser->temp_buffer, "-", 1)
                )) {
        RECONSUME_IN(HVML_EJSON_VALUE_NUMBER_INFINITY_STATE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_VALUE_NUMBER_FRACTION_STATE)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(HVML_EJSON_AFTER_VALUE_NUMBER_STATE);
    }

    if (is_ascii_digit(character)) {
        if (pchvml_buffer_end_with(parser->temp_buffer, "F", 1)) {
            SET_ERR(PCHVML_ERROR_BAD_JSON_NUMBER);
            RETURN_AND_STOP_PARSE();
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_VALUE_NUMBER_FRACTION_STATE);
    }
    if (character == 'F') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_VALUE_NUMBER_FRACTION_STATE);
    }
    if (character == 'L') {
        if (pchvml_buffer_end_with(parser->temp_buffer, "F", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            long double ld = strtold (
                    pchvml_buffer_get_buffer(parser->temp_buffer), NULL);
            RESTORE_VCM_NODE();
            struct pcvcm_node* node = pcvcm_node_new_longdouble(ld);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            ADVANCE_TO(HVML_EJSON_AFTER_VALUE_STATE);
        }
    }
    if (character == 'E' || character == 'e') {
        if (pchvml_buffer_end_with(parser->temp_buffer, ".", 1)) {
            SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION);
            RETURN_AND_STOP_PARSE();
        }
        APPEND_TO_TEMP_BUFFER('e');
        ADVANCE_TO(HVML_EJSON_VALUE_NUMBER_EXPONENT_STATE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_VALUE_NUMBER_EXPONENT_STATE)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(HVML_EJSON_AFTER_VALUE_NUMBER_STATE);
    }
    if (is_ascii_digit(character)) {
        RECONSUME_IN(HVML_EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE);
    }
    if (character == '+' || character == '-') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(HVML_EJSON_AFTER_VALUE_NUMBER_STATE);
    }
    if (is_ascii_digit(character)) {
        if (pchvml_buffer_end_with(parser->temp_buffer, "F", 1)) {
            SET_ERR(PCHVML_ERROR_BAD_JSON_NUMBER);
            RETURN_AND_STOP_PARSE();
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE);
    }
    if (character == 'F') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE);
    }
    if (character == 'L') {
        if (pchvml_buffer_end_with(parser->temp_buffer, "F", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            long double ld = strtold (
                    pchvml_buffer_get_buffer(parser->temp_buffer), NULL);
            RESTORE_VCM_NODE();
            struct pcvcm_node* node = pcvcm_node_new_longdouble(ld);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            ADVANCE_TO(HVML_EJSON_AFTER_VALUE_NUMBER_STATE);
        }
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_VALUE_NUMBER_SUFFIX_INTEGER_STATE)
    uint32_t last_c = pchvml_buffer_get_last_char(
            parser->temp_buffer);
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(HVML_EJSON_AFTER_VALUE_NUMBER_STATE);
    }
    if (character == 'U') {
        if (is_ascii_digit(last_c)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_VALUE_NUMBER_SUFFIX_INTEGER_STATE);
        }
    }
    if (character == 'L') {
        if (is_ascii_digit(last_c) || last_c == 'U') {
            APPEND_TO_TEMP_BUFFER(character);
            if (pchvml_buffer_end_with(parser->temp_buffer, "UL", 2)
                    ) {
                uint64_t u64 = strtoull (
                    pchvml_buffer_get_buffer(parser->temp_buffer),
                    NULL, 10);
                RESTORE_VCM_NODE();
                struct pcvcm_node* node = pcvcm_node_new_ulongint(u64);
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
                ADVANCE_TO(HVML_EJSON_AFTER_VALUE_STATE);
            }
            else if (pchvml_buffer_end_with(parser->temp_buffer,
                        "L", 1)) {
                int64_t i64 = strtoll (
                    pchvml_buffer_get_buffer(parser->temp_buffer),
                    NULL, 10);
                RESTORE_VCM_NODE();
                struct pcvcm_node* node = pcvcm_node_new_longint(i64);
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
                ADVANCE_TO(HVML_EJSON_AFTER_VALUE_STATE);
            }
        }
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_VALUE_NUMBER_INFINITY_STATE)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        if (pchvml_buffer_equal_to(parser->temp_buffer,
                    "-Infinity", 9)) {
            double d = -INFINITY;
            RESTORE_VCM_NODE();
            struct pcvcm_node* node = pcvcm_node_new_number(d);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(HVML_EJSON_AFTER_VALUE_STATE);
        }
        if (pchvml_buffer_equal_to(parser->temp_buffer,
                "Infinity", 8)) {
            double d = INFINITY;
            RESTORE_VCM_NODE();
            struct pcvcm_node* node = pcvcm_node_new_number(d);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(HVML_EJSON_AFTER_VALUE_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'I') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)
            || pchvml_buffer_equal_to(parser->temp_buffer, "-", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_VALUE_NUMBER_INFINITY_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (character == 'n') {
        if (pchvml_buffer_equal_to(parser->temp_buffer, "I", 1)
          || pchvml_buffer_equal_to(parser->temp_buffer, "-I", 2)
          || pchvml_buffer_equal_to(parser->temp_buffer, "Infi", 4)
          || pchvml_buffer_equal_to(parser->temp_buffer, "-Infi", 5)
            ) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_VALUE_NUMBER_INFINITY_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (character == 'f') {
        if (pchvml_buffer_equal_to(parser->temp_buffer, "In", 2)
            || pchvml_buffer_equal_to (parser->temp_buffer, "-In", 3)
                ) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_VALUE_NUMBER_INFINITY_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (character == 'i') {
        if (pchvml_buffer_equal_to(parser->temp_buffer, "Inf", 3)
         || pchvml_buffer_equal_to(parser->temp_buffer, "-Inf", 4)
         || pchvml_buffer_equal_to(parser->temp_buffer, "Infin", 5)
         || pchvml_buffer_equal_to(parser->temp_buffer, "-Infin", 6)
         ) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_VALUE_NUMBER_INFINITY_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (character == 't') {
        if (pchvml_buffer_equal_to(parser->temp_buffer, "Infini", 6)
            || pchvml_buffer_equal_to (parser->temp_buffer,
                "-Infini", 7)
                ) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_VALUE_NUMBER_INFINITY_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (character == 'y') {
        if (pchvml_buffer_equal_to(parser->temp_buffer, "Infinit", 7)
           || pchvml_buffer_equal_to (parser->temp_buffer,
               "-Infinit", 8)
                ) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_VALUE_NUMBER_INFINITY_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_VALUE_NAN_STATE)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        if (pchvml_buffer_equal_to(parser->temp_buffer, "NaN", 3)) {
            double d = NAN;
            RESTORE_VCM_NODE();
            struct pcvcm_node* node = pcvcm_node_new_number(d);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(HVML_EJSON_AFTER_VALUE_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'N') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)
          || pchvml_buffer_equal_to(parser->temp_buffer, "Na", 2)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_VALUE_NAN_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (character == 'a') {
        if (pchvml_buffer_equal_to(parser->temp_buffer, "N", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_VALUE_NAN_STATE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_STRING_ESCAPE_STATE)
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
              HVML_EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS_STATE);
            break;
        default:
            SET_ERR(PCHVML_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY);
            RETURN_AND_STOP_PARSE();
    }
END_STATE()

BEGIN_STATE(HVML_EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS_STATE)
    if (is_ascii_hex_digit(character)) {
        APPEND_TO_STRING_BUFFER(character);
        size_t nr_chars = pchvml_buffer_get_size_in_chars(
                parser->string_buffer);
        if (nr_chars == 4) {
            APPEND_BYTES_TO_TEMP_BUFFER("\\u", 2);
            APPEND_BUFFER_TO_TEMP_BUFFER(parser->string_buffer);
            RESET_STRING_BUFFER();
            ADVANCE_TO(parser->return_state);
        }
        ADVANCE_TO(
            HVML_EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS_STATE);
    }
    SET_ERR(PCHVML_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_JSONEE_VARIABLE_STATE)
    if (character == '"') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            RECONSUME_IN(HVML_EJSON_VALUE_DOUBLE_QUOTED_STATE);
        }
    }
    if (character == '\'') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            RECONSUME_IN(HVML_EJSON_VALUE_SINGLE_QUOTED_STATE);
        }
    }
    if (character == '$') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   pchvml_buffer_get_buffer(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        uint32_t uc = ejson_stack_top();
        while (uc == '$') {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            uc = ejson_stack_top();
        }
        if (uc == '(' || uc == '<' || uc == '.' || uc == '"') {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    if (is_context_variable(character)) {
        if (pchvml_buffer_is_empty(parser->temp_buffer)
            || pchvml_buffer_is_int(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_JSONEE_VARIABLE_STATE);
        }
        SET_ERR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '_' || is_ascii_digit(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_JSONEE_VARIABLE_STATE);
    }
    if (is_ascii_alpha(character) || character == '-') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_JSONEE_VARIABLE_STATE);
    }
    if (is_whitespace(character) || character == '}'
            || character == '"' || character == ']' || character == ')') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
            RETURN_AND_STOP_PARSE();
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   pchvml_buffer_get_buffer(parser->temp_buffer));
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
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    if (character == ',') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
            RETURN_AND_STOP_PARSE();
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   pchvml_buffer_get_buffer(parser->temp_buffer));
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
        RECONSUME_IN(HVML_EJSON_AFTER_VALUE_STATE);
    }
    if (character == ':') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_JSONEE_VARIABLE_STATE);
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   pchvml_buffer_get_buffer(parser->temp_buffer));
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
            struct pcvcm_node* node = pcvcm_node_new_object(0, NULL);
            APPEND_CHILD(node, parser->vcm_node);
            UPDATE_VCM_NODE(node);
        }
        if (ejson_stack_is_empty()) {
            RECONSUME_IN(HVML_EJSON_FINISHED_STATE);
        }
        ADVANCE_TO(HVML_EJSON_CONTROL_STATE);
    }
    if (character == '[' || character == '(') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
            RETURN_AND_STOP_PARSE();
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   pchvml_buffer_get_buffer(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        uint32_t uc = ejson_stack_top();
        if (uc == '$') {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    if (character == '<' || character == '>') {
        // FIXME
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   pchvml_buffer_get_buffer(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        uint32_t uc = ejson_stack_top();
        if (uc == '$') {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    if (character == '.') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
            RETURN_AND_STOP_PARSE();
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   pchvml_buffer_get_buffer(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        uint32_t uc = ejson_stack_top();
        if (uc == '$') {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        RECONSUME_IN(HVML_EJSON_JSONEE_FULL_STOP_SIGN_STATE);
    }
    if (character == '=') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(HVML_EJSON_JSONEE_VARIABLE_STATE);
        }
    }
    SET_ERR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_JSONEE_KEYWORD_STATE)
    if (is_ascii_digit(character)) {
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
            RETURN_AND_STOP_PARSE();
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_JSONEE_KEYWORD_STATE);
    }
    if (is_ascii_alpha(character) || character == '_' ||
            character == '-') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_JSONEE_KEYWORD_STATE);
    }
    if (is_whitespace(character) || character == '[' ||
            character == '(' || character == '<' || character == '}' ||
            character == '$' || character == '>' || character == ']'
            || character == ')' || character == '"') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
            RETURN_AND_STOP_PARSE();
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   pchvml_buffer_get_buffer(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        ejson_stack_pop();
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    if (character == ',') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
            RETURN_AND_STOP_PARSE();
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   pchvml_buffer_get_buffer(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        ejson_stack_pop();
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        uint32_t uc = ejson_stack_top();
        if (uc == '(' || uc == '<') {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        RECONSUME_IN(HVML_EJSON_AFTER_VALUE_STATE);
    }
    if (character == '.') {
        if (pchvml_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
            RETURN_AND_STOP_PARSE();
        }
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                   pchvml_buffer_get_buffer(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        ejson_stack_pop();
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        RECONSUME_IN(HVML_EJSON_JSONEE_FULL_STOP_SIGN_STATE);
    }
    SET_ERR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_JSONEE_STRING_STATE)
    uint32_t uc = ejson_stack_top();
    if (is_whitespace(character)) {
        if (uc == 'U') {
            RECONSUME_IN(HVML_EJSON_AFTER_JSONEE_STRING_STATE);
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_JSONEE_STRING_STATE);
    }
    if (character == '$') {
        if (uc != 'U' && uc != '"') {
            if (parser->vcm_node) {
                vcm_stack_push(parser->vcm_node);
            }
            struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                    NULL);
            UPDATE_VCM_NODE(snode);
            ejson_stack_push('"');
            if (!pchvml_buffer_is_empty(parser->temp_buffer)) {
                struct pcvcm_node* node = pcvcm_node_new_string(
                   pchvml_buffer_get_buffer(parser->temp_buffer));
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
                ADVANCE_TO(HVML_EJSON_JSONEE_STRING_STATE);
            }
        }
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    if (character == '\\') {
        SET_RETURN_STATE(curr_state);
        ADVANCE_TO(HVML_EJSON_STRING_ESCAPE_STATE);
    }
    if (character == '"') {
        if (parser->vcm_node) {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            vcm_stack_push(parser->vcm_node);
        }
        parser->vcm_node = pcvcm_node_new_string(
                pchvml_buffer_get_buffer(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        RECONSUME_IN(HVML_EJSON_AFTER_JSONEE_STRING_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == ':' && uc == ':') {
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        RESET_TEMP_BUFFER();
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(HVML_EJSON_JSONEE_STRING_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_AFTER_JSONEE_STRING_STATE)
    uint32_t uc = ejson_stack_top();
    if (is_whitespace(character)) {
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        if (uc == 'U') {
            ejson_stack_pop();
            if (!ejson_stack_is_empty()) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
        }
        RECONSUME_IN(HVML_EJSON_JSONEE_STRING_STATE);
    }
    if (character == '"') {
        if (uc == 'U') {
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_NAME);
            RETURN_AND_STOP_PARSE();
        }
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        ejson_stack_pop();
        if (!ejson_stack_is_empty()) {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        ADVANCE_TO(HVML_EJSON_CONTROL_STATE);
    }
    if (character == '}' || character == ']' || character == ')') {
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        ejson_stack_pop();
        if (!ejson_stack_is_empty()) {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        ADVANCE_TO(HVML_EJSON_CONTROL_STATE);
    }
    SET_ERR(PCHVML_ERROR_BAD_JSONEE_NAME);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(HVML_EJSON_TEMPLATE_DATA_STATE)
    if (character == '<') {
        if (!pchvml_buffer_is_empty(parser->temp_buffer) &&
                !pchvml_buffer_is_whitespace(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
                    pchvml_buffer_get_buffer(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        ADVANCE_TO(HVML_EJSON_TEMPLATE_DATA_LESS_THAN_SIGN_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '$') {
        if (!pchvml_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
                    pchvml_buffer_get_buffer(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(HVML_EJSON_CONTROL_STATE);
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(HVML_EJSON_TEMPLATE_DATA_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_TEMPLATE_DATA_LESS_THAN_SIGN_STATE)
    if (character == '/') {
        RESET_TEMP_BUFFER();
        ADVANCE_TO(HVML_EJSON_TEMPLATE_DATA_END_TAG_OPEN_STATE);
    }
    APPEND_TO_TEMP_BUFFER('<');
    RECONSUME_IN(HVML_EJSON_TEMPLATE_DATA_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_TEMPLATE_DATA_END_TAG_OPEN_STATE)
    if (is_ascii_alpha(character)) {
        RESET_STRING_BUFFER();
        RECONSUME_IN(HVML_EJSON_TEMPLATE_DATA_END_TAG_NAME_STATE);
    }
    APPEND_TO_TEMP_BUFFER('<');
    APPEND_TO_TEMP_BUFFER('/');
    RECONSUME_IN(HVML_EJSON_TEMPLATE_DATA_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_TEMPLATE_DATA_END_TAG_NAME_STATE)
    if (is_ascii_alpha(character)) {
        APPEND_TO_STRING_BUFFER(character);
        ADVANCE_TO(HVML_EJSON_TEMPLATE_DATA_END_TAG_NAME_STATE);
    }
    if (character == '>') {
        const char* name = pchvml_buffer_get_buffer(
                parser->string_buffer);
        if (pchvml_parser_is_appropriate_tag_name(parser, name)) {
            RECONSUME_IN(HVML_EJSON_TEMPLATE_FINISHED_STATE);
        }
    }
    APPEND_TO_TEMP_BUFFER('<');
    APPEND_TO_TEMP_BUFFER('/');
    APPEND_BUFFER_TO_TEMP_BUFFER(parser->string_buffer);
    RESET_STRING_BUFFER();
    RECONSUME_IN(HVML_EJSON_TEMPLATE_DATA_STATE);
END_STATE()

BEGIN_STATE(HVML_EJSON_TEMPLATE_FINISHED_STATE)
    while (!vcm_stack_is_empty()) {
        ejson_stack_pop();
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
    }

    struct pchvml_token* token = pchvml_token_new_vcm(parser->vcm_node);
    struct pchvml_token* next_token = pchvml_token_new_end_tag();
    pchvml_token_append_buffer_to_name(next_token,
            parser->string_buffer);
    RESET_VCM_NODE();
    RESET_STRING_BUFFER();
    ejson_stack_pop();
    RETURN_MULTIPLE_AND_SWITCH_TO(token, next_token, HVML_DATA_STATE);
END_STATE()

PCHVML_NEXT_TOKEN_END

#endif


