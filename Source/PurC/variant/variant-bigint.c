/**
 * @file variant-bigint.cpp
 * @author Vincent Wei
 * @date 2025/07/02
 * @brief The implementation of BigInt variant (copied from QuickJS).
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

#include "variant-internals.h"

#include "private/instance.h"

#include "purc-errors.h"
#include "purc-utils.h"

#if BIGINT_LIMB_BITS == 32

#define TAB                 dwords
#define NR_LIMBS_IN_WRAPPER 2
#define BIGINT_LIMB_DIGITS  9

#else

#define TAB                 qwords
#define NR_LIMBS_IN_WRAPPER 1
#define BIGINT_LIMB_DIGITS  19

#endif

#define BIGINT_MAX_SIZE ((1024 * 1024) / BIGINT_LIMB_BITS) /* in limbs */

struct bigint_limbs {
    size_t len;
    bi_limb_t tab[];
};

static purc_variant *bigint_set_si(bigint_buf *buf, bi_slimb_t a)
{
    purc_variant *r = (purc_variant *)buf;
    r->type = PURC_VARIANT_TYPE_BIGINT;
    r->size = 1;
    r->flags = 0;   /* do not use extra size */
    r->refc = 0;    /* fail safe */
    r->TAB[0] = a;
    return r;
}

purc_variant *bigint_set_i64(bigint_buf *buf, int64_t a)
{
#if BIGINT_LIMB_BITS == 64
    return bigint_set_si(buf, a);
#else
    purc_variant *r = (purc_variant *)buf;
    r->type = PURC_VARIANT_TYPE_BIGINT;
    r->flags = PCVRNT_FLAG_STATIC_DATA;
    r->refc = 0;    /* fail safe */
    if (a >= INT32_MIN && a <= INT32_MAX) {
        r->size = 1;
        r->TAB[0] = a;
    } else {
        r->size = 2;
        r->TAB[0] = a;
        r->TAB[1] = a >> BIGINT_LIMB_BITS;
    }
    return r;
#endif
}

purc_variant *bigint_set_u64(bigint_buf *buf, uint64_t a)
{
    if (a <= INT64_MAX) {
        return bigint_set_i64(buf, a);
    } else {
        purc_variant *r = (purc_variant *)buf;
        r->type = PCVRNT_FLAG_STATIC_DATA;
        r->flags = 0;   /* do not use extra size */
        r->refc = 0;    /* fail safe */

#if BIGINT_LIMB_BITS == 64
        r->size = 2;
        r->TAB[0] = a;
        r->TAB[1] = 0;
#else
        r->TAB[0] = a;
        r->TAB[1] = a >> 32;
        r->TAB[2] = 0;
        r->size = 3;
#endif
        return r;
    }
}

static void bigint_set_len(purc_variant *val, size_t len)
{
    assert(val->type == PURC_VARIANT_TYPE_BIGINT);
    if (val->flags & PCVRNT_FLAG_EXTRA_SIZE) {
        struct bigint_limbs *limbs = val->ptr;
        assert(len <= limbs->len);
        limbs->len = len;
    }
    else {
        assert(len <= val->size);
        val->size = len;
    }
}

static size_t bigint_get_len(const purc_variant *val)
{
    assert(val->type == PURC_VARIANT_TYPE_BIGINT);
    if (val->flags & PCVRNT_FLAG_EXTRA_SIZE) {
        struct bigint_limbs *limbs = val->ptr;
        return limbs->len;
    }
    else {
        return val->size;
    }
}

static bi_limb_t *bigint_get_tab(purc_variant *val, size_t *len)
{
    assert(val->type == PURC_VARIANT_TYPE_BIGINT);
    if (val->flags & PCVRNT_FLAG_EXTRA_SIZE) {
        struct bigint_limbs *limbs = val->ptr;
        if (len)
            *len = limbs->len;
        return limbs->tab;
    }
    else {
        if (len)
            *len = val->size;
        return val->TAB;
    }
}

static const bi_limb_t *
bigint_get_tab_const(const purc_variant *val, size_t *len)
{
    assert(val->type == PURC_VARIANT_TYPE_BIGINT);
    if (val->flags & PCVRNT_FLAG_EXTRA_SIZE) {
        struct bigint_limbs *limbs = val->ptr;
        if (len)
            *len = limbs->len;
        return limbs->tab;
    }
    else {
        if (len)
            *len = val->size;
        return val->TAB;
    }
}

static void
bigint_dump1(FILE *fp, const char *prefix, const bi_limb_t *tab, size_t len)
{
    size_t i;
    fprintf(fp, "%s: ", prefix);
    for(i = len - 1; i >= 0; i--) {
#if BIGINT_LIMB_BITS == 32
        fprintf(fp, " %08x", tab[i]);
#else
        fprintf(fp, " %016" PRIx64, tab[i]);
#endif
    }
    fprintf(fp, "\n");
}

void bigint_dump(FILE *fp, const char *prefix, purc_variant *p)
{
    size_t len;
    bi_limb_t *tab = bigint_get_tab(p, &len);
    bigint_dump1(fp, prefix, tab, len);
}

static purc_variant_t bigint_new(int nr_limbs)
{
    purc_variant_t v = pcvariant_get(PVT(_BIGINT));
    if (!v) {
        goto failed;
    }

    v->type = PVT(_BIGINT);
    v->len = nr_limbs;
    if (nr_limbs <= NR_LIMBS_IN_WRAPPER) {
        v->size = nr_limbs;
        v->flags = 0;
    }
    else {
        size_t sz_extra = sizeof(struct bigint_limbs);
        sz_extra += sizeof(bi_limb_t) * nr_limbs;
        struct bigint_limbs *limbs = malloc(sz_extra);
        if (limbs == NULL)
            goto failed;
        limbs->len = nr_limbs;

        v->size = 0;
        v->flags = PCVRNT_FLAG_EXTRA_SIZE;
        v->ptr = limbs;
        pcvariant_stat_inc_extra_size(v, sz_extra);
    }

    v->refc = 1;
    return v;

failed:
    if (v)
        pcvariant_put(v);
    pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
    return PURC_VARIANT_INVALID;
}

void pcvariant_bigint_release(purc_variant_t v)
{
    if (v->flags & PCVRNT_FLAG_EXTRA_SIZE) {
        assert(v->ptr);
        struct bigint_limbs *limbs = v->ptr;
        size_t sz_extra = sizeof(struct bigint_limbs);
        sz_extra += sizeof(bi_limb_t) * limbs->len;
        pcvariant_stat_dec_extra_size(v, sz_extra);
        free(v->ptr);
    }
}

static void bigint_free(purc_variant_t v)
{
    pcvariant_put(v);
}

size_t bigint_extra_size(const struct purc_variant *v)
{
    size_t sz_extra;

    if (v->flags & PCVRNT_FLAG_EXTRA_SIZE) {
        struct bigint_limbs *limbs = v->ptr;

        sz_extra = sizeof(struct bigint_limbs);
        sz_extra += sizeof(bi_limb_t) * limbs->len;
    }
    else {
        sz_extra = 0;
    }

    return sz_extra;
}

size_t bigint_clone_limbs(struct purc_variant *wrapper,
        const struct purc_variant *from)
{
    size_t sz_extra = bigint_extra_size(from);
    if (sz_extra) {
        wrapper->ptr = malloc(sz_extra);
        memcpy(wrapper->ptr, from->ptr, sz_extra);
        wrapper->flags = PCVRNT_FLAG_EXTRA_SIZE;
        wrapper->size = 0;
    }
    else {
        wrapper->ptr = from->ptr;
    }

    return sz_extra;
}

purc_variant_t bigint_clone(const struct purc_variant *a)
{
    size_t a_len;
    const bi_limb_t *a_tab = bigint_get_tab_const(a, &a_len);

    purc_variant_t b = bigint_new(a_len);
    if (b) {
        bi_limb_t *b_tab = bigint_get_tab(b, NULL);
        memcpy(b_tab, a_tab, sizeof(bi_limb_t) * a_len);
    }

    return b;
};

static purc_variant *bigint_new_si(bi_slimb_t a)
{
    purc_variant *r;
    r = bigint_new(1);
    if (!r)
        return NULL;

    r->TAB[0] = a;
    return r;
}

static purc_variant *bigint_new_di(bi_sdlimb_t a)
{
    purc_variant *r;
    if (a == (bi_slimb_t)a) {
        r = bigint_new(1);
        if (!r)
            return NULL;
        bi_limb_t *tab = bigint_get_tab(r, NULL);
        tab[0] = a;
    } else {
        r = bigint_new(2);
        if (!r)
            return NULL;
        bi_limb_t *tab = bigint_get_tab(r, NULL);
        tab[0] = a;
        tab[1] = a >> BIGINT_LIMB_BITS;
    }
    return r;
}

void bigint_move(purc_variant_t to, purc_variant_t from)
{
    assert(from->type == PURC_VARIANT_TYPE_BIGINT &&
            to->type == PURC_VARIANT_TYPE_BIGINT);

    if (to->flags & PCVRNT_FLAG_EXTRA_SIZE)
        free(to->ptr);

    to->size = from->size;
    if (from->flags & PCVRNT_FLAG_EXTRA_SIZE) {
        assert(from->ptr);
        to->ptr = from->ptr;
        to->flags = from->flags;
        from->flags = 0;
    }
    else {
        to->u64 = from->u64;    /* limbs are in wrapper */
    }

    pcvariant_put(from);
}

/* Remove redundant high order limbs. Warning: 'a' may be
   reallocated. Can never fail.
*/
static purc_variant *bigint_normalize1(purc_variant *a, size_t l)
{
    bi_limb_t v;

    assert(a->refc == 1);

    size_t len;
    bi_limb_t *tab = bigint_get_tab(a, &len);
    while (l > 1) {
        v = tab[l - 1];
        if ((v != 0 && v != (bi_limb_t)-1) ||
                (v & 1) != (tab[l - 2] >> (BIGINT_LIMB_BITS - 1))) {
            break;
        }
        l--;
    }

    if (l < len) {
        if (a->flags & PCVRNT_FLAG_EXTRA_SIZE) {
            /* realloc to reduce the size */
            size_t sz_old_extra = sizeof(struct bigint_limbs);
            struct bigint_limbs *limbs = a->ptr;
            sz_old_extra += sizeof(bi_limb_t) * limbs->len;

            size_t sz_extra = sizeof(struct bigint_limbs);
            sz_extra += sizeof(bi_limb_t) * l;
            a->ptr = realloc(a->ptr, sz_extra);

            pcvariant_stat_dec_extra_size(a, sz_old_extra);
            pcvariant_stat_inc_extra_size(a, sz_extra);
            limbs->len = l;
        }
        else {
            a->size = l;
        }
    }

    return a;
}

static purc_variant *bigint_normalize(purc_variant *a)
{
    return bigint_normalize1(a, bigint_get_len(a));
}

/* return 0 or 1 depending on the sign */
int bigint_sign(const purc_variant *a)
{
    size_t len;
    const bi_limb_t *tab = bigint_get_tab_const(a, &len);
    return tab[len - 1] >> (BIGINT_LIMB_BITS - 1);
}

int64_t bigint_get_si_sat(const purc_variant *a)
{
    size_t len;
    const bi_limb_t *tab = bigint_get_tab_const(a, &len);
    if (len == 1) {
        return tab[0];
    } else {
#if BIGINT_LIMB_BITS == 32
        if (bigint_sign(a))
            return INT32_MIN;
        else
            return INT32_MAX;
#else
        if (bigint_sign(a))
            return INT64_MIN;
        else
            return INT64_MAX;
#endif
    }
}

/* add the op1 limb */
static purc_variant *bigint_extend(purc_variant *r, bi_limb_t op1)
{
    size_t n2;
    bi_limb_t *tab = bigint_get_tab(r, &n2);

    if ((op1 != 0 && op1 != (bi_limb_t)-1) ||
            (op1 & 1) != tab[n2 - 1] >> (BIGINT_LIMB_BITS - 1)) {

        size_t sz_extra = sizeof(struct bigint_limbs);
        sz_extra += sizeof(bi_limb_t) * (n2 + 1);
        if (r->flags & PCVRNT_FLAG_EXTRA_SIZE) {

            size_t sz_old_extra = sizeof(struct bigint_limbs);
            struct bigint_limbs *limbs = r->ptr;
            sz_old_extra += sizeof(bi_limb_t) * limbs->len;

            r->ptr = realloc(r->ptr, sz_extra);
            if (r->ptr == NULL)
                goto failed;

            limbs->len = n2 + 1;
            limbs->tab[n2] = op1;

            pcvariant_stat_dec_extra_size(r, sz_old_extra);
            pcvariant_stat_inc_extra_size(r, sz_extra);
        }
        else if ((n2 + 1) <= NR_LIMBS_IN_WRAPPER) {
            r->size = n2 + 1;
            r->TAB[n2] = op1;
        }
        else {
            r->ptr = malloc(sz_extra);
            if (r->ptr == NULL)
                goto failed;

            struct bigint_limbs *limbs = r->ptr;
            memcpy(limbs->tab, r->TAB, sizeof(bi_limb_t) * r->size);
            limbs->len = n2 + 1;
            limbs->tab[n2] = op1;

            r->size = 0;
            r->flags = PCVRNT_FLAG_EXTRA_SIZE;
            pcvariant_stat_inc_extra_size(r, sz_extra);
        }
    } else {
        /* otherwise still need to normalize the result */
        r = bigint_normalize(r);
    }

    return r;

failed:
    pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
    bigint_free(r);
    return PURC_VARIANT_INVALID;
}

/* return NULL in case of error. Compute a + b (b_neg = 0) or a - b
   (b_neg = 1) */
/* XXX: optimize */
purc_variant *bigint_add(const purc_variant *a,
        const purc_variant *b, int b_neg)
{
    purc_variant *r;
    const bi_limb_t *a_tab, *b_tab;
    size_t a_len, b_len;
    a_tab = bigint_get_tab_const(a, &a_len);
    b_tab = bigint_get_tab_const(b, &b_len);

    size_t n1, n2, i;
    bi_limb_t carry, op1, op2, a_sign, b_sign;

    n2 = MAX(a_len, b_len);
    n1 = MIN(a_len, b_len);
    r = bigint_new(n2);
    if (!r)
        return NULL;

    /* XXX: optimize */
    /* common part */
    carry = b_neg;

    bi_limb_t *r_tab = bigint_get_tab(r, NULL);

    for (i = 0; i < n1; i++) {
        op1 = a_tab[i];
        op2 = b_tab[i] ^ (-b_neg);
        ADDC(r_tab[i], carry, op1, op2, carry);
    }
    a_sign = -bigint_sign(a);
    b_sign = (-bigint_sign(b)) ^ (-b_neg);
    /* part with sign extension of one operand  */
    if (a_len > b_len) {
        for(i = n1; i < n2; i++) {
            op1 = a_tab[i];
            ADDC(r_tab[i], carry, op1, b_sign, carry);
        }
    } else if (a_len < b_len) {
        for(i = n1; i < n2; i++) {
            op2 = b_tab[i] ^ (-b_neg);
            ADDC(r_tab[i], carry, a_sign, op2, carry);
        }
    }

    /* part with sign extension for both operands. Extend the result
       if necessary */
    return bigint_extend(r, a_sign + b_sign + carry);
}

/* XXX: optimize */
purc_variant *bigint_neg(const purc_variant *a)
{
    bigint_buf buf;
    purc_variant *b;
    b = bigint_set_si(&buf, 0);
    return bigint_add(b, a, 1);
}

purc_variant *bigint_abs(const purc_variant *a)
{
    if (bigint_sign(a)) {
        return bigint_neg(a);
    }

    return bigint_clone(a);
}

purc_variant *bigint_mul(const purc_variant *a,
        const purc_variant *b)
{
    purc_variant *r;
    const bi_limb_t *a_tab, *b_tab;
    size_t a_len, b_len;
    a_tab = bigint_get_tab_const(a, &a_len);
    b_tab = bigint_get_tab_const(b, &b_len);

    r = bigint_new(a_len + b_len);
    if (!r)
        return NULL;

    bi_limb_t *r_tab = bigint_get_tab(r, NULL);

    mp_mul_basecase(r_tab, a_tab, a_len, b_tab, b_len);
    /* correct the result if negative operands (no overflow is
       possible) */
    if (bigint_sign(a))
        mp_sub(r_tab + a_len, r_tab + a_len, b_tab, b->len, 0);
    if (bigint_sign(b))
        mp_sub(r_tab + b_len, r_tab + b_len, a_tab, a->len, 0);
    return bigint_normalize(r);
}

/* return the division or the remainder. 'b' must be != 0. return NULL
   in case of exception (division by zero or memory error) */
purc_variant *bigint_divrem(const purc_variant *a,
        const purc_variant *b, bool is_rem)
{
    const bi_limb_t *a_tab, *b_tab;
    size_t na, nb;
    a_tab = bigint_get_tab_const(a, &na);
    b_tab = bigint_get_tab_const(b, &nb);

    purc_variant *r, *q;
    bi_limb_t *tabb, h;
    int a_sign, b_sign, shift;

    if (nb == 1 && b_tab[0] == 0) {
        pcinst_set_error(PURC_ERROR_DIVBYZERO);
        return NULL;
    }

    a_sign = bigint_sign(a);
    b_sign = bigint_sign(b);

    r = bigint_new(na + 2);
    if (!r)
        return NULL;
    bi_limb_t *r_tab = bigint_get_tab(r, NULL);

    if (a_sign) {
        mp_neg(r_tab, a_tab, na);
    } else {
        memcpy(r_tab, a_tab, na * sizeof(a_tab[0]));
    }

    /* normalize */
    while (na > 1 && r_tab[na - 1] == 0)
        na--;

    tabb = malloc(nb * sizeof(tabb[0]));
    if (!tabb) {
        bigint_free(r);
        return NULL;
    }

    if (b_sign) {
        mp_neg(tabb, b_tab, nb);
    } else {
        memcpy(tabb, b_tab, nb * sizeof(tabb[0]));
    }

    /* normalize */
    while (nb > 1 && tabb[nb - 1] == 0)
        nb--;

    /* trivial case if 'a' is small */
    if (na < nb) {
        bigint_free(r);
        free(tabb);
        if (is_rem) {
            /* r = a */
            r = bigint_new(na);
            if (!r)
                return NULL;

            bi_limb_t *r_tab = bigint_get_tab(r, NULL);
            memcpy(r_tab, a_tab, na * sizeof(a_tab[0]));
            return r;
        } else {
            /* q = 0 */
            return bigint_new_si(0);
        }
    }

    /* normalize 'b' */
    shift = bi_limb_clz(tabb[nb - 1]);
    if (shift != 0) {
        mp_shl(tabb, tabb, nb, shift);
        h = mp_shl(r_tab, r_tab, na, shift);
        if (h != 0)
            r_tab[na++] = h;
    }

    q = bigint_new(na - nb + 2); /* one more limb for the sign */
    if (!q) {
        bigint_free(r);
        free(tabb);
        return NULL;
    }

    size_t q_len;
    bi_limb_t *q_tab = bigint_get_tab(q, &q_len);

    //    bigint_dump1("a", r->tab, na);
    //    bigint_dump1("b", tabb, nb);
    mp_divnorm(q_tab, r_tab, na, tabb, nb);
    free(tabb);

    if (is_rem) {
        bigint_free(q);
        if (shift != 0)
            mp_shr(r_tab, r_tab, nb, shift, 0);
        r_tab[nb++] = 0;

        if (a_sign)
            mp_neg(r_tab, r_tab, nb);
        r = bigint_normalize1(r, nb);
        return r;
    } else {
        bigint_free(r);
        q_tab[na - nb + 1] = 0;
        if (a_sign ^ b_sign) {
            mp_neg(q_tab, q_tab, q_len);
        }
        q = bigint_normalize(q);
        return q;
    }
}

/* and, or, xor */
purc_variant *bigint_logic(const purc_variant *a,
        const purc_variant *b, purc_variant_operator op)
{
    purc_variant *r;
    bi_limb_t b_sign;
    const bi_limb_t *a_tab, *b_tab;
    size_t a_len, b_len, i;

    a_len = bigint_get_len(a);
    b_len = bigint_get_len(b);
    if (a_len < b_len) {
        const purc_variant *tmp;
        tmp = a;
        a = b;
        b = tmp;
    }
    /* a_len >= b_len */

    a_tab = bigint_get_tab_const(a, &a_len);
    b_tab = bigint_get_tab_const(b, &b_len);

    b_sign = -bigint_sign(b);

    r = bigint_new(a_len);
    if (!r) {
        return NULL;
    }
    bi_limb_t *r_tab = bigint_get_tab(r, NULL);

    switch (op) {
    case OP_or:
        for(i = 0; i < b_len; i++) {
            r_tab[i] = a_tab[i] | b_tab[i];
        }
        for(i = b_len; i < a_len; i++) {
            r_tab[i] = a_tab[i] | b_sign;
        }
        break;

    case OP_and:
        for(i = 0; i < b_len; i++) {
            r_tab[i] = a_tab[i] & b_tab[i];
        }
        for(i = b_len; i < a_len; i++) {
            r_tab[i] = a_tab[i] & b_sign;
        }
        break;

    case OP_xor:
        for(i = 0; i < b_len; i++) {
            r_tab[i] = a_tab[i] ^ b_tab[i];
        }
        for(i = b_len; i < a_len; i++) {
            r_tab[i] = a_tab[i] ^ b_sign;
        }
        break;

    default:
        abort();
    }

    return bigint_normalize(r);
}

purc_variant *bigint_not(const purc_variant *a)
{
    purc_variant *r;
    const bi_limb_t *a_tab;
    size_t a_len, i;

    a_tab = bigint_get_tab_const(a, &a_len);

    r = bigint_new(a_len);
    if (!r)
        return NULL;

    bi_limb_t *r_tab = bigint_get_tab(r, NULL);
    for (i = 0; i < a_len; i++) {
        r_tab[i] = ~a_tab[i];
    }

    /* no normalization is needed */
    return r;
}

purc_variant *bigint_shl(const purc_variant *a, unsigned int shift1)
{
    purc_variant *r;
    bi_limb_t l;

    const bi_limb_t *a_tab;
    size_t a_len, i;
    a_tab = bigint_get_tab_const(a, &a_len);

    if (a_len == 1 && a_tab[0] == 0)
        return bigint_new_si(0); /* zero case */

    unsigned int d, shift;
    d = shift1 / BIGINT_LIMB_BITS;
    shift = shift1 % BIGINT_LIMB_BITS;

    r = bigint_new(a_len + d);
    if (!r)
        return NULL;
    bi_limb_t *r_tab = bigint_get_tab(r, NULL);

    for(i = 0; i < d; i++)
        r_tab[i] = 0;
    if (shift == 0) {
        for(i = 0; i < a_len; i++) {
            r_tab[i + d] = a_tab[i];
        }
    } else {
        l = mp_shl(r_tab + d, a_tab, a_len, shift);
        if (bigint_sign(a))
            l |= (bi_limb_t)(-1) << shift;
        r = bigint_extend(r, l);
    }
    return r;
}

purc_variant *bigint_shr(const purc_variant *a,
                               unsigned int shift1)
{
    unsigned int d, shift;
    int a_sign;
    purc_variant *r;

    const bi_limb_t *a_tab;
    size_t a_len, i, n1;
    a_tab = bigint_get_tab_const(a, &a_len);

    d = shift1 / BIGINT_LIMB_BITS;
    shift = shift1 % BIGINT_LIMB_BITS;
    a_sign = bigint_sign(a);

    if (d >= a_len)
        return bigint_new_si(-a_sign);

    n1 = a_len - d;
    r = bigint_new(n1);
    if (!r)
        return NULL;
    bi_limb_t *r_tab = bigint_get_tab(r, NULL);

    if (shift == 0) {
        for(i = 0; i < n1; i++) {
            r_tab[i] = a_tab[i + d];
        }
        /* no normalization is needed */
    } else {
        mp_shr(r_tab, a_tab + d, n1, shift, -a_sign);
        r = bigint_normalize(r);
    }
    return r;
}

purc_variant *bigint_pow(const purc_variant *a, const purc_variant *b)
{
    uint32_t e;
    int n_bits, i;
    purc_variant *r, *r1;

    /* b must be >= 0 */
    if (bigint_sign(b)) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    const bi_limb_t *a_tab, *b_tab;
    size_t a_len, b_len;
    a_tab = bigint_get_tab_const(a, &a_len);
    b_tab = bigint_get_tab_const(b, &b_len);

    if (b_len == 1 && b_tab[0] == 0) {
        /* a^0 = 1 */
        return bigint_new_si(1);
    } else if (a_len == 1) {
        bi_limb_t v;
        bool is_neg;

        v = a_tab[0];
        if (v <= 1)
            return bigint_new_si(v);
        else if (v == (bi_limb_t)-1)
            return bigint_new_si(1 - 2 * (b_tab[0] & 1));
        is_neg = (bi_slimb_t)v < 0;
        if (is_neg)
            v = -v;
        if ((v & (v - 1)) == 0) {
            uint64_t e1;
            int n;
            /* v = 2^n */
            n = BIGINT_LIMB_BITS - 1 - bi_limb_clz(v);
            if (b_len > 1)
                goto overflow;
            if (b_tab[0] > INT32_MAX)
                goto overflow;
            e = b_tab[0];
            e1 = (uint64_t)e * n;
            if (e1 > BIGINT_MAX_SIZE * BIGINT_LIMB_BITS)
                goto overflow;
            e = e1;
            if (is_neg)
                is_neg = b_tab[0] & 1;

            r = bigint_new(
                    (e + BIGINT_LIMB_BITS + 1 - is_neg) / BIGINT_LIMB_BITS);
            if (!r)
                return NULL;

            size_t r_len;
            bi_limb_t *r_tab = bigint_get_tab(r, &r_len);
            memset(r_tab, 0, sizeof(r_tab[0]) * r_len);
            r_tab[e / BIGINT_LIMB_BITS] =
                (bi_limb_t)(1 - 2 * is_neg) << (e % BIGINT_LIMB_BITS);
            return r;
        }
    }
    if (b_len > 1)
        goto overflow;
    if (b_tab[0] > INT32_MAX)
        goto overflow;
    e = b_tab[0];
    n_bits = 32 - clz32(e);

    r = bigint_new(a_len);
    if (!r)
        return NULL;

    bi_limb_t *r_tab = bigint_get_tab(r, NULL);
    memcpy(r_tab, a_tab, a_len * sizeof(a_tab[0]));
    for(i = n_bits - 2; i >= 0; i--) {
        r1 = bigint_mul(r, r);
        if (!r1)
            return NULL;
        bigint_free(r);
        r = r1;
        if ((e >> i) & 1) {
            r1 = bigint_mul(r, a);
            if (!r1)
                return NULL;
            bigint_free(r);
            r = r1;
        }
    }

    return r;

overflow:
    pcinst_set_error(PURC_ERROR_OVERFLOW);
    return NULL;
}

/* return (mant, exp) so that abs(a) ~ mant*2^(exp - (limb_bits -
   1). a must be != 0. */
static uint64_t bigint_get_mant_exp(int *pexp, const purc_variant *a)
{
    bi_limb_t t[4 - BIGINT_LIMB_BITS / 32], carry, v, low_bits;
    int sgn, shift, e;
    size_t n1, n2, i, j;
    uint64_t a1, a0;

    const bi_limb_t *a_tab;
    size_t a_len;
    a_tab = bigint_get_tab_const(a, &a_len);

    n2 = 4 - BIGINT_LIMB_BITS / 32;
    n1 = a_len - n2;
    sgn = bigint_sign(a);

    /* low_bits != 0 if there are a non zero low bit in abs(a) */
    low_bits = 0;
    carry = sgn;
    for(i = 0; i < n1; i++) {
        v = (a_tab[i] ^ (-sgn)) + carry;
        carry = v < carry;
        low_bits |= v;
    }
    /* get the n2 high limbs of abs(a) */
    for(j = 0; j < n2; j++) {
        i = j + n1;
        if (i < 0) {
            v = 0;
        } else {
            v = (a_tab[i] ^ (-sgn)) + carry;
            carry = v < carry;
        }
        t[j] = v;
    }

#if BIGINT_LIMB_BITS == 32
    a1 = ((uint64_t)t[2] << 32) | t[1];
    a0 = (uint64_t)t[0] << 32;
#else
    a1 = t[1];
    a0 = t[0];
#endif
    a0 |= (low_bits != 0);
    /* normalize */
    if (a1 == 0) {
        /* BIGINT_LIMB_BITS = 64 bit only */
        shift = 64;
        a1 = a0;
        a0 = 0;
    } else {
        shift = clz64(a1);
        if (shift != 0) {
            a1 = (a1 << shift) | (a0 >> (64 - shift));
            a0 <<= shift;
        }
    }
    a1 |= (a0 != 0); /* keep the bits for the final rounding */
    /* compute the exponent */
    e = a_len * BIGINT_LIMB_BITS - shift - 1;
    *pexp = e;
    return a1;
}

/* shift left with round to nearest, ties to even. n >= 1 */
static uint64_t shr_rndn(uint64_t a, int n)
{
    uint64_t addend = ((a >> n) & 1) + ((1 << (n - 1)) - 1);
    return (a + addend) >> n;
}

/* convert to float64 with round to nearest, ties to even. Return
   +/-infinity if too large. */
double bigint_to_float64(const purc_variant *a)
{
    int sgn, e;
    uint64_t mant;

    const bi_limb_t *a_tab;
    size_t a_len;
    a_tab = bigint_get_tab_const(a, &a_len);

    if (a_len == 1) {
        /* fast case, including zero */
        return (double)(bi_slimb_t)a_tab[0];
    }

    sgn = bigint_sign(a);
    mant = bigint_get_mant_exp(&e, a);
    if (e > 1023) {
        /* overflow: return infinity */
        mant = 0;
        e = 1024;
    } else {
        mant = (mant >> 1) | (mant & 1); /* avoid overflow in rounding */
        mant = shr_rndn(mant, 10);
        /* rounding can cause an overflow */
        if (mant >= ((uint64_t)1 << 53)) {
            mant >>= 1;
            e++;
        }
        mant &= (((uint64_t)1 << 52) - 1);
    }
    return uint64_as_float64(((uint64_t)sgn << 63) |
                             ((uint64_t)(e + 1023) << 52) |
                             mant);
}

/* return -1, 0, 1 or (2) (unordered) */
int bigint_float64_cmp(const purc_variant *a, double b)
{
    int b_sign, a_sign, e, f;
    uint64_t mant, b1, a_mant;

    const bi_limb_t *a_tab;
    size_t a_len;
    a_tab = bigint_get_tab_const(a, &a_len);

    b1 = float64_as_uint64(b);
    b_sign = b1 >> 63;
    e = (b1 >> 52) & ((1 << 11) - 1);
    mant = b1 & (((uint64_t)1 << 52) - 1);
    a_sign = bigint_sign(a);
    if (e == 2047) {
        if (mant != 0) {
            /* NaN */
            return 2;
        } else {
            /* +/- infinity */
            return 2 * b_sign - 1;
        }
    } else if (e == 0 && mant == 0) {
        /* b = +/-0 */
        if (a_len == 1 && a_tab[0] == 0)
            return 0;
        else
            return 1 - 2 * a_sign;
    } else if (a_len == 1 && a_tab[0] == 0) {
        /* a = 0, b != 0 */
        return 2 * b_sign - 1;
    } else if (a_sign != b_sign) {
        return 1 - 2 * a_sign;
    } else {
        e -= 1023;
        /* Note: handling denormals is not necessary because we
           compare to integers hence f >= 0 */
        /* compute f so that 2^f <= abs(a) < 2^(f+1) */
        a_mant = bigint_get_mant_exp(&f, a);
        if (f != e) {
            if (f < e)
                return -1;
            else
                return 1;
        } else {
            mant = (mant | ((uint64_t)1 << 52)) << 11; /* align to a_mant */
            if (a_mant < mant)
                return 2 * a_sign - 1;
            else if (a_mant > mant)
                return 1 - 2 * a_sign;
            else
                return 0;
        }
    }
}

/* return -1, 0 or 1 */
int bigint_cmp(const purc_variant *a, const purc_variant *b)
{
    const bi_limb_t *a_tab, *b_tab;
    size_t a_len, b_len;
    a_tab = bigint_get_tab_const(a, &a_len);
    b_tab = bigint_get_tab_const(b, &b_len);

    int a_sign, b_sign, res, i;
    a_sign = bigint_sign(a);
    b_sign = bigint_sign(b);
    if (a_sign != b_sign) {
        res = 1 - 2 * a_sign;
    } else {
        /* we assume the numbers are normalized */
        if (a_len != b_len) {
            if (a_len < b_len)
                res = 2 * a_sign - 1;
            else
                res = 1 - 2 * a_sign;
        } else {
            res = 0;
            for(i = a_len -1; i >= 0; i--) {
                if (a_tab[i] != b_tab[i]) {
                    if (a_tab[i] < b_tab[i])
                        res = -1;
                    else
                        res = 1;
                    break;
                }
            }
        }
    }
    return res;
}

/* return false if overflowed or underflowed */
bool bigint_to_i32(const purc_variant *a, int32_t *ret, bool force)
{
    const bi_limb_t *a_tab;
    size_t a_len;
    a_tab = bigint_get_tab_const(a, &a_len);

#if BIGINT_LIMB_BITS == 32
    if (a_len == 1) {
        /* fast case, including zero */
        *ret = (int32_t)a_tab[0];
        return true;
    }
#else
    if (a_len == 1) {
        int64_t i64 = (int64_t)a_tab[0];
        if (i64 <= INT32_MAX && i64 >= INT32_MIN) {
            *ret = (int32_t)i64;
            return true;
        }
    }
#endif

    if (force) {
        if (bigint_sign(a)) {
            *ret = INT32_MIN;
            return true;
        }
        else {
            *ret = INT32_MAX;
            return true;
        }
    }

    return false;
}

/* return false if overflowed or underflowed */
bool bigint_to_u32(const purc_variant *a, uint32_t *ret, bool force)
{
    if (bigint_sign(a)) {
        if (force) {
            *ret = 0;
            return true;
        }
        return false;
    }

    const bi_limb_t *a_tab;
    size_t a_len;
    a_tab = bigint_get_tab_const(a, &a_len);

#if BIGINT_LIMB_BITS == 32
    if (a_len == 1) {
        /* fast case, including zero */
        *ret = (uint32_t)a_tab[0];
        return true;
    }

    if (a_len == 2 && a_tab[1] == 0) {
        *ret = (uint32_t)a_tab[0];
        return true;
    }
#else
    if (a_len == 1) {
        uint64_t u64 = (uint64_t)a_tab[0];
        if (u64 <= UINT32_MAX) {
            *ret = (uint32_t)u64;
            return true;
        }
    }
#endif

    if (force) {
        *ret = UINT32_MAX;
        return true;
    }

    return false;
}

/* return false if overflowed or underflowed */
bool bigint_to_i64(const purc_variant *a, int64_t *ret, bool force)
{
    const bi_limb_t *a_tab;
    size_t a_len;
    a_tab = bigint_get_tab_const(a, &a_len);

    if (a_len == 1) {
        /* fast case, including zero */
        *ret = (int64_t)(bi_slimb_t)a_tab[0];
        return true;
    }

#if BIGINT_LIMB_BITS == 32
    if (a_len == 2) {
        *ret = r->TAB[1];
        *ret <<= BIGINT_LIMB_BITS;
        *ret |= r->TAB[0];
        return true;
    }
#endif

    if (force) {
        if (bigint_sign(a)) {
            *ret = INT64_MIN;
        }
        else {
            *ret = INT64_MAX;
        }
        return true;
    }

    return false;
}

/* return false if overflowed or underflowed */
bool bigint_to_u64(const purc_variant *a, uint64_t *ret, bool force)
{
    if (bigint_sign(a)) {
        if (force) {
            *ret = 0;
            return true;
        }

        return false;
    }

    const bi_limb_t *a_tab;
    size_t a_len;
    a_tab = bigint_get_tab_const(a, &a_len);

    if (a_len == 1) {
        /* fast case, including zero */
        *ret = (bi_limb_t)a_tab[0];
        return true;
    }

#if BIGINT_LIMB_BITS == 64
    if (a_len == 2 && a_tab[1] == 0) {
        *ret = (bi_limb_t)a_tab[0];
        return true;
    }
#else
    if (a_len == 2 || (a_len == 3 && a_tab[2] == 0)) {
        *ret = r->TAB[1];
        *ret <<= BIGINT_LIMB_BITS;
        *ret |= r->TAB[0];
        return true;
    }
#endif

    if (force) {
        *ret = UINT64_MAX;
        return true;
    }

    return false;
}

int bigint_i64_cmp(const purc_variant *a, int64_t i64)
{
    bigint_buf buf;
    purc_variant *b = bigint_set_i64(&buf, i64);
    return bigint_cmp(a, b);
}

int bigint_u64_cmp(const purc_variant *a, uint64_t u64)
{
    bigint_buf buf;
    purc_variant *b = bigint_set_u64(&buf, u64);
    return bigint_cmp(a, b);
}

/* contains 10^i */
static const bi_limb_t bi_pow_dec[BIGINT_LIMB_DIGITS + 1] = {
    1U,
    10U,
    100U,
    1000U,
    10000U,
    100000U,
    1000000U,
    10000000U,
    100000000U,
    1000000000U,
#if BIGINT_LIMB_BITS == 64
    10000000000U,
    100000000000U,
    1000000000000U,
    10000000000000U,
    100000000000000U,
    1000000000000000U,
    10000000000000000U,
    100000000000000000U,
    1000000000000000000U,
    10000000000000000000U,
#endif
};

static const char *skip_spaces(const char *pc)
{
    const uint8_t *p;
    uint32_t c;

    p = (const uint8_t *)pc;
    while (*p) {
        c = *p;
        if (c < 128) {
            if (!((c >= 0x09 && c <= 0x0d) || (c == 0x20)))
                break;
            p++;
        } else {
            break;
            /* TODO: handle Unicode space
            const uint8_t *next;
            next = pcutils_utf8_next_char(p);
            c = pcutils_utf8_to_unichar(p);
            if (!lre_is_space(c))
                break;
            p = next; */
        }
    }

    return (const char *)p;
}

static inline bool is_power_of_2(int a)
{
    return (a != 0) && ((a & (a - 1)) == 0);
}

/* syntax: [+-][0[xX]]digits in base radix. Return NULL if memory error. radix
   = 0, or >= 2 and <= 36 */
purc_variant_t
purc_variant_make_bigint_from_string(const char *str, char **end, int radix)
{
    const char *p;
    p = skip_spaces(str);

    int is_neg;
    size_t n_digits, n_limbs, len, log2_radix, n_bits, i;
    purc_variant *r;
    bi_limb_t v, c, h;

    is_neg = 0;
    if (*p == '-') {
        is_neg = 1;
        p++;
    }
    else if (*p == '+') {
        p++;
    }

    if (radix == 0) {
        /* determine the radix: 0x/0X -> 16, 0 -> 8 */
        if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
            radix = 16;
        }
        else if (p[0] == '0') {
            radix = 8;
        }
    }

    if (radix > 36 || radix < 2) {
        if (end)
            *end = (char *)p;

        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    while (*p == '0')
        p++;

    if (radix == 16 && (*p == 'x' || *p == 'X'))
        p++;

    n_digits = strlen(p);
    log2_radix = 32 - clz32(radix - 1); /* ceil(log2(radix)) */
    /* compute the maximum number of limbs */
    /* XXX: overflow */
    if (radix == 10) {
        n_bits = n_digits * log2_radix;
    } else {
        n_bits = (n_digits * 27 + 7) / 8; /* >= ceil(n_digits * log2(radix)) */
    }

    /* we add one extra bit for the sign */
    n_limbs = MAX(1, n_bits / BIGINT_LIMB_BITS + 1);
    r = bigint_new(n_limbs);
    if (!r)
        return NULL;

    bi_limb_t *r_tab = bigint_get_tab(r, NULL);

    if (!is_power_of_2(radix)) {
        size_t digits_per_limb = BIGINT_LIMB_DIGITS;
        len = 1;
        r_tab[0] = 0;
        for(;;) {
            /* XXX: slow */
            v = 0;
            for(i = 0; i < digits_per_limb; i++) {
                c = to_digit(*p);
                if (c >= (bi_limb_t)radix) {
                    break;
                }
                p++;
                v = v * radix + c;
            }
            if (i == 0)
                break;
            if (len == 1 && r_tab[0] == 0) {
                r_tab[0] = v;
            } else {
                h = mp_mul1(r_tab, r_tab, len, bi_pow_dec[i], v);
                if (h != 0) {
                    r_tab[len++] = h;
                }
            }
        }
        /* add one extra limb to have the correct sign*/
        if ((r_tab[len - 1] >> (BIGINT_LIMB_BITS - 1)) != 0)
            r_tab[len++] = 0;

        bigint_set_len(r, len);
    } else {
        unsigned int bit_pos, shift, pos;

        /* power of two base: no multiplication is needed */
        memset(r_tab, 0, sizeof(r_tab[0]) * n_limbs);
        for(i = 0; i < n_digits; i++) {
            c = to_digit(p[n_digits - 1 - i]);
            if (c >= (bi_limb_t)radix) {
                p += n_digits - 1 - i;
                break;
            }

            bit_pos = i * log2_radix;
            shift = bit_pos & (BIGINT_LIMB_BITS - 1);
            pos = bit_pos / BIGINT_LIMB_BITS;
            r_tab[pos] |= c << shift;
            /* if log2_radix does not divide BIGINT_LIMB_BITS, needed an
               additional op */
            if (shift + log2_radix > BIGINT_LIMB_BITS) {
                r_tab[pos + 1] |= c >> (BIGINT_LIMB_BITS - shift);
            }
        }

        p += n_digits;
    }

    if (end)
        *end = (char *)p;

    r = bigint_normalize(r);
    /* XXX: could do it in place */
    if (is_neg) {
        purc_variant *r1;
        r1 = bigint_neg(r);
        bigint_free(r);
        r = r1;
    }

    return r;
}

/* 2 <= base <= 36 */
static char const digits[36] = "0123456789abcdefghijklmnopqrstuvwxyz";

/* special version going backwards */
/* XXX: use dtoa.c */
static char *bi_u64toa(char *q, int64_t n, unsigned int base)
{
    int digit;
    if (base == 10) {
        /* division by known base uses multiplication */
        do {
            digit = (uint64_t)n % 10;
            n = (uint64_t)n / 10;
            *--q = '0' + digit;
        } while (n != 0);
    } else {
        do {
            digit = (uint64_t)n % base;
            n = (uint64_t)n / base;
            *--q = digits[digit];
        } while (n != 0);
    }
    return q;
}

/* len >= 1. 2 <= radix <= 36 */
static char *limb_to_a(char *q, bi_limb_t n, unsigned int radix, int len)
{
    int digit, i;

    if (radix == 10) {
        /* specific case with constant divisor */
        /* XXX: optimize */
        for(i = 0; i < len; i++) {
            digit = (bi_limb_t)n % 10;
            n = (bi_limb_t)n / 10;
            *--q = digit + '0';
        }
    } else {
        for(i = 0; i < len; i++) {
            digit = (bi_limb_t)n % radix;
            n = (bi_limb_t)n / radix;
            *--q = digits[digit];
        }
    }
    return q;
}

#define BIGINT_RADIX_MAX 36

static const uint8_t digits_per_limb_table[BIGINT_RADIX_MAX - 1] = {
#if BIGINT_LIMB_BITS == 32
32,20,16,13,12,11,10,10, 9, 9, 8, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
#else
64,40,32,27,24,22,21,20,19,18,17,17,16,16,16,15,15,15,14,14,14,14,13,13,13,13,13,13,13,12,12,12,12,12,12,
#endif
};

static const bi_limb_t radix_base_table[BIGINT_RADIX_MAX - 1] = {
#if BIGINT_LIMB_BITS == 32
 0x00000000, 0xcfd41b91, 0x00000000, 0x48c27395,
 0x81bf1000, 0x75db9c97, 0x40000000, 0xcfd41b91,
 0x3b9aca00, 0x8c8b6d2b, 0x19a10000, 0x309f1021,
 0x57f6c100, 0x98c29b81, 0x00000000, 0x18754571,
 0x247dbc80, 0x3547667b, 0x4c4b4000, 0x6b5a6e1d,
 0x94ace180, 0xcaf18367, 0x0b640000, 0x0e8d4a51,
 0x1269ae40, 0x17179149, 0x1cb91000, 0x23744899,
 0x2b73a840, 0x34e63b41, 0x40000000, 0x4cfa3cc1,
 0x5c13d840, 0x6d91b519, 0x81bf1000,
#else
 0x0000000000000000, 0xa8b8b452291fe821, 0x0000000000000000, 0x6765c793fa10079d,
 0x41c21cb8e1000000, 0x3642798750226111, 0x8000000000000000, 0xa8b8b452291fe821,
 0x8ac7230489e80000, 0x4d28cb56c33fa539, 0x1eca170c00000000, 0x780c7372621bd74d,
 0x1e39a5057d810000, 0x5b27ac993df97701, 0x0000000000000000, 0x27b95e997e21d9f1,
 0x5da0e1e53c5c8000, 0xd2ae3299c1c4aedb, 0x16bcc41e90000000, 0x2d04b7fdd9c0ef49,
 0x5658597bcaa24000, 0xa0e2073737609371, 0x0c29e98000000000, 0x14adf4b7320334b9,
 0x226ed36478bfa000, 0x383d9170b85ff80b, 0x5a3c23e39c000000, 0x8e65137388122bcd,
 0xdd41bb36d259e000, 0x0aee5720ee830681, 0x1000000000000000, 0x172588ad4f5f0981,
 0x211e44f7d02c1000, 0x2ee56725f06e5c71, 0x41c21cb8e1000000,
#endif
};

ssize_t bigint_stringify(const purc_variant_t val, int radix,
        void *ctxt, stringify_f cb)
{
    size_t val_len;
    const bi_limb_t *val_tab;
    val_tab = bigint_get_tab_const(val, &val_len);

    if (val_len == 1) {
        char buf[66];
        size_t len;
        len = i64toa_radix(buf, val_tab[0], radix);
        if (cb(buf, len, ctxt) == 0)
            return len;
        return -1;
    } else {
        purc_variant *tmp = NULL;
        char *buf, *q, *buf_end;
        int is_neg, n_bits, log2_radix, n_digits;
        bool is_binary_radix;

        if (val_len == 1 && val_tab[0] == 0) {
            /* '0' case */
            if (cb("0", 1, ctxt) == 0)
                return 1;
            return -1;
        }

        purc_variant *r;
        is_binary_radix = ((radix & (radix - 1)) == 0);
        is_neg = bigint_sign(val);
        if (is_neg) {
            tmp = bigint_neg(val);
            if (!tmp)
                return -1;

            r = tmp;
        } else if (!is_binary_radix) {
            /* need to modify 'val' */
            tmp = bigint_new(val_len);
            if (!tmp)
                return -1;

            bi_limb_t *tmp_tab;
            tmp_tab = bigint_get_tab(tmp, NULL);
            memcpy(tmp_tab, val_tab, val_len * sizeof(val_tab[0]));
            r = tmp;
        }
        else
            r = (purc_variant *)val;

        size_t r_len;
        bi_limb_t *r_tab;
        r_tab = bigint_get_tab(r, &r_len);

        log2_radix = 31 - clz32(radix); /* floor(log2(radix)) */
        n_bits = r_len * BIGINT_LIMB_BITS - bi_limb_safe_clz(r_tab[r_len - 1]);
        /* n_digits is exact only if radix is a power of
           two. Otherwise it is >= the exact number of digits */
        n_digits = (n_bits + log2_radix - 1) / log2_radix;
        buf = malloc(n_digits + is_neg + 1);
        if (!buf) {
            bigint_free(tmp);
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return -1;
        }

        q = buf + n_digits + is_neg + 1;
        *--q = '\0';
        buf_end = q;
        if (!is_binary_radix) {
            int len;
            bi_limb_t radix_base, v;
            radix_base = radix_base_table[radix - 2];
            len = r_len;
            for(;;) {
                /* remove leading zero limbs */
                while (len > 1 && r_tab[len - 1] == 0)
                    len--;
                if (len == 1 && r_tab[0] < radix_base) {
                    v = r_tab[0];
                    if (v != 0) {
                        q = bi_u64toa(q, v, radix);
                    }
                    break;
                } else {
                    v = mp_div1(r_tab, r_tab, len, radix_base, 0);
                    q = limb_to_a(q, v, radix, digits_per_limb_table[radix - 2]);
                }
            }
        } else {
            int i, shift;
            unsigned int bit_pos, pos, c;

            /* radix is a power of two */
            for(i = 0; i < n_digits; i++) {
                bit_pos = i * log2_radix;
                pos = bit_pos / BIGINT_LIMB_BITS;
                shift = bit_pos % BIGINT_LIMB_BITS;
                if (LIKELY((shift + log2_radix) <= BIGINT_LIMB_BITS)) {
                    c = r_tab[pos] >> shift;
                } else {
                    c = (r_tab[pos] >> shift) |
                        (r_tab[pos + 1] << (BIGINT_LIMB_BITS - shift));
                }
                c &= (radix - 1);
                *--q = digits[c];
            }
        }

        if (is_neg)
            *--q = '-';
        bigint_free(tmp);
        ssize_t sz = buf_end - q;
        if (cb(q, sz, ctxt))
            sz = -1;
        free(buf);
        return sz;
    }
}

purc_variant_t purc_variant_make_bigint_from_i64(int64_t a)
{
#if BIGINT_LIMB_BITS == 64
    return bigint_new_si(a);
#else
    if (a >= INT32_MIN && a <= INT32_MAX) {
        return bigint_new_si(a);
    } else {
        purc_variant *r;
        r = bigint_new(2);
        if (!r)
            return NULL;

        bi_limb_t *tab = bigint_get_tab(r, NULL);
        tab[0] = a;
        tab[1] = a >> 32;
        return r;
    }
#endif
}

purc_variant_t purc_variant_make_bigint_from_u64(uint64_t a)
{
    if (a <= INT64_MAX) {
        return purc_variant_make_bigint_from_i64(a);
    } else {
        purc_variant *r;
        r = bigint_new((65 + BIGINT_LIMB_BITS - 1) / BIGINT_LIMB_BITS);
        if (!r)
            return NULL;

        bi_limb_t *tab = bigint_get_tab(r, NULL);
#if BIGINT_LIMB_BITS == 64
        tab[0] = a;
        tab[1] = 0;
#else
        tab[0] = a;
        tab[1] = a >> 32;
        tab[2] = 0;
#endif
        return r;
    }
}

/* return (1, NULL) if not an integer, (2, NULL) if NaN or Infinity,
   (0, n) if an integer, (0, NULL) in case of memory error */
purc_variant_t purc_variant_make_bigint_from_f64(double f64)
{
    uint64_t a = float64_as_uint64(f64);
    int sgn, e, shift;
    uint64_t mant;
    bigint_buf buf;
    purc_variant *r;

    sgn = a >> 63;
    e = (a >> 52) & ((1 << 11) - 1);
    mant = a & (((uint64_t)1 << 52) - 1);
    if (e == 2047) {
        /* NaN, Infinity */
        pcinst_set_error(PURC_ERROR_OVERFLOW);
        return NULL;
    }
    if (e == 0 && mant == 0) {
        return bigint_new_si(0);
    }
    e -= 1023;
    /* 0 < a < 1 : not an integer */
    if (e < 0)
        goto not_an_integer;
    mant |= (uint64_t)1 << 52;
    if (e < 52) {
        shift = 52 - e;
        /* check that there is no fractional part */
        if (mant & (((uint64_t)1 << shift) - 1)) {
        not_an_integer:
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            return NULL;
        }
        mant >>= shift;
        e = 0;
    } else {
        e -= 52;
    }
    if (sgn)
        mant = -mant;
    /* the integer is mant*2^e */
    r = bigint_set_i64(&buf, (int64_t)mant);
    return bigint_shl(r, e);
}

