/**
 * @file diyfp.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for diyfp.
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
 *
 * This implementation of HTML parser is derived from Lexbor <http://lexbor.com/>.
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PCHTML_DIYFP_H
#define PCHTML_DIYFP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/base.h"

#include <math.h>


#define pchtml_diyfp(_s, _e)            (pchtml_diyfp_t)                        \
                                        { .significand = (_s), .exp = (_e) }
#define pchtml_uint64_hl(h, l)          (((uint64_t) (h) << 32) + (l))


#define PCHTML_DBL_SIGNIFICAND_SIZE    52
#define PCHTML_DBL_EXPONENT_BIAS       (0x3FF + PCHTML_DBL_SIGNIFICAND_SIZE)
#define PCHTML_DBL_EXPONENT_MIN        (-PCHTML_DBL_EXPONENT_BIAS)
#define PCHTML_DBL_EXPONENT_MAX        (0x7FF - PCHTML_DBL_EXPONENT_BIAS)
#define PCHTML_DBL_EXPONENT_DENORMAL   (-PCHTML_DBL_EXPONENT_BIAS + 1)

#define PCHTML_DBL_SIGNIFICAND_MASK    pchtml_uint64_hl(0x000FFFFF, 0xFFFFFFFF)
#define PCHTML_DBL_HIDDEN_BIT          pchtml_uint64_hl(0x00100000, 0x00000000)
#define PCHTML_DBL_EXPONENT_MASK       pchtml_uint64_hl(0x7FF00000, 0x00000000)

#define PCHTML_DIYFP_SIGNIFICAND_SIZE  64

#define PCHTML_SIGNIFICAND_SIZE        53
#define PCHTML_SIGNIFICAND_SHIFT       (PCHTML_DIYFP_SIGNIFICAND_SIZE          \
                                       - PCHTML_DBL_SIGNIFICAND_SIZE)

#define PCHTML_DECIMAL_EXPONENT_OFF    348
#define PCHTML_DECIMAL_EXPONENT_MIN    (-348)
#define PCHTML_DECIMAL_EXPONENT_MAX    340
#define PCHTML_DECIMAL_EXPONENT_DIST   8


typedef struct {
    uint64_t significand;
    int      exp;
}
pchtml_diyfp_t;


pchtml_diyfp_t
pchtml_cached_power_dec(int exp, int *dec_exp) WTF_INTERNAL;

pchtml_diyfp_t
pchtml_cached_power_bin(int exp, int *dec_exp) WTF_INTERNAL;


/*
 * Inline functions
 */
// #if (PCHTML_HAVE_BUILTIN_CLZLL)      // gengyue
// #define nxt_leading_zeros64(x)  (((x) == 0) ? 64 : __builtin_clzll(x))

// #else

static inline uint64_t
pchtml_diyfp_leading_zeros64(uint64_t x)
{
    uint64_t  n;

    if (x == 0) {
        return 64;
    }

    n = 0;

    while ((x & 0x8000000000000000) == 0) {
        n++;
        x <<= 1;
    }

    return n;
}

// #endif

static inline pchtml_diyfp_t
pchtml_diyfp_from_d2(double d)
{
    int biased_exp;
    uint64_t significand;
    pchtml_diyfp_t r;

    union {
        double   d;
        uint64_t u64;
    }
    u;

    u.d = d;

    biased_exp = (u.u64 & PCHTML_DBL_EXPONENT_MASK)
                 >> PCHTML_DBL_SIGNIFICAND_SIZE;
    significand = u.u64 & PCHTML_DBL_SIGNIFICAND_MASK;

    if (biased_exp != 0) {
        r.significand = significand + PCHTML_DBL_HIDDEN_BIT;
        r.exp = biased_exp - PCHTML_DBL_EXPONENT_BIAS;
    }
    else {
        r.significand = significand;
        r.exp = PCHTML_DBL_EXPONENT_MIN + 1;
    }

    return r;
}

static inline double
pchtml_diyfp_2d(pchtml_diyfp_t v)
{
    int exp;
    uint64_t significand, biased_exp;

    union {
        double   d;
        uint64_t u64;
    }
    u;

    exp = v.exp;
    significand = v.significand;

    while (significand > PCHTML_DBL_HIDDEN_BIT + PCHTML_DBL_SIGNIFICAND_MASK) {
        significand >>= 1;
        exp++;
    }

    if (exp >= PCHTML_DBL_EXPONENT_MAX) {
        return INFINITY;
    }

    if (exp < PCHTML_DBL_EXPONENT_DENORMAL) {
        return 0.0;
    }

    while (exp > PCHTML_DBL_EXPONENT_DENORMAL
           && (significand & PCHTML_DBL_HIDDEN_BIT) == 0)
    {
        significand <<= 1;
        exp--;
    }

    if (exp == PCHTML_DBL_EXPONENT_DENORMAL
        && (significand & PCHTML_DBL_HIDDEN_BIT) == 0)
    {
        biased_exp = 0;

    } else {
        biased_exp = (uint64_t) (exp + PCHTML_DBL_EXPONENT_BIAS);
    }

    u.u64 = (significand & PCHTML_DBL_SIGNIFICAND_MASK)
            | (biased_exp << PCHTML_DBL_SIGNIFICAND_SIZE);

    return u.d;
}

static inline pchtml_diyfp_t
pchtml_diyfp_shift_left(pchtml_diyfp_t v, unsigned shift)
{
    return pchtml_diyfp(v.significand << shift, v.exp - shift);
}

static inline pchtml_diyfp_t
pchtml_diyfp_shift_right(pchtml_diyfp_t v, unsigned shift)
{
    return pchtml_diyfp(v.significand >> shift, v.exp + shift);
}

static inline pchtml_diyfp_t
pchtml_diyfp_sub(pchtml_diyfp_t lhs, pchtml_diyfp_t rhs)
{
    return pchtml_diyfp(lhs.significand - rhs.significand, lhs.exp);
}

static inline pchtml_diyfp_t
pchtml_diyfp_mul(pchtml_diyfp_t lhs, pchtml_diyfp_t rhs)
{
//#if (PCHTML_HAVE_UNSIGNED_INT128)     // gengyue

//    uint64_t l, h;
//    pchtml_uint128_t u128;

//    u128 = (pchtml_uint128_t) (lhs.significand)
//           * (pchtml_uint128_t) (rhs.significand);

//    h = u128 >> 64;
//    l = (uint64_t) u128;

    /* rounding. */

//    if (l & ((uint64_t) 1 << 63)) {
//        h++;
//    }

//    return pchtml_diyfp(h, lhs.exp + rhs.exp + 64);

//#else

    uint64_t a, b, c, d, ac, bc, ad, bd, tmp;

    a = lhs.significand >> 32;
    b = lhs.significand & 0xffffffff;
    c = rhs.significand >> 32;
    d = rhs.significand & 0xffffffff;

    ac = a * c;
    bc = b * c;
    ad = a * d;
    bd = b * d;

    tmp = (bd >> 32) + (ad & 0xffffffff) + (bc & 0xffffffff);

    /* mult_round. */

    tmp += 1U << 31;

    return pchtml_diyfp(ac + (ad >> 32) + (bc >> 32) + (tmp >> 32),
                        lhs.exp + rhs.exp + 64);
//#endif
}

static inline pchtml_diyfp_t
pchtml_diyfp_normalize(pchtml_diyfp_t v)
{
    return pchtml_diyfp_shift_left(v,
                        (unsigned) pchtml_diyfp_leading_zeros64(v.significand));
}


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_DIYFP_H */
