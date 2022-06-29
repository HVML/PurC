/*
 * @file stringify.c
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

static int
_stringify_str(const char *s, void *ctxt, stringify_f cb)
{
    PC_ASSERT(s);

    const unsigned char *bs = (const unsigned char*)s;
    size_t len = strlen(s);
    return cb(bs, len, ctxt);
}

int
pcvar_str_stringify(purc_variant_t val, void *ctxt, stringify_f cb)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_string(val));

    int r;
    const char *s = purc_variant_get_string_const(val);
    r = _stringify_str(s, ctxt, cb);
    return r;
}

int
pcvar_atom_stringify(purc_variant_t val, void *ctxt, stringify_f cb)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_atomstring(val) ||
            purc_variant_is_exception(val));

    int r;
    const char *s = purc_atom_to_string(val->atom);
    r = _stringify_str(s, ctxt, cb);
    return r;
}

int
pcvar_bs_stringify(purc_variant_t val, void *ctxt, stringify_f cb)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_bsequence(val));

    int r = 0;

    const unsigned char *bs;
    size_t nr;
    bs = purc_variant_get_bytes_const(val, &nr);

    char buf[8];
    for (size_t i=0; i<nr; ++i) {
        unsigned char c = bs[i];
        snprintf(buf, sizeof(buf), "%02X", c);
        r = _stringify_str(buf, ctxt, cb);
        if (r)
            return r;
    }

    return r;
}

int
pcvar_dynamic_stringify(purc_variant_t val, void *ctxt, stringify_f cb)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_dynamic(val));

    purc_dvariant_method getter;
    getter = purc_variant_dynamic_get_getter(val);

    purc_dvariant_method setter;
    setter = purc_variant_dynamic_get_setter(val);

    char buf[256];
    snprintf(buf, sizeof(buf), "<dynamic: %p, %p>", getter, setter);
    return _stringify_str(buf, ctxt, cb);
}

int
pcvar_native_stringify(purc_variant_t val, void *ctxt, stringify_f cb)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_native(val));

    void *entity;
    entity = purc_variant_native_get_entity(val);

    char buf[256];
    snprintf(buf, sizeof(buf), "<native: %p>", entity);
    return _stringify_str(buf, ctxt, cb);
}

int
pcvar_obj_stringify(purc_variant_t val, void *ctxt, stringify_f cb)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_object(val));

    int r = 0;

    purc_variant_t k, v;
    foreach_key_value_in_variant_object(val, k, v) {
        PC_ASSERT(k != PURC_VARIANT_INVALID);
        PC_ASSERT(purc_variant_is_string(k));
        PC_ASSERT(v != PURC_VARIANT_INVALID);

        r = pcvar_stringify(k, ctxt, cb);
        if (r)
            return r;

        r = _stringify_str(":", ctxt, cb);
        if (r)
            return r;

        r = pcvar_stringify(v, ctxt, cb);
        if (r)
            return r;

        r = _stringify_str("\n", ctxt, cb);
        if (r)
            return r;
    }
    end_foreach;

    return r;
}

int
pcvar_arr_stringify(purc_variant_t val, void *ctxt, stringify_f cb)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_array(val));

    int r = 0;

    purc_variant_t v;
    size_t idx;
    foreach_value_in_variant_array(val, v, idx) {
        UNUSED_PARAM(idx);
        PC_ASSERT(v != PURC_VARIANT_INVALID);

        r = pcvar_stringify(v, ctxt, cb);
        if (r)
            return r;

        r = _stringify_str("\n", ctxt, cb);
        if (r)
            return r;
    }
    end_foreach;

    return r;
}

int
pcvar_set_stringify(purc_variant_t val, void *ctxt, stringify_f cb)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_set(val));

    int r = 0;

    purc_variant_t v;
    foreach_value_in_variant_set_order(val, v) {
        PC_ASSERT(v != PURC_VARIANT_INVALID);

        r = pcvar_stringify(v, ctxt, cb);
        if (r)
            return r;

        r = _stringify_str("\n", ctxt, cb);
        if (r)
            return r;
    }
    end_foreach;

    return r;
}

int
pcvar_tuple_stringify(purc_variant_t val, void *ctxt, stringify_f cb)
{
    PC_ASSERT(val);
    PC_ASSERT(purc_variant_is_set(val));
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(cb);

    PC_ASSERT(0); // Not implemented yet
}

int
pcvar_stringify(purc_variant_t val, void *ctxt, stringify_f cb)
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
            return pcvar_atom_stringify(val, ctxt, cb);

        case PURC_VARIANT_TYPE_NUMBER:
            return val->d;

        case PURC_VARIANT_TYPE_LONGINT:
            return (int)val->i64;

        case PURC_VARIANT_TYPE_ULONGINT:
            return (int)val->u64;

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            return (int)val->ld;

        case PURC_VARIANT_TYPE_ATOMSTRING:
            return pcvar_atom_stringify(val, ctxt, cb);

        case PURC_VARIANT_TYPE_STRING:
            return pcvar_str_stringify(val, ctxt, cb);

        case PURC_VARIANT_TYPE_BSEQUENCE:
            return pcvar_bs_stringify(val, ctxt, cb);

        case PURC_VARIANT_TYPE_DYNAMIC:
            return pcvar_dynamic_stringify(val, ctxt, cb);

        case PURC_VARIANT_TYPE_NATIVE:
            return pcvar_native_stringify(val, ctxt, cb);

        case PURC_VARIANT_TYPE_OBJECT:
            return pcvar_obj_stringify(val, ctxt, cb);

        case PURC_VARIANT_TYPE_ARRAY:
            return pcvar_arr_stringify(val, ctxt, cb);

        case PURC_VARIANT_TYPE_SET:
            return pcvar_set_stringify(val, ctxt, cb);

        case PURC_VARIANT_TYPE_TUPLE:
            return pcvar_tuple_stringify(val, ctxt, cb);

        default:
            PC_ASSERT(0);
    }
}

