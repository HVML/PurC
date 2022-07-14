/*
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <purc.h>

#include <stdio.h>

extern purc_variant_t
ChooseTimer(purc_variant_t on_value, purc_variant_t with_value)
{
    if (!purc_variant_is_set(on_value)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_string(with_value)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    return purc_variant_set_get_member_by_key_values(on_value, with_value);
}

