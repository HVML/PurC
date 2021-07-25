/**
 * @file state.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of css state.
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


#include <string.h>

#include "html/core/utils.h"

#include "html/css/syntax/state.h"
#include "html/css/syntax/consume.h"
#include "html/css/syntax/state_res.h"
#include "html/css/syntax/tokenizer/error.h"


#define PCHTML_CSS_SYNTAX_RES_NAME_MAP
#include "html/css/syntax/res.h"


static const unsigned char *
pchtml_css_syntax_state_comment_slash(pchtml_css_syntax_tokenizer_t *tkz,
                                   const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_comment(pchtml_css_syntax_tokenizer_t *tkz,
                             const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_comment_end(pchtml_css_syntax_tokenizer_t *tkz,
                                 const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_hash_name(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_hash_escape(pchtml_css_syntax_tokenizer_t *tkz,
                                 const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_hash_consume_name(pchtml_css_syntax_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_hash_consume_name_escape(pchtml_css_syntax_tokenizer_t *tkz,
                                              const unsigned char *data,
                                              const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_plus_check(pchtml_css_syntax_tokenizer_t *tkz,
                                const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_plus_check_digit(pchtml_css_syntax_tokenizer_t *tkz,
                                      const unsigned char *data,
                                      const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_minus_check(pchtml_css_syntax_tokenizer_t *tkz,
                                 const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_minus_check_digit(pchtml_css_syntax_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_minus_check_cdc(pchtml_css_syntax_tokenizer_t *tkz,
                                     const unsigned char *data,
                                     const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_minus_check_solidus(pchtml_css_syntax_tokenizer_t *tkz,
                                         const unsigned char *data,
                                         const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_full_stop_num(pchtml_css_syntax_tokenizer_t *tkz,
                                   const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_less_sign_check_exmark(pchtml_css_syntax_tokenizer_t *tkz,
                                            const unsigned char *data,
                                            const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_less_sign_check_fminus(pchtml_css_syntax_tokenizer_t *tkz,
                                            const unsigned char *data,
                                            const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_less_sign_check_tminus(pchtml_css_syntax_tokenizer_t *tkz,
                                            const unsigned char *data,
                                            const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_at_begin(pchtml_css_syntax_tokenizer_t *tkz,
                              const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_at_minus(pchtml_css_syntax_tokenizer_t *tkz,
                              const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_at_escape(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_at_name(pchtml_css_syntax_tokenizer_t *tkz,
                             const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_at_name_escape(pchtml_css_syntax_tokenizer_t *tkz,
                                    const unsigned char *data,
                                    const unsigned char *end);

static const unsigned char *
pchtml_css_syntax_state_rsolidus_check(pchtml_css_syntax_tokenizer_t *tkz,
                                    const unsigned char *data,
                                    const unsigned char *end);


const unsigned char *
pchtml_css_syntax_state_data(pchtml_css_syntax_tokenizer_t *tkz,
                          const unsigned char *data, const unsigned char *end)
{
    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_res_map[*data]);

    return tkz->state(tkz, data, end);
}

const unsigned char *
pchtml_css_syntax_state_delim(pchtml_css_syntax_tokenizer_t *tkz,
                           const unsigned char *data, const unsigned char *end)
{
    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_DELIM;
    pchtml_css_syntax_token_delim(tkz->token)->character = (uint32_t) *data;

    pchtml_css_syntax_token_delim(tkz->token)->begin = data;
    data += 1;
    pchtml_css_syntax_token_delim(tkz->token)->end = data;

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
    pchtml_css_syntax_state_token_done_m(tkz, end);

    return data;
}

/*
 * END-OF-FILE
 */
const unsigned char *
pchtml_css_syntax_state_eof(pchtml_css_syntax_tokenizer_t *tkz,
                         const unsigned char *data, const unsigned char *end)
{
    if (tkz->is_eof) {
        return end;
    }

    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_IDENT;
    pchtml_css_syntax_token_have_null_set(tkz);

    pchtml_css_syntax_token_ident(tkz->token)->begin = data;
    data += 1;
    pchtml_css_syntax_token_ident(tkz->token)->end = data;

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
    pchtml_css_syntax_state_token_done_m(tkz, end);

    return data;
}

/*
 * Comment
 */
const unsigned char *
pchtml_css_syntax_state_comment_begin(pchtml_css_syntax_tokenizer_t *tkz,
                                   const unsigned char *data, const unsigned char *end)
{
    /* Skip forward slash (/) */
    data += 1;

    if (data == end) {
        tkz->begin = data - 1;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_comment_slash);

        return data;
    }

    /* U+002A ASTERISK (*) */
    if (*data == 0x2A) {
        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_COMMENT;
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_comment);

        return (data + 1);
    }

    return pchtml_css_syntax_state_delim(tkz, (data - 1), end);
}

static const unsigned char *
pchtml_css_syntax_state_comment_slash(pchtml_css_syntax_tokenizer_t *tkz,
                                   const unsigned char *data, const unsigned char *end)
{
    UNUSED_PARAM(end);

    /* U+002A ASTERISK (*) */
    if (*data == 0x2A) {
        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_COMMENT;
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_comment);

        return (data + 1);
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

    return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
}

static const unsigned char *
pchtml_css_syntax_state_comment(pchtml_css_syntax_tokenizer_t *tkz,
                             const unsigned char *data, const unsigned char *end)
{
    /* EOF */
    if (tkz->is_eof) {
        pchtml_css_syntax_tokenizer_error_add(tkz->parse_errors,
                                           tkz->incoming_node->end,
                                           PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_EOINCO);

        pchtml_css_syntax_token_comment(tkz->token)->end = tkz->incoming_node->end;
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    if (pchtml_css_syntax_token_comment(tkz->token)->begin == NULL) {
        pchtml_css_syntax_token_comment(tkz->token)->begin = data;
    }

    for (; data != end; data++) {
        switch (*data) {
            case 0x00:
                pchtml_css_syntax_token_have_null_set(tkz);
                /* fall through */

            case 0x0D:
                pchtml_css_syntax_token_cr_set(tkz);
                /* fall through */

            case 0x0C:
                pchtml_css_syntax_token_ff_set(tkz);
                break;

            /* U+002A ASTERISK (*) */
            case 0x2A:
                data += 1;

                if (data == end) {
                    pchtml_css_syntax_token_comment(tkz->token)->end = data - 1;

                    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_comment_end);

                    return data;
                }

                /* U+002F Forward slash (/) */
                if (*data == 0x2F) {
                    pchtml_css_syntax_token_comment(tkz->token)->end = data - 1;

                    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                    pchtml_css_syntax_state_token_done_m(tkz, end);

                    return (data + 1);
                }

                data -= 1;

                break;
        }
    }

    return data;
}

static const unsigned char *
pchtml_css_syntax_state_comment_end(pchtml_css_syntax_tokenizer_t *tkz,
                                 const unsigned char *data, const unsigned char *end)
{
    /* EOF */
    if (tkz->is_eof) {
        pchtml_css_syntax_tokenizer_error_add(tkz->parse_errors,
                                           tkz->incoming_node->end,
                                           PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_EOINCO);

        pchtml_css_syntax_token_comment(tkz->token)->end = tkz->incoming_node->end;
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    /* U+002F Forward slash (/) */
    if (*data == 0x2F) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return (data + 1);
    }

    tkz->state = pchtml_css_syntax_state_comment;

    return data;
}

/*
 * Whitespace
 */
const unsigned char *
pchtml_css_syntax_state_whitespace(pchtml_css_syntax_tokenizer_t *tkz,
                                const unsigned char *data, const unsigned char *end)
{
    if (pchtml_css_syntax_token_whitespace(tkz->token)->begin == NULL) {
        pchtml_css_syntax_token_whitespace(tkz->token)->begin = data;
        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_WHITESPACE;
    }
    else if (tkz->is_eof) {
        pchtml_css_syntax_token_whitespace(tkz->token)->end = tkz->incoming_node->end;
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    for (; data != end; data++) {
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
                break;

            case 0x0C:
                pchtml_css_syntax_token_ff_set(tkz);
                /* fall through */

            case 0x09:
            case 0x20:
            case 0x0A:
                break;

            default:
                pchtml_css_syntax_token_whitespace(tkz->token)->end = data;

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                pchtml_css_syntax_state_token_done_m(tkz, end);

                return data;
        }
    }

    return data;
}

/*
 * String token for U+0022 Quotation Mark (") and U+0027 Apostrophe (')
 */
const unsigned char *
pchtml_css_syntax_state_string(pchtml_css_syntax_tokenizer_t *tkz,
                            const unsigned char *data, const unsigned char *end)
{
    UNUSED_PARAM(end);

    tkz->str_ending = *data;

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_string);

    return (data + 1);
}

/*
 * U+0023 NUMBER SIGN (#)
 */
const unsigned char *
pchtml_css_syntax_state_hash(pchtml_css_syntax_tokenizer_t *tkz,
                          const unsigned char *data, const unsigned char *end)
{
    /* Skip Number Sign (#) char */
    data += 1;

    if (data == end) {
        /*
         * Save position for the Number Sign (#) for the delim token
         * if some tests will be failed.
         */
        tkz->begin = data - 1;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_hash_name);

        return data;
    }

    pchtml_css_syntax_token_hash(tkz->token)->begin = data;

    if (pchtml_css_syntax_res_name_map[*data] == 0x00) {
        /* U+005C REVERSE SOLIDUS (\) */
        if (*data == 0x5C) {
            data += 1;

            if (data == end) {
                tkz->begin = data - 2;
                pchtml_css_syntax_token_hash(tkz->token)->begin = data - 1;

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_hash_escape);

                return data;
            }

            /*
             * U+000A LINE FEED (LF)
             * U+000C FORM FEED (FF)
             * U+000D CARRIAGE RETURN (CR)
             */
            if (*data == 0x0A || *data == 0x0C || *data == 0x0D) {
                return pchtml_css_syntax_state_delim(tkz, (data - 2), end);
            }
            else if (*data == 0x00) {
                pchtml_css_syntax_token_have_null_set(tkz);
            }

            pchtml_css_syntax_token_escaped_set(tkz);

            data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                        pchtml_css_syntax_state_hash_consume_name);
            if (data == end) {
                pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_HASH;

                return data;
            }
        }
        else if (*data == 0x00) {
            pchtml_css_syntax_token_have_null_set(tkz);
        }
        else {
            return pchtml_css_syntax_state_delim(tkz, (data - 1), end);
        }
    }

    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_HASH;

    for (; data < end; data++) {
        if (pchtml_css_syntax_res_name_map[*data] == 0x00) {
            /* U+005C REVERSE SOLIDUS (\) */
            if (*data == 0x5C) {
                data += 1;

                if (data == end) {
                    tkz->end = data - 1;
                    pchtml_css_syntax_token_hash(tkz->token)->end = tkz->end;

                    pchtml_css_syntax_state_set(tkz,
                                 pchtml_css_syntax_state_hash_consume_name_escape);
                    return data;
                }

                /*
                 * U+000A LINE FEED (LF)
                 * U+000C FORM FEED (FF)
                 * U+000D CARRIAGE RETURN (CR)
                 */
                if (*data == 0x0A || *data == 0x0C || *data == 0x0D) {
                    data -= 1;

                    pchtml_css_syntax_token_hash(tkz->token)->end = data;

                    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                    pchtml_css_syntax_state_token_done_m(tkz, end);

                    return data;
                }
                else if (*data == 0x00) {
                    pchtml_css_syntax_token_have_null_set(tkz);
                }

                pchtml_css_syntax_token_escaped_set(tkz);

                data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                        pchtml_css_syntax_state_hash_consume_name);
                if (data == end) {
                    return data;
                }

                data -= 1;
            }
            else if (*data == 0x00) {
                pchtml_css_syntax_token_have_null_set(tkz);
            }
            else {
                pchtml_css_syntax_token_hash(tkz->token)->end = data;

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                pchtml_css_syntax_state_token_done_m(tkz, end);

                return data;
            }
        }
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_hash_consume_name);

    return data;
}

static const unsigned char *
pchtml_css_syntax_state_hash_name(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
    }

    if (pchtml_css_syntax_res_name_map[*data] == 0x00) {
        /* U+005C REVERSE SOLIDUS (\) */
        if (*data == 0x5C) {
            data += 1;

            if (data == end) {
                pchtml_css_syntax_token_hash(tkz->token)->begin = data - 1;
                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_hash_escape);

                return data;
            }

            /*
             * U+000A LINE FEED (LF)
             * U+000C FORM FEED (FF)
             * U+000D CARRIAGE RETURN (CR)
             */
            if (*data == 0x0A || *data == 0x0C || *data == 0x0D) {
                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

                return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
            }
            else if (*data == 0x00) {
                pchtml_css_syntax_token_have_null_set(tkz);
            }

            pchtml_css_syntax_token_escaped_set(tkz);

            pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_HASH;
            pchtml_css_syntax_token_hash(tkz->token)->begin = data - 1;

            data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                        pchtml_css_syntax_state_hash_consume_name);
            if (data == end) {
                return data;
            }

            pchtml_css_syntax_state_set(tkz,
                                     pchtml_css_syntax_state_hash_consume_name);
            return data;
        }
        else if (*data == 0x00) {
            pchtml_css_syntax_token_have_null_set(tkz);
        }
        else {
            pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

            return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
        }
    }

    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_HASH;
    pchtml_css_syntax_token_hash(tkz->token)->begin = data;

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_hash_consume_name);

    return (data + 1);
}

static const unsigned char *
pchtml_css_syntax_state_hash_escape(pchtml_css_syntax_tokenizer_t *tkz,
                                 const unsigned char *data, const unsigned char *end)
{
    /*
     * U+000A LINE FEED (LF)
     * U+000C FORM FEED (FF)
     * U+000D CARRIAGE RETURN (CR)
     */
    if ((*data == 0x0A || *data == 0x0C || *data == 0x0D) || tkz->is_eof) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
    }
    else if (*data == 0x00) {
        pchtml_css_syntax_token_have_null_set(tkz);
    }

    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_HASH;
    pchtml_css_syntax_token_escaped_set(tkz);

    data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                        pchtml_css_syntax_state_hash_consume_name);
    if (data == end) {
        return data;
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_hash_consume_name);

    return data;
}

static const unsigned char *
pchtml_css_syntax_state_hash_consume_name(pchtml_css_syntax_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_token_hash(tkz->token)->end = tkz->incoming_node->end;

        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    for (; data < end; data++) {
        if (pchtml_css_syntax_res_name_map[*data] == 0x00) {
            /* U+005C REVERSE SOLIDUS (\) */
            if (*data == 0x5C) {
                data += 1;

                if (data == end) {
                    tkz->end = data - 1;
                    pchtml_css_syntax_token_hash(tkz->token)->end = tkz->end;

                    pchtml_css_syntax_state_set(tkz,
                                 pchtml_css_syntax_state_hash_consume_name_escape);
                    return data;
                }

                /*
                 * U+000A LINE FEED (LF)
                 * U+000C FORM FEED (FF)
                 * U+000D CARRIAGE RETURN (CR)
                 */
                if (*data == 0x0A || *data == 0x0C || *data == 0x0D) {
                    data -= 1;

                    pchtml_css_syntax_token_hash(tkz->token)->end = data;

                    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                    pchtml_css_syntax_state_token_done_m(tkz, end);

                    return data;
                }
                else if (*data == 0x00) {
                    pchtml_css_syntax_token_have_null_set(tkz);
                }

                pchtml_css_syntax_token_escaped_set(tkz);

                data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                        pchtml_css_syntax_state_hash_consume_name);
                if (data == end) {
                    return data;
                }

                data -= 1;
            }
            else if (*data == 0x00) {
                pchtml_css_syntax_token_have_null_set(tkz);
            }
            else {
                pchtml_css_syntax_token_hash(tkz->token)->end = data;

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                pchtml_css_syntax_state_token_done_m(tkz, end);

                return data;
            }
        }
    }

    return data;
}

static const unsigned char *
pchtml_css_syntax_state_hash_consume_name_escape(pchtml_css_syntax_tokenizer_t *tkz,
                                              const unsigned char *data,
                                              const unsigned char *end)
{
    /*
     * U+000A LINE FEED (LF)
     * U+000C FORM FEED (FF)
     * U+000D CARRIAGE RETURN (CR)
     */
    if (*data == 0x0A || *data == 0x0C || *data == 0x0D || tkz->is_eof) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->end);
    }
    else if (*data == 0x00) {
        pchtml_css_syntax_token_have_null_set(tkz);
    }

    pchtml_css_syntax_token_escaped_set(tkz);

    data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                        pchtml_css_syntax_state_hash_consume_name);
    if (data == end) {
        return data;
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_hash_consume_name);

    return data;
}

/*
 * U+0028 LEFT PARENTHESIS (()
 */
const unsigned char *
pchtml_css_syntax_state_lparenthesis(pchtml_css_syntax_tokenizer_t *tkz,
                                  const unsigned char *data, const unsigned char *end)
{
    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_L_PARENTHESIS;

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
    pchtml_css_syntax_state_token_done_m(tkz, end);

    return (data + 1);
}

/*
 * U+0029 RIGHT PARENTHESIS ())
 */
const unsigned char *
pchtml_css_syntax_state_rparenthesis(pchtml_css_syntax_tokenizer_t *tkz,
                                  const unsigned char *data, const unsigned char *end)
{
    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_R_PARENTHESIS;

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
    pchtml_css_syntax_state_token_done_m(tkz, end);

    return (data + 1);
}

/*
 * U+002B PLUS SIGN (+)
 */
const unsigned char *
pchtml_css_syntax_state_plus(pchtml_css_syntax_tokenizer_t *tkz,
                          const unsigned char *data, const unsigned char *end)
{
    /* Skip + */
    data += 1;

    if (data == end) {
        tkz->begin = data - 1;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_plus_check);

        return data;
    }

    /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
    if (*data >= 0x30 && *data <= 0x39) {
        tkz->numeric.is_negative = false;
        tkz->numeric.buf = tkz->numeric.data;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric);

        return data;
    }

    /* U+002E FULL STOP (.) */
    if (*data == 0x2E) {
        data += 1;

        if (data == end) {
            tkz->begin = data - 2;

            pchtml_css_syntax_state_set(tkz,
                                     pchtml_css_syntax_state_plus_check_digit);
            return data;
        }

        /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
        if (*data >= 0x30 && *data <= 0x39) {
            tkz->numeric.exponent = 0;
            tkz->numeric.is_negative = false;
            tkz->numeric.buf = tkz->numeric.data;

            pchtml_css_syntax_state_set(tkz,
                                     pchtml_css_syntax_consume_numeric_decimal);
            return data;
        }

        data -= 1;
    }

    return pchtml_css_syntax_state_delim(tkz, (data - 1), end);
}

static const unsigned char *
pchtml_css_syntax_state_plus_check(pchtml_css_syntax_tokenizer_t *tkz,
                                const unsigned char *data, const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
    }

    /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
    if (*data >= 0x30 && *data <= 0x39) {
        tkz->numeric.is_negative = false;
        tkz->numeric.buf = tkz->numeric.data;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric);

        return data;
    }

    /* U+002E FULL STOP (.) */
    if (*data == 0x2E) {
        data += 1;

        if (data == end) {
            pchtml_css_syntax_state_set(tkz,
                                     pchtml_css_syntax_state_plus_check_digit);
            return data;
        }

        /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
        if (*data >= 0x30 && *data <= 0x39) {
            tkz->numeric.exponent = 0;
            tkz->numeric.is_negative = false;
            tkz->numeric.buf = tkz->numeric.data;

            pchtml_css_syntax_state_set(tkz,
                                     pchtml_css_syntax_consume_numeric_decimal);
            return data;
        }
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

    return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
}

static const unsigned char *
pchtml_css_syntax_state_plus_check_digit(pchtml_css_syntax_tokenizer_t *tkz,
                                      const unsigned char *data,
                                      const unsigned char *end)
{
    UNUSED_PARAM(end);

    if (tkz->is_eof) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
    }

    /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
    if (*data >= 0x30 && *data <= 0x39) {
        tkz->numeric.exponent = 0;
        tkz->numeric.is_negative = false;
        tkz->numeric.buf = tkz->numeric.data;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_decimal);

        return data;
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

    return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
}

/*
 * U+002C COMMA (,)
 */
const unsigned char *
pchtml_css_syntax_state_comma(pchtml_css_syntax_tokenizer_t *tkz,
                           const unsigned char *data, const unsigned char *end)
{
    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_COMMA;

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
    pchtml_css_syntax_state_token_done_m(tkz, end);

    return (data + 1);
}

/*
 * U+002D HYPHEN-MINUS (-)
 */
const unsigned char *
pchtml_css_syntax_state_minus(pchtml_css_syntax_tokenizer_t *tkz,
                           const unsigned char *data, const unsigned char *end)
{
    /* Skip minus (-) */
    data += 1;

    /* Check for <number-token> */

    if (data == end) {
        tkz->begin = data - 1;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_minus_check);

        return data;
    }

    /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
    if (*data >= 0x30 && *data <= 0x39) {
        tkz->numeric.is_negative = true;
        tkz->numeric.buf = tkz->numeric.data;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric);

        return data;
    }

    /* U+002E FULL STOP (.) */
    if (*data == 0x2E) {
        data += 1;

        if (data == end) {
            tkz->begin = data - 2;

            pchtml_css_syntax_state_set(tkz,
                                     pchtml_css_syntax_state_minus_check_digit);
            return data;
        }

        /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
        if (*data >= 0x30 && *data <= 0x39) {
            tkz->numeric.exponent = 0;
            tkz->numeric.is_negative = true;
            tkz->numeric.buf = tkz->numeric.data;

            pchtml_css_syntax_state_set(tkz,
                                     pchtml_css_syntax_consume_numeric_decimal);
            return data;
        }

        data -= 1;
    }
    /* U+002D HYPHEN-MINUS (-) */
    else if (*data == 0x2D) {
        data += 1;

        /* Check for <CDC-token> */

        if (data == end) {
            tkz->begin = data - 2;

            pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_minus_check_cdc);

            return data;
        }

        /* U+003E GREATER-THAN SIGN (>) */
        if (*data == 0x3E) {
            pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_CDC;

            pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
            pchtml_css_syntax_state_token_done_m(tkz, end);

            return (data + 1);
        }

        pchtml_css_syntax_token_ident(tkz->token)->begin = data - 2;

        pchtml_css_syntax_state_set(tkz,
                                 pchtml_css_syntax_consume_ident_like_not_url);
        return data;
    }

    /* Check for <ident-token> */

    if (pchtml_css_syntax_res_name_map[*data] == PCHTML_CSS_SYNTAX_RES_NAME_START) {
        pchtml_css_syntax_token_ident(tkz->token)->begin = data - 1;

        pchtml_css_syntax_state_set(tkz,
                                 pchtml_css_syntax_consume_ident_like_not_url);
        return (data + 1);
    }
    /* U+005C REVERSE SOLIDUS (\) */
    else if (*data == 0x5C) {
        data += 1;

        if (data == end) {
            tkz->begin = data - 2;

            pchtml_css_syntax_state_set(tkz,
                                     pchtml_css_syntax_state_minus_check_solidus);
            return data;
        }

        if (*data != 0x0A && *data != 0x0C && *data != 0x0D) {
            if (*data == 0x00) {
                pchtml_css_syntax_token_have_null_set(tkz);
            }

            pchtml_css_syntax_token_escaped_set(tkz);
            pchtml_css_syntax_token_ident(tkz->token)->begin = data - 2;

            data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                        pchtml_css_syntax_state_hash_consume_name);
            if (data == end) {
                return data;
            }

            pchtml_css_syntax_state_set(tkz,
                                     pchtml_css_syntax_consume_ident_like_not_url);
            return data;
        }

        data -= 1;
    }
    /* U+0000 NULL (\0) */
    else if (*data == 0x00) {
        pchtml_css_syntax_token_have_null_set(tkz);
        pchtml_css_syntax_token_ident(tkz->token)->begin = data - 1;

        pchtml_css_syntax_state_set(tkz,
                                 pchtml_css_syntax_consume_ident_like_not_url);
        return (data + 1);
    }

    return pchtml_css_syntax_state_delim(tkz, (data - 1), end);
}

static const unsigned char *
pchtml_css_syntax_state_minus_check(pchtml_css_syntax_tokenizer_t *tkz,
                                 const unsigned char *data, const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
    }

    /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
    if (*data >= 0x30 && *data <= 0x39) {
        tkz->numeric.is_negative = true;
        tkz->numeric.buf = tkz->numeric.data;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric);

        return data;
    }

    /* U+002E FULL STOP (.) */
    if (*data == 0x2E) {
        data += 1;

        if (data == end) {
            pchtml_css_syntax_state_set(tkz,
                                     pchtml_css_syntax_state_minus_check_digit);
            return data;
        }

        /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
        if (*data >= 0x30 && *data <= 0x39) {
            tkz->numeric.exponent = 0;
            tkz->numeric.is_negative = true;
            tkz->numeric.buf = tkz->numeric.data;

            pchtml_css_syntax_state_set(tkz,
                                     pchtml_css_syntax_consume_numeric_decimal);
            return data;
        }

        data -= 1;
    }
    /* U+002D HYPHEN-MINUS (-) */
    else if (*data == 0x2D) {
        data += 1;

        /* Check for <CDC-token> */

        if (data == end) {
            pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_minus_check_cdc);

            return data;
        }

        /* U+003E GREATER-THAN SIGN (>) */
        if (*data == 0x3E) {
            pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_CDC;

            pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
            pchtml_css_syntax_state_token_done_m(tkz, end);

            return (data + 1);
        }

        pchtml_css_syntax_token_ident(tkz->token)->begin = tkz->begin;

        pchtml_css_syntax_state_set(tkz,
                                 pchtml_css_syntax_consume_ident_like_not_url);
        return data;
    }

    /* Check for <ident-token> */

    if (pchtml_css_syntax_res_name_map[*data] == PCHTML_CSS_SYNTAX_RES_NAME_START) {
        pchtml_css_syntax_token_ident(tkz->token)->begin = tkz->begin;

        pchtml_css_syntax_state_set(tkz,
                                 pchtml_css_syntax_consume_ident_like_not_url);
        return (data + 1);
    }
    /* U+005C REVERSE SOLIDUS (\) */
    else if (*data == 0x5C) {
        data += 1;

        if (data == end) {
            pchtml_css_syntax_state_set(tkz,
                                     pchtml_css_syntax_state_minus_check_solidus);
            return data;
        }

        if (*data != 0x0A && *data != 0x0C && *data != 0x0D) {
            if (*data == 0x00) {
                pchtml_css_syntax_token_have_null_set(tkz);
            }

            pchtml_css_syntax_token_escaped_set(tkz);
            pchtml_css_syntax_token_ident(tkz->token)->begin = tkz->begin;

            data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                        pchtml_css_syntax_state_hash_consume_name);
            if (data == end) {
                return data;
            }

            pchtml_css_syntax_state_set(tkz,
                                     pchtml_css_syntax_consume_ident_like_not_url);
            return data;
        }

        data -= 1;
    }
    /* U+0000 NULL (\0) */
    else if (*data == 0x00) {
        pchtml_css_syntax_token_have_null_set(tkz);
        pchtml_css_syntax_token_ident(tkz->token)->begin = tkz->begin;

        pchtml_css_syntax_state_set(tkz,
                                 pchtml_css_syntax_consume_ident_like_not_url);
        return (data + 1);
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

    return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
}

static const unsigned char *
pchtml_css_syntax_state_minus_check_digit(pchtml_css_syntax_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end)
{
    UNUSED_PARAM(end);

    if (tkz->is_eof) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
    }

    /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
    if (*data >= 0x30 && *data <= 0x39) {
        tkz->numeric.exponent = 0;
        tkz->numeric.is_negative = true;
        tkz->numeric.buf = tkz->numeric.data;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_decimal);

        return data;
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

    return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
}

static const unsigned char *
pchtml_css_syntax_state_minus_check_cdc(pchtml_css_syntax_tokenizer_t *tkz,
                                     const unsigned char *data,
                                     const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_state_set(tkz,
                                 pchtml_css_syntax_consume_ident_like_not_url);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
    }

    /* U+003E GREATER-THAN SIGN (>) */
    if (*data == 0x3E) {
        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_CDC;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return (data + 1);
    }

    pchtml_css_syntax_token_ident(tkz->token)->begin = tkz->begin;
    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_ident_like_not_url);

    return data;
}

static const unsigned char *
pchtml_css_syntax_state_minus_check_solidus(pchtml_css_syntax_tokenizer_t *tkz,
                                         const unsigned char *data,
                                         const unsigned char *end)
{
    if (*data == 0x0A || *data == 0x0C || *data == 0x0D || tkz->is_eof) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->begin);
    }
    else if (*data == 0x00) {
        pchtml_css_syntax_token_have_null_set(tkz);
    }

    pchtml_css_syntax_token_escaped_set(tkz);
    pchtml_css_syntax_token_ident(tkz->token)->begin = tkz->begin;

    data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                        pchtml_css_syntax_state_hash_consume_name);
    if (data == end) {
        return data;
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_ident_like_not_url);

    return data;
}

/*
 * U+002E FULL STOP (.)
 */
const unsigned char *
pchtml_css_syntax_state_full_stop(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, const unsigned char *end)
{
    data += 1;

    if (data == end) {
        tkz->end = data - 1;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_full_stop_num);

        return end;
    }

    /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
    if (*data >= 0x30 && *data <= 0x39) {
        tkz->numeric.exponent = 0;
        tkz->numeric.buf = tkz->numeric.data;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_decimal);

        return data;
    }

    return pchtml_css_syntax_state_delim(tkz, (data - 1), end);
}

static const unsigned char *
pchtml_css_syntax_state_full_stop_num(pchtml_css_syntax_tokenizer_t *tkz,
                                   const unsigned char *data, const unsigned char *end)
{
    UNUSED_PARAM(end);

    if (tkz->is_eof) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->end);
    }

    /* U+0030 DIGIT ZERO (0) and U+0039 DIGIT NINE (9) */
    if (*data >= 0x30 && *data <= 0x39) {
        tkz->numeric.exponent = 0;
        tkz->numeric.buf = tkz->numeric.data;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_numeric_decimal);

        return data;
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

    return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->end);
}

/*
 * U+003A COLON (:)
 */
const unsigned char *
pchtml_css_syntax_state_colon(pchtml_css_syntax_tokenizer_t *tkz,
                           const unsigned char *data, const unsigned char *end)
{
    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_COLON;

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
    pchtml_css_syntax_state_token_done_m(tkz, end);

    return (data + 1);
}

/*
 * U+003B SEMICOLON (;)
 */
const unsigned char *
pchtml_css_syntax_state_semicolon(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, const unsigned char *end)
{
    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_SEMICOLON;

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
    pchtml_css_syntax_state_token_done_m(tkz, end);

    return (data + 1);
}

/*
 * U+003C LESS-THAN SIGN (<)
 */
const unsigned char *
pchtml_css_syntax_state_less_sign(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, const unsigned char *end)
{
    data += 1;

    if ((end - data) > 2) {
        if (memcmp(data, "!--", sizeof(unsigned char) * 3) == 0) {
            pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_CDO;

            pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
            pchtml_css_syntax_state_token_done_m(tkz, end);

            return (data + 3);
        }

        return pchtml_css_syntax_state_delim(tkz, (data - 1), end);
    }

    if (data == end) {
        tkz->end = data - 1;

        pchtml_css_syntax_state_set(tkz,
                                 pchtml_css_syntax_state_less_sign_check_exmark);
        return data;
    }

    /* U+0021 EXCLAMATION MARK */
    if (*data != 0x21) {
        return pchtml_css_syntax_state_delim(tkz, (data - 1), end);
    }

    data += 1;

    if (data == end) {
        tkz->end = data - 2;

        pchtml_css_syntax_state_set(tkz,
                                 pchtml_css_syntax_state_less_sign_check_fminus);
        return data;
    }

    /* U+002D HYPHEN-MINUS */
    if (*data != 0x2D) {
        return pchtml_css_syntax_state_delim(tkz, (data - 2), end);
    }

    tkz->end = data - 2;

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_less_sign_check_tminus);

    return (data + 1);
}

static const unsigned char *
pchtml_css_syntax_state_less_sign_check_exmark(pchtml_css_syntax_tokenizer_t *tkz,
                                            const unsigned char *data,
                                            const unsigned char *end)
{
    UNUSED_PARAM(end);

    /* U+0021 EXCLAMATION MARK */
    if (tkz->is_eof || *data != 0x21) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->end);
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_less_sign_check_fminus);

    return (data + 1);
}

static const unsigned char *
pchtml_css_syntax_state_less_sign_check_fminus(pchtml_css_syntax_tokenizer_t *tkz,
                                            const unsigned char *data,
                                            const unsigned char *end)
{
    UNUSED_PARAM(end);

    /* U+002D HYPHEN-MINUS */
    if (tkz->is_eof || *data != 0x2D) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->end);
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_less_sign_check_tminus);

    return (data + 1);
}

static const unsigned char *
pchtml_css_syntax_state_less_sign_check_tminus(pchtml_css_syntax_tokenizer_t *tkz,
                                            const unsigned char *data,
                                            const unsigned char *end)
{
    /* U+002D HYPHEN-MINUS */
    if (tkz->is_eof || *data != 0x2D) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->end);
    }

    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_CDO;

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
    pchtml_css_syntax_state_token_done_m(tkz, end);

    return (data + 1);
}

/*
 * U+0040 COMMERCIAL AT (@)
 */
const unsigned char *
pchtml_css_syntax_state_at(pchtml_css_syntax_tokenizer_t *tkz,
                        const unsigned char *data, const unsigned char *end)
{
    data += 1;

    if (data == end) {
        tkz->end = data - 1;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_at_begin);

        return data;
    }

    pchtml_css_syntax_token_at_keyword(tkz->token)->begin = data;

    if (pchtml_css_syntax_res_name_map[*data] != PCHTML_CSS_SYNTAX_RES_NAME_START) {
        /* U+002D HYPHEN-MINUS */
        if (*data == 0x2D) {
            data += 1;

            if (data == end) {
                tkz->end = data - 2;

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_at_minus);

                return data;
            }

            if (pchtml_css_syntax_res_name_map[*data] != PCHTML_CSS_SYNTAX_RES_NAME_START) {
                /* U+005C REVERSE SOLIDUS (\) */
                if (*data == 0x5C) {
                    data += 1;

                    if (data == end) {
                        tkz->end = data - 3;

                        pchtml_css_syntax_state_set(tkz,
                                                 pchtml_css_syntax_state_at_escape);
                        return data;
                    }

                    /*
                     * U+000A LINE FEED (LF)
                     * U+000C FORM FEED (FF)
                     * U+000D CARRIAGE RETURN (CR)
                     */
                    if (*data == 0x0A || *data == 0x0C || *data == 0x0D) {
                        return pchtml_css_syntax_state_delim(tkz, (data - 3), end);
                    }
                    else if (*data == 0x00) {
                        pchtml_css_syntax_token_have_null_set(tkz);
                    }

                    pchtml_css_syntax_token_escaped_set(tkz);

                    data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                                  pchtml_css_syntax_state_at_name);
                    if (data == end) {
                        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_AT_KEYWORD;

                        return data;
                    }
                }
                else if (*data == 0x00) {
                    pchtml_css_syntax_token_have_null_set(tkz);
                }
                else if (*data != 0x2D) {
                    return pchtml_css_syntax_state_delim(tkz, (data - 2), end);
                }
            }
        }
        /* U+005C REVERSE SOLIDUS (\) */
        else if (*data == 0x5C) {
            data += 1;

            if (data == end) {
                tkz->end = data - 2;

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_at_escape);

                return data;
            }

            if (*data == 0x0A || *data == 0x0C || *data == 0x0D) {
                return pchtml_css_syntax_state_delim(tkz, (data - 2), end);
            }
            else if (*data == 0x00) {
                pchtml_css_syntax_token_have_null_set(tkz);
            }

            pchtml_css_syntax_token_escaped_set(tkz);

            data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                                  pchtml_css_syntax_state_at_name);
            if (data == end) {
                pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_AT_KEYWORD;

                return data;
            }
        }
        else if (*data == 0x00) {
            pchtml_css_syntax_token_have_null_set(tkz);
        }
        else {
            return pchtml_css_syntax_state_delim(tkz, (data - 1), end);
        }
    }

    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_AT_KEYWORD;

    for (; data < end; data++) {
        if (pchtml_css_syntax_res_name_map[*data] == 0x00) {
            /* U+005C REVERSE SOLIDUS (\) */
            if (*data == 0x5C) {
                data += 1;

                if (data == end) {
                    tkz->end = data - 1;
                    pchtml_css_syntax_token_at_keyword(tkz->token)->end = tkz->end;

                    pchtml_css_syntax_state_set(tkz,
                                             pchtml_css_syntax_state_at_name_escape);
                    return data;
                }

                /*
                 * U+000A LINE FEED (LF)
                 * U+000C FORM FEED (FF)
                 * U+000D CARRIAGE RETURN (CR)
                 */
                if (*data == 0x0A || *data == 0x0C || *data == 0x0D) {
                    data -= 1;

                    pchtml_css_syntax_token_at_keyword(tkz->token)->end = data;

                    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                    pchtml_css_syntax_state_token_done_m(tkz, end);

                    return data;
                }
                else if (*data == 0x00) {
                    pchtml_css_syntax_token_have_null_set(tkz);
                }

                pchtml_css_syntax_token_escaped_set(tkz);

                data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                                  pchtml_css_syntax_state_at_name);
                if (data == end) {
                    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_at_name);

                    return data;
                }

                data -= 1;

                continue;
            }
            else if (*data == 0x00) {
                pchtml_css_syntax_token_have_null_set(tkz);
            }
            else {
                pchtml_css_syntax_token_at_keyword(tkz->token)->end = data;

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                pchtml_css_syntax_state_token_done_m(tkz, end);

                return data;
            }
        }
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_at_name);

    return data;
}

static const unsigned char *
pchtml_css_syntax_state_at_begin(pchtml_css_syntax_tokenizer_t *tkz,
                              const unsigned char *data, const unsigned char *end)
{
    if (tkz->is_eof) {
        goto consume_delim;
    }

    pchtml_css_syntax_token_at_keyword(tkz->token)->begin = data;

    if (pchtml_css_syntax_res_name_map[*data] != PCHTML_CSS_SYNTAX_RES_NAME_START) {
        /* U+002D HYPHEN-MINUS */
        if (*data == 0x2D) {
            data += 1;

            if (data == end) {
                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_at_minus);

                return data;
            }

            if (pchtml_css_syntax_res_name_map[*data] != PCHTML_CSS_SYNTAX_RES_NAME_START) {
                /* U+005C REVERSE SOLIDUS (\) */
                if (*data == 0x5C) {
                    data += 1;

                    if (data == end) {
                        pchtml_css_syntax_state_set(tkz,
                                                 pchtml_css_syntax_state_at_escape);
                        return data;
                    }

                    /*
                     * U+000A LINE FEED (LF)
                     * U+000C FORM FEED (FF)
                     * U+000D CARRIAGE RETURN (CR)
                     */
                    if (*data == 0x0A || *data == 0x0C || *data == 0x0D) {
                        goto consume_delim;
                    }
                    else if (*data == 0x00) {
                        pchtml_css_syntax_token_have_null_set(tkz);
                    }

                    pchtml_css_syntax_token_escaped_set(tkz);

                    data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                                  pchtml_css_syntax_state_at_name);
                    if (data == end) {
                        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_AT_KEYWORD;

                        return data;
                    }

                    data -= 1;
                }
                else if (*data == 0x00) {
                    pchtml_css_syntax_token_have_null_set(tkz);
                }
                else if (*data != 0x2D) {
                    goto consume_delim;
                }
            }
        }
        /* U+005C REVERSE SOLIDUS (\) */
        else if (*data == 0x5C) {
            data += 1;

            if (data == end) {
                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_at_escape);

                return data;
            }

            if (*data == 0x0A || *data == 0x0C || *data == 0x0D) {
                goto consume_delim;
            }
            else if (*data == 0x00) {
                pchtml_css_syntax_token_have_null_set(tkz);
            }

            pchtml_css_syntax_token_escaped_set(tkz);

            data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                                  pchtml_css_syntax_state_at_name);
            if (data == end) {
                pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_AT_KEYWORD;

                return data;
            }

            data -= 1;
        }
        else if (*data == 0x00) {
            pchtml_css_syntax_token_have_null_set(tkz);
        }
        else {
            goto consume_delim;
        }
    }

    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_AT_KEYWORD;
    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_at_name);

    return (data + 1);


consume_delim:

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

    return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->end);
}

static const unsigned char *
pchtml_css_syntax_state_at_minus(pchtml_css_syntax_tokenizer_t *tkz,
                              const unsigned char *data, const unsigned char *end)
{
    if (tkz->is_eof) {
        goto consume_delim;
    }

    if (pchtml_css_syntax_res_name_map[*data] != PCHTML_CSS_SYNTAX_RES_NAME_START) {
        /* U+005C REVERSE SOLIDUS (\) */
        if (*data == 0x5C) {
            data += 1;

            if (data == end) {
                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_at_escape);

                return data;
            }

            /*
             * U+000A LINE FEED (LF)
             * U+000C FORM FEED (FF)
             * U+000D CARRIAGE RETURN (CR)
             */
            if (*data == 0x0A || *data == 0x0C || *data == 0x0D) {
                goto consume_delim;
            }
            else if (*data == 0x00) {
                pchtml_css_syntax_token_have_null_set(tkz);
            }

            pchtml_css_syntax_token_escaped_set(tkz);

            data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                                  pchtml_css_syntax_state_at_name);
            if (data == end) {
                pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_AT_KEYWORD;

                return data;
            }

            data -= 1;
        }
        else if (*data == 0x00) {
            pchtml_css_syntax_token_have_null_set(tkz);
        }
        /* U+002D HYPHEN-MINUS */
        else if (*data != 0x2D) {
            goto consume_delim;
        }
    }

    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_AT_KEYWORD;
    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_at_name);

    return (data + 1);


consume_delim:

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

    return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->end);
}

static const unsigned char *
pchtml_css_syntax_state_at_escape(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, const unsigned char *end)
{
    /*
     * U+000A LINE FEED (LF)
     * U+000C FORM FEED (FF)
     * U+000D CARRIAGE RETURN (CR)
     */
    if (*data == 0x0A || *data == 0x0C || *data == 0x0D || tkz->is_eof) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->end);
    }
    else if (*data == 0x00) {
        pchtml_css_syntax_token_have_null_set(tkz);
    }

    pchtml_css_syntax_token_escaped_set(tkz);

    data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                              pchtml_css_syntax_state_at_name);
    if (data == end) {
        pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_AT_KEYWORD;

        return data;
    }

    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_AT_KEYWORD;
    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_at_name);

    return data;
}

static const unsigned char *
pchtml_css_syntax_state_at_name(pchtml_css_syntax_tokenizer_t *tkz,
                             const unsigned char *data, const unsigned char *end)
{
    if (tkz->is_eof) {
        pchtml_css_syntax_token_at_keyword(tkz->token)->end = tkz->incoming_node->end;
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return end;
    }

    for (; data < end; data++) {
        if (pchtml_css_syntax_res_name_map[*data] == 0x00) {
            /* U+005C REVERSE SOLIDUS (\) */
            if (*data == 0x5C) {
                data += 1;

                if (data == end) {
                    tkz->end = data - 1;
                    pchtml_css_syntax_token_at_keyword(tkz->token)->end = tkz->end;

                    pchtml_css_syntax_state_set(tkz,
                                             pchtml_css_syntax_state_at_name_escape);
                    return data;
                }

                /*
                 * U+000A LINE FEED (LF)
                 * U+000C FORM FEED (FF)
                 * U+000D CARRIAGE RETURN (CR)
                 */
                if (*data == 0x0A || *data == 0x0C || *data == 0x0D) {
                    data -= 1;

                    pchtml_css_syntax_token_at_keyword(tkz->token)->end = data;

                    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                    pchtml_css_syntax_state_token_done_m(tkz, end);

                    return data;
                }
                else if (*data == 0x00) {
                    pchtml_css_syntax_token_have_null_set(tkz);
                }

                pchtml_css_syntax_token_escaped_set(tkz);

                data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                                  pchtml_css_syntax_state_at_name);
                if (data == end) {
                    return data;
                }

                data -= 1;
            }
            else if (*data == 0x00) {
                pchtml_css_syntax_token_have_null_set(tkz);
            }
            else {
                pchtml_css_syntax_token_at_keyword(tkz->token)->end = data;

                pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
                pchtml_css_syntax_state_token_done_m(tkz, end);

                return data;
            }
        }
    }

    return data;
}

static const unsigned char *
pchtml_css_syntax_state_at_name_escape(pchtml_css_syntax_tokenizer_t *tkz,
                                    const unsigned char *data,
                                    const unsigned char *end)
{
    /*
     * U+000A LINE FEED (LF)
     * U+000C FORM FEED (FF)
     * U+000D CARRIAGE RETURN (CR)
     */
    if (*data == 0x0A || *data == 0x0C || *data == 0x0D || tkz->is_eof) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
        pchtml_css_syntax_state_token_done_m(tkz, end);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->end);
    }
    else if (*data == 0x00) {
        pchtml_css_syntax_token_have_null_set(tkz);
    }

    pchtml_css_syntax_token_escaped_set(tkz);

    data = pchtml_css_syntax_state_check_escaped(tkz, data, end,
                                              pchtml_css_syntax_state_at_name);
    if (data == end) {
        return data;
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_at_name);

    return (data + 1);
}

/*
 * U+005B LEFT SQUARE BRACKET ([)
 */
const unsigned char *
pchtml_css_syntax_state_ls_bracket(pchtml_css_syntax_tokenizer_t *tkz,
                                const unsigned char *data, const unsigned char *end)
{
    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_LS_BRACKET;

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
    pchtml_css_syntax_state_token_done_m(tkz, end);

    return (data + 1);
}

/*
 * U+005C REVERSE SOLIDUS (\)
 */
const unsigned char *
pchtml_css_syntax_state_rsolidus(pchtml_css_syntax_tokenizer_t *tkz,
                              const unsigned char *data, const unsigned char *end)
{
    data += 1;

    if (data == end) {
        tkz->end = data - 1;

        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_rsolidus_check);

        return data;
    }

    if (*data == 0x0A || *data == 0x0C || *data == 0x0D) {
        return pchtml_css_syntax_state_delim(tkz, (data - 1), end);
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_ident_like);

    return (data - 1);
}

static const unsigned char *
pchtml_css_syntax_state_rsolidus_check(pchtml_css_syntax_tokenizer_t *tkz,
                                    const unsigned char *data,
                                    const unsigned char *end)
{
    UNUSED_PARAM(end);
    /*
     * U+000A LINE FEED (LF)
     * U+000C FORM FEED (FF)
     * U+000D CARRIAGE RETURN (CR)
     */
    if ((*data == 0x0A || *data == 0x0C || *data == 0x0D) || tkz->is_eof) {
        pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_delim);

        return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->end);
    }

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_consume_ident_like);

    return pchtml_css_syntax_tokenizer_change_incoming(tkz, tkz->end);
}

/*
 * U+005D RIGHT SQUARE BRACKET (])
 */
const unsigned char *
pchtml_css_syntax_state_rs_bracket(pchtml_css_syntax_tokenizer_t *tkz,
                                const unsigned char *data, const unsigned char *end)
{
    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_RS_BRACKET;

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
    pchtml_css_syntax_state_token_done_m(tkz, end);

    return (data + 1);
}

/*
 * U+007B LEFT CURLY BRACKET ({)
 */
const unsigned char *
pchtml_css_syntax_state_lc_bracket(pchtml_css_syntax_tokenizer_t *tkz,
                                const unsigned char *data, const unsigned char *end)
{
    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_LC_BRACKET;

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
    pchtml_css_syntax_state_token_done_m(tkz, end);

    return (data + 1);
}

/*
 * U+007D RIGHT CURLY BRACKET (})
 */
const unsigned char *
pchtml_css_syntax_state_rc_bracket(pchtml_css_syntax_tokenizer_t *tkz,
                                const unsigned char *data, const unsigned char *end)
{
    pchtml_css_syntax_token(tkz)->type = PCHTML_CSS_SYNTAX_TOKEN_RC_BRACKET;

    pchtml_css_syntax_state_set(tkz, pchtml_css_syntax_state_data);
    pchtml_css_syntax_state_token_done_m(tkz, end);

    return (data + 1);
}

