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


struct pchvml_token_attribute* pchvml_token_attribute_new ()
{
    struct pchvml_token_attribute* attr = (struct pchvml_token_attribute*)
        PCHVML_ALLOC(sizeof(struct pchvml_token_attribute));
    return attr;
}

void pchvml_token_done (struct pchvml_token* token)
{
    UNUSED_PARAM(token);
}

void pchvml_token_attribute_destroy (struct pchvml_token_attribute* attr)
{
    if (!attr) {
        return;
    }
    pchvml_temp_buffer_destroy(attr->name);
    pchvml_temp_buffer_destroy(attr->value);
    pcvcm_node_destroy (attr->vcm);
    PCHVML_FREE(attr);
}


void pchvml_token_attribute_list_free_fn(void *data)
{
    pchvml_token_attribute_destroy ((struct pchvml_token_attribute*)data);
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

    if (token->attr_list) {
        pcutils_arrlist_free (token->attr_list);
    }
    pchvml_token_attribute_destroy(token->curr_attr);
}

void pchvml_token_begin_attr (struct pchvml_token* token)
{
    token->curr_attr = pchvml_token_attribute_new();
}

void pchvml_token_append_to_attr_name (struct pchvml_token* token,
        const char* bytes, size_t sz_bytes)
{
    if (!token->curr_attr->name) {
        token->curr_attr->name = pchvml_temp_buffer_new ();
    }
    pchvml_temp_buffer_append_bytes(token->curr_attr->name, bytes, sz_bytes);
}

void pchvml_token_append_to_attr_value (struct pchvml_token* token,
        const char* bytes, size_t sz_bytes)
{
    if (!token->curr_attr->value) {
        token->curr_attr->value = pchvml_temp_buffer_new ();
    }
    pchvml_temp_buffer_append_bytes(token->curr_attr->value, bytes, sz_bytes);
}

void pchvml_token_append_vcm_to_attr (struct pchvml_token* token,
        struct pcvcm_node* vcm)
{
    token->curr_attr->vcm = vcm;
}

void pchvml_token_set_assignment_to_attr (struct pchvml_token* token,
        enum pchvml_attribute_assignment assignment)
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

    if (!token->attr_list) {
        token->attr_list = pcutils_arrlist_new(
                pchvml_token_attribute_list_free_fn);
    }
    pcutils_arrlist_add(token->attr_list, token->curr_attr);
    token->curr_attr = NULL;
}

void pchvml_token_append_to_name (struct pchvml_token* token, uint32_t uc)
{
    if (!token->name) {
        token->name = pchvml_temp_buffer_new();
    }
    pchvml_temp_buffer_append(token->name, uc);
}

const char* pchvml_token_get_name (struct pchvml_token* token)
{
    return token->name ? pchvml_temp_buffer_get_buffer (token->name) : NULL;
}

void pchvml_token_append_to_text (struct pchvml_token* token,
        uint32_t uc)
{
    if (!token->text_content) {
        token->text_content = pchvml_temp_buffer_new();
    }
    pchvml_temp_buffer_append(token->text_content, uc);
}

const char* pchvml_token_get_text (struct pchvml_token* token)
{
    return token->text_content ?
        pchvml_temp_buffer_get_buffer (token->text_content) : NULL;
}

void pchvml_token_append_bytes_to_text (struct pchvml_token* token,
        const char* bytes, size_t sz_bytes)
{
    if (!token->text_content) {
        token->text_content = pchvml_temp_buffer_new();
    }
    pchvml_temp_buffer_append_bytes(token->text_content, bytes, sz_bytes);
}

void pchvml_token_append_to_public_identifier (struct pchvml_token* token,
        uint32_t uc)
{
    if (!token->public_identifier) {
        token->public_identifier = pchvml_temp_buffer_new ();
    }
    pchvml_temp_buffer_append(token->public_identifier, uc);
}

const char* pchvml_token_get_public_identifier (struct pchvml_token* token)
{
    return token->public_identifier ?
        pchvml_temp_buffer_get_buffer (token->public_identifier) : NULL;
}

void pchvml_token_reset_public_identifier (struct pchvml_token* token)
{
    if (token->public_identifier) {
        pchvml_temp_buffer_reset (token->public_identifier);
    }
}

void pchvml_token_append_to_system_information (struct pchvml_token* token,
        uint32_t uc)
{
    if (!token->system_information) {
        token->system_information = pchvml_temp_buffer_new ();
    }
    pchvml_temp_buffer_append(token->system_information, uc);
}

const char* pchvml_token_get_system_information (struct pchvml_token* token)
{
    return token->system_information ?
        pchvml_temp_buffer_get_buffer (token->system_information) : NULL;
}

void pchvml_token_reset_system_information (struct pchvml_token* token)
{
    if (token->system_information) {
        pchvml_temp_buffer_reset (token->system_information);
    }
}

