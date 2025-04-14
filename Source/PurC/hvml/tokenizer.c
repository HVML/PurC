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
#include "private/ejson.h"

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

#define EXTERNAL_LANG_STYLE     "style"
#define EXTERNAL_LANG_SCRIPT    "script"

#define PARSER_ERROR_TYPE       "HVML parse error"

#define PLOG                    PC_INFO

#define PRINT_STATE(state_name)                                             \
    if (parser->enable_log) {                                               \
        PLOG(                                                               \
            "in %s|uc=%c|hex=0x%X|utf8=%s|fh=%d\n",                         \
            curr_state_name, character, character,                          \
            parser->curr_uc->utf8_buf,                                      \
            parser->is_in_file_header                                       \
            );                                                              \
    }

#define SET_ERR(err)    do {                                                \
    if (parser->curr_uc) {                                                  \
        char buf[ERROR_BUF_SIZE+1];                                         \
        snprintf(buf, ERROR_BUF_SIZE,                                       \
                "line=%d, column=%d, character=%c",                         \
                parser->curr_uc->line,                                      \
                parser->curr_uc->column,                                    \
                parser->curr_uc->character);                                \
        if (parser->enable_log) {                                           \
            PLOG( "%s:%d|%s|%s\n", __FILE__, __LINE__, #err, buf);          \
        }                                                                   \
    }                                                                       \
    tkz_set_error_info(parser->reader, parser->curr_uc, err,                \
            PARSER_ERROR_TYPE, NULL);                                       \
} while (0)

#define SET_ERR_WITH_UC(err, uc, extra)    do {                             \
    if (uc) {                                                               \
        char buf[ERROR_BUF_SIZE+1];                                         \
        snprintf(buf, ERROR_BUF_SIZE,                                       \
                "line=%d, column=%d, character=%c",                         \
                uc->line,                                                   \
                uc->column,                                                 \
                uc->character);                                             \
        if (parser->enable_log) {                                           \
            PLOG( "%s:%d|%s|%s\n", __FILE__, __LINE__, #err, buf);          \
        }                                                                   \
    }                                                                       \
    tkz_set_error_info(parser->reader, uc, err,                             \
            PARSER_ERROR_TYPE, extra);                                      \
} while (0)

#define PCHVML_NEXT_TOKEN_BEGIN                                         \
struct pchvml_token* pchvml_next_token(struct pchvml_parser* parser)    \
{                                                                       \
    struct tkz_uc first_uc = {};                                        \
    struct tkz_uc multi_token_first_uc = {};                            \
    uint32_t character = 0;                                             \
    if (parser->token) {                                                \
        struct pchvml_token* token = parser->token;                     \
        parser->token = NULL;                                           \
        parser->last_token_type = pchvml_token_get_type(token);         \
        return token;                                                   \
    }                                                                   \
                                                                        \
next_input:                                                             \
    parser->curr_uc = tkz_reader_next_char (parser->reader);            \
    if (!parser->curr_uc) {                                             \
        return NULL;                                                    \
    }                                                                   \
    if (!first_uc.line) {                                               \
        first_uc = *parser->curr_uc;                                    \
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
    if (parser->record_ucs) {                                           \
        APPEND_TO_TEMP_UCS(*parser->curr_uc);                           \
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
            RESET_TEMP_BUFFER();                                            \
            RESET_STRING_BUFFER();                                          \
            RESET_TEMP_UCS();                                               \
            START_RECORD_UCS();                                             \
            parser->state = TKZ_STATE_TEMPLATE_DATA;                        \
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
        pchvml_token_set_first_uc(token, &first_uc);                        \
        return token;                                                       \
    } while (false)

#define RETURN_AND_RECONSUME_IN(next_state)                                 \
    do {                                                                    \
        parser->state = next_state;                                         \
        pchvml_parser_save_tag_name(parser);                                \
        pchvml_token_done(parser->token);                                   \
        struct pchvml_token* token = parser->token;                         \
        parser->token = NULL;                                               \
        tkz_reader_reconsume_last_char(parser->reader);                     \
        parser->last_token_type = pchvml_token_get_type(token);             \
        pchvml_token_set_first_uc(token, &first_uc);                        \
        return token;                                                       \
    } while (false)

#define RETURN_CURRENT_TOKEN()                                              \
    do {                                                                    \
        pchvml_token_done(parser->token);                                   \
        struct pchvml_token* token = parser->token;                         \
        parser->token = NULL;                                               \
        parser->last_token_type = pchvml_token_get_type(token);             \
        pchvml_token_set_first_uc(token, &first_uc);                        \
        return token;                                                       \
    } while (false)

#define RETURN_NEW_EOF_TOKEN()                                              \
    do {                                                                    \
        if (parser->token) {                                                \
            struct pchvml_token* token = parser->token;                     \
            parser->token = pchvml_token_new_eof();                         \
            parser->last_token_type = pchvml_token_get_type(token);         \
            pchvml_token_set_first_uc(token, &first_uc);                    \
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
        pchvml_token_set_first_uc(token, &first_uc);                        \
        return token;                                                       \
    } while (false)

#define RETURN_AND_STOP_PARSE()                                             \
    do {                                                                    \
        return NULL;                                                        \
    } while (false)

#define RESET_TEMP_BUFFER()                                                 \
    do {                                                                    \
        tkz_buffer_reset(parser->temp_buffer);                              \
    } while (false)

#define APPEND_TO_TEMP_BUFFER(c)                                            \
    do {                                                                    \
        tkz_buffer_append(parser->temp_buffer, c);                          \
    } while (false)

#define APPEND_BYTES_TO_TEMP_BUFFER(bytes, nr_bytes)                        \
    do {                                                                    \
        tkz_buffer_append_bytes(parser->temp_buffer, bytes, nr_bytes);      \
    } while (false)

#define APPEND_BUFFER_TO_TEMP_BUFFER(buffer)                                \
    do {                                                                    \
        tkz_buffer_append_another(parser->temp_buffer, buffer);             \
    } while (false)

#define IS_TEMP_BUFFER_EMPTY()                                              \
        tkz_buffer_is_empty(parser->temp_buffer)

#define RESET_STRING_BUFFER()                                               \
    do {                                                                    \
        tkz_buffer_reset(parser->string_buffer);                            \
    } while (false)

#define APPEND_TO_STRING_BUFFER(uc)                                         \
    do {                                                                    \
        tkz_buffer_append(parser->string_buffer, uc);                       \
    } while (false)

#define APPEND_BYTES_TO_STRING_BUFFER(bytes, nr_bytes)                      \
    do {                                                                    \
        tkz_buffer_append_bytes(parser->string_buffer, bytes, nr_bytes);    \
    } while (false)

#define RESET_TEMP_UCS()                                                    \
    do {                                                                    \
        tkz_ucs_reset(parser->temp_ucs);                                    \
    } while (false)

#define APPEND_TO_TEMP_UCS(uc)                                              \
    do {                                                                    \
        tkz_ucs_add_tail(parser->temp_ucs, uc);                             \
    } while (false)

#define START_RECORD_UCS(uc)                                                \
    do {                                                                    \
        tkz_ucs_reset(parser->temp_ucs);                                    \
        parser->record_ucs = 1;                                             \
    } while (false)

#define STOP_RECORD_UCS(uc)                                                 \
    do {                                                                    \
        parser->record_ucs = 0;                                             \
    } while (false)

#define RESET_SINGLE_QUOTED_COUNTER()                                       \
    do {                                                                    \
        parser->nr_single_quoted = 0;                                       \
    } while (false)

#define RESET_DOUBLE_QUOTED_COUNTER()                                       \
    do {                                                                    \
        parser->nr_double_quoted = 0;                                       \
    } while (false)

#define APPEND_TO_TOKEN_NAME(uc)                                            \
    do {                                                                    \
        pchvml_token_append_to_name(parser->token, uc);                     \
    } while (false)

#define APPEND_TEMP_BUFFER_TO_TOKEN_TEXT()                                  \
    do {                                                                    \
        const char* c = tkz_buffer_get_bytes(parser->temp_buffer);          \
        size_t nr_c = tkz_buffer_get_size_in_bytes(                         \
                parser->temp_buffer);                                       \
        pchvml_token_append_bytes_to_text(parser->token, c, nr_c);          \
        tkz_buffer_reset(parser->temp_buffer);                              \
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
        const char* c = tkz_buffer_get_bytes(buffer);                       \
        size_t nr_c = tkz_buffer_get_size_in_bytes(buffer);                 \
        pchvml_token_append_bytes_to_attr_value(parser->token, c, nr_c);    \
    } while (false)

#define APPEND_TO_TOKEN_ATTR_NAME(c)                                        \
    do {                                                                    \
        pchvml_token_append_to_attr_name(parser->token, c);                 \
    } while (false)

#define APPEND_TEMP_BUFFER_TO_TOKEN_ATTR_NAME()                             \
    do {                                                                    \
        const char* c = tkz_buffer_get_bytes(parser->temp_buffer);          \
        size_t nr_c = tkz_buffer_get_size_in_bytes(                         \
                parser->temp_buffer);                                       \
        pchvml_token_append_bytes_to_attr_name(parser->token, c, nr_c);     \
        tkz_buffer_reset(parser->temp_buffer);                              \
    } while (false)

#define VERIFY_VERB_TAG_ATTR_NAME()                                         \
    do {                                                                    \
        struct pchvml_token_attr *attr = pchvml_token_get_curr_attr(        \
                parser->token);                                             \
        if (pchvml_parser_is_verb_tag_token(parser->token) && attr          \
            && !pchvml_parser_is_prep_or_adverb_attribute(attr)) {          \
            struct tkz_uc uc = tkz_ucs_read_head(parser->temp_ucs);         \
            struct tkz_uc *p = &uc;                                         \
            if (!is_ascii_alpha(p->character)) {                            \
                SET_ERR_WITH_UC(                                            \
                  PCHVML_ERROR_NOT_EXPLICIT_ATTRIBUTE_NAME, p, NULL);       \
            }                                                               \
            else {                                                          \
                SET_ERR_WITH_UC(                                            \
                    PCHVML_ERROR_UNKNOWN_ATTRIBUTE_NAME_FOR_VERB_ELEMENT,   \
                    p, NULL);                                               \
            }                                                               \
            RETURN_AND_STOP_PARSE();                                        \
        }                                                                   \
    } while (false)

#define VERIFY_VERB_TAG_ATTR_NAME_DUPLICATE()                               \
    do {                                                                    \
        struct pchvml_token_attr *attr = pchvml_token_get_curr_attr(        \
                parser->token);                                             \
        if (pchvml_parser_is_verb_tag_token(parser->token) && attr          \
            && pchvml_token_is_curr_attr_duplicate(parser->token)) {        \
            struct tkz_uc uc = tkz_ucs_read_head(parser->temp_ucs);         \
            struct tkz_uc *p = &uc;                                         \
            const char *name = pchvml_token_attr_get_name(attr);            \
            if (name) {                                                     \
                char extra[PURC_LEN_PROPERTY_NAME * 2] = {0};               \
                snprintf(extra, sizeof(extra), "`%s`.", name);              \
                SET_ERR_WITH_UC(                                            \
                    PCHVML_ERROR_DUPLICATE_ATTRIBUTE_NAME, p, extra);       \
            }                                                               \
            else {                                                          \
                SET_ERR_WITH_UC(                                            \
                    PCHVML_ERROR_DUPLICATE_ATTRIBUTE_NAME, p, NULL);        \
            }                                                               \
            RETURN_AND_STOP_PARSE();                                        \
        }                                                                   \
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
            || strcmp(name, "request") == 0
            || strcmp(name, "load") == 0
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
bool pchvml_parser_is_verb_tag(const char* name)
{
    if (!name) {
        return false;
    }
    const struct pchvml_tag_entry* entry = pchvml_tag_static_search(name,
            strlen(name));
    return (entry &&
            (entry->cats & PCHVML_TAGCAT_VERB));
}

static UNUSED_FUNCTION
bool pchvml_parser_is_in_operation(struct pchvml_parser* parser)
{
    const char* name = tkz_buffer_get_bytes(parser->tag_name);
    return pchvml_parser_is_operation_tag(name);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_external_lang(struct pchvml_parser* parser)
{
    const char* name = tkz_buffer_get_bytes(parser->tag_name);
    if (strcasecmp(name, EXTERNAL_LANG_STYLE) == 0
            || strcasecmp(name, EXTERNAL_LANG_SCRIPT) == 0) {
        return true;
    }
    return false;
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
bool pchvml_parser_is_verb_tag_token (struct pchvml_token* token)
{
    const char* name = pchvml_token_get_name(token);
    return pchvml_parser_is_verb_tag(name);
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
bool pchvml_parser_is_prep_or_adverb_attribute (
        struct pchvml_token_attr* attr)
{
    const char* name = pchvml_token_attr_get_name(attr);
    const struct pchvml_attr_entry* entry =pchvml_attr_static_search(name,
            strlen(name));
    return (entry && (entry->type == PCHVML_ATTR_TYPE_PREP ||
                entry->type == PCHVML_ATTR_TYPE_ADVERB));
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

#if 0
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
#endif

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
    UNUSED_PARAM(parser);
//    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
}

static bool
is_finished_default(struct pcejson *parser, uint32_t character)
{
    UNUSED_PARAM(parser);
    UNUSED_PARAM(character);
    return false;
}

struct pcvcm_node *
parse_ejson_ex(struct pchvml_parser *parser, struct tkz_ucs *ucs,
        bool parse_double_quoted_value)
{
    struct pcvcm_node *node = NULL;
    if (!ucs) {
        goto out;
    }

    uint32_t flags = PCEJSON_FLAG_ALL;
    if (parser->tag_has_raw_attr) {
        flags = flags & ~PCEJSON_FLAG_GET_VARIABLE;
    }
    pcejson_reset(parser->ejson_parser, parser->ejson_parser_max_depth,
            flags);
    struct tkz_reader *reader = tkz_reader_new();
    if (!reader) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }
    tkz_reader_set_data_source_ucs(reader, ucs);
    tkz_reader_set_lc(reader, parser->lc);

    /* use temp reader */
    if (parse_double_quoted_value) {
        pcejson_update_state_to_parse_double_quoted_attr_value(
                parser->ejson_parser);
    }
    pcejson_parse_full(&node, &parser->ejson_parser, reader,
            parser->ejson_parser_max_depth, is_finished_default);
    tkz_reader_destroy(reader);
out:
    return node;
}

bool
is_quoted_attr_finished(struct pcejson *ejson, uint32_t character)
{
    UNUSED_PARAM(ejson);
    if (character == '/' || character == '>' || is_whitespace(character)) {
        return true;
    }
    return false;
}

bool
is_unquoted_attr_finished(struct pcejson *ejson, uint32_t character)
{
    UNUSED_PARAM(ejson);
    if (character == '/' || character == '>') {
        return true;
    }
    return false;
}

bool
is_backquote_attr_finished(struct pcejson *ejson, uint32_t character)
{
    UNUSED_PARAM(ejson);
    if (character == '`') {
        return true;
    }
    return false;
}

bool
is_content_text_finished(struct pcejson *ejson, uint32_t character)
{
    UNUSED_PARAM(ejson);
    if (character == '<') {
        return true;
    }
    return false;
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
        RESET_TEMP_BUFFER();
        if (parser->token) {
            RETURN_AND_SWITCH_TO(TKZ_STATE_TAG_OPEN);
        }
        ADVANCE_TO(TKZ_STATE_TAG_OPEN);
    }
    if (is_eof(character)) {
        RETURN_NEW_EOF_TOKEN();
    }
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
            node->position = 0;
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

    bool operation = pchvml_parser_is_in_operation(parser);
    bool external_lang = pchvml_parser_is_external_lang(parser);

    uint32_t dest_state = TKZ_STATE_CONTENT_JSONEE;
    if ((parser->tag_has_raw_attr && !operation) || external_lang) {
        dest_state = TKZ_STATE_CONTENT_TEXT;
    }

    if(!IS_TEMP_BUFFER_EMPTY()) {
        struct pcvcm_node* node = TEMP_BUFFER_TO_VCM_NODE();
        if (!node) {
            RETURN_AND_STOP_PARSE();
        }
        node->position = 0;
        RESET_TEMP_BUFFER();
        parser->token = pchvml_token_new_vcm(node);
        if (!parser->token) {
            RETURN_AND_STOP_PARSE();
        }
        pchvml_token_set_is_whitespace(parser->token, true);
        RETURN_AND_RECONSUME_IN(dest_state);
    }
    RECONSUME_IN(dest_state);
END_STATE()

BEGIN_STATE(TKZ_STATE_TAG_NAME)
    if (is_whitespace(character)) {
        RESET_TEMP_UCS();
        ADVANCE_TO(TKZ_STATE_BEFORE_ATTRIBUTE_NAME);
    }
    if (character == '/') {
        RESET_TEMP_BUFFER();
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
    RESET_TEMP_UCS();
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
        struct pchvml_token_attr *attr = pchvml_token_get_curr_attr(
                parser->token);
        if (attr) {
            const char *name = pchvml_token_attr_get_name(attr);
            if (!name) {
                SET_ERR_WITH_UC(
                  PCHVML_ERROR_NOT_EXPLICIT_ATTRIBUTE_NAME, parser->curr_uc,
                  NULL);
                RETURN_AND_STOP_PARSE();
            }
        }
        RESET_TEMP_BUFFER();
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(
        TKZ_STATE_SPECIAL_ATTRIBUTE_OPERATOR_IN_ATTRIBUTE_NAME);
    }
    if (character == '/') {
        RECONSUME_IN(TKZ_STATE_AFTER_ATTRIBUTE_NAME);
    }
    APPEND_TO_TEMP_UCS(*parser->curr_uc);
    APPEND_TO_TOKEN_ATTR_NAME(character);
    ADVANCE_TO(TKZ_STATE_ATTRIBUTE_NAME);
END_STATE()

BEGIN_STATE(TKZ_STATE_AFTER_ATTRIBUTE_NAME)
    if (is_whitespace(character)) {
        ADVANCE_TO(TKZ_STATE_AFTER_ATTRIBUTE_NAME);
    }

    VERIFY_VERB_TAG_ATTR_NAME();
    VERIFY_VERB_TAG_ATTR_NAME_DUPLICATE();

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
        RESET_TEMP_BUFFER();
        ADVANCE_TO(TKZ_STATE_SELF_CLOSING_START_TAG);
    }
    RESET_TEMP_UCS();
    END_TOKEN_ATTR();
    BEGIN_TOKEN_ATTR();
    RECONSUME_IN(TKZ_STATE_ATTRIBUTE_NAME);
END_STATE()

BEGIN_STATE(TKZ_STATE_BEFORE_ATTRIBUTE_VALUE)
    VERIFY_VERB_TAG_ATTR_NAME();
    VERIFY_VERB_TAG_ATTR_NAME_DUPLICATE();

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
        RESET_DOUBLE_QUOTED_COUNTER();
        RESET_TEMP_UCS();
        START_RECORD_UCS();
        RECONSUME_IN(TKZ_STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED);
    }
    if (character == '\'') {
        RESET_TEMP_BUFFER();
        RESET_SINGLE_QUOTED_COUNTER();
        RECONSUME_IN(TKZ_STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED);
    }
    if (character == '`') {
        RESET_TEMP_BUFFER();
        RECONSUME_IN(TKZ_STATE_ATTRIBUTE_VALUE_BACKQUOTE);
    }
    RESET_TEMP_BUFFER();
    RECONSUME_IN(TKZ_STATE_ATTRIBUTE_VALUE_UNQUOTED);
END_STATE()

BEGIN_STATE(TKZ_STATE_AFTER_ATTRIBUTE_VALUE)
    if (is_whitespace(character)) {
        RESET_TEMP_UCS();
        ADVANCE_TO(TKZ_STATE_BEFORE_ATTRIBUTE_NAME);
    }
    if (character == '/') {
        RESET_TEMP_BUFFER();
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
        node->position = 0;
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
        ADVANCE_TO(TKZ_STATE_HEXADECIMAL_CHARACTER_REFERENCE);
    }
    if (is_ascii_upper_hex_digit(character)) {
        parser->char_ref_code *= 16;
        parser->char_ref_code += character - 0x37;
        ADVANCE_TO(TKZ_STATE_HEXADECIMAL_CHARACTER_REFERENCE);
    }
    if (is_ascii_lower_hex_digit(character)) {
        parser->char_ref_code *= 16;
        parser->char_ref_code += character - 0x57;
        ADVANCE_TO(TKZ_STATE_HEXADECIMAL_CHARACTER_REFERENCE);
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
        RESET_TEMP_BUFFER();
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
        RESET_TEMP_BUFFER();
        RECONSUME_IN(TKZ_STATE_SELF_CLOSING_START_TAG);
    }
    BEGIN_TOKEN_ATTR();
    APPEND_TEMP_BUFFER_TO_TOKEN_ATTR_NAME();
    RECONSUME_IN(TKZ_STATE_ATTRIBUTE_NAME);
END_STATE()

BEGIN_STATE(TKZ_STATE_TEMPLATE_DATA)
    if (character == '<') {
        multi_token_first_uc = *parser->curr_uc;
        ADVANCE_TO(TKZ_STATE_TEMPLATE_DATA_LESS_THAN_SIGN);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(TKZ_STATE_TEMPLATE_DATA);
END_STATE()

BEGIN_STATE(TKZ_STATE_TEMPLATE_DATA_LESS_THAN_SIGN)
    if (character == '/') {
        ADVANCE_TO(TKZ_STATE_TEMPLATE_DATA_END_TAG_OPEN);
    }
    APPEND_TO_TEMP_BUFFER('<');
    RECONSUME_IN(TKZ_STATE_TEMPLATE_DATA);
END_STATE()

BEGIN_STATE(TKZ_STATE_TEMPLATE_DATA_END_TAG_OPEN)
    if (is_ascii_alpha(character)) {
        RESET_STRING_BUFFER();
        RECONSUME_IN(TKZ_STATE_TEMPLATE_DATA_END_TAG_NAME);
    }
    APPEND_TO_TEMP_BUFFER('<');
    APPEND_TO_TEMP_BUFFER('/');
    RECONSUME_IN(TKZ_STATE_TEMPLATE_DATA);
END_STATE()

BEGIN_STATE(TKZ_STATE_TEMPLATE_DATA_END_TAG_NAME)
    if (is_ascii_alpha(character)) {
        APPEND_TO_STRING_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_TEMPLATE_DATA_END_TAG_NAME);
    }
    if (character == '>') {
        const char* name = tkz_buffer_get_bytes(
                parser->string_buffer);
        if (pchvml_parser_is_appropriate_tag_name(parser, name)) {
            /* </ + tag_name + > */
            size_t sz = strlen(name) + 3;
            tkz_ucs_delete_tail(parser->temp_ucs, sz);
            RECONSUME_IN(TKZ_STATE_TEMPLATE_FINISHED);
        }
    }
    APPEND_TO_TEMP_BUFFER('<');
    APPEND_TO_TEMP_BUFFER('/');
    APPEND_BUFFER_TO_TEMP_BUFFER(parser->string_buffer);
    RESET_STRING_BUFFER();
    RECONSUME_IN(TKZ_STATE_TEMPLATE_DATA);
END_STATE()

BEGIN_STATE(TKZ_STATE_TEMPLATE_FINISHED)
    struct pcvcm_node *node = NULL;
    if (tkz_buffer_is_whitespace(parser->temp_buffer)) {
        node = TEMP_BUFFER_TO_VCM_NODE();
        if (node) {
            node->position = 0;
        }
    }
    else {
        STOP_RECORD_UCS();
        node = parse_ejson_ex(parser, parser->temp_ucs, false);
        RESET_TEMP_UCS();
    }

    if (!node) {
        RETURN_AND_STOP_PARSE();
    }
    struct pchvml_token* token = pchvml_token_new_vcm(node);
    struct pchvml_token* next_token = pchvml_token_new_end_tag();
    pchvml_token_append_buffer_to_name(next_token,
            parser->string_buffer);
    pchvml_token_set_first_uc(next_token, &multi_token_first_uc);
    RESET_TEMP_BUFFER();
    RESET_STRING_BUFFER();
    RETURN_MULTIPLE_AND_SWITCH_TO(token, next_token, TKZ_STATE_DATA);
END_STATE()

BEGIN_STATE(TKZ_STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED)
#if 0
    tkz_reader_reconsume_last_char(parser->reader);
    pcejson_reset(parser->ejson_parser, parser->ejson_parser_max_depth,
            parser->ejson_parser_flags);
    struct pcvcm_node *node = NULL;
    pcejson_parse_full(&node, &parser->ejson_parser, parser->reader,
            parser->ejson_parser_max_depth, is_quoted_attr_finished);
    if (node) {
        tkz_reader_reconsume_last_char(parser->reader);
        pchvml_token_append_vcm_to_attr(parser->token, node);
        END_TOKEN_ATTR();
        ADVANCE_TO(TKZ_STATE_AFTER_ATTRIBUTE_VALUE);
    }
    RETURN_AND_STOP_PARSE();
#else
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '\"') {
        if (parser->nr_double_quoted == 0) {
            parser->nr_double_quoted++;
            ADVANCE_TO(TKZ_STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED);
        }
        else if (parser->nr_double_quoted == 1) {
            parser->nr_double_quoted++;
            ADVANCE_TO(TKZ_STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED);
        }
        else {
            SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
    }
    if (parser->nr_double_quoted < 2) {
#if 0
        /* handle on ejson parser */
        if (character == '&') {
            SET_RETURN_STATE(TKZ_STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED);
            ADVANCE_TO(TKZ_STATE_CHARACTER_REFERENCE);
        }
#endif
        if (is_c0(character)) {
            uint32_t c = tkz_buffer_get_last_char(parser->temp_buffer);
            if (c != '\\') {
                SET_ERR(PCHVML_ERROR_UNEXPECTED_UNESCAPED_CONTROL_CHARACTER);
                RETURN_AND_STOP_PARSE();
            }
        }

        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED);
    }

    struct pcvcm_node *node = NULL;
    if (tkz_buffer_is_empty(parser->temp_buffer)) {
        node = pcvcm_node_new_string("");
        node->position = 0;
    }
    else {
        STOP_RECORD_UCS();
        /* end with '"' + other char */
        tkz_ucs_delete_tail(parser->temp_ucs, 2);
        node = parse_ejson_ex(parser, parser->temp_ucs, true);
        RESET_TEMP_UCS();
    }
    if (node) {
        node->quoted_type = PCVCM_NODE_QUOTED_TYPE_DOUBLE;
        pchvml_token_set_quote(parser->token, '\"');
        pchvml_token_append_vcm_to_attr(parser->token, node);
        END_TOKEN_ATTR();
        RESET_TEMP_BUFFER();
        RECONSUME_IN(TKZ_STATE_AFTER_ATTRIBUTE_VALUE);
    }
    RETURN_AND_STOP_PARSE();
#endif
END_STATE()

BEGIN_STATE(TKZ_STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED)
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '\'') {
        if (parser->nr_single_quoted == 0) {
            parser->nr_single_quoted++;
            ADVANCE_TO(TKZ_STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED);
        }
        else if (parser->nr_single_quoted == 1) {
            parser->nr_single_quoted++;
            ADVANCE_TO(TKZ_STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED);
        }
        else {
            SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
    }
    if (parser->nr_single_quoted < 2) {
        if (character == '&') {
            SET_RETURN_STATE(TKZ_STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED);
            ADVANCE_TO(TKZ_STATE_CHARACTER_REFERENCE);
        }
        if (character == '\\') {
            SET_RETURN_STATE(curr_state);
            ADVANCE_TO(TKZ_STATE_ATTRIBUTE_STRING_ESCAPE);
        }
        if (is_c0(character)) {
            SET_ERR(PCHVML_ERROR_UNEXPECTED_UNESCAPED_CONTROL_CHARACTER);
            RETURN_AND_STOP_PARSE();
        }
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TKZ_STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED);
    }
    pchvml_token_set_quote(parser->token, '\'');
    APPEND_BUFFER_TO_TOKEN_ATTR_VALUE(parser->temp_buffer);
    END_TOKEN_ATTR();
    RESET_TEMP_BUFFER();
    RECONSUME_IN(TKZ_STATE_AFTER_ATTRIBUTE_VALUE);
END_STATE()

BEGIN_STATE(TKZ_STATE_ATTRIBUTE_VALUE_UNQUOTED)
    tkz_reader_reconsume_last_char(parser->reader);
    uint32_t flags = PCEJSON_FLAG_ALL & ~PCEJSON_FLAG_MULTI_JSONEE & ~PCEJSON_FLAG_KEEP_LAST_CHAR;
    pcejson_reset(parser->ejson_parser, parser->ejson_parser_max_depth,
            flags);
    struct pcvcm_node *node = NULL;
    /* use hvml parser->reader */
    pcejson_parse_full(&node, &parser->ejson_parser, parser->reader,
            parser->ejson_parser_max_depth, is_unquoted_attr_finished);
    if (node) {
        node->quoted_type = PCVCM_NODE_QUOTED_TYPE_NONE;
        tkz_reader_reconsume_last_char(parser->reader);
        pchvml_token_append_vcm_to_attr(parser->token, node);
        END_TOKEN_ATTR();
        RESET_TEMP_UCS();
        ADVANCE_TO(TKZ_STATE_BEFORE_ATTRIBUTE_NAME);
    }
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_ATTRIBUTE_VALUE_BACKQUOTE)
    tkz_reader_reconsume_last_char(parser->reader);
    uint32_t flags = PCEJSON_FLAG_ALL & ~PCEJSON_FLAG_MULTI_JSONEE & ~PCEJSON_FLAG_KEEP_LAST_CHAR;
    pcejson_reset(parser->ejson_parser, parser->ejson_parser_max_depth,
            flags);
    struct pcvcm_node *node = NULL;
    /* use hvml parser->reader */
    pcejson_parse_full(&node, &parser->ejson_parser, parser->reader,
            parser->ejson_parser_max_depth, is_backquote_attr_finished);
    if (node) {
        node->quoted_type = PCVCM_NODE_QUOTED_TYPE_BACKQUOTE;
        tkz_reader_reconsume_last_char(parser->reader);
        pchvml_token_append_vcm_to_attr(parser->token, node);
        END_TOKEN_ATTR();
        ADVANCE_TO(TKZ_STATE_BEFORE_ATTRIBUTE_NAME);
    }
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_ATTRIBUTE_STRING_ESCAPE)
    switch (character)
    {
        case 'b':
            APPEND_TO_TEMP_BUFFER('\b');
            ADVANCE_TO(parser->return_state);
            break;
        case 'v':
            APPEND_TO_TEMP_BUFFER('\v');
            ADVANCE_TO(parser->return_state);
            break;
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
                    TKZ_STATE_ATTRIBUTE_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS);
            break;
        default:
            SET_ERR(PCHVML_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY);
            RETURN_AND_STOP_PARSE();
    }
END_STATE()

BEGIN_STATE(TKZ_STATE_ATTRIBUTE_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS)
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
                SET_ERR(PCHVML_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY);
                RETURN_AND_STOP_PARSE();
            }

            APPEND_TO_TEMP_BUFFER(uc);
            ADVANCE_TO(parser->return_state);
        }
        ADVANCE_TO(
                TKZ_STATE_ATTRIBUTE_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS);
    }
    SET_ERR(PCHVML_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TKZ_STATE_CONTENT_TEXT)
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
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
                node->position = 0;
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
                node->position = 0;
                struct pchvml_token* next_token = pchvml_token_new_vcm(node);
                pchvml_token_set_is_whitespace(next_token, true);
                pchvml_token_set_first_uc(next_token, &multi_token_first_uc);
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
                node->position = 0;
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
        SET_RETURN_STATE(TKZ_STATE_CONTENT_TEXT);
        ADVANCE_TO(TKZ_STATE_CHARACTER_REFERENCE);
    }
    if (is_whitespace(character)) {
        if (parser->nr_whitespace == 0) {
            multi_token_first_uc = *parser->curr_uc;
        }
        parser->nr_whitespace++;
    }
    else {
        parser->nr_whitespace = 0;
    }
    APPEND_TO_TEMP_BUFFER(character);
    ADVANCE_TO(TKZ_STATE_CONTENT_TEXT);
END_STATE()

BEGIN_STATE(TKZ_STATE_CONTENT_JSONEE)
    tkz_reader_reconsume_last_char(parser->reader);
    uint32_t flags = PCEJSON_FLAG_ALL & ~PCEJSON_FLAG_KEEP_LAST_CHAR;
    if (parser->tag_has_raw_attr) {
        flags = flags & ~PCEJSON_FLAG_GET_VARIABLE;
    }
    pcejson_reset(parser->ejson_parser, parser->ejson_parser_max_depth,
            flags);
    struct pcvcm_node *node = NULL;

    if (!pchvml_parser_is_in_json_content_tag(parser)) {
        pcejson_set_state_param_string(parser->ejson_parser);
    }
    /* use hvml parser->reader */
    pcejson_parse_full(&node, &parser->ejson_parser, parser->reader,
            parser->ejson_parser_max_depth, is_content_text_finished);
    if (node) {
        tkz_reader_reconsume_last_char(parser->reader);
        parser->token = pchvml_token_new_vcm(node);
        RETURN_AND_SWITCH_TO(TKZ_STATE_DATA);
    }
    RETURN_AND_STOP_PARSE();
END_STATE()

PCHVML_NEXT_TOKEN_END


