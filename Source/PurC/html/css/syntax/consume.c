/**
 * @file consume.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of css parse processing.
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


#include "html/css/syntax/consume.h"
#include "html/css/syntax/state.h"
#include "html/css/syntax/tokenizer/error.h"

#define PCHTML_CSS_SYNTAX_RES_NAME_MAP
#include "html/css/syntax/res.h"

#define PCHTML_STR_RES_MAP_HEX
#define PCHTML_STR_RES_MAP_LOWERCASE
#include "html/core/str_res.h"
#include "html/core/strtod.h"

#include "html/core/utils.h"


static const unsigned char pchtml_css_syntax_consume_url_ch[] = "url";
static const unsigned char *pchtml_css_syntax_consume_url_ch_end =
    pchtml_css_syntax_consume_url_ch + sizeof(pchtml_css_syntax_consume_url_ch) - 1;

static const unsigned char *
pchtml_css_syntax_consume_string_solidus(pchtml_css_syntax_tokenizer_t *tkz,
                                      const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_string_solidus_n(pchtml_css_syntax_tokenizer_t *tkz,
                                        const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_numeric_e(pchtml_css_syntax_tokenizer_t *tkz,
                                 const unsigned char *data,
                                 const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_numeric_e_digit(pchtml_css_syntax_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_numeric_e_digits(pchtml_css_syntax_tokenizer_t *tkz,
                                        const unsigned char *data,
                                        const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_numeric_name_start(pchtml_css_syntax_tokenizer_t *tkz,
                                          const unsigned char *data,
                                          const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_numeric_name_start_minus(pchtml_css_syntax_tokenizer_t *tkz,
                                                const unsigned char *data,
                                                const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_numeric_before_name_escape(pchtml_css_syntax_tokenizer_t *tkz,
                                                  const unsigned char *data,
                                                  const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_numeric_name(pchtml_css_syntax_tokenizer_t *tkz,
                                    const unsigned char *data,
                                    const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_numeric_name_escape(pchtml_css_syntax_tokenizer_t *tkz,
                                           const unsigned char *data,
                                           const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_ident_like_name(pchtml_css_syntax_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_ident_like_escape(pchtml_css_syntax_tokenizer_t *tkz,
                                         const unsigned char *data,
                                         const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_ident_like_solidus_data(pchtml_css_syntax_tokenizer_t *tkz,
                                               const unsigned char *data,
                                               const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_ident_like_is_function(pchtml_css_syntax_tokenizer_t *tkz,
                                              const unsigned char *data,
                                              const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_ident_like_before_check_url(pchtml_css_syntax_tokenizer_t *tkz,
                                                   const unsigned char *data,
                                                   const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_ident_like_check_url(pchtml_css_syntax_tokenizer_t *tkz,
                                            const unsigned char *data,
                                            const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_url(pchtml_css_syntax_tokenizer_t *tkz,
                           const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_url_escape(pchtml_css_syntax_tokenizer_t *tkz,
                                  const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_url_end(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_bad_url(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_consume_ident_like_not_url_escape(pchtml_css_syntax_tokenizer_t *tkz,
                                                 const unsigned char *data,
                                                 const unsigned char *end);


/*
 * String
 */
const unsigned char *
pchtml_css_syntax_consume_string(pchtml_css_syntax_tokenizer_t *tkz,
                              const unsigned char *data, const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_tokenizer_error_add(tkz->parse_errors,
                                           tkz->incoming_node->end,
                                         PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_EOINST);

        if (pchtml_css_syntax_token_string(tkz->token)->begin == NULL) {
            pchtml_css_syntax_token_string(tkz->token)->begin = tkz->incoming_node->end;
        }

        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_STRING;
        pchtml_css_syntax_token_string(tkz->token)->end = tkz->incoming_node->end;

        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    if (pchtml_css_syntax_token_string(tkz->token)->begin == NULL) {
        pchtml_css_syntax_token_string(tkz->token)->begin = data;
    }

    for (; data != end; data++) {
        switch (*data) {
            case 0x00:
                pchtml_css_syntax_token_have_null_set(tkz);

                break;

            /*
             * U+000A LINE FEED
             * U+000D CARRIAGE RETURN
             * U+000C FORM FEED
             */
            case 0x0A:
            case 0x0D:
            case 0x0C:
                pchtml_css_syntax_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_NEINST);

                pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_BAD_STRING;
                pchtml_css_syntax_token_string(tkz->token)->end = data;

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                pchtml_css_syntax_state_token_done_m(tkz, end);

                return data;

            /* U+005C REVERSE SOLIDUS (\) */
            case 0x5C:
                data += 1;

                if (data == end) {
                    pchtml_css_syntax_state_set(tkz,
                                         pchtml_css_syntax_consume_string_solidus);
                    return data;
                }

                pchtml_css_syntax_token_escaped_set(tkz);

                /* U+000D CARRIAGE RETURN */
                if (*data == 0x0D) {
                    data += 1;

                    pchtml_css_syntax_token_cr_set(tkz);

                    if (data == end) {
                        pchtml_css_syntax_state_set(tkz,
                                       pchtml_css_syntax_consume_string_solidus_n);
                        return data;
                    }

                    /* U+000A LINE FEED */
                    if (*data != 0x0A) {
                        data -= 1;
                    }

                    break;
                }
                else if (*data == 0x00) {
                    pchtml_css_syntax_token_have_null_set(tkz);
                }

                data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                                 pchtml_css_syntax_consume_string);
                if (data == end) {
                    return data;
                }

                data -= 1;

                break;

            default:
                /* '"' or ''' */
                if (*data == tkz->str_ending) {
                    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_STRING;
                    pchtml_css_syntax_token_string(tkz->token)->end = data;

                    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                    pchtml_css_syntax_state_token_done_m(tkz, end);

                    return (data + 1);
                }

                break;
        }
    }

    return data;
}

static const unsigned char *
pchtml_css_syntax_consume_string_solidus(pchtml_css_syntax_tokenizer_t *tkz,
                                      const unsigned char *data, const unsigned char *end)
{
    if (tkz->is_eof) {
        /* Do nothing */
        return end;
    }

    /* U+000D CARRIAGE RETURN */
    if (*data == 0x0D) {
        data += 1;

        pchtml_css_syntax_token_cr_set(tkz);

        if (data == end) {
            pchtml_css_syntax_state_set(tkz,
                                     pchtml_css_syntax_consume_string_solidus_n);
            return data;
        }

        /* U+000A LINE FEED */
        if (*data == 0x0A) {
            return (data + 1);
        }

        return data;
    }
    else if (*data == 0x00) {
        pchtml_css_syntax_token_have_null_set(tkz);
    }

    pchtml_css_syntax_token_escaped_set(tkz);

    data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                              pchtml_css_syntax_consume_string);
    if (data == end) {
        return data;
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_string);

    return data;
}

static const unsigned char *
pchtml_css_syntax_consume_string_solidus_n(pchtml_css_syntax_tokenizer_t *tkz,
                                        const unsigned char *data,
                                        const unsigned char *end)
{
    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_string);

    if (tkz->is_eof) {
        pchtml_css_syntax_tokenizer_error_add(tkz->parse_errors,
                                           tkz->incoming_node->end,
                                         PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_EOINST);

        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_STRING;
        pchtml_css_syntax_token_string(tkz->token)->end = tkz->incoming_node->end;

        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_string);

    /* U+000A LINE FEED */
    if (*data == 0x0A) {
        return (data + 1);
    }

    return data;
}

/*
 * Numeric
 */
static inline void
pchtml_css_syntax_consume_numeric_set_int(pchtml_css_syntax_tokenizer_t *tkz)
{
    double num = pchtml_strtod_internal(tkz->numeric.data,
                                        (tkz->numeric.buf - tkz->numeric.data),
                                        tkz->numeric.exponent);
    if (tkz->numeric.is_negative) {
        num = -num;
    }

    pchtml_css_syntax_token_number(tkz->token)->is_float = false;
    pchtml_css_syntax_token_number(tkz->token)->num = num;

    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_NUMBER;
}

static inline void
pchtml_css_syntax_consume_numeric_set_float(pchtml_css_syntax_tokenizer_t *tkz)
{
    if (tkz->numeric.e_is_negative) {
        tkz->numeric.exponent -= tkz->numeric.e_digit;
    }
    else {
        tkz->numeric.exponent += tkz->numeric.e_digit;
    }

    double num = pchtml_strtod_internal(tkz->numeric.data,
                                        (tkz->numeric.buf - tkz->numeric.data),
                                        tkz->numeric.exponent);
    if (tkz->numeric.is_negative) {
        num = -num;
    }

    pchtml_css_syntax_token_number(tkz->token)->num = num;

    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_NUMBER;
}

const unsigned char *
pchtml_css_syntax_consume_before_numeric(pchtml_css_syntax_tokenizer_t *tkz,
                                      const unsigned char *data,
                                      const unsigned char *end)
{
    tkz->numeric.buf = tkz->numeric.data;
    tkz->numeric.is_negative = false;

    return pchtml_css_syntax_consume_numeric(tkz, data, end);
}

const unsigned char *
pchtml_css_syntax_consume_numeric(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, const unsigned char *end)
{
    const unsigned char *begin;
    pchtml_css_syntax_token_number_t *token;
    pchtml_css_syntax_tokenizer_numeric_t *numeric;

    if (tkz->is_eof) {
        pchtml_css_syntax_consume_numeric_set_int(tkz);

        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    token = (pchtml_css_syntax_token_number_t *) tkz->token;
    numeric = &tkz->numeric;

    do {
        /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
        if (*data < 0x30 || *data > 0x39) {
            break;
        }

        if (numeric->buf != numeric->end) {
            *numeric->buf++ = *data;
        }

        data += 1;
        if (data == end) {
            pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric);

            return data;
        }
    }
    while (1);

    /* U+002E FULL STOP (.) */
    if (*data != 0x2E) {
        pchtml_css_syntax_consume_numeric_set_int(tkz);

        pchtml_css_syntax_token_dimension(tkz->token)->begin = data;

        return pchtml_css_syntax_consume_numeric_name_start(tkz, data, end);
    }

    /* Decimal */
    data += 1;
    begin = data;

    numeric->exponent = 0;

    /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
    for (; data < end && (*data >= 0x30 && *data <= 0x39); data++) {
        if (numeric->buf != numeric->end) {
            *numeric->buf++ = *data;
            numeric->exponent -= 1;
        }
    }

    if (data == end) {
        tkz->begin = begin - 1;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_decimal);

        return data;
    }

    if (numeric->exponent == 0) {
        pchtml_css_syntax_consume_numeric_set_int(tkz);

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return (begin - 1);
    }

    token->is_float = true;
    numeric->e_digit = 0;

    /* U+0045 Latin Capital Letter (E) or U+0065 Latin Small Letter (e) */
    if (*data != 0x45 && *data != 0x65) {
        pchtml_css_syntax_consume_numeric_set_float(tkz);

        pchtml_css_syntax_token_dimension(tkz->token)->begin = data;

        return pchtml_css_syntax_consume_numeric_name_start(tkz, data, end);
    }

    data += 1;
    numeric->e_is_negative = false;

    if (data == end) {
        pchtml_css_syntax_token_dimension(tkz->token)->begin = data - 1;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_e);

        return data;
    }

    switch (*data) {
        /* U+002D HYPHEN-MINUS (-) */
        case 0x2D:
            numeric->e_is_negative = true;
            /* fall through */

        /* U+002B PLUS SIGN (+) */
        case 0x2B:
            data += 1;

            if (data == end) {
                pchtml_css_syntax_token_dimension(tkz->token)->begin = data - 2;

                pchtml_css_syntax_state_set(tkz,
                                        pchtml_css_syntax_consume_numeric_e_digit);
                return data;
            }

            /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
            if (*data < 0x30 || *data > 0x39) {
                data -= 2;

                pchtml_css_syntax_consume_numeric_set_float(tkz);

                pchtml_css_syntax_token_dimension(tkz->token)->begin = data;
                pchtml_css_syntax_state_set(tkz,
                                         pchtml_css_syntax_consume_numeric_name);
                return data;
            }

            break;

        default:
            /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
            if (*data < 0x30 || *data > 0x39) {
                data -= 1;

                pchtml_css_syntax_consume_numeric_set_float(tkz);

                pchtml_css_syntax_token_dimension(tkz->token)->begin = data;
                pchtml_css_syntax_state_set(tkz,
                                         pchtml_css_syntax_consume_numeric_name);
                return data;
            }
    }

    /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
    for (; data < end && (*data >= 0x30 && *data <= 0x39); data++) {
        numeric->e_digit = (*data - 0x30) + numeric->e_digit * 0x0A;
    }

    if (data == end) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_e_digits);

        return data;
    }

    pchtml_css_syntax_consume_numeric_set_float(tkz);

    pchtml_css_syntax_token_dimension(tkz->token)->begin = data;
    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_name_start);

    return data;
}

const unsigned char *
pchtml_css_syntax_consume_numeric_decimal(pchtml_css_syntax_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end)
{
    pchtml_css_syntax_token_number_t *token;
    pchtml_css_syntax_tokenizer_numeric_t *numeric;

    token = (pchtml_css_syntax_token_number_t *) tkz->token;
    numeric = &tkz->numeric;

    if (tkz->is_eof) {
        if (numeric->exponent == 0) {
            goto data_state;
        }

        token->is_float = true;

        pchtml_css_syntax_consume_numeric_set_float(tkz);

        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_NUMBER;
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    do {
        if (*data < 0x30 || *data > 0x39) {
            break;
        }

        if (numeric->buf != numeric->end) {
            *numeric->buf++ = *data;
            numeric->exponent -= 1;
        }

        data += 1;
        if (data == end) {
            return data;
        }
    }
    while (1);

    if (numeric->exponent == 0) {
        goto data_state;
    }

    token->is_float = true;
    numeric->e_digit = 0;

    pchtml_css_syntax_token_dimension(tkz->token)->begin = data;

    /* U+0045 Latin Capital Letter (E) or U+0065 Latin Small Letter (e) */
    if (*data == 0x45 || *data == 0x65) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_e);

        return (data + 1);
    }

    pchtml_css_syntax_consume_numeric_set_float(tkz);

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_name_start);

    return data;


data_state:

    pchtml_css_syntax_consume_numeric_set_int(tkz);

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
    pchtml_css_syntax_state_token_done_m(tkz, end);

    return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
}

static const unsigned char *
pchtml_css_syntax_consume_numeric_e(pchtml_css_syntax_tokenizer_t *tkz,
                                 const unsigned char *data,
                                 const unsigned char *end)
{
    UNUSED_PARAM(end);

    /* U+002B PLUS SIGN (+) or U+002D HYPHEN-MINUS (-) */
    if (*data == 0x2B) {
        tkz->numeric.e_is_negative = false;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_e_digit);

        return (data + 1);
    }
    else if (*data == 0x2D) {
        tkz->numeric.e_is_negative = true;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_e_digit);

        return (data + 1);
    }
    /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
    else if (*data >= 0x30 && *data <= 0x39) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_e_digits);

        return data;
    }

    pchtml_css_syntax_consume_numeric_set_float(tkz);

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_name_start);

    return pchtml_css_syntax_tokenizer_change_incoming(tkz,
                             pchtml_css_syntax_token_dimension(tkz->token)->begin);
}

static const unsigned char *
pchtml_css_syntax_consume_numeric_e_digit(pchtml_css_syntax_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end)
{
    UNUSED_PARAM(end);

    /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
    if (*data >= 0x30 && *data <= 0x39) {
        tkz->numeric.e_digit = 0;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_e_digits);

        return data;
    }

    pchtml_css_syntax_consume_numeric_set_float(tkz);

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_name_start);

    return pchtml_css_syntax_tokenizer_change_incoming(tkz,
                             pchtml_css_syntax_token_dimension(tkz->token)->begin);
}

static const unsigned char *
pchtml_css_syntax_consume_numeric_e_digits(pchtml_css_syntax_tokenizer_t *tkz,
                                        const unsigned char *data,
                                        const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_consume_numeric_set_float(tkz);

        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_NUMBER;
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    do {
        /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
        if (*data < 0x30 || *data > 0x39) {
            break;
        }

        tkz->numeric.e_digit = (*data - 0x30) + tkz->numeric.e_digit * 0x0A;

        data += 1;
        if (data == end) {
            return data;
        }
    }
    while (1);

    pchtml_css_syntax_consume_numeric_set_float(tkz);

    pchtml_css_syntax_token_dimension(tkz->token)->begin = data;
    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_name_start);

    return data;
}

static const unsigned char *
pchtml_css_syntax_consume_numeric_name_start(pchtml_css_syntax_tokenizer_t *tkz,
                                          const unsigned char *data,
                                          const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_NUMBER;
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    if (pchtml_css_syntax_res_name_map[*data] == PCHTML_CSS_SYNTAX_RES_NAME_START) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_name);

        return (data + 1);
    }
    else if (*data == 0x00) {
        pchtml_css_syntax_token_have_null_set(tkz);
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_name);

        return (data + 1);
    }

    /* U+0025 PERCENTAGE SIGN (%) */
    if (*data == 0x25) {
        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_PERCENTAGE;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return (data + 1);
    }

    /* U+002D HYPHEN-MINUS */
    if (*data == 0x2D) {
        pchtml_css_syntax_state_set(tkz,
                               pchtml_css_syntax_consume_numeric_name_start_minus);
        return (data + 1);
    }

    /* U+005C REVERSE SOLIDUS (\) */
    if (*data == 0x5C) {
        pchtml_css_syntax_state_set(tkz,
                             pchtml_css_syntax_consume_numeric_before_name_escape);
        return (data + 1);
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
    pchtml_css_syntax_state_token_done_m(tkz, end);

    return data;
}

static const unsigned char *
pchtml_css_syntax_consume_numeric_name_start_minus(pchtml_css_syntax_tokenizer_t *tkz,
                                                const unsigned char *data,
                                                const unsigned char *end)
{
    if (pchtml_css_syntax_res_name_map[*data] == PCHTML_CSS_SYNTAX_RES_NAME_START) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_name);

        return (data + 1);
    }
    else if (*data == 0x00) {
        pchtml_css_syntax_token_have_null_set(tkz);
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_name);

        return (data + 1);
    }

    /* U+002D HYPHEN-MINUS */
    if (*data == 0x2D) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_name);

        return (data + 1);
    }

    /* U+005C REVERSE SOLIDUS (\) */
    if (*data == 0x5C) {
        pchtml_css_syntax_state_set(tkz,
                             pchtml_css_syntax_consume_numeric_before_name_escape);
        return (data + 1);
    }

    tkz->begin = pchtml_css_syntax_token_dimension(tkz->token)->begin;

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
    pchtml_css_syntax_state_token_done_m(tkz, end);

    return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
}

static const unsigned char *
pchtml_css_syntax_consume_numeric_before_name_escape(pchtml_css_syntax_tokenizer_t *tkz,
                                                  const unsigned char *data,
                                                  const unsigned char *end)
{
    /*
     * U+000A LINE FEED (LF)
     * U+000C FORM FEED (FF)
     * U+000D CARRIAGE RETURN (CR)
     */
    if ((*data == 0x0A || *data == 0x0C || *data == 0x0D) || tkz->is_eof) {
        tkz->begin = pchtml_css_syntax_token_dimension(tkz->token)->begin;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
    }
    else if (*data == 0x00) {
        pchtml_css_syntax_token_have_null_set(tkz);
    }

    pchtml_css_syntax_token_escaped_set(tkz);

    data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                           pchtml_css_syntax_consume_numeric_name);
    if (data == end) {
        return data;
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_name);

    return data;
}

static const unsigned char *
pchtml_css_syntax_consume_numeric_name(pchtml_css_syntax_tokenizer_t *tkz,
                                    const unsigned char *data,
                                    const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_token_dimension(tkz->token)->end = tkz->incoming_node->end;
        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_DIMENSION;

        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    for (; data < end; data++) {
        if (pchtml_css_syntax_res_name_map[*data] == 0x00) {
            /* U+0000 NULL (\0) */
            if (*data == 0x00) {
                pchtml_css_syntax_token_have_null_set(tkz);

                continue;
            }
            /* U+005C REVERSE SOLIDUS (\) */
            else if (*data != 0x5C) {
                pchtml_css_syntax_token_dimension(tkz->token)->end = data;
                pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_DIMENSION;

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                pchtml_css_syntax_state_token_done_m(tkz, end);

                return data;
            }

            data += 1;

            if (data == end) {
                pchtml_css_syntax_token_dimension(tkz->token)->end = data - 1;

                pchtml_css_syntax_state_set(tkz,
                                         pchtml_css_syntax_consume_numeric_name_escape);
                return data;
            }

            /*
             * U+000A LINE FEED (LF)
             * U+000C FORM FEED (FF)
             * U+000D CARRIAGE RETURN (CR)
             */
            if (*data == 0x0A || *data == 0x0C || *data == 0x0D) {
                data -= 1;

                pchtml_css_syntax_token_dimension(tkz->token)->end = data;
                pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_DIMENSION;

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                pchtml_css_syntax_state_token_done_m(tkz, end);

                return data;
            }
            else if (*data == 0x00) {
                pchtml_css_syntax_token_have_null_set(tkz);
            }

            pchtml_css_syntax_token_escaped_set(tkz);

            data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                           pchtml_css_syntax_consume_numeric_name);
            if (data == end) {
                return data;
            }

            data -= 1;
        }
    }

    return data;
}

static const unsigned char *
pchtml_css_syntax_consume_numeric_name_escape(pchtml_css_syntax_tokenizer_t *tkz,
                                           const unsigned char *data,
                                           const unsigned char *end)
{
    /*
     * U+000A LINE FEED (LF)
     * U+000C FORM FEED (FF)
     * U+000D CARRIAGE RETURN (CR)
     */
    if ((*data == 0x0A || *data == 0x0C || *data == 0x0D) || tkz->is_eof) {
        tkz->end = pchtml_css_syntax_token_dimension(tkz->token)->end;

        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_DIMENSION;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->end);
    }
    /* U+0000 NULL (\0) */
    else if (*data == 0x00) {
        pchtml_css_syntax_token_have_null_set(tkz);
    }

    pchtml_css_syntax_token_escaped_set(tkz);

    data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                           pchtml_css_syntax_consume_numeric_name);
    if (data == end) {
        return data;
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_name);

    return data;
}

/*
 * Ident-like
 */
const unsigned char *
pchtml_css_syntax_consume_ident_like(pchtml_css_syntax_tokenizer_t *tkz,
                                  const unsigned char *data, const unsigned char *end)
{
    tkz->end = pchtml_css_syntax_consume_url_ch;

    pchtml_css_syntax_token_ident(tkz->token)->begin = data;

    return pchtml_css_syntax_consume_ident_like_name(tkz, data, end);
}

static const unsigned char *
pchtml_css_syntax_consume_ident_like_name(pchtml_css_syntax_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end)
{
    /* Having come here, we have at least one char (not escaped). */

    if (tkz->is_eof) {
        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_IDENT;
        pchtml_css_syntax_token_ident(tkz->token)->end = tkz->incoming_node->end;

        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    /* Check for "url(" */

    do {
        /* U+005C REVERSE SOLIDUS (\) */
        if (*data == 0x5C) {
            data += 1;

            if (data == end) {
                tkz->begin = data - 1;

                pchtml_css_syntax_state_set(tkz,
                                      pchtml_css_syntax_consume_ident_like_escape);
                return data;
            }

            /*
             * U+000A LINE FEED (LF)
             * U+000C FORM FEED (FF)
             * U+000D CARRIAGE RETURN (CR)
             */
            if (*data == 0x0A || *data == 0x0C || *data == 0x0D) {
                data -= 1;

                pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_IDENT;
                pchtml_css_syntax_token_ident(tkz->token)->end = data;

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                pchtml_css_syntax_state_token_done_m(tkz, end);

                return data;
            }

            /* U+0000 NULL (\0) */
            if (*data == 0x00) {
                pchtml_css_syntax_token_have_null_set(tkz);
            }

            pchtml_css_syntax_token_escaped_set(tkz);

            tkz->num = 0;

            for (tkz->count = 0; tkz->count < 6; tkz->count++, data++) {
                if (data == end) {
                    pchtml_css_syntax_state_set(tkz,
                                pchtml_css_syntax_consume_ident_like_solidus_data);
                    return data;
                }

                if (pchtml_str_res_map_hex[*data] == 0xFF) {
                    if (tkz->count == 0) {
                        tkz->num = *data;

                        break;
                    }

                    /*
                     * U+0009 CHARACTER TABULATION
                     * U+0020 SPACE
                     * U+000A LINE FEED (LF)
                     * U+000C FORM FEED (FF)
                     * U+000D CARRIAGE RETURN (CR)
                     */
                    switch (*data) {
                        case 0x0D:
                            pchtml_css_syntax_token_cr_set(tkz);

                            data += 1;

                            if (data == end) {
                                goto url_escape_newline;
                            }

                            if (*data == 0x0A) {
                                data += 1;
                            }

                            break;

                        case 0x0C:
                            pchtml_css_syntax_token_ff_set(tkz);
                            /* fall through */

                        case 0x09:
                        case 0x20:
                        case 0x0A:
                            data += 1;
                            break;
                    }

                    break;
                }

                tkz->num <<= 4;
                tkz->num |= pchtml_str_res_map_hex[*data];
            }

            if (tkz->num == 0x00 || tkz->num > 0x80) {
                break;
            }

            if (*tkz->end != pchtml_str_res_map_lowercase[ (unsigned char) tkz->num ]) {
                break;
            }
        }
        else if (*tkz->end == pchtml_str_res_map_lowercase[*data]) {
            data += 1;
        }
        else {
            break;
        }

        tkz->end += 1;

        if (tkz->end == pchtml_css_syntax_consume_url_ch_end) {
            if (data == end) {
                pchtml_css_syntax_state_set(tkz,
                                 pchtml_css_syntax_consume_ident_like_is_function);
                return data;
            }

            /* U+0028 LEFT PARENTHESIS (() */
            if (*data == 0x28) {
                pchtml_css_syntax_token_function(tkz->token)->end = data;

                data += 1;
                tkz->end = NULL;

                if (data == end) {
                    pchtml_css_syntax_state_set(tkz,
                            pchtml_css_syntax_consume_ident_like_before_check_url);

                    return data;
                }

                tkz->begin = data;

                pchtml_css_syntax_state_set(tkz,
                                   pchtml_css_syntax_consume_ident_like_check_url);
                return data;
            }
        }

        if (data == end) {
            pchtml_css_syntax_state_set(tkz,
                                pchtml_css_syntax_consume_ident_like_name);
            return data;
        }
    }
    while (1);

    for (; data != end; data++) {
        if (pchtml_css_syntax_res_name_map[*data] == 0x00) {
            if (*data == 0x00) {
                pchtml_css_syntax_token_have_null_set(tkz);

                continue;
            }
            /* U+0028 LEFT PARENTHESIS (() */
            if (*data == 0x28) {
                pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_FUNCTION;
                pchtml_css_syntax_token_function(tkz->token)->end = data;

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                pchtml_css_syntax_state_token_done_m(tkz, end);

                return (data + 1);
            }
            /* U+005C REVERSE SOLIDUS (\) */
            else if (*data == 0x5C) {
                data += 1;

                if (data == end) {
                    tkz->end = data - 1;

                    pchtml_css_syntax_state_set(tkz,
                              pchtml_css_syntax_consume_ident_like_not_url_escape);

                    return data;
                }

                if (*data != 0x0A && *data != 0x0C && *data != 0x0D) {
                    if (*data == 0x00) {
                        pchtml_css_syntax_token_have_null_set(tkz);
                    }

                    pchtml_css_syntax_token_escaped_set(tkz);

                    data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                     pchtml_css_syntax_consume_ident_like_not_url);
                    if (data == end) {
                        return data;
                    }

                    data -= 1;

                    continue;
                }

                data -= 1;
            }

            pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_IDENT;
            pchtml_css_syntax_token_ident(tkz->token)->end = data;

            pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
            pchtml_css_syntax_state_token_done_m(tkz, end);

            return data;
        }
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_ident_like_not_url);

    return data;


url_escape_newline:

    tkz->state = pchtml_css_syntax_state_check_newline;
    tkz->return_state = pchtml_css_syntax_consume_ident_like_not_url;

    return data;
}

static const unsigned char *
pchtml_css_syntax_consume_ident_like_escape(pchtml_css_syntax_tokenizer_t *tkz,
                                          const unsigned char *data,
                                          const unsigned char *end)
{
    if ((*data == 0x0A || *data == 0x0C || *data == 0x0D) || tkz->is_eof) {
        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_IDENT;
        pchtml_css_syntax_token_ident(tkz->token)->end = tkz->begin;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->end);
    }
    else if (*data == 0x00) {
        pchtml_css_syntax_token_have_null_set(tkz);
    }

    pchtml_css_syntax_token_escaped_set(tkz);

    tkz->num = 0;
    tkz->count = 0;

    pchtml_css_syntax_state_set(tkz,
                             pchtml_css_syntax_consume_ident_like_solidus_data);
    return data;
}

static const unsigned char *
pchtml_css_syntax_consume_ident_like_solidus_data(pchtml_css_syntax_tokenizer_t *tkz,
                                               const unsigned char *data,
                                               const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_IDENT;

        pchtml_css_syntax_token_ident(tkz->token)->end = tkz->incoming_node->end;
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    for (; tkz->count < 6; tkz->count++, data++) {
        if (data == end) {
            return data;
        }

        if (pchtml_str_res_map_hex[*data] == 0xFF) {
            if (tkz->count == 0) {
                tkz->num = *data;

                break;
            }

            /*
             * U+0009 CHARACTER TABULATION
             * U+0020 SPACE
             * U+000A LINE FEED (LF)
             * U+000C FORM FEED (FF)
             * U+000D CARRIAGE RETURN (CR)
             */
            switch (*data) {
                case 0x0D:
                    pchtml_css_syntax_token_cr_set(tkz);

                    data += 1;

                    if (data == end) {
                        goto url_escape_newline;
                    }

                    if (*data == 0x0A) {
                        data += 1;
                    }

                    break;

                case 0x0C:
                    pchtml_css_syntax_token_ff_set(tkz);
                    /* fall through */

                case 0x09:
                case 0x20:
                case 0x0A:
                    data += 1;
                    break;
            }

            break;
        }

        tkz->num <<= 4;
        tkz->num |= pchtml_str_res_map_hex[*data];
    }

    /*
     * Zero, or is for a surrogate, or is greater than
     * the maximum allowed code point (tkz->num > 0x10FFFF).
     */
    if (tkz->num == 0x00 || tkz->num > 0x80) {
        pchtml_css_syntax_state_set(tkz,
                                 pchtml_css_syntax_consume_ident_like_not_url);
        return data;
    }

    if (*tkz->end != pchtml_str_res_map_lowercase[ (unsigned char) tkz->num ]) {
        pchtml_css_syntax_state_set(tkz,
                                 pchtml_css_syntax_consume_ident_like_not_url);
        return data;
    }

    tkz->end += 1;

    if (tkz->end == pchtml_css_syntax_consume_url_ch_end) {
        pchtml_css_syntax_state_set(tkz,
                                 pchtml_css_syntax_consume_ident_like_is_function);
        return data;
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_ident_like_name);

    return data;

url_escape_newline:

    tkz->state = pchtml_css_syntax_state_check_newline;

    tkz->return_state = pchtml_css_syntax_consume_ident_like_not_url;

    return data;
}

static const unsigned char *
pchtml_css_syntax_consume_ident_like_is_function(pchtml_css_syntax_tokenizer_t *tkz,
                                              const unsigned char *data,
                                              const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_IDENT;
        pchtml_css_syntax_token_ident(tkz->token)->end = tkz->incoming_node->end;

        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    /* U+0028 LEFT PARENTHESIS (() */
    if (*data == 0x28) {
        pchtml_css_syntax_token_ident(tkz->token)->end = data;

        data += 1;
        tkz->end = NULL;

        if (data == end) {
            pchtml_css_syntax_state_set(tkz,
                            pchtml_css_syntax_consume_ident_like_before_check_url);
            return data;
        }

        tkz->begin = data;

        pchtml_css_syntax_state_set(tkz,
                                 pchtml_css_syntax_consume_ident_like_check_url);
        return data;
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_ident_like_not_url);

    return data;
}

static const unsigned char *
pchtml_css_syntax_consume_ident_like_before_check_url(pchtml_css_syntax_tokenizer_t *tkz,
                                                   const unsigned char *data,
                                                   const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_FUNCTION;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    tkz->begin = data;

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_ident_like_check_url);

    return data;
}

static const unsigned char *
pchtml_css_syntax_consume_ident_like_check_url(pchtml_css_syntax_tokenizer_t *tkz,
                                            const unsigned char *data,
                                            const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_FUNCTION;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
    }

    if (pchtml_utils_whitespace(*data, !=, &&)) {
        /* U+0022 QUOTATION MARK (") or U+0027 APOSTROPHE (') */
        if (*data == 0x22 || *data == 0x27) {
            pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_FUNCTION;

            pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
            pchtml_css_syntax_state_token_done_m(tkz, end);

            if (tkz->end != NULL) {
                return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->end);
            }

            return data;
        }

        pchtml_css_syntax_token_url(tkz->token)->begin = data;
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_url);

        return data;
    }

    data += 1;

    for (; data != end; data++) {
        if (pchtml_utils_whitespace(*data, !=, &&)) {
            /* U+0022 QUOTATION MARK (") or U+0027 APOSTROPHE (') */
            if (*data == 0x22 || *data == 0x27) {
                pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_FUNCTION;

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                pchtml_css_syntax_state_token_done_m(tkz, end);

                return (data - 1);
            }

            pchtml_css_syntax_token_url(tkz->token)->begin = data;
            pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_url);

            return data;
        }
    }

    tkz->end = data - 1;

    return data;
}

/*
 * URL
 */
static const unsigned char *
pchtml_css_syntax_consume_url(pchtml_css_syntax_tokenizer_t *tkz,
                           const unsigned char *data, const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_EOINUR);

        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_URL;
        pchtml_css_syntax_token_url(tkz->token)->end = tkz->incoming_node->end;

        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    for (; data != end; data++) {
        switch (*data) {
            /* U+0000 NULL (\0) */
            case 0x00:
                pchtml_css_syntax_token_have_null_set(tkz);

                break;

            /* U+0029 RIGHT PARENTHESIS ()) */
            case 0x29:
                pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_URL;
                pchtml_css_syntax_token_url(tkz->token)->end = data;

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                pchtml_css_syntax_state_token_done_m(tkz, end);

                return (data + 1);

            /*
             * U+0022 QUOTATION MARK (")
             * U+0027 APOSTROPHE (')
             * U+0028 LEFT PARENTHESIS (()
             * U+000B LINE TABULATION
             * U+007F DELETE
             */
            case 0x22:
            case 0x27:
            case 0x28:
            case 0x0B:
            case 0x7F:
                pchtml_css_syntax_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_QOINUR);

                pchtml_css_syntax_token_url(tkz->token)->end = data;
                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_bad_url);

                return (data + 1);

            /* U+005C REVERSE SOLIDUS (\) */
            case 0x5C:
                data += 1;

                if (data == end) {
                    pchtml_css_syntax_token_url(tkz->token)->end = data;

                    pchtml_css_syntax_state_set(tkz,
                                             pchtml_css_syntax_consume_url_escape);
                    return data;
                }

                /*
                 * U+000A LINE FEED (LF)
                 * U+000C FORM FEED (FF)
                 * U+000D CARRIAGE RETURN (CR)
                 */
                if (*data == 0x0A || *data == 0x0C || *data == 0x0D) {
                    pchtml_css_syntax_tokenizer_error_add(tkz->parse_errors, data,
                                       PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_WRESINUR);

                    pchtml_css_syntax_token_url(tkz->token)->end = data;
                    pchtml_css_syntax_state_set(tkz,
                                             pchtml_css_syntax_consume_bad_url);
                    return data;
                }
                else if (*data == 0x00) {
                    pchtml_css_syntax_token_have_null_set(tkz);
                }

                pchtml_css_syntax_token_escaped_set(tkz);

                data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                                    pchtml_css_syntax_consume_url);
                if (data == end) {
                    return data;
                }

                data -= 1;

                break;
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
                pchtml_css_syntax_token_url(tkz->token)->end = data;

                data += 1;

                for (; data != end; data++) {
                    if (pchtml_utils_whitespace(*data, !=, &&)) {
                        /* U+0029 RIGHT PARENTHESIS ()) */
                        if (*data == 0x29) {
                            pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_URL;

                            pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                            pchtml_css_syntax_state_token_done_m(tkz, end);

                            return (data + 1);
                        }

                        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_bad_url);

                        return data;
                    }
                }

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_url_end);

                return data;

            default:
                /*
                 * Inclusive:
                 * U+0000 NULL and U+0008 BACKSPACE or
                 * U+000E SHIFT OUT and U+001F INFORMATION SEPARATOR ONE
                 */
                //if ((*data >= 0x00 && *data <= 0x08)      // gengyue
                if ((*data <= 0x08)
                    || (*data >= 0x0E && *data <= 0x1F))
                {
                    pchtml_css_syntax_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_QOINUR);

                    pchtml_css_syntax_token_url(tkz->token)->end = data;
                    pchtml_css_syntax_state_set(tkz,
                                             pchtml_css_syntax_consume_bad_url);
                    return (data + 1);
                }

                break;
        }
    }

    return data;
}

static const unsigned char *
pchtml_css_syntax_consume_url_escape(pchtml_css_syntax_tokenizer_t *tkz,
                                  const unsigned char *data, const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_tokenizer_error_add(tkz->parse_errors, data,
                                       PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_WRESINUR);

        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_BAD_URL;
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    /*
     * U+000A LINE FEED (LF)
     * U+000C FORM FEED (FF)
     * U+000D CARRIAGE RETURN (CR)
     */
    if (*data == 0x0A || *data == 0x0C || *data == 0x0D) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_bad_url);

        return (data + 1);
    }
    else if (*data == 0x00) {
        pchtml_css_syntax_token_have_null_set(tkz);
    }

    pchtml_css_syntax_token_escaped_set(tkz);

    data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                              pchtml_css_syntax_consume_url);
    if (data == end) {
        return data;
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_url);

    return (data + 1);
}

static const unsigned char *
pchtml_css_syntax_consume_url_end(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_tokenizer_error_add(tkz->parse_errors, data,
                                         PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_EOINUR);

        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_URL;
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    for (; data != end; data++) {
        if (pchtml_utils_whitespace(*data, !=, &&)) {
            /* U+0029 RIGHT PARENTHESIS ()) */
            if (*data == 0x29) {
                pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_URL;

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                pchtml_css_syntax_state_token_done_m(tkz, end);

                return (data + 1);
            }

            pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_bad_url);

            return data;
        }
    }

    return data;
}

/*
 * Bad URL
 */
static const unsigned char *
pchtml_css_syntax_consume_bad_url(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_BAD_URL;
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    for (; data != end; data++) {
        /* U+0029 RIGHT PARENTHESIS ()) */
        if (*data == 0x29) {
            pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_BAD_URL;

            pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
            pchtml_css_syntax_state_token_done_m(tkz, end);

            return (data + 1);
        }
        /* U+005C REVERSE SOLIDUS (\) */
        else if (*data == 0x5C) {
            data += 1;
        }
    }

    return data;
}

const unsigned char *
pchtml_css_syntax_consume_ident_like_not_url(pchtml_css_syntax_tokenizer_t *tkz,
                                          const unsigned char *data,
                                          const unsigned char *end)
{
    if (tkz->is_eof) {
        if (pchtml_css_syntax_token_ident(tkz->token)->begin == NULL) {
            pchtml_css_syntax_token_ident(tkz->token)->begin = tkz->incoming_node->end;
        }

        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_IDENT;
        pchtml_css_syntax_token_ident(tkz->token)->end = tkz->incoming_node->end;

        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    if (pchtml_css_syntax_token_ident(tkz->token)->begin == NULL) {
        pchtml_css_syntax_token_ident(tkz->token)->begin = data;
    }

    for (; data != end; data++) {
        if (pchtml_css_syntax_res_name_map[*data] == 0x00) {
            /* U+0028 LEFT PARENTHESIS (() */
            if (*data == 0x28) {
                pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_FUNCTION;
                pchtml_css_syntax_token_function(tkz->token)->end = data;

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                pchtml_css_syntax_state_token_done_m(tkz, end);

                return (data + 1);
            }
            /* U+005C REVERSE SOLIDUS (\) */
            else if (*data == 0x5C) {
                data += 1;

                if (data == end) {
                    tkz->end = data - 1;

                    pchtml_css_syntax_state_set(tkz,
                              pchtml_css_syntax_consume_ident_like_not_url_escape);

                    return data;
                }

                if (*data != 0x0A && *data != 0x0C && *data != 0x0D) {
                    if (*data == 0x00) {
                        pchtml_css_syntax_token_have_null_set(tkz);
                    }

                    pchtml_css_syntax_token_escaped_set(tkz);

                    data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                     pchtml_css_syntax_consume_ident_like_not_url);
                    if (data == end) {
                        return data;
                    }

                    data -= 1;

                    continue;
                }

                data -= 1;
            }
            else if (*data == 0x00) {
                pchtml_css_syntax_token_have_null_set(tkz);

                continue;
            }

            pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_IDENT;
            pchtml_css_syntax_token_ident(tkz->token)->end = data;

            pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
            pchtml_css_syntax_state_token_done_m(tkz, end);

            return data;
        }
    }

    return data;
}

static const unsigned char *
pchtml_css_syntax_consume_ident_like_not_url_escape(pchtml_css_syntax_tokenizer_t *tkz,
                                                 const unsigned char *data,
                                                 const unsigned char *end)
{
    if ((*data == 0x0A || *data == 0x0C || *data == 0x0D) || tkz->is_eof) {
        pchtml_css_syntax_token_string(tkz->token)->end = tkz->end;
        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_IDENT;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->end);
    }
    else if (*data == 0x00) {
        pchtml_css_syntax_token_have_null_set(tkz);
    }

    pchtml_css_syntax_token_escaped_set(tkz);

    data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                     pchtml_css_syntax_consume_ident_like_not_url);
    if (data == end) {
        return data;
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_ident_like_not_url);

    return data;
}
