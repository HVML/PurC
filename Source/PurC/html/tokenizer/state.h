/**
 * @file state.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html parser status.
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


#ifndef PCHTML_HTML_TOKENIZER_STATE_H
#define PCHTML_HTML_TOKENIZER_STATE_H

#include "config.h"
#include "html/tokenizer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define pchtml_html_tokenizer_state_begin_set(tkz, v_data)                        \
    (tkz->begin = v_data)

#define pchtml_html_tokenizer_state_append_data_m(tkz, v_data)                    \
    do {                                                                       \
        if (pchtml_html_tokenizer_temp_append_data(tkz, v_data)) {                \
            return end;                                                        \
        }                                                                      \
    }                                                                          \
    while (0)

#define pchtml_html_tokenizer_state_append_m(tkz, v_data, size)                   \
    do {                                                                       \
        if (pchtml_html_tokenizer_temp_append(tkz, (const unsigned char *) (v_data), \
                                           (size)))                            \
        {                                                                      \
            return end;                                                        \
        }                                                                      \
    }                                                                          \
    while (0)

#define pchtml_html_tokenizer_state_append_replace_m(tkz)                         \
    do {                                                                       \
        if (pchtml_html_tokenizer_temp_append(tkz,                                \
                        pchtml_str_res_ansi_replacement_character,             \
                        sizeof(pchtml_str_res_ansi_replacement_character) - 1))\
        {                                                                      \
            return end;                                                        \
        }                                                                      \
    }                                                                          \
    while (0)

#define pchtml_html_tokenizer_state_set_tag_m(tkz, _start, _end)                  \
    do {                                                                       \
        const pchtml_tag_data_t *tag;                                             \
        tag = pchtml_tag_append_lower(tkz->tags, (_start), (_end) - (_start));    \
        if (tag == NULL) {                                                     \
            tkz->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;                  \
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);                        \
            return end;                                                        \
        }                                                                      \
        tkz->token->tag_id = tag->tag_id;                                      \
    }                                                                          \
    while (0)

#define pchtml_html_tokenizer_state_set_name_m(tkz)                               \
    do {                                                                       \
        pcedom_attr_data_t *data;                                             \
        data = pcedom_attr_local_name_append(tkz->attrs, tkz->start,          \
                                              tkz->pos - tkz->start);          \
        if (data == NULL) {                                                    \
            tkz->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;                  \
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);                        \
            return end;                                                        \
        }                                                                      \
        tkz->token->attr_last->name = data;                                    \
    }                                                                          \
    while (0)

#define pchtml_html_tokenizer_state_set_value_m(tkz)                              \
    do {                                                                       \
        pchtml_html_token_attr_t *attr = tkz->token->attr_last;                   \
                                                                               \
        attr->value_size = (size_t) (tkz->pos - tkz->start);                   \
                                                                               \
        attr->value = pchtml_mraw_alloc(tkz->attrs_mraw, attr->value_size + 1);\
        if (attr->value == NULL) {                                             \
            tkz->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;                  \
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);                        \
            return end;                                                        \
        }                                                                      \
        memcpy(attr->value, tkz->start, attr->value_size);                     \
        attr->value[attr->value_size] = 0x00;                                  \
    }                                                                          \
    while (0)

#define pchtml_html_tokenizer_state_token_set_begin(tkz, v_begin)                 \
    do {                                                                       \
        tkz->pos = tkz->start;                                                 \
        tkz->token->begin = v_begin;                                           \
    }                                                                          \
    while (0)

#define pchtml_html_tokenizer_state_token_set_end(tkz, v_end)                     \
    (tkz->token->end = v_end)

#define pchtml_html_tokenizer_state_token_set_end_down(tkz, v_end, offset)        \
    do {                                                                       \
        tkz->token->end = pchtml_in_node_pos_down(tkz->incoming_node, NULL,    \
                                                  v_end, offset);              \
    }                                                                          \
    while (0)

#define pchtml_html_tokenizer_state_token_set_end_oef(tkz)                        \
    (tkz->token->end = tkz->last)

#define pchtml_html_tokenizer_state_token_attr_add_m(tkz, attr, v_return)         \
    do {                                                                       \
        attr = pchtml_html_token_attr_append(tkz->token, tkz->dobj_token_attr);   \
        if (attr == NULL) {                                                    \
            tkz->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;                  \
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);                        \
            return v_return;                                                   \
        }                                                                      \
    }                                                                          \
    while (0)

#define pchtml_html_tokenizer_state_token_attr_set_name_begin(tkz, v_begin)       \
    do {                                                                       \
        tkz->pos = tkz->start;                                                 \
        tkz->token->attr_last->name_begin = v_begin;                           \
    }                                                                          \
    while (0)

#define pchtml_html_tokenizer_state_token_attr_set_name_end(tkz, v_end)           \
    (tkz->token->attr_last->name_end = v_end)

#define pchtml_html_tokenizer_state_token_attr_set_name_end_oef(tkz)              \
    (tkz->token->attr_last->name_end = tkz->last)

#define pchtml_html_tokenizer_state_token_attr_set_value_begin(tkz, v_begin)      \
    do {                                                                       \
        tkz->pos = tkz->start;                                                 \
        tkz->token->attr_last->value_begin = v_begin;                          \
    }                                                                          \
    while (0)

#define pchtml_html_tokenizer_state_token_attr_set_value_end(tkz, v_end)          \
    (tkz->token->attr_last->value_end = v_end)

#define pchtml_html_tokenizer_state_token_attr_set_value_end_oef(tkz)             \
    (tkz->token->attr_last->value_end = tkz->last)

#define _pchtml_html_tokenizer_state_token_done_m(tkz, v_end)                     \
    tkz->token = tkz->callback_token_done(tkz, tkz->token,                     \
                                          tkz->callback_token_ctx);            \
    if (tkz->token == NULL) {                                                  \
        if (tkz->status == PCHTML_STATUS_OK) {                                    \
            tkz->status = PCHTML_STATUS_ERROR;                                    \
            pcinst_set_error (PCHTML_ERROR);                                   \
        }                                                                      \
        return v_end;                                                          \
    }

#define pchtml_html_tokenizer_state_token_done_m(tkz, v_end)                      \
    do {                                                                       \
        if (tkz->token->begin+1 != tkz->token->end) {                            \
            _pchtml_html_tokenizer_state_token_done_m(tkz, v_end)                 \
        }                                                                      \
        pchtml_html_token_clean(tkz->token);                                      \
        tkz->pos = tkz->start;                                                 \
    }                                                                          \
    while (0)

#define pchtml_html_tokenizer_state_token_done_wo_check_m(tkz, v_end)             \
    do {                                                                       \
        _pchtml_html_tokenizer_state_token_done_m(tkz, v_end)                     \
        pchtml_html_token_clean(tkz->token);                                      \
    }                                                                          \
    while (0)

#define pchtml_html_tokenizer_state_set_text(tkz)                                 \
    do {                                                                       \
        tkz->token->text_start = tkz->start;                                   \
        tkz->token->text_end = tkz->pos;                                       \
    }                                                                          \
    while (0)

#define pchtml_html_tokenizer_state_token_emit_text_not_empty_m(tkz, v_end)       \
    do {                                                                       \
        if (tkz->token->begin+1 != tkz->token->end) {                            \
            tkz->token->tag_id = PCHTML_TAG__TEXT;                                \
                                                                               \
            pchtml_html_tokenizer_state_set_text(tkz);                            \
            _pchtml_html_tokenizer_state_token_done_m(tkz, v_end)                 \
            pchtml_html_token_clean(tkz->token);                                  \
        }                                                                      \
    }                                                                          \
    while (0)


const unsigned char *
pchtml_html_tokenizer_state_data_before(pchtml_html_tokenizer_t *tkz,
                const unsigned char *data, const unsigned char *end) WTF_INTERNAL;

const unsigned char *
pchtml_html_tokenizer_state_plaintext_before(pchtml_html_tokenizer_t *tkz,
                const unsigned char *data, const unsigned char *end) WTF_INTERNAL;

const unsigned char *
pchtml_html_tokenizer_state_before_attribute_name(pchtml_html_tokenizer_t *tkz,
                const unsigned char *data, const unsigned char *end) WTF_INTERNAL;

const unsigned char *
pchtml_html_tokenizer_state_self_closing_start_tag(pchtml_html_tokenizer_t *tkz,
                const unsigned char *data, const unsigned char *end) WTF_INTERNAL;

const unsigned char *
pchtml_html_tokenizer_state_cr(pchtml_html_tokenizer_t *tkz, const unsigned char *data,
                const unsigned char *end) WTF_INTERNAL;

const unsigned char *
pchtml_html_tokenizer_state_char_ref(pchtml_html_tokenizer_t *tkz,
                const unsigned char *data, const unsigned char *end) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_TOKENIZER_STATE_H */
