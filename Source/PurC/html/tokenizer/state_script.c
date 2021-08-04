/**
 * @file state_script.c 
 * @author 
 * @date 2021/07/02
 * @brief The complementation of html scripte state.
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
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache 
 * License Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h" 

#include "html/tokenizer/state_script.h"
#include "html/tokenizer/state.h"

#define PCHTML_STR_RES_ALPHA_CHARACTER
#define PCHTML_STR_RES_ANSI_REPLACEMENT_CHARACTER
#include "html/str_res.h"

#include "html/str_res.h"


const pchtml_tag_data_t *
pchtml_tag_append_lower(pchtml_hash_t *hash,
                     const unsigned char *name, size_t length);


static const unsigned char *
pchtml_html_tokenizer_state_script_data(pchtml_html_tokenizer_t *tkz,
                                     const unsigned char *data,
                                     const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_less_than_sign(pchtml_html_tokenizer_t *tkz,
                                                    const unsigned char *data,
                                                    const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_end_tag_open(pchtml_html_tokenizer_t *tkz,
                                                  const unsigned char *data,
                                                  const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_end_tag_name(pchtml_html_tokenizer_t *tkz,
                                                  const unsigned char *data,
                                                  const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_escape_start(pchtml_html_tokenizer_t *tkz,
                                                  const unsigned char *data,
                                                  const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_escape_start_dash(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_escaped(pchtml_html_tokenizer_t *tkz,
                                             const unsigned char *data,
                                             const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_escaped_dash(pchtml_html_tokenizer_t *tkz,
                                                  const unsigned char *data,
                                                  const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_escaped_dash_dash(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_escaped_less_than_sign(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_escaped_end_tag_open(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_escaped_end_tag_name(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_double_escape_start(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_double_escaped(pchtml_html_tokenizer_t *tkz,
                                                    const unsigned char *data,
                                                    const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_double_escaped_dash(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_double_escaped_dash_dash(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_double_escaped_less_than_sign(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_double_escaped_end_tag_open(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_script_data_double_escape_end(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);


/*
 * Helper function. No in the specification. For 12.2.5.4 Script data state
 */
const unsigned char *
pchtml_html_tokenizer_state_script_data_before(pchtml_html_tokenizer_t *tkz,
                                            const unsigned char *data,
                                            const unsigned char *end)
{
    UNUSED_PARAM(end);

    if (tkz->is_eof == false) {
        pchtml_html_tokenizer_state_token_set_begin(tkz, data);
    }

    tkz->state = pchtml_html_tokenizer_state_script_data;

    return data;
}

/*
 * 12.2.5.4 Script data state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data(pchtml_html_tokenizer_t *tkz,
                                     const unsigned char *data,
                                     const unsigned char *end)
{
    pchtml_html_tokenizer_state_begin_set(tkz, data);

    while (data != end) {
        switch (*data) {
            /* U+003C LESS-THAN SIGN (<) */
            case 0x3C:
                pchtml_html_tokenizer_state_append_data_m(tkz, data + 1);
                pchtml_html_tokenizer_state_token_set_end(tkz, data);

                tkz->state =
                    pchtml_html_tokenizer_state_script_data_less_than_sign;

                return (data + 1);

            /* U+000D CARRIAGE RETURN (CR) */
            case 0x0D:
                if (++data >= end) {
                    pchtml_html_tokenizer_state_append_data_m(tkz, data - 1);

                    tkz->state = pchtml_html_tokenizer_state_cr;
                    tkz->state_return = pchtml_html_tokenizer_state_script_data;

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

                    tkz->token->tag_id = PCHTML_TAG__TEXT;

                    pchtml_html_tokenizer_state_set_text(tkz);
                    pchtml_html_tokenizer_state_token_done_m(tkz, end);

                    return end;
                }

                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);
                pchtml_html_tokenizer_state_append_replace_m(tkz);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_HTML_TOKENIZER_ERROR_UNNUCH);
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
 * 12.2.5.15 Script data less-than sign state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_less_than_sign(pchtml_html_tokenizer_t *tkz,
                                                    const unsigned char *data,
                                                    const unsigned char *end)
{
    UNUSED_PARAM(end);

    switch (*data) {
        /* U+002F SOLIDUS (/) */
        case 0x2F:
            tkz->state = pchtml_html_tokenizer_state_script_data_end_tag_open;

            return (data + 1);

        /* U+0021 EXCLAMATION MARK (!) */
        case 0x21:
            tkz->state = pchtml_html_tokenizer_state_script_data_escape_start;

            return (data + 1);

        default:
            tkz->state = pchtml_html_tokenizer_state_script_data;

            break;
    }

    return data;
}

/*
 * 12.2.5.16 Script data end tag open state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_end_tag_open(pchtml_html_tokenizer_t *tkz,
                                                  const unsigned char *data,
                                                  const unsigned char *end)
{
    if (pchtml_str_res_alpha_character[*data] != PCHTML_STR_RES_SLIP) {
        tkz->entity_start = (tkz->pos - 1) - tkz->start;
        tkz->temp = data;

        tkz->state = pchtml_html_tokenizer_state_script_data_end_tag_name;
    }
    else {
        tkz->state = pchtml_html_tokenizer_state_script_data;
    }

    pchtml_html_tokenizer_state_append_m(tkz, "/", 1);

    return data;
}

/*
 * 12.2.5.17 Script data end tag name state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_end_tag_name(pchtml_html_tokenizer_t *tkz,
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
             */
            case 0x09:
            case 0x0A:
            case 0x0C:
            case 0x0D:
            case 0x20:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_set_tag_m(tkz, &tkz->start[tkz->entity_start] + 2,
                                                   tkz->pos);

                if (tkz->tmp_tag_id != tkz->token->tag_id) {
                    goto anything_else;
                }

                tkz->state = pchtml_html_tokenizer_state_before_attribute_name;

                goto done;

            /* U+002F SOLIDUS (/) */
            case 0x2F:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_set_tag_m(tkz, &tkz->start[tkz->entity_start] + 2,
                                                   tkz->pos);

                if (tkz->tmp_tag_id != tkz->token->tag_id) {
                    goto anything_else;
                }

                tkz->state = pchtml_html_tokenizer_state_self_closing_start_tag;

                goto done;

            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_set_tag_m(tkz, &tkz->start[tkz->entity_start] + 2,
                                                   tkz->pos);

                if (tkz->tmp_tag_id != tkz->token->tag_id) {
                    goto anything_else;
                }

                tkz->state = pchtml_html_tokenizer_state_data_before;

                /* Emit text token */
                tkz->token->tag_id = PCHTML_TAG__TEXT;
                tkz->pos = &tkz->start[tkz->entity_start];

                pchtml_html_tokenizer_state_set_text(tkz);
                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                /* Init close token */
                tkz->token->tag_id = tkz->tmp_tag_id;
                tkz->token->begin = tkz->temp;
                tkz->token->end = data;
                tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_CLOSE;

                /* Emit close token */
                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return (data + 1);

            default:
                if (pchtml_str_res_alpha_character[*data]
                    == PCHTML_STR_RES_SLIP)
                {
                    goto anything_else;
                }

                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;

anything_else:

    tkz->state = pchtml_html_tokenizer_state_script_data;

    return data;

done:

    /* Emit text token */
    tkz->token->tag_id = PCHTML_TAG__TEXT;
    tkz->pos = &tkz->start[tkz->entity_start];

    pchtml_html_tokenizer_state_set_text(tkz);
    pchtml_html_tokenizer_state_token_done_m(tkz, end);

    /* Init close token */
    tkz->token->tag_id = tkz->tmp_tag_id;
    tkz->token->begin = tkz->temp;
    tkz->token->end = data;
    tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_CLOSE;

    return (data + 1);
}

/*
 * 12.2.5.18 Script data escape start state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_escape_start(pchtml_html_tokenizer_t *tkz,
                                                  const unsigned char *data,
                                                  const unsigned char *end)
{
    /* U+002D HYPHEN-MINUS (-) */
    if (*data == 0x2D) {
        tkz->state = pchtml_html_tokenizer_state_script_data_escape_start_dash;

        return (data + 1);
    }

    pchtml_html_tokenizer_state_append_m(tkz, "!", 1);

    tkz->state = pchtml_html_tokenizer_state_script_data;

    return data;
}

/*
 * 12.2.5.19 Script data escape start dash state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_escape_start_dash(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end)
{
    /* U+002D HYPHEN-MINUS (-) */
    if (*data == 0x2D) {
        pchtml_html_tokenizer_state_append_m(tkz, "!--", 3);

        tkz->state = pchtml_html_tokenizer_state_script_data_escaped_dash_dash;

        return (data + 1);
    }

    pchtml_html_tokenizer_state_append_m(tkz, "!-", 2);

    tkz->state = pchtml_html_tokenizer_state_script_data;

    return data;
}

/*
 * 12.2.5.20 Script data escaped state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_escaped(pchtml_html_tokenizer_t *tkz,
                                             const unsigned char *data,
                                             const unsigned char *end)
{
    pchtml_html_tokenizer_state_begin_set(tkz, data);

    while (data != end) {
        switch (*data) {
            /* U+002D HYPHEN-MINUS (-) */
            case 0x2D:
                pchtml_html_tokenizer_state_append_data_m(tkz, data + 1);

                tkz->state = pchtml_html_tokenizer_state_script_data_escaped_dash;

                return (data + 1);

            /* U+003C LESS-THAN SIGN (<) */
            case 0x3C:
                pchtml_html_tokenizer_state_append_data_m(tkz, data + 1);
                pchtml_html_tokenizer_state_token_set_end(tkz, data);

                tkz->state =
                    pchtml_html_tokenizer_state_script_data_escaped_less_than_sign;

                return (data + 1);

            /* U+000D CARRIAGE RETURN (CR) */
            case 0x0D:
                if (++data >= end) {
                    pchtml_html_tokenizer_state_append_data_m(tkz, data - 1);

                    tkz->state = pchtml_html_tokenizer_state_cr;
                    tkz->state_return = pchtml_html_tokenizer_state_script_data_escaped;

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
                    pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                       PCHTML_HTML_TOKENIZER_ERROR_EOINSCHTCOLITE);

                    tkz->token->tag_id = PCHTML_TAG__TEXT;

                    pchtml_html_tokenizer_state_set_text(tkz);
                    pchtml_html_tokenizer_state_token_set_end_oef(tkz);
                    pchtml_html_tokenizer_state_token_done_m(tkz, end);

                    return end;
                }

                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);
                pchtml_html_tokenizer_state_append_replace_m(tkz);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_HTML_TOKENIZER_ERROR_UNNUCH);
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
 * 12.2.5.21 Script data escaped dash state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_escaped_dash(pchtml_html_tokenizer_t *tkz,
                                                  const unsigned char *data,
                                                  const unsigned char *end)
{
    switch (*data) {
        /* U+002D HYPHEN-MINUS (-) */
        case 0x2D:
            pchtml_html_tokenizer_state_append_m(tkz, data, 1);

            tkz->state = pchtml_html_tokenizer_state_script_data_escaped_dash_dash;

            return (data + 1);

        /* U+003C LESS-THAN SIGN (<) */
        case 0x3C:
            pchtml_html_tokenizer_state_append_m(tkz, data, 1);
            pchtml_html_tokenizer_state_token_set_end(tkz, data);

            tkz->state =
                pchtml_html_tokenizer_state_script_data_escaped_less_than_sign;

            return (data + 1);

        /*
         * U+0000 NULL
         * EOF
         */
        case 0x00:
            if (tkz->is_eof) {
                pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                       PCHTML_HTML_TOKENIZER_ERROR_EOINSCHTCOLITE);

                tkz->token->tag_id = PCHTML_TAG__TEXT;

                pchtml_html_tokenizer_state_set_text(tkz);
                pchtml_html_tokenizer_state_token_set_end_oef(tkz);
                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return end;
            }

            pchtml_html_tokenizer_state_append_replace_m(tkz);

            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_UNNUCH);

            tkz->state = pchtml_html_tokenizer_state_script_data_escaped;

            return (data + 1);

        default:
            tkz->state = pchtml_html_tokenizer_state_script_data_escaped;

            return data;
    }
}

/*
 * 12.2.5.22 Script data escaped dash dash state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_escaped_dash_dash(pchtml_html_tokenizer_t *tkz,
                                                       const unsigned char *data,
                                                       const unsigned char *end)
{
    switch (*data) {
        /* U+002D HYPHEN-MINUS (-) */
        case 0x2D:
            pchtml_html_tokenizer_state_append_m(tkz, "-", 1);
            return (data + 1);

        /* U+003C LESS-THAN SIGN (<) */
        case 0x3C:
            pchtml_html_tokenizer_state_append_m(tkz, "<", 1);
            pchtml_html_tokenizer_state_token_set_end(tkz, data);

            tkz->state =
                pchtml_html_tokenizer_state_script_data_escaped_less_than_sign;

            return (data + 1);

        /* U+003E GREATER-THAN SIGN (>) */
        case 0x3E:
            tkz->state = pchtml_html_tokenizer_state_script_data;
            return data;

        default:
            tkz->state = pchtml_html_tokenizer_state_script_data_escaped;
            return data;
    }
}

/*
 * 12.2.5.23 Script data escaped less-than sign state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_escaped_less_than_sign(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end)
{
    UNUSED_PARAM(end);

    /* U+002F SOLIDUS (/) */
    if (*data == 0x2F) {
        tkz->state = pchtml_html_tokenizer_state_script_data_escaped_end_tag_open;

        return (data + 1);
    }

    /* ASCII alpha */
    if (pchtml_str_res_alpha_character[*data] != PCHTML_STR_RES_SLIP) {
        tkz->entity_start = tkz->pos - tkz->start;

        tkz->state = pchtml_html_tokenizer_state_script_data_double_escape_start;

        return data;
    }

    tkz->state = pchtml_html_tokenizer_state_script_data_escaped;

    return data;
}

/*
 * 12.2.5.24 Script data escaped end tag open state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_escaped_end_tag_open(pchtml_html_tokenizer_t *tkz,
                                                          const unsigned char *data,
                                                          const unsigned char *end)
{
    if (pchtml_str_res_alpha_character[*data] != PCHTML_STR_RES_SLIP) {
        tkz->temp = data;
        tkz->entity_start = (tkz->pos - 1) - tkz->start;

        tkz->state = pchtml_html_tokenizer_state_script_data_escaped_end_tag_name;
    }
    else {
        tkz->state = pchtml_html_tokenizer_state_script_data_escaped;
    }

    pchtml_html_tokenizer_state_append_m(tkz, "/", 1);

    return data;
}

/*
 * 12.2.5.25 Script data escaped end tag name state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_escaped_end_tag_name(
                                                      pchtml_html_tokenizer_t *tkz,
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
            */
            case 0x09:
            case 0x0A:
            case 0x0C:
            case 0x0D:
            case 0x20:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_set_tag_m(tkz, &tkz->start[tkz->entity_start] + 2,
                                                   tkz->pos);

                if (tkz->tmp_tag_id != tkz->token->tag_id) {
                    goto anything_else;
                }

                tkz->state = pchtml_html_tokenizer_state_before_attribute_name;

                goto done;

            /* U+002F SOLIDUS (/) */
            case 0x2F:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_set_tag_m(tkz, &tkz->start[tkz->entity_start] + 2,
                                                   tkz->pos);

                if (tkz->tmp_tag_id != tkz->token->tag_id) {
                    goto anything_else;
                }

                tkz->state = pchtml_html_tokenizer_state_self_closing_start_tag;

                goto done;

            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_set_tag_m(tkz, &tkz->start[tkz->entity_start] + 2,
                                                   tkz->pos);

                if (tkz->tmp_tag_id != tkz->token->tag_id) {
                    goto anything_else;
                }

                tkz->state = pchtml_html_tokenizer_state_data_before;

                /* Emit text token */
                tkz->token->tag_id = PCHTML_TAG__TEXT;
                tkz->pos = &tkz->start[tkz->entity_start];

                pchtml_html_tokenizer_state_set_text(tkz);
                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                /* Init close token */
                tkz->token->tag_id = tkz->tmp_tag_id;
                tkz->token->begin = tkz->temp;
                tkz->token->end = data;
                tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_CLOSE;

                /* Emit close token */
                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return (data + 1);

            default:
                if (pchtml_str_res_alpha_character[*data]
                    == PCHTML_STR_RES_SLIP)
                {
                    pchtml_html_tokenizer_state_append_data_m(tkz, data);
                    goto anything_else;
                }

                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;

anything_else:

    tkz->state = pchtml_html_tokenizer_state_script_data_escaped;

    return data;

done:

    /* Emit text token */
    tkz->token->tag_id = PCHTML_TAG__TEXT;
    tkz->pos = &tkz->start[tkz->entity_start];

    pchtml_html_tokenizer_state_set_text(tkz);
    pchtml_html_tokenizer_state_token_done_m(tkz, end);

    /* Init close token */
    tkz->token->tag_id = tkz->tmp_tag_id;
    tkz->token->begin = tkz->temp;
    tkz->token->end = data;
    tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_CLOSE;

    return (data + 1);
}

/*
 * 12.2.5.26 Script data double escape start state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_double_escape_start(pchtml_html_tokenizer_t *tkz,
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

                if ((tkz->pos - &tkz->start[tkz->entity_start]) == 6
                    && pchtml_str_data_ncasecmp(&tkz->start[tkz->entity_start],
                                                (const unsigned char *) "script", 6))
                {
                    tkz->state =
                        pchtml_html_tokenizer_state_script_data_double_escaped;

                    return data;
                }

                tkz->state = pchtml_html_tokenizer_state_script_data_escaped;

                return data;

            default:
                if (pchtml_str_res_alpha_character[*data]
                    == PCHTML_STR_RES_SLIP)
                {
                    pchtml_html_tokenizer_state_append_data_m(tkz, data);

                    tkz->state = pchtml_html_tokenizer_state_script_data_escaped;

                    return data;
                }

                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;
}

/*
 * 12.2.5.27 Script data double escaped state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_double_escaped(pchtml_html_tokenizer_t *tkz,
                                                    const unsigned char *data,
                                                    const unsigned char *end)
{
    pchtml_html_tokenizer_state_begin_set(tkz, data);

    while (data != end) {
        switch (*data) {
            /* U+002D HYPHEN-MINUS (-) */
            case 0x2D:
                pchtml_html_tokenizer_state_append_data_m(tkz, data + 1);

                tkz->state =
                    pchtml_html_tokenizer_state_script_data_double_escaped_dash;

                return (data + 1);

            /* U+003C LESS-THAN SIGN (<) */
            case 0x3C:
                pchtml_html_tokenizer_state_append_data_m(tkz, data + 1);

                tkz->state =
             pchtml_html_tokenizer_state_script_data_double_escaped_less_than_sign;

                return (data + 1);

            /* U+000D CARRIAGE RETURN (CR) */
            case 0x0D:
                if (++data >= end) {
                    pchtml_html_tokenizer_state_append_data_m(tkz, data - 1);

                    tkz->state = pchtml_html_tokenizer_state_cr;
                    tkz->state_return = pchtml_html_tokenizer_state_script_data_double_escaped;

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
                    pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                       PCHTML_HTML_TOKENIZER_ERROR_EOINSCHTCOLITE);

                    tkz->token->tag_id = PCHTML_TAG__TEXT;

                    pchtml_html_tokenizer_state_set_text(tkz);
                    pchtml_html_tokenizer_state_token_set_end_oef(tkz);
                    pchtml_html_tokenizer_state_token_done_m(tkz, end);

                    return end;
                }

                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);
                pchtml_html_tokenizer_state_append_replace_m(tkz);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_HTML_TOKENIZER_ERROR_UNNUCH);
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
 * 12.2.5.28 Script data double escaped dash state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_double_escaped_dash(pchtml_html_tokenizer_t *tkz,
                                                         const unsigned char *data,
                                                         const unsigned char *end)
{
    switch (*data) {
        /* U+002D HYPHEN-MINUS (-) */
        case 0x2D:
            pchtml_html_tokenizer_state_append_m(tkz, data, 1);

            tkz->state =
                pchtml_html_tokenizer_state_script_data_double_escaped_dash_dash;

            return (data + 1);

        /* U+003C LESS-THAN SIGN (<) */
        case 0x3C:
            pchtml_html_tokenizer_state_append_m(tkz, data, 1);

            tkz->state =
             pchtml_html_tokenizer_state_script_data_double_escaped_less_than_sign;

            return (data + 1);

        /*
         * U+0000 NULL
         * EOF
         */
        case 0x00:
            if (tkz->is_eof) {
                pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                       PCHTML_HTML_TOKENIZER_ERROR_EOINSCHTCOLITE);

                tkz->token->tag_id = PCHTML_TAG__TEXT;

                pchtml_html_tokenizer_state_set_text(tkz);
                pchtml_html_tokenizer_state_token_set_end_oef(tkz);
                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return end;
            }

            pchtml_html_tokenizer_state_append_replace_m(tkz);

            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_UNNUCH);

            tkz->state = pchtml_html_tokenizer_state_script_data_double_escaped;

            return (data + 1);

        default:
            tkz->state = pchtml_html_tokenizer_state_script_data_double_escaped;

            return data;
    }
}

/*
 * 12.2.5.29 Script data double escaped dash dash state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_double_escaped_dash_dash(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end)
{
    switch (*data) {
        /* U+002D HYPHEN-MINUS (-) */
        case 0x2D:
            pchtml_html_tokenizer_state_append_m(tkz, data, 1);
            return (data + 1);

        /* U+003C LESS-THAN SIGN (<) */
        case 0x3C:
            pchtml_html_tokenizer_state_append_m(tkz, data, 1);

            tkz->state =
             pchtml_html_tokenizer_state_script_data_double_escaped_less_than_sign;

            return (data + 1);

        /* U+003E GREATER-THAN SIGN (>) */
        case 0x3E:
            pchtml_html_tokenizer_state_append_m(tkz, data, 1);

            tkz->state = pchtml_html_tokenizer_state_script_data;

            return (data + 1);

        /*
         * U+0000 NULL
         * EOF
         */
        case 0x00:
            if (tkz->is_eof) {
                pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                       PCHTML_HTML_TOKENIZER_ERROR_EOINSCHTCOLITE);

                tkz->token->tag_id = PCHTML_TAG__TEXT;

                pchtml_html_tokenizer_state_set_text(tkz);
                pchtml_html_tokenizer_state_token_set_end_oef(tkz);
                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return end;
            }

            pchtml_html_tokenizer_state_append_replace_m(tkz);

            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_UNNUCH);

            tkz->state = pchtml_html_tokenizer_state_script_data_double_escaped;

            return (data + 1);

        default:
            tkz->state = pchtml_html_tokenizer_state_script_data_double_escaped;

            return data;
    }

    return data;
}

/*
 * 12.2.5.30 Script data double escaped less-than sign state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_double_escaped_less_than_sign(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end)
{
    UNUSED_PARAM(end);

    /* U+002F SOLIDUS (/) */
    if (*data == 0x2F) {
        tkz->state =
            pchtml_html_tokenizer_state_script_data_double_escaped_end_tag_open;

        return (data + 1);
    }

    tkz->state = pchtml_html_tokenizer_state_script_data_double_escaped;

    return data;
}

/*
 * 12.2.5.30.5 Helper function. No in the specification.
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_double_escaped_end_tag_open(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end)
{
    if (pchtml_str_res_alpha_character[*data] != PCHTML_STR_RES_SLIP) {
        tkz->entity_start = (tkz->pos + 1) - tkz->start;

        tkz->state = pchtml_html_tokenizer_state_script_data_double_escape_end;
    }
    else {
        tkz->state = pchtml_html_tokenizer_state_script_data_double_escaped;
    }

    pchtml_html_tokenizer_state_append_m(tkz, "/", 1);

    return data;
}

/*
 * 12.2.5.31 Script data double escape end state
 */
static const unsigned char *
pchtml_html_tokenizer_state_script_data_double_escape_end(
                                                      pchtml_html_tokenizer_t *tkz,
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

                if ((tkz->pos - &tkz->start[tkz->entity_start]) == 6
                    && pchtml_str_data_ncasecmp(&tkz->start[tkz->entity_start],
                                                (const unsigned char *) "script", 6))
                {
                    tkz->state = pchtml_html_tokenizer_state_script_data_escaped;
                    return data;
                }

                tkz->state = pchtml_html_tokenizer_state_script_data_double_escaped;

                return data;

            default:
                if (pchtml_str_res_alpha_character[*data]
                    == PCHTML_STR_RES_SLIP)
                {
                    pchtml_html_tokenizer_state_append_data_m(tkz, data);

                    tkz->state = pchtml_html_tokenizer_state_script_data_double_escaped;
                    return data;
                }

                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;
}
