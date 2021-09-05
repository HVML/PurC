/*
 * @file rwswrap.h
 * @author XueShuming
 * @date 2021/09/05
 * @brief The interfaces for rwswrap.
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

#ifndef PURC_HVML_RWSWRAP_H
#define PURC_HVML_RWSWRAP_H

#include "config.h"
#include "purc-utils.h"
#include "purc-rwstream.h"
#include "private/list.h"

#include <stddef.h>
#include <stdint.h>

struct pchvml_rwswrap {
    purc_rwstream_t rws;
    struct list_head uc_list;
};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

struct pchvml_rwswrap* pchvml_rwswrap_new (void);

void pchvml_rwswrap_set_rwstream (struct pchvml_rwswrap* wrap,
        purc_rwstream_t rws);

wchar_t pchvml_rwswrap_next_char (struct pchvml_rwswrap* wrap);

int pchvml_rwswrap_next_utf8_char (struct pchvml_rwswrap* wrap, char* bytes,
        wchar_t* uc);

bool pchvml_rwswrap_buffer_chars (struct pchvml_rwswrap* wrap,
        wchar_t* ucs, size_t nr_ucs);

void pchvml_rwswrap_destroy (struct pchvml_rwswrap* wrap);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_HVML_RWSWRAP_H */

