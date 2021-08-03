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
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under Apahce 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#ifndef PCHTML_TAG_TAG_H
#define PCHTML_TAG_TAG_H

#include "config.h"
#include "html/hash.h"
#include "html/shs.h"
#include "html/dobject.h"
#include "html/str.h"

#include "tag_const.h"


#ifdef __cplusplus
extern "C" {
#endif

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


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_TAG_TAG_H */
