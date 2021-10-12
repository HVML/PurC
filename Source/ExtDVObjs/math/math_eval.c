/*
 * @file math_eval.c
 * @author Xu Xiaohong
 * @date 2021/10/09
 * @brief The implementation of math.eval
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

#include "purc-variant.h"
#include "mathlib.h"

#define UNUSED_PARAM (void)

#include "math.tab.c"

purc_variant_t
eval_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    return PURC_VARIANT_INVALID;
}

