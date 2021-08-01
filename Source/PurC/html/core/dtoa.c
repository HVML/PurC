/**
 * @file dtoa.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of dtoa.
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

#include "html/core/str.h"
#include "html/core/diyfp.h"
#include "html/core/dtoa.h"

#include <math.h>
#include <string.h>


static inline void
pchtml_grisu2_round(unsigned char *start, size_t len, uint64_t delta, uint64_t rest,
                    uint64_t ten_kappa, uint64_t wp_w)
{
    while (rest < wp_w && delta - rest >= ten_kappa
           && (rest + ten_kappa < wp_w ||  /* closer */
               wp_w - rest > rest + ten_kappa - wp_w))
    {
        start[len - 1]--;
        rest += ten_kappa;
    }
}

static inline int
pchtml_dec_count(uint32_t n)
{
    if (n < 10) return 1;
    if (n < 100) return 2;
    if (n < 1000) return 3;
    if (n < 10000) return 4;
    if (n < 100000) return 5;
    if (n < 1000000) return 6;
    if (n < 10000000) return 7;
    if (n < 100000000) return 8;
    if (n < 1000000000) return 9;

    return 10;
}

static inline size_t
pchtml_grisu2_gen(pchtml_diyfp_t W, pchtml_diyfp_t Mp, uint64_t delta,
                  unsigned char *begin, unsigned char *end, int *dec_exp)
{
    int kappa;
    unsigned char c, *p;
    uint32_t p1, d;
    uint64_t p2, tmp;
    pchtml_diyfp_t one, wp_w;

    static const uint64_t pow10[] = {
        1,
        10,
        100,
        1000,
        10000,
        100000,
        1000000,
        10000000,
        100000000,
        1000000000
    };

    wp_w = pchtml_diyfp_sub(Mp, W);

    one = pchtml_diyfp((uint64_t) 1 << -Mp.exp, Mp.exp);
    p1 = (uint32_t) (Mp.significand >> -one.exp);
    p2 = Mp.significand & (one.significand - 1);

    p = begin;

    /* GCC 4.2 complains about uninitialized d. */
    d = 0;

    kappa = pchtml_dec_count(p1);

    while (kappa > 0) {
        switch (kappa) {
            case 10: d = p1 / 1000000000; p1 %= 1000000000; break;
            case  9: d = p1 /  100000000; p1 %=  100000000; break;
            case  8: d = p1 /   10000000; p1 %=   10000000; break;
            case  7: d = p1 /    1000000; p1 %=    1000000; break;
            case  6: d = p1 /     100000; p1 %=     100000; break;
            case  5: d = p1 /      10000; p1 %=      10000; break;
            case  4: d = p1 /       1000; p1 %=       1000; break;
            case  3: d = p1 /        100; p1 %=        100; break;
            case  2: d = p1 /         10; p1 %=         10; break;
            case  1: d = p1;              p1 =           0; break;
            default:
                /* Never go here. */
                return 0;
        }

        if (d != 0 || p != begin) {
            *p = '0' + d;

            p += 1;
            if (p == end) {
                return p - begin;
            }
        }

        kappa--;

        tmp = ((uint64_t) p1 << -one.exp) + p2;

        if (tmp <= delta) {
            *dec_exp += kappa;
            pchtml_grisu2_round(begin, p - begin, delta, tmp,
                                pow10[kappa] << -one.exp, wp_w.significand);
            return p - begin;
        }
    }

    /* kappa = 0. */

    for ( ;; ) {
        p2 *= 10;
        delta *= 10;
        c = (char) (p2 >> -one.exp);

        if (c != 0 || p != begin) {
            *p = '0' + c;

            p += 1;
            if (p == end) {
                return p - begin;
            }
        }

        p2 &= one.significand - 1;
        kappa--;

        if (p2 < delta) {
            *dec_exp += kappa;
            tmp = (-kappa < 10) ? pow10[-kappa] : 0;
            pchtml_grisu2_round(begin, p - begin, delta, p2, one.significand,
                                wp_w.significand * tmp);
            break;
        }
    }

    return p - begin;
}

static inline pchtml_diyfp_t
pchtml_diyfp_normalize_boundary(pchtml_diyfp_t v)
{
    while ((v.significand & (PCHTML_DBL_HIDDEN_BIT << 1)) == 0) {
            v.significand <<= 1;
            v.exp--;
    }

    return pchtml_diyfp_shift_left(v, PCHTML_SIGNIFICAND_SHIFT - 2);
}

static inline void
pchtml_diyfp_normalize_boundaries(pchtml_diyfp_t v, pchtml_diyfp_t* minus,
                                  pchtml_diyfp_t* plus)
{
    pchtml_diyfp_t pl, mi;

    pl = pchtml_diyfp_normalize_boundary(pchtml_diyfp((v.significand << 1) + 1,
                                                      v.exp - 1));

    if (v.significand == PCHTML_DBL_HIDDEN_BIT) {
        mi = pchtml_diyfp((v.significand << 2) - 1, v.exp - 2);

    } else {
        mi = pchtml_diyfp((v.significand << 1) - 1, v.exp - 1);
    }

    mi.significand <<= mi.exp - pl.exp;
    mi.exp = pl.exp;

    *plus = pl;
    *minus = mi;
}

static inline size_t
pchtml_grisu2(double value, unsigned char *begin, unsigned char *end, int *dec_exp)
{
    pchtml_diyfp_t v, w_m, w_p, c_mk, W, Wp, Wm;

    v = pchtml_diyfp_from_d2(value);

    pchtml_diyfp_normalize_boundaries(v, &w_m, &w_p);

    c_mk = pchtml_cached_power_bin(w_p.exp, dec_exp);
    W = pchtml_diyfp_mul(pchtml_diyfp_normalize(v), c_mk);

    Wp = pchtml_diyfp_mul(w_p, c_mk);
    Wm = pchtml_diyfp_mul(w_m, c_mk);

    Wm.significand++;
    Wp.significand--;

   return pchtml_grisu2_gen(W, Wp, Wp.significand - Wm.significand, begin, end,
                            dec_exp);
}

static inline size_t
pchtml_write_exponent(int exp, unsigned char *begin, unsigned char *end)
{
    char *p;
    size_t len;
    uint32_t u32;
    char buf[4];

    /* -324 <= exp <= 308. */

    if ((begin + (sizeof(buf) - 1) + 1) >= end) {
        return 0;
    }

    if (exp < 0) {
        *begin = '-';
        begin += 1;

        exp = -exp;
    }
    else {
        *begin++ = '+';
    }

    u32 = exp;
    p = buf + (sizeof(buf) - 1);

    do {
        *--p = u32 % 10 + '0';
        u32 /= 10;
    }
    while (u32 != 0);

    len = buf + (sizeof(buf) - 1) - p;

    memcpy(begin, p, len);

    return len + 1;
}

static inline size_t
pchtml_prettify(unsigned char *begin, unsigned char *end, size_t len, int dec_exp)
{
    int kk, offset, length;
    size_t size;

    /* 10^(kk-1) <= v < 10^kk */

    length = (int) len;
    kk = length + dec_exp;

    if (length <= kk && kk <= 21) {
        /* 1234e7 -> 12340000000 */

        if (kk - length > 0) {
            if ((&begin[length] + (kk - length)) < end) {
                memset(&begin[length], '0', kk - length);
            }
            else {
                memset(&begin[length], '0', (end - &begin[length]));
            }
        }

        return kk;
    }
    else if (0 < kk && kk <= 21) {
        /* 1234e-2 -> 12.34 */

        if ((&begin[kk + 1] + (length - kk)) >= end) {
            return length;
        }

        memmove(&begin[kk + 1], &begin[kk], length - kk);
        begin[kk] = '.';

        return (length + 1);
    }
    else if (-6 < kk && kk <= 0) {
        /* 1234e-6 -> 0.001234 */

        offset = 2 - kk;
        if ((&begin[offset] + length) >= end
            || (begin + 2) >= end)
        {
            return length;
        }

        memmove(&begin[offset], begin, length);
        begin[0] = '0';
        begin[1] = '.';

        if (offset - 2 > 0) {
            if ((&begin[2] + (offset - 2)) >= end) {
                return length;
            }

            memset(&begin[2], '0', offset - 2);
        }

        return (length + offset);
    }
    else if (length == 1) {
        /* 1e30 */

        if ((begin + 1) >= end) {
            return length;
        }

        begin[1] = 'e';

        size =  pchtml_write_exponent(kk - 1, &begin[2], end);

        return (size + 2);
    }

    /* 1234e30 -> 1.234e33 */

    if ((&begin[2] + (length - 1)) >= end) {
        return length;
    }

    memmove(&begin[2], &begin[1], length - 1);
    begin[1] = '.';
    begin[length + 1] = 'e';

    size = pchtml_write_exponent(kk - 1, &begin[length + 2], end);

    return (size + length + 2);
}

size_t
pchtml_dtoa(double value, unsigned char *begin, size_t len)
{
    int dec_exp, minus;
    size_t length;
    unsigned char *end = begin + len;

    /* Not handling NaN and inf. */

    minus = 0;

    if (value == 0) {
        *begin = '0';

        return 1;
    }

    if (signbit(value)) {
        *begin = '-';

        begin += 1;
        if (begin == end) {
            return 1;
        }

        value = -value;
        minus = 1;
    }

    length = pchtml_grisu2(value, begin, end, &dec_exp);
    length = pchtml_prettify(begin, end, length, dec_exp);

    return (minus + length);
}
