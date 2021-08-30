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

#define HVML_END_OF_FILE       0

#if HAVE(GLIB)
#define    HVML_ALLOC(sz)   g_slice_alloc0(sz)
#define    HVML_FREE(p)     g_slice_free1(sizeof(*p), (gpointer)p)
#else
#define    HVML_ALLOC(sz)   calloc(1, sz)
#define    HVML_FREE(p)     free(p)
#endif


struct pchvml_token_attribute* pchvml_token_attribute_new ()
{
    struct pchvml_token_attribute* attr = (struct pchvml_token_attribute*)
        HVML_ALLOC(sizeof(struct pchvml_token_attribute));
    return attr;
}

void pchvml_token_attribute_destroy (struct pchvml_token_attribute* attr)
{
    if (!attr) {
        return;
    }
    free (attr->name);
    pcvcm_node_destroy (attr->value);
    HVML_FREE(attr);
}


void pchvml_token_attribute_list_free_fn(void *data)
{
    pchvml_token_attribute_destroy ((struct pchvml_token_attribute*)data);
}

struct pchvml_token* pchvml_token_new (enum hvml_token_type type)
{
    struct pchvml_token* token = (struct pchvml_token*) HVML_ALLOC(
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

void pchvml_token_begin_attribute (struct pchvml_token* token)
{
    token->curr_attr = pchvml_token_attribute_new();
}

void pchvml_token_append_to_attribute_name (struct pchvml_token* token,
        char* bytes)
{
    token->curr_attr->name = bytes;
}

void pchvml_token_append_to_attribute_value (struct pchvml_token* token,
        struct pcvcm_node* node)
{
    token->curr_attr->value = node;
}

void pchvml_token_end_attribute (struct pchvml_token* token)
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

struct pchvml_token* pchvml_token_new_character (const char* buf)
{
    struct pchvml_token* token = pchvml_token_new (HVML_TOKEN_CHARACTER);
    if (!token) {
        return NULL;
    }

    struct pcvcm_node* vcm = pcvcm_node_new_string (buf);
    pchvml_token_begin_attribute(token);
    pchvml_token_append_to_attribute_value (token, vcm);
    pchvml_token_end_attribute(token);
    return token;
}

struct pchvml_token* pchvml_token_new_start_tag ()
{
    return pchvml_token_new (HVML_TOKEN_START_TAG);
}

struct pchvml_token* pchvml_token_new_end_tag ()
{
    return pchvml_token_new (HVML_TOKEN_END_TAG);
}

void pchvml_token_append_character (struct pchvml_token* token,
        const char* bytes, size_t sz_bytes)
{
    if (!token->temp_buffer) {
        token->temp_buffer = pchvml_temp_buffer_new (32);
    }
    pchvml_temp_buffer_append_char(token->temp_buffer, bytes, sz_bytes);
}
