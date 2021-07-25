/**
 * @file state.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for css state.
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


#ifndef PCHTML_CSS_SYNTAX_STATE_H
#define PCHTML_CSS_SYNTAX_STATE_H

#define pchtml_css_syntax_state_set(tkz, func)                                    \
    do {                                                                       \
        tkz->state = func;                                                     \
    }                                                                          \
    while (0)

#define _pchtml_css_syntax_state_token_done_m(tkz, v_end)                         \
    tkz->token = tkz->cb_token_done(tkz, tkz->token, tkz->cb_token_ctx);       \
    if (tkz->token == NULL) {                                                  \
        if (tkz->status == PCHTML_STATUS_OK) {                                    \
            tkz->status = PCHTML_STATUS_ERROR;                                    \
        }                                                                      \
        return v_end;                                                          \
    }

#define pchtml_css_syntax_state_token_done_m(tkz, v_end)                          \
    do {                                                                       \
        _pchtml_css_syntax_state_token_done_m(tkz, v_end)                         \
        pchtml_css_syntax_token_clean(tkz->token);                                \
    }                                                                          \
    while (0)

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "private/errors.h"

#include "html/css/syntax/base.h"
#include "html/css/syntax/tokenizer.h"

#define PCHTML_STR_RES_MAP_HEX
#include "html/core/str_res.h"


const unsigned char *
pchtml_css_syntax_state_data(pchtml_css_syntax_tokenizer_t *tkz,
                          const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_delim(pchtml_css_syntax_tokenizer_t *tkz,
                           const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_eof(pchtml_css_syntax_tokenizer_t *tkz,
                         const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_comment_begin(pchtml_css_syntax_tokenizer_t *tkz,
                                   const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_whitespace(pchtml_css_syntax_tokenizer_t *tkz,
                                const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_string(pchtml_css_syntax_tokenizer_t *tkz,
                            const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_hash(pchtml_css_syntax_tokenizer_t *tkz,
                          const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_lparenthesis(pchtml_css_syntax_tokenizer_t *tkz,
                                  const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_rparenthesis(pchtml_css_syntax_tokenizer_t *tkz,
                                  const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_plus(pchtml_css_syntax_tokenizer_t *tkz,
                          const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_comma(pchtml_css_syntax_tokenizer_t *tkz,
                           const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_minus(pchtml_css_syntax_tokenizer_t *tkz,
                           const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_full_stop(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_colon(pchtml_css_syntax_tokenizer_t *tkz,
                           const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_semicolon(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_less_sign(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_at(pchtml_css_syntax_tokenizer_t *tkz,
                        const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_ls_bracket(pchtml_css_syntax_tokenizer_t *tkz,
                                const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_rsolidus(pchtml_css_syntax_tokenizer_t *tkz,
                              const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_rs_bracket(pchtml_css_syntax_tokenizer_t *tkz,
                                const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_lc_bracket(pchtml_css_syntax_tokenizer_t *tkz,
                                const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_state_rc_bracket(pchtml_css_syntax_tokenizer_t *tkz,
                                const unsigned char *data, const unsigned char *end);


/*
 * Inline functions
 */
static inline const unsigned char *
pchtml_css_syntax_state_check_newline(pchtml_css_syntax_tokenizer_t *tkz,
                                   const unsigned char *data,
                                   const unsigned char *end)
{
    UNUSED_PARAM(end);

    tkz->state = tkz->return_state;

    if (tkz->is_eof) {
        return data;
    }

    if (*data == 0x0A) {
        data += 1;
    }

    return data;
}

static inline const unsigned char *
pchtml_css_syntax_state_check_escaped_loop(pchtml_css_syntax_tokenizer_t *tkz,
                                        const unsigned char *data,
                                        const unsigned char *end)
{
    if (tkz->is_eof) {
        tkz->state = tkz->return_state;

        return data;
    }

    for (; tkz->count < 6; tkz->count++, data++) {
        if (data == end) {
            return data;
        }

        if (pchtml_str_res_map_hex[*data] == 0xFF) {
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
                        tkz->state = pchtml_css_syntax_state_check_newline;

                        return data;
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
    }

    tkz->state = tkz->return_state;

    return data;
}

static inline const unsigned char *
pchtml_css_syntax_state_check_escaped(pchtml_css_syntax_tokenizer_t *tkz,
                                   const unsigned char *data,
                                   const unsigned char *end,
                                   pchtml_css_syntax_tokenizer_state_f ret_state)
{
    for (tkz->count = 0; tkz->count < 6; tkz->count++, data++) {
        if (data == end) {
            tkz->state = pchtml_css_syntax_state_check_escaped_loop;
            tkz->return_state = ret_state;

            return data;
        }

        if (pchtml_str_res_map_hex[*data] == 0xFF) {
            if (tkz->count == 0) {
                data += 1;
                if (data == end) {
                    tkz->state = ret_state;
                }

                break;
            }

            switch (*data) {
                case 0x0D:
                    pchtml_css_syntax_token_cr_set(tkz);

                    data += 1;
                    if (data == end) {
                        tkz->state = pchtml_css_syntax_state_check_newline;
                        tkz->return_state = ret_state;

                        return data;
                    }

                    if (*data == 0x0A) {
                        data += 1;
                        if (data == end) {
                            tkz->state = ret_state;

                            return data;
                        }
                    }

                    break;

                case 0x0C:
                    pchtml_css_syntax_token_ff_set(tkz);
                    /* fall through */

                case 0x09:
                case 0x20:
                case 0x0A:
                    data += 1;
                    if (data == end) {
                        tkz->state = ret_state;

                        return data;
                    }

                    break;
            }

            break;
        }
    }

    return data;
}


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_CSS_SYNTAX_STATE_H */
