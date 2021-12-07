/*
 * @file var-mgr.h
 * @author XueShuming
 * @date 2021/12/06
 * @brief The api for PurC variable manager.
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

#ifndef PURC_PRIVATE_VAR_MGR_H
#define PURC_PRIVATE_VAR_MGR_H

#include "purc.h"

#include "config.h"

#include "private/variant.h"
#include "private/map.h"

typedef struct pcutils_map*  pcvarmgr_list_t;

pcvarmgr_list_t pcvarmgr_list_create(void);

void pcvarmgr_list_destroy(pcvarmgr_list_t list);

bool pcvarmgr_list_add(pcvarmgr_list_t list, const char* name,
        purc_variant_t variant);

purc_variant_t pcvarmgr_list_get(pcvarmgr_list_t list, const char* name);

bool pcvarmgr_list_remove(pcvarmgr_list_t list, const char* name);

#endif /* not defined PURC_PRIVATE_VAR_MGR_H */

