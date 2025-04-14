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
#include "private/debug.h"
#include "private/errors.h"
#include "private/vcm.h"
#include "private/tkz-helper.h"
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
    enum pchvml_attr_operator assignment;
    struct tkz_buffer* name;
    struct tkz_buffer* value;
    struct pcvcm_node* vcm;
    uint32_t quote;
    bool vcm_reserved;
};

struct pchvml_token {
    enum pchvml_token_type type;
    bool self_closing;
    bool force_quirks;
    bool whitespace;
    bool has_raw_attr;

    struct tkz_buffer* name;
    struct pcutils_arrlist* attr_list;

    struct tkz_buffer* text_content;
    struct pcvcm_node* vcm_content;

    struct tkz_buffer* public_identifier;
    struct tkz_buffer* system_information;

    struct pchvml_token_attr* curr_attr;

    struct tkz_uc first_uc;
};

struct pchvml_token_attr* pchvml_token_attr_new()
{
    struct pchvml_token_attr* attr = (struct pchvml_token_attr*)
        PCHVML_ALLOC(sizeof(struct pchvml_token_attr));
    attr->vcm_reserved = false;
    return attr;
}

struct pchvml_token* pchvml_token_new_vcm(struct pcvcm_node* vcm) {
    struct pchvml_token* token =  pchvml_token_new(PCHVML_TOKEN_VCM_TREE);
    token->vcm_content = vcm;
    return token;
}

void pchvml_token_done(struct pchvml_token* token)
{
    UNUSED_PARAM(token);
}

void pchvml_token_attr_destroy(struct pchvml_token_attr* attr)
{
    if (!attr) {
        return;
    }
    if (attr->name) {
        tkz_buffer_destroy(attr->name);
    }
    if (attr->value) {
        tkz_buffer_destroy(attr->value);
    }
    if (attr->vcm && !attr->vcm_reserved) {
        pcvcm_node_destroy(attr->vcm);
    }
    PCHVML_FREE(attr);
}


void pchvml_token_attr_list_free_fn(void *data)
{
    pchvml_token_attr_destroy((struct pchvml_token_attr*)data);
}

struct pchvml_token* pchvml_token_new(enum pchvml_token_type type)
{
    struct pchvml_token* token = (struct pchvml_token*) PCHVML_ALLOC(
            sizeof(struct pchvml_token));
    token->type = type;
    return token;
}

void pchvml_token_destroy(struct pchvml_token* token)
{
    if (!token) {
        return;
    }

    if (token->name) {
        tkz_buffer_destroy(token->name);
    }

    if (token->attr_list) {
        pcutils_arrlist_free(token->attr_list);
    }

    if (token->text_content) {
        tkz_buffer_destroy(token->text_content);
    }

    if (token->vcm_content) {
        pcvcm_node_destroy(token->vcm_content);
    }

    if (token->public_identifier) {
        tkz_buffer_destroy(token->public_identifier);
    }

    if (token->system_information) {
        tkz_buffer_destroy(token->system_information);
    }

    pchvml_token_attr_destroy(token->curr_attr);
    PCHVML_FREE(token);
}

void pchvml_token_begin_attr(struct pchvml_token* token)
{
    pchvml_token_end_attr(token);
    token->curr_attr = pchvml_token_attr_new();
}

void pchvml_token_append_to_attr_name(struct pchvml_token* token,
        uint32_t uc)
{
    if (!token->curr_attr->name) {
        token->curr_attr->name = tkz_buffer_new();
    }
    tkz_buffer_append(token->curr_attr->name, uc);
}

void pchvml_token_append_bytes_to_attr_name(struct pchvml_token* token,
        const char* bytes, size_t sz_bytes)
{
    if (!token->curr_attr->name) {
        token->curr_attr->name = tkz_buffer_new();
    }
    tkz_buffer_append_bytes(token->curr_attr->name, bytes, sz_bytes);
}

void pchvml_token_append_to_attr_value(struct pchvml_token* token,
        uint32_t uc)
{
    if (!token->curr_attr->value) {
        token->curr_attr->value = tkz_buffer_new();
    }
    tkz_buffer_append(token->curr_attr->value, uc);
}

void pchvml_token_append_bytes_to_attr_value(struct pchvml_token* token,
        const char* bytes, size_t sz_bytes)
{
    if (!token->curr_attr->value) {
        token->curr_attr->value = tkz_buffer_new();
    }
    tkz_buffer_append_bytes(token->curr_attr->value, bytes, sz_bytes);
}

void pchvml_token_append_vcm_to_attr(struct pchvml_token* token,
        struct pcvcm_node* vcm)
{
    token->curr_attr->vcm = vcm;
}

void pchvml_token_set_assignment_to_attr(struct pchvml_token* token,
        enum pchvml_attr_operator assignment)
{
    if (token->curr_attr) {
        token->curr_attr->assignment = assignment;
    }
}

void pchvml_token_set_quote(struct pchvml_token* token, uint32_t quote)
{
    if (token->curr_attr) {
        token->curr_attr->quote = quote;
    }
}

#define RAW_STRING          "raw"
#define HVML_RAW_STRING     "hvml:raw"

void pchvml_token_end_attr(struct pchvml_token* token)
{
    if (!token->curr_attr) {
        return;
    }

    if (token->curr_attr->value) {
        token->curr_attr->vcm = pcvcm_node_new_string(
                tkz_buffer_get_bytes(token->curr_attr->value));
        if (token->curr_attr->quote == '\'') {
            token->curr_attr->vcm->quoted_type = PCVCM_NODE_QUOTED_TYPE_SINGLE;
        }
        else if (token->curr_attr->quote == '"') {
            token->curr_attr->vcm->quoted_type = PCVCM_NODE_QUOTED_TYPE_DOUBLE;
        }
    }

    if (!token->attr_list) {
        token->attr_list = pcutils_arrlist_new(
                pchvml_token_attr_list_free_fn);
    }
    const char* attr_name = tkz_buffer_get_bytes(token->curr_attr->name);
    if (strcmp(attr_name, RAW_STRING) == 0 ||
            strcmp(attr_name, HVML_RAW_STRING) == 0) {
        token->has_raw_attr = true;
    }
    pcutils_arrlist_append(token->attr_list, token->curr_attr);
    token->curr_attr = NULL;
}

struct tkz_uc* pchvml_token_get_first_uc(struct pchvml_token* token)
{
    return &token->first_uc;
}

void pchvml_token_set_first_uc(struct pchvml_token* token, struct tkz_uc *uc)
{
    token->first_uc = *uc;
}

void pchvml_token_append_to_name(struct pchvml_token* token, uint32_t uc)
{
    if (!token->name) {
        token->name = tkz_buffer_new();
    }
    tkz_buffer_append(token->name, uc);
}

void pchvml_token_append_buffer_to_name(struct pchvml_token* token,
        struct tkz_buffer* buffer)
{
    if (!token->name) {
        token->name = tkz_buffer_new();
    }
    tkz_buffer_append_another(token->name, buffer);
}

const char* pchvml_token_get_name(struct pchvml_token* token)
{
    return token->name ? tkz_buffer_get_bytes(token->name) : NULL;
}

const char* pchvml_token_get_text(struct pchvml_token* token)
{
    return token->text_content ?
        tkz_buffer_get_bytes(token->text_content) : NULL;
}

void pchvml_token_append_bytes_to_text(struct pchvml_token* token,
        const char* bytes, size_t sz_bytes)
{
    if (!token->text_content) {
        token->text_content = tkz_buffer_new();
    }
    tkz_buffer_append_bytes(token->text_content, bytes, sz_bytes);
}

void pchvml_token_append_to_public_identifier(struct pchvml_token* token,
        uint32_t uc)
{
    if (!token->public_identifier) {
        token->public_identifier = tkz_buffer_new();
    }
    tkz_buffer_append(token->public_identifier, uc);
}

const char* pchvml_token_get_public_identifier(struct pchvml_token* token)
{
    return token->public_identifier ?
        tkz_buffer_get_bytes(token->public_identifier) : NULL;
}

void pchvml_token_reset_public_identifier(struct pchvml_token* token)
{
    if (token->public_identifier) {
        tkz_buffer_reset(token->public_identifier);
    }
}

void pchvml_token_append_to_system_information(struct pchvml_token* token,
        uint32_t uc)
{
    if (!token->system_information) {
        token->system_information = tkz_buffer_new();
    }
    tkz_buffer_append(token->system_information, uc);
}

const char* pchvml_token_get_system_information(struct pchvml_token* token)
{
    return token->system_information ?
        tkz_buffer_get_bytes(token->system_information) : NULL;
}

void pchvml_token_reset_system_information(struct pchvml_token* token)
{
    if (token->system_information) {
        tkz_buffer_reset(token->system_information);
    }
}
bool pchvml_token_is_type(struct pchvml_token* token,
        enum pchvml_token_type type)
{
    return token && token->type == type;
}

enum pchvml_token_type pchvml_token_get_type(struct pchvml_token* token)
{
    return token->type;
}

const char* pchvml_token_type_name(enum pchvml_token_type type)
{
    switch (type) {
    case PCHVML_TOKEN_DOCTYPE:
        return "PCHVML_TOKEN_DOCTYPE";
    case PCHVML_TOKEN_START_TAG:
        return "PCHVML_TOKEN_START_TAG";
    case PCHVML_TOKEN_END_TAG:
        return "PCHVML_TOKEN_END_TAG";
    case PCHVML_TOKEN_COMMENT:
        return "PCHVML_TOKEN_COMMENT";
    case PCHVML_TOKEN_CHARACTER:
        return "PCHVML_TOKEN_CHARACTER";
    case PCHVML_TOKEN_VCM_TREE:
        return "PCHVML_TOKEN_VCM_TREE";
    case PCHVML_TOKEN_EOF:
        return "PCHVML_TOKEN_EOF";
    }
    return "INVALID TOKEN TYPE";
}

const char* pchvml_token_get_type_name(struct pchvml_token* token)
{
    return pchvml_token_type_name(token->type);
}

struct pcvcm_node* pchvml_token_get_vcm_content(struct pchvml_token* token)
{
    return token->vcm_content;
}

struct pcvcm_node* pchvml_token_detach_vcm_content(struct pchvml_token* token)
{
    struct pcvcm_node *v = token->vcm_content;
    token->vcm_content = NULL;
    return v;
}

void pchvml_token_set_self_closing(struct pchvml_token* token, bool b)
{
    token->self_closing = b;
}

bool pchvml_token_is_self_closing(struct pchvml_token* token)
{
    return token->self_closing;
}

void pchvml_token_set_force_quirks(struct pchvml_token* token, bool b)
{
    token->force_quirks = b;
}

bool pchvml_token_is_force_quirks(struct pchvml_token* token)
{
    return token->force_quirks;
}

void pchvml_token_set_is_whitespace(struct pchvml_token* token, bool b)
{
    token->whitespace = b;
}

bool pchvml_token_is_whitespace(struct pchvml_token* token)
{
    return token->whitespace;
}

struct pchvml_token_attr* pchvml_token_get_curr_attr(
        struct pchvml_token* token)
{
    return token->curr_attr;
}

bool pchvml_token_is_curr_attr_duplicate(
        struct pchvml_token* token)
{
    bool ret = false;
    struct pchvml_token_attr* curr = token->curr_attr;
    if (!curr) {
        goto out;
    }

    const char* name = pchvml_token_attr_get_name(curr);
    size_t nr_attrs = pchvml_token_get_attr_size(token);
    for (size_t i = 0; i < nr_attrs; ++i) {
        struct pchvml_token_attr* attr = pchvml_token_get_attr(token, i);
        const char *attr_name = pchvml_token_attr_get_name(attr);
        if (strcmp(name, attr_name) == 0) {
            ret = true;
            break;
        }
    }

out:
    return ret;
}

bool pchvml_token_is_in_attr(struct pchvml_token* token)
{
    return token->curr_attr != NULL;
}

size_t pchvml_token_get_attr_size(struct pchvml_token* token)
{
    return token->attr_list ? pcutils_arrlist_length(token->attr_list) : 0;
}

bool pchvml_token_has_raw_attr(struct pchvml_token* token)
{
    return token && token->has_raw_attr;
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
    return attr->name ? tkz_buffer_get_bytes(attr->name) : NULL;
}

struct pcvcm_node* pchvml_token_attr_get_value_ex(
        struct pchvml_token_attr* attr, bool res_vcm)
{
    attr->vcm_reserved = res_vcm;
    return attr->vcm;
}

enum pchvml_attr_operator pchvml_token_attr_get_operator(
        struct pchvml_token_attr* attr)
{
    return attr->assignment;
}

struct tkz_buffer* pchvml_token_attr_to_string(
        struct pchvml_token_attr* attr)
{
    if (!attr) {
        return NULL;
    }
    struct tkz_buffer* buffer = tkz_buffer_new();
    // name
    tkz_buffer_append_another(buffer, attr->name);

    if (!attr->vcm) {
        return buffer;
    }

    // assignment
    switch (attr->assignment) {
    case PCHVML_ATTRIBUTE_OPERATOR:
        tkz_buffer_append_bytes(buffer, "=", 1);
        break;

    case PCHVML_ATTRIBUTE_ADDITION_OPERATOR:
        tkz_buffer_append_bytes(buffer, "+=", 2);
        break;

    case PCHVML_ATTRIBUTE_SUBTRACTION_OPERATOR:
        tkz_buffer_append_bytes(buffer, "-=", 2);
        break;

    case PCHVML_ATTRIBUTE_ASTERISK_OPERATOR:
        tkz_buffer_append_bytes(buffer, "*=", 2);
        break;

    case PCHVML_ATTRIBUTE_REGEX_OPERATOR:
        tkz_buffer_append_bytes(buffer, "/=", 2);
        break;

    case PCHVML_ATTRIBUTE_PRECISE_OPERATOR:
        tkz_buffer_append_bytes(buffer, "%=", 2);
        break;

    case PCHVML_ATTRIBUTE_REPLACE_OPERATOR:
        tkz_buffer_append_bytes(buffer, "~=", 2);
        break;

    case PCHVML_ATTRIBUTE_HEAD_OPERATOR:
        tkz_buffer_append_bytes(buffer, "^=", 2);
        break;

    case PCHVML_ATTRIBUTE_TAIL_OPERATOR:
        tkz_buffer_append_bytes(buffer, "$=", 2);
        break;

    default:
        PC_ASSERT(0);
    }
    // value
    size_t nr_vcm_buffer = 0;
    char* vcm_buffer = pcvcm_node_to_string(attr->vcm, &nr_vcm_buffer);
    if (!vcm_buffer) {
        return buffer;
    }
#if 0
    switch (attr->quote) {
    case '"':
        tkz_buffer_append_bytes(buffer, "\"", 1);
        tkz_buffer_append_bytes(buffer, vcm_buffer, nr_vcm_buffer);
        tkz_buffer_append_bytes(buffer, "\"", 1);
        break;

    case '\'':
        tkz_buffer_append_bytes(buffer, "\'", 1);
        tkz_buffer_append_bytes(buffer, vcm_buffer, nr_vcm_buffer);
        tkz_buffer_append_bytes(buffer, "\'", 1);
        break;

    case 'U':
        tkz_buffer_append_bytes(buffer, vcm_buffer, nr_vcm_buffer);
        break;

    default:
        tkz_buffer_append_bytes(buffer, "\"", 1);
        tkz_buffer_append_bytes(buffer, vcm_buffer, nr_vcm_buffer);
        tkz_buffer_append_bytes(buffer, "\"", 1);
        break;
    }
#else
    tkz_buffer_append_bytes(buffer, vcm_buffer, nr_vcm_buffer);
#endif
    free(vcm_buffer);

    return buffer;
}

void pchvml_add_attr_list_to_buffer(struct tkz_buffer* buffer, 
        struct pcutils_arrlist* attrs)
{
    if (!attrs) {
        return;
    }
    struct pchvml_token_attr* attr = NULL;
    struct tkz_buffer* attr_buffer = NULL;
    size_t nr_attrs = pcutils_arrlist_length(attrs);
    for (size_t i = 0; i < nr_attrs; i++) {
        tkz_buffer_append_bytes(buffer, " ", 1);
        attr = (struct pchvml_token_attr*) pcutils_arrlist_get_idx(attrs, i);
        attr_buffer = pchvml_token_attr_to_string(attr);
        if (attr_buffer) {
            tkz_buffer_append_another(buffer, attr_buffer);
            tkz_buffer_destroy(attr_buffer);
        }
    }
}

struct tkz_buffer* pchvml_token_to_string(struct pchvml_token* token)
{
    if (!token) {
        return NULL;
    }
    struct tkz_buffer* buffer = NULL;

    switch (token->type) {
    case PCHVML_TOKEN_DOCTYPE:
        buffer = tkz_buffer_new();
        tkz_buffer_append_bytes(buffer, "<!DOCTYPE ", 10);
        tkz_buffer_append_another(buffer, token->name);
        if (token->public_identifier) {
            tkz_buffer_append_bytes(buffer, " PUBLIC \"", 9);
            tkz_buffer_append_another(buffer, token->public_identifier);
            tkz_buffer_append_bytes(buffer, "\"", 1);
        }

        if (token->system_information) {
            tkz_buffer_append_bytes(buffer, " SYSTEM \"", 9);
            tkz_buffer_append_another(buffer, token->system_information);
            tkz_buffer_append_bytes(buffer, "\"", 1);
        }

        tkz_buffer_append_bytes(buffer, ">", 1);
        break;

    case PCHVML_TOKEN_START_TAG:
        buffer = tkz_buffer_new();
        tkz_buffer_append_bytes(buffer, "<", 1);
        tkz_buffer_append_another(buffer, token->name);
        pchvml_add_attr_list_to_buffer(buffer, token->attr_list);
        if (token->self_closing) {
            tkz_buffer_append_bytes(buffer, "/", 1);
        }
        tkz_buffer_append_bytes(buffer, ">", 1);
        break;

    case PCHVML_TOKEN_END_TAG:
        buffer = tkz_buffer_new();
        tkz_buffer_append_bytes(buffer, "</", 2);
        tkz_buffer_append_another(buffer, token->name);
        pchvml_add_attr_list_to_buffer(buffer, token->attr_list);
        tkz_buffer_append_bytes(buffer, ">", 1);
        break;

    case PCHVML_TOKEN_COMMENT:
        buffer = tkz_buffer_new();
        tkz_buffer_append_bytes(buffer, "<!--", 4);
        if (token->text_content) {
            tkz_buffer_append_another(buffer, token->text_content);
        }
        tkz_buffer_append_bytes(buffer, "-->", 3);
        break;

    case PCHVML_TOKEN_CHARACTER:
        buffer = tkz_buffer_new();
        if (token->text_content) {
            tkz_buffer_append_another(buffer, token->text_content);
        }
        break;

    case PCHVML_TOKEN_VCM_TREE:
        {
            buffer = tkz_buffer_new();
            size_t nr_vcm_buffer = 0;
            char* vcm_buffer = pcvcm_node_to_string(token->vcm_content,
                    &nr_vcm_buffer);
            tkz_buffer_append_bytes(buffer, vcm_buffer, nr_vcm_buffer);
            free(vcm_buffer);
        }
        break;

    case PCHVML_TOKEN_EOF:
        return buffer;
    default:
        PC_ASSERT(0);
    }
    return buffer;
}

void
pchvml_util_dump_token(const struct pchvml_token *token,
        const char *file, int line, const char *func)
{
    PC_ASSERT(token);
    if (token->type == PCHVML_TOKEN_EOF) {
        PC_DEBUG("%s[%d]:%s(): EOF\n",
                pcutils_basename((char*)file), line, func);
        return;
    }

    struct tkz_buffer* token_buff;
    token_buff = pchvml_token_to_string((struct pchvml_token*)token);
    if (token_buff) {
        const char* type_name;
        type_name = pchvml_token_get_type_name((struct pchvml_token*)token);
        PC_DEBUG("%s[%d]:%s(): %s:%s\n",
                pcutils_basename((char*)file), line, func,
                type_name, tkz_buffer_get_bytes(token_buff));
        tkz_buffer_destroy(token_buff);
        return;
    }

    PC_DEBUG("%s[%d]:%s(): OUT_OF_MEMORY\n",
            pcutils_basename((char*)file), line, func);
}

