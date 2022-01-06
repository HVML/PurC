/**
 * @file container-ops.c
 * @author Xue Shuming
 * @date 2022/01/05
 * @brief The API for container ops.
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

#include "config.h"
#include "private/variant.h"
#include "private/errors.h"
#include "variant-internals.h"
#include "purc-errors.h"
#include "purc-utils.h"


#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define SET_SILENT_ERROR(error)                                     \
    do {                                                            \
        if (!silently)                                              \
        {                                                           \
            pcinst_set_error(error);                                \
        }                                                           \
    } while (0)

typedef bool (*foreach_callback_fn)(void* ctxt, purc_variant_t value,
        void* extra);

typedef bool (*foreach_fn)(purc_variant_t v, foreach_callback_fn fn,
        void* ctxt);

// foreach_callback_fn  value=key, extra=value
static bool
object_foreach(purc_variant_t object, foreach_callback_fn fn, void* ctxt)
{
    bool ret = false;
    size_t sz = purc_variant_object_get_size(object);
    if (sz == 0 || sz == PURC_VARIANT_BADSIZE) {
        goto end;
    }

    purc_variant_t k, v;
    foreach_in_variant_object_safe_x(object, k, v)
        if (!fn(ctxt, k, v)) {
            goto end;
        }
    end_foreach;
    ret = true;

end:
    return ret;
}

static bool
array_foreach(purc_variant_t array, foreach_callback_fn fn, void* ctxt)
{
    bool ret = false;
    size_t sz = purc_variant_array_get_size(array);
    if (sz == 0 || sz == PURC_VARIANT_BADSIZE) {
        goto end;
    }

    purc_variant_t val;
    size_t curr;
    foreach_value_in_variant_array_safe(array, val, curr)
        if (!fn(ctxt, val, &curr)) {
            goto end;
        }
        --curr;
    end_foreach;
    ret = true;

end:
    return ret;
}

static bool
set_foreach(purc_variant_t set, foreach_callback_fn fn, void* ctxt)
{
    bool ret = false;
    size_t sz = purc_variant_set_get_size(set);
    if (sz == 0 || sz == PURC_VARIANT_BADSIZE) {
        goto end;
    }

    purc_variant_t v;
    struct rb_node *n;
    UNUSED_VARIABLE(n);
    foreach_value_in_variant_set_safe_x(set, v, n)
        if (!fn(ctxt, v, NULL)) {
            goto end;
        }
    end_foreach;
    ret = true;

end:
    return ret;
}

static bool
remove_object_member(void* ctxt, purc_variant_t key, void* extra)
{
    UNUSED_PARAM(extra);
    return purc_variant_object_remove((purc_variant_t)ctxt, key);
}

static bool
add_object_member(void* dst, purc_variant_t key, void* extra)
{
    return purc_variant_object_set((purc_variant_t)dst, key,
            (purc_variant_t)extra);
}

static bool
remove_array_member(void* ctxt, purc_variant_t value, void* extra)
{
    UNUSED_PARAM(value);
    int idx = *(int*)extra;
    return purc_variant_array_remove((purc_variant_t)ctxt, idx);
}

static bool
append_array_member(void* ctxt, purc_variant_t value, void* extra)
{
    UNUSED_PARAM(extra);
    return purc_variant_array_append((purc_variant_t)ctxt, value);
}

static bool
prepend_array_member(void* ctxt, purc_variant_t value, void* extra)
{
    UNUSED_PARAM(extra);
    return purc_variant_array_prepend((purc_variant_t)ctxt, value);
}

static bool
remove_set_member(void* ctxt, purc_variant_t value, void* extra)
{
    UNUSED_PARAM(extra);
    return purc_variant_set_remove((purc_variant_t)ctxt, value);
}

static bool
add_set_member(void* ctxt, purc_variant_t value, void* extra)
{
    UNUSED_PARAM(extra);
    return purc_variant_set_add((purc_variant_t)ctxt, value, false);
}

static bool
add_set_member_overwrite(void* ctxt, purc_variant_t value, void* extra)
{
    UNUSED_PARAM(extra);
    return purc_variant_set_add((purc_variant_t)ctxt, value, true);
}

static bool
object_displace(purc_variant_t dst, purc_variant_t src, bool silently)
{
    UNUSED_PARAM(silently);

    bool ret = false;

    if (!purc_variant_is_object(src)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    if(!object_foreach(dst, remove_object_member, dst)) {
        goto end;
    }

    if(!object_foreach(src, add_object_member, dst)) {
        goto end;
    }
    ret = true;

end:
    return ret;
}

static bool
array_displace(purc_variant_t dst, purc_variant_t src, bool silently)
{
    UNUSED_PARAM(silently);

    bool ret = false;

    if (!purc_variant_is_array(src) && !purc_variant_is_set(src)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    if (!array_foreach(dst, remove_array_member, dst)) {
        goto end;
    }

    foreach_fn foreach = purc_variant_is_array(src) ? array_foreach :
        set_foreach;
    if (!foreach(src, append_array_member, dst)) {
        goto end;
    }
    ret = true;

end:
    return ret;
}

static bool
set_displace(purc_variant_t dst, purc_variant_t src, bool silently)
{
    UNUSED_PARAM(silently);

    bool ret = false;

    enum purc_variant_type type = purc_variant_get_type(src);
    switch (type) {
        case PURC_VARIANT_TYPE_OBJECT:
            if (!set_foreach(dst, remove_set_member, dst)) {
                goto end;
            }
            if (!purc_variant_set_add(dst, src, false)) {
                goto end;
            }
            ret = true;
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            if (!set_foreach(dst, remove_set_member, dst)) {
                goto end;
            }
            if (!array_foreach(src, add_set_member, dst)) {
                goto end;
            }
            ret = true;
            break;

        case PURC_VARIANT_TYPE_SET:
            if (!set_foreach(dst, remove_set_member, dst)) {
                goto end;
            }
            if (!set_foreach(src, add_set_member, dst)) {
                goto end;
            }
            ret = true;
            break;

        default:
            SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
            ret = false;
            break;
    }

end:
    return ret;
}

static bool
is_in_set(purc_variant_t set, purc_variant_t v)
{
    bool ret = false;
    purc_variant_t val;
    foreach_value_in_variant_set(set, val)
        if (purc_variant_compare_ex(val, v, PCVARIANT_COMPARE_OPT_AUTO) == 0) {
            ret = true;
            goto end;
        }
    end_foreach;

end:
    return ret;
}

static bool
is_in_array(purc_variant_t array, purc_variant_t v)
{
    bool ret = false;
    purc_variant_t val;
    foreach_value_in_variant_array(array, val)
        if (purc_variant_compare_ex(val, v, PCVARIANT_COMPARE_OPT_AUTO) == 0) {
            ret = true;
            goto end;
        }
    end_foreach;

end:
    return ret;
}

bool
purc_variant_container_displace(purc_variant_t dst,
        purc_variant_t src, bool silently)
{
    bool ret = false;

    if (dst == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_ARGUMENT_MISSED);
        goto end;
    }

    enum purc_variant_type type = purc_variant_get_type(dst);
    switch (type) {
        case PURC_VARIANT_TYPE_OBJECT:
            ret = object_displace(dst, src, silently);
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            ret = array_displace(dst, src, silently);
            break;

        case PURC_VARIANT_TYPE_SET:
            ret = set_displace(dst, src, silently);
            break;

        default:
            SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
            break;
    }

end:
    return ret;
}

bool
purc_variant_array_append_another(purc_variant_t dst,
        purc_variant_t another, bool silently)
{
    UNUSED_PARAM(silently);

    bool ret = false;

    if (dst == PURC_VARIANT_INVALID || another == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_ARGUMENT_MISSED);
        goto end;
    }

    if (!purc_variant_is_array(dst) || !purc_variant_is_array(another)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    if(!array_foreach(another, append_array_member, dst)) {
        goto end;
    }
    ret = true;

end:
    return ret;
}

bool
purc_variant_array_prepend_another(purc_variant_t dst,
        purc_variant_t another, bool silently)
{
    UNUSED_PARAM(silently);

    bool ret = false;

    if (dst == PURC_VARIANT_INVALID || another == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_ARGUMENT_MISSED);
        goto end;
    }

    if (!purc_variant_is_array(dst) || !purc_variant_is_array(another)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    // FIXME :
    // dst : 1, 2, 3, 4
    // another  : A, B, C, D
    // now result : D, C, B, A, 1, 2, 3, 4
    // OR  :  A, B, C, D, 1, 2, 3, 4
    if(!array_foreach(another, prepend_array_member, dst)) {
        goto end;
    }
    ret = true;

end:
    return ret;
}

bool
purc_variant_object_merge_another(purc_variant_t dst,
        purc_variant_t another, bool silently)
{
    UNUSED_PARAM(silently);

    bool ret = false;

    if (dst == PURC_VARIANT_INVALID || another == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_ARGUMENT_MISSED);
        goto end;
    }

    if (!purc_variant_is_object(dst) || !purc_variant_is_object(another)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    purc_variant_t k, v;
    foreach_key_value_in_variant_object(another, k, v)
        if(!purc_variant_object_set(dst, k, v)) {
            goto end;
        }
    end_foreach;
    ret = true;

end:
    return ret;
}

bool
purc_variant_array_insert_another_before(purc_variant_t dst, int idx,
        purc_variant_t another, bool silently)
{
    UNUSED_PARAM(silently);

    bool ret = false;

    if (dst == PURC_VARIANT_INVALID || another == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_ARGUMENT_MISSED);
        goto end;
    }

    if (!purc_variant_is_array(dst) || !purc_variant_is_array(another)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    // FIXME: like purc_variant_array_prepend_another
    purc_variant_t v;
    foreach_value_in_variant_array(another, v)
        if (!purc_variant_array_insert_before(dst, idx, v)) {
            goto end;
        }
    end_foreach;
    ret = true;

end:
    return ret;
}

bool
purc_variant_array_insert_another_after(purc_variant_t dst, int idx,
        purc_variant_t another, bool silently)
{
    UNUSED_PARAM(silently);

    bool ret = false;

    if (dst == PURC_VARIANT_INVALID || another == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_ARGUMENT_MISSED);
        goto end;
    }

    if (!purc_variant_is_array(dst) || !purc_variant_is_array(another)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    // FIXME: like purc_variant_array_prepend_another
    purc_variant_t v;
    foreach_value_in_variant_array(another, v)
        if (!purc_variant_array_insert_after(dst, idx, v)) {
            goto end;
        }
    end_foreach;
    ret = true;

end:
    return ret;
}

bool
purc_variant_container_unite(purc_variant_t dst,
        purc_variant_t src, bool silently)
{
    UNUSED_PARAM(silently);

    bool ret = false;

    if (dst == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_ARGUMENT_MISSED);
        goto end;
    }

    if (!purc_variant_is_set(dst)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    if (purc_variant_is_set(src)) {
        ret = set_foreach(src, add_set_member_overwrite, dst);
    }
    else if (purc_variant_is_array(src)) {
        ret = array_foreach(src, add_set_member_overwrite, dst);
    }
    else {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        ret = false;
    }

end:
    return ret;
}

bool
purc_variant_container_intersect(purc_variant_t dst,
        purc_variant_t src, bool silently)
{
    UNUSED_PARAM(silently);

    bool ret = false;

    if (dst == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_ARGUMENT_MISSED);
        goto end;
    }

    if (!purc_variant_is_set(dst)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    purc_variant_t result = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (result == PURC_VARIANT_INVALID) {
        goto end;
    }

    purc_variant_t val;
    if (purc_variant_is_set(src)) {
        foreach_value_in_variant_set(src, val)
            if (is_in_set(dst, val)) {
                if(!purc_variant_array_append(result, val)) {
                    goto error;
                }
            }
        end_foreach;
        ret = set_displace(dst, result, silently);
    }
    else if (purc_variant_is_array(src)) {
        foreach_value_in_variant_array(src, val)
            if (is_in_set(dst, val)) {
                if(!purc_variant_array_append(result, val)) {
                    goto error;
                }
            }
        end_foreach;
        ret = set_displace(dst, result, silently);
    }
    else {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        ret = false;
    }

error:
    purc_variant_unref(result);

end:
    return ret;
}

bool
purc_variant_container_subtract(purc_variant_t dst,
        purc_variant_t src, bool silently)
{
    UNUSED_PARAM(silently);

    bool ret = false;

    if (dst == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_ARGUMENT_MISSED);
        goto end;
    }

    if (!purc_variant_is_set(dst)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    purc_variant_t result = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (result == PURC_VARIANT_INVALID) {
        goto end;
    }

    purc_variant_t val;
    if (purc_variant_is_set(src)) {
        foreach_value_in_variant_set(dst, val)
            if (!is_in_set(src, val)) {
                if(!purc_variant_array_append(result, val)) {
                    goto error;
                }
            }
        end_foreach;
        ret = set_displace(dst, result, silently);
    }
    else if (purc_variant_is_array(src)) {
        foreach_value_in_variant_set(dst, val)
            if (!is_in_array(src, val)) {
                if(!purc_variant_array_append(result, val)) {
                    goto error;
                }
            }
        end_foreach;
        ret = set_displace(dst, result, silently);
    }
    else {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        ret = false;
    }

error:
    purc_variant_unref(result);

end:
    return ret;
}

bool
purc_variant_container_xor(purc_variant_t dst,
        purc_variant_t src, bool silently)
{
    UNUSED_PARAM(silently);

    bool ret = false;

    if (dst == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_ARGUMENT_MISSED);
        goto end;
    }

    if (!purc_variant_is_set(dst)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    purc_variant_t result = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (result == PURC_VARIANT_INVALID) {
        goto end;
    }

    purc_variant_t val;
    if (purc_variant_is_set(src)) {
        foreach_value_in_variant_set(dst, val)
            if (!is_in_set(src, val)) {
                if(!purc_variant_array_append(result, val)) {
                    goto error;
                }
            }
        end_foreach;

        foreach_value_in_variant_set(src, val)
            if (!is_in_set(dst, val)) {
                if(!purc_variant_array_append(result, val)) {
                    goto error;
                }
            }
        end_foreach;
        ret = set_displace(dst, result, silently);
    }
    else if (purc_variant_is_array(src)) {
        foreach_value_in_variant_set(dst, val)
            if (!is_in_array(src, val)) {
                if(!purc_variant_array_append(result, val)) {
                    goto error;
                }
            }
        end_foreach;

        foreach_value_in_variant_array(src, val)
            if (!is_in_set(dst, val)) {
                if(!purc_variant_array_append(result, val)) {
                    goto error;
                }
            }
        end_foreach;
        ret = set_displace(dst, result, silently);
    }
    else {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        ret = false;
    }

    ret = set_displace(dst, result, silently);
error:
    purc_variant_unref(result);

end:
    return ret;
}

bool
purc_variant_container_overwrite(purc_variant_t dst,
        purc_variant_t src, bool silently)
{
    UNUSED_PARAM(silently);

    bool ret = false;

    if (dst == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_ARGUMENT_MISSED);
        goto end;
    }

    if (!purc_variant_is_set(dst)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    enum purc_variant_type type = purc_variant_get_type(src);
    switch (type) {
        case PURC_VARIANT_TYPE_OBJECT:
            ret = purc_variant_set_add(dst, src, true);
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            ret = array_foreach(src, add_set_member_overwrite, dst);
            break;

        case PURC_VARIANT_TYPE_SET:
            ret = set_foreach(src, add_set_member_overwrite, dst);
            break;

        default:
            SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
            break;
    }

end:
    return ret;
}

