/**
 * @file ns.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for name space.
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


#ifndef PCHTML_NS_H
#define PCHTML_NS_H

#include "config.h"
#include "private/hash.h"
#include "html/shs.h"

#include "ns_const.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    pchtml_hash_entry_t  entry;

    pchtml_ns_id_t          ns_id;
    size_t               ref_count;
    bool                 read_only;
}
pchtml_ns_data_t;

typedef struct {
    pchtml_hash_entry_t  entry;

    pchtml_ns_prefix_id_t   prefix_id;
    size_t               ref_count;
    bool                 read_only;
}
pchtml_ns_prefix_data_t;


/* Link */
const unsigned char *
pchtml_ns_by_id(pchtml_hash_t *hash, pchtml_ns_id_t ns_id, 
                size_t *length) WTF_INTERNAL;

const pchtml_ns_data_t *
pchtml_ns_data_by_id(pchtml_hash_t *hash, pchtml_ns_id_t ns_id) WTF_INTERNAL;

const pchtml_ns_data_t *
pchtml_ns_data_by_link(pchtml_hash_t *hash, const unsigned char *name, 
                size_t length) WTF_INTERNAL;

/* Prefix */
const pchtml_ns_prefix_data_t *
pchtml_ns_prefix_append(pchtml_hash_t *hash,
                const unsigned char *prefix, size_t length) WTF_INTERNAL;

const pchtml_ns_prefix_data_t *
pchtml_ns_prefix_data_by_id(pchtml_hash_t *hash, 
                pchtml_ns_prefix_id_t prefix_id) WTF_INTERNAL;

const pchtml_ns_prefix_data_t *
pchtml_ns_prefix_data_by_name(pchtml_hash_t *hash,
                const unsigned char *name, size_t length) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_NS_H */
