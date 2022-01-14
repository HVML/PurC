/*
 * @file match_for.c
 * @author Xu Xiaohong
 * @date 2021/10/10
 * @brief The implementation of public part for match_for.
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
match_for_rule_apply(struct match_for_rule *rule, purc_variant_t val,
        bool *matched)
{
    struct string_matching_logical_expression *smle = rule->smle;
    *matched = false;
    if (!smle) {
        *matched = true;
        return 0;
    }

    return string_matching_logical_expression_match(smle, val, matched);
}

