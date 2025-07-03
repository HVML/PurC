/*
 * @file mpops.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2025/07/03
 * @brief The internal interfaces for multi precision operations.
 *
 * Copyright (C) 2025 FMSoft <https://www.fmsoft.cn>
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

#ifndef PURC_PRIVATE_MPOPS_H
#define PURC_PRIVATE_MPOPS_H

#include "config.h"

#if HAVE(INT128_T)
#   define BIGINT_LIMB_BITS            64
    typedef __int128 int128_t;
    typedef unsigned __int128 uint128_t;
    typedef int64_t bi_slimb_t;
    typedef uint64_t bi_limb_t;
    typedef int128_t bi_sdlimb_t;
    typedef uint128_t bi_dlimb_t;
#else
#   define BIGINT_LIMB_BITS            32
    typedef int32_t bi_slimb_t;
    typedef uint32_t bi_limb_t;
    typedef int64_t bi_sdlimb_t;
    typedef uint64_t bi_dlimb_t;
#endif

/* WARNING: undefined if a = 0 */
static inline int clz32(unsigned int a)
{
    return __builtin_clz(a);
}

/* WARNING: undefined if a = 0 */
static inline int clz64(uint64_t a)
{
    return __builtin_clzll(a);
}

/* WARNING: undefined if a = 0 */
static inline int ctz32(unsigned int a)
{
    return __builtin_ctz(a);
}

/* WARNING: undefined if a = 0 */
static inline int ctz64(uint64_t a)
{
    return __builtin_ctzll(a);
}

static inline uint64_t float64_as_uint64(double d)
{
    union {
        double d;
        uint64_t u64;
    } u;
    u.d = d;
    return u.u64;
}

static inline double uint64_as_float64(uint64_t u64)
{
    union {
        double d;
        uint64_t u64;
    } u;
    u.u64 = u64;
    return u.d;
}

static inline double fromfp16(uint16_t v)
{
    double d;
    uint32_t v1;
    v1 = v & 0x7fff;
    if (UNLIKELY(v1 >= 0x7c00))
        v1 += 0x1f8000; /* NaN or infinity */
    d = uint64_as_float64(((uint64_t)(v >> 15) << 63) | ((uint64_t)v1 << (52 - 10)));
    return d * 0x1p1008;
}

static inline uint16_t tofp16(double d)
{
    uint64_t a, addend;
    uint32_t v, sgn;
    int shift;

    a = float64_as_uint64(d);
    sgn = a >> 63;
    a = a & 0x7fffffffffffffff;
    if (UNLIKELY(a > 0x7ff0000000000000)) {
        /* nan */
        v = 0x7c01;
    } else if (a < 0x3f10000000000000) { /* 0x1p-14 */
        /* subnormal f16 number or zero */
        if (a <= 0x3e60000000000000) { /* 0x1p-25 */
            v = 0x0000; /* zero */
        } else {
            shift = 1051 - (a >> 52);
            a = ((uint64_t)1 << 52) | (a & (((uint64_t)1 << 52) - 1));
            addend = ((a >> shift) & 1) + (((uint64_t)1 << (shift - 1)) - 1);
            v = (a + addend) >> shift;
        }
    } else {
        /* normal number or infinity */
        a -= 0x3f00000000000000; /* adjust the exponent */
        /* round */
        addend = ((a >> (52 - 10)) & 1) + (((uint64_t)1 << (52 - 11)) - 1);
        v = (a + addend) >> (52 - 10);
        /* overflow ? */
        if (UNLIKELY(v > 0x7c00))
            v = 0x7c00;
    }
    return v | (sgn << 15);
}

static inline int isfp16nan(uint16_t v)
{
    return (v & 0x7FFF) > 0x7C00;
}

static inline int isfp16zero(uint16_t v)
{
    return (v & 0x7FFF) == 0;
}

static inline int to_digit(int c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'Z')
        return c - 'A' + 10;
    else if (c >= 'a' && c <= 'z')
        return c - 'a' + 10;
    else
        return 36;
}

#define ADDC(res, carry_out, op1, op2, carry_in)        \
do {                                                    \
    bi_limb_t __v, __a, __k, __k1;                      \
    __v = (op1);                                        \
    __a = __v + (op2);                                  \
    __k1 = __a < __v;                                   \
    __k = (carry_in);                                   \
    __a = __a + __k;                                    \
    carry_out = (__a < __k) | __k1;                     \
    res = __a;                                          \
} while (0)

#if BIGINT_LIMB_BITS == 32
/* a != 0 */
static inline bi_limb_t bi_limb_clz(bi_limb_t a)
{
    return clz32(a);
}
#else
static inline bi_limb_t bi_limb_clz(bi_limb_t a)
{
    return clz64(a);
}
#endif

/* handle a = 0 too */
static inline bi_limb_t bi_limb_safe_clz(bi_limb_t a)
{
    if (a == 0)
        return BIGINT_LIMB_BITS;
    else
        return bi_limb_clz(a);
}

#ifdef __cplusplus
extern "C" {
#endif

bi_limb_t mp_add(bi_limb_t *res, const bi_limb_t *op1, const bi_limb_t *op2,
        bi_limb_t n, bi_limb_t carry) WTF_INTERNAL;
bi_limb_t mp_sub(bi_limb_t *res, const bi_limb_t *op1, const bi_limb_t *op2,
        int n, bi_limb_t carry) WTF_INTERNAL;
bi_limb_t mp_neg(bi_limb_t *res, const bi_limb_t *op2, int n) WTF_INTERNAL;

bi_limb_t mp_mul1(bi_limb_t *tabr, const bi_limb_t *taba, bi_limb_t n,
        bi_limb_t b, bi_limb_t l) WTF_INTERNAL;
bi_limb_t mp_div1(bi_limb_t *tabr, const bi_limb_t *taba, bi_limb_t n,
        bi_limb_t b, bi_limb_t r) WTF_INTERNAL;
bi_limb_t mp_add_mul1(bi_limb_t *tabr, const bi_limb_t *taba,
        bi_limb_t n, bi_limb_t b) WTF_INTERNAL;

void mp_mul_basecase(bi_limb_t *result,
        const bi_limb_t *op1, bi_limb_t op1_size,
        const bi_limb_t *op2, bi_limb_t op2_size) WTF_INTERNAL;

bi_limb_t mp_sub_mul1(bi_limb_t *tabr, const bi_limb_t *taba,
        bi_limb_t n, bi_limb_t b) WTF_INTERNAL;
bi_limb_t mp_div1norm(bi_limb_t *tabr, const bi_limb_t *taba,
        bi_limb_t n, bi_limb_t b, bi_limb_t r) WTF_INTERNAL;
void mp_divnorm(bi_limb_t *tabq, bi_limb_t *taba, bi_limb_t na,
        const bi_limb_t *tabb, bi_limb_t nb) WTF_INTERNAL;

bi_limb_t mp_shl(bi_limb_t *tabr, const bi_limb_t *taba,
        int n, int shift) WTF_INTERNAL;
bi_limb_t mp_shr(bi_limb_t *tab_r, const bi_limb_t *tab,
        int n, int shift, bi_limb_t high) WTF_INTERNAL;

size_t u32toa(char *buf, uint32_t n) WTF_INTERNAL;
size_t i32toa(char *buf, int32_t n) WTF_INTERNAL;
size_t u64toa(char *buf, uint64_t n) WTF_INTERNAL;
size_t i64toa(char *buf, int64_t n) WTF_INTERNAL;
size_t u64toa_radix(char *buf, uint64_t n, unsigned int radix) WTF_INTERNAL;
size_t i64toa_radix(char *buf, int64_t n, unsigned int radix) WTF_INTERNAL;

#ifdef __cplusplus
}
#endif

#endif /* not defined PURC_PRIVATE_MPOPS_H */

