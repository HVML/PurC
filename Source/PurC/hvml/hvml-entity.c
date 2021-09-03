/*
 * @file hvml-entity.h
 * @author XueShuming
 * @date 2021/09/03
 * @brief The impl for hvml entity.
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

#include "hvml-entity.h"


#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

struct pchvml_entity {
    const char* entity;
    size_t nr_entity;
    wchar_t first_value;
    wchar_t second_value;
};

const char* pchvml_entity_get_entity(struct pchvml_entity* entity)
{
    return entity->entity;
}

size_t pchvml_entity_get_entity_size(struct pchvml_entity* entity)
{
    return entity->nr_entity;
}

wchar_t pchvml_entity_get_first_value(struct pchvml_entity* entity)
{
    return entity->first_value;
}

wchar_t pchvml_entity_get_last_value(struct pchvml_entity* entity)
{
    return entity->second_value;
}

