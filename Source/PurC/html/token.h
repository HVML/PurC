/**
 * @file token.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html token.
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

#ifndef PCHTML_HTML_TOKEN_H
#define PCHTML_HTML_TOKEN_H

#include "config.h"
#include "html/dobject.h"
#include "html/in.h"
#include "html/str.h"

#include "html/base.h"
#include "html/token_attr.h"
#include "html/tag.h"


typedef int pchtml_html_token_type_t;


enum pchtml_html_token_type {
    PCHTML_HTML_TOKEN_TYPE_OPEN         = 0x0000,
    PCHTML_HTML_TOKEN_TYPE_CLOSE        = 0x0001,
    PCHTML_HTML_TOKEN_TYPE_CLOSE_SELF   = 0x0002,
    PCHTML_HTML_TOKEN_TYPE_FORCE_QUIRKS = 0x0004,
    PCHTML_HTML_TOKEN_TYPE_DONE         = 0x0008
};

typedef struct {
    const unsigned char      *begin;
    const unsigned char      *end;

    const unsigned char      *text_start;
    const unsigned char      *text_end;

    pchtml_in_node_t      *in_begin;

    pchtml_html_token_attr_t *attr_first;
    pchtml_html_token_attr_t *attr_last;

    void                  *base_element;

    size_t                null_count;
    pchtml_tag_id_t          tag_id;
    pchtml_html_token_type_t type;
}
pchtml_html_token_t;


#ifdef __cplusplus
extern "C" {
#endif

pchtml_html_token_t *
pchtml_html_token_create(pchtml_dobject_t *dobj) WTF_INTERNAL;

pchtml_html_token_t *
pchtml_html_token_destroy(pchtml_html_token_t *token, 
                pchtml_dobject_t *dobj) WTF_INTERNAL;

pchtml_html_token_attr_t *
pchtml_html_token_attr_append(pchtml_html_token_t *token, 
                pchtml_dobject_t *dobj) WTF_INTERNAL;

void
pchtml_html_token_attr_remove(pchtml_html_token_t *token,
                pchtml_html_token_attr_t *attr) WTF_INTERNAL;

void
pchtml_html_token_attr_delete(pchtml_html_token_t *token,
                pchtml_html_token_attr_t *attr, 
                pchtml_dobject_t *dobj) WTF_INTERNAL;

unsigned int
pchtml_html_token_make_text(pchtml_html_token_t *token, pchtml_str_t *str,
                pchtml_mraw_t *mraw) WTF_INTERNAL;

unsigned int
pchtml_html_token_make_text_drop_null(pchtml_html_token_t *token, 
                pchtml_str_t *str, pchtml_mraw_t *mraw) WTF_INTERNAL;

unsigned int
pchtml_html_token_make_text_replace_null(pchtml_html_token_t *token,
                pchtml_str_t *str, pchtml_mraw_t *mraw) WTF_INTERNAL;

unsigned int
pchtml_html_token_data_skip_ws_begin(pchtml_html_token_t *token) WTF_INTERNAL;

unsigned int
pchtml_html_token_data_skip_one_newline_begin(
                pchtml_html_token_t *token) WTF_INTERNAL;

unsigned int
pchtml_html_token_data_split_ws_begin(pchtml_html_token_t *token,
                pchtml_html_token_t *ws_token) WTF_INTERNAL;

unsigned int
pchtml_html_token_doctype_parse(pchtml_html_token_t *token,
                pcedom_document_type_t *doc_type) WTF_INTERNAL;

pchtml_html_token_attr_t *
pchtml_html_token_find_attr(pchtml_html_tokenizer_t *tkz, pchtml_html_token_t *token,
                const unsigned char *name, size_t name_len) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline void
pchtml_html_token_clean(pchtml_html_token_t *token)
{
    memset(token, 0, sizeof(pchtml_html_token_t));
}

static inline pchtml_html_token_t *
pchtml_html_token_create_eof(pchtml_dobject_t *dobj)
{
    return (pchtml_html_token_t *) pchtml_dobject_calloc(dobj);
}


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_TOKEN_H */

