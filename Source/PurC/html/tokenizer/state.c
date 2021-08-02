/**
 * @file state.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of html state.
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
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under Apahce 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"

#include "html/tokenizer/state.h"
#include "html/tokenizer/state_comment.h"
#include "html/tokenizer/state_doctype.h"

#define PCHTML_STR_RES_ANSI_REPLACEMENT_CHARACTER
#define PCHTML_STR_RES_ALPHANUMERIC_CHARACTER
#define PCHTML_STR_RES_REPLACEMENT_CHARACTER
#define PCHTML_STR_RES_ALPHA_CHARACTER
#define PCHTML_STR_RES_MAP_HEX
#define PCHTML_STR_RES_MAP_NUM
#include "html/str_res.h"

#define PCHTML_PARSER_TOKENIZER_RES_ENTITIES_SBST
#include "html/tokenizer/res.h"


const pchtml_tag_data_t *
pchtml_tag_append_lower(pchtml_hash_t *hash,
                     const unsigned char *name, size_t length);

pcedom_attr_data_t *
pcedom_attr_local_name_append(pchtml_hash_t *hash,
                               const unsigned char *name, size_t length);


static const unsigned char *
pchtml_html_tokenizer_state_data(pchtml_html_tokenizer_t *tkz,
                              const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_plaintext(pchtml_html_tokenizer_t *tkz,
                                   const unsigned char *data,
                                   const unsigned char *end);

/* Tag */
static const unsigned char *
pchtml_html_tokenizer_state_tag_open(pchtml_html_tokenizer_t *tkz,
                                  const unsigned char *data,
                                  const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_end_tag_open(pchtml_html_tokenizer_t *tkz,
                                      const unsigned char *data,
                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_tag_name(pchtml_html_tokenizer_t *tkz,
                                  const unsigned char *data,
                                  const unsigned char *end);

/* Attribute */
static const unsigned char *
pchtml_html_tokenizer_state_attribute_name(pchtml_html_tokenizer_t *tkz,
                                        const unsigned char *data,
                                        const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_after_attribute_name(pchtml_html_tokenizer_t *tkz,
                                              const unsigned char *data,
                                              const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_before_attribute_value(pchtml_html_tokenizer_t *tkz,
                                                const unsigned char *data,
                                                const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_attribute_value_double_quoted(pchtml_html_tokenizer_t *tkz,
                                                       const unsigned char *data,
                                                       const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_attribute_value_single_quoted(pchtml_html_tokenizer_t *tkz,
                                                       const unsigned char *data,
                                                       const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_attribute_value_unquoted(pchtml_html_tokenizer_t *tkz,
                                                  const unsigned char *data,
                                                  const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_after_attribute_value_quoted(pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_bogus_comment_before(pchtml_html_tokenizer_t *tkz,
                                              const unsigned char *data,
                                              const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_bogus_comment(pchtml_html_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end);

/* Markup declaration */
static const unsigned char *
pchtml_html_tokenizer_state_markup_declaration_open(pchtml_html_tokenizer_t *tkz,
                                                 const unsigned char *data,
                                                 const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_markup_declaration_comment(pchtml_html_tokenizer_t *tkz,
                                                    const unsigned char *data,
                                                    const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_markup_declaration_doctype(pchtml_html_tokenizer_t *tkz,
                                                    const unsigned char *data,
                                                    const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_markup_declaration_cdata(pchtml_html_tokenizer_t *tkz,
                                                  const unsigned char *data,
                                                  const unsigned char *end);

/* CDATA Section */
static const unsigned char *
pchtml_html_tokenizer_state_cdata_section_before(pchtml_html_tokenizer_t *tkz,
                                              const unsigned char *data,
                                              const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_cdata_section(pchtml_html_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_cdata_section_bracket(pchtml_html_tokenizer_t *tkz,
                                               const unsigned char *data,
                                               const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_cdata_section_end(pchtml_html_tokenizer_t *tkz,
                                           const unsigned char *data,
                                           const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_char_ref_attr(pchtml_html_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end);

static const unsigned char *
_pchtml_html_tokenizer_state_char_ref(pchtml_html_tokenizer_t *tkz,
                                   const unsigned char *data,
                                   const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_char_ref_named(pchtml_html_tokenizer_t *tkz,
                                        const unsigned char *data,
                                        const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_char_ref_ambiguous_ampersand(pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_char_ref_numeric(pchtml_html_tokenizer_t *tkz,
                                          const unsigned char *data,
                                          const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_char_ref_hexademical_start(pchtml_html_tokenizer_t *tkz,
                                                    const unsigned char *data,
                                                    const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_char_ref_decimal_start(pchtml_html_tokenizer_t *tkz,
                                                const unsigned char *data,
                                                const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_char_ref_hexademical(pchtml_html_tokenizer_t *tkz,
                                              const unsigned char *data,
                                              const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_char_ref_decimal(pchtml_html_tokenizer_t *tkz,
                                          const unsigned char *data,
                                          const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_char_ref_numeric_end(pchtml_html_tokenizer_t *tkz,
                                              const unsigned char *data,
                                              const unsigned char *end);

static size_t
pchtml_html_tokenizer_state_to_ascii_utf_8(size_t codepoint, unsigned char *data);


/*
 * Helper function. No in the specification. For 12.2.5.1 Data state
 */
const unsigned char *
pchtml_html_tokenizer_state_data_before(pchtml_html_tokenizer_t *tkz,
                                     const unsigned char *data,
                                     const unsigned char *end)
{
    UNUSED_PARAM(end);

    if (tkz->is_eof == false) {
        pchtml_html_tokenizer_state_token_set_begin(tkz, data);
    }

    /*
     * Text node init param sets before emit token.
     */

    tkz->state = pchtml_html_tokenizer_state_data;

    return data;
}

/*
 * 12.2.5.1 Data state
 */
static const unsigned char *
pchtml_html_tokenizer_state_data(pchtml_html_tokenizer_t *tkz,
                              const unsigned char *data, const unsigned char *end)
{
    pchtml_html_tokenizer_state_begin_set(tkz, data);

    while (data != end) {
        switch (*data) {
            /* U+003C LESS-THAN SIGN (<) */
            case 0x3C:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_token_set_end(tkz, data);

                tkz->state = pchtml_html_tokenizer_state_tag_open;
                return (data + 1);

            /* U+0026 AMPERSAND (&) */
            case 0x26:
                pchtml_html_tokenizer_state_append_data_m(tkz, data + 1);

                tkz->state = pchtml_html_tokenizer_state_char_ref;
                tkz->state_return = pchtml_html_tokenizer_state_data;

                return data + 1;

            /* U+000D CARRIAGE RETURN (CR) */
            case 0x0D:
                if (++data >= end) {
                    pchtml_html_tokenizer_state_append_data_m(tkz, data - 1);

                    tkz->state = pchtml_html_tokenizer_state_cr;
                    tkz->state_return = pchtml_html_tokenizer_state_data;

                    return data;
                }

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                tkz->pos[-1] = 0x0A;

                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);

                if (*data != 0x0A) {
                    pchtml_html_tokenizer_state_begin_set(tkz, data);
                    data--;
                }

                break;

            /*
             * U+0000 NULL
             * EOF
             */
            case 0x00:
                if (tkz->is_eof) {
                    /* Emit TEXT node if not empty */
                    if (tkz->token->begin != NULL) {
                        pchtml_html_tokenizer_state_token_set_end_oef(tkz);
                    }

                    if (tkz->token->begin+1 != tkz->token->end) {
                        tkz->token->tag_id = PCHTML_TAG__TEXT;

                        pchtml_html_tokenizer_state_append_data_m(tkz, data);

                        pchtml_html_tokenizer_state_set_text(tkz);
                        pchtml_html_tokenizer_state_token_done_wo_check_m(tkz,end);
                    }

                    return end;
                }

                if (SIZE_MAX - tkz->token->null_count < 1) {
                    tkz->status = PCHTML_STATUS_ERROR_OVERFLOW;
                    pcinst_set_error (PCHTML_OVERFLOW);
                    return end;
                }

                tkz->token->null_count++;

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_PARSER_TOKENIZER_ERROR_UNNUCH);
                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;
}

/*
 * Helper function. No in the specification. For 12.2.5.5 PLAINTEXT state
 */
const unsigned char *
pchtml_html_tokenizer_state_plaintext_before(pchtml_html_tokenizer_t *tkz,
                                          const unsigned char *data,
                                          const unsigned char *end)
{
    UNUSED_PARAM(end);

    if (tkz->is_eof == false) {
        pchtml_html_tokenizer_state_token_set_begin(tkz, data);
    }

    tkz->token->tag_id = PCHTML_TAG__TEXT;

    tkz->state = pchtml_html_tokenizer_state_plaintext;

    return data;
}

/*
 * 12.2.5.5 PLAINTEXT state
 */
static const unsigned char *
pchtml_html_tokenizer_state_plaintext(pchtml_html_tokenizer_t *tkz,
                                   const unsigned char *data,
                                   const unsigned char *end)
{
    pchtml_html_tokenizer_state_begin_set(tkz, data);

    while (data != end) {
        switch (*data) {
            /* U+000D CARRIAGE RETURN (CR) */
            case 0x0D:
                if (++data >= end) {
                    pchtml_html_tokenizer_state_append_data_m(tkz, data - 1);

                    tkz->state = pchtml_html_tokenizer_state_cr;
                    tkz->state_return = pchtml_html_tokenizer_state_plaintext;

                    return data;
                }

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                tkz->pos[-1] = 0x0A;

                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);

                if (*data != 0x0A) {
                    pchtml_html_tokenizer_state_begin_set(tkz, data);
                    data--;
                }

                break;

            /*
             * U+0000 NULL
             * EOF
             */
            case 0x00:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);

                if (tkz->is_eof) {
                    if (tkz->token->begin != NULL) {
                        pchtml_html_tokenizer_state_token_set_end_oef(tkz);
                    }

                    pchtml_html_tokenizer_state_set_text(tkz);
                    pchtml_html_tokenizer_state_token_done_m(tkz, end);

                    return end;
                }

                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);
                pchtml_html_tokenizer_state_append_replace_m(tkz);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_PARSER_TOKENIZER_ERROR_UNNUCH);
                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;
}

/*
 * 12.2.5.6 Tag open state
 */
static const unsigned char *
pchtml_html_tokenizer_state_tag_open(pchtml_html_tokenizer_t *tkz,
                                  const unsigned char *data, const unsigned char *end)
{
    /* ASCII alpha */
    if (pchtml_str_res_alpha_character[ *data ] != PCHTML_STR_RES_SLIP) {
        tkz->state = pchtml_html_tokenizer_state_tag_name;

        pchtml_html_tokenizer_state_token_emit_text_not_empty_m(tkz, end);
        pchtml_html_tokenizer_state_token_set_begin(tkz, data);

        return data;
    }

    /* U+002F SOLIDUS (/) */
    else if (*data == 0x2F) {
        tkz->state = pchtml_html_tokenizer_state_end_tag_open;

        return (data + 1);
    }

    /* U+0021 EXCLAMATION MARK (!) */
    else if (*data == 0x21) {
        tkz->state = pchtml_html_tokenizer_state_markup_declaration_open;

        pchtml_html_tokenizer_state_token_emit_text_not_empty_m(tkz, end);

        return (data + 1);
    }

    /* U+003F QUESTION MARK (?) */
    else if (*data == 0x3F) {
        tkz->state = pchtml_html_tokenizer_state_bogus_comment_before;

        pchtml_html_tokenizer_state_token_emit_text_not_empty_m(tkz, end);
        pchtml_html_tokenizer_state_token_set_begin(tkz, data);

        pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                     PCHTML_PARSER_TOKENIZER_ERROR_UNQUMAINOFTANA);

        return data;
    }

    /* EOF */
    else if (*data == 0x00) {
        if (tkz->is_eof) {
            pchtml_html_tokenizer_state_append_m(tkz, "<", 1);

            pchtml_html_tokenizer_state_token_set_end_oef(tkz);
            pchtml_html_tokenizer_state_token_emit_text_not_empty_m(tkz, end);

            pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->token->end,
                                         PCHTML_PARSER_TOKENIZER_ERROR_EOBETANA);

            return end;
        }
    }

    pchtml_html_tokenizer_state_append_m(tkz, "<", 1);

    pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                 PCHTML_PARSER_TOKENIZER_ERROR_INFICHOFTANA);

    tkz->state = pchtml_html_tokenizer_state_data;

    return data;
}

/*
 * 12.2.5.7 End tag open state
 */
static const unsigned char *
pchtml_html_tokenizer_state_end_tag_open(pchtml_html_tokenizer_t *tkz,
                                      const unsigned char *data,
                                      const unsigned char *end)
{
    /* ASCII alpha */
    if (pchtml_str_res_alpha_character[ *data ] != PCHTML_STR_RES_SLIP) {
        tkz->state = pchtml_html_tokenizer_state_tag_name;

        pchtml_html_tokenizer_state_token_emit_text_not_empty_m(tkz, end);
        pchtml_html_tokenizer_state_token_set_begin(tkz, data);

        tkz->token->type |= PCHTML_PARSER_TOKEN_TYPE_CLOSE;

        return data;
    }

    /* U+003E GREATER-THAN SIGN (>) */
    else if (*data == 0x3E) {
        tkz->state = pchtml_html_tokenizer_state_data;

        pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                     PCHTML_PARSER_TOKENIZER_ERROR_MIENTANA);

        return (data + 1);
    }

    /* Fake EOF */
    else if (*data == 0x00) {
        if (tkz->is_eof) {
            pchtml_html_tokenizer_state_append_m(tkz, "</", 2);

            pchtml_html_tokenizer_state_token_set_end_oef(tkz);
            pchtml_html_tokenizer_state_token_emit_text_not_empty_m(tkz, end);

            pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->token->end,
                                         PCHTML_PARSER_TOKENIZER_ERROR_EOBETANA);

            return end;
        }
    }

    tkz->state = pchtml_html_tokenizer_state_bogus_comment_before;

    pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                 PCHTML_PARSER_TOKENIZER_ERROR_INFICHOFTANA);

    pchtml_html_tokenizer_state_token_emit_text_not_empty_m(tkz, end);
    pchtml_html_tokenizer_state_token_set_begin(tkz, data);

    return data;
}

/*
 * 12.2.5.8 Tag name state
 */
static const unsigned char *
pchtml_html_tokenizer_state_tag_name(pchtml_html_tokenizer_t *tkz,
                                  const unsigned char *data, const unsigned char *end)
{
    pchtml_html_tokenizer_state_begin_set(tkz, data);

    while (data != end) {
        switch (*data) {
            /*
             * U+0009 CHARACTER TABULATION (tab)
             * U+000A LINE FEED (LF)
             * U+000C FORM FEED (FF)
             * U+000D CARRIAGE RETURN (CR)
             * U+0020 SPACE
             */
            case 0x09:
            case 0x0A:
            case 0x0C:
            case 0x0D:
            case 0x20:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_set_tag_m(tkz, tkz->start, tkz->pos);
                pchtml_html_tokenizer_state_token_set_end(tkz, data);

                tkz->state = pchtml_html_tokenizer_state_before_attribute_name;
                return (data + 1);

            /* U+002F SOLIDUS (/) */
            case 0x2F:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_set_tag_m(tkz, tkz->start, tkz->pos);
                pchtml_html_tokenizer_state_token_set_end(tkz, data);

                tkz->state = pchtml_html_tokenizer_state_self_closing_start_tag;
                return (data + 1);

            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                tkz->state = pchtml_html_tokenizer_state_data_before;

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_set_tag_m(tkz, tkz->start, tkz->pos);
                pchtml_html_tokenizer_state_token_set_end(tkz, data);
                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return (data + 1);

            /* U+0000 NULL */
            case 0x00:
                if (tkz->is_eof) {
                    pchtml_html_tokenizer_state_token_set_end_oef(tkz);

                    pchtml_html_tokenizer_error_add(tkz->parse_errors,
                                               tkz->token->end,
                                               PCHTML_PARSER_TOKENIZER_ERROR_EOINTA);
                    return end;
                }

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);
                pchtml_html_tokenizer_state_append_replace_m(tkz);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_PARSER_TOKENIZER_ERROR_UNNUCH);
                break;

            default:
                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;
}

/*
 * 12.2.5.32 Before attribute name state
 */
const unsigned char *
pchtml_html_tokenizer_state_before_attribute_name(pchtml_html_tokenizer_t *tkz,
                                               const unsigned char *data,
                                               const unsigned char *end)
{
    pchtml_html_token_attr_t *attr;

    while (data != end) {
        switch (*data) {
            /*
             * U+0009 CHARACTER TABULATION (tab)
             * U+000A LINE FEED (LF)
             * U+000C FORM FEED (FF)
             * U+000D CARRIAGE RETURN (CR)
             * U+0020 SPACE
             */
            case 0x09:
            case 0x0A:
            case 0x0C:
            case 0x0D:
            case 0x20:
                break;

            /* U+003D EQUALS SIGN (=) */
            case 0x3D:
                pchtml_html_tokenizer_state_token_attr_add_m(tkz, attr, end);
                pchtml_html_tokenizer_state_token_attr_set_name_begin(tkz, data);

                pchtml_html_tokenizer_state_append_m(tkz, data, 1);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_PARSER_TOKENIZER_ERROR_UNEQSIBEATNA);

                tkz->state = pchtml_html_tokenizer_state_attribute_name;
                return (data + 1);

            /*
             * U+002F SOLIDUS (/)
             * U+003E GREATER-THAN SIGN (>)
             */
            case 0x2F:
            case 0x3E:
                tkz->state = pchtml_html_tokenizer_state_after_attribute_name;
                return data;

            /* EOF */
            case 0x00:
                if (tkz->is_eof) {
                    tkz->state = pchtml_html_tokenizer_state_after_attribute_name;
                    return data;
                }
                /* fall through */

            /* Anything else */
            default:
                pchtml_html_tokenizer_state_token_attr_add_m(tkz, attr, end);
                pchtml_html_tokenizer_state_token_attr_set_name_begin(tkz, data);

                tkz->state = pchtml_html_tokenizer_state_attribute_name;
                return data;
        }

        data++;
    }

    return data;
}

/*
 * 12.2.5.33 Attribute name state
 */
static const unsigned char *
pchtml_html_tokenizer_state_attribute_name(pchtml_html_tokenizer_t *tkz,
                                        const unsigned char *data,
                                        const unsigned char *end)
{
    pchtml_html_tokenizer_state_begin_set(tkz, data);

    while (data != end) {
        switch (*data) {
            /*
             * U+0009 CHARACTER TABULATION (tab)
             * U+000A LINE FEED (LF)
             * U+000C FORM FEED (FF)
             * U+000D CARRIAGE RETURN (CR)
             * U+0020 SPACE
             * U+002F SOLIDUS (/)
             * U+003E GREATER-THAN SIGN (>)
             */
            case 0x09:
            case 0x0A:
            case 0x0C:
            case 0x0D:
            case 0x20:
            case 0x2F:
            case 0x3E:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_set_name_m(tkz);
                pchtml_html_tokenizer_state_token_attr_set_name_end(tkz, data);

                tkz->state = pchtml_html_tokenizer_state_after_attribute_name;
                return data;

            /*
             * U+0000 NULL
             * EOF
             */
            case 0x00:
                if (tkz->is_eof) {
                    pchtml_html_tokenizer_state_token_attr_set_name_end_oef(tkz);

                    tkz->state = pchtml_html_tokenizer_state_after_attribute_name;
                    return data;
                }

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);
                pchtml_html_tokenizer_state_append_replace_m(tkz);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_PARSER_TOKENIZER_ERROR_UNNUCH);
                break;

            /* U+003D EQUALS SIGN (=) */
            case 0x3D:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_set_name_m(tkz);
                pchtml_html_tokenizer_state_token_attr_set_name_end(tkz, data);

                tkz->state = pchtml_html_tokenizer_state_before_attribute_value;
                return (data + 1);

            /*
             * U+0022 QUOTATION MARK (")
             * U+0027 APOSTROPHE (')
             * U+003C LESS-THAN SIGN (<)
             */
            case 0x22:
            case 0x27:
            case 0x3C:
                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                           PCHTML_PARSER_TOKENIZER_ERROR_UNCHINATNA);
                break;

            default:
                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;
}

/*
 * 12.2.5.34 After attribute name state
 */
static const unsigned char *
pchtml_html_tokenizer_state_after_attribute_name(pchtml_html_tokenizer_t *tkz,
                                              const unsigned char *data,
                                              const unsigned char *end)
{
    pchtml_html_token_attr_t *attr;

    while (data != end) {
        switch (*data) {
            /*
             * U+0009 CHARACTER TABULATION (tab)
             * U+000A LINE FEED (LF)
             * U+000C FORM FEED (FF)
             * U+000D CARRIAGE RETURN (CR)
             * U+0020 SPACE
             */
            case 0x09:
            case 0x0A:
            case 0x0C:
            case 0x0D:
            case 0x20:
                break;

            /* U+002F SOLIDUS (/) */
            case 0x2F:
                tkz->state = pchtml_html_tokenizer_state_self_closing_start_tag;
                return (data + 1);

            /* U+003D EQUALS SIGN (=) */
            case 0x3D:
                tkz->state = pchtml_html_tokenizer_state_before_attribute_value;
                return (data + 1);

            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                tkz->state = pchtml_html_tokenizer_state_data_before;

                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return (data + 1);

            case 0x00:
                if (tkz->is_eof) {
                    pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                               PCHTML_PARSER_TOKENIZER_ERROR_EOINTA);
                    return end;
                }
                /* fall through */

            default:
                pchtml_html_tokenizer_state_token_attr_add_m(tkz, attr, end);
                pchtml_html_tokenizer_state_token_attr_set_name_begin(tkz, data);

                tkz->state = pchtml_html_tokenizer_state_attribute_name;
                return data;
        }

        data++;
    }

    return data;
}

/*
 * 12.2.5.35 Before attribute value state
 */
static const unsigned char *
pchtml_html_tokenizer_state_before_attribute_value(pchtml_html_tokenizer_t *tkz,
                                                const unsigned char *data,
                                                const unsigned char *end)
{
    while (data != end) {
        switch (*data) {
            /*
             * U+0009 CHARACTER TABULATION (tab)
             * U+000A LINE FEED (LF)
             * U+000C FORM FEED (FF)
             * U+000D CARRIAGE RETURN (CR)
             * U+0020 SPACE
             */
            case 0x09:
            case 0x0A:
            case 0x0C:
            case 0x0D:
            case 0x20:
                break;

            /* U+0022 QUOTATION MARK (") */
            case 0x22:
                tkz->state =
                    pchtml_html_tokenizer_state_attribute_value_double_quoted;

                return (data + 1);

            /* U+0027 APOSTROPHE (') */
            case 0x27:
                tkz->state =
                    pchtml_html_tokenizer_state_attribute_value_single_quoted;

                return (data + 1);

            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                tkz->state = pchtml_html_tokenizer_state_data_before;

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_PARSER_TOKENIZER_ERROR_MIATVA);

                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return (data + 1);

            default:
                tkz->state = pchtml_html_tokenizer_state_attribute_value_unquoted;
                return data;
        }

        data++;
    }

    return data;
}

/*
 * 12.2.5.36 Attribute value (double-quoted) state
 */
static const unsigned char *
pchtml_html_tokenizer_state_attribute_value_double_quoted(pchtml_html_tokenizer_t *tkz,
                                                       const unsigned char *data,
                                                       const unsigned char *end)
{
    if (tkz->token->attr_last->value_begin == NULL && tkz->is_eof == false) {
        pchtml_html_tokenizer_state_token_attr_set_value_begin(tkz, data);
    }

    pchtml_html_tokenizer_state_begin_set(tkz, data);

    while (data != end) {
        switch (*data) {
            /* U+0022 QUOTATION MARK (") */
            case 0x22:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_token_attr_set_value_end(tkz, data);
                pchtml_html_tokenizer_state_set_value_m(tkz);

                tkz->state =
                    pchtml_html_tokenizer_state_after_attribute_value_quoted;

                return (data + 1);

            /* U+0026 AMPERSAND (&) */
            case 0x26:
                pchtml_html_tokenizer_state_append_data_m(tkz, data + 1);

                tkz->state = pchtml_html_tokenizer_state_char_ref_attr;
                tkz->state_return = pchtml_html_tokenizer_state_attribute_value_double_quoted;

                return data + 1;

            /* U+000D CARRIAGE RETURN (CR) */
            case 0x0D:
                if (++data >= end) {
                    pchtml_html_tokenizer_state_append_data_m(tkz, data - 1);

                    tkz->state = pchtml_html_tokenizer_state_cr;
                    tkz->state_return = pchtml_html_tokenizer_state_attribute_value_double_quoted;

                    return data;
                }

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                tkz->pos[-1] = 0x0A;

                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);

                if (*data != 0x0A) {
                    pchtml_html_tokenizer_state_begin_set(tkz, data);
                    data--;
                }

                break;

            /*
             * U+0000 NULL
             * EOF
             */
            case 0x00:
                if (tkz->is_eof) {
                    if (tkz->token->attr_last->value_begin != NULL) {
                     pchtml_html_tokenizer_state_token_attr_set_value_end_oef(tkz);
                    }

                    pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                               PCHTML_PARSER_TOKENIZER_ERROR_EOINTA);
                    return end;
                }

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);
                pchtml_html_tokenizer_state_append_replace_m(tkz);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_PARSER_TOKENIZER_ERROR_UNNUCH);
                break;

            default:
                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;
}

/*
 * 12.2.5.37 Attribute value (single-quoted) state
 */
static const unsigned char *
pchtml_html_tokenizer_state_attribute_value_single_quoted(pchtml_html_tokenizer_t *tkz,
                                                       const unsigned char *data,
                                                       const unsigned char *end)
{
    if (tkz->token->attr_last->value_begin == NULL && tkz->is_eof == false) {
        pchtml_html_tokenizer_state_token_attr_set_value_begin(tkz, data);
    }

    pchtml_html_tokenizer_state_begin_set(tkz, data);

    while (data != end) {
        switch (*data) {
            /* U+0027 APOSTROPHE (') */
            case 0x27:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_token_attr_set_value_end(tkz, data);
                pchtml_html_tokenizer_state_set_value_m(tkz);

                tkz->state =
                    pchtml_html_tokenizer_state_after_attribute_value_quoted;

                return (data + 1);

            /* U+0026 AMPERSAND (&) */
            case 0x26:
                pchtml_html_tokenizer_state_append_data_m(tkz, data + 1);

                tkz->state = pchtml_html_tokenizer_state_char_ref_attr;
                tkz->state_return = pchtml_html_tokenizer_state_attribute_value_single_quoted;

                return data + 1;

            /* U+000D CARRIAGE RETURN (CR) */
            case 0x0D:
                if (++data >= end) {
                    pchtml_html_tokenizer_state_append_data_m(tkz, data - 1);

                    tkz->state = pchtml_html_tokenizer_state_cr;
                    tkz->state_return = pchtml_html_tokenizer_state_attribute_value_single_quoted;

                    return data;
                }

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                tkz->pos[-1] = 0x0A;

                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);

                if (*data != 0x0A) {
                    pchtml_html_tokenizer_state_begin_set(tkz, data);
                    data--;
                }

                break;

            /*
             * U+0000 NULL
             * EOF
             */
            case 0x00:
                if (tkz->is_eof) {
                    if (tkz->token->attr_last->value_begin != NULL) {
                     pchtml_html_tokenizer_state_token_attr_set_value_end_oef(tkz);
                    }

                    pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                               PCHTML_PARSER_TOKENIZER_ERROR_EOINTA);
                    return end;
                }

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);
                pchtml_html_tokenizer_state_append_replace_m(tkz);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_PARSER_TOKENIZER_ERROR_UNNUCH);
                break;

            default:
                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;
}

/*
 * 12.2.5.38 Attribute value (unquoted) state
 */
static const unsigned char *
pchtml_html_tokenizer_state_attribute_value_unquoted(pchtml_html_tokenizer_t *tkz,
                                                  const unsigned char *data,
                                                  const unsigned char *end)
{
    if (tkz->token->attr_last->value_begin == NULL && tkz->is_eof == false) {
        pchtml_html_tokenizer_state_token_attr_set_value_begin(tkz, data);
    }

    pchtml_html_tokenizer_state_begin_set(tkz, data);

    while (data != end) {
        switch (*data) {
           /*
            * U+0009 CHARACTER TABULATION (tab)
            * U+000A LINE FEED (LF)
            * U+000C FORM FEED (FF)
            * U+000D CARRIAGE RETURN (CR)
            * U+0020 SPACE
            */
            case 0x09:
            case 0x0A:
            case 0x0C:
            case 0x0D:
            case 0x20:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_token_attr_set_value_end(tkz, data);
                pchtml_html_tokenizer_state_set_value_m(tkz);

                tkz->state = pchtml_html_tokenizer_state_before_attribute_name;
                return (data + 1);

            /* U+0026 AMPERSAND (&) */
            case 0x26:
                pchtml_html_tokenizer_state_append_data_m(tkz, data + 1);

                tkz->state = pchtml_html_tokenizer_state_char_ref_attr;
                tkz->state_return = pchtml_html_tokenizer_state_attribute_value_unquoted;

                return data + 1;

            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                tkz->state = pchtml_html_tokenizer_state_data_before;

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_token_attr_set_value_end(tkz, data);
                pchtml_html_tokenizer_state_set_value_m(tkz);

                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return (data + 1);

            /*
             * U+0000 NULL
             * EOF
             */
            case 0x00:
                if (tkz->is_eof) {
                    if (tkz->token->attr_last->value_begin != NULL) {
                     pchtml_html_tokenizer_state_token_attr_set_value_end_oef(tkz);
                    }

                    pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                                 PCHTML_PARSER_TOKENIZER_ERROR_EOINTA);
                    return end;
                }

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);
                pchtml_html_tokenizer_state_append_replace_m(tkz);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_PARSER_TOKENIZER_ERROR_UNNUCH);
                break;

            /*
             * U+0022 QUOTATION MARK (")
             * U+0027 APOSTROPHE (')
             * U+003C LESS-THAN SIGN (<)
             * U+003D EQUALS SIGN (=)
             * U+0060 GRAVE ACCENT (`)
             */
            case 0x22:
            case 0x27:
            case 0x3C:
            case 0x3D:
            case 0x60:
                pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->token->end,
                                             PCHTML_PARSER_TOKENIZER_ERROR_UNCHINUNATVA);
                break;

            default:
                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;
}

/*
 * 12.2.5.39 After attribute value (quoted) state
 */
static const unsigned char *
pchtml_html_tokenizer_state_after_attribute_value_quoted(pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end)
{
    switch (*data) {
        /*
         * U+0009 CHARACTER TABULATION (tab)
         * U+000A LINE FEED (LF)
         * U+000C FORM FEED (FF)
         * U+000D CARRIAGE RETURN (CR)
         * U+0020 SPACE
         */
        case 0x09:
        case 0x0A:
        case 0x0C:
        case 0x0D:
        case 0x20:
            tkz->state = pchtml_html_tokenizer_state_before_attribute_name;

            return (data + 1);

        /* U+002F SOLIDUS (/) */
        case 0x2F:
            tkz->state = pchtml_html_tokenizer_state_self_closing_start_tag;

            return (data + 1);

        /* U+003E GREATER-THAN SIGN (>) */
        case 0x3E:
            tkz->state = pchtml_html_tokenizer_state_data_before;

            pchtml_html_tokenizer_state_token_done_m(tkz, end);

            return (data + 1);

        /* EOF */
        case 0x00:
            if (tkz->is_eof) {
                pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                             PCHTML_PARSER_TOKENIZER_ERROR_EOINTA);
                return end;
            }
            /* fall through */

        default:
            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_PARSER_TOKENIZER_ERROR_MIWHBEAT);

            tkz->state = pchtml_html_tokenizer_state_before_attribute_name;

            return data;
    }

    return data;
}


const unsigned char *
pchtml_html_tokenizer_state_cr(pchtml_html_tokenizer_t *tkz, const unsigned char *data,
                            const unsigned char *end)
{
    pchtml_html_tokenizer_state_append_m(tkz, "\n", 1);

    if (*data == 0x0A) {
        data++;
    }

    tkz->state = tkz->state_return;

    return data;
}

/*
 * 12.2.5.40 Self-closing start tag state
 */
const unsigned char *
pchtml_html_tokenizer_state_self_closing_start_tag(pchtml_html_tokenizer_t *tkz,
                                                const unsigned char *data,
                                                const unsigned char *end)
{
    switch (*data) {
        /* U+003E GREATER-THAN SIGN (>) */
        case 0x3E:
            tkz->state = pchtml_html_tokenizer_state_data_before;
            tkz->token->type |= PCHTML_PARSER_TOKEN_TYPE_CLOSE_SELF;

            pchtml_html_tokenizer_state_token_done_m(tkz, end);

            return (data + 1);

        /* EOF */
        case 0x00:
            if (tkz->is_eof) {
                pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->token->end,
                                             PCHTML_PARSER_TOKENIZER_ERROR_EOINTA);
                return end;
            }
            /* fall through */

        default:
            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_PARSER_TOKENIZER_ERROR_UNSOINTA);

            tkz->state = pchtml_html_tokenizer_state_before_attribute_name;

            return data;
    }

    return data;
}

/*
 * Helper function. No in the specification. For 12.2.5.41 Bogus comment state
 */
static const unsigned char *
pchtml_html_tokenizer_state_bogus_comment_before(pchtml_html_tokenizer_t *tkz,
                                              const unsigned char *data,
                                              const unsigned char *end)
{
    UNUSED_PARAM(end);

    tkz->token->tag_id = PCHTML_TAG__EM_COMMENT;

    tkz->state = pchtml_html_tokenizer_state_bogus_comment;

    return data;
}

/*
 * 12.2.5.41 Bogus comment state
 */
static const unsigned char *
pchtml_html_tokenizer_state_bogus_comment(pchtml_html_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end)
{
    pchtml_html_tokenizer_state_begin_set(tkz, data);

    while (data != end) {
        switch (*data) {
            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                tkz->state = pchtml_html_tokenizer_state_data_before;

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_token_set_end(tkz, data);
                pchtml_html_tokenizer_state_set_text(tkz);
                pchtml_html_tokenizer_state_token_done_wo_check_m(tkz, end);

                return (data + 1);

            /* U+000D CARRIAGE RETURN (CR) */
            case 0x0D:
                if (++data >= end) {
                    pchtml_html_tokenizer_state_append_data_m(tkz, data - 1);

                    tkz->state = pchtml_html_tokenizer_state_cr;
                    tkz->state_return = pchtml_html_tokenizer_state_bogus_comment;

                    return data;
                }

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                tkz->pos[-1] = 0x0A;

                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);

                if (*data != 0x0A) {
                    pchtml_html_tokenizer_state_begin_set(tkz, data);
                    data--;
                }

                break;

            /*
             * EOF
             * U+0000 NULL
             */
            case 0x00:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);

                if (tkz->is_eof) {
                    if (tkz->token->begin != NULL) {
                        pchtml_html_tokenizer_state_token_set_end_oef(tkz);
                    }

                    pchtml_html_tokenizer_state_set_text(tkz);
                    pchtml_html_tokenizer_state_token_done_wo_check_m(tkz, end);

                    return end;
                }

                pchtml_html_tokenizer_state_append_replace_m(tkz);
                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_PARSER_TOKENIZER_ERROR_UNNUCH);
                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;
}

/*
 * 12.2.5.42 Markup declaration open state
 */
static const unsigned char *
pchtml_html_tokenizer_state_markup_declaration_open(pchtml_html_tokenizer_t *tkz,
                                                 const unsigned char *data,
                                                 const unsigned char *end)
{
    /* Check first char for change parse state */
    if (tkz->is_eof == false) {
        pchtml_html_tokenizer_state_token_set_begin(tkz, data);
    }

    /* U+002D HYPHEN-MINUS characters (-) */
    if (*data == 0x2D) {
        if ((end - data) < 2) {
            tkz->state = pchtml_html_tokenizer_state_markup_declaration_comment;
            return (data + 1);
        }

        if (data[1] == 0x2D) {
            tkz->state = pchtml_html_tokenizer_state_comment_before_start;
            return (data + 2);
        }
    }
    /*
     * ASCII case-insensitive match for the word "DOCTYPE"
     * U+0044 character (D) or U+0064 character (d)
     */
    else if (*data == 0x44 || *data == 0x64) {
        if ((end - data) < 7) {
            tkz->markup = (unsigned char *) "doctype";

            tkz->state = pchtml_html_tokenizer_state_markup_declaration_doctype;
            return data;
        }

        if (pchtml_str_data_ncasecmp((unsigned char *) "doctype", data, 7)) {
            tkz->state = pchtml_html_tokenizer_state_doctype_before;
            return (data + 7);
        }
    }
    /* Case-sensitive match for the string "[CDATA["
     * (the five uppercase letters "CDATA" with a U+005B LEFT SQUARE BRACKET
     * character before and after)
     */
    else if (*data == 0x5B) {
        if ((end - data) < 7) {
            tkz->markup = (unsigned char *) "[CDATA[";

            tkz->state = pchtml_html_tokenizer_state_markup_declaration_cdata;
            return data;
        }

        if (pchtml_str_data_ncmp((unsigned char *) "[CDATA[", data, 7)) {
            pchtml_ns_id_t ns = pchtml_html_tokenizer_current_namespace(tkz);

            if (ns != PCHTML_NS_HTML && ns != PCHTML_NS__UNDEF) {
                data += 7;

                pchtml_html_tokenizer_state_token_set_begin(tkz, data);

                tkz->state = pchtml_html_tokenizer_state_cdata_section_before;

                return data;
            }

            tkz->state = pchtml_html_tokenizer_state_bogus_comment_before;

            return data;
        }
    }

    if (tkz->is_eof) {
        pchtml_html_tokenizer_state_token_set_end_oef(tkz);

        tkz->token->begin = tkz->token->end;
    }

    pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                 PCHTML_PARSER_TOKENIZER_ERROR_INOPCO);

    tkz->state = pchtml_html_tokenizer_state_bogus_comment_before;

    return data;
}

/*
 * Helper function. No in the specification. For 12.2.5.42
 * For a comment tag <!--
 */
static const unsigned char *
pchtml_html_tokenizer_state_markup_declaration_comment(pchtml_html_tokenizer_t *tkz,
                                                    const unsigned char *data,
                                                    const unsigned char *end)
{
    UNUSED_PARAM(end);

    /* U+002D HYPHEN-MINUS characters (-) */
    if (*data == 0x2D) {
        tkz->state = pchtml_html_tokenizer_state_comment_before_start;
        return (data + 1);
    }

    pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                 PCHTML_PARSER_TOKENIZER_ERROR_INOPCO);

    tkz->state = pchtml_html_tokenizer_state_bogus_comment_before;
    return data;
}

/*
 * Helper function. No in the specification. For 12.2.5.42
 * For a DOCTYPE tag <!DOCTYPE
 */
static const unsigned char *
pchtml_html_tokenizer_state_markup_declaration_doctype(pchtml_html_tokenizer_t *tkz,
                                                    const unsigned char *data,
                                                    const unsigned char *end)
{
    const unsigned char *pos;
    pos = pchtml_str_data_ncasecmp_first(tkz->markup, data, (end - data));

    if (pos == NULL) {
        pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                     PCHTML_PARSER_TOKENIZER_ERROR_INOPCO);

        tkz->state = pchtml_html_tokenizer_state_bogus_comment_before;
        return data;
    }

    if (*pos == '\0') {
        data = (data + (pos - tkz->markup));

        tkz->state = pchtml_html_tokenizer_state_doctype_before;
        return data;
    }

    tkz->markup = pos;

    return end;
}

/*
 * Helper function. No in the specification. For 12.2.5.42
 * For a CDATA tag <![CDATA[
 */
static const unsigned char *
pchtml_html_tokenizer_state_markup_declaration_cdata(pchtml_html_tokenizer_t *tkz,
                                                  const unsigned char *data,
                                                  const unsigned char *end)
{
    const unsigned char *pos;
    pos = pchtml_str_data_ncasecmp_first(tkz->markup, data, (end - data));

    if (pos == NULL) {
        pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                     PCHTML_PARSER_TOKENIZER_ERROR_INOPCO);

        tkz->state = pchtml_html_tokenizer_state_bogus_comment_before;
        return data;
    }

    if (*pos == '\0') {
        pchtml_ns_id_t ns = pchtml_html_tokenizer_current_namespace(tkz);

        if (ns != PCHTML_NS_HTML && ns != PCHTML_NS__UNDEF) {
            data = (data + (pos - tkz->markup));

            tkz->state = pchtml_html_tokenizer_state_cdata_section_before;
            return data;
        }

        pchtml_html_tokenizer_state_append_m(tkz, "[CDATA", 6);

        tkz->state = pchtml_html_tokenizer_state_bogus_comment_before;
        return data;
    }

    tkz->markup = pos;

    return end;
}

/*
 * Helper function. No in the specification. For 12.2.5.69
 */
static const unsigned char *
pchtml_html_tokenizer_state_cdata_section_before(pchtml_html_tokenizer_t *tkz,
                                              const unsigned char *data,
                                              const unsigned char *end)
{
    UNUSED_PARAM(end);

    if (tkz->is_eof == false) {
        pchtml_html_tokenizer_state_token_set_begin(tkz, data);
    }
    else {
        pchtml_html_tokenizer_state_token_set_begin(tkz, tkz->last);
    }

    tkz->token->tag_id = PCHTML_TAG__TEXT;

    tkz->state = pchtml_html_tokenizer_state_cdata_section;

    return data;
}

/*
 * 12.2.5.69 CDATA section state
 */
static const unsigned char *
pchtml_html_tokenizer_state_cdata_section(pchtml_html_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end)
{
    pchtml_html_tokenizer_state_begin_set(tkz, data);

    while (data != end) {
        switch (*data) {
            /* U+005D RIGHT SQUARE BRACKET (]) */
            case 0x5D:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_token_set_end(tkz, data);

                tkz->state = pchtml_html_tokenizer_state_cdata_section_bracket;
                return (data + 1);

            /* U+000D CARRIAGE RETURN (CR) */
            case 0x0D:
                if (++data >= end) {
                    pchtml_html_tokenizer_state_append_data_m(tkz, data - 1);

                    tkz->state = pchtml_html_tokenizer_state_cr;
                    tkz->state_return = pchtml_html_tokenizer_state_cdata_section;

                    return data;
                }

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                tkz->pos[-1] = 0x0A;

                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);

                if (*data != 0x0A) {
                    pchtml_html_tokenizer_state_begin_set(tkz, data);
                    data--;
                }

                break;

            /* EOF */
            case 0x00:
                if (tkz->is_eof) {
                    pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                                 PCHTML_PARSER_TOKENIZER_ERROR_EOINCD);

                    if (tkz->token->begin != NULL) {
                        pchtml_html_tokenizer_state_append_data_m(tkz, data);
                        pchtml_html_tokenizer_state_token_set_end_oef(tkz);
                    }

                    pchtml_html_tokenizer_state_set_text(tkz);
                    pchtml_html_tokenizer_state_token_done_m(tkz, end);

                    return end;
                }

                if (SIZE_MAX - tkz->token->null_count < 1) {
                    pcinst_set_error (PCHTML_OVERFLOW);
                    tkz->status = PCHTML_STATUS_ERROR_OVERFLOW;
                    return end;
                }

                tkz->token->null_count++;

                break;

            default:
                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;
}

/*
 * 12.2.5.70 CDATA section bracket state
 */
static const unsigned char *
pchtml_html_tokenizer_state_cdata_section_bracket(pchtml_html_tokenizer_t *tkz,
                                               const unsigned char *data,
                                               const unsigned char *end)
{
    /* U+005D RIGHT SQUARE BRACKET (]) */
    if (*data == 0x5D) {
        tkz->state = pchtml_html_tokenizer_state_cdata_section_end;
        return (data + 1);
    }

    pchtml_html_tokenizer_state_append_m(tkz, "]", 1);

    tkz->state = pchtml_html_tokenizer_state_cdata_section;

    return data;
}

/*
 * 12.2.5.71 CDATA section end state
 */
static const unsigned char *
pchtml_html_tokenizer_state_cdata_section_end(pchtml_html_tokenizer_t *tkz,
                                           const unsigned char *data,
                                           const unsigned char *end)
{
    /* U+005D RIGHT SQUARE BRACKET (]) */
    if (*data == 0x5D) {
        pchtml_html_tokenizer_state_append_m(tkz, data, 1);
        return (data + 1);
    }
    /* U+003E GREATER-THAN SIGN character */
    else if (*data == 0x3E) {
        tkz->state = pchtml_html_tokenizer_state_data_before;

        pchtml_html_tokenizer_state_set_text(tkz);
        pchtml_html_tokenizer_state_token_done_m(tkz, end);

        return (data + 1);
    }

    pchtml_html_tokenizer_state_append_m(tkz, "]]", 2);

    tkz->state = pchtml_html_tokenizer_state_cdata_section;

    return data;
}

/*
 * 12.2.5.72 Character reference state
 */
const unsigned char *
pchtml_html_tokenizer_state_char_ref(pchtml_html_tokenizer_t *tkz,
                                  const unsigned char *data, const unsigned char *end)
{
    tkz->is_attribute = false;

    return _pchtml_html_tokenizer_state_char_ref(tkz, data, end);
}

static const unsigned char *
pchtml_html_tokenizer_state_char_ref_attr(pchtml_html_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end)
{
    tkz->is_attribute = true;

    return _pchtml_html_tokenizer_state_char_ref(tkz, data, end);
}

static const unsigned char *
_pchtml_html_tokenizer_state_char_ref(pchtml_html_tokenizer_t *tkz,
                                   const unsigned char *data,
                                   const unsigned char *end)
{
    /* ASCII alphanumeric */
    if (pchtml_str_res_alphanumeric_character[ *data ] != PCHTML_STR_RES_SLIP) {
        tkz->entity = &pchtml_html_tokenizer_res_entities_sbst[1];
        tkz->entity_match = NULL;
        tkz->entity_start = (tkz->pos - 1) - tkz->start;

        tkz->state = pchtml_html_tokenizer_state_char_ref_named;

        return data;
    }
    /* U+0023 NUMBER SIGN (#) */
    else if (*data == 0x23) {
        tkz->markup = data;
        tkz->entity_start = (tkz->pos - 1) - tkz->start;

        pchtml_html_tokenizer_state_append_m(tkz, data, 1);

        tkz->state = pchtml_html_tokenizer_state_char_ref_numeric;

        return (data + 1);
    }
    else {
        tkz->state = tkz->state_return;
    }

    return data;
}

/*
 * 12.2.5.73 Named character reference state
 *
 * The slowest part in HTML parsing!!!
 *
 * This option works correctly and passes all tests (stream parsing too).
 * We must seriously think about how to accelerate this part.
 */
static const unsigned char *
pchtml_html_tokenizer_state_char_ref_named(pchtml_html_tokenizer_t *tkz,
                                        const unsigned char *data,
                                        const unsigned char *end)
{
    size_t size, tail_size;
    unsigned char *start;
    const pchtml_sbst_entry_static_t *entry = tkz->entity;

    const unsigned char *begin = data;

    while (data < end) {
        entry = pchtml_sbst_entry_static_find(pchtml_html_tokenizer_res_entities_sbst,
                                              entry, *data);
        if (entry == NULL) {
            pchtml_html_tokenizer_state_append_m(tkz, begin, (data - begin));
            goto done;
        }

        if (entry->value != NULL) {
            tkz->entity_end = (tkz->pos + (data - begin)) - tkz->start;
            tkz->entity_match = entry;
        }

        entry = &pchtml_html_tokenizer_res_entities_sbst[ entry->next ];

        data++;
    }

    /* If entry not NULL and buffer empty, then wait next buffer. */
    tkz->entity = entry;

    pchtml_html_tokenizer_state_append_m(tkz, begin, (end - begin));
    return data;

done:

    /* If we have bad entity */
    if (tkz->entity_match == NULL) {
        tkz->state = pchtml_html_tokenizer_state_char_ref_ambiguous_ampersand;

        return data;
    }

    tkz->state = tkz->state_return;

    /*
     * If the character reference was consumed as part of an attribute,
     * and the last character matched is not a U+003B SEMICOLON character (;),
     * and the next input character is either a U+003D EQUALS SIGN character (=)
     * or an ASCII alphanumeric, then, for historical reasons,
     * flush code points consumed as a character reference
     * and switch to the return state.
     */
    /* U+003B SEMICOLON character (;) */
    if (tkz->is_attribute && tkz->entity_match->key != 0x3B) {
        /* U+003D EQUALS SIGN character (=) or ASCII alphanumeric */
        if (*data == 0x3D
            || pchtml_str_res_alphanumeric_character[*data] != PCHTML_STR_RES_SLIP)
        {
            return data;
        }
    }

    if (tkz->entity_match->key != 0x3B) {
        pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                     PCHTML_PARSER_TOKENIZER_ERROR_MISEAFCHRE);
    }

    start = &tkz->start[tkz->entity_start];

    size = tkz->pos - start;
    tail_size = tkz->pos - &tkz->start[tkz->entity_end] - 1;

    if (tail_size != 0) {
        if ((size + tail_size) + start > tkz->end) {
            if(pchtml_html_tokenizer_temp_realloc(tkz, size)) {
                return end;
            }
        }

        memmove(start + tkz->entity_match->value_len,
                tkz->pos - tail_size, tail_size);
    }

    memcpy(start, tkz->entity_match->value, tkz->entity_match->value_len);

    tkz->pos = start + (tkz->entity_match->value_len + tail_size);

    return data;
}

/*
 * 12.2.5.74 Ambiguous ampersand state
 */
static const unsigned char *
pchtml_html_tokenizer_state_char_ref_ambiguous_ampersand(pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end)
{
    UNUSED_PARAM(end);

    /* ASCII alphanumeric */
    /* Skipped, not need */

    /* U+003B SEMICOLON (;) */
    if (*data == 0x3B) {
        pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                     PCHTML_PARSER_TOKENIZER_ERROR_UNNACHRE);
    }

    tkz->state = tkz->state_return;

    return data;
}

/*
 * 12.2.5.75 Numeric character reference state
 */
static const unsigned char *
pchtml_html_tokenizer_state_char_ref_numeric(pchtml_html_tokenizer_t *tkz,
                                          const unsigned char *data,
                                          const unsigned char *end)
{
    tkz->entity_number = 0;

    /*
     * U+0078 LATIN SMALL LETTER X
     * U+0058 LATIN CAPITAL LETTER X
     */
    if (*data == 0x78 || *data == 0x58) {
        pchtml_html_tokenizer_state_append_m(tkz, data, 1);

        tkz->state = pchtml_html_tokenizer_state_char_ref_hexademical_start;

        return (data + 1);
    }

    tkz->state = pchtml_html_tokenizer_state_char_ref_decimal_start;

    return data;
}

/*
 * 12.2.5.76 Hexademical character reference start state
 */
static const unsigned char *
pchtml_html_tokenizer_state_char_ref_hexademical_start(pchtml_html_tokenizer_t *tkz,
                                                    const unsigned char *data,
                                                    const unsigned char *end)
{
    UNUSED_PARAM(end);

    /* ASCII hex digit */
    if (pchtml_str_res_map_hex[ *data ] != PCHTML_STR_RES_SLIP) {
        tkz->state = pchtml_html_tokenizer_state_char_ref_hexademical;
    }
    else {
        pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                     PCHTML_PARSER_TOKENIZER_ERROR_ABOFDIINNUCHRE);

        tkz->state = tkz->state_return;
    }

    return data;
}

/*
 * 12.2.5.77 Decimal character reference start state
 */
static const unsigned char *
pchtml_html_tokenizer_state_char_ref_decimal_start(pchtml_html_tokenizer_t *tkz,
                                                const unsigned char *data,
                                                const unsigned char *end)
{
    UNUSED_PARAM(end);

    /* ASCII digit */
    if (pchtml_str_res_map_num[ *data ] != PCHTML_STR_RES_SLIP) {
        tkz->state = pchtml_html_tokenizer_state_char_ref_decimal;
    }
    else {
        pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                     PCHTML_PARSER_TOKENIZER_ERROR_ABOFDIINNUCHRE);

        tkz->state = tkz->state_return;
    }

    return data;
}

/*
 * 12.2.5.78 Hexademical character reference state
 */
static const unsigned char *
pchtml_html_tokenizer_state_char_ref_hexademical(pchtml_html_tokenizer_t *tkz,
                                              const unsigned char *data,
                                              const unsigned char *end)
{
    while (data != end) {
        if (pchtml_str_res_map_hex[ *data ] == PCHTML_STR_RES_SLIP) {
            tkz->state = tkz->state_return;

            if (*data == ';') {
                data++;
            }

            return pchtml_html_tokenizer_state_char_ref_numeric_end(tkz, data, end);
        }

        if (tkz->entity_number <= 0x10FFFF) {
            tkz->entity_number <<= 4;
            tkz->entity_number |= pchtml_str_res_map_hex[ *data ];
        }

        data++;
    }

    return data;
}

/*
 * 12.2.5.79 Decimal character reference state
 */
static const unsigned char *
pchtml_html_tokenizer_state_char_ref_decimal(pchtml_html_tokenizer_t *tkz,
                                          const unsigned char *data,
                                          const unsigned char *end)
{
    while (data != end) {
        if (pchtml_str_res_map_num[ *data ] == PCHTML_STR_RES_SLIP) {
            tkz->state = tkz->state_return;

            if (*data == ';') {
                data++;
            }

            return pchtml_html_tokenizer_state_char_ref_numeric_end(tkz, data, end);
        }

        if (tkz->entity_number <= 0x10FFFF) {
            tkz->entity_number = pchtml_str_res_map_num[ *data ]
                                 + tkz->entity_number * 10;
        }

        data++;
    }

    return data;
}

/*
 * 12.2.5.80 Numeric character reference end state
 */
static const unsigned char *
pchtml_html_tokenizer_state_char_ref_numeric_end(pchtml_html_tokenizer_t *tkz,
                                              const unsigned char *data,
                                              const unsigned char *end)
{
    unsigned char *start = &tkz->start[tkz->entity_start];

    if ((start + 4) > tkz->end) {
        if(pchtml_html_tokenizer_temp_realloc(tkz, 4)) {
            return end;
        }

        start = &tkz->start[tkz->entity_start];
    }

    if (tkz->entity_number == 0x00) {
        pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->markup,
                                     PCHTML_PARSER_TOKENIZER_ERROR_NUCHRE);

        goto xFFFD;
    }
    else if (tkz->entity_number > 0x10FFFF) {
        pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->markup,
                                     PCHTML_PARSER_TOKENIZER_ERROR_CHREOUUNRA);

        goto xFFFD;
    }
    else if (tkz->entity_number >= 0xD800 && tkz->entity_number <= 0xDFFF) {
        pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->markup,
                                     PCHTML_PARSER_TOKENIZER_ERROR_SUCHRE);

        goto xFFFD;
    }
    else if (tkz->entity_number >= 0xFDD0 && tkz->entity_number <= 0xFDEF) {
        pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->markup,
                                     PCHTML_PARSER_TOKENIZER_ERROR_NOCHRE);
    }

    switch (tkz->entity_number) {
        case 0xFFFE:  case 0xFFFF:  case 0x1FFFE: case 0x1FFFF: case 0x2FFFE:
        case 0x2FFFF: case 0x3FFFE: case 0x3FFFF: case 0x4FFFE: case 0x4FFFF:
        case 0x5FFFE: case 0x5FFFF: case 0x6FFFE: case 0x6FFFF: case 0x7FFFE:
        case 0x7FFFF: case 0x8FFFE: case 0x8FFFF: case 0x9FFFE: case 0x9FFFF:
        case 0xAFFFE: case 0xAFFFF: case 0xBFFFE: case 0xBFFFF: case 0xCFFFE:
        case 0xCFFFF: case 0xDFFFE: case 0xDFFFF: case 0xEFFFE: case 0xEFFFF:
        case 0xFFFFE: case 0xFFFFF:
        case 0x10FFFE:
        case 0x10FFFF:
            pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->markup,
                                         PCHTML_PARSER_TOKENIZER_ERROR_NOCHRE);
            break;

        default:
            break;
    }

    if (tkz->entity_number <= 0x1F
        || (tkz->entity_number >= 0x7F && tkz->entity_number <= 0x9F))
    {
        pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->markup,
                                     PCHTML_PARSER_TOKENIZER_ERROR_COCHRE);
    }

    if (tkz->entity_number <= 0x9F) {
        tkz->entity_number = (uint32_t) pchtml_str_res_replacement_character[tkz->entity_number];
    }

    start += pchtml_html_tokenizer_state_to_ascii_utf_8(tkz->entity_number, start);

    tkz->pos = start;

    return data;

xFFFD:

    memcpy(start, pchtml_str_res_ansi_replacement_character,
           sizeof(pchtml_str_res_ansi_replacement_character) - 1);

    tkz->pos = start + sizeof(pchtml_str_res_ansi_replacement_character) - 1;

    return data;
}

static size_t
pchtml_html_tokenizer_state_to_ascii_utf_8(size_t codepoint, unsigned char *data)
{
    /* 0x80 -- 10xxxxxx */
    /* 0xC0 -- 110xxxxx */
    /* 0xE0 -- 1110xxxx */
    /* 0xF0 -- 11110xxx */

    if (codepoint <= 0x0000007F) {
        /* 0xxxxxxx */
        data[0] = (char) codepoint;

        return 1;
    }
    else if (codepoint <= 0x000007FF) {
        /* 110xxxxx 10xxxxxx */
        data[0] = (char) (0xC0 | (codepoint >> 6  ));
        data[1] = (char) (0x80 | (codepoint & 0x3F));

        return 2;
    }
    else if (codepoint <= 0x0000FFFF) {
        /* 1110xxxx 10xxxxxx 10xxxxxx */
        data[0] = (char) (0xE0 | ((codepoint >> 12)));
        data[1] = (char) (0x80 | ((codepoint >> 6 ) & 0x3F));
        data[2] = (char) (0x80 | ( codepoint & 0x3F));

        return 3;
    }
    else if (codepoint <= 0x001FFFFF) {
        /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
        data[0] = (char) (0xF0 | ( codepoint >> 18));
        data[1] = (char) (0x80 | ((codepoint >> 12) & 0x3F));
        data[2] = (char) (0x80 | ((codepoint >> 6 ) & 0x3F));
        data[3] = (char) (0x80 | ( codepoint & 0x3F));

        return 4;
    }

    return 0;
}
