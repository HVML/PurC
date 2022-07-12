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


enum math_pre_defined_var {
    MATH_PI,
    MATH_E,
    MATH_LN2,
    MATH_LN10,
    MATH_LOG2E,
    MATH_LOG10E,
    MATH_SQRT1_2,
    MATH_SQRT2,
    MATH_PRE_DEFINED_MAX,
};

typedef purc_variant_t (*pcdvobjs_create) (void);

int
math_eval(const char *input, double *d, purc_variant_t param)
__attribute__((visibility("hidden")));

int
math_eval_l(const char *input, long double *d, purc_variant_t param)
__attribute__((visibility("hidden")));

int
math_voi(double *r, double (*f)(void))
__attribute__((visibility("hidden")));

int
math_voi_l(long double *r, long double (*f)(void))
__attribute__((visibility("hidden")));

int
math_uni(double *r, double (*f)(double a), double a)
__attribute__((visibility("hidden")));

int
math_uni_l(long double *r, long double (*f)(long double a), long double a)
__attribute__((visibility("hidden")));

int
math_bin(double *r, double (*f)(double a, double b), double a, double b)
__attribute__((visibility("hidden")));

int
math_bin_l(long double *r, long double (*f)(long double a, long double b),
        long double a, long double b)
__attribute__((visibility("hidden")));

double
math_max(double a, double b)
__attribute__((visibility("hidden")));

long double
math_max_l(long double a, long double b)
__attribute__((visibility("hidden")));

double
math_min(double a, double b)
__attribute__((visibility("hidden")));

long double
math_min_l(long double a, long double b)
__attribute__((visibility("hidden")));

double
math_abs(double a)
__attribute__((visibility("hidden")));

long double
math_abs_l(long double a)
__attribute__((visibility("hidden")));

double
math_sign(double a)
__attribute__((visibility("hidden")));

long double
math_sign_l(long double a)
__attribute__((visibility("hidden")));

double
math_random(void)
__attribute__((visibility("hidden")));

long double
math_random_l(void)
__attribute__((visibility("hidden")));

double
math_pre_defined(enum math_pre_defined_var v)
__attribute__((visibility("hidden")));

long double
math_pre_defined_l(enum math_pre_defined_var v)
__attribute__((visibility("hidden")));

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  // _DVOJBS_MATH_H_
