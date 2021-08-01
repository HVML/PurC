/**
 * @file tag.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html tag.
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
 * This implementation of HTML parser is derived from Lexbor <http://lexbor.com/>.
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#ifndef PCHTML_TAG_TAG_H
#define PCHTML_TAG_TAG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/hash.h"
#include "html/core/shs.h"
#include "html/core/dobject.h"
#include "html/core/str.h"

#include "html/tag_tag_const.h"


typedef struct {
    pchtml_hash_entry_t entry;
    pchtml_tag_id_t        tag_id;
    size_t              ref_count;
    bool                read_only;
}
pchtml_tag_data_t;


const pchtml_tag_data_t *
pchtml_tag_data_by_id(pchtml_hash_t *hash, pchtml_tag_id_t tag_id) WTF_INTERNAL;

const pchtml_tag_data_t *
pchtml_tag_data_by_name(pchtml_hash_t *hash, const unsigned char *name, 
                size_t len) WTF_INTERNAL;

const pchtml_tag_data_t *
pchtml_tag_data_by_name_upper(pchtml_hash_t *hash,
                const unsigned char *name, size_t len) WTF_INTERNAL;

/*
 * Inline functions
 */
static inline const unsigned char *
pchtml_tag_name_by_id(pchtml_hash_t *hash, pchtml_tag_id_t tag_id, size_t *len)
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

    return pchtml_hash_entry_str(&data->entry);
}

static inline const unsigned char *
pchtml_tag_name_upper_by_id(pchtml_hash_t *hash, pchtml_tag_id_t tag_id, size_t *len)
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

    return pchtml_hash_entry_str(&data->entry);
}

static inline pchtml_tag_id_t
pchtml_tag_id_by_name(pchtml_hash_t *hash, const unsigned char *name, size_t len)
{
    const pchtml_tag_data_t *data = pchtml_tag_data_by_name(hash, name, len);
    if (data == NULL) {
        return PCHTML_TAG__UNDEF;
    }

    return data->tag_id;
}

static inline pchtml_mraw_t *
pchtml_tag_mraw(pchtml_hash_t *hash)
{
    return pchtml_hash_mraw(hash);
}


/*
 * No inline functions for ABI.
 */
const unsigned char *
pchtml_tag_name_by_id_noi(pchtml_hash_t *hash, pchtml_tag_id_t tag_id,
                size_t *len) WTF_INTERNAL;

const unsigned char *
pchtml_tag_name_upper_by_id_noi(pchtml_hash_t *hash,
                pchtml_tag_id_t tag_id, size_t *len) WTF_INTERNAL;

pchtml_tag_id_t
pchtml_tag_id_by_name_noi(pchtml_hash_t *hash,
                const unsigned char *name, size_t len) WTF_INTERNAL;

pchtml_mraw_t *
pchtml_tag_mraw_noi(pchtml_hash_t *hash) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_TAG_TAG_H */
