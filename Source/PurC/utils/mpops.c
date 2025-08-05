/**
 * @file mpops.c
 * @author Vincent Wei
 * @date 2025/07/02
 * @brief The implementation of multi precision operations
 *      (copied from QuickJS).
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

/*
 * QuickJS Javascript Engine
 *
 * Copyright (c) 2017-2025 Fabrice Bellard
 * Copyright (c) 2017-2025 Charlie Gordon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#define _GNU_SOURCE

#include "config.h"

#include "private/mpops.h"

#include <string.h>

bi_limb_t
mp_add(bi_limb_t *res, const bi_limb_t *op1, const bi_limb_t *op2,
        bi_limb_t n, bi_limb_t carry)
{
    bi_limb_t i;
    for (i = 0; i < n; i++) {
        ADDC(res[i], carry, op1[i], op2[i], carry);
    }

    return carry;
}

bi_limb_t
mp_sub(bi_limb_t *res, const bi_limb_t *op1, const bi_limb_t *op2,
        int n, bi_limb_t carry)
{
    int i;
    bi_limb_t k, a, v, k1;

    k = carry;
    for(i=0;i<n;i++) {
        v = op1[i];
        a = v - op2[i];
        k1 = a > v;
        v = a - k;
        k = (v > a) | k1;
        res[i] = v;
    }
    return k;
}

/* compute 0 - op2. carry = 0 or 1. */
bi_limb_t
mp_neg(bi_limb_t *res, const bi_limb_t *op2, int n)
{
    int i;
    bi_limb_t v, carry;

    carry = 1;
    for(i=0;i<n;i++) {
        v = ~op2[i] + carry;
        carry = v < carry;
        res[i] = v;
    }
    return carry;
}

/* tabr[] = taba[] * b + l. Return the high carry */
bi_limb_t
mp_mul1(bi_limb_t *tabr, const bi_limb_t *taba, bi_limb_t n,
                      bi_limb_t b, bi_limb_t l)
{
    bi_limb_t i;
    bi_dlimb_t t;

    for(i = 0; i < n; i++) {
        t = (bi_dlimb_t)taba[i] * (bi_dlimb_t)b + l;
        tabr[i] = t;
        l = t >> BIGINT_LIMB_BITS;
    }
    return l;
}

bi_limb_t
mp_div1(bi_limb_t *tabr, const bi_limb_t *taba, bi_limb_t n,
                      bi_limb_t b, bi_limb_t r)
{
    bi_slimb_t i;
    bi_dlimb_t a1;
    for(i = n - 1; i >= 0; i--) {
        a1 = ((bi_dlimb_t)r << BIGINT_LIMB_BITS) | taba[i];
        tabr[i] = a1 / b;
        r = a1 % b;
    }
    return r;
}

/* tabr[] += taba[] * b, return the high word. */
bi_limb_t
mp_add_mul1(bi_limb_t *tabr, const bi_limb_t *taba, bi_limb_t n, bi_limb_t b)
{
    bi_limb_t i, l;
    bi_dlimb_t t;

    l = 0;
    for (i = 0; i < n; i++) {
        t = (bi_dlimb_t)taba[i] * (bi_dlimb_t)b + l + tabr[i];
        tabr[i] = t;
        l = t >> BIGINT_LIMB_BITS;
    }
    return l;
}

/* size of the result : op1_size + op2_size. */
void mp_mul_basecase(bi_limb_t *result,
        const bi_limb_t *op1, bi_limb_t op1_size,
        const bi_limb_t *op2, bi_limb_t op2_size)
{
    bi_limb_t i;
    bi_limb_t r;

    result[op1_size] = mp_mul1(result, op1, op1_size, op2[0], 0);
    for (i=1;i<op2_size;i++) {
        r = mp_add_mul1(result + i, op1, op1_size, op2[i]);
        result[i + op1_size] = r;
    }
}

/* tabr[] -= taba[] * b. Return the value to substract to the high
   word. */
bi_limb_t
mp_sub_mul1(bi_limb_t *tabr, const bi_limb_t *taba, bi_limb_t n, bi_limb_t b)
{
    bi_limb_t i, l;
    bi_dlimb_t t;

    l = 0;
    for(i = 0; i < n; i++) {
        t = tabr[i] - (bi_dlimb_t)taba[i] * (bi_dlimb_t)b - l;
        tabr[i] = t;
        l = -(t >> BIGINT_LIMB_BITS);
    }
    return l;
}

/* WARNING: d must be >= 2^(BIGINT_LIMB_BITS-1) */
static inline bi_limb_t udiv1norm_init(bi_limb_t d)
{
    bi_limb_t a0, a1;
    a1 = -d - 1;
    a0 = -1;
    return (((bi_dlimb_t)a1 << BIGINT_LIMB_BITS) | a0) / d;
}

/* return the quotient and the remainder in '*pr'of 'a1*2^BIGINT_LIMB_BITS+a0
   / d' with 0 <= a1 < d. */
static inline bi_limb_t udiv1norm(bi_limb_t *pr, bi_limb_t a1, bi_limb_t a0,
        bi_limb_t d, bi_limb_t d_inv)
{
    bi_limb_t n1m, n_adj, q, r, ah;
    bi_dlimb_t a;
    n1m = ((bi_slimb_t)a0 >> (BIGINT_LIMB_BITS - 1));
    n_adj = a0 + (n1m & d);
    a = (bi_dlimb_t)d_inv * (a1 - n1m) + n_adj;
    q = (a >> BIGINT_LIMB_BITS) + a1;
    /* compute a - q * r and update q so that the remainder is\
       between 0 and d - 1 */
    a = ((bi_dlimb_t)a1 << BIGINT_LIMB_BITS) | a0;
    a = a - (bi_dlimb_t)q * d - d;
    ah = a >> BIGINT_LIMB_BITS;
    q += 1 + ah;
    r = (bi_limb_t)a + (ah & d);
    *pr = r;
    return q;
}

#define UDIV1NORM_THRESHOLD 3

/* b must be >= 1 << (BIGINT_LIMB_BITS - 1) */
bi_limb_t
mp_div1norm(bi_limb_t *tabr, const bi_limb_t *taba, bi_limb_t n,
        bi_limb_t b, bi_limb_t r)
{
    bi_slimb_t i;

    if (n >= UDIV1NORM_THRESHOLD) {
        bi_limb_t b_inv;
        b_inv = udiv1norm_init(b);
        for(i = n - 1; i >= 0; i--) {
            tabr[i] = udiv1norm(&r, r, taba[i], b, b_inv);
        }
    } else {
        bi_dlimb_t a1;
        for(i = n - 1; i >= 0; i--) {
            a1 = ((bi_dlimb_t)r << BIGINT_LIMB_BITS) | taba[i];
            tabr[i] = a1 / b;
            r = a1 % b;
        }
    }
    return r;
}

/* base case division: divides taba[0..na-1] by tabb[0..nb-1]. tabb[nb
   - 1] must be >= 1 << (BIGINT_LIMB_BITS - 1). na - nb must be >= 0. 'taba'
   is modified and contains the remainder (nb limbs). tabq[0..na-nb]
   contains the quotient with tabq[na - nb] <= 1. */
void
mp_divnorm(bi_limb_t *tabq, bi_limb_t *taba, bi_limb_t na,
        const bi_limb_t *tabb, bi_limb_t nb)
{
    bi_limb_t r, a, c, q, v, b1, b1_inv, n, dummy_r;
    int i, j;

    b1 = tabb[nb - 1];
    if (nb == 1) {
        taba[0] = mp_div1norm(tabq, taba, na, b1, 0);
        return;
    }
    n = na - nb;

    if (n >= UDIV1NORM_THRESHOLD)
        b1_inv = udiv1norm_init(b1);
    else
        b1_inv = 0;

    /* first iteration: the quotient is only 0 or 1 */
    q = 1;
    for(j = nb - 1; j >= 0; j--) {
        if (taba[n + j] != tabb[j]) {
            if (taba[n + j] < tabb[j])
                q = 0;
            break;
        }
    }
    tabq[n] = q;
    if (q) {
        mp_sub(taba + n, taba + n, tabb, nb, 0);
    }

    for(i = n - 1; i >= 0; i--) {
        if (UNLIKELY(taba[i + nb] >= b1)) {
            q = -1;
        } else if (b1_inv) {
            q = udiv1norm(&dummy_r, taba[i + nb], taba[i + nb - 1], b1, b1_inv);
        } else {
            bi_dlimb_t al;
            al = ((bi_dlimb_t)taba[i + nb] << BIGINT_LIMB_BITS) | taba[i + nb - 1];
            q = al / b1;
            r = al % b1;
        }
        r = mp_sub_mul1(taba + i, tabb, nb, q);

        v = taba[i + nb];
        a = v - r;
        c = (a > v);
        taba[i + nb] = a;

        if (c != 0) {
            /* negative result */
            for(;;) {
                q--;
                c = mp_add(taba + i, taba + i, tabb, nb, 0);
                /* propagate carry and test if positive result */
                if (c != 0) {
                    if (++taba[i + nb] == 0) {
                        break;
                    }
                }
            }
        }
        tabq[i] = q;
    }
}

/* 1 <= shift <= BIGINT_LIMB_BITS - 1 */
bi_limb_t
mp_shl(bi_limb_t *tabr, const bi_limb_t *taba, int n, int shift)
{
    int i;
    bi_limb_t l, v;
    l = 0;
    for(i = 0; i < n; i++) {
        v = taba[i];
        tabr[i] = (v << shift) | l;
        l = v >> (BIGINT_LIMB_BITS - shift);
    }
    return l;
}

/* r = (a + high*B^n) >> shift. Return the remainder r (0 <= r < 2^shift). 
   1 <= shift <= LIMB_BITS - 1 */
bi_limb_t
mp_shr(bi_limb_t *tab_r, const bi_limb_t *tab, int n, int shift, bi_limb_t high)
{
    int i;
    bi_limb_t l, a;

    l = high;
    for(i = n - 1; i >= 0; i--) {
        a = tab[i];
        tab_r[i] = (a >> shift) | (l << (BIGINT_LIMB_BITS - shift));
        l = a;
    }
    return l & (((bi_limb_t)1 << shift) - 1);
}

