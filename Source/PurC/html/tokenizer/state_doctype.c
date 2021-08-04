/**
 * @file state_doctype.c 
 * @author 
 * @date 2021/07/02
 * @brief The complementation of html document type state.
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

#include "html/tokenizer/state_doctype.h"
#include "html/tokenizer/state.h"


#define PCHTML_STR_RES_ANSI_REPLACEMENT_CHARACTER
#include "html/str_res.h"


pcedom_attr_data_t *
pcedom_attr_local_name_append(pchtml_hash_t *hash,
                               const unsigned char *name, size_t length);


static const unsigned char *
pchtml_html_tokenizer_state_doctype(pchtml_html_tokenizer_t *tkz,
                                 const unsigned char *data,
                                 const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_doctype_before_name(pchtml_html_tokenizer_t *tkz,
                                             const unsigned char *data,
                                             const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_doctype_name(pchtml_html_tokenizer_t *tkz,
                                      const unsigned char *data,
                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_doctype_after_name(pchtml_html_tokenizer_t *tkz,
                                            const unsigned char *data,
                                            const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_doctype_after_name_public(pchtml_html_tokenizer_t *tkz,
                                                   const unsigned char *data,
                                                   const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_doctype_after_name_system(pchtml_html_tokenizer_t *tkz,
                                                   const unsigned char *data,
                                                   const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_doctype_after_public_keyword(pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_doctype_before_public_identifier(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_doctype_public_identifier_double_quoted(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_doctype_public_identifier_single_quoted(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_doctype_after_public_identifier(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_doctype_between_public_and_system_identifiers(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_doctype_after_system_keyword(pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_doctype_before_system_identifier(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_doctype_system_identifier_double_quoted(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_doctype_system_identifier_single_quoted(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_doctype_after_system_identifier(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_doctype_bogus(pchtml_html_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end);


/*
 * Helper function. No in the specification. For 12.2.5.53
 */
const unsigned char *
pchtml_html_tokenizer_state_doctype_before(pchtml_html_tokenizer_t *tkz,
                                        const unsigned char *data,
                                        const unsigned char *end)
{
    if (tkz->is_eof == false) {
        pchtml_html_tokenizer_state_token_set_end(tkz, data);
    }
    else {
        pchtml_html_tokenizer_state_token_set_end_oef(tkz);
    }

    tkz->token->tag_id = PCHTML_TAG__EM_DOCTYPE;

    return pchtml_html_tokenizer_state_doctype(tkz, data, end);
}

/*
 * 12.2.5.53 DOCTYPE state
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype(pchtml_html_tokenizer_t *tkz,
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
            data++;
            break;

        /* U+003E GREATER-THAN SIGN (>) */
        case 0x3E:
            break;

        /* EOF */
        case 0x00:
            if (tkz->is_eof) {
                pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                             PCHTML_HTML_TOKENIZER_ERROR_EOINDO);

                tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;

                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return end;
            }
            /* fall through */

        default:
            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_MIWHBEDONA);
            break;
    }

    tkz->state = pchtml_html_tokenizer_state_doctype_before_name;

    return data;
}

/*
 * 12.2.5.54 Before DOCTYPE name state
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype_before_name(pchtml_html_tokenizer_t *tkz,
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

            /*
             * U+0000 NULL
             * EOF
             */
            case 0x00:
                if (tkz->is_eof) {
                    pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                               PCHTML_HTML_TOKENIZER_ERROR_EOINDO);

                    tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;

                    pchtml_html_tokenizer_state_token_done_m(tkz, end);

                    return end;
                }

                pchtml_html_tokenizer_state_token_attr_add_m(tkz, attr, end);
                pchtml_html_tokenizer_state_token_attr_set_name_begin(tkz, data);
                pchtml_html_tokenizer_state_append_replace_m(tkz);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_HTML_TOKENIZER_ERROR_UNNUCH);

                tkz->token->attr_last->type
                    |= PCHTML_HTML_TOKEN_ATTR_TYPE_NAME_NULL;

                tkz->state = pchtml_html_tokenizer_state_doctype_name;

                return (data + 1);

            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;
                tkz->state = pchtml_html_tokenizer_state_data_before;

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_HTML_TOKENIZER_ERROR_MIDONA);

                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return (data + 1);

            /*
             * ASCII upper alpha
             * Anything else
             */
            default:
                pchtml_html_tokenizer_state_token_attr_add_m(tkz, attr, end);
                pchtml_html_tokenizer_state_token_attr_set_name_begin(tkz, data);

                tkz->state = pchtml_html_tokenizer_state_doctype_name;

                return data;
        }

        data++;
    }

    return data;
}

/*
 * 12.2.5.55 DOCTYPE name state
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype_name(pchtml_html_tokenizer_t *tkz,
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
                pchtml_html_tokenizer_state_set_name_m(tkz);
                pchtml_html_tokenizer_state_token_attr_set_name_end(tkz, data);

                tkz->state = pchtml_html_tokenizer_state_doctype_after_name;

                return (data + 1);

            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                tkz->state = pchtml_html_tokenizer_state_data_before;

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_set_name_m(tkz);
                pchtml_html_tokenizer_state_token_attr_set_name_end(tkz, data);
                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return (data + 1);

            /*
             * U+0000 NULL
             * EOF
             */
            case 0x00:
                pchtml_html_tokenizer_state_append_data_m(tkz, data);

                if (tkz->is_eof) {
                    pchtml_html_tokenizer_state_token_attr_set_name_end_oef(tkz);

                    pchtml_html_tokenizer_error_add(tkz->parse_errors,
                                               tkz->token->attr_last->name_end,
                                               PCHTML_HTML_TOKENIZER_ERROR_EOINDO);

                    tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;

                    pchtml_html_tokenizer_state_set_name_m(tkz);
                    pchtml_html_tokenizer_state_token_done_m(tkz, end);

                    return end;
                }

                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);
                pchtml_html_tokenizer_state_append_replace_m(tkz);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_HTML_TOKENIZER_ERROR_UNNUCH);

                tkz->token->attr_last->type
                    |= PCHTML_HTML_TOKEN_ATTR_TYPE_NAME_NULL;

                break;

            /* Anything else */
            default:
                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;
}

/*
 * 12.2.5.56 After DOCTYPE name state
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype_after_name(pchtml_html_tokenizer_t *tkz,
                                            const unsigned char *data,
                                            const unsigned char *end)
{
    pchtml_html_token_attr_t *attr;
    const pcedom_attr_data_t *attr_data;

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

            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                tkz->state = pchtml_html_tokenizer_state_data_before;

                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return (data + 1);

            /* EOF */
            case 0x00:
                if (tkz->is_eof) {
                    pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                               PCHTML_HTML_TOKENIZER_ERROR_EOINDO);

                    tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;

                    pchtml_html_tokenizer_state_token_done_m(tkz, end);

                    return end;
                }
                /* fall through */

            /* Anything else */
            default:
                pchtml_html_tokenizer_state_token_attr_add_m(tkz, attr, end);
                pchtml_html_tokenizer_state_token_attr_set_name_begin(tkz, data);

                if ((data + 6) > end) {
                    /*
                     * ASCII case-insensitive match for the word "PUBLIC"
                     * U+0044 character (P) or U+0050 character (p)
                     */
                    if (*data == 0x50 || *data == 0x70) {
                        tkz->markup = (unsigned char *) "PUBLIC";

                        tkz->state =
                            pchtml_html_tokenizer_state_doctype_after_name_public;

                        return data;
                    }

                    /*
                     * ASCII case-insensitive match for the word "SYSTEM"
                     * U+0044 character (S) or U+0053 character (s)
                     */
                    if (*data == 0x53 || *data == 0x73) {
                        tkz->markup = (unsigned char *) "SYSTEM";

                        tkz->state =
                            pchtml_html_tokenizer_state_doctype_after_name_system;

                        return data;
                    }
                }
                else if (pchtml_str_data_ncasecmp((unsigned char *) "PUBLIC",
                                                  data, 6))
                {
                    pchtml_html_tokenizer_state_token_attr_set_name_end(tkz,
                                                                    (data + 6));

                    attr_data = pcedom_attr_data_by_id(tkz->attrs,
                                                        PCEDOM_ATTR_PUBLIC);
                    if (attr_data == NULL) {
                        pcinst_set_error (PCHTML_ERROR);
                        tkz->status = PCHTML_STATUS_ERROR;
                        return end;
                    }

                    tkz->token->attr_last->name = attr_data;

                    tkz->state =
                        pchtml_html_tokenizer_state_doctype_after_public_keyword;

                    return (data + 6);
                }
                else if (pchtml_str_data_ncasecmp((unsigned char *) "SYSTEM",
                                                  data, 6))
                {
                    pchtml_html_tokenizer_state_token_attr_set_name_end(tkz,
                                                                    (data + 6));

                    attr_data = pcedom_attr_data_by_id(tkz->attrs,
                                                        PCEDOM_ATTR_SYSTEM);
                    if (attr_data == NULL) {
                        pcinst_set_error (PCHTML_ERROR);
                        tkz->status = PCHTML_STATUS_ERROR;
                        return end;
                    }

                    tkz->token->attr_last->name = attr_data;

                    tkz->state =
                        pchtml_html_tokenizer_state_doctype_after_system_keyword;

                    return (data + 6);
                }

                pchtml_html_token_attr_delete(tkz->token, attr,
                                           tkz->dobj_token_attr);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_INCHSEAFDONA);

                tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;
                tkz->state = pchtml_html_tokenizer_state_doctype_bogus;

                return data;
        }

        data++;
    }

    return data;
}

/*
 * Helper function. No in the specification. For 12.2.5.56
 * For doctype PUBLIC
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype_after_name_public(pchtml_html_tokenizer_t *tkz,
                                                   const unsigned char *data,
                                                   const unsigned char *end)
{
    const unsigned char *pos;
    const pcedom_attr_data_t *attr_data;

    pos = pchtml_str_data_ncasecmp_first(tkz->markup, data, (end - data));

    if (pos == NULL) {
        pchtml_html_token_attr_delete(tkz->token, tkz->token->attr_last,
                                   tkz->dobj_token_attr);

        pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                     PCHTML_HTML_TOKENIZER_ERROR_INCHSEAFDONA);

        tkz->state = pchtml_html_tokenizer_state_doctype_bogus;

        return data;
    }

    if (*pos == '\0') {
        pos = data + (pos - tkz->markup);

        pchtml_html_tokenizer_state_token_attr_set_name_end(tkz, pos);

        attr_data = pcedom_attr_data_by_id(tkz->attrs,
                                            PCEDOM_ATTR_PUBLIC);
        if (attr_data == NULL) {
            pcinst_set_error (PCHTML_ERROR);
            tkz->status = PCHTML_STATUS_ERROR;
            return end;
        }

        tkz->token->attr_last->name = attr_data;

        tkz->state = pchtml_html_tokenizer_state_doctype_after_public_keyword;

        return (pos + 1);
    }

    tkz->markup = pos;

    return end;
}

/*
 * Helper function. No in the specification. For 12.2.5.56
 * For doctype SYSTEM
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype_after_name_system(pchtml_html_tokenizer_t *tkz,
                                                   const unsigned char *data,
                                                   const unsigned char *end)
{
    const unsigned char *pos;
    const pcedom_attr_data_t *attr_data;

    pos = pchtml_str_data_ncasecmp_first(tkz->markup, data, (end - data));

    if (pos == NULL) {
        pchtml_html_token_attr_delete(tkz->token, tkz->token->attr_last,
                                   tkz->dobj_token_attr);

        pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                     PCHTML_HTML_TOKENIZER_ERROR_INCHSEAFDONA);

        tkz->state = pchtml_html_tokenizer_state_doctype_bogus;

        return data;
    }

    if (*pos == '\0') {
        pos = data + (pos - tkz->markup);

        pchtml_html_tokenizer_state_token_attr_set_name_end(tkz, pos);

        attr_data = pcedom_attr_data_by_id(tkz->attrs,
                                            PCEDOM_ATTR_SYSTEM);
        if (attr_data == NULL) {
            pcinst_set_error (PCHTML_ERROR);
            tkz->status = PCHTML_STATUS_ERROR;
            return end;
        }

        tkz->token->attr_last->name = attr_data;

        tkz->state = pchtml_html_tokenizer_state_doctype_after_system_keyword;

        return (pos + 1);
    }

    tkz->markup = pos;

    return end;
}

/*
 * 12.2.5.57 After DOCTYPE public keyword state
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype_after_public_keyword(pchtml_html_tokenizer_t *tkz,
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
            tkz->state =
                pchtml_html_tokenizer_state_doctype_before_public_identifier;

            return (data + 1);

        /* U+0022 QUOTATION MARK (") */
        case 0x22:
            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_MIWHAFDOPUKE);

            tkz->state =
               pchtml_html_tokenizer_state_doctype_public_identifier_double_quoted;

            return (data + 1);

        /* U+0027 APOSTROPHE (') */
        case 0x27:
            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_MIWHAFDOPUKE);

            tkz->state =
               pchtml_html_tokenizer_state_doctype_public_identifier_single_quoted;

            return (data + 1);

        /* U+003E GREATER-THAN SIGN (>) */
        case 0x3E:
            tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;
            tkz->state = pchtml_html_tokenizer_state_data_before;

            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_MIDOPUID);

            pchtml_html_tokenizer_state_token_done_m(tkz, end);

            return (data + 1);

        /* EOF */
        case 0x00:
            if (tkz->is_eof) {
                tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;

                pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                             PCHTML_HTML_TOKENIZER_ERROR_EOINDO);

                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return end;
            }
            /* fall through */

        default:
            tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;
            tkz->state = pchtml_html_tokenizer_state_doctype_bogus;

            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_MIQUBEDOPUID);

            return data;
    }

    return data;
}

/*
 * 12.2.5.58 Before DOCTYPE public identifier state
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype_before_public_identifier(pchtml_html_tokenizer_t *tkz,
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
            break;

        /* U+0022 QUOTATION MARK (") */
        case 0x22:
            tkz->state =
               pchtml_html_tokenizer_state_doctype_public_identifier_double_quoted;

            break;

        /* U+0027 APOSTROPHE (') */
        case 0x27:
            tkz->state =
               pchtml_html_tokenizer_state_doctype_public_identifier_single_quoted;

            break;

        /* U+003E GREATER-THAN SIGN (>) */
        case 0x3E:
            tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;
            tkz->state = pchtml_html_tokenizer_state_data_before;

            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_MIDOPUID);

            pchtml_html_tokenizer_state_token_done_m(tkz, end);

            break;

        /* EOF */
        case 0x00:
            if (tkz->is_eof) {
                pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                             PCHTML_HTML_TOKENIZER_ERROR_EOINDO);

                tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;

                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return end;
            }
            /* fall through */

        default:
            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_MIQUBEDOPUID);

            tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;
            tkz->state = pchtml_html_tokenizer_state_doctype_bogus;

            return data;
    }

    return (data + 1);
}

/*
 * 12.2.5.59 DOCTYPE public identifier (double-quoted) state
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype_public_identifier_double_quoted(pchtml_html_tokenizer_t *tkz,
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
                pchtml_html_tokenizer_state_set_value_m(tkz);
                pchtml_html_tokenizer_state_token_attr_set_value_end(tkz, data);

                tkz->state =
                    pchtml_html_tokenizer_state_doctype_after_public_identifier;

                return (data + 1);

            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                tkz->state = pchtml_html_tokenizer_state_data_before;

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_HTML_TOKENIZER_ERROR_ABDOPUID);

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_set_value_m(tkz);
                pchtml_html_tokenizer_state_token_attr_set_value_end(tkz, data);
                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return (data + 1);

            /* U+000D CARRIAGE RETURN (CR) */
            case 0x0D:
                if (++data >= end) {
                    pchtml_html_tokenizer_state_append_data_m(tkz, data - 1);

                    tkz->state = pchtml_html_tokenizer_state_cr;
                    tkz->state_return = pchtml_html_tokenizer_state_doctype_public_identifier_double_quoted;

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
                    pchtml_html_tokenizer_state_token_attr_set_value_end_oef(tkz);

                    if (tkz->token->attr_last->value_begin == NULL) {
                        pchtml_html_tokenizer_state_token_attr_set_value_begin(tkz,
                                            tkz->token->attr_last->value_end);
                    }

                    pchtml_html_tokenizer_error_add(tkz->parse_errors,
                                               tkz->token->attr_last->value_end,
                                               PCHTML_HTML_TOKENIZER_ERROR_EOINDO);

                    tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;

                    pchtml_html_tokenizer_state_set_value_m(tkz);
                    pchtml_html_tokenizer_state_token_done_m(tkz, end);

                    return end;
                }

                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);
                pchtml_html_tokenizer_state_append_replace_m(tkz);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_HTML_TOKENIZER_ERROR_UNNUCH);

                tkz->token->attr_last->type
                    |= PCHTML_HTML_TOKEN_ATTR_TYPE_VALUE_NULL;

                break;

            /* Anything else */
            default:
                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;
}

/*
 * 12.2.5.60 DOCTYPE public identifier (single-quoted) state
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype_public_identifier_single_quoted(pchtml_html_tokenizer_t *tkz,
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
                pchtml_html_tokenizer_state_set_value_m(tkz);
                pchtml_html_tokenizer_state_token_attr_set_value_end(tkz, data);

                tkz->state =
                    pchtml_html_tokenizer_state_doctype_after_public_identifier;

                return (data + 1);

            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                tkz->state = pchtml_html_tokenizer_state_data_before;

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_HTML_TOKENIZER_ERROR_ABDOPUID);

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_set_value_m(tkz);
                pchtml_html_tokenizer_state_token_attr_set_value_end(tkz, data);
                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return (data + 1);

            /* U+000D CARRIAGE RETURN (CR) */
            case 0x0D:
                if (++data >= end) {
                    pchtml_html_tokenizer_state_append_data_m(tkz, data - 1);

                    tkz->state = pchtml_html_tokenizer_state_cr;
                    tkz->state_return = pchtml_html_tokenizer_state_doctype_public_identifier_single_quoted;

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
                    pchtml_html_tokenizer_state_token_attr_set_value_end_oef(tkz);

                    if (tkz->token->attr_last->value_begin == NULL) {
                        pchtml_html_tokenizer_state_token_attr_set_value_begin(tkz,
                                              tkz->token->attr_last->value_end);
                    }

                    pchtml_html_tokenizer_error_add(tkz->parse_errors,
                                               tkz->token->attr_last->value_end,
                                               PCHTML_HTML_TOKENIZER_ERROR_EOINDO);

                    tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;

                    pchtml_html_tokenizer_state_set_value_m(tkz);
                    pchtml_html_tokenizer_state_token_done_m(tkz, end);

                    return end;
                }

                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);
                pchtml_html_tokenizer_state_append_replace_m(tkz);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_HTML_TOKENIZER_ERROR_UNNUCH);

                tkz->token->attr_last->type
                    |= PCHTML_HTML_TOKEN_ATTR_TYPE_VALUE_NULL;

                break;

            /* Anything else */
            default:
                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;
}

/*
 * 12.2.5.61 After DOCTYPE public identifier state
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype_after_public_identifier(pchtml_html_tokenizer_t *tkz,
                                                         const unsigned char *data,
                                                         const unsigned char *end)
{
    pchtml_html_token_attr_t *attr;

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
            tkz->state =
         pchtml_html_tokenizer_state_doctype_between_public_and_system_identifiers;

            return (data + 1);

        /* U+003E GREATER-THAN SIGN (>) */
        case 0x3E:
            tkz->state = pchtml_html_tokenizer_state_data_before;

            pchtml_html_tokenizer_state_token_done_m(tkz, end);

            return (data + 1);

        /* U+0022 QUOTATION MARK (") */
        case 0x22:
            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                     PCHTML_HTML_TOKENIZER_ERROR_MIWHBEDOPUANSYID);

            pchtml_html_tokenizer_state_token_attr_add_m(tkz, attr, end);

            tkz->state =
               pchtml_html_tokenizer_state_doctype_system_identifier_double_quoted;

            return (data + 1);

        /* U+0027 APOSTROPHE (') */
        case 0x27:
            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                     PCHTML_HTML_TOKENIZER_ERROR_MIWHBEDOPUANSYID);

            pchtml_html_tokenizer_state_token_attr_add_m(tkz, attr, end);

            tkz->state =
               pchtml_html_tokenizer_state_doctype_system_identifier_single_quoted;

            return (data + 1);

        /* EOF */
        case 0x00:
            if (tkz->is_eof) {
                pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                             PCHTML_HTML_TOKENIZER_ERROR_EOINDO);

                tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;
                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return end;
            }
            /* fall through */

        default:
            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_MIQUBEDOSYID);

            tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;
            tkz->state = pchtml_html_tokenizer_state_doctype_bogus;

            return data;
    }

    return data;
}

/*
 * 12.2.5.62 Between DOCTYPE public and system identifiers state
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype_between_public_and_system_identifiers(pchtml_html_tokenizer_t *tkz,
                                                                       const unsigned char *data,
                                                                       const unsigned char *end)
{
    pchtml_html_token_attr_t *attr;

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
            return (data + 1);

        /* U+003E GREATER-THAN SIGN (>) */
        case 0x3E:
            tkz->state = pchtml_html_tokenizer_state_data_before;

            pchtml_html_tokenizer_state_token_done_m(tkz, end);

            return (data + 1);

        /* U+0022 QUOTATION MARK (") */
        case 0x22:
            pchtml_html_tokenizer_state_token_attr_add_m(tkz, attr, end);

            tkz->state =
               pchtml_html_tokenizer_state_doctype_system_identifier_double_quoted;

            return (data + 1);

        /* U+0027 APOSTROPHE (') */
        case 0x27:
            pchtml_html_tokenizer_state_token_attr_add_m(tkz, attr, end);

            tkz->state =
               pchtml_html_tokenizer_state_doctype_system_identifier_single_quoted;

            return (data + 1);

        /* EOF */
        case 0x00:
            if (tkz->is_eof) {
                pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                             PCHTML_HTML_TOKENIZER_ERROR_EOINDO);

                tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;
                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return end;
            }
            /* fall through */

        default:
            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_MIQUBEDOSYID);

            tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;
            tkz->state = pchtml_html_tokenizer_state_doctype_bogus;

            return data;
    }

    return data;
}

/*
 * 12.2.5.63 After DOCTYPE system keyword state
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype_after_system_keyword(pchtml_html_tokenizer_t *tkz,
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
            tkz->state =
                pchtml_html_tokenizer_state_doctype_before_system_identifier;

            return (data + 1);

        /* U+0022 QUOTATION MARK (") */
        case 0x22:
            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_MIWHAFDOSYKE);

            tkz->state =
               pchtml_html_tokenizer_state_doctype_system_identifier_double_quoted;

            return (data + 1);

        /* U+0027 APOSTROPHE (') */
        case 0x27:
            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_MIWHAFDOSYKE);

            tkz->state =
               pchtml_html_tokenizer_state_doctype_system_identifier_single_quoted;

            return (data + 1);

        /* U+003E GREATER-THAN SIGN (>) */
        case 0x3E:
            tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;
            tkz->state = pchtml_html_tokenizer_state_data_before;

            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_MIDOSYID);

            pchtml_html_tokenizer_state_token_done_m(tkz, end);

            return (data + 1);

        /* EOF */
        case 0x00:
            if (tkz->is_eof) {
                pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                             PCHTML_HTML_TOKENIZER_ERROR_EOINDO);

                tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;
                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return end;
            }
            /* fall through */

        default:
            tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;
            tkz->state = pchtml_html_tokenizer_state_doctype_bogus;

            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_MIQUBEDOSYID);

            return data;
    }

    return data;
}

/*
 * 12.2.5.64 Before DOCTYPE system identifier state
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype_before_system_identifier(pchtml_html_tokenizer_t *tkz,
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
            return (data + 1);

        /* U+0022 QUOTATION MARK (") */
        case 0x22:
            tkz->state =
               pchtml_html_tokenizer_state_doctype_system_identifier_double_quoted;

            return (data + 1);

        /* U+0027 APOSTROPHE (') */
        case 0x27:
            tkz->state =
               pchtml_html_tokenizer_state_doctype_system_identifier_single_quoted;

            return (data + 1);

        /* U+003E GREATER-THAN SIGN (>) */
        case 0x3E:
            tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;
            tkz->state = pchtml_html_tokenizer_state_data_before;

            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_MIDOSYID);

            pchtml_html_tokenizer_state_token_done_m(tkz, end);

            return (data + 1);

        /* EOF */
        case 0x00:
            if (tkz->is_eof) {
                pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                             PCHTML_HTML_TOKENIZER_ERROR_EOINDO);

                tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;

                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return end;
            }
            /* fall through */

        default:
            tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;
            tkz->state = pchtml_html_tokenizer_state_doctype_bogus;

            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_MIQUBEDOSYID);

            return data;
    }

    return data;
}

/*
 * 12.2.5.65 DOCTYPE system identifier (double-quoted) state
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype_system_identifier_double_quoted(pchtml_html_tokenizer_t *tkz,
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
                pchtml_html_tokenizer_state_set_value_m(tkz);
                pchtml_html_tokenizer_state_token_attr_set_value_end(tkz, data);

                tkz->state =
                    pchtml_html_tokenizer_state_doctype_after_system_identifier;

                return (data + 1);

            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                tkz->state = pchtml_html_tokenizer_state_data_before;

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_HTML_TOKENIZER_ERROR_ABDOSYID);

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_set_value_m(tkz);
                pchtml_html_tokenizer_state_token_attr_set_value_end(tkz, data);
                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return (data + 1);

            /* U+000D CARRIAGE RETURN (CR) */
            case 0x0D:
                if (++data >= end) {
                    pchtml_html_tokenizer_state_append_data_m(tkz, data - 1);

                    tkz->state = pchtml_html_tokenizer_state_cr;
                    tkz->state_return = pchtml_html_tokenizer_state_doctype_system_identifier_double_quoted;

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
                    pchtml_html_tokenizer_state_token_attr_set_value_end_oef(tkz);

                    if (tkz->token->attr_last->value_begin == NULL) {
                        pchtml_html_tokenizer_state_token_attr_set_value_begin(tkz,
                                              tkz->token->attr_last->value_end);
                    }

                    pchtml_html_tokenizer_error_add(tkz->parse_errors,
                                               tkz->token->attr_last->value_end,
                                               PCHTML_HTML_TOKENIZER_ERROR_EOINDO);

                    tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;

                    pchtml_html_tokenizer_state_set_value_m(tkz);
                    pchtml_html_tokenizer_state_token_done_m(tkz, end);

                    return end;
                }

                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);
                pchtml_html_tokenizer_state_append_replace_m(tkz);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_HTML_TOKENIZER_ERROR_UNNUCH);

                tkz->token->attr_last->type
                    |= PCHTML_HTML_TOKEN_ATTR_TYPE_VALUE_NULL;

                break;

            /* Anything else */
            default:
                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;
}

/*
 * 12.2.5.66 DOCTYPE system identifier (single-quoted) state
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype_system_identifier_single_quoted(pchtml_html_tokenizer_t *tkz,
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
                pchtml_html_tokenizer_state_set_value_m(tkz);
                pchtml_html_tokenizer_state_token_attr_set_value_end(tkz, data);

                tkz->state =
                    pchtml_html_tokenizer_state_doctype_after_system_identifier;

                return (data + 1);

            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                tkz->state = pchtml_html_tokenizer_state_data_before;

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_HTML_TOKENIZER_ERROR_ABDOSYID);

                pchtml_html_tokenizer_state_append_data_m(tkz, data);
                pchtml_html_tokenizer_state_set_value_m(tkz);
                pchtml_html_tokenizer_state_token_attr_set_value_end(tkz, data);
                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return (data + 1);

            /* U+000D CARRIAGE RETURN (CR) */
            case 0x0D:
                if (++data >= end) {
                    pchtml_html_tokenizer_state_append_data_m(tkz, data - 1);

                    tkz->state = pchtml_html_tokenizer_state_cr;
                    tkz->state_return = pchtml_html_tokenizer_state_doctype_system_identifier_single_quoted;

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
                    pchtml_html_tokenizer_state_token_attr_set_value_end_oef(tkz);

                    if (tkz->token->attr_last->value_begin == NULL) {
                        pchtml_html_tokenizer_state_token_attr_set_value_begin(tkz,
                                              tkz->token->attr_last->value_end);
                    }

                    pchtml_html_tokenizer_error_add(tkz->parse_errors,
                                               tkz->token->attr_last->value_end,
                                               PCHTML_HTML_TOKENIZER_ERROR_EOINDO);

                    tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;

                    pchtml_html_tokenizer_state_set_value_m(tkz);
                    pchtml_html_tokenizer_state_token_done_m(tkz, end);

                    return end;
                }

                pchtml_html_tokenizer_state_begin_set(tkz, data + 1);
                pchtml_html_tokenizer_state_append_replace_m(tkz);

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_HTML_TOKENIZER_ERROR_UNNUCH);

                tkz->token->attr_last->type
                    |= PCHTML_HTML_TOKEN_ATTR_TYPE_VALUE_NULL;

                break;

            /* Anything else */
            default:
                break;
        }

        data++;
    }

    pchtml_html_tokenizer_state_append_data_m(tkz, data);

    return data;
}

/*
 * 12.2.5.67 After DOCTYPE system identifier state
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype_after_system_identifier(
                                                      pchtml_html_tokenizer_t *tkz,
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
                                             PCHTML_HTML_TOKENIZER_ERROR_EOINDO);

                tkz->token->type |= PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS;

                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return end;
            }
            /* fall through */

        default:
            pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_HTML_TOKENIZER_ERROR_UNCHAFDOSYID);

            tkz->state = pchtml_html_tokenizer_state_doctype_bogus;

            return data;
    }

    return data;
}

/*
 * 12.2.5.68 Bogus DOCTYPE state
 */
static const unsigned char *
pchtml_html_tokenizer_state_doctype_bogus(pchtml_html_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end)
{
    while (data != end) {
        switch (*data) {
            /* U+003E GREATER-THAN SIGN (>) */
            case 0x3E:
                tkz->state = pchtml_html_tokenizer_state_data_before;

                pchtml_html_tokenizer_state_token_done_m(tkz, end);

                return (data + 1);

            /*
             * U+0000 NULL
             * EOF
             */
            case 0x00:
                if (tkz->is_eof) {
                    pchtml_html_tokenizer_state_token_done_m(tkz, end);

                    return end;
                }

                pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                             PCHTML_HTML_TOKENIZER_ERROR_UNNUCH);

                break;

            /* Anything else */
            default:
                break;
        }

        data++;
    }

    return data;
}
