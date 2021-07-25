/**
 * @file tokenizer.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of css tokenizer.
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

#include "html/css/syntax/tokenizer.h"
#include "html/css/syntax/tokenizer/error.h"
#include "html/css/syntax/state.h"


const unsigned char *pchtml_css_syntax_tokenizer_eof = (const unsigned char *) "\x00";


static void
pchtml_css_syntax_tokenizer_erase_incoming(pchtml_css_syntax_tokenizer_t *tkz);

static pchtml_css_syntax_token_t *
pchtml_css_syntax_tokenizer_cb_done(pchtml_css_syntax_tokenizer_t *tkz,
                                 pchtml_css_syntax_token_t *token, void *ctx);

static void
pchtml_css_syntax_tokenizer_process(pchtml_css_syntax_tokenizer_t *tkz,
                                 const unsigned char *data, size_t size);

static const unsigned char *
pchtml_css_syntax_tokenizer_change_incoming_eof(pchtml_css_syntax_tokenizer_t *tkz,
                                             const unsigned char *pos);


pchtml_css_syntax_tokenizer_t *
pchtml_css_syntax_tokenizer_create(void)
{
    return pchtml_calloc(1, sizeof(pchtml_css_syntax_tokenizer_t));
}

unsigned int
pchtml_css_syntax_tokenizer_init(pchtml_css_syntax_tokenizer_t *tkz)
{
    if (tkz == NULL) {
        return PCHTML_STATUS_ERROR_OBJECT_IS_NULL;
    }

    unsigned int status;

    /* Init Token */
    tkz->token = NULL;

    tkz->dobj_token = pchtml_dobject_create();
    status = pchtml_dobject_init(tkz->dobj_token,
                                 4096, sizeof(pchtml_css_syntax_token_t));
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    /* Incoming */
    tkz->incoming = pchtml_in_create();
    status = pchtml_in_init(tkz->incoming, 32);
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    tkz->incoming_first = NULL;
    tkz->incoming_node = NULL;
    tkz->incoming_done = NULL;

    /* mraw */
    tkz->mraw = pchtml_mraw_create();
    status = pchtml_mraw_init(tkz->mraw, 1024);
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    /* Parse errors */
    tkz->parse_errors = pchtml_array_obj_create();
    status = pchtml_array_obj_init(tkz->parse_errors, 16,
                                   sizeof(pchtml_css_syntax_tokenizer_error_t));
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    tkz->cb_token_done = pchtml_css_syntax_tokenizer_cb_done;
    tkz->cb_token_ctx = NULL;

    tkz->is_eof = false;
    tkz->status = PCHTML_STATUS_OK;

    tkz->opt = PCHTML_CSS_SYNTAX_TOKENIZER_OPT_UNDEF;
    tkz->process_state = PCHTML_CSS_SYNTAX_TOKENIZER_BEGIN;

    tkz->numeric.end = tkz->numeric.data
        + sizeof(tkz->numeric.data) / sizeof(unsigned char);

    return PCHTML_STATUS_OK;
}

void
pchtml_css_syntax_tokenizer_clean(pchtml_css_syntax_tokenizer_t *tkz)
{
    pchtml_css_syntax_tokenizer_erase_incoming(tkz);

    pchtml_in_clean(tkz->incoming);

    pchtml_mraw_clean(tkz->mraw);
    pchtml_dobject_clean(tkz->dobj_token);
    pchtml_array_obj_clean(tkz->parse_errors);

    tkz->status = PCHTML_STATUS_OK;
    tkz->process_state = PCHTML_CSS_SYNTAX_TOKENIZER_BEGIN;
}

pchtml_css_syntax_tokenizer_t *
pchtml_css_syntax_tokenizer_destroy(pchtml_css_syntax_tokenizer_t *tkz)
{
    if (tkz == NULL) {
        return NULL;
    }

    pchtml_css_syntax_tokenizer_erase_incoming(tkz);

    tkz->incoming = pchtml_in_destroy(tkz->incoming, true);

    tkz->mraw = pchtml_mraw_destroy(tkz->mraw, true);
    tkz->dobj_token = pchtml_dobject_destroy(tkz->dobj_token, true);
    tkz->parse_errors = pchtml_array_obj_destroy(tkz->parse_errors, true);

    return pchtml_free(tkz);
}

static void
pchtml_css_syntax_tokenizer_erase_incoming(pchtml_css_syntax_tokenizer_t *tkz)
{
    pchtml_in_node_t *next_node;

    while (tkz->incoming_first != NULL)
    {
        if (tkz->incoming_first->opt & PCHTML_IN_OPT_ALLOC) {
            pchtml_free((unsigned char *) tkz->incoming_first->begin);
        }

        next_node = tkz->incoming_first->next;

        pchtml_in_node_destroy(tkz->incoming, tkz->incoming_first, true);

        tkz->incoming_first = next_node;
    }

    tkz->incoming_done = NULL;
}

static pchtml_css_syntax_token_t *
pchtml_css_syntax_tokenizer_cb_done(pchtml_css_syntax_tokenizer_t *tkz,
                                 pchtml_css_syntax_token_t *token, void *ctx)
{
    UNUSED_PARAM(tkz);
    UNUSED_PARAM(token);
    UNUSED_PARAM(ctx);
    return token;
}

unsigned int
pchtml_css_syntax_tokenizer_begin(pchtml_css_syntax_tokenizer_t *tkz)
{
    if (tkz->process_state == PCHTML_CSS_SYNTAX_TOKENIZER_PROCESS) {
        return PCHTML_STATUS_ERROR_WRONG_STAGE;
    }

    tkz->is_eof = false;
    tkz->status = PCHTML_STATUS_OK;
    tkz->state = pchtml_css_syntax_state_data;
    tkz->opt = PCHTML_CSS_SYNTAX_TOKENIZER_OPT_UNDEF;

    if (tkz->token == NULL) {
        tkz->token = pchtml_css_syntax_token_create(tkz->dobj_token);
        if (tkz->token == NULL) {
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    tkz->process_state = PCHTML_CSS_SYNTAX_TOKENIZER_PROCESS;

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_css_syntax_tokenizer_chunk(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, size_t size)
{
    if (tkz->process_state != PCHTML_CSS_SYNTAX_TOKENIZER_PROCESS) {
        tkz->status = PCHTML_STATUS_ERROR_WRONG_STAGE;

        return tkz->status;
    }

    unsigned char *copied;
    pchtml_in_node_t *next_node;

    if (tkz->opt & PCHTML_CSS_SYNTAX_TOKENIZER_OPT_WO_COPY) {
        tkz->incoming_node = pchtml_in_node_make(tkz->incoming, tkz->incoming_node,
                                                 data, size);
        if (tkz->incoming_node == NULL) {
            pchtml_css_syntax_tokenizer_erase_incoming(tkz);

            tkz->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

            return tkz->status;
        }

        if (tkz->incoming_first == NULL) {
            tkz->incoming_first = tkz->incoming_node;
        }

        pchtml_css_syntax_tokenizer_process(tkz, data, size);

        goto done;
    }

    copied = pchtml_malloc(sizeof(unsigned char) * size);
    if (copied == NULL) {
        pchtml_css_syntax_tokenizer_erase_incoming(tkz);

        tkz->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return tkz->status;
    }

    memcpy(copied, data, sizeof(unsigned char) * size);

    tkz->incoming_node = pchtml_in_node_make(tkz->incoming, tkz->incoming_node,
                                             copied, size);
    if (tkz->incoming_node == NULL) {
        pchtml_free(copied);
        pchtml_css_syntax_tokenizer_erase_incoming(tkz);

        tkz->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return tkz->status;
    }

    if (tkz->incoming_first == NULL) {
        tkz->incoming_first = tkz->incoming_node;
    }

    tkz->incoming_node->opt = PCHTML_IN_OPT_ALLOC;

    pchtml_css_syntax_tokenizer_process(tkz, copied, size);

done:

    if (tkz->status != PCHTML_STATUS_OK) {
        pchtml_css_syntax_tokenizer_erase_incoming(tkz);

        return tkz->status;
    }

    if (tkz->incoming_done != NULL) {
        while (tkz->incoming_first != tkz->incoming_done) {
            if (tkz->incoming_first->opt & PCHTML_IN_OPT_ALLOC) {
                pchtml_free((unsigned char *) tkz->incoming_first->begin);
            }

            next_node = tkz->incoming_first->next;

            pchtml_in_node_destroy(tkz->incoming, tkz->incoming_first, true);

            tkz->incoming_first = next_node;
            next_node->prev = NULL;
        }
    }

    return tkz->status;
}

static void
pchtml_css_syntax_tokenizer_process(pchtml_css_syntax_tokenizer_t *tkz,
                                 const unsigned char *data, size_t size)
{
    pchtml_in_node_t *in_node;
    const unsigned char *end = data + size;

    while (data < end) {
        data = tkz->state(tkz, data, end);
    }

    if (tkz->incoming_node->next != NULL) {

reuse:

        in_node = tkz->incoming_node;
        data = in_node->use;

        for (;;) {
            while (data < in_node->end) {
                data = tkz->state(tkz, data, in_node->end);
            }

            if (in_node != tkz->incoming_node) {
                goto reuse;
            }

            in_node->use = in_node->end;

            if (in_node->next == NULL) {
                break;
            }

            in_node = in_node->next;
            tkz->incoming_node = in_node;

            data = in_node->begin;
        }
    }

    tkz->incoming_node->use = end;
}

unsigned int
pchtml_css_syntax_tokenizer_end(pchtml_css_syntax_tokenizer_t *tkz)
{
    if (tkz->process_state != PCHTML_CSS_SYNTAX_TOKENIZER_PROCESS) {
        tkz->status = PCHTML_STATUS_ERROR_WRONG_STAGE;

        return tkz->status;
    }

    const unsigned char *data, *end;

    /*
     * Send a fake EOF data (not added in to incoming buffer chain)
     * If some state change incoming buffer,
     * then we need parse again all buffers after current position
     * and try again send fake EOF.
     */
    do {
        data = pchtml_css_syntax_tokenizer_eof;
        end = pchtml_css_syntax_tokenizer_eof + 1UL;

        tkz->is_eof = true;

        while (tkz->state(tkz, data, end) < end) {
            /* empty loop */
        }

        if (tkz->reuse) {
            tkz->is_eof = false;
            data = tkz->incoming_node->use;

            for (;;) {
                while (data < tkz->incoming_node->end) {
                    data = tkz->state(tkz, data, tkz->incoming_node->end);
                }

                if (tkz->incoming_node->next == NULL) {
                    break;
                }

                tkz->incoming_node->use = tkz->incoming_node->end;
                tkz->incoming_node = tkz->incoming_node->next;

                data = tkz->incoming_node->begin;
            }

            tkz->reuse = false;
        }
        else {
            break;
        }
    }
    while (1);

    if (tkz->status != PCHTML_STATUS_OK) {
        return tkz->status;
    }

    tkz->is_eof = false;

    /* Emit token: END OF FILE */
    pchtml_css_syntax_token_clean(tkz->token);

    tkz->token->types.base.type = PCHTML_CSS_SYNTAX_TOKEN__EOF;

    tkz->token = tkz->cb_token_done(tkz, tkz->token, tkz->cb_token_ctx);

    if (tkz->token == NULL && tkz->status == PCHTML_STATUS_OK) {
        tkz->status = PCHTML_STATUS_ERROR;
    }

    tkz->process_state = PCHTML_CSS_SYNTAX_TOKENIZER_END;

    return tkz->status;
}

unsigned int
pchtml_css_syntax_tokenizer_parse(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, size_t size)
{
    unsigned int old_opt = tkz->opt;

    tkz->opt |= PCHTML_CSS_SYNTAX_TOKENIZER_OPT_WO_COPY;

    pchtml_css_syntax_tokenizer_begin(tkz);
    if (tkz->status != PCHTML_STATUS_OK) {
        goto done;
    }

    pchtml_css_syntax_tokenizer_chunk(tkz, data, size);
    if (tkz->status != PCHTML_STATUS_OK) {
        goto done;
    }

    pchtml_css_syntax_tokenizer_end(tkz);

done:

    tkz->opt = old_opt;

    return tkz->status;
}

const unsigned char *
pchtml_css_syntax_tokenizer_change_incoming(pchtml_css_syntax_tokenizer_t *tkz,
                                         const unsigned char *pos)
{
    if (tkz->is_eof) {
        return pchtml_css_syntax_tokenizer_change_incoming_eof(tkz, pos);
    }

    if (pchtml_in_segment(tkz->incoming_node, pos)) {
        tkz->incoming_node->use = pos;

        return pos;
    }

    pchtml_in_node_t *node = tkz->incoming_node;
    tkz->incoming_node = pchtml_in_node_find(tkz->incoming_node, pos);

    if (tkz->incoming_node == NULL) {
        tkz->status = PCHTML_STATUS_ERROR;
        tkz->incoming_node = node;

        return tkz->incoming_node->end;
    }

    tkz->incoming_node->use = pos;

    return node->end;
}

static const unsigned char *
pchtml_css_syntax_tokenizer_change_incoming_eof(pchtml_css_syntax_tokenizer_t *tkz,
                                             const unsigned char *pos)
{
    if (pos == pchtml_css_syntax_tokenizer_eof) {
        return pos;
    }

    tkz->reuse = true;

    if (pchtml_in_segment(tkz->incoming_node, pos)) {
        tkz->incoming_node->use = pos;

        return pchtml_css_syntax_tokenizer_eof + 1;
    }

    pchtml_in_node_t *node = tkz->incoming_node;
    tkz->incoming_node = pchtml_in_node_find(tkz->incoming_node, pos);

    if (tkz->incoming_node == NULL) {
        tkz->reuse = false;

        tkz->status = PCHTML_STATUS_ERROR;
        tkz->incoming_node = node;

        return pchtml_css_syntax_tokenizer_eof + 1;
    }

    tkz->incoming_node->use = pos;

    return pchtml_css_syntax_tokenizer_eof + 1;
}

/*
 * No inline functions for ABI.
 */
void
pchtml_css_syntax_tokenizer_token_cb_set_noi(pchtml_css_syntax_tokenizer_t *tkz,
                                          pchtml_css_syntax_tokenizer_cb_f cb_done,
                                          void *ctx)
{
    pchtml_css_syntax_tokenizer_token_cb_set(tkz, cb_done, ctx);
}

void
pchtml_css_syntax_tokenizer_last_needed_in_noi(pchtml_css_syntax_tokenizer_t *tkz,
                                            pchtml_in_node_t *in)
{
    pchtml_css_syntax_tokenizer_last_needed_in(tkz, in);
}

unsigned int
pchtml_css_syntax_tokenizer_make_data_noi(pchtml_css_syntax_tokenizer_t *tkz,
                                       pchtml_css_syntax_token_t *token)
{
    return pchtml_css_syntax_tokenizer_make_data(tkz, token);
}

unsigned int
pchtml_css_syntax_tokenizer_status_noi(pchtml_css_syntax_tokenizer_t *tkz)
{
    return pchtml_css_syntax_tokenizer_status(tkz);
}
