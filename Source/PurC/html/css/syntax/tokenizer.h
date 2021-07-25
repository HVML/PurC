/**
 * @file tokenizer.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for css tokenizer.
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


#ifndef PCHTML_CSS_SYNTAX_TOKENIZER_H
#define PCHTML_CSS_SYNTAX_TOKENIZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/in.h"
#include "html/core/array_obj.h"

#include "html/css/syntax/base.h"
#include "html/css/syntax/token.h"


typedef struct pchtml_css_syntax_tokenizer pchtml_css_syntax_tokenizer_t;

/* State */
typedef const unsigned char *
(*pchtml_css_syntax_tokenizer_state_f)(pchtml_css_syntax_tokenizer_t *tkz,
                                 const unsigned char *data, const unsigned char *end);

typedef pchtml_css_syntax_token_t *
(*pchtml_css_syntax_tokenizer_cb_f)(pchtml_css_syntax_tokenizer_t *tkz,
                                 pchtml_css_syntax_token_t *token, void *ctx);


enum pchtml_css_syntax_tokenizer_opt {
    PCHTML_CSS_SYNTAX_TOKENIZER_OPT_UNDEF   = 0x00,
    PCHTML_CSS_SYNTAX_TOKENIZER_OPT_WO_COPY = 0x01
};

typedef enum {
    PCHTML_CSS_SYNTAX_TOKENIZER_BEGIN   = 0x00,
    PCHTML_CSS_SYNTAX_TOKENIZER_PROCESS = 0x01,
    PCHTML_CSS_SYNTAX_TOKENIZER_END     = 0x02
}
pchtml_css_syntax_process_state_t;


typedef struct pchtml_css_syntax_tokenizer_numeric {
    unsigned char data[128];
    unsigned char *buf;
    unsigned char *end;

    int        exponent;
    int        e_digit;
    bool       is_negative;
    bool       e_is_negative;
}
pchtml_css_syntax_tokenizer_numeric_t;

struct pchtml_css_syntax_tokenizer {
    pchtml_css_syntax_tokenizer_state_f   state;
    pchtml_css_syntax_tokenizer_state_f   return_state;

    pchtml_css_syntax_tokenizer_cb_f      cb_token_done;
    void                               *cb_token_ctx;

    /* Current process token */
    pchtml_css_syntax_token_t             *token;

    /* Memory for tokens */
    pchtml_dobject_t                   *dobj_token;
    pchtml_mraw_t                      *mraw;

    /* Incoming Buffer and current process buffer */
    pchtml_in_t                        *incoming;
    pchtml_in_node_t                   *incoming_first;
    pchtml_in_node_t                   *incoming_node;
    pchtml_in_node_t                   *incoming_done;

    pchtml_array_obj_t                 *parse_errors;

    /* Temp */
    int                                count;
    size_t                             num;
    const unsigned char                   *begin;
    const unsigned char                   *end;
    unsigned char                         str_ending;
    pchtml_css_syntax_tokenizer_numeric_t numeric;
    pchtml_css_syntax_token_data_t        token_data;

    /* Process */
    unsigned int                       opt;             /* bitmap */
    pchtml_css_syntax_process_state_t     process_state;
    unsigned int                       status;
    bool                               is_eof;
    bool                               reuse;
};


pchtml_css_syntax_tokenizer_t *
pchtml_css_syntax_tokenizer_create(void) WTF_INTERNAL;

unsigned int
pchtml_css_syntax_tokenizer_init(
                pchtml_css_syntax_tokenizer_t *tkz) WTF_INTERNAL;

void
pchtml_css_syntax_tokenizer_clean(
                pchtml_css_syntax_tokenizer_t *tkz) WTF_INTERNAL;

pchtml_css_syntax_tokenizer_t *
pchtml_css_syntax_tokenizer_destroy(
                pchtml_css_syntax_tokenizer_t *tkz) WTF_INTERNAL;


unsigned int
pchtml_css_syntax_tokenizer_parse(pchtml_css_syntax_tokenizer_t *tkz,
                const unsigned char *data, size_t size) WTF_INTERNAL;

unsigned int
pchtml_css_syntax_tokenizer_begin(
                pchtml_css_syntax_tokenizer_t *tkz) WTF_INTERNAL;

unsigned int
pchtml_css_syntax_tokenizer_chunk(pchtml_css_syntax_tokenizer_t *tkz,
                const unsigned char *data, size_t size) WTF_INTERNAL;

unsigned int
pchtml_css_syntax_tokenizer_end(
                pchtml_css_syntax_tokenizer_t *tkz) WTF_INTERNAL;

const unsigned char *
pchtml_css_syntax_tokenizer_change_incoming(pchtml_css_syntax_tokenizer_t *tkz,
                const unsigned char *pos) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline void
pchtml_css_syntax_tokenizer_token_cb_set(pchtml_css_syntax_tokenizer_t *tkz,
                                      pchtml_css_syntax_tokenizer_cb_f cb_done,
                                      void *ctx)
{
    tkz->cb_token_done = cb_done;
    tkz->cb_token_ctx = ctx;
}

static inline void
pchtml_css_syntax_tokenizer_last_needed_in(pchtml_css_syntax_tokenizer_t *tkz,
                                        pchtml_in_node_t *in)
{
    tkz->incoming_done = in;
}

static inline unsigned int
pchtml_css_syntax_tokenizer_make_data(pchtml_css_syntax_tokenizer_t *tkz,
                                   pchtml_css_syntax_token_t *token)
{
    unsigned int status;

    status = pchtml_css_syntax_token_make_data(token, tkz->incoming_node,
                                            tkz->mraw, &tkz->token_data);
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    pchtml_css_syntax_tokenizer_last_needed_in(tkz, tkz->token_data.node_done);

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_css_syntax_tokenizer_status(pchtml_css_syntax_tokenizer_t *tkz)
{
    return tkz->status;
}

/*
 * No inline functions for ABI.
 */
void
pchtml_css_syntax_tokenizer_token_cb_set_noi(pchtml_css_syntax_tokenizer_t *tkz,
                                          pchtml_css_syntax_tokenizer_cb_f cb_done,
                                          void *ctx);

void
pchtml_css_syntax_tokenizer_last_needed_in_noi(pchtml_css_syntax_tokenizer_t *tkz,
                                            pchtml_in_node_t *in);

unsigned int
pchtml_css_syntax_tokenizer_make_data_noi(pchtml_css_syntax_tokenizer_t *tkz,
                                       pchtml_css_syntax_token_t *token);

unsigned int
pchtml_css_syntax_tokenizer_status_noi(pchtml_css_syntax_tokenizer_t *tkz);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_CSS_SYNTAX_TOKENIZER_H */
