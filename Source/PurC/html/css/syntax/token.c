/**
 * @file token.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of css token.
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


#include "private/errors.h"

#include "html/core/shs.h"
#include "html/core/conv.h"

#include "html/css/syntax/token.h"

#define PCHTML_CSS_SYNTAX_TOKEN_RES_NAME_SHS_MAP
#include "html/css/syntax/token_res.h"

#define PCHTML_STR_RES_MAP_HEX
#define PCHTML_STR_RES_ANSI_REPLACEMENT_CHARACTER
#include "html/core/str_res.h"


typedef struct {
    pchtml_str_t  *str;
    pchtml_mraw_t *mraw;
}
pchtml_css_syntax_token_ctx_t;


static unsigned int
pchtml_css_syntax_token_make_data_hard(pchtml_in_node_t *in, pchtml_mraw_t *mraw,
                                    pchtml_css_syntax_token_data_t *td,
                                    pchtml_str_t *str, const unsigned char *begin,
                                    const unsigned char *end);

static unsigned int
pchtml_css_syntax_token_make_data_simple(pchtml_in_node_t *in, pchtml_mraw_t *mraw,
                                      pchtml_css_syntax_token_data_t *td,
                                      pchtml_str_t *str, const unsigned char *begin,
                                      const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_token_make_data_conv(const unsigned char *begin, const unsigned char *end,
                                    pchtml_str_t *str, pchtml_mraw_t *mraw,
                                    pchtml_css_syntax_token_data_t *td);

static const unsigned char *
pchtml_css_syntax_token_make_data_conv_num(const unsigned char *begin, const unsigned char *end,
                                        pchtml_str_t *str, pchtml_mraw_t *mraw,
                                        pchtml_css_syntax_token_data_t *td);

static const unsigned char *
pchtml_css_syntax_token_make_data_conv_cr(const unsigned char *begin, const unsigned char *end,
                                       pchtml_str_t *str, pchtml_mraw_t *mraw,
                                       pchtml_css_syntax_token_data_t *td);

static unsigned int
pchtml_css_syntax_token_codepoint_to_ascii(uint32_t cp, pchtml_str_t *str,
                                        pchtml_mraw_t *mraw);

static unsigned int
pchtml_css_syntax_token_str_cb(const unsigned char *data, size_t len, void *ctx);


const unsigned char *
pchtml_css_syntax_token_type_name_by_id(pchtml_css_syntax_token_type_t type)
{
    switch (type) {
        case PCHTML_CSS_SYNTAX_TOKEN_IDENT:
            return (unsigned char *) "ident";
        case PCHTML_CSS_SYNTAX_TOKEN_FUNCTION:
            return (unsigned char *) "function";
        case PCHTML_CSS_SYNTAX_TOKEN_AT_KEYWORD:
            return (unsigned char *) "at-keyword";
        case PCHTML_CSS_SYNTAX_TOKEN_HASH:
            return (unsigned char *) "hash";
        case PCHTML_CSS_SYNTAX_TOKEN_STRING:
            return (unsigned char *) "string";
        case PCHTML_CSS_SYNTAX_TOKEN_BAD_STRING:
            return (unsigned char *) "bad-string";
        case PCHTML_CSS_SYNTAX_TOKEN_URL:
            return (unsigned char *) "url";
        case PCHTML_CSS_SYNTAX_TOKEN_BAD_URL:
            return (unsigned char *) "bad-url";
        case PCHTML_CSS_SYNTAX_TOKEN_DELIM:
            return (unsigned char *) "delim";
        case PCHTML_CSS_SYNTAX_TOKEN_NUMBER:
            return (unsigned char *) "number";
        case PCHTML_CSS_SYNTAX_TOKEN_PERCENTAGE:
            return (unsigned char *) "percentage";
        case PCHTML_CSS_SYNTAX_TOKEN_DIMENSION:
            return (unsigned char *) "dimension";
        case PCHTML_CSS_SYNTAX_TOKEN_WHITESPACE:
            return (unsigned char *) "whitespace";
        case PCHTML_CSS_SYNTAX_TOKEN_CDO:
            return (unsigned char *) "CDO";
        case PCHTML_CSS_SYNTAX_TOKEN_CDC:
            return (unsigned char *) "CDC";
        case PCHTML_CSS_SYNTAX_TOKEN_COLON:
            return (unsigned char *) "colon";
        case PCHTML_CSS_SYNTAX_TOKEN_SEMICOLON:
            return (unsigned char *) "semicolon";
        case PCHTML_CSS_SYNTAX_TOKEN_COMMA:
            return (unsigned char *) "comma";
        case PCHTML_CSS_SYNTAX_TOKEN_LS_BRACKET:
            return (unsigned char *) "left-square-bracket";
        case PCHTML_CSS_SYNTAX_TOKEN_RS_BRACKET:
            return (unsigned char *) "right-square-bracket";
        case PCHTML_CSS_SYNTAX_TOKEN_L_PARENTHESIS:
            return (unsigned char *) "left-parenthesis";
        case PCHTML_CSS_SYNTAX_TOKEN_R_PARENTHESIS:
            return (unsigned char *) "right-parenthesis";
        case PCHTML_CSS_SYNTAX_TOKEN_LC_BRACKET:
            return (unsigned char *) "left-curly-bracket";
        case PCHTML_CSS_SYNTAX_TOKEN_RC_BRACKET:
            return (unsigned char *) "right-curly-bracket";
        case PCHTML_CSS_SYNTAX_TOKEN_COMMENT:
            return (unsigned char *) "comment";
        case PCHTML_CSS_SYNTAX_TOKEN__EOF:
            return (unsigned char *) "end-of-file";
    }

    return (unsigned char *) "undefined";
}

pchtml_css_syntax_token_type_t
pchtml_css_syntax_token_type_id_by_name(const unsigned char *type_name, size_t len)
{
    const pchtml_shs_entry_t *entry;

    entry = pchtml_shs_entry_get_lower_static(pchtml_css_syntax_token_res_name_shs_map,
                                              type_name, len);
    if (entry == NULL) {
        return PCHTML_CSS_SYNTAX_TOKEN_UNDEF;
    }

    return (pchtml_css_syntax_token_type_t) (uintptr_t) entry->value;
}

unsigned int
pchtml_css_syntax_token_make_data(pchtml_css_syntax_token_t *token, pchtml_in_node_t *in,
                               pchtml_mraw_t *mraw, pchtml_css_syntax_token_data_t *td)
{
    switch (token->types.base.type) {
        /* All this types inherit from pchtml_css_syntax_token_string_t. */
        case PCHTML_CSS_SYNTAX_TOKEN_IDENT:
        case PCHTML_CSS_SYNTAX_TOKEN_FUNCTION:
        case PCHTML_CSS_SYNTAX_TOKEN_AT_KEYWORD:
        case PCHTML_CSS_SYNTAX_TOKEN_HASH:
        case PCHTML_CSS_SYNTAX_TOKEN_STRING:
        case PCHTML_CSS_SYNTAX_TOKEN_BAD_STRING:
        case PCHTML_CSS_SYNTAX_TOKEN_URL:
        case PCHTML_CSS_SYNTAX_TOKEN_BAD_URL:
        case PCHTML_CSS_SYNTAX_TOKEN_COMMENT:
        case PCHTML_CSS_SYNTAX_TOKEN_WHITESPACE:
            if (token->types.base.data_type == PCHTML_CSS_SYNTAX_TOKEN_DATA_SIMPLE) {
                return pchtml_css_syntax_token_make_data_simple(in, mraw, td,
                                                     &token->types.string.data,
                                                     token->types.string.begin,
                                                     token->types.string.end);
            }

            td->cb = pchtml_css_syntax_token_make_data_conv;

            return pchtml_css_syntax_token_make_data_hard(in, mraw, td,
                                                       &token->types.string.data,
                                                       token->types.string.begin,
                                                       token->types.string.end);

        case PCHTML_CSS_SYNTAX_TOKEN_DIMENSION:
            if (token->types.base.data_type == PCHTML_CSS_SYNTAX_TOKEN_DATA_SIMPLE) {
                return pchtml_css_syntax_token_make_data_simple(in, mraw, td,
                                                   &token->types.dimension.data,
                                                   token->types.dimension.begin,
                                                   token->types.dimension.end);
            }

            td->cb = pchtml_css_syntax_token_make_data_conv;

            return pchtml_css_syntax_token_make_data_hard(in, mraw, td,
                                                       &token->types.dimension.data,
                                                       token->types.dimension.begin,
                                                       token->types.dimension.end);
    }

    return PCHTML_STATUS_OK;
}

static unsigned int
pchtml_css_syntax_token_make_data_hard(pchtml_in_node_t *in, pchtml_mraw_t *mraw,
                                    pchtml_css_syntax_token_data_t *td,
                                    pchtml_str_t *str, const unsigned char *begin,
                                    const unsigned char *end)
{
    size_t len = 0;
    const unsigned char *ptr = end;

    td->is_last = false;
    td->status = PCHTML_STATUS_OK;

    in = pchtml_in_node_find(in, end);

    do {
        if (pchtml_in_segment(in, begin)) {
            len += ptr - begin;

            break;
        }

        len += ptr - in->begin;

        if (in->prev == NULL) {
            return PCHTML_STATUS_ERROR;
        }

        in = in->prev;
        ptr = in->end;
    }
    while (1);

    if (str->data == NULL) {
        pchtml_str_init(str, mraw, len);
        if (str->data == NULL) {
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }
    else if (pchtml_str_size(str) <= len) {
        ptr = pchtml_str_realloc(str, mraw, (len + 1));
        if (ptr == NULL) {
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    while (pchtml_in_segment(in, end) == false) {
        while (begin < in->end) {
            begin = td->cb(begin, in->end, str, mraw, td);
        }

        if (td->status != PCHTML_STATUS_OK) {
            return td->status;
        }

        in = in->next;
        begin = in->begin;
    }

    td->node_done = in;
    td->is_last = true;

    do {
        begin = td->cb(begin, end, str, mraw, td);
    }
    while (begin < end);

    return td->status;
}

static unsigned int
pchtml_css_syntax_token_make_data_simple(pchtml_in_node_t *in, pchtml_mraw_t *mraw,
                                      pchtml_css_syntax_token_data_t *td,
                                      pchtml_str_t *str, const unsigned char *begin,
                                      const unsigned char *end)
{
    size_t len = 0;
    const unsigned char *ptr = end;

    in = pchtml_in_node_find(in, end);

    do {
        if (pchtml_in_segment(in, begin)) {
            len += ptr - begin;

            break;
        }

        len += ptr - in->begin;

        if (in->prev == NULL) {
            return PCHTML_STATUS_ERROR;
        }

        in = in->prev;
        ptr = in->end;
    }
    while (1);

    if (str->data == NULL) {
        pchtml_str_init(str, mraw, len);
        if (str->data == NULL) {
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }
    else if (pchtml_str_size(str) <= len) {
        ptr = pchtml_str_realloc(str, mraw, (len + 1));
        if (ptr == NULL) {
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    while (pchtml_in_segment(in, end) == false) {
        memcpy(&str->data[str->length], begin, (in->end - begin));
        str->length += (in->end - begin);

        in = in->next;
        begin = in->begin;
    }

    memcpy(&str->data[str->length], begin, (end - begin));
    str->length += (end - begin);

    str->data[str->length] = 0x00;

    td->node_done = in;

    return PCHTML_STATUS_OK;
}

static const unsigned char *
pchtml_css_syntax_token_make_data_conv(const unsigned char *begin, const unsigned char *end,
                                    pchtml_str_t *str, pchtml_mraw_t *mraw,
                                    pchtml_css_syntax_token_data_t *td)
{
    const unsigned char *ptr = begin;

    while (begin < end) {
        switch (*begin) {
            /* U+005C REVERSE SOLIDUS (\) */
            case 0x5C:
                memcpy(&str->data[str->length], ptr, (begin - ptr));
                str->length += begin - ptr;

                begin += 1;

                td->num = 0;
                td->count = 0;

                for (; td->count < 6; td->count++, begin++) {
                    if (begin == end) {
                        if (td->is_last == false) {
                            td->cb = pchtml_css_syntax_token_make_data_conv_num;

                            return begin;
                        }

                        break;
                    }

                    if (pchtml_str_res_map_hex[*begin] == 0xFF) {
                        if (td->count == 0) {
                            if (*begin == 0x0A || *begin == 0x0D
                                || *begin == 0x0C)
                            {
                                goto ws_processing;
                            }

                            td->num = *begin;

                            begin += 1;
                        }

                        break;
                    }

                    td->num <<= 4;
                    td->num |= pchtml_str_res_map_hex[*begin];
                }

                td->status = pchtml_css_syntax_token_codepoint_to_ascii(td->num,
                                                                     str, mraw);
                if (td->status != PCHTML_STATUS_OK) {
                    return end;
                }

ws_processing:

                if (begin == end) {
                    return end;
                }

                /*
                 * U+0009 CHARACTER TABULATION
                 * U+0020 SPACE
                 * U+000A LINE FEED (LF)
                 * U+000C FORM FEED (FF)
                 * U+000D CARRIAGE RETURN (CR)
                 */
                switch (*begin) {
                    case 0x0D:
                        begin += 1;

                        if (begin == end) {
                            td->cb = pchtml_css_syntax_token_make_data_conv_cr;

                            return begin;
                        }

                        if (*begin == 0x0A) {
                            begin += 1;
                        }

                        break;

                    case 0x09:
                    case 0x20:
                    case 0x0A:
                    case 0x0C:
                        begin += 1;

                        break;
                }

                ptr = begin;

                continue;

            /* U+000C FORM FEED (FF) */
            case 0x0C:
                memcpy(&str->data[str->length], ptr, (begin - ptr));
                str->length += begin - ptr;

                begin += 1;

                str->data[str->length] = 0x0A;
                str->length++;

                ptr = begin;

                continue;

            /* U+000D CARRIAGE RETURN (CR) */
            case 0x0D:
                memcpy(&str->data[str->length], ptr, (begin - ptr));
                str->length += begin - ptr;

                begin += 1;

                str->data[str->length] = 0x0A;
                str->length++;

                if (begin == end) {
                    if (td->is_last == false) {
                        td->cb = pchtml_css_syntax_token_make_data_conv_cr;

                        return begin;
                    }

                    return begin;
                }

                /* U+000A LINE FEED (LF) */
                if (*begin == 0x0A) {
                    begin += 1;
                }

                ptr = begin;

                continue;

            /* U+0000 NULL (\0) */
            case 0x00:
                memcpy(&str->data[str->length], ptr, (begin - ptr));
                str->length += begin - ptr;

                ptr = pchtml_str_realloc(str, mraw, pchtml_str_size(str) + 2);
                if (ptr == NULL) {
                    td->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

                    return end;
                }

                memcpy(&str->data[str->length],
                       pchtml_str_res_ansi_replacement_character, 3);

                str->length += 3;

                ptr = begin + 1;
        }

        begin += 1;
    }

    if (ptr < begin) {
        memcpy(&str->data[str->length], ptr, (begin - ptr));
        str->length += begin - ptr;
    }

    if (td->is_last) {
        str->data[str->length] = 0x00;
    }

    return begin;
}

static const unsigned char *
pchtml_css_syntax_token_make_data_conv_num(const unsigned char *begin, const unsigned char *end,
                                        pchtml_str_t *str, pchtml_mraw_t *mraw,
                                        pchtml_css_syntax_token_data_t *td)
{
    for (; td->count < 6; td->count++, begin++) {
        if (begin == end) {
            if (td->is_last == false) {
                return begin;
            }

            break;
        }

        if (pchtml_str_res_map_hex[*begin] == 0xFF) {
            if (td->count == 0) {
                if (*begin == 0x0A || *begin == 0x0D || *begin == 0x0C) {
                    td->cb = pchtml_css_syntax_token_make_data_conv;

                    goto ws_processing;
                }

                td->num = *begin;

                begin += 1;
            }

            break;
        }

        td->num <<= 4;
        td->num |= pchtml_str_res_map_hex[*begin];
    }

    td->cb = pchtml_css_syntax_token_make_data_conv;

    td->status = pchtml_css_syntax_token_codepoint_to_ascii(td->num, str, mraw);
    if (td->status != PCHTML_STATUS_OK) {
        return end;
    }

    if (td->is_last) {
        str->data[str->length] = 0x00;
    }

ws_processing:

    if (begin == end) {
        return end;
    }

    /*
     * U+0009 CHARACTER TABULATION
     * U+0020 SPACE
     * U+000A LINE FEED (LF)
     * U+000C FORM FEED (FF)
     * U+000D CARRIAGE RETURN (CR)
     */
    switch (*begin) {
        case 0x0D:
            begin += 1;

            if (begin == end) {
                td->cb = pchtml_css_syntax_token_make_data_conv_cr;

                return begin;
            }

            if (*begin == 0x0A) {
                begin += 1;
            }

            break;

        case 0x09:
        case 0x20:
        case 0x0A:
        case 0x0C:
            begin += 1;

            break;
    }

    return begin;
}

static const unsigned char *
pchtml_css_syntax_token_make_data_conv_cr(const unsigned char *begin, const unsigned char *end,
                                       pchtml_str_t *str, pchtml_mraw_t *mraw,
                                       pchtml_css_syntax_token_data_t *td)
{
    UNUSED_PARAM(end);
    UNUSED_PARAM(str);
    UNUSED_PARAM(mraw);

    td->cb = pchtml_css_syntax_token_make_data_conv;

    /* U+000A LINE FEED (LF) */
    if (*begin == 0x0A) {
        return (begin + 1);
    }

    return begin;
}

static unsigned int
pchtml_css_syntax_token_codepoint_to_ascii(uint32_t cp, pchtml_str_t *str,
                                        pchtml_mraw_t *mraw)
{
    /*
     * Zero, or is for a surrogate, or is greater than
     * the maximum allowed code point (tkz->num > 0x10FFFF).
     */
    if (cp == 0) {
        unsigned char *ptr;

        ptr = pchtml_str_realloc(str, mraw, pchtml_str_size(str) + 1);
        if (ptr == NULL) {
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        memcpy(&str->data[str->length],
               pchtml_str_res_ansi_replacement_character, 3);

        str->length += 3;

        return PCHTML_STATUS_OK;
    }
    else if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
        memcpy(&str->data[str->length],
               pchtml_str_res_ansi_replacement_character, 3);

        str->length += 3;

        return PCHTML_STATUS_OK;
    }

    unsigned char *data = &str->data[str->length];

    if (cp <= 0x0000007F) {
        /* 0xxxxxxx */
        data[0] = (unsigned char) cp;

        str->length += 1;
    }
    else if (cp <= 0x000007FF) {
        /* 110xxxxx 10xxxxxx */
        data[0] = (char)(0xC0 | (cp >> 6  ));
        data[1] = (char)(0x80 | (cp & 0x3F));

        str->length += 2;
    }
    else if (cp <= 0x0000FFFF) {
        /* 1110xxxx 10xxxxxx 10xxxxxx */
        data[0] = (char)(0xE0 | ((cp >> 12)));
        data[1] = (char)(0x80 | ((cp >> 6 ) & 0x3F));
        data[2] = (char)(0x80 | ( cp & 0x3F));

        str->length += 3;
    }
    else if (cp <= 0x001FFFFF) {
        /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
        data[0] = (char)(0xF0 | ( cp >> 18));
        data[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        data[2] = (char)(0x80 | ((cp >> 6 ) & 0x3F));
        data[3] = (char)(0x80 | ( cp & 0x3F));

        str->length += 4;
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_css_syntax_token_serialize_cb(pchtml_css_syntax_token_t *token,
                                  pchtml_css_syntax_token_cb_f cb, void *ctx)
{
    size_t len;
    unsigned int status;
    unsigned char buf[128];

    switch (token->types.base.type) {
        case PCHTML_CSS_SYNTAX_TOKEN_DELIM:
            return cb(&token->types.delim.character, 1, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_NUMBER:
            len = pchtml_conv_float_to_data(token->types.number.num,
                                            buf, (sizeof(buf) - 1));

            return cb(buf, len, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_PERCENTAGE:
            len = pchtml_conv_float_to_data(token->types.number.num,
                                            buf, (sizeof(buf) - 1));

            status = cb(buf, len, ctx);
            if (status != PCHTML_STATUS_OK) {
                return status;
            }

            return cb((unsigned char *) "%", 1, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_CDO:
            return cb((unsigned char *) "<!--", 4, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_CDC:
            return cb((unsigned char *) "-->", 3, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_COLON:
            return cb((unsigned char *) ":", 1, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_SEMICOLON:
            return cb((unsigned char *) ";", 1, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_COMMA:
            return cb((unsigned char *) ",", 1, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_LS_BRACKET:
            return cb((unsigned char *) "[", 1, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_RS_BRACKET:
            return cb((unsigned char *) "]", 1, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_L_PARENTHESIS:
            return cb((unsigned char *) "(", 1, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_R_PARENTHESIS:
            return cb((unsigned char *) ")", 1, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_LC_BRACKET:
            return cb((unsigned char *) "{", 1, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_RC_BRACKET:
            return cb((unsigned char *) "}", 1, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_HASH:
            status = cb((unsigned char *) "#", 1, ctx);
            if (status != PCHTML_STATUS_OK) {
                return status;
            }

            return cb(token->types.string.data.data,
                      token->types.string.data.length, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_AT_KEYWORD:
            status = cb((unsigned char *) "@", 1, ctx);
            if (status != PCHTML_STATUS_OK) {
                return status;
            }

            return cb(token->types.string.data.data,
                      token->types.string.data.length, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_IDENT:
            return cb(token->types.string.data.data,
                      token->types.string.data.length, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_FUNCTION:
            status = cb(token->types.string.data.data,
                        token->types.string.data.length, ctx);
            if (status != PCHTML_STATUS_OK) {
                return status;
            }

            return cb((unsigned char *) "(", 1, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_STRING:
        case PCHTML_CSS_SYNTAX_TOKEN_BAD_STRING: {
            status = cb((unsigned char *) "\"", 1, ctx);
            if (status != PCHTML_STATUS_OK) {
                return status;
            }

            const unsigned char *begin = token->types.string.data.data;
            const unsigned char *end = token->types.string.data.data
                                    + token->types.string.data.length;

            const unsigned char *ptr = begin;

            for (; begin < end; begin++) {
                /* 0x5C; '\'; Inverse/backward slash */
                if (*begin == 0x5C) {
                    begin += 1;

                    status = cb(ptr, (begin - ptr), ctx);
                    if (status != PCHTML_STATUS_OK) {
                        return status;
                    }

                    if (begin == end) {
                        status = cb((const unsigned char *) "\\", 1, ctx);
                        if (status != PCHTML_STATUS_OK) {
                            return status;
                        }

                        break;
                    }

                    begin -= 1;
                    ptr = begin;
                }
                /* 0x22; '"'; Only quotes above */
                else if (*begin == 0x22) {
                    if (ptr != begin) {
                        status = cb(ptr, (begin - ptr), ctx);
                        if (status != PCHTML_STATUS_OK) {
                            return status;
                        }
                    }

                    status = cb((const unsigned char *) "\\", 1, ctx);
                    if (status != PCHTML_STATUS_OK) {
                        return status;
                    }

                    ptr = begin;
                }
            }

            if (ptr != begin) {
                status = cb(ptr, (begin - ptr), ctx);
                if (status != PCHTML_STATUS_OK) {
                    return status;
                }
            }

            return cb((const unsigned char *) "\"", 1, ctx);
        }

        case PCHTML_CSS_SYNTAX_TOKEN_URL:
        case PCHTML_CSS_SYNTAX_TOKEN_BAD_URL:
            status = cb((unsigned char *) "url(", 4, ctx);
            if (status != PCHTML_STATUS_OK) {
                return status;
            }

            status = cb(token->types.string.data.data,
                        token->types.string.data.length, ctx);
            if (status != PCHTML_STATUS_OK) {
                return status;
            }

            return cb((unsigned char *) ")", 1, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_COMMENT:
            status = cb((unsigned char *) "/*", 2, ctx);
            if (status != PCHTML_STATUS_OK) {
                return status;
            }

            status = cb(token->types.string.data.data,
                        token->types.string.data.length, ctx);
            if (status != PCHTML_STATUS_OK) {
                return status;
            }

            return cb((unsigned char *) "*/", 2, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_WHITESPACE:
            return cb(token->types.whitespace.data.data,
                      token->types.whitespace.data.length, ctx);

        case PCHTML_CSS_SYNTAX_TOKEN_DIMENSION:
            len = pchtml_conv_float_to_data(token->types.number.num,
                                            buf, (sizeof(buf) - 1));

            status = cb(buf, len, ctx);
            if (status != PCHTML_STATUS_OK) {
                return status;
            }

            return cb(token->types.dimension.data.data,
                      token->types.dimension.data.length, ctx);
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_css_syntax_token_serialize_str(pchtml_css_syntax_token_t *token,
                                   pchtml_str_t *str, pchtml_mraw_t *mraw)
{
    pchtml_css_syntax_token_ctx_t ctx;

    ctx.str = str;
    ctx.mraw = mraw;

    if (str->data == NULL) {
        pchtml_str_init(str, mraw, 1);
        if (str->data == NULL) {
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    return pchtml_css_syntax_token_serialize_cb(token,
                                             pchtml_css_syntax_token_str_cb, &ctx);
}

static unsigned int
pchtml_css_syntax_token_str_cb(const unsigned char *data, size_t len, void *cb_ctx)
{
    unsigned char *ptr;
    pchtml_css_syntax_token_ctx_t *ctx = (pchtml_css_syntax_token_ctx_t *) cb_ctx;

    ptr = pchtml_str_append(ctx->str, ctx->mraw, data, len);
    if (ptr == NULL) {
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    return PCHTML_STATUS_OK;
}

/*
 * No inline functions for ABI.
 */
pchtml_css_syntax_token_t *
pchtml_css_syntax_token_create_noi(pchtml_dobject_t *dobj)
{
    return pchtml_css_syntax_token_create(dobj);
}

void
pchtml_css_syntax_token_clean_noi(pchtml_css_syntax_token_t *token)
{
    pchtml_css_syntax_token_clean(token);
}

pchtml_css_syntax_token_t *
pchtml_css_syntax_token_destroy_noi(pchtml_css_syntax_token_t *token,
                                 pchtml_dobject_t *dobj)
{
    return pchtml_css_syntax_token_destroy(token, dobj);
}

const unsigned char *
pchtml_css_syntax_token_type_name_noi(pchtml_css_syntax_token_t *token)
{
    return pchtml_css_syntax_token_type_name(token);
}

pchtml_css_syntax_token_type_t
pchtml_css_syntax_token_type_noi(pchtml_css_syntax_token_t *token)
{
    return pchtml_css_syntax_token_type(token);
}
