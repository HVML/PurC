/**
 * @file tokenizer.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html tokenizer.
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
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#ifndef PCHTML_HTML_TOKENIZER_H
#define PCHTML_HTML_TOKENIZER_H

#include "config.h"

#include "private/array_obj.h"
#include "purc-rwstream.h"

#include "html/in.h"
#include "private/sbst.h"
#include "html/base.h"
#include "html/token.h"
#include "html/tag.h"


/* State */
typedef const unsigned char *
(*pchtml_html_tokenizer_state_f)(pchtml_html_tokenizer_t *tkz,
                              const unsigned char *data, const unsigned char *end);

typedef pchtml_html_token_t *
(*pchtml_html_tokenizer_token_f)(pchtml_html_tokenizer_t *tkz,
                              pchtml_html_token_t *token, void *ctx);


struct pchtml_html_tokenizer {
    pchtml_html_tokenizer_state_f       state;
    pchtml_html_tokenizer_state_f       state_return;

    pchtml_html_tokenizer_token_f       callback_token_done;
    void                             *callback_token_ctx;

    pcutils_hash_t                    *tags;
    pcutils_hash_t                    *attrs;
    pcutils_mraw_t                    *attrs_mraw;

    /* For a temp strings and other templary data */
    pcutils_mraw_t                    *mraw;

    /* Current process token */
    pchtml_html_token_t                 *token;

    /* Memory for token and attr */
    pcutils_dobject_t                 *dobj_token;
    pcutils_dobject_t                 *dobj_token_attr;

    /* Parse error */
    pcutils_array_obj_t               *parse_errors;

    /*
     * Leak abstractions.
     * The only place where the specification causes mixing Tree Builder
     * and Tokenizer. We kill all beauty.
     * Current Tree parser. This is not ref (not ref count).
     */
    pchtml_html_tree_t                  *tree;

    /* Temp */
    const unsigned char                 *markup;
    const unsigned char                 *temp;
    pchtml_tag_id_t                     tmp_tag_id;

    unsigned char                       *start;
    unsigned char                       *pos;
    const unsigned char                 *end;
    const unsigned char                 *begin;
    const unsigned char                 *last;

    /* Entities */
    const pcutils_sbst_entry_static_t *entity;
    const pcutils_sbst_entry_static_t *entity_match;
    uintptr_t                        entity_start;
    uintptr_t                        entity_end;
    uint32_t                         entity_length;
    uint32_t                         entity_number;
    bool                             is_attribute;

    /* Process */
    pchtml_html_tokenizer_opt_t         opt;
    unsigned int                     status;
    bool                             is_eof;

    pchtml_html_tokenizer_t             *base;
    size_t                           ref_count;
};


#include "html/tokenizer/error.h"


extern const unsigned char *pchtml_html_tokenizer_eof;

#ifdef __cplusplus
extern "C" {
#endif

pchtml_html_tokenizer_t *
pchtml_html_tokenizer_create(void) WTF_INTERNAL;

unsigned int
pchtml_html_tokenizer_init(pchtml_html_tokenizer_t *tkz) WTF_INTERNAL;

unsigned int
pchtml_html_tokenizer_inherit(pchtml_html_tokenizer_t *tkz_to,
                pchtml_html_tokenizer_t *tkz_from) WTF_INTERNAL;

pchtml_html_tokenizer_t *
pchtml_html_tokenizer_ref(pchtml_html_tokenizer_t *tkz) WTF_INTERNAL;

pchtml_html_tokenizer_t *
pchtml_html_tokenizer_unref(pchtml_html_tokenizer_t *tkz) WTF_INTERNAL;

void
pchtml_html_tokenizer_clean(pchtml_html_tokenizer_t *tkz) WTF_INTERNAL;

pchtml_html_tokenizer_t *
pchtml_html_tokenizer_destroy(pchtml_html_tokenizer_t *tkz) WTF_INTERNAL;

unsigned int
pchtml_html_tokenizer_tags_make(pchtml_html_tokenizer_t *tkz, 
                size_t table_size) WTF_INTERNAL;

void
pchtml_html_tokenizer_tags_destroy(pchtml_html_tokenizer_t *tkz) WTF_INTERNAL;

unsigned int
pchtml_html_tokenizer_attrs_make(pchtml_html_tokenizer_t *tkz, 
                size_t table_size) WTF_INTERNAL;

void
pchtml_html_tokenizer_attrs_destroy(pchtml_html_tokenizer_t *tkz) WTF_INTERNAL;

unsigned int
pchtml_html_tokenizer_begin(pchtml_html_tokenizer_t *tkz) WTF_INTERNAL;

unsigned int
pchtml_html_tokenizer_chunk(pchtml_html_tokenizer_t *tkz,
                const unsigned char *data, size_t sz) WTF_INTERNAL;

unsigned int
pchtml_html_tokenizer_end(pchtml_html_tokenizer_t *tkz) WTF_INTERNAL;


const unsigned char *
pchtml_html_tokenizer_change_incoming(pchtml_html_tokenizer_t *tkz,
                const unsigned char *pos) WTF_INTERNAL;

pchtml_ns_id_t
pchtml_html_tokenizer_current_namespace(pchtml_html_tokenizer_t *tkz);

void
pchtml_html_tokenizer_set_state_by_tag(pchtml_html_tokenizer_t *tkz, bool scripting,
                pchtml_tag_id_t tag_id, pchtml_ns_id_t ns) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline void
pchtml_html_tokenizer_status_set(pchtml_html_tokenizer_t *tkz, unsigned int status)
{
    tkz->status = status;
}

static inline void
pchtml_html_tokenizer_tags_set(pchtml_html_tokenizer_t *tkz, pcutils_hash_t *tags)
{
    tkz->tags = tags;
}

static inline pcutils_hash_t *
pchtml_html_tokenizer_tags(pchtml_html_tokenizer_t *tkz)
{
    return tkz->tags;
}

static inline void
pchtml_html_tokenizer_attrs_set(pchtml_html_tokenizer_t *tkz, pcutils_hash_t *attrs)
{
    tkz->attrs = attrs;
}

static inline pcutils_hash_t *
pchtml_html_tokenizer_attrs(pchtml_html_tokenizer_t *tkz)
{
    return tkz->attrs;
}

static inline void
pchtml_html_tokenizer_attrs_mraw_set(pchtml_html_tokenizer_t *tkz,
                                  pcutils_mraw_t *mraw)
{
    tkz->attrs_mraw = mraw;
}

static inline pcutils_mraw_t *
pchtml_html_tokenizer_attrs_mraw(pchtml_html_tokenizer_t *tkz)
{
    return tkz->attrs_mraw;
}

static inline void
pchtml_html_tokenizer_callback_token_done_set(pchtml_html_tokenizer_t *tkz,
                                           pchtml_html_tokenizer_token_f call_func,
                                           void *ctx)
{
    tkz->callback_token_done = call_func;
    tkz->callback_token_ctx = ctx;
}

static inline void *
pchtml_html_tokenizer_callback_token_done_ctx(pchtml_html_tokenizer_t *tkz)
{
    return tkz->callback_token_ctx;
}

static inline void
pchtml_html_tokenizer_state_set(pchtml_html_tokenizer_t *tkz,
                             pchtml_html_tokenizer_state_f state)
{
    tkz->state = state;
}

static inline void
pchtml_html_tokenizer_tmp_tag_id_set(pchtml_html_tokenizer_t *tkz,
                                  pchtml_tag_id_t tag_id)
{
    tkz->tmp_tag_id = tag_id;
}

static inline pchtml_html_tree_t *
pchtml_html_tokenizer_tree(pchtml_html_tokenizer_t *tkz)
{
    return tkz->tree;
}

static inline void
pchtml_html_tokenizer_tree_set(pchtml_html_tokenizer_t *tkz, pchtml_html_tree_t *tree)
{
    tkz->tree = tree;
}

static inline pcutils_mraw_t *
pchtml_html_tokenizer_mraw(pchtml_html_tokenizer_t *tkz)
{
    return tkz->mraw;
}

static inline unsigned int
pchtml_html_tokenizer_temp_realloc(pchtml_html_tokenizer_t *tkz, size_t size)
{
    size_t length = tkz->pos - tkz->start;
    size_t new_size = (tkz->end - tkz->start) + size + 4096;

    tkz->start = (unsigned char *)pcutils_realloc(tkz->start, new_size);
    if (tkz->start == NULL) {
        tkz->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        return tkz->status;
    }

    tkz->pos = tkz->start + length;
    tkz->end = tkz->start + new_size;

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_html_tokenizer_temp_append_data(pchtml_html_tokenizer_t *tkz,
                                    const unsigned char *data)
{
    size_t size = data - tkz->begin;

    if ((tkz->pos + size) > tkz->end) {
        if(pchtml_html_tokenizer_temp_realloc(tkz, size)) {
            return tkz->status;
        }
    }

    tkz->pos = (unsigned char *) memcpy(tkz->pos, tkz->begin, size) + size;

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_html_tokenizer_temp_append(pchtml_html_tokenizer_t *tkz,
                               const unsigned char *data, size_t size)
{
    if ((tkz->pos + size) > tkz->end) {
        if(pchtml_html_tokenizer_temp_realloc(tkz, size)) {
            return tkz->status;
        }
    }

    tkz->pos = (unsigned char *) memcpy(tkz->pos, data, size) + size;

    return PCHTML_STATUS_OK;
}


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_TOKENIZER_H */
