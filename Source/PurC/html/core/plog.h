/**
 * @file plog.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for log.
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


#ifndef PCHTML_PLOG_H
#define PCHTML_PLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/array_obj.h"


typedef struct {
    const unsigned char *data;
    void             *context;
    unsigned         id;
}
pchtml_plog_entry_t;

typedef struct {
    pcutils_array_obj_t list;
}
pchtml_plog_t;


unsigned int
pchtml_plog_init(pchtml_plog_t *plog, size_t init_size, 
                size_t struct_size) WTF_INTERNAL;

pchtml_plog_t *
pchtml_plog_destroy(pchtml_plog_t *plog, bool self_destroy) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline pchtml_plog_t *
pchtml_plog_create(void)
{
    return (pchtml_plog_t *) pchtml_calloc(1, sizeof(pchtml_plog_t));
}

static inline void
pchtml_plog_clean(pchtml_plog_t *plog)
{
    pcutils_array_obj_clean(&plog->list);
}

static inline void *
pchtml_plog_push(pchtml_plog_t *plog, const unsigned char *data, void *ctx,
                 unsigned id)
{
    pchtml_plog_entry_t *entry;

    if (plog == NULL) {
        return NULL;
    }

    entry = (pchtml_plog_entry_t *) pcutils_array_obj_push(&plog->list);
    if (entry == NULL) {
        return NULL;
    }

    entry->data = data;
    entry->context = ctx;
    entry->id = id;

    return (void *) entry;
}

static inline size_t
pchtml_plog_length(pchtml_plog_t *plog)
{
    return pcutils_array_obj_length(&plog->list);
}

/*
 * No inline functions for ABI.
 */
pchtml_plog_t *
pchtml_plog_create_noi(void);

void
pchtml_plog_clean_noi(pchtml_plog_t *plog);

void *
pchtml_plog_push_noi(pchtml_plog_t *plog, const unsigned char *data, void *ctx,
                     unsigned id);

size_t
pchtml_plog_length_noi(pchtml_plog_t *plog);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_PLOG_H */

