/*
 * @file hvml-token.c
 * @author XueShuming
 * @date 2021/08/29
 * @brief The impl of hvml token.
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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "purc-utils.h"
#include "purc-errors.h"
#include "private/errors.h"
#include "private/vcm.h"
#include "hvml-token.h"

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

#define PCHVML_END_OF_FILE       0

#if HAVE(GLIB)
#define    PCHVML_ALLOC(sz)   g_slice_alloc0(sz)
#define    PCHVML_FREE(p)     g_slice_free1(sizeof(*p), (gpointer)p)
#else
#define    PCHVML_ALLOC(sz)   calloc(1, sz)
#define    PCHVML_FREE(p)     free(p)
#endif

struct pchvml_token_attr {
    enum pchvml_attr_assignment assignment;
    struct pchvml_buffer* name;
    struct pchvml_buffer* value;
    struct pcvcm_node* vcm;
    uint32_t quote;
    bool vcm_reserved;
};

struct pchvml_token {
    enum pchvml_token_type type;
    bool self_closing;
    bool force_quirks;

    struct pchvml_buffer* name;
    struct pcutils_arrlist* attr_list;

    struct pchvml_buffer* text_content;
    struct pcvcm_node* vcm_content;

    struct pchvml_buffer* public_identifier;
    struct pchvml_buffer* system_information;

    struct pchvml_token_attr* curr_attr;
};

struct pchvml_token_attr* pchvml_token_attr_new ()
{
    struct pchvml_token_attr* attr = (struct pchvml_token_attr*)
        PCHVML_ALLOC(sizeof(struct pchvml_token_attr));
    attr->vcm_reserved = false;
    return attr;
}

struct pchvml_token* pchvml_token_new_vcm (struct pcvcm_node* vcm) {
    struct pchvml_token* token =  pchvml_token_new(PCHVML_TOKEN_VCM_TREE);
    token->vcm_content = vcm;
    return token;
}

void pchvml_token_done (struct pchvml_token* token)
{
    UNUSED_PARAM(token);
}

void pchvml_token_attr_destroy (struct pchvml_token_attr* attr)
{
    if (!attr) {
        return;
    }
    if (attr->name) {
        pchvml_buffer_destroy(attr->name);
    }
    if (attr->value && !attr->vcm_reserved) {
        pchvml_buffer_destroy(attr->value);
    }
    if (attr->vcm) {
        pcvcm_node_destroy (attr->vcm);
    }
    PCHVML_FREE(attr);
}


void pchvml_token_attr_list_free_fn(void *data)
{
    pchvml_token_attr_destroy ((struct pchvml_token_attr*)data);
}

struct pchvml_token* pchvml_token_new (enum pchvml_token_type type)
{
    struct pchvml_token* token = (struct pchvml_token*) PCHVML_ALLOC(
            sizeof(struct pchvml_token));
    token->type = type;
    return token;
}

void pchvml_token_destroy (struct pchvml_token* token)
{
    if (!token) {
        return;
    }

    if (token->name) {
        pchvml_buffer_destroy(token->name);
    }

    if (token->attr_list) {
        pcutils_arrlist_free (token->attr_list);
    }

    if (token->text_content) {
        pchvml_buffer_destroy(token->text_content);
    }

    if (token->vcm_content) {
        pcvcm_node_destroy(token->vcm_content);
    }

    if (token->public_identifier) {
        pchvml_buffer_destroy(token->public_identifier);
    }

    if (token->system_information) {
        pchvml_buffer_destroy(token->system_information);
    }

    pchvml_token_attr_destroy(token->curr_attr);
    PCHVML_FREE(token);
}

void pchvml_token_begin_attr (struct pchvml_token* token)
{
    token->curr_attr = pchvml_token_attr_new();
}

void pchvml_token_append_to_attr_name (struct pchvml_token* token,
        uint32_t uc)
{
    if (!token->curr_attr->name) {
        token->curr_attr->name = pchvml_buffer_new ();
    }
    pchvml_buffer_append(token->curr_attr->name, uc);
}

void pchvml_token_append_bytes_to_attr_name (struct pchvml_token* token,
        const char* bytes, size_t sz_bytes)
{
    if (!token->curr_attr->name) {
        token->curr_attr->name = pchvml_buffer_new ();
    }
    pchvml_buffer_append_bytes(token->curr_attr->name, bytes, sz_bytes);
}

void pchvml_token_append_to_attr_value (struct pchvml_token* token,
        uint32_t uc)
{
    if (!token->curr_attr->value) {
        token->curr_attr->value = pchvml_buffer_new ();
    }
    pchvml_buffer_append(token->curr_attr->value, uc);
}

void pchvml_token_append_bytes_to_attr_value (struct pchvml_token* token,
        const char* bytes, size_t sz_bytes)
{
    if (!token->curr_attr->value) {
        token->curr_attr->value = pchvml_buffer_new ();
    }
    pchvml_buffer_append_bytes(token->curr_attr->value, bytes, sz_bytes);
}

void pchvml_token_append_vcm_to_attr (struct pchvml_token* token,
        struct pcvcm_node* vcm)
{
    token->curr_attr->vcm = vcm;
}

void pchvml_token_set_assignment_to_attr (struct pchvml_token* token,
        enum pchvml_attr_assignment assignment)
{
    if (token->curr_attr) {
        token->curr_attr->assignment = assignment;
    }
}

void pchvml_token_end_attr (struct pchvml_token* token)
{
    if (!token->curr_attr) {
        return;
    }

    if (token->curr_attr->value) {
        token->curr_attr->vcm = pcvcm_node_new_string(
                pchvml_buffer_get_buffer(token->curr_attr->value));
    }

    if (!token->attr_list) {
        token->attr_list = pcutils_arrlist_new(
                pchvml_token_attr_list_free_fn);
    }
    pcutils_arrlist_add(token->attr_list, token->curr_attr);
    token->curr_attr = NULL;
}

void pchvml_token_append_to_name (struct pchvml_token* token, uint32_t uc)
{
    if (!token->name) {
        token->name = pchvml_buffer_new();
    }
    pchvml_buffer_append(token->name, uc);
}

const char* pchvml_token_get_name (struct pchvml_token* token)
{
    return token->name ? pchvml_buffer_get_buffer (token->name) : NULL;
}

void pchvml_token_append_to_text (struct pchvml_token* token,
        uint32_t uc)
{
    if (!token->text_content) {
        token->text_content = pchvml_buffer_new();
    }
    pchvml_buffer_append(token->text_content, uc);
}

const char* pchvml_token_get_text (struct pchvml_token* token)
{
    return token->text_content ?
        pchvml_buffer_get_buffer (token->text_content) : NULL;
}

void pchvml_token_append_bytes_to_text (struct pchvml_token* token,
        const char* bytes, size_t sz_bytes)
{
    if (!token->text_content) {
        token->text_content = pchvml_buffer_new();
    }
    pchvml_buffer_append_bytes(token->text_content, bytes, sz_bytes);
}

void pchvml_token_append_to_public_identifier (struct pchvml_token* token,
        uint32_t uc)
{
    if (!token->public_identifier) {
        token->public_identifier = pchvml_buffer_new ();
    }
    pchvml_buffer_append(token->public_identifier, uc);
}

const char* pchvml_token_get_public_identifier (struct pchvml_token* token)
{
    return token->public_identifier ?
        pchvml_buffer_get_buffer (token->public_identifier) : NULL;
}

void pchvml_token_reset_public_identifier (struct pchvml_token* token)
{
    if (token->public_identifier) {
        pchvml_buffer_reset (token->public_identifier);
    }
}

void pchvml_token_append_to_system_information (struct pchvml_token* token,
        uint32_t uc)
{
    if (!token->system_information) {
        token->system_information = pchvml_buffer_new ();
    }
    pchvml_buffer_append(token->system_information, uc);
}

const char* pchvml_token_get_system_information (struct pchvml_token* token)
{
    return token->system_information ?
        pchvml_buffer_get_buffer (token->system_information) : NULL;
}

void pchvml_token_reset_system_information (struct pchvml_token* token)
{
    if (token->system_information) {
        pchvml_buffer_reset (token->system_information);
    }
}
bool pchvml_token_is_type (struct pchvml_token* token,
        enum pchvml_token_type type)
{
    return token && token->type == type;
}

enum pchvml_token_type pchvml_token_get_type(struct pchvml_token* token)
{
    return token->type;
}

struct pcvcm_node* pchvml_token_get_vcm(struct pchvml_token* token)
{
    return token->vcm_content;
}

void pchvml_token_set_self_closing (struct pchvml_token* token, bool b)
{
    token->self_closing = b;
}

bool pchvml_token_is_self_closing (struct pchvml_token* token)
{
    return token->self_closing;
}

void pchvml_token_set_force_quirks (struct pchvml_token* token, bool b)
{
    token->force_quirks = b;
}

bool pchvml_token_is_force_quirks (struct pchvml_token* token)
{
    return token->force_quirks;
}

struct pchvml_token_attr* pchvml_token_get_curr_attr (
        struct pchvml_token* token)
{
    return token->curr_attr;
}

bool pchvml_token_is_in_attr (struct pchvml_token* token)
{
    return token->curr_attr != NULL;
}

size_t pchvml_token_get_attr_size(struct pchvml_token* token)
{
    return token->attr_list ? pcutils_arrlist_length(token->attr_list) : 0;
}

struct pchvml_token_attr* pchvml_token_get_attr(
        struct pchvml_token* token, size_t i)
{
    if (token->attr_list) {
        return (struct pchvml_token_attr*) pcutils_arrlist_get_idx(
                token->attr_list, i);
    }
    return NULL;
}

const char* pchvml_token_attr_get_name(struct pchvml_token_attr* attr)
{
    return pchvml_buffer_get_buffer(attr->name);
}

struct pcvcm_node* pchvml_token_attr_get_value_ex(
        struct pchvml_token_attr* attr, bool res_vcm)
{
    attr->vcm_reserved = res_vcm;
    return attr->vcm;
}

enum pchvml_attr_assignment pchvml_token_attr_get_assignment(
        struct pchvml_token_attr* attr)
{
    return attr->assignment;
}

struct pchvml_buffer* pchvml_token_attr_to_string(
        struct pchvml_token_attr* attr)
{
    if (!attr) {
        return NULL;
    }
    struct pchvml_buffer* buffer = pchvml_buffer_new();
    // name
    pchvml_buffer_append_temp_buffer (buffer, attr->name);

    if (!attr->vcm) {
        return buffer;
    }

    // assignment
    switch (attr->assignment) {
    case PCHVML_ATTRIBUTE_ASSIGNMENT:
        pchvml_buffer_append_bytes(buffer, "=", 1);
        break;

    case PCHVML_ATTRIBUTE_ADDITION_ASSIGNMENT:
        pchvml_buffer_append_bytes(buffer, "+=", 2);
        break;

    case PCHVML_ATTRIBUTE_SUBTRACTION_ASSIGNMENT:
        pchvml_buffer_append_bytes(buffer, "-=", 2);
        break;

    case PCHVML_ATTRIBUTE_REMAINDER_ASSIGNMENT:
        pchvml_buffer_append_bytes(buffer, "%=", 2);
        break;

    case PCHVML_ATTRIBUTE_REPLACE_ASSIGNMENT:
        pchvml_buffer_append_bytes(buffer, "~=", 2);
        break;

    case PCHVML_ATTRIBUTE_HEAD_ASSIGNMENT:
        pchvml_buffer_append_bytes(buffer, "^=", 2);
        break;

    case PCHVML_ATTRIBUTE_TAIL_ASSIGNMENT:
        pchvml_buffer_append_bytes(buffer, "$=", 2);
        break;
    }
    // value
    size_t nr_vcm_buffer = 0;
    char* vcm_buffer = pcvcm_node_to_string(attr->vcm, &nr_vcm_buffer);
    if (!vcm_buffer) {
        return buffer;
    }
    switch (attr->quote) {
    case '"':
        pchvml_buffer_append_bytes(buffer, "\"", 1);
        pchvml_buffer_append_bytes(buffer, vcm_buffer, nr_vcm_buffer);
        pchvml_buffer_append_bytes(buffer, "\"", 1);
        break;

    case '\'':
        pchvml_buffer_append_bytes(buffer, "\'", 1);
        pchvml_buffer_append_bytes(buffer, vcm_buffer, nr_vcm_buffer);
        pchvml_buffer_append_bytes(buffer, "\'", 1);
        break;

    case 'U':
        pchvml_buffer_append_bytes(buffer, vcm_buffer, nr_vcm_buffer);
        break;

    default:
        pchvml_buffer_append_bytes(buffer, "\"", 1);
        pchvml_buffer_append_bytes(buffer, vcm_buffer, nr_vcm_buffer);
        pchvml_buffer_append_bytes(buffer, "\"", 1);
        break;
    }
    free(vcm_buffer);

    return buffer;
}

void pchvml_add_attr_list_to_buffer(struct pchvml_buffer* buffer, 
        struct pcutils_arrlist* attrs)
{
    if (!attrs) {
        return;
    }
    struct pchvml_token_attr* attr = NULL;
    struct pchvml_buffer* attr_buffer = NULL;
    size_t nr_attrs = pcutils_arrlist_length(attrs);
    for (size_t i = 0; i < nr_attrs; i++) {
        pchvml_buffer_append_bytes(buffer, " ", 1);
        attr = (struct pchvml_token_attr*) pcutils_arrlist_get_idx(attrs, i);
        attr_buffer = pchvml_token_attr_to_string(attr);
        if (attr_buffer) {
            pchvml_buffer_append_temp_buffer (buffer, attr_buffer);
            pchvml_buffer_destroy(attr_buffer);
        }
    }
}

struct pchvml_buffer* pchvml_token_to_string(struct pchvml_token* token)
{
    if (!token) {
        return NULL;
    }
    struct pchvml_buffer* buffer = NULL;

    switch (token->type) {
    case PCHVML_TOKEN_DOCTYPE:
        break;

    case PCHVML_TOKEN_START_TAG:
        buffer = pchvml_buffer_new();
        pchvml_buffer_append_bytes(buffer, "<", 1);
        pchvml_buffer_append_temp_buffer (buffer, token->name);
        pchvml_add_attr_list_to_buffer(buffer, token->attr_list);
        if (token->self_closing) {
            pchvml_buffer_append_bytes(buffer, "/", 1);
        }
        pchvml_buffer_append_bytes(buffer, ">", 1);
        break;

    case PCHVML_TOKEN_END_TAG:
        buffer = pchvml_buffer_new();
        pchvml_buffer_append_bytes(buffer, "</", 2);
        pchvml_buffer_append_temp_buffer (buffer, token->name);
        pchvml_add_attr_list_to_buffer(buffer, token->attr_list);
        pchvml_buffer_append_bytes(buffer, ">", 1);
        break;

    case PCHVML_TOKEN_COMMENT:
        break;

    case PCHVML_TOKEN_CHARACTER:
        buffer = pchvml_buffer_new();
        if (token->text_content) {
            pchvml_buffer_append_temp_buffer (buffer, token->text_content);
        }
        break;

    case PCHVML_TOKEN_VCM_TREE:
        break;

    case PCHVML_TOKEN_EOF:
        return buffer;
    }
    return buffer;
}
