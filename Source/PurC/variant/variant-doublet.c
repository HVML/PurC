/**
 * @file variant-doublet.c
 * @author Vincent Wei
 * @date 2022/06/06
 * @brief The implement of doublet variant.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

#include "config.h"
#include "private/variant.h"
#include "private/errors.h"
#include "variant-internals.h"
#include "purc-errors.h"
#include "purc-utils.h"

purc_variant_t purc_variant_make_doublet(purc_variant_t first,
        purc_variant_t second)
{
    PCVARIANT_CHECK_FAIL_RET(first && second, PURC_VARIANT_INVALID);

    purc_variant_t var = pcvariant_get(PVT(_DOUBLET));
    if (!var) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    var->doublet[0] = purc_variant_ref(first);
    var->doublet[1] = purc_variant_ref(second);

    return var;
}

purc_variant_t purc_variant_doublet_get(purc_variant_t doublet, int idx)
{
    PCVARIANT_CHECK_FAIL_RET(doublet && doublet->type==PVT(_DOUBLET) &&
            idx >= 0 && idx < 2, PURC_VARIANT_INVALID);

    if (idx == 0)
        return doublet->doublet[0];

    return doublet->doublet[1];
}

bool purc_variant_doublet_set(purc_variant_t doublet,
        int idx, purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(doublet && doublet->type==PVT(_DOUBLET) &&
            idx >= 0 && idx < 2, false);

    if (idx == 0) {
        purc_variant_unref(doublet->doublet[0]);
        doublet->doublet[0] = purc_variant_ref(value);
    }
    else {
        purc_variant_unref(doublet->doublet[1]);
        doublet->doublet[1] = purc_variant_ref(value);
    }

    return true;
}

purc_variant_t
pcvariant_doublet_clone(purc_variant_t doublet, bool recursively)
{
    purc_variant_t cloned;
    cloned = pcvariant_get(PVT(_DOUBLET));
    if (cloned == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    purc_variant_t first, second;
    if (recursively) {
        first = pcvariant_container_clone(doublet->doublet[0], recursively);
        if (first == PURC_VARIANT_INVALID) {
            goto failed;
        }

        second = pcvariant_container_clone(doublet->doublet[1], recursively);
        if (second == PURC_VARIANT_INVALID) {
            purc_variant_unref(first);
            goto failed;
        }
    }
    else {
        first = purc_variant_ref(doublet->doublet[0]);
        second = purc_variant_ref(doublet->doublet[1]);
    }

    cloned->doublet[0] = first;
    cloned->doublet[1] = second;

    return cloned;

failed:
    pcvariant_put(cloned);
    return PURC_VARIANT_INVALID;
}

void pcvariant_doublet_release(purc_variant_t doublet)
{
    PURC_VARIANT_SAFE_CLEAR(doublet->doublet[0]);
    PURC_VARIANT_SAFE_CLEAR(doublet->doublet[1]);
}

