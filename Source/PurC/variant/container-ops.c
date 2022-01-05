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

#define _GNU_SOURCE       // qsort_r

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
        if (!silent)                                                \
        {                                                           \
            pcinst_set_error(error);                                \
        }                                                           \
    } while (0)

static bool
object_clear(purc_variant_t object)
{
    size_t sz = purc_variant_object_get_size(object);
    if (!sz) {
        return true;
    }
    purc_variant_t keys = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (keys == PURC_VARIANT_INVALID) {
        return false;
    }
    purc_variant_t k, v;
    UNUSED_VARIABLE(v);
    foreach_key_value_in_variant_object(object, k, v)
        if(!purc_variant_array_append(keys, k)) {
            return false;
        }
    end_foreach;

    foreach_value_in_variant_array(keys, k)
        if (!purc_variant_object_remove(object, k)) {
            return false;
        }
    end_foreach;

    purc_variant_unref(keys);
    return true;
}

bool
object_displace(purc_variant_t container, purc_variant_t value, bool silent)
{
    UNUSED_PARAM(silent);
    if (!purc_variant_is_object(value)) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return false;
    }

    if (!object_clear(container)) {
        return false;
    }
    purc_variant_t k, v;
    foreach_key_value_in_variant_object(value, k, v)
        purc_variant_object_set(container, k, v);
    end_foreach;
    return true;
}

bool
array_displace(purc_variant_t container, purc_variant_t value, bool silent)
{
    UNUSED_PARAM(container);
    UNUSED_PARAM(value);
    UNUSED_PARAM(silent);
    if (!purc_variant_is_array(value) && !purc_variant_is_set(value)) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return false;
    }

    // clear
    purc_variant_t val;
    size_t curr;
    foreach_value_in_variant_array_safe(container, val, curr)
        if (!purc_variant_array_remove(container, curr)) {
            return false;
        }
    end_foreach;


    if (purc_variant_is_array(value)) {
        foreach_value_in_variant_array_safe(value, val, curr)
            if (!purc_variant_array_append(container, val)) {
                return false;
            }
        end_foreach;
    }
    else {
        foreach_value_in_variant_set(value, val)
            if (!purc_variant_array_append(container, val)) {
                return false;
            }
        end_foreach;
    }

    return false;
}

static bool
set_clear(purc_variant_t set)
{
    purc_variant_t v;
    struct rb_node *n;
    foreach_value_in_variant_set_safe_x(set, v, n)
        if (!purc_variant_set_remove(set, v)) {
            return false;
        }
    end_foreach;
    UNUSED_VARIABLE(n);
    return true;
}

bool
set_displace(purc_variant_t container, purc_variant_t value, bool silent)
{
    UNUSED_PARAM(silent);
    enum purc_variant_type type = purc_variant_get_type(value);
    switch (type) {
        case PURC_VARIANT_TYPE_OBJECT:
            if (!set_clear(container)) {
                return false;
            }
            if (!purc_variant_set_add(container, value, false)) {
                return false;
            }
            return true;

        case PURC_VARIANT_TYPE_ARRAY:
            if (!set_clear(container)) {
                return false;
            }
            purc_variant_t val;
            size_t curr;
            foreach_value_in_variant_array_safe(value, val, curr)
                if (!purc_variant_set_add(container, val, false)) {
                    return false;
                }
            end_foreach;
            return true;

        case PURC_VARIANT_TYPE_SET:
            if (!set_clear(container)) {
                return false;
            }
            purc_variant_t v;
            foreach_value_in_variant_set(value, v)
                if (!purc_variant_set_add(container, v, false)) {
                    return false;
                }
            end_foreach;
            return true;

        default:
            pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            return false;
    }
    return false;
}

bool
purc_variant_container_displace(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    if (container == PURC_VARIANT_INVALID || value == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return false;
    }

    enum purc_variant_type type = purc_variant_get_type(container);
    switch (type) {
        case PURC_VARIANT_TYPE_OBJECT:
            return object_displace(container, value, silent);

        case PURC_VARIANT_TYPE_ARRAY:
            return array_displace(container, value, silent);

        case PURC_VARIANT_TYPE_SET:
            return set_displace(container, value, silent);

        default:
            pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            return false;
    }

    PC_ASSERT(0);
    return false;
}

bool
purc_variant_container_append(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(silent);

    if (container == PURC_VARIANT_INVALID || value == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return false;
    }

    if (!purc_variant_is_array(container) || !purc_variant_is_array(value)) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return false;
    }

    purc_variant_t v;
    foreach_value_in_variant_array(value, v)
        if (!purc_variant_array_append(container, v)) {
            return false;
        }
    end_foreach;

    return true;
}

bool
purc_variant_container_prepend(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(silent);
    if (container == PURC_VARIANT_INVALID || value == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return false;
    }

    if (!purc_variant_is_array(container) || !purc_variant_is_array(value)) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return false;
    }

    // FIXME :
    // container : 1, 2, 3, 4
    // value  : A, B, C, D
    // now result : D, C, B, A, 1, 2, 3, 4
    // OR  :  A, B, C, D, 1, 2, 3, 4
    purc_variant_t v;
    foreach_value_in_variant_array(value, v)
        if (!purc_variant_array_prepend(container, v)) {
            return false;
        }
    end_foreach;

    return true;
}

bool
purc_variant_container_merge(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(silent);
    if (container == PURC_VARIANT_INVALID || value == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return false;
    }

    if (!purc_variant_is_object(container) || !purc_variant_is_object(value)) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return false;
    }

    purc_variant_t k, v;
    foreach_key_value_in_variant_object(value, k, v)
        if(!purc_variant_object_set(container, k, v)) {
            return false;
        }
    end_foreach;

    return true;
}

bool
purc_variant_container_insert_before(purc_variant_t container, int idx,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(silent);
    if (container == PURC_VARIANT_INVALID || value == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return false;
    }

    if (!purc_variant_is_array(container) || !purc_variant_is_array(value)) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return false;
    }

    // FIXME: like purc_variant_container_prepend
    purc_variant_t v;
    foreach_value_in_variant_array(value, v)
        if (!purc_variant_array_insert_before(container, idx, v)) {
            return false;
        }
    end_foreach;

    return true;
}

bool
purc_variant_container_insert_after(purc_variant_t container, int idx,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(silent);
    if (container == PURC_VARIANT_INVALID || value == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return false;
    }

    if (!purc_variant_is_array(container) || !purc_variant_is_array(value)) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return false;
    }

    // FIXME: like purc_variant_container_prepend
    purc_variant_t v;
    foreach_value_in_variant_array(value, v)
        if (!purc_variant_array_insert_after(container, idx, v)) {
            return false;
        }
    end_foreach;

    return true;
}

bool
purc_variant_container_unite(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(silent);
    if (container == PURC_VARIANT_INVALID || value == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return false;
    }

    if (!purc_variant_is_set(container)) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return false;
    }

    purc_variant_t val;
    if (purc_variant_is_set(value)) {
        foreach_value_in_variant_set(value, val)
            if (!purc_variant_set_add(container, val, true)) {
                return false;
            }
        end_foreach;
        return true;
    }
    else if (purc_variant_is_array(value)) {
        foreach_value_in_variant_array(value, val)
            if (!purc_variant_set_add(container, val, true)) {
                return false;
            }
        end_foreach;
        return true;
    }
    return false;
}

static bool
is_in_set(purc_variant_t set, purc_variant_t v)
{
    purc_variant_t val;
    foreach_value_in_variant_set(set, val)
        if (purc_variant_compare_ex(val, v, PCVARIANT_COMPARE_OPT_AUTO) == 0) {
            return true;
        }
    end_foreach;
    return false;
}

static bool
is_in_array(purc_variant_t array, purc_variant_t v)
{
    purc_variant_t val;
    foreach_value_in_variant_array(array, val)
        if (purc_variant_compare_ex(val, v, PCVARIANT_COMPARE_OPT_AUTO) == 0) {
            return true;
        }
    end_foreach;
    return false;
}

bool
purc_variant_container_intersect(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(silent);
    if (container == PURC_VARIANT_INVALID || value == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return false;
    }

    if (!purc_variant_is_set(container)) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return false;
    }

    purc_variant_t result = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (result == PURC_VARIANT_INVALID) {
        return false;
    }

    purc_variant_t val;
    if (purc_variant_is_set(value)) {
        foreach_value_in_variant_set(value, val)
            if (is_in_set(container, val)) {
                if(!purc_variant_array_append(result, val)) {
                    purc_variant_unref(result);
                    return false;
                }
            }
        end_foreach;
    }
    else if (purc_variant_is_array(value)) {
        foreach_value_in_variant_array(value, val)
            if (is_in_set(container, val)) {
                if(!purc_variant_array_append(result, val)) {
                    purc_variant_unref(result);
                    return false;
                }
            }
        end_foreach;
    }

    bool ret = set_displace(container, result, silent);
    purc_variant_unref(result);
    return ret;
}

bool
purc_variant_container_subtract(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(silent);
    if (container == PURC_VARIANT_INVALID || value == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return false;
    }

    if (!purc_variant_is_set(container)) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return false;
    }

    purc_variant_t result = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (result == PURC_VARIANT_INVALID) {
        return false;
    }

    purc_variant_t val;
    if (purc_variant_is_set(value)) {
        foreach_value_in_variant_set(container, val)
            if (!is_in_set(value, val)) {
                if(!purc_variant_array_append(result, val)) {
                    purc_variant_unref(result);
                    return false;
                }
            }
        end_foreach;
    }
    else if (purc_variant_is_array(value)) {
        foreach_value_in_variant_set(container, val)
            if (!is_in_array(value, val)) {
                if(!purc_variant_array_append(result, val)) {
                    purc_variant_unref(result);
                    return false;
                }
            }
        end_foreach;
    }

    bool ret = set_displace(container, result, silent);
    purc_variant_unref(result);
    return ret;
}

bool
purc_variant_container_xor(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(container);
    UNUSED_PARAM(value);
    UNUSED_PARAM(silent);
    return false;
}

bool
purc_variant_container_overwrite(purc_variant_t container,
        purc_variant_t value, bool silent)
{
    UNUSED_PARAM(container);
    UNUSED_PARAM(value);
    UNUSED_PARAM(silent);
    return false;
}

