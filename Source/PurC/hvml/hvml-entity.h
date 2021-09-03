/*
 * @file hvml-entity.h
 * @author XueShuming
 * @date 2021/09/03
 * @brief The interfaces for hvml entity.
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

#ifndef PURC_HVML_ENTITY_H
#define PURC_HVML_ENTITY_H

#include <stddef.h>
#include <stdint.h>
#include <private/arraylist.h>
#include "tempbuffer.h"


struct pchvml_entity;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

const char* pchvml_entity_get_entity(struct pchvml_entity* entity);
size_t pchvml_entity_get_entity_size(struct pchvml_entity* entity);
wchar_t pchvml_entity_get_first_value(struct pchvml_entity* entity);
wchar_t pchvml_entity_get_last_value(struct pchvml_entity* entity);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_HVML_ENTITY_H */

