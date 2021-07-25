/**
 * @file strtod.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of strtod.
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


#include <stdint.h>
#include <math.h>

#include "html/core/diyfp.h"
#include "html/core/strtod.h"


/*
 * Max double: 1.7976931348623157 x 10^308
 * Min non-zero double: 4.9406564584124654 x 10^-324
 * Any x >= 10^309 is interpreted as +infinity.
 * Any x <= 10^-324 is interpreted as 0.
 * Note that 2.5e-324 (despite being smaller than the min double)
 * will be read as non-zero (equal to the min non-zero double).
 */

#define PCHTML_DECIMAL_POWER_MAX 309
#define PCHTML_DECIMAL_POWER_MIN (-324)

#define PCHTML_UINT64_MAX pchtml_uint64_hl(0xFFFFFFFF, 0xFFFFFFFF)
#define PCHTML_UINT64_DECIMAL_DIGITS_MAX 19

#define PCHTML_DENOM_LOG   3
#define PCHTML_DENOM       (1 << PCHTML_DENOM_LOG)


static pchtml_diyfp_t
pchtml_strtod_diyfp_read(const unsigned char *start, size_t length,
                         int *remaining);

static double
pchtml_strtod_diyfp_strtod(const unsigned char *start, size_t length, int exp);


/*
 * Reads digits from the buffer and converts them to a uint64.
 * Reads in as many digits as fit into a uint64.
 * When the string starts with "1844674407370955161" no further digit is read.
 * Since 2^64 = 18446744073709551616 it would still be possible read another
 * digit if it was less or equal than 6, but this would complicate the code.
 */
lxb_inline uint64_t
pchtml_strtod_read_uint64(const unsigned char *start, size_t length,
                          size_t *ndigits)
{
    unsigned char d;
    uint64_t value;
    const unsigned char *p, *e;

    value = 0;

    p = start;
    e = p + length;

    while (p < e && value <= (UINT64_MAX / 10 - 1)) {
        d = *p++ - '0';
        value = 10 * value + d;
    }

    *ndigits = p - start;

    return value;
}

/*
 * Reads a nxt_diyfp_t from the buffer.
 * The returned nxt_diyfp_t is not necessarily normalized.
 * If remaining is zero then the returned nxt_diyfp_t is accurate.
 * Otherwise it has been rounded and has error of at most 1/2 ulp.
 */
static pchtml_diyfp_t
pchtml_strtod_diyfp_read(const unsigned char *start, size_t length, int *remaining)
{
    size_t read;
    uint64_t significand;

    significand = pchtml_strtod_read_uint64(start, length, &read);

    /* Round the significand. */

    if (length != read) {
        if (start[read] >= '5') {
            significand++;
        }
    }

    *remaining = (int) (length - read);

    return pchtml_diyfp(significand, 0);
}

/*
 * Returns 10^exp as an exact nxt_diyfp_t.
 * The given exp must be in the range [1; NXT_DECIMAL_EXPONENT_DIST[.
 */
lxb_inline pchtml_diyfp_t
pchtml_strtod_adjust_pow10(int exp)
{
    switch (exp) {
    case 1:
        return pchtml_diyfp(pchtml_uint64_hl(0xa0000000, 00000000), -60);
    case 2:
        return pchtml_diyfp(pchtml_uint64_hl(0xc8000000, 00000000), -57);
    case 3:
        return pchtml_diyfp(pchtml_uint64_hl(0xfa000000, 00000000), -54);
    case 4:
        return pchtml_diyfp(pchtml_uint64_hl(0x9c400000, 00000000), -50);
    case 5:
        return pchtml_diyfp(pchtml_uint64_hl(0xc3500000, 00000000), -47);
    case 6:
        return pchtml_diyfp(pchtml_uint64_hl(0xf4240000, 00000000), -44);
    case 7:
        return pchtml_diyfp(pchtml_uint64_hl(0x98968000, 00000000), -40);
    default:
        return pchtml_diyfp(0, 0);
    }
}

/*
 * Returns the significand size for a given order of magnitude.
 * If v = f*2^e with 2^p-1 <= f <= 2^p then p+e is v's order of magnitude.
 * This function returns the number of significant binary digits v will have
 * once its encoded into a double. In almost all cases this is equal to
 * NXT_SIGNIFICAND_SIZE. The only exception are denormals. They start with
 * leading zeroes and their effective significand-size is hence smaller.
 */
lxb_inline int
pchtml_strtod_diyfp_sgnd_size(int order)
{
    if (order >= (PCHTML_DBL_EXPONENT_DENORMAL + PCHTML_SIGNIFICAND_SIZE)) {
        return PCHTML_SIGNIFICAND_SIZE;
    }

    if (order <= PCHTML_DBL_EXPONENT_DENORMAL) {
        return 0;
    }

    return order - PCHTML_DBL_EXPONENT_DENORMAL;
}

/*
 * Returns either the correct double or the double that is just below
 * the correct double.
 */
static double
pchtml_strtod_diyfp_strtod(const unsigned char *start, size_t length, int exp)
{
    int magnitude, prec_digits;
    int remaining, dec_exp, adj_exp, orig_e, shift;
    int64_t error;
    uint64_t prec_bits, half_way;
    pchtml_diyfp_t value, pow, adj_pow, rounded;

    value = pchtml_strtod_diyfp_read(start, length, &remaining);

    exp += remaining;

    /*
     * Since some digits may have been dropped the value is not accurate.
     * If remaining is different than 0 than the error is at most .5 ulp
     * (unit in the last place).
     * Using a common denominator to avoid dealing with fractions.
     */

    error = (remaining == 0 ? 0 : PCHTML_DENOM / 2);

    orig_e = value.exp;
    value = pchtml_diyfp_normalize(value);
    error <<= orig_e - value.exp;

    if (exp < PCHTML_DECIMAL_EXPONENT_MIN) {
        return 0.0;
    }

    pow = pchtml_cached_power_dec(exp, &dec_exp);

    if (dec_exp != exp) {
        adj_exp = exp - dec_exp;
        adj_pow = pchtml_strtod_adjust_pow10(exp - dec_exp);
        value = pchtml_diyfp_mul(value, adj_pow);

        if (PCHTML_UINT64_DECIMAL_DIGITS_MAX - (int) length < adj_exp) {
            /*
             * The adjustment power is exact. There is hence only
             * an error of 0.5.
             */
            error += PCHTML_DENOM / 2;
        }
    }

    value = pchtml_diyfp_mul(value, pow);

    /*
     * The error introduced by a multiplication of a * b equals
     *  error_a + error_b + error_a * error_b / 2^64 + 0.5
     *  Substituting a with 'value' and b with 'pow':
     *  error_b = 0.5  (all cached powers have an error of less than 0.5 ulp),
     *  error_ab = 0 or 1 / NXT_DENOM > error_a * error_b / 2^64.
     */

    error += PCHTML_DENOM + (error != 0 ? 1 : 0);

    orig_e = value.exp;
    value = pchtml_diyfp_normalize(value);
    error <<= orig_e - value.exp;

    /*
     * Check whether the double's significand changes when the error is added
     * or substracted.
     */

    magnitude = PCHTML_DIYFP_SIGNIFICAND_SIZE + value.exp;
    prec_digits = PCHTML_DIYFP_SIGNIFICAND_SIZE
        - pchtml_strtod_diyfp_sgnd_size(magnitude);

    if (prec_digits + PCHTML_DENOM_LOG >= PCHTML_DIYFP_SIGNIFICAND_SIZE) {
        /*
         * This can only happen for very small denormals. In this case the
         * half-way multiplied by the denominator exceeds the range of uint64.
         * Simply shift everything to the right.
         */
        shift = prec_digits + PCHTML_DENOM_LOG
            - PCHTML_DIYFP_SIGNIFICAND_SIZE + 1;

        value = pchtml_diyfp_shift_right(value, shift);

        /*
         * Add 1 for the lost precision of error, and NXT_DENOM
         * for the lost precision of value.significand.
         */
        error = (error >> shift) + 1 + PCHTML_DENOM;
        prec_digits -= shift;
    }

    prec_bits = value.significand & (((uint64_t) 1 << prec_digits) - 1);
    prec_bits *= PCHTML_DENOM;

    half_way = (uint64_t) 1 << (prec_digits - 1);
    half_way *= PCHTML_DENOM;

    rounded = pchtml_diyfp_shift_right(value, prec_digits);

    if (prec_bits >= half_way + error) {
        rounded.significand++;
    }

    return pchtml_diyfp_2d(rounded);
}

double
pchtml_strtod_internal(const unsigned char *start, size_t length, int exp)
{
    size_t left, right;
    const unsigned char *p, *e, *b;

    /* Trim leading zeroes. */

    p = start;
    e = p + length;

    while (p < e) {
        if (*p != '0') {
            start = p;
            break;
        }

        p++;
    }

    left = e - p;

    /* Trim trailing zeroes. */

    b = start;
    p = b + left - 1;

    while (p > b) {
        if (*p != '0') {
            break;
        }

        p--;
    }

    right = p - b + 1;

    length = right;

    if (length == 0) {
        return 0.0;
    }

    exp += (int) (left - right);

    if (exp + (int) length - 1 >= PCHTML_DECIMAL_POWER_MAX) {
        return INFINITY;
    }

    if (exp + (int) length <= PCHTML_DECIMAL_POWER_MIN) {
        return 0.0;
    }

    return pchtml_strtod_diyfp_strtod(start, length, exp);
}

#undef PCHTML_DECIMAL_POWER_MAX
#undef PCHTML_DECIMAL_POWER_MIN

#undef PCHTML_UINT64_MAX
#undef PCHTML_UINT64_DECIMAL_DIGITS_MAX

#undef PCHTML_DENOM_LOG
#undef PCHTML_RDENOM
