/**
 * @file token.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of html token.
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


#include "html/html/token.h"
#include "html/html/tokenizer.h"

#define PCHTML_STR_RES_MAP_LOWERCASE
#define PCHTML_STR_RES_ANSI_REPLACEMENT_CHARACTER
#define PCHTML_STR_RES_MAP_HEX
#define PCHTML_STR_RES_MAP_NUM
#include "html/core/str_res.h"

#include "private/edom/document_type.h"


const pchtml_tag_data_t *
pchtml_tag_append_lower(pchtml_hash_t *hash,
                     const unsigned char *name, size_t length);


pchtml_html_token_t *
pchtml_html_token_create(pchtml_dobject_t *dobj)
{
    return pchtml_dobject_calloc(dobj);
}

pchtml_html_token_t *
pchtml_html_token_destroy(pchtml_html_token_t *token, pchtml_dobject_t *dobj)
{
    return pchtml_dobject_free(dobj, token);
}

pchtml_html_token_attr_t *
pchtml_html_token_attr_append(pchtml_html_token_t *token, pchtml_dobject_t *dobj)
{
    pchtml_html_token_attr_t *attr = pchtml_html_token_attr_create(dobj);
    if (attr == NULL) {
        return NULL;
    }

    if (token->attr_last == NULL) {
        token->attr_first = attr;
        token->attr_last = attr;

        return attr;
    }

    token->attr_last->next = attr;
    attr->prev = token->attr_last;

    token->attr_last = attr;

    return attr;
}

void
pchtml_html_token_attr_remove(pchtml_html_token_t *token, pchtml_html_token_attr_t *attr)
{
    if (token->attr_first == attr) {
        token->attr_first = attr->next;
    }

    if (token->attr_last == attr) {
        token->attr_last = attr->prev;
    }

    if (attr->next != NULL) {
        attr->next->prev = attr->prev;
    }

    if (attr->prev != NULL) {
        attr->prev->next = attr->next;
    }

    attr->next = NULL;
    attr->prev = NULL;
}

void
pchtml_html_token_attr_delete(pchtml_html_token_t *token,
                           pchtml_html_token_attr_t *attr, pchtml_dobject_t *dobj)
{
    pchtml_html_token_attr_remove(token, attr);
    pchtml_html_token_attr_destroy(attr, dobj);
}

unsigned int
pchtml_html_token_make_text(pchtml_html_token_t *token, pchtml_str_t *str,
                         pchtml_mraw_t *mraw)
{
    size_t len = token->text_end - token->text_start;

    (void) pchtml_str_init(str, mraw, len);
    if (str->data == NULL) {
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    memcpy(str->data, token->text_start, len);

    str->data[len] = 0x00;
    str->length = len;

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_token_make_text_drop_null(pchtml_html_token_t *token, pchtml_str_t *str,
                                   pchtml_mraw_t *mraw)
{
    unsigned char *p, c;
    const unsigned char *data = token->text_start;
    const unsigned char *end = token->text_end;

    size_t len = (end - data) - token->null_count;

    (void) pchtml_str_init(str, mraw, len);
    if (str->data == NULL) {
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    p = str->data;

    while (data < end) {
        c = *data++;

        if (c != 0x00) {
            *p++ = c;
        }
    }

    str->data[len] = 0x00;
    str->length = len;

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_token_make_text_replace_null(pchtml_html_token_t *token,
                                      pchtml_str_t *str, pchtml_mraw_t *mraw)
{
    unsigned char *p, c;
    const unsigned char *data = token->text_start;
    const unsigned char *end = token->text_end;

    static const unsigned rep_len = sizeof(pchtml_str_res_ansi_replacement_character) - 1;

    size_t len = (end - data) + (token->null_count * rep_len) - token->null_count;

    (void) pchtml_str_init(str, mraw, len);
    if (str->data == NULL) {
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    p = str->data;

    while (data < end) {
        c = *data++;

        if (c == 0x00) {
            memcpy(p, pchtml_str_res_ansi_replacement_character, rep_len);
            p += rep_len;

            continue;
        }

        *p++ = c;
    }

    str->data[len] = 0x00;
    str->length = len;

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_token_data_skip_ws_begin(pchtml_html_token_t *token)
{
    const unsigned char *data = token->text_start;
    const unsigned char *end = token->text_end;

    while (data < end) {
        switch (*data) {
            /*
             * U+0009 CHARACTER TABULATION (tab)
             * U+000A LINE FEED (LF)
             * U+000C FORM FEED (FF)
             * U+0020 SPACE
             */
            case 0x09:
            case 0x0A:
            case 0x0D:
            case 0x20:
                break;

            default:
                token->begin += data - token->text_start;
                token->text_start = data;

                return PCHTML_STATUS_OK;
        }

        data++;
    }

    token->begin += data - token->text_start;
    token->text_start = data;

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_token_data_skip_one_newline_begin(pchtml_html_token_t *token)
{
    const unsigned char *data = token->text_start;
    const unsigned char *end = token->text_end;

    if (data < end) {
        /* U+000A LINE FEED (LF) */
        if (*data == 0x0A) {
            token->begin++;
            token->text_start++;
        }
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_token_data_split_ws_begin(pchtml_html_token_t *token,
                                   pchtml_html_token_t *ws_token)
{
    *ws_token = *token;

    unsigned int status = pchtml_html_token_data_skip_ws_begin(token);
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    if (token->text_start == token->text_end) {
        return PCHTML_STATUS_OK;
    }

    if (token->text_start == ws_token->text_start) {
        memset(ws_token, 0, sizeof(pchtml_html_token_t));

        return PCHTML_STATUS_OK;
    }

    ws_token->end = token->begin;
    ws_token->text_end = token->text_start;

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_html_token_doctype_parse(pchtml_html_token_t *token,
                             pcedom_document_type_t *doc_type)
{
    pchtml_html_token_attr_t *attr;
    pchtml_mraw_t *mraw = doc_type->node.owner_document->mraw;

    /* Set all to empty string if attr not exist */
    if (token->attr_first == NULL) {
        goto set_name_pub_sys_empty;
    }

    /* Name */
    attr = token->attr_first;

    doc_type->name = attr->name->attr_id;

    /* PUBLIC or SYSTEM */
    attr = attr->next;
    if (attr == NULL) {
        goto set_pub_sys_empty;
    }

    if (attr->name->attr_id == PCEDOM_ATTR_PUBLIC) {
        (void) pchtml_str_init(&doc_type->public_id, mraw, attr->value_size);
        if (doc_type->public_id.data == NULL) {
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        if (attr->value_begin == NULL) {
            return PCHTML_STATUS_OK;
        }

        (void) pchtml_str_append(&doc_type->public_id, mraw, attr->value,
                                 attr->value_size);
    }
    else if (attr->name->attr_id == PCEDOM_ATTR_SYSTEM) {
        (void) pchtml_str_init(&doc_type->system_id, mraw, attr->value_size);
        if (doc_type->system_id.data == NULL) {
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        if (attr->value_begin == NULL) {
            return PCHTML_STATUS_OK;
        }

        (void) pchtml_str_append(&doc_type->system_id, mraw, attr->value,
                                 attr->value_size);

        return PCHTML_STATUS_OK;
    }
    else {
        goto set_pub_sys_empty;
    }

    /* SUSTEM */
    attr = attr->next;
    if (attr == NULL) {
        goto set_sys_empty;
    }

    (void) pchtml_str_init(&doc_type->system_id, mraw, attr->value_size);
    if (doc_type->system_id.data == NULL) {
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    (void) pchtml_str_append(&doc_type->system_id, mraw, attr->value,
                             attr->value_size);

    return PCHTML_STATUS_OK;

set_name_pub_sys_empty:

    doc_type->name = PCEDOM_ATTR__UNDEF;

set_pub_sys_empty:

    (void) pchtml_str_init(&doc_type->public_id, mraw, 0);
    if (doc_type->public_id.data == NULL) {
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

set_sys_empty:

    (void) pchtml_str_init(&doc_type->system_id, mraw, 0);
    if (doc_type->system_id.data == NULL) {
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    return PCHTML_HTML_STATUS_OK;
}

pchtml_html_token_attr_t *
pchtml_html_token_find_attr(pchtml_html_tokenizer_t *tkz, pchtml_html_token_t *token,
                         const unsigned char *name, size_t name_len)
{
    const pcedom_attr_data_t *data;
    pchtml_html_token_attr_t *attr = token->attr_first;

    data = pcedom_attr_data_by_local_name(tkz->attrs, name, name_len);
    if (data == NULL) {
        return NULL;
    }

    while (attr != NULL) {
        if (attr->name->attr_id == data->attr_id) {
            return attr;
        }

        attr = attr->next;
    }

    return NULL;
}

/*
 * No inline functions for ABI.
 */
void
pchtml_html_token_clean_noi(pchtml_html_token_t *token)
{
    pchtml_html_token_clean(token);
}

pchtml_html_token_t *
pchtml_html_token_create_eof_noi(pchtml_dobject_t *dobj)
{
    return pchtml_html_token_create_eof(dobj);
}
