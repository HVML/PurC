/**
 * @file element.c
 * @author Xu Xiaohong
 * @date 2021/11/18
 * @brief The internal interfaces for interpreter/element
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
 */

#include "config.h"

#include "element.h"
#include "private/interpreter.h"

static inline bool
obj_add_element(purc_variant_t obj, struct pcedom_element *elem)
{
    uint64_t u64 = (uint64_t)elem;
    purc_variant_t v;
    v = purc_variant_make_ulongint(u64);
    if (v == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    bool ok = purc_variant_object_set_by_static_ckey(obj, "__elem", v);
    if (!ok) {
        purc_variant_unref(v);
        return false;
    }

    return true;
}

static inline struct pcedom_element*
attr_get_element(purc_variant_t root)
{
    PC_ASSERT(root != PURC_VARIANT_INVALID);

    purc_variant_t v_elem;
    v_elem = purc_variant_object_get_by_ckey(root, "__elem");
    PC_ASSERT(v_elem != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_ulongint(v_elem));

    uint64_t u64;
    bool ok;
    ok = purc_variant_cast_to_ulongint(v_elem, &u64, false);
    PC_ASSERT(ok);
    PC_ASSERT(u64);

    struct pcedom_element *elem;
    elem = (struct pcedom_element*)u64;

    return elem;
}

static inline purc_variant_t
attr_get_first(purc_variant_t root)
{
    struct pcedom_element *elem = attr_get_element(root);
    PC_ASSERT(elem);
    // TODO: get first attr value from elem
    PC_ASSERT(0);
    return PURC_VARIANT_INVALID;
}

static inline purc_variant_t
attr_get_by_name(purc_variant_t root, purc_variant_t name)
{
    struct pcedom_element *elem = attr_get_element(root);
    PC_ASSERT(elem);

    if (name == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_string(name)) {
        pcinst_set_error(PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char *s_name = purc_variant_get_string_const(name);
    PC_ASSERT(s_name);
    // TODO: get attr value from elem by s_name
    PC_ASSERT(0);
    return PURC_VARIANT_INVALID;
}

static inline purc_variant_t
attr_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    if (nr_args == 0) {
        if (argv != NULL) {
            pcinst_set_error(PURC_ERROR_WRONG_ARGS);
            return PURC_VARIANT_INVALID;
        }
        return attr_get_first(root);
    }

    if (nr_args == 1) {
        if (argv == NULL || argv[0] == PURC_VARIANT_INVALID) {
            pcinst_set_error(PURC_ERROR_WRONG_ARGS);
            return PURC_VARIANT_INVALID;
        }
        purc_variant_t v = argv[0];
        if (!purc_variant_is_string(v)) {
            pcinst_set_error(PURC_ERROR_WRONG_ARGS);
            return PURC_VARIANT_INVALID;
        }
        return attr_get_by_name(root, argv[0]);
    }

    return PURC_VARIANT_INVALID;
}

static inline purc_variant_t
style_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    PC_ASSERT(0); // Not implemented yet
    return PURC_VARIANT_INVALID;
}

static inline purc_variant_t
make_element(struct pcedom_element *elem)
{
    struct pcintr_dynamic_args args[] = {
        {"attr",     attr_getter,    NULL},
        {"style",    style_getter,   NULL},
    };

    purc_variant_t obj;
    obj = pcintr_make_object_of_dynamic_variants(PCA_TABLESIZE(args), args);
    if (obj == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    bool ok = obj_add_element(obj, elem);
    if (!ok) {
        purc_variant_unref(obj);
        return PURC_VARIANT_INVALID;
    }

    return obj;
}

static inline bool
set_add_element(purc_variant_t set, struct pcedom_element *elem)
{
    purc_variant_t v;
    v = make_element(elem);
    if (v == PURC_VARIANT_INVALID)
        return false;

    bool ok;
    ok = purc_variant_set_add(set, v, true); // FIXME: true or false!!!!
    if (!ok) {
        purc_variant_unref(v);
        return false;
    }

    return true;
}

static inline bool
set_make_elements(purc_variant_t set,
        size_t nr_elems, struct pcedom_element **elems)
{
    for (size_t i=0; i<nr_elems; ++i) {
        struct pcedom_element *elem;
        elem = elems[i];
        if (!set_add_element(set, elem))
            return false;
    }
    return true;
}

purc_variant_t
pcintr_make_elements(size_t nr_elems, struct pcedom_element **elems)
{
    purc_variant_t v;
    v = purc_variant_make_set_by_ckey(0, NULL, PURC_VARIANT_INVALID);
    if (v == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    bool ok = set_make_elements(v, nr_elems, elems);
    if (!ok) {
        purc_variant_unref(v);
        return PURC_VARIANT_INVALID;
    }

    return v;
}

