/*
 * @file numerify.c
 * @author Xu Xiaohong
 * @date 2022/06/29
 * @brief The implementation of public part for variant.
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

#include "variant-internals.h"

static double
_numerify_str(const char *s)
{
    PC_ASSERT(s);
    if (!*s)
        return 0.0;

    return strtod(s, NULL);
}

double
pcvar_str_numerify(purc_variant_t val)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_string(val));

    const char *s = purc_variant_get_string_const(val);
    return _numerify_str(s);
}

double
pcvar_atom_numerify(purc_variant_t val)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_atomstring(val) ||
            purc_variant_is_exception(val));

    const char *s = purc_atom_to_string(val->atom);
    return _numerify_str(s);
}

double
pcvar_bs_numerify(purc_variant_t val)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_bsequence(val));
    const unsigned char *bs;
    size_t nr;
    bs = purc_variant_get_bytes_const(val, &nr);
    if (nr == 0)
        return 0.0;

    int64_t v = 0;
    unsigned char *p = (unsigned char*)&v;
    for (size_t i=0; i<nr && i<8; ++i) {
        p[i] = bs[i];
    }

    return v;
}

double
pcvar_dynamic_numerify(purc_variant_t val)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_dynamic(val));

    purc_dvariant_method method;
    method = purc_variant_dynamic_get_getter(val);

    if (!method)
        return 0.0;

    bool silently = true;
    purc_variant_t v = method(val, 0, NULL, silently);
    if (v == PURC_VARIANT_INVALID)
        return 0.0;

    double d = pcvar_numerify(v);
    PURC_VARIANT_SAFE_CLEAR(v);

    return d;
}

double
pcvar_native_numerify(purc_variant_t val)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_native(val));

    void *entity;
    entity = purc_variant_native_get_entity(val);

    struct purc_native_ops *ops;
    ops = purc_variant_native_get_ops(val);
    if (!ops || !ops->property_getter)
        return 0.0;

    purc_nvariant_method method;
    method = ops->property_getter(entity, "__number");
    if (!method)
        return 0.0;

    purc_variant_t v;
    v = method(entity, "__number", 0, NULL, PCVRT_CALL_FLAG_SILENTLY);
    if (v == PURC_VARIANT_INVALID)
        return 0.0;

    double d = pcvar_numerify(v);
    PURC_VARIANT_SAFE_CLEAR(v);

    return d;
}

double
pcvar_obj_numerify(purc_variant_t val)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_object(val));

    double d = 0.0;

    purc_variant_t v;
    foreach_value_in_variant_object(val, v) {
        d += pcvar_numerify(v);
    }
    end_foreach;

    return d;
}

double
pcvar_arr_numerify(purc_variant_t val)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_array(val));

    double d = 0.0;

    purc_variant_t v;
    size_t idx;
    foreach_value_in_variant_array(val, v, idx) {
        UNUSED_PARAM(idx);
        d += pcvar_numerify(v);
    }
    end_foreach;

    return d;
}

double
pcvar_set_numerify(purc_variant_t val)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_set(val));

    double d = 0.0;

    purc_variant_t v;
    size_t idx;
    foreach_value_in_variant_set_order(val, v) {
        UNUSED_PARAM(idx);
        d += pcvar_numerify(v);
    }
    end_foreach;

    return d;
}

double
pcvar_tuple_numerify(purc_variant_t val)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_set(val));

    double d = 0.0;
    purc_variant_t *members;
    size_t sz;
    members = tuple_members(val, &sz);
    assert(members);

    purc_variant_t v;
    for (size_t idx = 0; idx < sz; idx++) {
        v = members[idx];
        d += pcvar_numerify(v);
    }
    return d;
}

double
pcvar_numerify(purc_variant_t val)
{
    PC_ASSERT(val != PURC_VARIANT_INVALID);

    switch (val->type) {
        case PURC_VARIANT_TYPE_UNDEFINED:
            return 0;

        case PURC_VARIANT_TYPE_NULL:
            return 0;

        case PURC_VARIANT_TYPE_BOOLEAN:
            return val->b ? 1 : 0;

        case PURC_VARIANT_TYPE_EXCEPTION:
            return pcvar_atom_numerify(val);

        case PURC_VARIANT_TYPE_NUMBER:
            return val->d;

        case PURC_VARIANT_TYPE_LONGINT:
            return (double)val->i64;

        case PURC_VARIANT_TYPE_ULONGINT:
            return (double)val->u64;

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            return (double)val->ld;

        case PURC_VARIANT_TYPE_ATOMSTRING:
            return pcvar_atom_numerify(val);

        case PURC_VARIANT_TYPE_STRING:
            return pcvar_str_numerify(val);

        case PURC_VARIANT_TYPE_BSEQUENCE:
            return pcvar_bs_numerify(val);

        case PURC_VARIANT_TYPE_DYNAMIC:
            return pcvar_dynamic_numerify(val);

        case PURC_VARIANT_TYPE_NATIVE:
            return pcvar_native_numerify(val);

        case PURC_VARIANT_TYPE_OBJECT:
            return pcvar_obj_numerify(val);

        case PURC_VARIANT_TYPE_ARRAY:
            return pcvar_arr_numerify(val);

        case PURC_VARIANT_TYPE_SET:
            return pcvar_set_numerify(val);

        case PURC_VARIANT_TYPE_TUPLE:
            return pcvar_tuple_numerify(val);

        default:
            PC_ASSERT(0);
            break;
    }

    return 0;
}

int
pcvar_diff_numerify(purc_variant_t l, purc_variant_t r)
{
    double ld = pcvar_numerify(l);
    double rd = pcvar_numerify(r);

    // FIXME: delta compare??? NaN/Inf??? fpclassify???
    if (ld < rd)
        return -1;

    if (ld > rd)
        return -1;

    return 0;
}

