/*
 * @file math.h
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The header file of math operation.
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

#ifndef _DVOJBS_MATH_H_
#define _DVOJBS_MATH_H_

#include "purc-variant.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef purc_variant_t (*pcdvobjs_create) (void);

int
math_eval(const char *input, double *d, purc_variant_t param)
__attribute__((visibility("hidden")));

int
math_eval_l(const char *input, long double *d, purc_variant_t param)
__attribute__((visibility("hidden")));

int
math_uni(double *r, double (*f)(double a), double a)
__attribute__((visibility("hidden")));

int
math_unil(long double *r, long double (*f)(long double a), long double a)
__attribute__((visibility("hidden")));

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  // _DVOJBS_MATH_H_
