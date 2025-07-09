/*
 * @file serializer.c
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
            cmp = bigint_float64_cmp(a, (double)b->ld);
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
            a = v1->ld;
            b = purc_variant_numerify(v2);
        }
        else {
            a = purc_variant_numerify(v1);
            b = v2->ld;
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
    else if (is_variant_ordinary(v1))
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
    else if (is_variant_ordinary(v1))
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

        purc_variant_t r = purc_variant_make_bigint_from_u64(v->u64);
        if (r) {
            purc_variant_t r1 = bigint_neg(r);
            purc_variant_unref(r);
            r = r1;
        }
        return r;
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

#if 0
purc_variant_t
purc_variant_operator_add(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_sub(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_mul(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_truediv(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_floordiv(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_mod(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_pow(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_invert(purc_variant_t v)
{
}

purc_variant_t
purc_variant_operator_and(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_or(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_xor(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_lshift(purc_variant_t v, purc_variant_t c)
{
}

purc_variant_t
purc_variant_operator_rshift(purc_variant_t v, purc_variant_t c)
{
}

purc_variant_t
purc_variant_operator_index(purc_variant_t v)
{
}

purc_variant_t
purc_variant_operator_concat(purc_variant_t a, purc_variant_t b)
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

purc_variant_t
purc_variant_operator_iadd(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_isub(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_imul(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_itruediv(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_ifloordiv(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_imod(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_ipow(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_iand(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_ior(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_ixor(purc_variant_t v1, purc_variant_t v2)
{
}

purc_variant_t
purc_variant_operator_ilshift(purc_variant_t v, purc_variant_t c)
{
}

purc_variant_t
purc_variant_operator_irshift(purc_variant_t v, purc_variant_t c)
{
}

purc_variant_t
purc_variant_operator_iconcat(purc_variant_t a, purc_variant_t b)
{
}
#endif
