/**
 * @file state_comment.c 
 * @author 
 * @date 2021/07/02
 * @brief The complementation of html comment state.
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

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"

#include "html/html/tokenizer/state_comment.h"
#include "html/html/tokenizer/state.h"

#define PCHTML_STR_RES_ANSI_REPLACEMENT_CHARACTER
#include "html/core/str_res.h"


static const unsigned char *
pchtml_html_tokenizer_state_comment_start(pchtml_html_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_comment_start_dash(pchtml_html_tokenizer_t *tkz,
                                            const unsigned char *data,
                                            const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_comment(pchtml_html_tokenizer_t *tkz,
                                 const unsigned char *data,
                                 const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_comment_less_than_sign(pchtml_html_tokenizer_t *tkz,
                                                const unsigned char *data,
                                                const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_comment_less_than_sign_bang(pchtml_html_tokenizer_t *tkz,
                                                     const unsigned char *data,
                                                     const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_comment_less_than_sign_bang_dash(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_comment_less_than_sign_bang_dash_dash(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_comment_end_dash(pchtml_html_tokenizer_t *tkz,
                                          const unsigned char *data,
                                          const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_comment_end(pchtml_html_tokenizer_t *tkz,
                                     const unsigned char *data,
                                     const unsigned char *end);

static const unsigned char *
pchtml_html_tokenizer_state_comment_end_bang(pchtml_html_tokenizer_t *tkz,
                                          const unsigned char *data,
                                          const unsigned char *end);


/*
 * Helper function. No in the specification. For 12.2.5.43
 */
const unsigned char *
pchtml_html_tokenizer_state_comment_before_start(pchtml_html_tokenizer_t *tkz,
                                              const unsigned char *data,
                                              const unsigned char *end)
{
    if (tkz->is_eof == false) {
        pchtml_html_tokenizer_state_token_set_begin(tkz, data);
        pchtml_html_tokenizer_state_token_set_end(tkz, data);
    }

    tkz->token->tag_id = PCHTML_TAG__EM_COMMENT;

    return pchtml_html_tokenizer_state_comment_start(tkz, data, end);
}

/*
 * 12.2.5.43 Comment start state
 */
static const unsigned char *
pchtml_html_tokenizer_state_comment_start(pchtml_html_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end)
{
    /* U+002D HYPHEN-MINUS (-) */
    if (*data == 0x2D) {
        data++;
        tkz->state = pchtml_html_tokenizer_state_comment_start_dash;
    }
    /* U+003E GREATER-THAN SIGN (>) */
    else if (*data == 0x3E) {
        tkz->state = pchtml_html_tokenizer_state_data_before;

        pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                     PCHTML_HTML_TOKENIZER_ERROR_ABCLOFEMCO);

        pchtml_html_tokenizer_state_set_text(tkz);
        pchtml_html_tokenizer_state_token_done_wo_check_m(tkz, end);

        data++;
    }
    else {
        tkz->state = pchtml_html_tokenizer_state_comment;
    }

    return data;
}

/*
 * 12.2.5.44 Comment start dash state
 */
static const unsigned char *
pchtml_html_tokenizer_state_comment_start_dash(pchtml_html_tokenizer_t *tkz,
                                            const unsigned char *data,
                                            const unsigned char *end)
{
    /* U+002D HYPHEN-MINUS (-) */
    if (*data == 0x2D) {
        tkz->state = pchtml_html_tokenizer_state_comment_end;

        return (data + 1);
    }
    /* U+003E GREATER-THAN SIGN (>) */
    else if (*data == 0x3E) {
        tkz->state = pchtml_html_tokenizer_state_data_before;

        pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                     PCHTML_HTML_TOKENIZER_ERROR_ABCLOFEMCO);

        pchtml_html_tokenizer_state_set_text(tkz);
        pchtml_html_tokenizer_state_token_done_wo_check_m(tkz, end);

        return (data + 1);
    }
    /* EOF */
    else if (*data == 0x00) {
        if (tkz->is_eof) {
            pchtml_html_tokenizer_state_append_m(tkz, "-", 1);

            pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                         PCHTML_HTML_TOKENIZER_ERROR_EOINCO);

            pchtml_html_tokenizer_state_set_text(tkz);
            pchtml_html_tokenizer_state_token_done_wo_check_m(tkz, end);

            return end;
        }
    }

    pchtml_html_tokenizer_state_append_m(tkz, "-", 1);

    tkz->state = pchtml_html_tokenizer_state_comment;

    return data;
}

/*
 * 12.2.5.45 Comment state
 */
static const unsigned char *
pchtml_html_tokenizer_state_comment(pchtml_html_tokenizer_t *tkz,
                                 const unsigned char *data,
                                 const unsigned char *end)
{
    pchtml_html_tokenizer_state_begin_set(tkz, data);

    while (data != end) {
        switch (*data) {
            /* U+003C LESS-THAN SIGN (<) */
            case 0x3C:
                data++;

                pchtml_html_tokenizer_state_append_data_m(tkz, data);

                tkz->state = pchtml_html_tokenizer_state_comment_less_than_sign;

                return data;

            /* U+002D HYPHEN-MINUS (-) */
            case 0x2D:
                pchtml_html_tokenizer_state_token_set_end(tkz, data - 1);
                pchtml_html_tokenizer_state_append_data_m(tkz, data);

                tkz->state = pchtml_html_tokenizer_state_comment_end_dash;

                return (data + 1);

            /* U+000D CARRIAGE RETURN (CR) */
            case 0x0D:
                if (++data >= end) {
                    pchtml_html_tokenizer_state_append_data_m(tkz, data - 1);

                    tkz->state = pchtml_html_tokenizer_state_cr;
                    tkz->state_return = pchtml_html_tokenizer_state_comment;

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

                    pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->token->end,
                                                 PCHTML_HTML_TOKENIZER_ERROR_EOINCO);

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
 * 12.2.5.46 Comment less-than sign state
 */
static const unsigned char *
pchtml_html_tokenizer_state_comment_less_than_sign(pchtml_html_tokenizer_t *tkz,
                                                const unsigned char *data,
                                                const unsigned char *end)
{
    /* U+0021 EXCLAMATION MARK (!) */
    if (*data == 0x21) {
        pchtml_html_tokenizer_state_append_m(tkz, data, 1);

        tkz->state = pchtml_html_tokenizer_state_comment_less_than_sign_bang;

        return (data + 1);
    }
    /* U+003C LESS-THAN SIGN (<) */
    else if (*data == 0x3C) {
        pchtml_html_tokenizer_state_append_m(tkz, data, 1);

        return (data + 1);
    }

    tkz->state = pchtml_html_tokenizer_state_comment;

    return data;
}

/*
 * 12.2.5.47 Comment less-than sign bang state
 */
static const unsigned char *
pchtml_html_tokenizer_state_comment_less_than_sign_bang(pchtml_html_tokenizer_t *tkz,
                                                     const unsigned char *data,
                                                     const unsigned char *end)
{
    UNUSED_PARAM(end);

    /* U+002D HYPHEN-MINUS (-) */
    if (*data == 0x2D) {
        tkz->state = pchtml_html_tokenizer_state_comment_less_than_sign_bang_dash;

        return (data + 1);
    }

    tkz->state = pchtml_html_tokenizer_state_comment;

    return data;
}

/*
 * 12.2.5.48 Comment less-than sign bang dash state
 */
static const unsigned char *
pchtml_html_tokenizer_state_comment_less_than_sign_bang_dash(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end)
{
    UNUSED_PARAM(end);

    /* U+002D HYPHEN-MINUS (-) */
    if (*data == 0x2D) {
        tkz->state =
            pchtml_html_tokenizer_state_comment_less_than_sign_bang_dash_dash;

        return (data + 1);
    }

    tkz->state = pchtml_html_tokenizer_state_comment_end_dash;

    return data;
}

/*
 * 12.2.5.49 Comment less-than sign bang dash dash state
 */
static const unsigned char *
pchtml_html_tokenizer_state_comment_less_than_sign_bang_dash_dash(
                                                      pchtml_html_tokenizer_t *tkz,
                                                      const unsigned char *data,
                                                      const unsigned char *end)
{
    UNUSED_PARAM(end);

    /* U+003E GREATER-THAN SIGN (>) */
    if (*data == 0x3E) {
        tkz->state = pchtml_html_tokenizer_state_comment_end;

        return data;
    }
    /* EOF */
    else if (*data == 0x00) {
        if (tkz->is_eof) {
            tkz->state = pchtml_html_tokenizer_state_comment_end;

            return data;
        }
    }

    pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                 PCHTML_HTML_TOKENIZER_ERROR_NECO);

    tkz->state = pchtml_html_tokenizer_state_comment_end;

    return data;
}

/*
 * 12.2.5.50 Comment end dash state
 */
static const unsigned char *
pchtml_html_tokenizer_state_comment_end_dash(pchtml_html_tokenizer_t *tkz,
                                          const unsigned char *data,
                                          const unsigned char *end)
{
    /* U+002D HYPHEN-MINUS (-) */
    if (*data == 0x2D) {
        tkz->state = pchtml_html_tokenizer_state_comment_end;

        return (data + 1);
    }
    /* EOF */
    else if (*data == 0x00) {
        if (tkz->is_eof) {
            pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                         PCHTML_HTML_TOKENIZER_ERROR_EOINCO);

            pchtml_html_tokenizer_state_set_text(tkz);
            pchtml_html_tokenizer_state_token_done_wo_check_m(tkz, end);

            return end;
        }
    }

    pchtml_html_tokenizer_state_append_m(tkz, "-", 1);

    tkz->state = pchtml_html_tokenizer_state_comment;

    return data;
}

/*
 * 12.2.5.51 Comment end state
 */
static const unsigned char *
pchtml_html_tokenizer_state_comment_end(pchtml_html_tokenizer_t *tkz,
                                     const unsigned char *data,
                                     const unsigned char *end)
{
    /* U+003E GREATER-THAN SIGN (>) */
    if (*data == 0x3E) {
        /* Skip two '-' characters in comment tag end "-->"
         * For <!----> or <!-----> ...
         */
        tkz->state = pchtml_html_tokenizer_state_data_before;

        pchtml_html_tokenizer_state_set_text(tkz);
        pchtml_html_tokenizer_state_token_done_wo_check_m(tkz, end);

        return (data + 1);
    }
    /* U+0021 EXCLAMATION MARK (!) */
    else if (*data == 0x21) {
        tkz->state = pchtml_html_tokenizer_state_comment_end_bang;

        return (data + 1);
    }
    /* U+002D HYPHEN-MINUS (-) */
    else if (*data == 0x2D) {
        pchtml_html_tokenizer_state_append_m(tkz, data, 1);

        return (data + 1);
    }
    /* EOF */
    else if (*data == 0x00) {
        if (tkz->is_eof) {
            pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                         PCHTML_HTML_TOKENIZER_ERROR_EOINCO);

            pchtml_html_tokenizer_state_set_text(tkz);
            pchtml_html_tokenizer_state_token_done_wo_check_m(tkz, end);

            return end;
        }
    }

    pchtml_html_tokenizer_state_append_m(tkz, "--", 2);

    tkz->state = pchtml_html_tokenizer_state_comment;

    return data;
}

/*
 * 12.2.5.52 Comment end bang state
 */
static const unsigned char *
pchtml_html_tokenizer_state_comment_end_bang(pchtml_html_tokenizer_t *tkz,
                                          const unsigned char *data,
                                          const unsigned char *end)
{
    /* U+002D HYPHEN-MINUS (-) */
    if (*data == 0x2D) {
        tkz->state = pchtml_html_tokenizer_state_comment_end_dash;

        return (data + 1);
    }
    /* U+003E GREATER-THAN SIGN (>) */
    else if (*data == 0x3E) {
        tkz->state = pchtml_html_tokenizer_state_data_before;

        pchtml_html_tokenizer_error_add(tkz->parse_errors, data,
                                     PCHTML_HTML_TOKENIZER_ERROR_INCLCO);

        pchtml_html_tokenizer_state_set_text(tkz);
        pchtml_html_tokenizer_state_token_done_wo_check_m(tkz, end);

        return (data + 1);
    }
    /* EOF */
    else if (*data == 0x00) {
        if (tkz->is_eof) {
            pchtml_html_tokenizer_error_add(tkz->parse_errors, tkz->last,
                                         PCHTML_HTML_TOKENIZER_ERROR_EOINCO);

            pchtml_html_tokenizer_state_set_text(tkz);
            pchtml_html_tokenizer_state_token_done_wo_check_m(tkz, end);

            return end;
        }
    }

    tkz->state = pchtml_html_tokenizer_state_comment;

    return data;
}
