/*
 * @file match_for.c
 * @author Xu Xiaohong
 * @date 2022/01/14
 * @brief The implementation of public part for MATCH FOR executor.
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

#include "match_for.h"

#include "pcexe-helper.h"

#include "private/executor.h"
#include "private/variant.h"

#include "private/debug.h"
#include "private/errors.h"

#include <math.h>

int
match_for_rule_eval(struct match_for_rule *rule, purc_variant_t val,
        bool *result)
{
    // TODO: check type of val

    struct number_comparing_logical_expression *ncle = rule->ncle;
    struct string_matching_logical_expression  *smle = rule->smle;
    if (!ncle && !smle) {
        *result = true;
        return 0;
    }

    *result = false;
    if (smle) {
        PC_ASSERT(!ncle);
        return string_matching_logical_expression_match(smle, val, result);
    }

    double curr = purc_variant_numerify(val);
    return number_comparing_logical_expression_match(ncle, curr, result);
}

