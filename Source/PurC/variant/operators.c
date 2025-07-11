/*
 * @file operators.c
 * @author Vincent Wei
 * @date 2025/07/09
 * @brief The operators of variant.
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

#include "variant-internals.h"

#include "private/variant.h"
#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"

#include "purc-variant.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <assert.h>

static int variant_compare(purc_variant_t v1, purc_variant_t v2)
{
    int cmp, swapped = 1;

    if (v1->type == PURC_VARIANT_TYPE_BIGINT ||
            v2->type == PURC_VARIANT_TYPE_BIGINT) {
        purc_variant_t a, b;
        if (v1->type == PURC_VARIANT_TYPE_BIGINT) {
            a = v1;
            b = v2;
        }
        else {
            a = v2;
            b = v1;
            swapped = -1;
        }

        switch (b->type) {
        case PURC_VARIANT_TYPE_BIGINT:
            cmp = bigint_cmp(a, b);
            break;
        case PURC_VARIANT_TYPE_LONGINT:
            cmp = bigint_i64_cmp(a, b->i64);
            break;
        case PURC_VARIANT_TYPE_ULONGINT:
            cmp = bigint_u64_cmp(a, b->u64);
            break;
        case PURC_VARIANT_TYPE_LONGDOUBLE:
            cmp = bigint_float64_cmp(a, (double)*b->ld);
            break;
        case PURC_VARIANT_TYPE_NUMBER:
            cmp = bigint_float64_cmp(a, b->d);
            break;
        default:
            cmp = bigint_float64_cmp(a, purc_variant_numerify(b));
            break;
        }
    }
    else if (v1->type == PURC_VARIANT_TYPE_LONGDOUBLE ||
            v2->type == PURC_VARIANT_TYPE_LONGDOUBLE) {
        long double a, b;
        if (v1->type == PURC_VARIANT_TYPE_LONGDOUBLE) {
            a = *v1->ld;
            b = purc_variant_numerify(v2);
        }
        else {
            a = purc_variant_numerify(v1);
            b = *v2->ld;
        }

        if (pcutils_equal_longdoubles(a, b)) {
            cmp = 0;
        }
        else if (a > b) {
            cmp = 1;
        }
        else {
            cmp = -1;
        }
    }
    else if (v1->type == PURC_VARIANT_TYPE_NUMBER ||
            v2->type == PURC_VARIANT_TYPE_NUMBER) {
        double a, b;
        if (v1->type == PURC_VARIANT_TYPE_NUMBER) {
            a = v1->d;
            b = purc_variant_numerify(v2);
        }
        else {
            a = purc_variant_numerify(v1);
            b = v2->d;
        }

        if (pcutils_equal_doubles(a, b)) {
            cmp = 0;
        }
        else if (a > b) {
            cmp = 1;
        }
        else {
            cmp = -1;
        }
    }
    else if (v1->type == PURC_VARIANT_TYPE_LONGINT &&
            v2->type == PURC_VARIANT_TYPE_LONGINT) {
        int64_t a = v1->i64, b = v2->i64;
        if (a == b) {
            cmp = 0;
        }
        else if (a > b) {
            cmp = 1;
        }
        else {
            cmp = -1;
        }
    }
    else if (v1->type == PURC_VARIANT_TYPE_ULONGINT &&
            v2->type == PURC_VARIANT_TYPE_ULONGINT) {
        uint64_t a = v1->u64, b = v2->u64;
        if (a == b) {
            cmp = 0;
        }
        else if (a > b) {
            cmp = 1;
        }
        else {
            cmp = -1;
        }
    }
    else if (v1->type == PURC_VARIANT_TYPE_ULONGINT &&
            v2->type == PURC_VARIANT_TYPE_LONGINT) {
        if (v2->i64 < 0) {
            cmp = 1;
        }
        else {
            uint64_t a = v1->u64, b = (uint64_t)v2->i64;
            if (a == b) {
                cmp = 0;
            }
            else if (a > b) {
                cmp = 1;
            }
            else {
                cmp = -1;
            }
        }
    }
    else if (v1->type == PURC_VARIANT_TYPE_LONGINT &&
            v2->type == PURC_VARIANT_TYPE_ULONGINT) {
        if (v1->i64 < 0) {
            cmp = -1;
        }
        else {
            uint64_t a = (uint64_t)v1->i64, b = v2->u64;
            if (a == b) {
                cmp = 0;
            }
            else if (a > b) {
                cmp = 1;
            }
            else {
                cmp = -1;
            }
        }
    }
    else {
        /* for any other situations */
        double a, b;
        a = purc_variant_numerify(v1);
        b = purc_variant_numerify(v2);

        if (pcutils_equal_doubles(a, b)) {
            cmp = 0;
        }
        else if (a > b) {
            cmp = 1;
        }
        else {
            cmp = -1;
        }
    }

    return cmp * swapped;
}

purc_variant_t
purc_variant_operator_lt(purc_variant_t v1, purc_variant_t v2)
{
    return purc_variant_make_boolean(variant_compare(v1, v2) < 0);
}

purc_variant_t
purc_variant_operator_le(purc_variant_t v1, purc_variant_t v2)
{
    return purc_variant_make_boolean(variant_compare(v1, v2) <= 0);
}

purc_variant_t
purc_variant_operator_eq(purc_variant_t v1, purc_variant_t v2)
{
    return purc_variant_make_boolean(variant_compare(v1, v2) == 0);
}

purc_variant_t
purc_variant_operator_ne(purc_variant_t v1, purc_variant_t v2)
{
    return purc_variant_make_boolean(variant_compare(v1, v2) != 0);
}

purc_variant_t
purc_variant_operator_gt(purc_variant_t v1, purc_variant_t v2)
{
    return purc_variant_make_boolean(variant_compare(v1, v2) > 0);
}

purc_variant_t
purc_variant_operator_ge(purc_variant_t v1, purc_variant_t v2)
{
    return purc_variant_make_boolean(variant_compare(v1, v2) >= 0);
}

purc_variant_t
purc_variant_operator_not(purc_variant_t v)
{
    bool truth = purc_variant_booleanize(v);
    return purc_variant_make_boolean(!truth);
}

purc_variant_t
purc_variant_operator_truth(purc_variant_t v)
{
    bool truth = purc_variant_booleanize(v);
    return purc_variant_make_boolean(truth);
}

purc_variant_t
purc_variant_operator_is(purc_variant_t v1, purc_variant_t v2)
{
    bool res;

    if (v1 == v2)
        res = true;
    else if (v1->type != v2->type)
        res = false;
    else if (is_variant_scalar(v1))
        res = v1->u64 == v2->u64;
    else
        res = false;

    return purc_variant_make_boolean(res);
}

purc_variant_t
purc_variant_operator_is_not(purc_variant_t v1, purc_variant_t v2)
{
    bool res;

    if (v1 == v2)
        res = true;
    else if (v1->type != v2->type)
        res = false;
    else if (is_variant_scalar(v1))
        res = v1->u64 == v2->u64;
    else
        res = false;

    return purc_variant_make_boolean(!res);
}

purc_variant_t
purc_variant_operator_abs(purc_variant_t v)
{
    if (v->type == PURC_VARIANT_TYPE_LONGINT) {
        return purc_variant_make_longint(llabs(v->i64));
    }
    else if (v->type == PURC_VARIANT_TYPE_ULONGINT) {
        return purc_variant_make_ulongint(v->u64);
    }
    else if (v->type == PURC_VARIANT_TYPE_BIGINT) {
        return bigint_abs(v);
    }
    else {
        double f64 = purc_variant_numerify(v);
        return purc_variant_make_number(fabs(f64));
    }
}

purc_variant_t
purc_variant_operator_neg(purc_variant_t v)
{
    if (v->type == PURC_VARIANT_TYPE_LONGINT) {
        return purc_variant_make_longint(-v->i64);
    }
    else if (v->type == PURC_VARIANT_TYPE_ULONGINT) {
        if (v->u64 < INT64_MAX) {
            int64_t i64 = (int64_t)v->u64;
            return purc_variant_make_longint(-i64);
        }

        bigint_buf a_buf;
        purc_variant_t a = bigint_set_u64(&a_buf, v->u64);
        return bigint_neg(a);
    }
    else if (v->type == PURC_VARIANT_TYPE_BIGINT) {
        return bigint_neg(v);
    }
    else {
        double f64 = purc_variant_numerify(v);
        return purc_variant_make_number(-f64);
    }

}

purc_variant_t
purc_variant_operator_pos(purc_variant_t v)
{
    if (v->type == PURC_VARIANT_TYPE_LONGINT) {
        return purc_variant_make_longint(v->i64);
    }
    else if (v->type == PURC_VARIANT_TYPE_ULONGINT) {
        return purc_variant_make_ulongint(v->u64);
    }
    else if (v->type == PURC_VARIANT_TYPE_BIGINT) {
        return bigint_clone(v);
    }
    else {
        double f64 = purc_variant_numerify(v);
        return purc_variant_make_number(f64);
    }
}

static uint64_t
binary_lifting_power(uint64_t base, uint64_t exponent, bool *overflow)
{
    uint64_t res = 1;
    *overflow = false;

    while (exponent) {
        if (exponent & 1) {
            res *= base;

            if (res < base) {
                *overflow = true;
                break;
            }
        }
        uint64_t new_base = base * base;
        if (new_base < base) {
            *overflow = true;
            break;
        }

        base = new_base;
        exponent >>= 1;
    }

    return res;
}

static int64_t
binary_lifting_power_sbase(int64_t base, uint64_t exponent, bool *overflow)
{
    uint64_t res;
    int64_t ans = 1;

    if (base >= 0) {
        res = binary_lifting_power(base, exponent, overflow);
        if ((!*overflow) && res > INT64_MAX) {
            *overflow = true;
        }
        else if (!*overflow) {
            ans = res;
        }
    }
    else {
        res = binary_lifting_power(-base, exponent, overflow);
        if ((!*overflow) && res > INT64_MAX) {
            *overflow = true;
        }
        else if (!*overflow) {
            if (exponent & 0x01)
                ans = -res;
            else
                ans = res;
        }
    }

    return ans;
}

#if !HAVE(INT128_T)
#   error Unsupported
#endif

static purc_variant_t
variant_arithmetic_op_as_bigint(purc_variant_t v1, purc_variant_t v2,
        purc_variant_operator op)
{
    bigint_buf a_buf, b_buf;
    purc_variant_t a, b;
    if (v1->type == PURC_VARIANT_TYPE_ULONGINT)
        a = bigint_set_u64(&a_buf, v1->u64);
    else
        a = bigint_set_i64(&a_buf, v1->i64);

    if (v2->type == PURC_VARIANT_TYPE_ULONGINT)
        b = bigint_set_u64(&b_buf, v2->u64);
    else
        b = bigint_set_i64(&b_buf, v2->i64);

    purc_variant_t res = PURC_VARIANT_INVALID;

    switch (op) {
    case OP_add:
        res = bigint_add(a, b, 0);
        break;

    case OP_sub:
        res = bigint_add(a, b, 1);
        break;

    case OP_mul:
        res = bigint_mul(a, b);
        break;

    case OP_floordiv:
        res = bigint_divrem(a, b, false);
        break;

    case OP_truediv:
        assert(0);
        break;

    case OP_mod:
        res = bigint_divrem(a, b, true);
        break;

    case OP_pow:
        assert(bigint_sign(b) == 0);
        res = bigint_pow(a, b);
        break;

    default:
        assert(0);
        break;
    }

    return res;
}

static purc_variant_t
variant_arithmetic_op(purc_variant_t v1, purc_variant_t v2,
        purc_variant_operator op)
{
    int ec = PURC_ERROR_OK;
    purc_variant_t res = PURC_VARIANT_INVALID;

    if (!IS_NUMBER(v1->type) || !IS_NUMBER(v2->type)) {
        ec = PURC_ERROR_INVALID_OPERAND;
        goto done;
    }

    if (v1->type == PURC_VARIANT_TYPE_LONGDOUBLE ||
            v2->type == PURC_VARIANT_TYPE_LONGDOUBLE) {
        long double a, b, c;
        if (v1->type == PURC_VARIANT_TYPE_LONGDOUBLE) {
            a = *v1->ld;
            if (!purc_variant_cast_to_longdouble(v2, &b, false)) {
                ec = PURC_ERROR_INVALID_OPERAND;
                goto done;
            }
        }
        else {
            if (!purc_variant_cast_to_longdouble(v1, &a, false)) {
                ec = PURC_ERROR_INVALID_OPERAND;
                goto done;
            }
            b = *v2->ld;
        }

        switch (op) {
        case OP_add:
            c = a + b;
            break;

        case OP_sub:
            c = a - b;
            break;

        case OP_mul:
            c = a * b;
            break;

        case OP_floordiv:
            c = floorl(a / b);
            break;

        case OP_truediv:
            c = a / b;
            break;

        case OP_mod:
            c = fmodl(a, b);
            break;

        case OP_pow:
            c = powl(a, b);
            break;

        default:
            assert(0);
            break;
        }

        res = purc_variant_make_longdouble(c);
    }
    else if (v1->type == PURC_VARIANT_TYPE_NUMBER ||
            v2->type == PURC_VARIANT_TYPE_NUMBER) {
        double a, b, c;
        if (v1->type == PURC_VARIANT_TYPE_NUMBER) {
            a = v1->d;
            if (!purc_variant_cast_to_number(v2, &b, false)) {
                ec = PURC_ERROR_INVALID_OPERAND;
                goto done;
            }
        }
        else {
            if (!purc_variant_cast_to_number(v1, &a, false)) {
                ec = PURC_ERROR_INVALID_OPERAND;
                goto done;
            }
            b = v2->d;
        }

        switch (op) {
        case OP_add:
            c = a + b;
            break;

        case OP_sub:
            c = a - b;
            break;

        case OP_mul:
            c = a * b;
            break;

        case OP_floordiv:
            c = floor(a / b);
            break;

        case OP_truediv:
            c = a / b;
            break;

        case OP_mod:
            c = fmod(a, b);
            break;

        case OP_pow:
            c = pow(a, b);
            break;

        default:
            assert(0);
            break;
        }

        res = purc_variant_make_number(c);
    }
    else if (v1->type == PURC_VARIANT_TYPE_BIGINT ||
            v2->type == PURC_VARIANT_TYPE_BIGINT) {
        purc_variant_t a, b;
        if (v1->type == PURC_VARIANT_TYPE_BIGINT) {
            a = v1;
            b = v2;
        }
        else {
            a = v2;
            b = v1;
        }

        bigint_buf buf;
        switch (b->type) {
        case PURC_VARIANT_TYPE_BIGINT:
            break;

        case PURC_VARIANT_TYPE_LONGINT:
            b = bigint_set_i64(&buf, b->i64);
            break;

        case PURC_VARIANT_TYPE_ULONGINT:
            b = bigint_set_u64(&buf, b->u64);
            break;

        default:
            assert(0);  // never be here.
            break;
        }

        if (b) {
            switch (op) {
            case OP_add:
                res = bigint_add(a, b, 0);
                break;

            case OP_sub:
                if (a == v1)
                    res = bigint_add(a, b, 1);
                else
                    res = bigint_add(b, a, 1);
                break;

            case OP_mul:
                res = bigint_mul(a, b);
                break;

            case OP_floordiv:
                if (a == v1)
                    res = bigint_divrem(a, b, false);
                else
                    res = bigint_divrem(b, a, false);
                break;

            case OP_truediv: {
                double f_a = bigint_to_float64(a);
                double f_b = bigint_to_float64(b);
                double f_c;
                if (a == v1)
                    f_c = f_a / f_b;
                else
                    f_c = f_b / f_a;
                res = purc_variant_make_number(f_c);
                break;
            }

            case OP_mod:
                if (a == v1)
                    res = bigint_divrem(a, b, true);
                else
                    res = bigint_divrem(b, a, true);
                break;

            case OP_pow:
                if (a == v1) {
                    if (bigint_sign(b)) {
                        double base = bigint_to_float64(a);
                        double exp = bigint_to_float64(b);
                        res = purc_variant_make_number(pow(base, exp));
                    }
                    else {
                        res = bigint_pow(a, b);
                    }
                }
                else {
                    if (bigint_sign(a)) {
                        double base = bigint_to_float64(b);
                        double exp = bigint_to_float64(a);
                        res = purc_variant_make_number(pow(base, exp));
                    }
                    else {
                        res = bigint_pow(b, a);
                    }
                }
                break;

            default:
                assert(0);
                break;
            }

            if (b != (void *)&buf && b != v1 && b != v2)
                pcvariant_put(b);
        }
    }
    else if (v1->type == PURC_VARIANT_TYPE_ULONGINT ||
            v2->type == PURC_VARIANT_TYPE_ULONGINT) {
        int128_t a, b, c;
        if (v1->type == PURC_VARIANT_TYPE_ULONGINT)
            a = v1->u64;
        else
            a = v1->i64;

        if (v2->type == PURC_VARIANT_TYPE_ULONGINT)
            b = v2->u64;
        else
            b = v2->i64;

        double d = 0;
        bool use_float = false, overflow = false;

        switch (op) {
        case OP_add:
            c = a + b;
            if (c < INT64_MAX || c > UINT64_MAX)
                overflow = true;
            break;

        case OP_sub:
            c = a - b;
            if (c < INT64_MAX || c > UINT64_MAX)
                overflow = true;
            break;

        case OP_mul:
            c = a * b;
            if (a && (c / a != b))
                overflow = true;
            break;

        case OP_floordiv:
            if (b == 0) {
                ec = PURC_ERROR_DIVBYZERO;
                goto done;
            }
            c = a / b;
            break;

        case OP_truediv:
            if (b == 0) {
                ec = PURC_ERROR_DIVBYZERO;
                goto done;
            }
            d = 1.0 * a / b;
            use_float = true;
            break;

        case OP_mod:
            if (b == 0) {
                ec = PURC_ERROR_DIVBYZERO;
                goto done;
            }
            c = a % b;
            break;

        case OP_pow: {
            if (v2->i64 == 0) {
                c = 1;
            }
            else if (v2->type == PURC_VARIANT_TYPE_LONGINT && v2->i64 < 0) {
                double base;
                if (v1->type == PURC_VARIANT_TYPE_ULONGINT)
                    base = v1->u64;
                else
                    base = v1->i64;

                d = pow(base, v2->i64);
                use_float = true;
            }
            else {
                if (v1->type == PURC_VARIANT_TYPE_ULONGINT)
                    c = binary_lifting_power(v1->u64, (uint64_t)v2->i64,
                            &overflow);
                else
                    c = binary_lifting_power_sbase(v1->i64, (uint64_t)v2->i64,
                            &overflow);
            }
            break;
        }

        default:
            assert(0);
            break;
        }

        if (use_float)
            res = purc_variant_make_number(d);
        else if (overflow)
            res = variant_arithmetic_op_as_bigint(v1, v2, op);
        else if (c < 0)
            res = purc_variant_make_longint((int64_t)c);
        else
            res = purc_variant_make_ulongint((uint64_t)c);
    }
    else {
        assert(v1->type == PURC_VARIANT_TYPE_LONGINT &&
            v2->type == PURC_VARIANT_TYPE_LONGINT);

        int128_t a = v1->i64, b = v2->i64, c = 0;
        double d = 0;
        bool use_float = false, overflow = false;

        switch (op) {
        case OP_add:
            c = a + b;
            if (c < INT64_MAX || c > INT64_MAX)
                overflow = true;
            break;

        case OP_sub:
            c = a - b;
            if (c < INT64_MAX || c > INT64_MAX)
                overflow = true;
            break;

        case OP_mul:
            c = a * b;
            if (a && (c / a != b)) {
                overflow = true;
            }
            break;

        case OP_floordiv:
            if (b == 0) {
                ec = PURC_ERROR_DIVBYZERO;
                goto done;
            }
            c = a / b;
            break;

        case OP_truediv:
            if (b == 0) {
                ec = PURC_ERROR_DIVBYZERO;
                goto done;
            }
            d = 1.0 * a / b;
            use_float = true;
            break;

        case OP_mod:
            if (b == 0) {
                ec = PURC_ERROR_DIVBYZERO;
                goto done;
            }
            c = a % b;
            break;

        case OP_pow: {
            if (b == 0) {
                c = 1;
            }
            else if (b < 0) {
                d = pow(v1->i64, v2->i64);
                use_float = true;
            }
            else {
                c = binary_lifting_power_sbase(v2->i64, (uint64_t)v2->i64,
                        &overflow);
            }
            break;
        }

        default:
            assert(0);
            break;
        }

        if (use_float)
            res = purc_variant_make_number(d);
        else if (overflow)
            res = variant_arithmetic_op_as_bigint(v1, v2, op);
        else
            res = purc_variant_make_longint((int64_t)c);
    }
#if 0
    else {
        /* for any other situations */
        double a, b, c = 0;
        a = purc_variant_numerify(v1);
        b = purc_variant_numerify(v2);

        switch (op) {
        case OP_add:
            c = a + b;
            break;

        case OP_sub:
            c = a - b;
            break;

        case OP_mul:
            c = a * b;
            break;

        case OP_floordiv:
            c = floor(a / b);
            break;

        case OP_truediv:
            c = a / b;
            break;

        case OP_mod:
            c = fmod(a, b);
            break;

        case OP_pow:
            c = pow(a, b);
            break;

        default:
            assert(0);
            break;
        }

        res = purc_variant_make_number(c);
    }
#endif

done:
    if (ec)
        pcinst_set_error(ec);
    return res;
}

purc_variant_t
purc_variant_operator_add(purc_variant_t v1, purc_variant_t v2)
{
    return variant_arithmetic_op(v1, v2, OP_add);
}

purc_variant_t
purc_variant_operator_sub(purc_variant_t v1, purc_variant_t v2)
{
    return variant_arithmetic_op(v1, v2, OP_sub);
}

purc_variant_t
purc_variant_operator_mul(purc_variant_t v1, purc_variant_t v2)
{
    return variant_arithmetic_op(v1, v2, OP_mul);
}

purc_variant_t
purc_variant_operator_truediv(purc_variant_t v1, purc_variant_t v2)
{
    return variant_arithmetic_op(v1, v2, OP_truediv);
}

purc_variant_t
purc_variant_operator_floordiv(purc_variant_t v1, purc_variant_t v2)
{
    return variant_arithmetic_op(v1, v2, OP_floordiv);
}

purc_variant_t
purc_variant_operator_mod(purc_variant_t v1, purc_variant_t v2)
{
    return variant_arithmetic_op(v1, v2, OP_mod);
}

purc_variant_t
purc_variant_operator_pow(purc_variant_t v1, purc_variant_t v2)
{
    return variant_arithmetic_op(v1, v2, OP_pow);
}

purc_variant_t
purc_variant_operator_invert(purc_variant_t v)
{
    purc_variant_t res = PURC_VARIANT_INVALID;

    switch (v->type) {
    case PURC_VARIANT_TYPE_BIGINT:
        res = bigint_not(v);
        break;

    case PURC_VARIANT_TYPE_LONGINT:
        res = purc_variant_make_longint(~v->i64);
        break;

    case PURC_VARIANT_TYPE_ULONGINT:
        res = purc_variant_make_ulongint(~v->u64);
        break;

    default:
        pcinst_set_error(PURC_ERROR_INVALID_OPERAND);
        break;
    }

    return res;
}

static purc_variant_t
variant_bitwise_op(purc_variant_t v1, purc_variant_t v2,
        purc_variant_operator op)
{
    purc_variant_t res = PURC_VARIANT_INVALID;

    if (v1->type == PURC_VARIANT_TYPE_BIGINT ||
            v2->type == PURC_VARIANT_TYPE_BIGINT) {
        purc_variant_t a, b;
        if (v1->type == PURC_VARIANT_TYPE_BIGINT) {
            a = v1;
            b = v2;
        }
        else {
            a = v2;
            b = v1;
        }

        bigint_buf buf;
        switch (b->type) {
        case PURC_VARIANT_TYPE_BIGINT:
            break;

        case PURC_VARIANT_TYPE_LONGINT:
            b = bigint_set_i64(&buf, b->i64);
            break;

        case PURC_VARIANT_TYPE_ULONGINT:
            b = bigint_set_i64(&buf, b->u64);
            break;

        default:
            b = PURC_VARIANT_INVALID;
            pcinst_set_error(PURC_ERROR_INVALID_OPERAND);
            break;
        }

        if (b) {
            res = bigint_logic(a, b, op);
        }
    }
    else if (v1->type == PURC_VARIANT_TYPE_LONGINT &&
            v2->type == PURC_VARIANT_TYPE_LONGINT) {
        int64_t a = v1->i64, b = v2->i64, c;

        switch (op) {
        case OP_and:
            c = a & b;
            break;
        case OP_or:
            c = a | b;
            break;
        case OP_xor:
            c = a ^ b;
            break;
        default:
            assert(0);
            break;
        }

        res = purc_variant_make_longint(c);
    }
    else if ((v1->type == PURC_VARIANT_TYPE_ULONGINT &&
                v2->type == PURC_VARIANT_TYPE_ULONGINT) ||
            (v1->type == PURC_VARIANT_TYPE_ULONGINT &&
                v2->type == PURC_VARIANT_TYPE_LONGINT) ||
            (v1->type == PURC_VARIANT_TYPE_LONGINT &&
                v2->type == PURC_VARIANT_TYPE_ULONGINT)) {
        /* always returns ulongint */
        uint64_t a = v1->u64, b = v2->u64, c;

        switch (op) {
        case OP_and:
            c = a & b;
            break;
        case OP_or:
            c = a | b;
            break;
        case OP_xor:
            c = a ^ b;
            break;
        default:
            assert(0);
            break;
        }

        res = purc_variant_make_ulongint(c);
    }
    else {
        pcinst_set_error(PURC_ERROR_INVALID_OPERAND);
    }

    return res;
}

purc_variant_t
purc_variant_operator_and(purc_variant_t v1, purc_variant_t v2)
{
    return variant_bitwise_op(v1, v2, OP_and);
}

purc_variant_t
purc_variant_operator_or(purc_variant_t v1, purc_variant_t v2)
{
    return variant_bitwise_op(v1, v2, OP_or);
}

purc_variant_t
purc_variant_operator_xor(purc_variant_t v1, purc_variant_t v2)
{
    return variant_bitwise_op(v1, v2, OP_xor);
}

static purc_variant_t
variant_shift_op(purc_variant_t v, purc_variant_t c, bool is_right)
{
    purc_variant_t res = PURC_VARIANT_INVALID;

    uint32_t u32;
    if (!purc_variant_cast_to_uint32(c, &u32, false)) {
        pcinst_set_error(PURC_ERROR_INVALID_OPERAND);
    }
    else if (v->type == PURC_VARIANT_TYPE_BIGINT) {
        if (is_right)
            res = bigint_shr(v, u32);
        else
            res = bigint_shl(v, u32);
    }
    else if (v->type == PURC_VARIANT_TYPE_ULONGINT) {
        uint64_t u64;
        if (is_right) {
            u64 = v->u64 >> u32;
        }
        else {
            u64 = v->u64 << u32;
        }

        res = purc_variant_make_ulongint(u64);
    }
    else if (v->type == PURC_VARIANT_TYPE_LONGINT) {
        int64_t i64;
        if (is_right) {
            i64 = v->i64 >> u32;
        }
        else {
            i64 = v->i64 << u32;
        }

        res = purc_variant_make_longint(i64);
    }
    else {
        pcinst_set_error(PURC_ERROR_INVALID_OPERAND);
    }

    return res;
}

purc_variant_t
purc_variant_operator_lshift(purc_variant_t v, purc_variant_t c)
{
    return variant_shift_op(v, c, false);
}

purc_variant_t
purc_variant_operator_rshift(purc_variant_t v, purc_variant_t c)
{
    return variant_shift_op(v, c, true);
}

int
purc_variant_operator_iadd(purc_variant_t v1, purc_variant_t v2)
{
    purc_variant_t res = variant_arithmetic_op(v1, v2, OP_add);
    if (res) {
        pcvariant_move_scalar(v1, res);
        return 0;
    }

    return -1;
}

int
purc_variant_operator_isub(purc_variant_t v1, purc_variant_t v2)
{
    purc_variant_t res = variant_arithmetic_op(v1, v2, OP_sub);
    if (res) {
        pcvariant_move_scalar(v1, res);
        return 0;
    }

    return -1;
}

int
purc_variant_operator_imul(purc_variant_t v1, purc_variant_t v2)
{
    purc_variant_t res = variant_arithmetic_op(v1, v2, OP_mul);
    if (res) {
        pcvariant_move_scalar(v1, res);
        return 0;
    }

    return -1;
}

int
purc_variant_operator_itruediv(purc_variant_t v1, purc_variant_t v2)
{
    purc_variant_t res = variant_arithmetic_op(v1, v2, OP_truediv);
    if (res) {
        pcvariant_move_scalar(v1, res);
        return 0;
    }

    return -1;
}

int
purc_variant_operator_ifloordiv(purc_variant_t v1, purc_variant_t v2)
{
    purc_variant_t res = variant_arithmetic_op(v1, v2, OP_floordiv);
    if (res) {
        pcvariant_move_scalar(v1, res);
        return 0;
    }

    return -1;
}

int
purc_variant_operator_imod(purc_variant_t v1, purc_variant_t v2)
{
    purc_variant_t res = variant_arithmetic_op(v1, v2, OP_mod);
    if (res) {
        pcvariant_move_scalar(v1, res);
        return 0;
    }

    return -1;
}

int
purc_variant_operator_ipow(purc_variant_t v1, purc_variant_t v2)
{
    purc_variant_t res = variant_arithmetic_op(v1, v2, OP_mod);
    if (res) {
        pcvariant_move_scalar(v1, res);
        return 0;
    }

    return -1;
}

static int
variant_bitwise_iop(purc_variant_t v1, purc_variant_t v2,
        purc_variant_operator op)
{
    int ret = 0;

    if (v1->type == PURC_VARIANT_TYPE_BIGINT ||
            v2->type == PURC_VARIANT_TYPE_BIGINT) {
        purc_variant_t a, b;
        if (v1->type == PURC_VARIANT_TYPE_BIGINT) {
            a = v1;
            b = v2;
        }
        else {
            a = v2;
            b = v1;
        }

        bigint_buf buf;
        switch (b->type) {
        case PURC_VARIANT_TYPE_BIGINT:
            break;

        case PURC_VARIANT_TYPE_LONGINT:
            b = bigint_set_i64(&buf, b->i64);
            break;

        case PURC_VARIANT_TYPE_ULONGINT:
            b = bigint_set_u64(&buf, b->u64);
            break;

        default:
            b = PURC_VARIANT_INVALID;
            pcinst_set_error(PURC_ERROR_INVALID_OPERAND);
            ret = -1;
            break;
        }

        if (b) {
            purc_variant_t res = bigint_logic(a, b, op);
            bigint_move(a, res);
        }
    }
    else if (v1->type == PURC_VARIANT_TYPE_LONGINT &&
            v2->type == PURC_VARIANT_TYPE_LONGINT) {
        switch (op) {
        case OP_and:
            v1->i64 &= v2->i64;
            break;
        case OP_or:
            v1->i64 |= v2->i64;
            break;
        case OP_xor:
            v1->i64 ^= v2->i64;
            break;
        default:
            assert(0);
            break;
        }
    }
    else if ((v1->type == PURC_VARIANT_TYPE_ULONGINT &&
                v2->type == PURC_VARIANT_TYPE_ULONGINT) ||
            (v1->type == PURC_VARIANT_TYPE_ULONGINT &&
                v2->type == PURC_VARIANT_TYPE_LONGINT) ||
            (v1->type == PURC_VARIANT_TYPE_LONGINT &&
                v2->type == PURC_VARIANT_TYPE_ULONGINT)) {
        /* always cast to ulongint */
        v1->type = PURC_VARIANT_TYPE_ULONGINT;
        switch (op) {
        case OP_and:
            v1->u64 &= v2->u64;
            break;
        case OP_or:
            v1->u64 |= v2->u64;
            break;
        case OP_xor:
            v1->u64 ^= v2->u64;
            break;
        default:
            assert(0);
            break;
        }
    }
    else {
        pcinst_set_error(PURC_ERROR_INVALID_OPERAND);
        ret = -1;
    }

    return ret;
}

int
purc_variant_operator_iand(purc_variant_t v1, purc_variant_t v2)
{
    return variant_bitwise_iop(v1, v2, OP_and);
}

int
purc_variant_operator_ior(purc_variant_t v1, purc_variant_t v2)
{
    return variant_bitwise_iop(v1, v2, OP_or);
}

int
purc_variant_operator_ixor(purc_variant_t v1, purc_variant_t v2)
{
    return variant_bitwise_iop(v1, v2, OP_xor);
}

static int
variant_shift_iop(purc_variant_t v, purc_variant_t c, bool is_right)
{
    int res = 0;

    uint32_t u32;
    if (!purc_variant_cast_to_uint32(c, &u32, false)) {
        pcinst_set_error(PURC_ERROR_INVALID_OPERAND);
        res = -1;
    }
    else if (v->type == PURC_VARIANT_TYPE_BIGINT) {
        purc_variant_t tmp;
        if (is_right)
            tmp = bigint_shr(v, u32);
        else
            tmp = bigint_shl(v, u32);
        bigint_move(v, tmp);
    }
    else if (v->type == PURC_VARIANT_TYPE_ULONGINT) {
        if (is_right) {
            v->u64 >>= u32;
        }
        else {
            v->u64 <<= u32;
        }
    }
    else if (v->type == PURC_VARIANT_TYPE_LONGINT) {
        if (is_right) {
            v->i64 >>= u32;
        }
        else {
            v->i64 <<= u32;
        }
    }
    else {
        pcinst_set_error(PURC_ERROR_INVALID_OPERAND);
        res = -1;
    }

    return res;
}

int
purc_variant_operator_ilshift(purc_variant_t v, purc_variant_t c)
{
    return variant_shift_iop(v, c, false);
}

int
purc_variant_operator_irshift(purc_variant_t v, purc_variant_t c)
{
    return variant_shift_iop(v, c, true);
}

purc_variant_t
purc_variant_operator_concat(purc_variant_t a, purc_variant_t b)
{
    purc_variant_t res = PURC_VARIANT_INVALID;
    int ec = PURC_ERROR_OK;

    const char *str_a, *str_b;
    size_t len_a, len_b;
    str_a = purc_variant_get_string_const_ex(a, &len_a);
    str_b = purc_variant_get_string_const_ex(b, &len_b);

    if (str_a && str_b) {
        size_t sz_buf = len_a + len_b + 1;
        char *buf = malloc(sz_buf);
        if (buf == NULL) {
            ec = PURC_ERROR_OUT_OF_MEMORY;
            goto failed;
        }
        memcpy(buf, str_a, len_a);
        memcpy(buf + len_a, str_b, len_b);
        buf[len_a + len_b] = 0;
        res = purc_variant_make_string_reuse_buff(buf, sz_buf, false);
    }
    else {
        const unsigned char *bytes_a, *bytes_b;
        bytes_a = purc_variant_get_bytes_const(a, &len_a);
        bytes_b = purc_variant_get_bytes_const(b, &len_b);
        if (bytes_a && bytes_b) {
            size_t sz_buf = len_a + len_b;
            char *buf = malloc(sz_buf);
            if (buf == NULL) {
                ec = PURC_ERROR_OUT_OF_MEMORY;
                goto failed;
            }
            memcpy(buf, bytes_a, len_a);
            memcpy(buf + len_a, bytes_b, len_b);
            res = purc_variant_make_byte_sequence_reuse_buff(
                    buf, sz_buf, sz_buf);
        }
        else {
            ec = PURC_ERROR_INVALID_OPERAND;
        }
    }

failed:
    if (ec)
        pcinst_set_error(ec);
    return res;
}

int
purc_variant_operator_iconcat(purc_variant_t a, purc_variant_t b)
{
    if (!IS_SEQUENCE(a->type)) {
        pcinst_set_error(PURC_ERROR_INVALID_OPERAND);
        return -1;
    }

    purc_variant_t res = purc_variant_operator_concat(a, b);
    if (res) {
        pcvariant_move_sequence(a, res);
        return 0;
    }

    return -1;
}

#if 0
purc_variant_t
purc_variant_operator_index(purc_variant_t v)
{
}

purc_variant_t
purc_variant_operator_contains(purc_variant_t a, purc_variant_t b)
{
}

purc_variant_t
purc_variant_operator_getitem(purc_variant_t a, purc_variant_t b)
{
}

purc_variant_t
purc_variant_operator_setitem(purc_variant_t a, purc_variant_t b,
        purc_variant_t c)
{
}

purc_variant_t
purc_variant_operator_delitem(purc_variant_t a, purc_variant_t b)
{
}
#endif
