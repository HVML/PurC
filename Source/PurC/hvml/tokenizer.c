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
#include "private/tkz-helper.h"

#include "hvml-token.h"
#include "hvml-attr.h"
#include "hvml-tag.h"

#include <math.h>

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

#define ERROR_BUF_SIZE  100

#define PRINT_STATE(state_name)                                             \
    if (parser->enable_log) {                                               \
        size_t len;                                                         \
        char *s = pcvcm_node_to_string(parser->vcm_node, &len);             \
        PC_DEBUG(                                                           \
            "in %s|uc=%c|hex=0x%X|stack_is_empty=%d"                        \
            "|stack_top=%c|stack_size=%ld|vcm_node=%s|fh=%d\n",             \
            curr_state_name, character, character,                          \
            ejson_stack_is_empty(), (char)ejson_stack_top(),                \
            ejson_stack_size(), s, parser->is_in_file_header);              \
        free(s); \
    }

#define SET_ERR(err)    do {                                                \
    purc_variant_t exinfo = PURC_VARIANT_INVALID;                           \
    if (parser->curr_uc) {                                                  \
        char buf[ERROR_BUF_SIZE+1];                                         \
        snprintf(buf, ERROR_BUF_SIZE,                                       \
                "line=%d, column=%d, character=%c",                         \
                parser->curr_uc->line,                                      \
                parser->curr_uc->column,                                    \
                parser->curr_uc->character);                                \
        exinfo = purc_variant_make_string(buf, false);                      \
        if (parser->enable_log) {                                           \
            PC_DEBUG( "%s:%d|%s|%s\n", __FILE__, __LINE__, #err, buf);      \
        }                                                                   \
    }                                                                       \
    purc_set_error_exinfo(err, exinfo);                                     \
} while (0)

#define PCHVML_NEXT_TOKEN_BEGIN                                         \
struct pchvml_token* pchvml_next_token(struct pchvml_parser* parser,    \
                                          purc_rwstream_t rws)          \
{                                                                       \
    uint32_t character = 0;                                             \
    if (parser->token) {                                                \
        struct pchvml_token* token = parser->token;                     \
        parser->token = NULL;                                           \
        parser->last_token_type = pchvml_token_get_type(token);         \
        return token;                                                   \
    }                                                                   \
                                                                        \
    tkz_reader_set_rwstream (parser->reader, rws);                 \
                                                                        \
next_input:                                                             \
    parser->curr_uc = tkz_reader_next_char (parser->reader);       \
    if (!parser->curr_uc) {                                             \
        return NULL;                                                    \
    }                                                                   \
                                                                        \
    character = parser->curr_uc->character;                             \
    if (character == TKZ_INVALID_CHARACTER) {                           \
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
        tkz_buffer_to_vcm_node(parser->temp_buffer)

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

#define BEGIN_STATE(state_name)                                             \
    case state_name:                                                        \
    {                                                                       \
        const char* curr_state_name = ""#state_name;                        \
        int curr_state = state_name;                                        \
        UNUSED_PARAM(curr_state_name);                                      \
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

#define RESET_TRANSIT_STATE()                                               \
    do {                                                                    \
        parser->transit_state = TKZ_STATE_DATA;                             \
    } while (false)

#define CHECK_TEMPLATE_TAG_AND_SWITCH_STATE(token)                          \
    do {                                                                    \
        const char* name = pchvml_token_get_name(token);                    \
        if (pchvml_token_is_type(token, PCHVML_TOKEN_START_TAG) &&          \
                pchvml_parser_is_template_tag(name)) {                      \
            parser->state = TKZ_STATE_EJSON_DATA;                           \
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
        parser->last_token_type = pchvml_token_get_type(token);             \
        return token;                                                       \
    } while (false)

#define RETURN_AND_RECONSUME_IN(next_state)                                 \
    do {                                                                    \
        parser->state = next_state;                                         \
        pchvml_parser_save_tag_name(parser);                                \
        pchvml_token_done(parser->token);                                   \
        struct pchvml_token* token = parser->token;                         \
        parser->token = NULL;                                               \
        tkz_reader_reconsume_last_char(parser->reader);                \
        parser->last_token_type = pchvml_token_get_type(token);             \
        return token;                                                       \
    } while (false)

#define RETURN_CURRENT_TOKEN()                                              \
    do {                                                                    \
        pchvml_token_done(parser->token);                                   \
        struct pchvml_token* token = parser->token;                         \
        parser->token = NULL;                                               \
        parser->last_token_type = pchvml_token_get_type(token);             \
        return token;                                                       \
    } while (false)

#define RETURN_NEW_EOF_TOKEN()                                              \
    do {                                                                    \
        if (parser->token) {                                                \
            struct pchvml_token* token = parser->token;                     \
            parser->token = pchvml_token_new_eof();                         \
            parser->last_token_type = pchvml_token_get_type(token);         \
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
        parser->last_token_type = pchvml_token_get_type(token);             \
        pchvml_parser_save_tag_name(parser);                                \
        return token;                                                       \
    } while (false)

#define RETURN_AND_STOP_PARSE()                                             \
    do {                                                                    \
        return NULL;                                                        \
    } while (false)

#define RESET_TEMP_BUFFER()                                                 \
    do {                                                                    \
        tkz_buffer_reset(parser->temp_buffer);                           \
    } while (false)

#define APPEND_TO_TEMP_BUFFER(c)                                            \
    do {                                                                    \
        tkz_buffer_append(parser->temp_buffer, c);                       \
    } while (false)

#define APPEND_BYTES_TO_TEMP_BUFFER(bytes, nr_bytes)                        \
    do {                                                                    \
        tkz_buffer_append_bytes(parser->temp_buffer, bytes, nr_bytes);   \
    } while (false)

#define APPEND_BUFFER_TO_TEMP_BUFFER(buffer)                                \
    do {                                                                    \
        tkz_buffer_append_another(parser->temp_buffer, buffer);          \
    } while (false)

#define IS_TEMP_BUFFER_EMPTY()                                              \
        tkz_buffer_is_empty(parser->temp_buffer)

#define RESET_STRING_BUFFER()                                               \
    do {                                                                    \
        tkz_buffer_reset(parser->string_buffer);                         \
    } while (false)

#define APPEND_TO_STRING_BUFFER(uc)                                         \
    do {                                                                    \
        tkz_buffer_append(parser->string_buffer, uc);                    \
    } while (false)

#define APPEND_BYTES_TO_STRING_BUFFER(bytes, nr_bytes)                      \
    do {                                                                    \
        tkz_buffer_append_bytes(parser->string_buffer, bytes, nr_bytes); \
    } while (false)

#define RESET_QUOTED_COUNTER()                                              \
    do {                                                                    \
        parser->nr_quoted = 0;                                              \
    } while (false)

#define APPEND_TO_TOKEN_NAME(uc)                                            \
    do {                                                                    \
        pchvml_token_append_to_name(parser->token, uc);                     \
    } while (false)

#define APPEND_TEMP_BUFFER_TO_TOKEN_TEXT()                                  \
    do {                                                                    \
        const char* c = tkz_buffer_get_bytes(parser->temp_buffer);      \
        size_t nr_c = tkz_buffer_get_size_in_bytes(                      \
                parser->temp_buffer);                                       \
        pchvml_token_append_bytes_to_text(parser->token, c, nr_c);          \
        tkz_buffer_reset(parser->temp_buffer);                           \
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

#define APPEND_TO_TOKEN_ATTR_VALUE(uc)                                      \
    do {                                                                    \
        pchvml_token_append_to_attr_value(parser->token, uc);               \
    } while (false)

#define APPEND_BYTES_TO_TOKEN_ATTR_VALUE(c, nr_c)                           \
    do {                                                                    \
        pchvml_token_append_bytes_to_attr_value(parser->token, c, nr_c);    \
    } while (false)

#define APPEND_BUFFER_TO_TOKEN_ATTR_VALUE(buffer)                           \
    do {                                                                    \
        const char* c = tkz_buffer_get_bytes(buffer);                   \
        size_t nr_c = tkz_buffer_get_size_in_bytes(buffer);              \
        pchvml_token_append_bytes_to_attr_value(parser->token, c, nr_c);    \
    } while (false)

#define APPEND_TO_TOKEN_ATTR_NAME(c)                                        \
    do {                                                                    \
        pchvml_token_append_to_attr_name(parser->token, c);                 \
    } while (false)

#define APPEND_TEMP_BUFFER_TO_TOKEN_ATTR_NAME()                             \
    do {                                                                    \
        const char* c = tkz_buffer_get_bytes(parser->temp_buffer);      \
        size_t nr_c = tkz_buffer_get_size_in_bytes(                      \
                parser->temp_buffer);                                       \
        pchvml_token_append_bytes_to_attr_name(parser->token, c, nr_c);     \
        tkz_buffer_reset(parser->temp_buffer);                           \
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
    return (strcmp(name, "init") == 0
            || strcmp(name, "archedata") == 0
            || strcmp(name, "update") == 0
            || strcmp(name, "bind") == 0
            || strcmp(name, "hvml") == 0);
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
        parser->tag_has_raw_attr = pchvml_token_has_raw_attr(parser->token);
        tkz_buffer_reset(parser->tag_name);
        tkz_buffer_append_bytes(parser->tag_name,
                name, strlen(name));
    }
    if (pchvml_token_is_type (parser->token, PCHVML_TOKEN_END_TAG)) {
        tkz_buffer_reset(parser->tag_name);
        parser->tag_is_operation = false;
        parser->tag_has_raw_attr = false;
    }
}

static UNUSED_FUNCTION
bool pchvml_parser_is_appropriate_end_tag(struct pchvml_parser* parser)
{
    const char* name = pchvml_token_get_name(parser->token);
    return tkz_buffer_equal_to (parser->tag_name, name,
            strlen(name));
}

static UNUSED_FUNCTION
bool pchvml_parser_is_appropriate_tag_name(struct pchvml_parser* parser,
        const char* name)
{
    return tkz_buffer_equal_to (parser->tag_name, name,
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
    const char* name = tkz_buffer_get_bytes(parser->tag_name);
    return pchvml_parser_is_template_tag(name);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_in_json_content_tag (struct pchvml_parser* parser)
{
    const char* name = tkz_buffer_get_bytes(parser->tag_name);
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
            && (strcmp(name, "on") == 0 ||
                strcmp(name, "with") == 0)) {
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
bool pchvml_parser_is_in_raw_template (struct pchvml_parser* parser)
{
    const char* name = tkz_buffer_get_bytes(parser->tag_name);
    const struct pchvml_tag_entry* entry = pchvml_tag_static_search(name,
            strlen(name));
    bool template = (entry && (entry->id == PCHVML_TAG_ARCHETYPE
                || entry->id == PCHVML_TAG_ARCHEDATA
                || entry->id == PCHVML_TAG_ERROR
                || entry->id == PCHVML_TAG_EXCEPT));
    return template && parser->tag_has_raw_attr;
}

static UNUSED_FUNCTION
struct pcvcm_node* pchvml_parser_new_byte_sequence (struct pchvml_parser* hvml,
    struct tkz_buffer* buffer)
{
    UNUSED_PARAM(hvml);
    UNUSED_PARAM(buffer);
    size_t nr_bytes = tkz_buffer_get_size_in_bytes(buffer);
    const char* bytes = tkz_buffer_get_bytes(buffer);
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
struct pcvcm_node* tkz_buffer_to_vcm_node(struct tkz_buffer* buffer)
{
    return buffer ? pcvcm_node_new_string(
                    tkz_buffer_get_bytes(buffer)) : NULL;
}

static UNUSED_FUNCTION
bool pchvml_parser_is_in_attribute (struct pchvml_parser* parser)
{
    return parser->token && pchvml_token_is_in_attr(parser->token);
}

void pchvml_switch_to_ejson_state(struct pchvml_parser* parser)
{
    parser->state = TKZ_STATE_EJSON_DATA;
}

PCHVML_NEXT_TOKEN_BEGIN


BEGIN_STATE(TKZ_STATE_DATA)
    if (character == '#' && parser->is_in_file_header) {
        ADVANCE_TO(TKZ_STATE_HASH);
    }
    if (is_whitespace(character) && parser->is_in_file_header) {
        ADVANCE_TO(TKZ_STATE_DATA);
    }
    if (character == '&') {
        SET_RETURN_STATE(TKZ_STATE_DATA);
        ADVANCE_TO(TKZ_STATE_CHARACTER_REFERENCE);
    }
    if (character == '<') {
        if (parser->token) {
            RETURN_AND_SWITCH_TO(TKZ_STATE_TAG_OPEN);
        }
        ADVANCE_TO(TKZ_STATE_TAG_OPEN);
    }
    if (is_eof(character)) {
        RETURN_NEW_EOF_TOKEN();
    }
    RESET_TEMP_BUFFER();
    parser->nr_whitespace = 0;
    RECONSUME_IN(TKZ_STATE_TAG_CONTENT);
END_STATE()

BEGIN_STATE(TKZ_STATE_HASH)
    if (character == '\n') {
        ADVANCE_TO(TKZ_STATE_DATA);
    }
    ADVANCE_TO(TKZ_STATE_HASH);
END_STATE()

BEGIN_STATE(TKZ_STATE_TAG_OPEN)
    parser->is_in_file_header = false;
    if (character == '!') {
        ADVANCE_TO(TKZ_STATE_MARKUP_DECLARATION_OPEN);
    }
    if (character == '/') {
        ADVANCE_TO(TKZ_STATE_END_TAG_OPEN);
    }
    if (is_ascii_alpha(character)) {
        parser->token = pchvml_token_new_start_tag ();
        RECONSUME_IN(TKZ_STATE_TAG_NAME);
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

BEGIN_STATE(TKZ_STATE_END_TAG_OPEN)
    if (is_ascii_alpha(character)) {
        parser->token = pchvml_token_new_end_tag();
        RECONSUME_IN(TKZ_STATE_TAG_NAME);
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

BEGIN_STATE(TKZ_STATE_TAG_CONTENT)
    if (is_eof(character)) {
        RETURN_NEW_EOF_TOKEN();
    }
    if (is_whitespace(character)) {
        parser->nr_whitespace++;
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_TAG_CONTENT);
    }
    parser->nr_whitespace = 0;
    if (character == '<') {
        if(!IS_TEMP_BUFFER_EMPTY()) {
            struct pcvcm_node* node = TEMP_BUFFER_TO_VCM_NODE();
            if (!node) {
                RETURN_AND_STOP_PARSE();
            }
            RESET_TEMP_BUFFER();
            parser->token = pchvml_token_new_vcm(node);
            if (!parser->token) {
                RETURN_AND_STOP_PARSE();
            }
            pchvml_token_set_is_whitespace(parser->token, true);
            RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
        }
        RECONSUME_IN(TKZ_STATE_DATA);
    }
    if (pchvml_parser_is_in_json_content_tag(parser)) {
        if(!IS_TEMP_BUFFER_EMPTY()) {
            struct pcvcm_node* node = TEMP_BUFFER_TO_VCM_NODE();
            if (!node) {
                RETURN_AND_STOP_PARSE();
            }
            RESET_TEMP_BUFFER();
            parser->token = pchvml_token_new_vcm(node);
            if (!parser->token) {
                RETURN_AND_STOP_PARSE();
            }
            pchvml_token_set_is_whitespace(parser->token, true);
            RETURN_AND_RECONSUME_IN(TKZ_STATE_JSONTEXT_CONTENT);
        }
        RECONSUME_IN(TKZ_STATE_JSONTEXT_CONTENT);
    }
    if (character == '{' || character == '[' || character == '$') {
        if(!IS_TEMP_BUFFER_EMPTY()) {
            struct pcvcm_node* node = TEMP_BUFFER_TO_VCM_NODE();
            if (!node) {
                RETURN_AND_STOP_PARSE();
            }
            RESET_TEMP_BUFFER();
            parser->token = pchvml_token_new_vcm(node);
            if (!parser->token) {
                RETURN_AND_STOP_PARSE();
            }
            pchvml_token_set_is_whitespace(parser->token, true);
            RETURN_AND_RECONSUME_IN(TKZ_STATE_JSONTEXT_CONTENT);
        }
        RECONSUME_IN(TKZ_STATE_JSONTEXT_CONTENT);
    }
    if (character == '\'' || character == '"') {
        if(!IS_TEMP_BUFFER_EMPTY()) {
            struct pcvcm_node* node = TEMP_BUFFER_TO_VCM_NODE();
            if (!node) {
                RETURN_AND_STOP_PARSE();
            }
            RESET_TEMP_BUFFER();
            parser->token = pchvml_token_new_vcm(node);
            if (!parser->token) {
                RETURN_AND_STOP_PARSE();
            }
            pchvml_token_set_is_whitespace(parser->token, true);
            RETURN_AND_RECONSUME_IN(TKZ_STATE_JSONTEXT_CONTENT);
        }
        RECONSUME_IN(TKZ_STATE_JSONTEXT_CONTENT);
    }
    if ((parser->last_token_type == PCHVML_TOKEN_START_TAG
                || parser->last_token_type == PCHVML_TOKEN_END_TAG)
            && !IS_TEMP_BUFFER_EMPTY()) {
        struct pcvcm_node* node = TEMP_BUFFER_TO_VCM_NODE();
        if (!node) {
            RETURN_AND_STOP_PARSE();
        }
        RESET_TEMP_BUFFER();
        parser->token = pchvml_token_new_vcm(node);
        if (!parser->token) {
            RETURN_AND_STOP_PARSE();
        }
        pchvml_token_set_is_whitespace(parser->token, true);
        RETURN_AND_RECONSUME_IN(TKZ_STATE_TEXT_CONTENT);
    }
    RECONSUME_IN(TKZ_STATE_TEXT_CONTENT);
END_STATE()

BEGIN_STATE(TKZ_STATE_TAG_NAME)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_BEFORE_ATTRIBUTE_NAME);
    }
    if (character == '/') {
        ADVANCE_TO(TKZ_STATE_SELF_CLOSING_START_TAG);
    }
    if (character == '>') {
        RETURN_AND_SWITCH_TO(TKZ_STATE_DATA);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TOKEN_NAME(character);
    ADVANCE_TO(TKZ_STATE_TAG_NAME);
END_STATE()

BEGIN_STATE(TKZ_STATE_BEFORE_ATTRIBUTE_NAME)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_BEFORE_ATTRIBUTE_NAME);
    }
    if (character == '/' || character == '>') {
        RECONSUME_IN(TKZ_STATE_AFTER_ATTRIBUTE_NAME);
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
    RECONSUME_IN(TKZ_STATE_ATTRIBUTE_NAME);
END_STATE()

BEGIN_STATE(TKZ_STATE_ATTRIBUTE_NAME)
    if (is_whitespace(character) || character == '>') {
        RECONSUME_IN(TKZ_STATE_AFTER_ATTRIBUTE_NAME);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '=') {
        ADVANCE_TO(TKZ_STATE_BEFORE_ATTRIBUTE_VALUE);
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
        TKZ_STATE_SPECIAL_ATTRIBUTE_OPERATOR_IN_ATTRIBUTE_NAME);
    }
    if (character == '/') {
        RECONSUME_IN(TKZ_STATE_AFTER_ATTRIBUTE_NAME);
    }
    APPEND_TO_TOKEN_ATTR_NAME(character);
    ADVANCE_TO(TKZ_STATE_ATTRIBUTE_NAME);
END_STATE()

BEGIN_STATE(TKZ_STATE_AFTER_ATTRIBUTE_NAME)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_AFTER_ATTRIBUTE_NAME);
    }
    if (character == '=') {
        ADVANCE_TO(TKZ_STATE_BEFORE_ATTRIBUTE_VALUE);
    }
    if (character == '>') {
        END_TOKEN_ATTR();
        RETURN_AND_SWITCH_TO(TKZ_STATE_DATA);
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
        TKZ_STATE_SPECIAL_ATTRIBUTE_OPERATOR_AFTER_ATTRIBUTE_NAME
        );
    }
    if (pchvml_parser_is_operation_tag_token(parser->token)
        && pchvml_parser_is_preposition_attribute(
                pchvml_token_get_curr_attr(parser->token))) {
        RECONSUME_IN(TKZ_STATE_BEFORE_ATTRIBUTE_VALUE);
    }
    if (character == '/') {
        END_TOKEN_ATTR();
        ADVANCE_TO(TKZ_STATE_SELF_CLOSING_START_TAG);
    }
    END_TOKEN_ATTR();
    BEGIN_TOKEN_ATTR();
    RECONSUME_IN(TKZ_STATE_ATTRIBUTE_NAME);
END_STATE()

BEGIN_STATE(TKZ_STATE_BEFORE_ATTRIBUTE_VALUE)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_BEFORE_ATTRIBUTE_VALUE);
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
        RESET_TEMP_BUFFER();
        ADVANCE_TO(TKZ_STATE_JSONEE_ATTRIBUTE_VALUE_DOUBLE_QUOTED);
    }
    if (character == '\'') {
        ADVANCE_TO(TKZ_STATE_JSONEE_ATTRIBUTE_VALUE_SINGLE_QUOTED);
    }
    RESET_TEMP_BUFFER();
    ejson_stack_push('U');
    RECONSUME_IN(TKZ_STATE_EJSON_DATA);
END_STATE()

BEGIN_STATE(TKZ_STATE_AFTER_ATTRIBUTE_VALUE)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_BEFORE_ATTRIBUTE_NAME);
    }
    if (character == '/') {
        ADVANCE_TO(TKZ_STATE_SELF_CLOSING_START_TAG);
    }
    if (character == '>') {
        RETURN_AND_SWITCH_TO(TKZ_STATE_DATA);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_NEW_EOF_TOKEN();
    }
    SET_ERR(PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_ATTRIBUTES);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_SELF_CLOSING_START_TAG)
    if (character == '>') {
        pchvml_token_set_self_closing(parser->token, true);
        RETURN_AND_SWITCH_TO(TKZ_STATE_DATA);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_NEW_EOF_TOKEN();
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_SOLIDUS_IN_TAG);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_MARKUP_DECLARATION_OPEN)
    if (parser->sbst == NULL) {
        parser->sbst = tkz_sbst_new_markup_declaration_open_state();
    }
    bool ret = tkz_sbst_advance_ex(parser->sbst, character, false);
    if (!ret) {
        SET_ERR(PCHVML_ERROR_INCORRECTLY_OPENED_COMMENT);
        tkz_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        RETURN_AND_STOP_PARSE();
    }

    const char* value = tkz_sbst_get_match(parser->sbst);
    if (value == NULL) {
        ADVANCE_TO(TKZ_STATE_MARKUP_DECLARATION_OPEN);
    }

    if (strcmp(value, "--") == 0) {
        parser->token = pchvml_token_new_comment();
        tkz_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        RESET_TEMP_BUFFER();
        ADVANCE_TO(TKZ_STATE_COMMENT_START);
    }
    if (strcmp(value, "DOCTYPE") == 0) {
        tkz_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        ADVANCE_TO(TKZ_STATE_DOCTYPE);
    }
    if (strcmp(value, "[CDATA[") == 0) {
        tkz_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        RESET_TEMP_BUFFER();
        ADVANCE_TO(TKZ_STATE_CDATA_SECTION);
    }
    SET_ERR(PCHVML_ERROR_INCORRECTLY_OPENED_COMMENT);
    tkz_sbst_destroy(parser->sbst);
    parser->sbst = NULL;
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_COMMENT_START)
    if (character == '-') {
        ADVANCE_TO(TKZ_STATE_COMMENT_START_DASH);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_ABRUPT_CLOSING_OF_EMPTY_COMMENT);
        RETURN_AND_STOP_PARSE();
    }
    RECONSUME_IN(TKZ_STATE_COMMENT);
END_STATE()

BEGIN_STATE(TKZ_STATE_COMMENT_START_DASH)
    if (character == '-') {
        ADVANCE_TO(TKZ_STATE_COMMENT_END);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_ABRUPT_CLOSING_OF_EMPTY_COMMENT);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_COMMENT);
        RETURN_NEW_EOF_TOKEN();
    }
    APPEND_TO_TEMP_BUFFER('-');
    RECONSUME_IN(TKZ_STATE_COMMENT);
END_STATE()

BEGIN_STATE(TKZ_STATE_COMMENT)
    if (character == '<') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_COMMENT_LESS_THAN_SIGN);
    }
    if (character == '-') {
        ADVANCE_TO(TKZ_STATE_COMMENT_END_DASH);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_COMMENT);
        RETURN_NEW_EOF_TOKEN();
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(TKZ_STATE_COMMENT);
END_STATE()

BEGIN_STATE(TKZ_STATE_COMMENT_LESS_THAN_SIGN)
    if (character == '!') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_COMMENT_LESS_THAN_SIGN_BANG);
    }
    if (character == '<') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_COMMENT_LESS_THAN_SIGN);
    }
    RECONSUME_IN(TKZ_STATE_COMMENT);
END_STATE()

BEGIN_STATE(TKZ_STATE_COMMENT_LESS_THAN_SIGN_BANG)
    if (character == '-') {
        ADVANCE_TO(TKZ_STATE_COMMENT_LESS_THAN_SIGN_BANG_DASH);
    }
    RECONSUME_IN(TKZ_STATE_COMMENT);
END_STATE()

BEGIN_STATE(TKZ_STATE_COMMENT_LESS_THAN_SIGN_BANG_DASH)
    if (character == '-') {
        ADVANCE_TO(TKZ_STATE_COMMENT_LESS_THAN_SIGN_BANG_DASH_DASH);
    }
    RECONSUME_IN(TKZ_STATE_COMMENT_END_DASH);
END_STATE()

BEGIN_STATE(TKZ_STATE_COMMENT_LESS_THAN_SIGN_BANG_DASH_DASH)
    if (character == '>' || is_eof(character)) {
        RECONSUME_IN(TKZ_STATE_COMMENT_END);
    }
    SET_ERR(PCHVML_ERROR_NESTED_COMMENT);
    RECONSUME_IN(TKZ_STATE_COMMENT_END);
END_STATE()

BEGIN_STATE(TKZ_STATE_COMMENT_END_DASH)
    if (character == '-') {
        ADVANCE_TO(TKZ_STATE_COMMENT_END);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_COMMENT);
        RETURN_NEW_EOF_TOKEN();
    }
    APPEND_TO_TEMP_BUFFER('-');
    RECONSUME_IN(TKZ_STATE_COMMENT);
END_STATE()

BEGIN_STATE(TKZ_STATE_COMMENT_END)
    if (character == '>') {
        APPEND_TEMP_BUFFER_TO_TOKEN_TEXT();
        RETURN_AND_SWITCH_TO(TKZ_STATE_DATA);
    }
    if (character == '!') {
        ADVANCE_TO(TKZ_STATE_COMMENT_END_BANG);
    }
    if (character == '-') {
        APPEND_TO_TEMP_BUFFER('-');
        ADVANCE_TO(TKZ_STATE_COMMENT_END);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_COMMENT);
        RETURN_NEW_EOF_TOKEN();
    }
    APPEND_TO_TEMP_BUFFER('-');
    APPEND_TO_TEMP_BUFFER('-');
    RECONSUME_IN(TKZ_STATE_COMMENT);
END_STATE()

BEGIN_STATE(TKZ_STATE_COMMENT_END_BANG)
    if (character == '-') {
        APPEND_TO_TEMP_BUFFER('-');
        APPEND_TO_TEMP_BUFFER('-');
        APPEND_TO_TEMP_BUFFER('!');
        ADVANCE_TO(TKZ_STATE_COMMENT_END_DASH);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_INCORRECTLY_CLOSED_COMMENT);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_COMMENT);
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    APPEND_TO_TEMP_BUFFER('-');
    APPEND_TO_TEMP_BUFFER('-');
    APPEND_TO_TEMP_BUFFER('!');
    RECONSUME_IN(TKZ_STATE_COMMENT);
END_STATE()

BEGIN_STATE(TKZ_STATE_DOCTYPE)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_BEFORE_DOCTYPE_NAME);
    }
    if (character == '>') {
        RECONSUME_IN(TKZ_STATE_BEFORE_DOCTYPE_NAME);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        parser->token = pchvml_token_new_doctype();
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    SET_ERR(PCHVML_ERROR_MISSING_WHITESPACE_BEFORE_DOCTYPE_NAME);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_BEFORE_DOCTYPE_NAME)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_BEFORE_DOCTYPE_NAME);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_MISSING_DOCTYPE_NAME);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        parser->token = pchvml_token_new_doctype();
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    parser->token = pchvml_token_new_doctype();
    APPEND_TO_TOKEN_NAME(character);
    ADVANCE_TO(TKZ_STATE_DOCTYPE_NAME);
END_STATE()

BEGIN_STATE(TKZ_STATE_DOCTYPE_NAME)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_AFTER_DOCTYPE_NAME);
    }
    if (character == '>') {
        RETURN_AND_SWITCH_TO(TKZ_STATE_DATA);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    APPEND_TO_TOKEN_NAME(character);
    ADVANCE_TO(TKZ_STATE_DOCTYPE_NAME);
END_STATE()

BEGIN_STATE(TKZ_STATE_AFTER_DOCTYPE_NAME)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_AFTER_DOCTYPE_NAME);
    }
    if (character == '>') {
        RETURN_AND_SWITCH_TO(TKZ_STATE_DATA);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    if (parser->sbst == NULL) {
        parser->sbst = tkz_sbst_new_after_doctype_name_state();
    }
    bool ret = tkz_sbst_advance_ex(parser->sbst, character, true);
    if (!ret) {
        SET_ERR(PCHVML_ERROR_INVALID_CHARACTER_SEQUENCE_AFTER_DOCTYPE_NAME);
        tkz_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        RETURN_AND_STOP_PARSE();
    }

    const char* value = tkz_sbst_get_match(parser->sbst);
    if (value == NULL) {
        ADVANCE_TO(TKZ_STATE_AFTER_DOCTYPE_NAME);
    }

    if (strcmp(value, "PUBLIC") == 0) {
        tkz_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        ADVANCE_TO(TKZ_STATE_AFTER_DOCTYPE_PUBLIC_KEYWORD);
    }
    if (strcmp(value, "SYSTEM") == 0) {
        tkz_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        ADVANCE_TO(TKZ_STATE_AFTER_DOCTYPE_SYSTEM_KEYWORD);
    }
    SET_ERR(PCHVML_ERROR_INVALID_CHARACTER_SEQUENCE_AFTER_DOCTYPE_NAME);
    tkz_sbst_destroy(parser->sbst);
    parser->sbst = NULL;
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_AFTER_DOCTYPE_PUBLIC_KEYWORD)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_BEFORE_DOCTYPE_PUBLIC_ID);
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
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    SET_ERR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_ID);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_BEFORE_DOCTYPE_PUBLIC_ID)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_BEFORE_DOCTYPE_PUBLIC_ID);
    }
    if (character == '"') {
        RESET_TOKEN_PUBLIC_ID();
        ADVANCE_TO(TKZ_STATE_DOCTYPE_PUBLIC_ID_DOUBLE_QUOTED);
    }
    if (character == '\'') {
        RESET_TOKEN_PUBLIC_ID();
        ADVANCE_TO(TKZ_STATE_DOCTYPE_PUBLIC_ID_SINGLE_QUOTED);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_MISSING_DOCTYPE_PUBLIC_ID);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    SET_ERR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_ID);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_DOCTYPE_PUBLIC_ID_DOUBLE_QUOTED)
    if (character == '"') {
        ADVANCE_TO(TKZ_STATE_AFTER_DOCTYPE_PUBLIC_ID);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_ABRUPT_DOCTYPE_PUBLIC_ID);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    APPEND_TO_TOKEN_PUBLIC_ID(character);
    ADVANCE_TO(TKZ_STATE_DOCTYPE_PUBLIC_ID_DOUBLE_QUOTED);
END_STATE()

BEGIN_STATE(TKZ_STATE_DOCTYPE_PUBLIC_ID_SINGLE_QUOTED)
    if (character == '\'') {
        ADVANCE_TO(TKZ_STATE_AFTER_DOCTYPE_PUBLIC_ID);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_ABRUPT_DOCTYPE_PUBLIC_ID);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    APPEND_TO_TOKEN_PUBLIC_ID(character);
    ADVANCE_TO(TKZ_STATE_DOCTYPE_PUBLIC_ID_SINGLE_QUOTED);
END_STATE()

BEGIN_STATE(TKZ_STATE_AFTER_DOCTYPE_PUBLIC_ID)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_BETWEEN_DOCTYPE_PUBLIC_ID_AND_SYSTEM_INFO);
    }
    if (character == '>') {
        RETURN_AND_SWITCH_TO(TKZ_STATE_DATA);
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
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    SET_ERR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_BETWEEN_DOCTYPE_PUBLIC_ID_AND_SYSTEM_INFO)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_BETWEEN_DOCTYPE_PUBLIC_ID_AND_SYSTEM_INFO);
    }
    if (character == '>') {
        RETURN_AND_SWITCH_TO(TKZ_STATE_DATA);
    }
    if (character == '"') {
        RESET_TOKEN_SYSTEM_INFO();
        ADVANCE_TO(TKZ_STATE_DOCTYPE_SYSTEM_DOUBLE_QUOTED);
    }
    if (character == '\'') {
        RESET_TOKEN_SYSTEM_INFO();
        ADVANCE_TO(TKZ_STATE_DOCTYPE_SYSTEM_SINGLE_QUOTED);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    RECONSUME_IN(TKZ_STATE_AFTER_DOCTYPE_NAME);
    //SET_ERR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM);
    //RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_AFTER_DOCTYPE_SYSTEM_KEYWORD)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_BEFORE_DOCTYPE_SYSTEM);
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
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    SET_ERR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_BEFORE_DOCTYPE_SYSTEM)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_BEFORE_DOCTYPE_SYSTEM);
    }
    if (character == '"') {
        RESET_TOKEN_SYSTEM_INFO();
        ADVANCE_TO(TKZ_STATE_DOCTYPE_SYSTEM_DOUBLE_QUOTED);
    }
    if (character == '\'') {
        RESET_TOKEN_SYSTEM_INFO();
        ADVANCE_TO(TKZ_STATE_DOCTYPE_SYSTEM_SINGLE_QUOTED);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_MISSING_DOCTYPE_SYSTEM);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    SET_ERR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_DOCTYPE_SYSTEM_DOUBLE_QUOTED)
    if (character == '"') {
        ADVANCE_TO(TKZ_STATE_AFTER_DOCTYPE_SYSTEM);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_ABRUPT_DOCTYPE_SYSTEM);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    APPEND_TO_TOKEN_SYSTEM_INFO(character);
    ADVANCE_TO(TKZ_STATE_DOCTYPE_SYSTEM_DOUBLE_QUOTED);
END_STATE()

BEGIN_STATE(TKZ_STATE_DOCTYPE_SYSTEM_SINGLE_QUOTED)
    if (character == '\'') {
        ADVANCE_TO(TKZ_STATE_AFTER_DOCTYPE_SYSTEM);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_ABRUPT_DOCTYPE_SYSTEM);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    APPEND_TO_TOKEN_SYSTEM_INFO(character);
    ADVANCE_TO(TKZ_STATE_DOCTYPE_SYSTEM_SINGLE_QUOTED);
END_STATE()

BEGIN_STATE(TKZ_STATE_AFTER_DOCTYPE_SYSTEM)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_AFTER_DOCTYPE_SYSTEM);
    }
    if (character == '>') {
        RETURN_AND_SWITCH_TO(TKZ_STATE_DATA);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_DOCTYPE);
        pchvml_token_set_force_quirks(parser->token, true);
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER_AFTER_DOCTYPE_SYSTEM);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_BOGUS_DOCTYPE)
    if (character == '>') {
        RETURN_AND_SWITCH_TO(TKZ_STATE_DATA);
    }
    if (is_eof(character)) {
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    ADVANCE_TO(TKZ_STATE_BOGUS_DOCTYPE);
END_STATE()

BEGIN_STATE(TKZ_STATE_CDATA_SECTION)
    if (character == ']') {
        ADVANCE_TO(TKZ_STATE_CDATA_SECTION_BRACKET);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_CDATA);
        RECONSUME_IN(TKZ_STATE_DATA);
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(TKZ_STATE_CDATA_SECTION);
END_STATE()

BEGIN_STATE(TKZ_STATE_CDATA_SECTION_BRACKET)
    if (character == ']') {
        ADVANCE_TO(TKZ_STATE_CDATA_SECTION_END);
    }
    APPEND_TO_TEMP_BUFFER(']');
    RECONSUME_IN(TKZ_STATE_CDATA_SECTION);
END_STATE()

BEGIN_STATE(TKZ_STATE_CDATA_SECTION_END)
    if (character == ']') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_CDATA_SECTION_END);
    }
    if (character == '>') {
        struct pcvcm_node* node = TEMP_BUFFER_TO_VCM_NODE();
        if (!node) {
            RETURN_AND_STOP_PARSE();
        }
        RESET_TEMP_BUFFER();
        parser->token = pchvml_token_new_vcm(node);
        RETURN_AND_SWITCH_TO(TKZ_STATE_DATA);
    }
    APPEND_TO_TEMP_BUFFER(']');
    APPEND_TO_TEMP_BUFFER(']');
    RECONSUME_IN(TKZ_STATE_CDATA_SECTION);
END_STATE()

BEGIN_STATE(TKZ_STATE_CHARACTER_REFERENCE)
    RESET_STRING_BUFFER();
    APPEND_TO_STRING_BUFFER('&');
    if (is_ascii_alpha_numeric(character)) {
        RECONSUME_IN(TKZ_STATE_NAMED_CHARACTER_REFERENCE);
    }
    if (character == '#') {
        APPEND_TO_STRING_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_NUMERIC_CHARACTER_REFERENCE);
    }
    APPEND_BUFFER_TO_TEMP_BUFFER(parser->string_buffer);
    RESET_STRING_BUFFER();
    RECONSUME_IN(parser->return_state);
END_STATE()

BEGIN_STATE(TKZ_STATE_NAMED_CHARACTER_REFERENCE)
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
        ADVANCE_TO(TKZ_STATE_AMBIGUOUS_AMPERSAND);
    }

    const char* value = tkz_sbst_get_match(parser->sbst);
    if (value == NULL) {
        ADVANCE_TO(TKZ_STATE_NAMED_CHARACTER_REFERENCE);
    }
    if (character != ';') {
        ADVANCE_TO(TKZ_STATE_NAMED_CHARACTER_REFERENCE);
    }
    APPEND_BYTES_TO_TEMP_BUFFER(value, strlen(value));
    RESET_STRING_BUFFER();

    tkz_sbst_destroy(parser->sbst);
    parser->sbst = NULL;
    ADVANCE_TO(parser->return_state);
END_STATE()

BEGIN_STATE(TKZ_STATE_AMBIGUOUS_AMPERSAND)
    if (is_ascii_alpha_numeric(character)) {
        if (pchvml_parser_is_in_attribute(parser)) {
            APPEND_TO_TOKEN_ATTR_VALUE(character);
            ADVANCE_TO(TKZ_STATE_AMBIGUOUS_AMPERSAND);
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

BEGIN_STATE(TKZ_STATE_NUMERIC_CHARACTER_REFERENCE)
    parser->char_ref_code = 0;
    if (character == 'x' || character == 'X') {
        APPEND_TO_STRING_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_HEXADECIMAL_CHARACTER_REFERENCE_START);
    }
    RECONSUME_IN(TKZ_STATE_DECIMAL_CHARACTER_REFERENCE_START);
END_STATE()

BEGIN_STATE(TKZ_STATE_HEXADECIMAL_CHARACTER_REFERENCE_START)
    if (is_ascii_hex_digit(character)) {
        RECONSUME_IN(TKZ_STATE_HEXADECIMAL_CHARACTER_REFERENCE);
    }
    SET_ERR(
        PCHVML_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_DECIMAL_CHARACTER_REFERENCE_START)
    if (is_ascii_digit(character)) {
        RECONSUME_IN(TKZ_STATE_DECIMAL_CHARACTER_REFERENCE);
    }
    SET_ERR(
        PCHVML_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_HEXADECIMAL_CHARACTER_REFERENCE)
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
        ADVANCE_TO(TKZ_STATE_NUMERIC_CHARACTER_REFERENCE_END);
    }
    SET_ERR(PCHVML_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_DECIMAL_CHARACTER_REFERENCE)
    if (is_ascii_digit(character)) {
        parser->char_ref_code *= 10;
        parser->char_ref_code += character - 0x30;
        ADVANCE_TO(TKZ_STATE_DECIMAL_CHARACTER_REFERENCE);
    }
    if (character == ';') {
        ADVANCE_TO(TKZ_STATE_NUMERIC_CHARACTER_REFERENCE_END);
    }
    SET_ERR(PCHVML_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_NUMERIC_CHARACTER_REFERENCE_END)
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

BEGIN_STATE(TKZ_STATE_SPECIAL_ATTRIBUTE_OPERATOR_IN_ATTRIBUTE_NAME)
    if (character == '=') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            pchvml_token_set_assignment_to_attr(
                    parser->token,
                    PCHVML_ATTRIBUTE_OPERATOR);
        }
        else {
            uint32_t op = tkz_buffer_get_last_char(
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
                            PCHVML_ATTRIBUTE_PRECISE_OPERATOR);
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
        ADVANCE_TO(TKZ_STATE_BEFORE_ATTRIBUTE_VALUE);
    }
    if (character == '>'
        &&  tkz_buffer_equal_to(parser->temp_buffer, "/", 1)) {
        END_TOKEN_ATTR();
        RECONSUME_IN(TKZ_STATE_SELF_CLOSING_START_TAG);
    }
    APPEND_TEMP_BUFFER_TO_TOKEN_ATTR_NAME();
    RECONSUME_IN(TKZ_STATE_ATTRIBUTE_NAME);
END_STATE()

BEGIN_STATE(TKZ_STATE_SPECIAL_ATTRIBUTE_OPERATOR_AFTER_ATTRIBUTE_NAME)
    if (character == '=') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            pchvml_token_set_assignment_to_attr(
                    parser->token,
                    PCHVML_ATTRIBUTE_OPERATOR);
        }
        else {
            uint32_t op = tkz_buffer_get_last_char(
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
                            PCHVML_ATTRIBUTE_PRECISE_OPERATOR);
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
        ADVANCE_TO(TKZ_STATE_BEFORE_ATTRIBUTE_VALUE);
    }
    if (tkz_buffer_equal_to(parser->temp_buffer, "$", 1)) {
        tkz_reader_reconsume_last_char(parser->reader);
        tkz_reader_reconsume_last_char(parser->reader);
        ADVANCE_TO(TKZ_STATE_BEFORE_ATTRIBUTE_VALUE);
    }
    if (character == '>'
        &&  tkz_buffer_equal_to(parser->temp_buffer, "/", 1)) {
        END_TOKEN_ATTR();
        RECONSUME_IN(TKZ_STATE_SELF_CLOSING_START_TAG);
    }
    BEGIN_TOKEN_ATTR();
    APPEND_TEMP_BUFFER_TO_TOKEN_ATTR_NAME();
    RECONSUME_IN(TKZ_STATE_ATTRIBUTE_NAME);
END_STATE()

BEGIN_STATE(TKZ_STATE_TEXT_CONTENT)
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_CDATA);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '<') {
        if (!IS_TEMP_BUFFER_EMPTY()) {
            size_t nr_chars = tkz_buffer_get_size_in_chars(parser->temp_buffer);
            if (parser->nr_whitespace > 0
                    && nr_chars > parser->nr_whitespace) {
                size_t nr_bytes = tkz_buffer_get_size_in_bytes(
                        parser->temp_buffer);
                const char* bytes = tkz_buffer_get_bytes(
                        parser->temp_buffer);
                RESET_STRING_BUFFER();
                APPEND_BYTES_TO_STRING_BUFFER(bytes, nr_bytes - parser->nr_whitespace);
                struct pcvcm_node* node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->string_buffer)
                    );
                if (!node) {
                    RETURN_AND_STOP_PARSE();
                }
                struct pchvml_token* token = pchvml_token_new_vcm(node);

                RESET_STRING_BUFFER();
                const char* pos = bytes + (nr_bytes - parser->nr_whitespace);
                APPEND_BYTES_TO_STRING_BUFFER(pos, parser->nr_whitespace);
                node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->string_buffer)
                    );
                if (!node) {
                    RETURN_AND_STOP_PARSE();
                }
                struct pchvml_token* next_token = pchvml_token_new_vcm(node);
                pchvml_token_set_is_whitespace(next_token, true);
                parser->nr_whitespace = 0;
                tkz_reader_reconsume_last_char(parser->reader);
                RETURN_MULTIPLE_AND_SWITCH_TO(
                        token, next_token, TKZ_STATE_DATA);
            }
            else {
                struct pcvcm_node* node = TEMP_BUFFER_TO_VCM_NODE();
                if (!node) {
                    RETURN_AND_STOP_PARSE();
                }
                RESET_TEMP_BUFFER();
                parser->token = pchvml_token_new_vcm(node);
                parser->nr_whitespace = 0;
                RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
            }
        }
        parser->nr_whitespace = 0;
        RECONSUME_IN(TKZ_STATE_DATA);
    }
    if (character == '&') {
        parser->nr_whitespace = 0;
        SET_RETURN_STATE(TKZ_STATE_TEXT_CONTENT);
        ADVANCE_TO(TKZ_STATE_CHARACTER_REFERENCE);
    }
    if (!parser->tag_has_raw_attr) {
        if (character == '$') {
            parser->nr_whitespace = 0;
            if (!IS_TEMP_BUFFER_EMPTY()) {
                if (tkz_buffer_equal_to(parser->temp_buffer, "{", 1)) {
                    tkz_reader_reconsume_last_char(parser->reader);
                    tkz_reader_reconsume_last_char(parser->reader);
                    RESET_VCM_NODE();
                    SET_TRANSIT_STATE(TKZ_STATE_TEXT_CONTENT);
                    ADVANCE_TO(TKZ_STATE_EJSON_DATA);
                }
                if (tkz_buffer_equal_to(parser->temp_buffer, "{{", 2)) {
                    tkz_reader_reconsume_last_char(parser->reader);
                    tkz_reader_reconsume_last_char(parser->reader);
                    tkz_reader_reconsume_last_char(parser->reader);
                    RESET_VCM_NODE();
                    SET_TRANSIT_STATE(TKZ_STATE_TEXT_CONTENT);
                    ADVANCE_TO(TKZ_STATE_EJSON_DATA);
                }
                struct pcvcm_node* node = TEMP_BUFFER_TO_VCM_NODE();
                if (!node) {
                    RETURN_AND_STOP_PARSE();
                }
                RESET_TEMP_BUFFER();
                parser->token = pchvml_token_new_vcm(node);
                RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
            }
            RESET_VCM_NODE();
            SET_TRANSIT_STATE(TKZ_STATE_TEXT_CONTENT);
            RECONSUME_IN(TKZ_STATE_EJSON_DATA);
        }
        if (character == '{') {
            parser->nr_whitespace = 0;
            if (!IS_TEMP_BUFFER_EMPTY() &&
                    !tkz_buffer_end_with(parser->temp_buffer, "{", 1)) {
                struct pcvcm_node* node = TEMP_BUFFER_TO_VCM_NODE();
                if (!node) {
                    RETURN_AND_STOP_PARSE();
                }
                RESET_TEMP_BUFFER();
                parser->token = pchvml_token_new_vcm(node);
                RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
            }
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_TEXT_CONTENT);
        }
        if (is_whitespace(character)) {
            if (tkz_buffer_equal_to(parser->temp_buffer, "{{", 2)) {
                ejson_stack_push('C');
                struct pcvcm_node *node = pcvcm_node_new_cjsonee();
                UPDATE_VCM_NODE(node);
                ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
            }
        }
    }
    if (is_whitespace(character)) {
        parser->nr_whitespace++;
    }
    else {
        parser->nr_whitespace = 0;
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(TKZ_STATE_TEXT_CONTENT);
END_STATE()

BEGIN_STATE(TKZ_STATE_JSONTEXT_CONTENT)
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_CDATA);
        RETURN_AND_STOP_PARSE();
    }
    SET_TRANSIT_STATE(TKZ_STATE_TEXT_CONTENT);
    RECONSUME_IN(TKZ_STATE_EJSON_DATA);
END_STATE()

BEGIN_STATE(TKZ_STATE_JSONEE_ATTRIBUTE_VALUE_DOUBLE_QUOTED)
    if (tkz_buffer_end_with(parser->temp_buffer, "\\", 1)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_JSONEE_ATTRIBUTE_VALUE_DOUBLE_QUOTED);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_CDATA);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '"') {
        if (!IS_TEMP_BUFFER_EMPTY()) {
            APPEND_BUFFER_TO_TOKEN_ATTR_VALUE(parser->temp_buffer);
            RESET_TEMP_BUFFER();
        }
        else {
            APPEND_BYTES_TO_TOKEN_ATTR_VALUE(NULL, 0);
        }
        END_TOKEN_ATTR();
        ADVANCE_TO(TKZ_STATE_AFTER_ATTRIBUTE_VALUE);
    }
    if (character == '&') {
        SET_RETURN_STATE(TKZ_STATE_JSONEE_ATTRIBUTE_VALUE_DOUBLE_QUOTED);
        ADVANCE_TO(TKZ_STATE_CHARACTER_REFERENCE);
    }
    if (character == '[' &&
            tkz_buffer_equal_to(parser->temp_buffer, "~", 1)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_JSONEE_ATTRIBUTE_VALUE_DOUBLE_QUOTED);
    }
    if (character == '$') {
        bool handle = pchvml_parser_is_handle_as_jsonee(parser->token,
                character);
        bool buffer_is_white = tkz_buffer_is_whitespace(
                parser->temp_buffer);
        if (handle && buffer_is_white) {
            ejson_stack_push('"');
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_DATA);
        }

        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('"');

        if (IS_TEMP_BUFFER_EMPTY()) {
            RECONSUME_IN(TKZ_STATE_EJSON_DATA);
        }
        if (tkz_buffer_end_with(parser->temp_buffer, "{", 1)) {
            tkz_reader_reconsume_last_char(parser->reader);
            tkz_reader_reconsume_last_char(parser->reader);
            tkz_buffer_delete_tail_chars(parser->temp_buffer, 1);
        }
        else {
            tkz_reader_reconsume_last_char(parser->reader);
        }
        struct pcvcm_node* node = TEMP_BUFFER_TO_VCM_NODE();
        APPEND_AS_VCM_CHILD(node);
        RESET_TEMP_BUFFER();
        ADVANCE_TO(TKZ_STATE_EJSON_DATA);
    }
    if (character == '{' || character == '[') {
        bool handle = pchvml_parser_is_handle_as_jsonee(parser->token,
                character);
        if (handle) {
            bool buffer_is_white = tkz_buffer_is_whitespace(
                parser->temp_buffer);
            if (buffer_is_white) {
                ejson_stack_push('"');
                RESET_TEMP_BUFFER();
                RECONSUME_IN(TKZ_STATE_EJSON_DATA);
            }
            if (parser->vcm_node) {
                vcm_stack_push(parser->vcm_node);
            }
            struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                    NULL);
            UPDATE_VCM_NODE(snode);
            ejson_stack_push('"');
            if (!IS_TEMP_BUFFER_EMPTY()) {
                struct pcvcm_node* node = TEMP_BUFFER_TO_VCM_NODE();
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
            }
            RECONSUME_IN(TKZ_STATE_EJSON_DATA);
        }
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(TKZ_STATE_JSONEE_ATTRIBUTE_VALUE_DOUBLE_QUOTED);
END_STATE()

BEGIN_STATE(TKZ_STATE_JSONEE_ATTRIBUTE_VALUE_SINGLE_QUOTED)
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '\'') {
        END_TOKEN_ATTR();
        ADVANCE_TO(TKZ_STATE_AFTER_ATTRIBUTE_VALUE);
    }
    if (character == '&') {
        SET_RETURN_STATE(TKZ_STATE_JSONEE_ATTRIBUTE_VALUE_SINGLE_QUOTED);
        ADVANCE_TO(TKZ_STATE_CHARACTER_REFERENCE);
    }
    APPEND_TO_TOKEN_ATTR_VALUE(character);
    ADVANCE_TO(TKZ_STATE_JSONEE_ATTRIBUTE_VALUE_SINGLE_QUOTED);
END_STATE()

BEGIN_STATE(TKZ_STATE_JSONEE_ATTRIBUTE_VALUE_UNQUOTED)
    if (is_whitespace(character)) {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {

            if (tkz_buffer_is_int(parser->temp_buffer)) {
                int64_t i64 = strtoll (
                    tkz_buffer_get_bytes(parser->temp_buffer),
                    NULL, 10);
                struct pcvcm_node* node = pcvcm_node_new_longint(i64);
                pchvml_token_append_vcm_to_attr(parser->token, node);
            }
            else if (tkz_buffer_is_number(parser->temp_buffer)) {
                double d = strtod(
                    tkz_buffer_get_bytes(parser->temp_buffer), NULL);
                struct pcvcm_node* node = pcvcm_node_new_number(d);
                pchvml_token_append_vcm_to_attr(parser->token, node);
            }
            else {
                if (tkz_buffer_equal_to(parser->temp_buffer, "true", 4)) {
                    struct pcvcm_node* node = pcvcm_node_new_boolean(true);
                    pchvml_token_append_vcm_to_attr(parser->token, node);
                }
                else if (tkz_buffer_equal_to(parser->temp_buffer, "false",
                            5)) {
                    struct pcvcm_node* node = pcvcm_node_new_boolean(false);
                    pchvml_token_append_vcm_to_attr(parser->token, node);
                }
                else if (tkz_buffer_equal_to(parser->temp_buffer, "null", 4)) {
                    struct pcvcm_node* node = pcvcm_node_new_null();
                    pchvml_token_append_vcm_to_attr(parser->token, node);
                }
                else if (tkz_buffer_equal_to(parser->temp_buffer, "undefined", 9)) {
                    struct pcvcm_node* node = pcvcm_node_new_undefined();
                    pchvml_token_append_vcm_to_attr(parser->token, node);
                }
                else {
                    APPEND_BUFFER_TO_TOKEN_ATTR_VALUE(parser->temp_buffer);
                }
            }
            RESET_TEMP_BUFFER();

        }
        END_TOKEN_ATTR();
        ADVANCE_TO(TKZ_STATE_BEFORE_ATTRIBUTE_NAME);
    }
    if (character == '&') {
        SET_RETURN_STATE(TKZ_STATE_JSONEE_ATTRIBUTE_VALUE_UNQUOTED);
        ADVANCE_TO(TKZ_STATE_CHARACTER_REFERENCE);
    }
    if (character == '/') {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {

            if (tkz_buffer_is_int(parser->temp_buffer)) {
                int64_t i64 = strtoll (
                    tkz_buffer_get_bytes(parser->temp_buffer),
                    NULL, 10);
                struct pcvcm_node* node = pcvcm_node_new_longint(i64);
                pchvml_token_append_vcm_to_attr(parser->token, node);
            }
            else if (tkz_buffer_is_number(parser->temp_buffer)) {
                double d = strtod(
                    tkz_buffer_get_bytes(parser->temp_buffer), NULL);
                struct pcvcm_node* node = pcvcm_node_new_number(d);
                pchvml_token_append_vcm_to_attr(parser->token, node);
            }
            else {
                if (tkz_buffer_equal_to(parser->temp_buffer, "true", 4)) {
                    struct pcvcm_node* node = pcvcm_node_new_boolean(true);
                    pchvml_token_append_vcm_to_attr(parser->token, node);
                }
                else if (tkz_buffer_equal_to(parser->temp_buffer, "false",
                            5)) {
                    struct pcvcm_node* node = pcvcm_node_new_boolean(false);
                    pchvml_token_append_vcm_to_attr(parser->token, node);
                }
                else if (tkz_buffer_equal_to(parser->temp_buffer, "null", 4)) {
                    struct pcvcm_node* node = pcvcm_node_new_null();
                    pchvml_token_append_vcm_to_attr(parser->token, node);
                }
                else if (tkz_buffer_equal_to(parser->temp_buffer, "undefined", 9)) {
                    struct pcvcm_node* node = pcvcm_node_new_undefined();
                    pchvml_token_append_vcm_to_attr(parser->token, node);
                }
                else {
                    APPEND_BUFFER_TO_TOKEN_ATTR_VALUE(parser->temp_buffer);
                }
            }
            RESET_TEMP_BUFFER();

        }
        END_TOKEN_ATTR();
        RECONSUME_IN(TKZ_STATE_BEFORE_ATTRIBUTE_NAME);
    }
    if (character == '>') {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {

            if (tkz_buffer_is_int(parser->temp_buffer)) {
                int64_t i64 = strtoll (
                    tkz_buffer_get_bytes(parser->temp_buffer),
                    NULL, 10);
                struct pcvcm_node* node = pcvcm_node_new_longint(i64);
                pchvml_token_append_vcm_to_attr(parser->token, node);
            }
            else if (tkz_buffer_is_number(parser->temp_buffer)) {
                double d = strtod(
                    tkz_buffer_get_bytes(parser->temp_buffer), NULL);
                struct pcvcm_node* node = pcvcm_node_new_number(d);
                pchvml_token_append_vcm_to_attr(parser->token, node);
            }
            else {
                if (tkz_buffer_equal_to(parser->temp_buffer, "true", 4)) {
                    struct pcvcm_node* node = pcvcm_node_new_boolean(true);
                    pchvml_token_append_vcm_to_attr(parser->token, node);
                }
                else if (tkz_buffer_equal_to(parser->temp_buffer, "false",
                            5)) {
                    struct pcvcm_node* node = pcvcm_node_new_boolean(false);
                    pchvml_token_append_vcm_to_attr(parser->token, node);
                }
                else if (tkz_buffer_equal_to(parser->temp_buffer, "null", 4)) {
                    struct pcvcm_node* node = pcvcm_node_new_null();
                    pchvml_token_append_vcm_to_attr(parser->token, node);
                }
                else if (tkz_buffer_equal_to(parser->temp_buffer, "undefined", 9)) {
                    struct pcvcm_node* node = pcvcm_node_new_undefined();
                    pchvml_token_append_vcm_to_attr(parser->token, node);
                }
                else {
                    APPEND_BUFFER_TO_TOKEN_ATTR_VALUE(parser->temp_buffer);
                }
            }
            RESET_TEMP_BUFFER();
        }
        END_TOKEN_ATTR();
        RETURN_AND_SWITCH_TO(TKZ_STATE_DATA);
    }
    if (is_eof(character)) {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            APPEND_BUFFER_TO_TOKEN_ATTR_VALUE(parser->temp_buffer);
            RESET_TEMP_BUFFER();
        }
        END_TOKEN_ATTR();
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RECONSUME_IN(TKZ_STATE_DATA);
    }
    if (character == '$' || character == '{' || character == '[') {
        bool handle = pchvml_parser_is_handle_as_jsonee(parser->token,
                character);
        bool buffer_is_white = tkz_buffer_is_whitespace(
                parser->temp_buffer);
        if (handle && buffer_is_white) {
            ejson_stack_push('U');
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_DATA);
        }

        ejson_stack_push('U');
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            if (parser->vcm_node) {
                vcm_stack_push(parser->vcm_node);
            }
            struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                    NULL);
            UPDATE_VCM_NODE(snode);
            struct pcvcm_node* node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_DATA);
    }
    if (character == '"' || character == '\'' || character == '<'
            || character == '=' || character == '`') {
        SET_ERR(
            PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_UNQUOTED_ATTRIBUTE_VALUE);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(TKZ_STATE_JSONEE_ATTRIBUTE_VALUE_UNQUOTED);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_DATA)
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
        RECONSUME_IN(TKZ_STATE_EJSON_TEMPLATE_DATA);
    }
    if (is_whitespace (character) || character == 0xFEFF) {
        ADVANCE_TO(TKZ_STATE_EJSON_DATA);
    }
    RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_FINISHED)
    while (!vcm_stack_is_empty()) {
        ejson_stack_pop();
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
    }
    ejson_stack_reset();
    if (parser->token == NULL ||
        parser->transit_state == TKZ_STATE_TEXT_CONTENT ||
        parser->transit_state == TKZ_STATE_JSONTEXT_CONTENT) {
        parser->token = pchvml_token_new_vcm(parser->vcm_node);
        parser->vcm_node = NULL;
        RESET_TRANSIT_STATE();
        RESET_VCM_NODE();
        RETURN_AND_RECONSUME_IN(TKZ_STATE_DATA);
    }
    pchvml_token_append_vcm_to_attr(parser->token, parser->vcm_node);
    END_TOKEN_ATTR();
    RESET_TRANSIT_STATE();
    RESET_VCM_NODE();
    RECONSUME_IN(TKZ_STATE_AFTER_ATTRIBUTE_VALUE);
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
        if (uc == 'T') {
            if (parser->vcm_node->type !=
                    PCVCM_NODE_TYPE_FUNC_CONCAT_STRING) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            RECONSUME_IN(TKZ_STATE_EJSON_TEMPLATE_DATA);
        }
        ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
    }
    if (character == '{') {
        RECONSUME_IN(TKZ_STATE_EJSON_LEFT_BRACE);
    }
    if (character == '}') {
        if (parser->vcm_node && (parser->vcm_node->type ==
                    PCVCM_NODE_TYPE_FUNC_CONCAT_STRING)
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
#if 0
        if (parser->vcm_node->type ==
                PCVCM_NODE_TYPE_FUNC_CONCAT_STRING
                && (uc == '"' || uc == '\'' || uc == 'U')) {
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_JSONEE_STRING);
        }
#endif
        RECONSUME_IN(TKZ_STATE_EJSON_RIGHT_BRACKET);
    }
    if (character == '<' || character == '>') {
        if (pchvml_parser_is_in_template(parser)) {
            if (parser->vcm_node->type !=
                    PCVCM_NODE_TYPE_FUNC_CONCAT_STRING) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            RECONSUME_IN(TKZ_STATE_EJSON_TEMPLATE_DATA);
        }
        RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
    }
    if (character == '/') {
        if (uc == 'U') {
            ejson_stack_pop();
        }
        if (ejson_stack_is_empty() && parser->vcm_node) {
            RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
        }
        else if (uc != '"') {
            SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
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
        RECONSUME_IN(TKZ_STATE_EJSON_RIGHT_PARENTHESIS);
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
        else if (uc == 'T') {
            if (parser->vcm_node->type !=
                    PCVCM_NODE_TYPE_FUNC_CONCAT_STRING) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            RECONSUME_IN(TKZ_STATE_EJSON_TEMPLATE_DATA);
        }
        else {
            RESET_TEMP_BUFFER();
            RESET_QUOTED_COUNTER();
            RECONSUME_IN(TKZ_STATE_EJSON_VALUE_DOUBLE_QUOTED);
        }
    }
    if (character == '\'') {
        if (uc == 'T') {
            if (parser->vcm_node->type !=
                    PCVCM_NODE_TYPE_FUNC_CONCAT_STRING) {
                POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            }
            RECONSUME_IN(TKZ_STATE_EJSON_TEMPLATE_DATA);
        }
        RESET_TEMP_BUFFER();
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
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
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
                struct pcvcm_node* node = pcvcm_node_new_string(
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
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '.') {
        RECONSUME_IN(TKZ_STATE_EJSON_JSONEE_FULL_STOP_SIGN);
    }
    if (uc == '[') {
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
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
    if (parser->vcm_node && (parser->vcm_node->type ==
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
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '$') {
        if (pchvml_parser_is_in_raw_template(parser)) {
            uint32_t uc = ejson_stack_top();
            if (uc == '"' || uc == 'U') {
                RESET_TEMP_BUFFER();
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_STRING);
            }
            else {
                if (parser->vcm_node) {
                    vcm_stack_push(parser->vcm_node);
                }
                struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                        NULL);
                UPDATE_VCM_NODE(snode);
                ejson_stack_push('U');
                RESET_TEMP_BUFFER();
                APPEND_TO_TEMP_BUFFER(character);
                ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_STRING);
            }
        }
        else {
            if (parser->vcm_node) {
                size_t nr_children = pcvcm_node_children_count(parser->vcm_node);
                if ((parser->vcm_node->type ==
                        PCVCM_NODE_TYPE_FUNC_GET_VARIABLE) && nr_children == 1) {
                    pcvcm_node_set_closed(parser->vcm_node, true);
                    POP_AS_VCM_PARENT_AND_UPDATE_VCM();
                }
                else if ((parser->vcm_node->type ==
                        PCVCM_NODE_TYPE_FUNC_GET_ELEMENT) && nr_children == 2) {
                    pcvcm_node_set_closed(parser->vcm_node, true);
                    POP_AS_VCM_PARENT_AND_UPDATE_VCM();
                }

                if (ejson_stack_is_empty()) {
                    RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
                }
                else {
                    vcm_stack_push(parser->vcm_node);
                }
            }
            ejson_stack_push('$');
            struct pcvcm_node* snode = pcvcm_node_new_get_variable(NULL);
            UPDATE_VCM_NODE(snode);
            ADVANCE_TO(TKZ_STATE_EJSON_DOLLAR);
        }
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
                ejson_stack_push('{');
                if (parser->vcm_node) {
                    vcm_stack_push(parser->vcm_node);
                }
                struct pcvcm_node *node = pcvcm_node_new_object(0, NULL);
                UPDATE_VCM_NODE(node);
                RECONSUME_IN(TKZ_STATE_EJSON_BEFORE_NAME);
            }
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (uc == 'P') {
        ejson_stack_pop();
        ejson_stack_push('{');
        if (parser->vcm_node) {
            vcm_stack_push(parser->vcm_node);
        }
        struct pcvcm_node* node = pcvcm_node_new_object(0, NULL);
        UPDATE_VCM_NODE(node);
        RECONSUME_IN(TKZ_STATE_EJSON_BEFORE_NAME);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_RIGHT_BRACE)
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
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
        SET_ERR(PCHVML_ERROR_UNEXPECTED_RIGHT_BRACE);
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
    if (character == '/') {
        uint32_t uc = ejson_stack_top();
        while (uc == '$') {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            uc = ejson_stack_top();
        }
        if (ejson_stack_is_empty()) {
            RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
        }
        else if (uc == 'U') {
            ejson_stack_pop();
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
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
            struct pcvcm_node* node = pcvcm_node_new_object(0, NULL);
            APPEND_CHILD(node, parser->vcm_node);
            vcm_stack_push(node);
            RESET_VCM_NODE();
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
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
            struct pcvcm_node* node = pcvcm_node_new_get_element(NULL,
                    NULL);
            APPEND_CHILD(node, parser->vcm_node);
            UPDATE_VCM_NODE(node);
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
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
                || uc == '"' || uc == 'U') {
            ejson_stack_push('[');
            if (parser->vcm_node) {
                vcm_stack_push(parser->vcm_node);
            }
            struct pcvcm_node* node = pcvcm_node_new_array(0, NULL);
            UPDATE_VCM_NODE(node);
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
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

BEGIN_STATE(TKZ_STATE_EJSON_RIGHT_BRACKET)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_EJSON_RIGHT_BRACKET);
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
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            struct pcvcm_node* parent = (struct pcvcm_node*)
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
        SET_ERR(PCHVML_ERROR_UNEXPECTED_RIGHT_BRACKET);
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
            ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
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
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    if (ejson_stack_is_empty()) {
        RECONSUME_IN(TKZ_STATE_EJSON_FINISHED);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_RIGHT_PARENTHESIS)
#if 0
    uint32_t uc = ejson_stack_top();
    if (character == '.' || character == '[') {
        if (uc == '(' || uc == '<') {
            ejson_stack_pop();
#if 0
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
            SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
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
            SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
#else
    uint32_t uc = ejson_stack_top();
    if (uc == '(' || uc == '<') {
        ejson_stack_pop();

// #define PRINT_DEBUG
#ifdef PRINT_DEBUG             /* { */
        PRINT_VCM_NODE(parser->vcm_node);
#endif                         /* } */
        if (parser->vcm_node->type == PCVCM_NODE_TYPE_FUNC_CALL_GETTER
                || parser->vcm_node->type == PCVCM_NODE_TYPE_FUNC_CALL_SETTER) {
            if (!pcvcm_node_is_closed(parser->vcm_node)) {
                pcvcm_node_set_closed(parser->vcm_node, true);
#ifdef PRINT_DEBUG             /* { */
                PRINT_VCM_NODE(parser->vcm_node);
#endif                         /* } */
                ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
            }
        }

        if (!vcm_stack_is_empty()) {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            pcvcm_node_set_closed(parser->vcm_node, true);
#ifdef PRINT_DEBUG             /* { */
            PRINT_VCM_NODE(parser->vcm_node);
#endif                         /* } */
        }
        ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
#undef PRINT_DEBUG
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
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '"' || character == '\'') {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
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
        RECONSUME_IN(TKZ_STATE_EJSON_RIGHT_PARENTHESIS);
    }
    if (character == ',') {
        if (uc == '{') {
            ejson_stack_pop();
            ADVANCE_TO(TKZ_STATE_EJSON_BEFORE_NAME);
        }
        if (uc == '[') {
            if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                struct pcvcm_node* node = pcvcm_node_new_string(
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
                struct pcvcm_node* node = pcvcm_node_new_string(
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
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '/' || character == '<' || character == '.') {
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    if (character == ';' || character == '|' || character == '&') {
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    if (uc == '"' || uc  == 'U') {
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
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
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_AFTER_NAME)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_EJSON_AFTER_NAME);
    }
    if (character == ':') {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer));
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
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
        struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('U');
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
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
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
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
            struct pcvcm_node* node = pcvcm_node_new_string ("");
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
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
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
        parser->nr_single_quoted++;
        size_t nr_buf_chars = tkz_buffer_get_size_in_chars(
                parser->temp_buffer);
        if (nr_buf_chars >= 1 || parser->nr_single_quoted == 2) {
            parser->nr_single_quoted = 0;
            struct pcvcm_node* node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer));
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        else {
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_SINGLE_QUOTED);
        }
    }
    if (character == '\\') {
        parser->nr_single_quoted = 0;
        SET_RETURN_STATE(curr_state);
        ADVANCE_TO(TKZ_STATE_EJSON_STRING_ESCAPE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
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
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            if (tkz_buffer_end_with(parser->temp_buffer, "{", 1)) {
                tkz_reader_reconsume_last_char(parser->reader);
                tkz_reader_reconsume_last_char(parser->reader);
                tkz_buffer_delete_tail_chars(parser->temp_buffer, 1);
                if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                    struct pcvcm_node* node = pcvcm_node_new_string(
                            tkz_buffer_get_bytes(parser->temp_buffer)
                            );
                    APPEND_AS_VCM_CHILD(node);
                    RESET_TEMP_BUFFER();
                }
            }
            else if (tkz_buffer_end_with(parser->temp_buffer, "{{", 2)) {
                tkz_reader_reconsume_last_char(parser->reader);
                tkz_reader_reconsume_last_char(parser->reader);
                tkz_reader_reconsume_last_char(parser->reader);
                tkz_buffer_delete_tail_chars(parser->temp_buffer, 2);
                if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                    struct pcvcm_node* node = pcvcm_node_new_string(
                            tkz_buffer_get_bytes(parser->temp_buffer)
                            );
                    APPEND_AS_VCM_CHILD(node);
                    RESET_TEMP_BUFFER();
                }
            }
            else {
                tkz_reader_reconsume_last_char(parser->reader);
                struct pcvcm_node* node = pcvcm_node_new_string(
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
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
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
    struct pcvcm_node* node = pcvcm_node_new_string(
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
            struct pcvcm_node* node = pcvcm_node_new_string(
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
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
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
        struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('U');
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
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
        tkz_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        uint32_t uc = ejson_stack_top();
        // XXX:
        if (uc == 'U' || uc == 0) {
            RECONSUME_IN(TKZ_STATE_EJSON_JSONEE_STRING);
        }
        else {
            SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD);
            RETURN_AND_STOP_PARSE();
        }
    }

    const char* value = tkz_sbst_get_match(parser->sbst);
    if (value == NULL) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_KEYWORD);
    }
    else {
        RESET_TEMP_BUFFER();
        APPEND_BYTES_TO_TEMP_BUFFER(value, strlen(value));
        tkz_sbst_destroy(parser->sbst);
        parser->sbst = NULL;
        ADVANCE_TO(TKZ_STATE_EJSON_AFTER_KEYWORD);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_AFTER_KEYWORD)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ','
            || character == ')' || character == ';' || character == '&'
            || character == '|') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "true", 4)) {
            RESTORE_VCM_NODE();
            struct pcvcm_node* node = pcvcm_node_new_boolean(true);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        if (tkz_buffer_equal_to(parser->temp_buffer, "false",
                    5)) {
            RESTORE_VCM_NODE();
            struct pcvcm_node* node = pcvcm_node_new_boolean(false);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        if (tkz_buffer_equal_to(parser->temp_buffer, "null", 4)) {
            struct pcvcm_node* node = pcvcm_node_new_null();
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        if (tkz_buffer_equal_to(parser->temp_buffer, "undefined", 9)) {
            struct pcvcm_node* node = pcvcm_node_new_undefined();
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        RESET_TEMP_BUFFER();
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        RETURN_AND_STOP_PARSE();
    }
    RESET_TEMP_BUFFER();
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
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
        struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('U');
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_AFTER_BYTE_SEQUENCE)
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
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_HEX_BYTE_SEQUENCE)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_BYTE_SEQUENCE);
    }
    else if (is_ascii_digit(character)
            || is_ascii_hex_digit(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_HEX_BYTE_SEQUENCE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_BINARY_BYTE_SEQUENCE)
    if (is_whitespace(character) || character == '}'
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
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_BASE64_BYTE_SEQUENCE)
    if (is_whitespace(character) || character == '}'
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
        SET_ERR(PCHVML_ERROR_UNEXPECTED_BASE64);
        RETURN_AND_STOP_PARSE();
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || character == '/' || character == '>') {
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
        struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                NULL);
        UPDATE_VCM_NODE(snode);
        ejson_stack_push('U');
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    SET_ERR(PCHVML_ERROR_BAD_JSON_NUMBER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || character == '/' || character == '>') {
        if (tkz_buffer_end_with(parser->temp_buffer, "-", 1)
            || tkz_buffer_end_with(parser->temp_buffer, "E", 1)
            || tkz_buffer_end_with(parser->temp_buffer, "e", 1)) {
            SET_ERR(PCHVML_ERROR_BAD_JSON_NUMBER);
            RETURN_AND_STOP_PARSE();
        }
        double d = strtod(
                tkz_buffer_get_bytes(parser->temp_buffer), NULL);
        RESTORE_VCM_NODE();
        struct pcvcm_node* node = pcvcm_node_new_number(d);
        APPEND_AS_VCM_CHILD(node);
        RESET_TEMP_BUFFER();
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER_INTEGER)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || character == '>' || character == '/') {
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
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
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
    uint32_t uc = ejson_stack_top();
    if (uc == 'U') {
        RECONSUME_IN(TKZ_STATE_EJSON_JSONEE_STRING);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER_FRACTION)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || character == '/' || character == '>') {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER);
    }

    if (is_ascii_digit(character)) {
        if (tkz_buffer_end_with(parser->temp_buffer, "F", 1)) {
            SET_ERR(PCHVML_ERROR_BAD_JSON_NUMBER);
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
            struct pcvcm_node* node = pcvcm_node_new_longdouble(ld);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            ADVANCE_TO(TKZ_STATE_EJSON_AFTER_VALUE);
        }
    }
    if (character == 'E' || character == 'e') {
        if (tkz_buffer_end_with(parser->temp_buffer, ".", 1)) {
            SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION);
            RETURN_AND_STOP_PARSE();
        }
        APPEND_TO_TEMP_BUFFER('e');
        ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_EXPONENT);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER_EXPONENT)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || character == '/' || character == '>') {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER);
    }
    if (is_ascii_digit(character)) {
        RECONSUME_IN(TKZ_STATE_EJSON_VALUE_NUMBER_EXPONENT_INTEGER);
    }
    if (character == '+' || character == '-') {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_EXPONENT_INTEGER);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER_EXPONENT_INTEGER)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || character == '/' || character == '>') {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER);
    }
    if (is_ascii_digit(character)) {
        if (tkz_buffer_end_with(parser->temp_buffer, "F", 1)) {
            SET_ERR(PCHVML_ERROR_BAD_JSON_NUMBER);
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
            struct pcvcm_node* node = pcvcm_node_new_longdouble(ld);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            ADVANCE_TO(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER);
        }
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER_SUFFIX_INTEGER)
    uint32_t last_c = tkz_buffer_get_last_char(
            parser->temp_buffer);
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || character == '/' || character == '>') {
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
                struct pcvcm_node* node = pcvcm_node_new_ulongint(u64);
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
                struct pcvcm_node* node = pcvcm_node_new_longint(i64);
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
                ADVANCE_TO(TKZ_STATE_EJSON_AFTER_VALUE);
            }
        }
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER_HEX)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || character == '/' || character == '>') {
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER_HEX);
    }
    if (is_ascii_hex_digit(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_HEX);
    }
    if (character == 'U' || character == 'L') {
        RECONSUME_IN(TKZ_STATE_EJSON_VALUE_NUMBER_HEX_SUFFIX);
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER_HEX_SUFFIX)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || character == '/' || character == '>') {
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
    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_AFTER_VALUE_NUMBER_HEX)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')'
            || is_eof(character)
            || character == '/' || character == '>') {
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
    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NUMBER_INFINITY)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        if (tkz_buffer_equal_to(parser->temp_buffer,
                    "-Infinity", 9)) {
            double d = -INFINITY;
            RESTORE_VCM_NODE();
            struct pcvcm_node* node = pcvcm_node_new_number(d);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        if (tkz_buffer_equal_to(parser->temp_buffer,
                "Infinity", 8)) {
            double d = INFINITY;
            RESTORE_VCM_NODE();
            struct pcvcm_node* node = pcvcm_node_new_number(d);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'I') {
        if (tkz_buffer_is_empty(parser->temp_buffer)
            || tkz_buffer_equal_to(parser->temp_buffer, "-", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_INFINITY);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
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
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (character == 'f') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "In", 2)
            || tkz_buffer_equal_to (parser->temp_buffer, "-In", 3)
                ) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NUMBER_INFINITY);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
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
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
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
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
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
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_VALUE_NAN)
    if (is_whitespace(character) || character == '}'
            || character == ']' || character == ',' || character == ')') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "NaN", 3)) {
            double d = NAN;
            RESTORE_VCM_NODE();
            struct pcvcm_node* node = pcvcm_node_new_number(d);
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_VALUE);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }
    if (character == 'N') {
        if (tkz_buffer_is_empty(parser->temp_buffer)
          || tkz_buffer_equal_to(parser->temp_buffer, "Na", 2)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NAN);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    if (character == 'a') {
        if (tkz_buffer_equal_to(parser->temp_buffer, "N", 1)) {
            APPEND_TO_TEMP_BUFFER(character);
            ADVANCE_TO(TKZ_STATE_EJSON_VALUE_NAN);
        }
        SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
        RETURN_AND_STOP_PARSE();
    }

    SET_ERR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
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
            SET_ERR(PCHVML_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY);
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
    SET_ERR(PCHVML_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY);
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
    if (is_whitespace(character) || character == '}'
            || character == '"' || character == ']' || character == ')'
            || character == ';' || character == '&' || character == '|') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
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
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
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
            struct pcvcm_node* node = pcvcm_node_new_object(0, NULL);
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
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
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
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
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
    SET_ERR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_JSONEE_KEYWORD)
    if (is_ascii_digit(character)) {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
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
    if (is_whitespace(character) || character == '[' ||
            character == '(' || character == '<' || character == '}' ||
            character == '$' || character == '>' || character == ']'
            || character == ')' || character == ':' || character == ';'
            || character == '&' || character == '|') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
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
    if (character == '"') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
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
        //ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    if (character == ',') {
        if (tkz_buffer_is_empty(parser->temp_buffer)) {
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
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
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
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
    SET_ERR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_JSONEE_STRING)
    uint32_t uc = ejson_stack_top();
    if (is_whitespace(character)) {
        if (uc == 'U') {
            if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                struct pcvcm_node* node = pcvcm_node_new_string(
                   tkz_buffer_get_bytes(parser->temp_buffer));
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
            }
            RECONSUME_IN(TKZ_STATE_EJSON_AFTER_JSONEE_STRING);
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_STRING);
    }
    if (character == '<' || character == '>' || character == '/') {
        if (uc == 'U' || uc == 0) {
            if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                struct pcvcm_node* node = pcvcm_node_new_string(
                   tkz_buffer_get_bytes(parser->temp_buffer));
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
            }
            RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_STRING);
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
            if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                struct pcvcm_node* node = pcvcm_node_new_string(
                   tkz_buffer_get_bytes(parser->temp_buffer));
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
                ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_STRING);
            }
        }
        else {
            if (!tkz_buffer_is_empty(parser->temp_buffer)) {
                if (!parser->vcm_node) {
                    struct pcvcm_node* snode = pcvcm_node_new_concat_string(0,
                            NULL);
                    UPDATE_VCM_NODE(snode);
                    ejson_stack_push('"');
                }
                struct pcvcm_node* node = pcvcm_node_new_string(
                   tkz_buffer_get_bytes(parser->temp_buffer));
                APPEND_AS_VCM_CHILD(node);
                RESET_TEMP_BUFFER();
            }
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    if (character == '\\') {
        SET_RETURN_STATE(curr_state);
        ADVANCE_TO(TKZ_STATE_EJSON_STRING_ESCAPE);
    }
    if (character == '"') {
        struct pcvcm_node* node = pcvcm_node_new_string(
                tkz_buffer_get_bytes(parser->temp_buffer));
        RESET_TEMP_BUFFER();
        if (parser->vcm_node &&
                parser->vcm_node->type != PCVCM_NODE_TYPE_FUNC_CONCAT_STRING) {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
            vcm_stack_push(parser->vcm_node);
            parser->vcm_node = node;
        }
        else {
            APPEND_AS_VCM_CHILD(node);
        }
        RECONSUME_IN(TKZ_STATE_EJSON_AFTER_JSONEE_STRING);
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
    ADVANCE_TO(TKZ_STATE_EJSON_JSONEE_STRING);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_AFTER_JSONEE_STRING)
    uint32_t uc = ejson_stack_top();
    if (is_whitespace(character)) {
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
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
            SET_ERR(PCHVML_ERROR_BAD_JSONEE_NAME);
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
        POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        ejson_stack_pop();
        if (!ejson_stack_is_empty()) {
            POP_AS_VCM_PARENT_AND_UPDATE_VCM();
        }
        ADVANCE_TO(TKZ_STATE_EJSON_CONTROL);
    }
    SET_ERR(PCHVML_ERROR_BAD_JSONEE_NAME);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_TEMPLATE_DATA)
    if (character == '<') {
        if (!tkz_buffer_is_empty(parser->temp_buffer) &&
                !tkz_buffer_is_whitespace(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        ADVANCE_TO(TKZ_STATE_EJSON_TEMPLATE_DATA_LESS_THAN_SIGN);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '$' && !parser->tag_has_raw_attr) {
        if (!tkz_buffer_is_empty(parser->temp_buffer)) {
            struct pcvcm_node* node = pcvcm_node_new_string(
                    tkz_buffer_get_bytes(parser->temp_buffer)
                    );
            APPEND_AS_VCM_CHILD(node);
            RESET_TEMP_BUFFER();
        }
        RECONSUME_IN(TKZ_STATE_EJSON_CONTROL);
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(TKZ_STATE_EJSON_TEMPLATE_DATA);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_TEMPLATE_DATA_LESS_THAN_SIGN)
    if (character == '/') {
        RESET_TEMP_BUFFER();
        ADVANCE_TO(TKZ_STATE_EJSON_TEMPLATE_DATA_END_TAG_OPEN);
    }
    APPEND_TO_TEMP_BUFFER('<');
    RECONSUME_IN(TKZ_STATE_EJSON_TEMPLATE_DATA);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_TEMPLATE_DATA_END_TAG_OPEN)
    if (is_ascii_alpha(character)) {
        RESET_STRING_BUFFER();
        RECONSUME_IN(TKZ_STATE_EJSON_TEMPLATE_DATA_END_TAG_NAME);
    }
    APPEND_TO_TEMP_BUFFER('<');
    APPEND_TO_TEMP_BUFFER('/');
    RECONSUME_IN(TKZ_STATE_EJSON_TEMPLATE_DATA);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_TEMPLATE_DATA_END_TAG_NAME)
    if (is_ascii_alpha(character)) {
        APPEND_TO_STRING_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_EJSON_TEMPLATE_DATA_END_TAG_NAME);
    }
    if (character == '>') {
        const char* name = tkz_buffer_get_bytes(
                parser->string_buffer);
        if (pchvml_parser_is_appropriate_tag_name(parser, name)) {
            RECONSUME_IN(TKZ_STATE_EJSON_TEMPLATE_FINISHED);
        }
    }
    APPEND_TO_TEMP_BUFFER('<');
    APPEND_TO_TEMP_BUFFER('/');
    APPEND_BUFFER_TO_TEMP_BUFFER(parser->string_buffer);
    RESET_STRING_BUFFER();
    RECONSUME_IN(TKZ_STATE_EJSON_TEMPLATE_DATA);
END_STATE()

BEGIN_STATE(TKZ_STATE_EJSON_TEMPLATE_FINISHED)
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
    RETURN_MULTIPLE_AND_SWITCH_TO(token, next_token, TKZ_STATE_DATA);
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
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
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
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
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
    SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    RETURN_AND_STOP_PARSE();
END_STATE()

PCHVML_NEXT_TOKEN_END


