/**
 * @file container-ops.c
 * @author Xue Shuming
 * @date 2022/01/05
 * @brief The API for container ops.
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

#define _GNU_SOURCE       // qsort_r

#include "config.h"
#include "private/variant.h"
#include "private/errors.h"
#include "variant-internals.h"
#include "purc-errors.h"
#include "purc-utils.h"


#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

bool
purc_variant_container_displace(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(container);
    UNUSED_PARAM(value);
    UNUSED_PARAM(silent);
    return false;
}

bool
purc_variant_container_append(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(container);
    UNUSED_PARAM(value);
    UNUSED_PARAM(silent);
    return false;
}

bool
purc_variant_container_prepend(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(container);
    UNUSED_PARAM(value);
    UNUSED_PARAM(silent);
    return false;
}

bool
purc_variant_container_merge(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(container);
    UNUSED_PARAM(value);
    UNUSED_PARAM(silent);
    return false;
}

bool
purc_variant_container_insert_before(purc_variant_t container, int idx,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(container);
    UNUSED_PARAM(idx);
    UNUSED_PARAM(value);
    UNUSED_PARAM(silent);
    return false;
}

bool
purc_variant_container_insert_after(purc_variant_t container, int idx,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(container);
    UNUSED_PARAM(idx);
    UNUSED_PARAM(value);
    UNUSED_PARAM(silent);
    return false;
}

bool
purc_variant_container_unit(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(container);
    UNUSED_PARAM(value);
    UNUSED_PARAM(silent);
    return false;
}

bool
purc_variant_container_intersect(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(container);
    UNUSED_PARAM(value);
    UNUSED_PARAM(silent);
    return false;
}

bool
purc_variant_container_subtract(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(container);
    UNUSED_PARAM(value);
    UNUSED_PARAM(silent);
    return false;
}

bool
purc_variant_container_xor(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(container);
    UNUSED_PARAM(value);
    UNUSED_PARAM(silent);
    return false;
}

bool
purc_variant_container_overwrite(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(container);
    UNUSED_PARAM(value);
    UNUSED_PARAM(silent);
    return false;
}

