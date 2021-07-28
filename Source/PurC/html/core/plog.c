/**
 * @file plog.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of log.
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

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "html/core/plog.h"


unsigned int
pchtml_plog_init(pchtml_plog_t *plog, size_t init_size, size_t struct_size)
{
    unsigned int status;

    if (plog == NULL) {
        pcinst_set_error (PCHTML_OBJECT_IS_NULL);
        return PCHTML_STATUS_ERROR_OBJECT_IS_NULL;
    }

    if (struct_size < sizeof(pchtml_plog_entry_t)) {
        struct_size = sizeof(pchtml_plog_entry_t);
    }

    status = pchtml_array_obj_init(&plog->list, init_size, struct_size);
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    return PCHTML_STATUS_OK;
}

pchtml_plog_t *
pchtml_plog_destroy(pchtml_plog_t *plog, bool self_destroy)
{
    if (plog == NULL) {
        return NULL;
    }

    pchtml_array_obj_destroy(&plog->list, false);

    if (self_destroy) {
        return pchtml_free(plog);
    }

    return plog;
}

/*
 * No inline functions.
 */
pchtml_plog_t *
pchtml_plog_create_noi(void)
{
    return pchtml_plog_create();
}

void
pchtml_plog_clean_noi(pchtml_plog_t *plog)
{
    pchtml_plog_clean(plog);
}

void *
pchtml_plog_push_noi(pchtml_plog_t *plog, const unsigned char *data, void *ctx,
                     unsigned id)
{
    return pchtml_plog_push(plog, data, ctx, id);
}

size_t
pchtml_plog_length_noi(pchtml_plog_t *plog)
{
    return pchtml_plog_length(plog);
}
