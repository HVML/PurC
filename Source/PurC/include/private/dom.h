/**
 * @file dom.h
 * @author
 * @date 2021/07/02
 * @brief The internal interfaces for dom.
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

#ifndef PURC_PRIVATE_DOM_H
#define PURC_PRIVATE_DOM_H

#include "config.h"

#include "private/str.h"
#include "private/hash.h"

#include "purc-dom.h"
#include "purc-html.h"
#include "purc-errors.h"

#include "ns_const.h"
#include "html_tag_const.h"
#include "html_attr_const.h"

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define PURC_ERROR_DOM PURC_ERROR_FIRST_DOM

// ============================= for ns ================================
typedef struct {
    pcutils_hash_entry_t    entry;

    pchtml_ns_id_t          ns_id;
    size_t                  ref_count;
    bool                    read_only;
} pchtml_ns_data_t;

typedef struct {
    pcutils_hash_entry_t    entry;

    pchtml_ns_prefix_id_t   prefix_id;
    size_t                  ref_count;
    bool                    read_only;
} pchtml_ns_prefix_data_t;


/* Link */
const unsigned char *
pchtml_ns_by_id(pcutils_hash_t *hash, pchtml_ns_id_t ns_id,
                size_t *length) WTF_INTERNAL;

const pchtml_ns_data_t *
pchtml_ns_data_by_id(pcutils_hash_t *hash, pchtml_ns_id_t ns_id) WTF_INTERNAL;

const pchtml_ns_data_t *
pchtml_ns_data_by_link(pcutils_hash_t *hash, const unsigned char *name, 
                size_t length) WTF_INTERNAL;

/* Prefix */
const pchtml_ns_prefix_data_t *
pchtml_ns_prefix_append(pcutils_hash_t *hash,
                const unsigned char *prefix, size_t length) WTF_INTERNAL;

const pchtml_ns_prefix_data_t *
pchtml_ns_prefix_data_by_id(pcutils_hash_t *hash, 
                pchtml_ns_prefix_id_t prefix_id) WTF_INTERNAL;

const pchtml_ns_prefix_data_t *
pchtml_ns_prefix_data_by_name(pcutils_hash_t *hash,
                const unsigned char *name, size_t length) WTF_INTERNAL;


// ============================= for tag ================================
typedef struct {
    pcutils_hash_entry_t    entry;
    pchtml_tag_id_t         tag_id;
    size_t                  ref_count;
    bool                    read_only;
} pchtml_tag_data_t;

const pchtml_tag_data_t *
pchtml_tag_data_by_id(pcutils_hash_t *hash, pchtml_tag_id_t tag_id) WTF_INTERNAL;

const pchtml_tag_data_t *
pchtml_tag_data_by_name(pcutils_hash_t *hash, const unsigned char *name, 
                size_t len) WTF_INTERNAL;

const pchtml_tag_data_t *
pchtml_tag_data_by_name_upper(pcutils_hash_t *hash,
                const unsigned char *name, size_t len) WTF_INTERNAL;

/*
 * Inline functions
 */
static inline const unsigned char *
pchtml_tag_name_by_id(pcutils_hash_t *hash, pchtml_tag_id_t tag_id, size_t *len)
{
    const pchtml_tag_data_t *data = pchtml_tag_data_by_id(hash, tag_id);
    if (data == NULL) {
        if (len != NULL) {
            *len = 0;
        }

        return NULL;
    }

    if (len != NULL) {
        *len = data->entry.length;
    }

    return pcutils_hash_entry_str(&data->entry);
}

static inline const unsigned char *
pchtml_tag_name_upper_by_id(pcutils_hash_t *hash, pchtml_tag_id_t tag_id, size_t *len)
{
    const pchtml_tag_data_t *data = pchtml_tag_data_by_id(hash, tag_id);
    if (data == NULL) {
        if (len != NULL) {
            *len = 0;
        }

        return NULL;
    }

    if (len != NULL) {
        *len = data->entry.length;
    }

    return pcutils_hash_entry_str(&data->entry);
}

static inline pchtml_tag_id_t
pchtml_tag_id_by_name(pcutils_hash_t *hash, const unsigned char *name, size_t len)
{
    const pchtml_tag_data_t *data = pchtml_tag_data_by_name(hash, name, len);
    if (data == NULL) {
        return PCHTML_TAG__UNDEF;
    }

    return data->tag_id;
}

static inline pcutils_mraw_t *
pchtml_tag_mraw(pcutils_hash_t *hash)
{
    return pcutils_hash_mraw(hash);
}


/* VW NOTE: eDOM module should work without instance
struct pcinst;
// initialize the dom module for a PurC instance.
void pcdom_init_instance(struct pcinst* inst) WTF_INTERNAL;
// clean up the dom module for a PurC instance.
void pcdom_cleanup_instance(struct pcinst* inst) WTF_INTERNAL;
*/

// ============================= for id_elem hash =============================
typedef struct {
    pcutils_hash_entry_t    entry;

    pcdom_element_t        *elem;
} pchtml_id_elem_data_t;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_DOM_H */

