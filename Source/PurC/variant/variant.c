/*
 * @file variant.c
 * @author Geng Yue, Vincent Wei
 * @date 2021/07/02
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

#include "private/atom-buckets.h"
#include "private/variant.h"
#include "private/instance.h"
#include "private/ejson.h"
#include "private/vcm.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/dvobjs.h"
#include "private/utils.h"
#include "variant-internals.h"

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <float.h>

#if OS(LINUX) || OS(UNIX)
    #include <dlfcn.h>
#endif

#if HAVE(GLIB)
    #include <gmodule.h>
#endif

#include "variant_err_msgs.inc"

static pcvariant_release_fn variant_releasers[PURC_VARIANT_TYPE_NR] = {
    NULL,                           // PURC_VARIANT_TYPE_UNDEFINED
    NULL,                           // PURC_VARIANT_TYPE_NULL
    NULL,                           // PURC_VARIANT_TYPE_BOOLEAN
    NULL,                           // PURC_VARIANT_TYPE_EXCEPTION
    NULL,                           // PURC_VARIANT_TYPE_NUMBER
    NULL,                           // PURC_VARIANT_TYPE_LONGINT
    NULL,                           // PURC_VARIANT_TYPE_ULONGINT
    NULL,                           // PURC_VARIANT_TYPE_LONGDOUBLE
    NULL,                           // PURC_VARIANT_TYPE_ATOM_STRING
    pcvariant_string_release,       // PURC_VARIANT_TYPE_STRING
    pcvariant_sequence_release,     // PURC_VARIANT_TYPE_SEQUENCE
    NULL,                           // PURC_VARIANT_TYPE_DYNAMIC
    pcvariant_native_release,       // PURC_VARIANT_TYPE_NATIVE
    pcvariant_object_release,       // PURC_VARIANT_TYPE_OBJECT
    pcvariant_array_release,        // PURC_VARIANT_TYPE_ARRAY
    pcvariant_set_release,          // PURC_VARIANT_TYPE_SET
    pcvariant_tuple_release,        // PURC_VARIANT_TYPE_TUPLE
};


/* Make sure the number of error messages matches the number of error codes */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(msgs,
        PCA_TABLESIZE(variant_err_msgs) == PCVRNT_ERROR_NR);

#undef _COMPILE_TIME_ASSERT

static struct err_msg_seg _variant_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_FIRST_VARIANT,
    PURC_ERROR_FIRST_VARIANT + PCA_TABLESIZE(variant_err_msgs) - 1,
    variant_err_msgs
};

#if HAVE(GLIB)
purc_variant *pcvariant_alloc(void) {
    return (purc_variant *)g_slice_alloc(sizeof(purc_variant));
}

purc_variant *pcvariant_alloc_0(void) {
    return (purc_variant *)g_slice_alloc0(sizeof(purc_variant));
}

void pcvariant_free(purc_variant *v) {
    return g_slice_free1(sizeof(purc_variant), (gpointer)v);
}
#else
purc_variant *pcvariant_alloc(void) {
    return (purc_variant *)malloc(sizeof(purc_variant));
}

purc_variant *pcvariant_alloc_0(void) {
    return (purc_variant *)calloc(1, sizeof(purc_variant));
}

void pcvariant_free(purc_variant *v) {
    return free(v);
}
#endif

purc_atom_t pcvariant_atom_grow;
purc_atom_t pcvariant_atom_shrink;
purc_atom_t pcvariant_atom_change;
// purc_atom_t pcvariant_atom_reference;
// purc_atom_t pcvariant_atom_unreference;

static int _init_once(void)
{
    // register error message
    pcinst_register_error_message_segment(&_variant_err_msgs_seg);

    // initialize others
    pcvariant_atom_grow = purc_atom_from_static_string_ex(ATOM_BUCKET_MSG,
        "grow");
    pcvariant_atom_shrink = purc_atom_from_static_string_ex(ATOM_BUCKET_MSG,
        "shrink");
    pcvariant_atom_change = purc_atom_from_static_string_ex(ATOM_BUCKET_MSG,
        "change");

    return 0;
}

static void _cleanup_instance(struct pcinst *inst)
{
    struct pcvariant_heap *heap = inst->variant_heap;

    if (inst->variables) {
        pcvarmgr_destroy(inst->variables);
        inst->variables = NULL;
    }

    if (inst->dvobjs) {
        size_t nr = pcutils_array_length(inst->dvobjs);
        for (size_t i = 0; i < nr; i++) {
            purc_variant_t v = pcutils_array_get(inst->dvobjs, i);
            purc_variant_unload_dvobj(v);
        }
        pcutils_array_destroy(inst->dvobjs, true);
        inst->dvobjs = NULL;
    }

    if (heap == NULL)
        return;

    /* VWNOTE: do not try to release the extra memory here. */
#if USE(LOOP_BUFFER_FOR_RESERVED)
    for (int i = 0; i < MAX_RESERVED_VARIANTS; i++) {
        if (heap->v_reserved[i]) {
            pcvariant_free(heap->v_reserved[i]);
            heap->v_reserved[i] = NULL;
        }
    }
    heap->headpos = 0;
    heap->tailpos = 0;
#else
    struct list_head *p, *n;
    list_for_each_safe(p, n, &heap->v_reserved) {
        purc_variant_t v = list_entry(p, struct purc_variant, reserved);

        list_del(p);
        pcvariant_free(v);
    }
#endif

    assert(heap->v_undefined.refc == 0);
    assert(heap->v_null.refc == 0);
    assert(heap->v_true.refc == 0);
    assert(heap->v_false.refc == 0);

    free(heap);
    inst->variant_heap = NULL;
    inst->org_vrt_heap = NULL;
}

static int _init_instance(struct pcinst *curr_inst,
        const purc_instance_extra_info* extra_info)
{
    UNUSED_PARAM(extra_info);

    struct pcinst *inst = curr_inst;

    inst->variant_heap = calloc(1, sizeof(*inst->variant_heap));
    if (inst->variant_heap == NULL) {
        return PURC_ERROR_OUT_OF_MEMORY;
    }

    inst->org_vrt_heap = inst->variant_heap;

    // initialize const values in instance
    inst->variant_heap->v_undefined.type = PURC_VARIANT_TYPE_UNDEFINED;
    inst->variant_heap->v_undefined.refc = 0;
    inst->variant_heap->v_undefined.flags = PCVRNT_FLAG_NOFREE;
    // INIT_LIST_HEAD(&inst->variant_heap->v_undefined.listeners);

    inst->variant_heap->v_null.type = PURC_VARIANT_TYPE_NULL;
    inst->variant_heap->v_null.refc = 0;
    inst->variant_heap->v_null.flags = PCVRNT_FLAG_NOFREE;
    // INIT_LIST_HEAD(&inst->variant_heap->v_null.listeners);

    inst->variant_heap->v_false.type = PURC_VARIANT_TYPE_BOOLEAN;
    inst->variant_heap->v_false.refc = 0;
    inst->variant_heap->v_false.flags = PCVRNT_FLAG_NOFREE;
    inst->variant_heap->v_false.b = false;
    // INIT_LIST_HEAD(&inst->variant_heap->v_false.listeners);

    inst->variant_heap->v_true.type = PURC_VARIANT_TYPE_BOOLEAN;
    inst->variant_heap->v_true.refc = 0;
    inst->variant_heap->v_true.flags = PCVRNT_FLAG_NOFREE;
    inst->variant_heap->v_true.b = true;
    // INIT_LIST_HEAD(&inst->variant_heap->v_true.listeners);

    /* XXX: there are two values of boolean.  */
    struct purc_variant_stat *stat = &(inst->variant_heap->stat);
    stat->nr_values[PURC_VARIANT_TYPE_UNDEFINED] = 0;
    stat->sz_mem[PURC_VARIANT_TYPE_UNDEFINED] = sizeof(purc_variant);
    stat->nr_values[PURC_VARIANT_TYPE_NULL] = 0;
    stat->sz_mem[PURC_VARIANT_TYPE_NULL] = sizeof(purc_variant);
    stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN] = 0;
    stat->sz_mem[PURC_VARIANT_TYPE_BOOLEAN] = sizeof(purc_variant) * 2;
    stat->nr_total_values = 4;
    stat->sz_total_mem = 4 * sizeof(purc_variant);

    stat->nr_reserved = 0;
    stat->nr_max_reserved = MAX_RESERVED_VARIANTS;

#if !USE(LOOP_BUFFER_FOR_RESERVED)
    INIT_LIST_HEAD(&inst->variant_heap->v_reserved);
#endif

    return PURC_ERROR_OK;
}

struct pcmodule _module_variant = {
    .id              = PURC_HAVE_VARIANT,
    .module_inited   = 0,

    .init_once          = _init_once,
    .init_instance      = _init_instance,
    .cleanup_instance   = _cleanup_instance,
};


static const char *typenames[] = {
    PURC_VARIANT_TYPE_NAME_UNDEFINED,
    PURC_VARIANT_TYPE_NAME_NULL,
    PURC_VARIANT_TYPE_NAME_BOOLEAN,
    PURC_VARIANT_TYPE_NAME_EXCEPTION,
    PURC_VARIANT_TYPE_NAME_NUMBER,
    PURC_VARIANT_TYPE_NAME_LONGINT,
    PURC_VARIANT_TYPE_NAME_ULONGINT,
    PURC_VARIANT_TYPE_NAME_LONGDOUBLE,
    PURC_VARIANT_TYPE_NAME_ATOMSTRING,
    PURC_VARIANT_TYPE_NAME_STRING,
    PURC_VARIANT_TYPE_NAME_BYTESEQUENCE,
    PURC_VARIANT_TYPE_NAME_DYNAMIC,
    PURC_VARIANT_TYPE_NAME_NATIVE,
    PURC_VARIANT_TYPE_NAME_OBJECT,
    PURC_VARIANT_TYPE_NAME_ARRAY,
    PURC_VARIANT_TYPE_NAME_SET,
    PURC_VARIANT_TYPE_NAME_TUPLE,
};

/* Make sure the number of variant types matches the size of `type_names` */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(types, PCA_TABLESIZE(typenames) == PURC_VARIANT_TYPE_NR);
#undef _COMPILE_TIME_ASSERT

size_t
purc_variant_wrapper_size(void)
{
    return sizeof(purc_variant);
}

const char* purc_variant_typename(enum purc_variant_type type)
{
    assert(type >= 0 && type < PURC_VARIANT_TYPE_NR);
    return typenames[type];
}

bool purc_variant_is_type(purc_variant_t value, enum purc_variant_type type)
{
    PC_ASSERT(value);
    return (value->type == type);
}

enum purc_variant_type purc_variant_get_type(purc_variant_t value)
{
    return value->type;
}

bool pcvariant_is_mutable(purc_variant_t val)
{
    switch (val->type) {
        case PURC_VARIANT_TYPE_ARRAY:
        case PURC_VARIANT_TYPE_OBJECT:
        case PURC_VARIANT_TYPE_SET:
        case PURC_VARIANT_TYPE_TUPLE:
            return true;
        default:
            return false;
    }
}

static void
referenced(purc_variant_t value)
{
    // pcvariant_on_post_fired(value, pcvariant_atom_reference, 0, NULL);
    UNUSED_PARAM(value);
}

static void
unreferenced(purc_variant_t value)
{
    // pcvariant_on_post_fired(value, pcvariant_atom_unreference, 0, NULL);
    UNUSED_PARAM(value);
}

unsigned int
purc_variant_ref_count(purc_variant_t value)
{
    PC_ASSERT(value);

    /* this should not occur */
    if (UNLIKELY(value->refc == 0)) {
        PC_ASSERT(0);
        return 0;
    }

    return value->refc;
}

purc_variant_t purc_variant_ref (purc_variant_t value)
{
    PC_ASSERT(value);

    /* this should not occur */
    if (UNLIKELY(value->refc == 0)) {
        PC_ASSERT(0);
        return PURC_VARIANT_INVALID;
    }

    value->refc++;

    referenced(value);

    return value;
}

unsigned int purc_variant_unref(purc_variant_t value)
{
    PC_ASSERT(value);

    /* this should not occur */
    if (UNLIKELY(value->refc == 0)) {
        PC_ASSERT(0);
        return 0;
    }

    // FIXME: pre or post?
    unreferenced(value);

    value->refc--;

    // VWNOTE: only non-constant values has a releaser
    if (value->refc == 0 && !(value->flags & PCVRNT_FLAG_NOFREE)) {
        // release the extra memory used by the variant
        pcvariant_release_fn release_fn = variant_releasers[value->type];
        if (release_fn)
            release_fn(value);

        // release the variant itself
        pcvariant_put(value);
        return 0;
    }

    return value->refc;
}

const struct purc_variant_stat *purc_variant_usage_stat(void)
{
    struct pcinst *inst = pcinst_current();
    PC_ASSERT(inst);

    purc_variant_t value = &(inst->variant_heap->v_undefined);
    inst->variant_heap->stat.nr_values[PURC_VARIANT_TYPE_UNDEFINED] = value->refc;

    value = &(inst->variant_heap->v_null);
    inst->variant_heap->stat.nr_values[PURC_VARIANT_TYPE_NULL] = value->refc;

    value = &(inst->variant_heap->v_true);
    inst->variant_heap->stat.nr_values[PURC_VARIANT_TYPE_BOOLEAN] = value->refc;

    value = &(inst->variant_heap->v_false);
    inst->variant_heap->stat.nr_values[PURC_VARIANT_TYPE_BOOLEAN] += value->refc;

    return &inst->variant_heap->stat;
}

void pcvariant_stat_set_extra_size(purc_variant_t value, size_t extra_size)
{
    struct pcinst *instance = pcinst_current();

    PC_ASSERT(value);
    PC_ASSERT(instance);

    struct purc_variant_stat *stat = &(instance->variant_heap->stat);
    int type = value->type;

    if (value->flags & PCVRNT_FLAG_EXTRA_SIZE) {
        stat->sz_mem[type] -= value->sz_ptr[0];
        stat->sz_total_mem -= value->sz_ptr[0];

        value->sz_ptr[0] = extra_size;

        stat->sz_mem[type] += extra_size;
        stat->sz_total_mem += extra_size;
    }
}

purc_variant_t pcvariant_get(enum purc_variant_type type)
{
    purc_variant_t value = NULL;
    struct pcinst *instance = pcinst_current();
    struct pcvariant_heap *heap = instance->variant_heap;
    struct purc_variant_stat *stat = &(heap->stat);

#if USE(LOOP_BUFFER_FOR_RESERVED)
    if (heap->headpos == heap->tailpos) {
        // no reserved, allocate one
        value = pcvariant_alloc_0();
        if (value == NULL)
            return PURC_VARIANT_INVALID;

        stat->sz_mem[type] += sizeof(purc_variant);
        stat->sz_total_mem += sizeof(purc_variant);
    }
    else {
        value = heap->v_reserved[heap->tailpos];
        value->sz_ptr[0] = 0;

        // VWNOTE: set the slot as NULL
        heap->v_reserved[heap->tailpos] = NULL;
        heap->tailpos = (heap->tailpos + 1) % MAX_RESERVED_VARIANTS;

        /* VWNOTE: do not forget to set nr_reserved. */
        stat->nr_reserved--;
    }
#else
    if (list_empty(&heap->v_reserved)) {
        // no reserved, allocate one
        value = pcvariant_alloc_0();
        if (value == NULL)
            return PURC_VARIANT_INVALID;

        stat->sz_mem[type] += sizeof(purc_variant);
        stat->sz_total_mem += sizeof(purc_variant);
    }
    else {
        value = list_first_entry(&heap->v_reserved, purc_variant, reserved);
        value->sz_ptr[0] = 0;

        list_del(&value->reserved);

        /* VWNOTE: do not forget to set nr_reserved. */
        stat->nr_reserved--;
    }
#endif

    // set stat information
    stat->nr_values[type]++;
    stat->nr_total_values++;

    // init listeners
    INIT_LIST_HEAD(&value->listeners);

    return value;
}

void pcvariant_put(purc_variant_t value)
{
    struct pcinst *instance = pcinst_current();
    struct pcvariant_heap *heap = instance->variant_heap;
    struct purc_variant_stat *stat = &(heap->stat);

    PC_ASSERT(value);
    if (IS_CONTAINER(value->type)) {
        PC_ASSERT(list_empty(&value->listeners));
    }

    // set stat information
    stat->nr_values[value->type]--;
    stat->nr_total_values--;

#if USE(LOOP_BUFFER_FOR_RESERVED)
    if ((heap->headpos + 1) % MAX_RESERVED_VARIANTS == heap->tailpos) {
        stat->sz_mem[value->type] -= sizeof(purc_variant);
        stat->sz_total_mem -= sizeof(purc_variant);

        pcvariant_free(value);
    }
    else {
        heap->v_reserved[heap->headpos] = value;
        heap->headpos = (heap->headpos + 1) % MAX_RESERVED_VARIANTS;

        /* VWNOTE: do not forget to set nr_reserved. */
        stat->nr_reserved++;
    }
#else
    if (stat->nr_reserved >= stat->nr_max_reserved) {
        stat->sz_mem[value->type] -= sizeof(purc_variant);
        stat->sz_total_mem -= sizeof(purc_variant);

        pcvariant_free(value);
    }
    else {
        list_add_tail(&value->reserved, &heap->v_reserved);

        /* VWNOTE: do not forget to set nr_reserved. */
        stat->nr_reserved++;
    }
#endif
}

/* securely comparison of floating-point variables */
static bool equal_doubles(double a, double b)
{
    double max_val = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
    return (fabs(a - b) <= max_val * DBL_EPSILON);
}

/* securely comparison of floating-point variables */
static bool equal_long_doubles(long double a, long double b)
{
    long double max_val = fabsl(a) > fabsl(b) ? fabsl(a) : fabsl(b);
    return (fabsl(a - b) <= max_val * LDBL_EPSILON);
}

static bool equal_objects(purc_variant_t v1, purc_variant_t v2)
{
    purc_variant_t key;
    purc_variant_t m1, m2;
    size_t sz1 = purc_variant_object_get_size(v1);
    size_t sz2 = purc_variant_object_get_size(v2);

    if (sz1 != sz2)
        return false;

    foreach_key_value_in_variant_object(v1, key, m1)
        m2 = purc_variant_object_get(v2, key);
        if (!purc_variant_is_equal_to(m1, m2))
            return false;
    end_foreach;

    return true;
}

static bool equal_arrays(purc_variant_t v1, purc_variant_t v2)
{
    size_t idx;
    size_t sz1 = purc_variant_array_get_size(v1);
    size_t sz2 = purc_variant_array_get_size(v2);
    purc_variant_t m1, m2;

    if (sz1 != sz2)
        return false;

    idx = 0;
    size_t curr;
    foreach_value_in_variant_array(v1, m1, curr)
        (void)curr;
        m2 = purc_variant_array_get(v2, idx);
        if (!purc_variant_is_equal_to(m1, m2))
            return false;
        idx++;
    end_foreach;

    return true;
}

static bool equal_sets(purc_variant_t v1, purc_variant_t v2)
{
    size_t sz1 = purc_variant_set_get_size(v1);
    size_t sz2 = purc_variant_set_get_size(v2);
    struct pcvrnt_set_iterator* it;
    purc_variant_t m1, m2;

    if (sz1 != sz2)
        return false;

    it = pcvrnt_set_iterator_create_begin(v2);
    foreach_value_in_variant_set_order(v1, m1)

        m2 = pcvrnt_set_iterator_get_value(it);
        if (!purc_variant_is_equal_to(m1, m2))
            goto not_equal;

        pcvrnt_set_iterator_next(it);
    end_foreach;

    pcvrnt_set_iterator_release(it);
    return true;

not_equal:
    pcvrnt_set_iterator_release(it);
    return false;
}

static bool equal_tuples(purc_variant_t v1, purc_variant_t v2)
{
    purc_variant_t *members1, *members2;
    size_t sz1, sz2;

    members1 = tuple_members(v1, &sz1);
    members2 = tuple_members(v2, &sz2);
    assert(members1 && members2);

    if (sz1 != sz2)
        return false;

    for (size_t n = 0; n < sz1; n++) {
        if (!purc_variant_is_equal_to(members1[n], members2[n]))
            return false;
    }

    return true;
}

bool purc_variant_is_equal_to(purc_variant_t v1, purc_variant_t v2)
{
    const char *str1, *str2;
    size_t len1, len2;

    if (v1 == NULL)
        return v2 ? false : true;
    if (v2 == NULL)
        return v1 ? false : true;

    if (v1->type != v2->type) {
        return false;
    }

    switch (v1->type) {
        case PURC_VARIANT_TYPE_UNDEFINED:
        case PURC_VARIANT_TYPE_NULL:
            return true;

        case PURC_VARIANT_TYPE_BOOLEAN:
            return v1->b == v2->b;

        case PURC_VARIANT_TYPE_EXCEPTION:
            return v1->atom == v2->atom;

        case PURC_VARIANT_TYPE_NUMBER:
            return equal_doubles(v1->d, v2->d);

        case PURC_VARIANT_TYPE_LONGINT:
            return v1->i64 == v2->i64;

        case PURC_VARIANT_TYPE_ULONGINT:
            return v1->u64 == v2->u64;

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            return equal_long_doubles(v1->ld, v2->ld);

        case PURC_VARIANT_TYPE_ATOMSTRING:
            str1 = purc_atom_to_string(v1->atom);
            str2 = purc_atom_to_string(v2->atom);
            return strcmp(str1, str2) == 0;

        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (v1->flags & PCVRNT_FLAG_STRING_STATIC) {
                str1 = (const char*)v1->sz_ptr[1];
                len1 = v1->sz_ptr[0];
            }
            else if (v1->flags & PCVRNT_FLAG_EXTRA_SIZE) {
                str1 = (const char*)v1->sz_ptr[1];
                len1 = v1->sz_ptr[0];
            }
            else {
                str1 = (const char*)v1->bytes;
                len1 = v1->size;
            }

            if (v2->flags & PCVRNT_FLAG_STRING_STATIC) {
                str2 = (const char*)v2->sz_ptr[1];
                len2 = v2->sz_ptr[0];
            }
            else if (v2->flags & PCVRNT_FLAG_EXTRA_SIZE) {
                str2 = (const char*)v2->sz_ptr[1];
                len2 = v2->sz_ptr[0];
            }
            else {
                str2 = (const char*)v2->bytes;
                len2 = v2->size;
            }

            return (len1 == len2 && memcmp(str1, str2, len1) == 0);

        case PURC_VARIANT_TYPE_DYNAMIC:
        case PURC_VARIANT_TYPE_NATIVE:
            return memcmp(v1->ptr_ptr, v2->ptr_ptr, sizeof(void *) * 2) == 0;

        case PURC_VARIANT_TYPE_OBJECT:
            return equal_objects(v1, v2);

        case PURC_VARIANT_TYPE_ARRAY:
            return equal_arrays(v1, v2);

        case PURC_VARIANT_TYPE_SET:
            return equal_sets(v1, v2);

        case PURC_VARIANT_TYPE_TUPLE:
            return equal_tuples(v1, v2);
    }

    return false;

#if 0
    if (0) {
        long double ld1, ld2;
        if (purc_variant_cast_to_longdouble(v1, &ld1, false) &&
                purc_variant_cast_to_longdouble(v2, &ld2, false)) {
            if (equal_long_doubles(ld1, ld2))
                return 0;

            // VWNOTE: this may get zero because of too small difference:
            // return (int)(ld1 - ld2);
            if (ld1 > ld2)
                return 1;
            return -1;
        }

        const void *bytes1, *bytes2;
        size_t sz1, sz2;
        if (purc_variant_cast_to_byte_sequence(v1, &bytes1, &sz1) &&
                purc_variant_cast_to_byte_sequence(v2, &bytes2, &sz2)) {

            int i = memcmp(bytes1, bytes2, (sz1 > sz2) ? sz2 : sz1);
            if (i == 0 && sz2 != sz1) {
                return (int)(sz1 - sz2);
            }
            return i;
        }

        static const char* type_names[PURC_VARIANT_TYPE_NR] = {
            "undefined",        // PURC_VARIANT_TYPE_UNDEFINED
            "null",             // PURC_VARIANT_TYPE_NULL
            "boolean",          // PURC_VARIANT_TYPE_BOOLEAN
            "0",                // PURC_VARIANT_TYPE_EXCEPTION
            "0",                // PURC_VARIANT_TYPE_NUMBER
            "0",                // PURC_VARIANT_TYPE_LONGINT
            "0",                // PURC_VARIANT_TYPE_ULONGINT
            "0",                // PURC_VARIANT_TYPE_LONGDOUBLE
            "\"\"",             // PURC_VARIANT_TYPE_ATOM_STRING
            "\"\"",             // PURC_VARIANT_TYPE_STRING
            "b",                // PURC_VARIANT_TYPE_SEQUENCE
            "<dynamic>",        // PURC_VARIANT_TYPE_DYNAMIC
            "<native>",         // PURC_VARIANT_TYPE_NATIVE
            "{}",               // PURC_VARIANT_TYPE_OBJECT
            "[]",               // PURC_VARIANT_TYPE_ARRAY
            "[]",               // PURC_VARIANT_TYPE_TUPLE
            "[<set>]",          // PURC_VARIANT_TYPE_SET
        };

        return strcmp(type_names[v1->type], type_names[v2->type]);
    }
#endif
}

bool
purc_variant_is_true(purc_variant_t v)
{
    PC_ASSERT(v);

    if (v->type == PURC_VARIANT_TYPE_BOOLEAN) {
        return v->b;
    }

    return false;
}

bool
purc_variant_is_false(purc_variant_t v)
{
    PC_ASSERT(v);

    if (v->type == PURC_VARIANT_TYPE_BOOLEAN) {
        return !v->b;
    }

    return false;
}

bool
purc_variant_cast_to_int32(purc_variant_t v, int32_t *i32, bool force)
{
    const char *bytes;
    size_t sz;

    PC_ASSERT(v);

    switch (v->type) {
        case PURC_VARIANT_TYPE_NULL:
            if (force) {
                *i32 = 0;
                return true;
            }
            break;

        case PURC_VARIANT_TYPE_BOOLEAN:
            if (force) {
                *i32 = (int32_t)v->b;
                return true;
            }
            break;

        case PURC_VARIANT_TYPE_NUMBER:
            if (isnan(v->d))
                break;

            if (isinf(v->d) == -1 || v->d < INT32_MIN) {
                if (force)
                    *i32 = INT32_MIN;
                else
                    break;
            }
            else if (isinf(v->d) == 1 || v->d > INT32_MAX) {
                if (force)
                    *i32 = INT32_MAX;
                else
                    break;
            }
            else {
                *i32 = (int32_t)v->d;
            }
            return true;

        case PURC_VARIANT_TYPE_LONGINT:
            if (force || (v->i64 >= INT32_MIN && v->i64 <= INT32_MAX))
                *i32 = (int32_t)v->i64;
            else
                break;
            return true;

        case PURC_VARIANT_TYPE_ULONGINT:
            if (v->u64 > INT32_MAX) {
                if (force)
                    *i32 = (int32_t)v->u64;
                else
                    break;
            }
            else {
                *i32 = (int32_t)v->u64;
            }
            return true;

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            if (isnan(v->ld))
                break;

            if (isinf(v->d) == -1 || v->ld < INT32_MIN) {
                if (force)
                    *i32 = INT32_MIN;
                else
                    break;
            }
            else if (isinf(v->d) == 1 || v->ld > INT32_MAX) {
                if (force)
                    *i32 = INT32_MAX;
                else
                    break;
            }
            else {
                *i32 = (int32_t)v->ld;
            }
            return true;

        case PURC_VARIANT_TYPE_ATOMSTRING:
            if (!force)
                break;

            bytes = purc_atom_to_string(v->atom);
            sz = strlen(bytes);
            if (pcutils_parse_int32(bytes, sz, i32) != 0) {
                *i32 = 0;
            }
            return true;

        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (!force)
                break;

            if (v->flags & PCVRNT_FLAG_STRING_STATIC) {
                bytes = (void*)v->sz_ptr[1];
                sz = strlen((const char*)bytes);
            }
            else if (v->flags & PCVRNT_FLAG_EXTRA_SIZE) {
                bytes = (void*)v->sz_ptr[1];
                sz = v->sz_ptr[0];
            }
            else {
                bytes = (void*)v->bytes;
                sz = v->size;
            }
            /* FIXME: DONOT call this function directly for bsequence! */
            if (pcutils_parse_int32(bytes, sz, i32) != 0) {
                *i32 = 0;
            }
            return true;

        default:
            break;
    }

    return false;
}

bool
purc_variant_cast_to_uint32(purc_variant_t v, uint32_t *u32, bool force)
{
    const char *bytes;
    size_t sz;

    PC_ASSERT(v);

    switch (v->type) {
        case PURC_VARIANT_TYPE_NULL:
            if (force) {
                *u32 = 0;
                return true;
            }
            break;

        case PURC_VARIANT_TYPE_BOOLEAN:
            if (force) {
                *u32 = (uint32_t)v->b;
                return true;
            }
            break;

        case PURC_VARIANT_TYPE_NUMBER:
            if (isnan(v->d))
                break;

            if (isinf(v->d) == -1 || v->d < 0) {
                if (force)
                    *u32 = 0;
                else
                    break;
            }
            else if (isinf(v->d) == 1 || v->d > UINT32_MAX) {
                if (force)
                    *u32 = UINT32_MAX;
                else
                    break;
            }
            else {
                *u32 = (uint32_t)v->d;
            }
            return true;

        case PURC_VARIANT_TYPE_LONGINT:
            if (v->i64 < 0) {
                if (force)
                    *u32 = (uint32_t)v->i64;
                else
                    break;
            }
            else
                *u32 = (uint32_t)v->i64;
            return true;

        case PURC_VARIANT_TYPE_ULONGINT:
            if (force || v->u64 < UINT32_MAX)
                *u32 = (uint32_t)v->u64;
            else
                break;

            return true;

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            if (isnan(v->ld))
                break;

            if (isinf(v->ld) == -1 || v->ld < 0) {
                if (force)
                    *u32 = 0;
                else
                    break;
            }
            else if (isinf(v->ld) == 1 || v->ld > UINT32_MAX) {
                if (force)
                    *u32 = UINT32_MAX;
                else
                    break;
            }
            else {
                *u32 = (uint32_t)v->ld;
            }
            return true;

        case PURC_VARIANT_TYPE_ATOMSTRING:
            if (!force)
                break;

            bytes = purc_atom_to_string(v->atom);
            sz = strlen(bytes);
            if (pcutils_parse_uint32(bytes, sz, u32) != 0) {
                *u32 = 0;
            }
            return true;

        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (!force)
                break;

            if (v->flags & PCVRNT_FLAG_STRING_STATIC) {
                bytes = (void*)v->sz_ptr[1];
                sz = strlen((const char*)bytes);
            }
            else if (v->flags & PCVRNT_FLAG_EXTRA_SIZE) {
                bytes = (void*)v->sz_ptr[1];
                sz = v->sz_ptr[0];
            }
            else {
                bytes = (void*)v->bytes;
                sz = v->size;
            }

            /* FIXME: DONOT call this function directly for bsequence! */
            if (pcutils_parse_uint32(bytes, sz, u32) != 0) {
                *u32 = 0;
            }
            return true;

        default:
            break;
    }

    return false;
}

bool
purc_variant_cast_to_longint(purc_variant_t v, int64_t *i64, bool force)
{
    const char *bytes;
    size_t sz;

    PC_ASSERT(v);

    switch (v->type) {
        case PURC_VARIANT_TYPE_NULL:
            if (force) {
                *i64 = 0;
                return true;
            }
            break;

        case PURC_VARIANT_TYPE_BOOLEAN:
            if (force) {
                *i64 = (int64_t)v->b;
                return true;
            }
            break;

        case PURC_VARIANT_TYPE_NUMBER:
            if (isnan(v->d))
                break;

            if (isinf(v->d) == -1 || v->d < INT64_MIN) {
                if (force)
                    *i64 = INT64_MIN;
                else
                    break;
            }
            else if (isinf(v->d) == 1 || v->d > INT64_MAX) {
                if (force)
                    *i64 = INT64_MAX;
                else
                    break;
            }
            else {
                *i64 = (int64_t)v->d;
            }
            return true;

        case PURC_VARIANT_TYPE_LONGINT:
            *i64 = v->i64;
            return true;

        case PURC_VARIANT_TYPE_ULONGINT:
            if (v->u64 > INT64_MAX) {
                if (force)
                    *i64 = (int64_t)v->u64;
                else
                    break;
            }
            else {
                *i64 = (int64_t)v->u64;
            }
            return true;

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            if (isnan(v->ld))
                break;

            if (isinf(v->d) == -1 || v->ld < INT64_MIN) {
                if (force)
                    *i64 = INT64_MIN;
                else
                    break;
            }
            else if (isinf(v->d) == 1 || v->ld > INT64_MAX) {
                if (force)
                    *i64 = INT64_MAX;
                else
                    break;
            }
            else {
                *i64 = (int64_t)v->ld;
            }
            return true;

        case PURC_VARIANT_TYPE_ATOMSTRING:
            if (!force)
                break;

            bytes = purc_atom_to_string(v->atom);
            sz = strlen(bytes);
            if (pcutils_parse_int64(bytes, sz, i64) != 0) {
                *i64 = 0;
            }
            return true;

        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (!force)
                break;

            if (v->flags & PCVRNT_FLAG_STRING_STATIC) {
                bytes = (void*)v->sz_ptr[1];
                sz = strlen((const char*)bytes);
            }
            else if (v->flags & PCVRNT_FLAG_EXTRA_SIZE) {
                bytes = (void*)v->sz_ptr[1];
                sz = v->sz_ptr[0];
            }
            else {
                bytes = (void*)v->bytes;
                sz = v->size;
            }

            /* FIXME: DONOT call this function directly for bsequence! */
            if (pcutils_parse_int64(bytes, sz, i64) != 0) {
                *i64 = 0;
            }
            return true;

        default:
            break;
    }

    return false;
}

bool
purc_variant_cast_to_ulongint(purc_variant_t v, uint64_t *u64, bool force)
{
    const char *bytes;
    size_t sz;

    PC_ASSERT(v);

    switch (v->type) {
        case PURC_VARIANT_TYPE_NULL:
            if (force) {
                *u64 = 0;
                return true;
            }
            break;

        case PURC_VARIANT_TYPE_BOOLEAN:
            if (force) {
                *u64 = (uint64_t)v->b;
                return true;
            }
            break;

        case PURC_VARIANT_TYPE_NUMBER:
            if (isnan(v->d))
                break;

            if (isinf(v->d) == -1 || v->d <= 0) {
                if (force)
                    *u64 = 0;
                else
                    break;
            }
            else if (isinf(v->d) == 1 || v->d >= UINT64_MAX) {
                if (force)
                    *u64 = UINT64_MAX;
                else
                    break;
            }
            else {
                *u64 = (uint64_t)v->d;
            }
            return true;

        case PURC_VARIANT_TYPE_LONGINT:
            if (v->i64 < 0) {
                if (force)
                    *u64 = (uint64_t)v->i64;
                else
                    break;
            }
            else
                *u64 = (uint64_t)v->i64;
            return true;

        case PURC_VARIANT_TYPE_ULONGINT:
            *u64 = v->u64;
            return true;

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            if (isnan(v->ld))
                break;

            if (isinf(v->ld) == -1 || v->ld < 0) {
                if (force)
                    *u64 = 0;
                else
                    break;
            }
            else if (isinf(v->ld) == 1 || v->ld >= UINT64_MAX) {
                *u64 = UINT64_MAX;
            }
            else {
                *u64 = (uint64_t)v->ld;
            }
            return true;

        case PURC_VARIANT_TYPE_ATOMSTRING:
            if (!force)
                break;

            bytes = purc_atom_to_string(v->atom);
            sz = strlen(bytes);
            if (pcutils_parse_uint64(bytes, sz, u64) != 0) {
                *u64 = 0;
            }
            return true;

        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (!force)
                break;

            if (v->flags & PCVRNT_FLAG_STRING_STATIC) {
                bytes = (void*)v->sz_ptr[1];
                sz = strlen((const char*)bytes);
            }
            else if (v->flags & PCVRNT_FLAG_EXTRA_SIZE) {
                bytes = (void*)v->sz_ptr[1];
                sz = v->sz_ptr[0];
            }
            else {
                bytes = (void*)v->bytes;
                sz = v->size;
            }

            /* FIXME: DONOT call this function directly for bsequence! */
            if (pcutils_parse_uint64(bytes, sz, u64) != 0) {
                *u64 = 0;
            }
            return true;

        default:
            break;
    }

    return false;
}

bool purc_variant_cast_to_number(purc_variant_t v, double *d, bool force)
{
    const char *bytes;
    size_t sz;

    PC_ASSERT(v);

    switch (v->type) {
        case PURC_VARIANT_TYPE_NULL:
            if (force) {
                *d = 0;
                return true;
            }
            break;

        case PURC_VARIANT_TYPE_BOOLEAN:
            if (force) {
                *d = (double)v->b;
                return true;
            }
            break;

        case PURC_VARIANT_TYPE_NUMBER:
            *d = (double)v->d;
            return true;

        case PURC_VARIANT_TYPE_LONGINT:
            *d = (double)v->i64;
            return true;

        case PURC_VARIANT_TYPE_ULONGINT:
            *d = (double)v->u64;
            return true;

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            *d = (double)v->ld;
            return true;

        case PURC_VARIANT_TYPE_ATOMSTRING:
            if (!force)
                break;

            bytes = purc_atom_to_string(v->atom);
            sz = strlen(bytes);
            if (pcutils_parse_double(bytes, sz, d) != 0) {
                *d = 0;
            }
            return true;

        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (!force)
                break;

            if (v->flags & PCVRNT_FLAG_STRING_STATIC) {
                bytes = (void*)v->sz_ptr[1];
                sz = strlen((const char*)bytes);
            }
            else if (v->flags & PCVRNT_FLAG_EXTRA_SIZE) {
                bytes = (void*)v->sz_ptr[1];
                sz = v->sz_ptr[0];
            }
            else {
                bytes = (void*)v->bytes;
                sz = v->size;
            }

            /* FIXME: DONOT call this function directly for bsequence! */
            if (pcutils_parse_double(bytes, sz, d) != 0) {
                *d = 0;
            }
            return true;

        default:
            break;
    }

    return false;
}

bool
purc_variant_cast_to_longdouble(purc_variant_t v, long double *d,
        bool force)
{
    const char *bytes;
    size_t sz;

    PC_ASSERT(v);

    switch (v->type) {
        case PURC_VARIANT_TYPE_NULL:
            if (force) {
                *d = 0;
                return true;
            }
            break;

        case PURC_VARIANT_TYPE_BOOLEAN:
            if (force) {
                *d = (long double)v->b;
                return true;
            }
            break;

        case PURC_VARIANT_TYPE_NUMBER:
            *d = (long double)v->d;
            return true;

        case PURC_VARIANT_TYPE_LONGINT:
            *d = (long double)v->i64;
            return true;

        case PURC_VARIANT_TYPE_ULONGINT:
            *d = (long double)v->u64;
            return true;

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            *d = (long double)v->ld;
            return true;

        case PURC_VARIANT_TYPE_ATOMSTRING:
            if (!force)
                break;

            bytes = purc_atom_to_string(v->atom);
            sz = strlen(bytes);
            if (pcutils_parse_long_double(bytes, sz, d) != 0) {
                *d = 0;
            }
            return true;

        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (!force)
                break;

            if (v->flags & PCVRNT_FLAG_STRING_STATIC) {
                bytes = (void*)v->sz_ptr[1];
                sz = strlen((const char*)bytes);
            }
            else if (v->flags & PCVRNT_FLAG_EXTRA_SIZE) {
                bytes = (void*)v->sz_ptr[1];
                sz = v->sz_ptr[0];
            }
            else {
                bytes = (void*)v->bytes;
                sz = v->size;
            }

            /* FIXME: DONOT call this function directly for bsequence! */
            if (pcutils_parse_long_double(bytes, sz, d) != 0) {
                *d = 0;
            }
            return true;

        default:
            break;
    }

    return false;
}

bool purc_variant_cast_to_byte_sequence(purc_variant_t v,
        const void **bytes, size_t *sz)
{
    PC_ASSERT(v);

    switch (v->type) {
        case PURC_VARIANT_TYPE_ATOMSTRING:
            *bytes = purc_atom_to_string(v->atom);
            // NOTE: strlen()+1, in case when purc_variant_compare with string
            *sz = strlen(*bytes) + 1;
            return true;

        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (v->type == PURC_VARIANT_TYPE_STRING &&
                    v->flags & PCVRNT_FLAG_STRING_STATIC) {
                *bytes = (void*)v->sz_ptr[1];
                *sz = v->sz_ptr[0]; // strlen((const char*)*bytes) + 1;
            }
            else if (v->flags & PCVRNT_FLAG_EXTRA_SIZE) {
                *bytes = (void*)v->sz_ptr[1];
                *sz = v->sz_ptr[0];
            }
            else {
                *bytes = (void*)v->bytes;
                *sz = v->size;
            }
            return true;

        default:
            break;
    }

    return false;
}

static int compare_number_method (purc_variant_t v1, purc_variant_t v2)
{
    int ret = 0;
    double number1 = purc_variant_numerify (v1);
    double number2 = purc_variant_numerify (v2);

    if (equal_doubles (number1, number2))
        ret = 0;
    else
        ret = number1 < number2 ? -1: 1;

    return ret;
}

static char *
compare_stringify (purc_variant_t v, char *stackbuffer, size_t size)
{
    char * buffer = NULL;
    size_t total = 0;
    size_t length = 0;
    int num_write = 0;

    switch (v->type) {
        case PURC_VARIANT_TYPE_OBJECT:
        case PURC_VARIANT_TYPE_ARRAY:
        case PURC_VARIANT_TYPE_SET:
        case PURC_VARIANT_TYPE_TUPLE:
            num_write = purc_variant_stringify_alloc (&buffer, v);
            if (num_write < 0) {
                buffer = 0;
                stackbuffer[0] = '\0';
            }
            break;

        case PURC_VARIANT_TYPE_EXCEPTION:
        case PURC_VARIANT_TYPE_ATOMSTRING:
        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (v->type == PURC_VARIANT_TYPE_STRING) {
                length = purc_variant_string_size (v);
                total = length;
            }
            else if (v->type == PURC_VARIANT_TYPE_BSEQUENCE) {
                length = purc_variant_bsequence_length (v);
                total = length * 2;
            }
            else if (v->type == PURC_VARIANT_TYPE_ATOMSTRING) {
                length = strlen (purc_variant_get_atom_string_const (v));
                total = length;
            }
            else {
                length = strlen (
                        purc_variant_get_exception_string_const (v));
                total = length;
            }

            if (total + 1 > size) {
                buffer = malloc (total + 1);
                if (buffer == NULL)
                    stackbuffer[0] = '\0';
                else
                    purc_variant_stringify_buff (buffer, total+1, v);
            }
            else
                purc_variant_stringify_buff (stackbuffer, size, v);
            break;

        default:
            purc_variant_stringify_buff (stackbuffer, size, v);
            break;
    }
    return buffer;
}

static int compare_string_method (purc_variant_t v1, purc_variant_t v2,
        pcvrnt_compare_method_k opt)
{
    int compare = 0.0L;
    char *buf1 = NULL;
    char *buf2 = NULL;
    char stackbuf1[128];
    char stackbuf2[sizeof(stackbuf1)];

    buf1 = compare_stringify (v1, stackbuf1, sizeof(stackbuf1));
    if (buf1 == NULL)
        buf1 = stackbuf1;

    buf2 = compare_stringify (v2, stackbuf2, sizeof(stackbuf2));
    if (buf2 == NULL)
        buf2 = stackbuf2;

    if (opt == PCVRNT_COMPARE_METHOD_CASE || opt == PCVRNT_COMPARE_METHOD_AUTO)
        compare = (double)strcmp (buf1, buf2);
    else
        compare = (double)pcutils_strcasecmp (buf1, buf2);

    if (buf1 != stackbuf1)
        free (buf1);
    if (buf2 != stackbuf2)
        free (buf2);

    return compare;
}

int purc_variant_compare_ex (purc_variant_t v1,
        purc_variant_t v2, pcvrnt_compare_method_k opt)
{
    int compare = 0;

    PC_ASSERT(v1);
    PC_ASSERT(v2);

    if ((opt == PCVRNT_COMPARE_METHOD_CASELESS) ||
            (opt == PCVRNT_COMPARE_METHOD_CASE))
        compare = compare_string_method (v1, v2, opt);
    else if (opt == PCVRNT_COMPARE_METHOD_NUMBER)
        compare = compare_number_method (v1, v2);
    else if (opt == PCVRNT_COMPARE_METHOD_AUTO) {
        if (v1 && ((v1->type == PURC_VARIANT_TYPE_NUMBER) ||
                (v1->type == PURC_VARIANT_TYPE_LONGINT) ||
                (v1->type == PURC_VARIANT_TYPE_ULONGINT) ||
                (v1->type == PURC_VARIANT_TYPE_LONGDOUBLE)))
            compare = compare_number_method (v1, v2);
        else
            compare = compare_string_method (v1, v2, opt);
    }
    else {
        PC_ASSERT(0);
    }

    return compare;
}

purc_variant_t purc_variant_load_from_json_stream(purc_rwstream_t stream)
{
    if (stream  == NULL) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t value = PURC_VARIANT_INVALID;
    struct pcvcm_node* root = NULL;
    struct pcejson* parser = NULL;

    int ret = pcejson_parse (&root, &parser, stream, PCEJSON_DEFAULT_DEPTH);
    if (ret != PCEJSON_SUCCESS) {
        goto ret;
    }

    value = pcvcm_eval (root, NULL, false);

ret:
    pcvcm_node_destroy (root);
    pcejson_destroy(parser);
    return value;
}

purc_variant_t purc_variant_make_from_json_string(const char* json, size_t sz)
{
    purc_variant_t value;
    purc_rwstream_t rwstream = purc_rwstream_new_from_mem((void*)json, sz);
    if (rwstream == NULL)
        return PURC_VARIANT_INVALID;

    value = purc_variant_load_from_json_stream(rwstream);
    purc_rwstream_destroy(rwstream);

    return value;
}

purc_variant_t purc_variant_load_from_json_file(const char* file)
{
    purc_variant_t value;
    purc_rwstream_t rwstream = purc_rwstream_new_from_file(file, "r");
    if (rwstream == NULL)
        return PURC_VARIANT_INVALID;

    value = purc_variant_load_from_json_stream(rwstream);
    purc_rwstream_destroy(rwstream);

    return value;
}

purc_variant_t purc_variant_load_dvobj_from_so (const char *so_name,
        const char *var_name)
{
    PC_ASSERT(so_name || var_name);

    purc_variant_t value = PURC_VARIANT_INVALID;

#if OS(LINUX) || OS(UNIX) || OS(DARWIN)
    const char *ext = ".so";
#   if OS(DARWIN)
    ext = ".dylib";
#   endif
    purc_variant_t val = PURC_VARIANT_INVALID;
    int ver_code;

    void *library_handle = NULL;

    char so[PATH_MAX+1];
    int n;
    do {
        if (so_name && strchr(so_name, '/')) {
            // let dlopen to handle path search
            n = snprintf(so, sizeof(so), "%s", so_name);
            PC_ASSERT(n>0 && (size_t)n<sizeof(so));
            library_handle = dlopen(so, RTLD_LAZY | RTLD_GLOBAL);
            break;
        }
        if (so_name && strchr(so_name, '.')) {
            // user specified dynamic library filename
            n = snprintf(so, sizeof(so), "%s", so_name);
            PC_ASSERT(n>0 && (size_t)n<sizeof(so));
        }
        else {
            // we build dynamic library filename
            // TODO: check validity of name!!!!
            n = snprintf(so, sizeof(so), "libpurc-dvobj-%s%s",
                    so_name ? so_name : var_name, ext);
            PC_ASSERT(n>0 && (size_t)n<sizeof(so));
        }

        /* XXX: the order of searching directories:
         *
         * 1. the valid directories contains in the environment variable:
         *      PURC_DVOBJS_PATH
         * 2. /usr/local/lib/purc-<purc-api-version>/
         * 3. /usr/lib/purc-<purc-api-version>/
         * 4. /lib/purc-<purc-api-version>/
         */

        // step1: search in directories defined by the env var
        const char *env = getenv(PURC_ENVV_DVOBJS_PATH);
        if (env) {
            char *path = strdup(env);
            char *str1;
            char *saveptr1;
            char *dir;

            for (str1 = path; ; str1 = NULL) {
                dir = strtok_r(str1, ":;", &saveptr1);
                if (dir == NULL || dir[0] != '/') {
                    break;
                }

                n = snprintf(so, sizeof(so),
                        "%s/libpurc-dvobj-%s%s",
                        dir, so_name ? so_name : var_name, ext);
                PC_ASSERT(n>0 && (size_t)n<sizeof(so));
                library_handle = dlopen(so, RTLD_LAZY | RTLD_GLOBAL);

                if (library_handle) {
                    break;
                }
            }

            free(path);

            if (library_handle)
                break;
        }

        static const char *ver = PURC_API_VERSION_STRING;

        // try in system directories.
        static const char *other_tries[] = {
            "/usr/local/lib/purc-%s/libpurc-dvobj-%s%s",
            "/usr/lib/purc-%s/libpurc-dvobj-%s%s",
            "/lib/purc-%s/libpurc-dvobj-%s%s",
        };

        for (size_t i = 0; i < PCA_TABLESIZE(other_tries); i++) {
            n = snprintf(so, sizeof(so), other_tries[i],
                    ver, so_name ? so_name : var_name, ext);
            PC_ASSERT(n>0 && (size_t)n<sizeof(so));
            library_handle = dlopen(so, RTLD_LAZY | RTLD_GLOBAL);
            if (library_handle) {
                break;
            }
        }

    } while (0);

    if (!library_handle) {
        purc_set_error_with_info(PURC_ERROR_BAD_SYSTEM_CALL,
                "failed to load: %s", so);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t (* purcex_load_dynamic_variant)(const char *, int *);

    purcex_load_dynamic_variant = (purc_variant_t (*) (const char *, int *))
        dlsym(library_handle, EXOBJ_LOAD_ENTRY);
    if(dlerror() != NULL) {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        dlclose(library_handle);
        return PURC_VARIANT_INVALID;
    }

    value = purcex_load_dynamic_variant (var_name, &ver_code);
    if(value == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        dlclose(library_handle);
        return PURC_VARIANT_INVALID;
    }

    if (purc_variant_is_type (value, PURC_VARIANT_TYPE_OBJECT)) {
        val = purc_variant_make_ulongint ((uint64_t)library_handle);
        purc_variant_object_set_by_static_ckey (value,
                EXOBJ_LOAD_HANDLE_KEY, val);
        purc_variant_unref (val);
    } else {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        purc_variant_unref (value);
        dlclose(library_handle);
        value = PURC_VARIANT_INVALID;
    }

#else
    UNUSED_PARAM(so_name);
    UNUSED_PARAM(var_name);

    // TODO: Add codes for other OS.
    pcinst_set_error (PURC_ERROR_NOT_SUPPORTED);
    PC_ASSERT(0); // Not implemented yet
#endif

    return value;
}

bool purc_variant_unload_dvobj (purc_variant_t dvobj)
{
    bool ret = true;

    if (dvobj == PURC_VARIANT_INVALID)  {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return false;
    }

    if (!purc_variant_is_type (dvobj, PURC_VARIANT_TYPE_OBJECT)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return false;
    }

    uint64_t u64 = 0;
    purc_variant_t val = purc_variant_object_get_by_ckey (dvobj,
            EXOBJ_LOAD_HANDLE_KEY);
    if (val == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return false;
    }

    if (!purc_variant_cast_to_ulongint (val, &u64, false)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return false;
    }

#if OS(LINUX) || OS(UNIX)
    if (u64) {
        purc_variant_unref (dvobj);
        if (dlclose((void *)u64) != 0) {
            pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
            ret = false;
        }
    } else {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        ret = false;
    }
#else
    // TODO: Add codes for other OS.
    pcinst_set_error (PURC_ERROR_NOT_SUPPORTED);
#endif

    return ret;
}

static double
numerify_str(const char *s)
{
    if (!s || !*s)
        return 0.0;

    return strtod(s, NULL);
}

static double
numerify_bs(const unsigned char *s, size_t nr)
{
    if (!s || nr == 0)
        return 0.0;

    long int number = 0;
    size_t size = sizeof(long int);

    if (nr > size) {
        s += (nr - size);
        nr = size;
    }

    /* FIXME: Use purc_fetch_xxx */
#if CPU(BIG_ENDIAN)
    char buffer[size] = {0,};
    for (int i = 0; i < nr; i++)
        buffer[size - 1 - i] = *(s + i);
    memcpy(&number, buffer, nr);
#elif CPU(LITTLE_ENDIAN)
    memcpy(&number, s, nr);
#endif

    return (double)number;
}

static double
numerify_dynamic(purc_variant_t value)
{
    purc_dvariant_method getter;
    getter = purc_variant_dynamic_get_getter(value);

    if (!getter)
        return 0.0;

    purc_variant_t v = getter(value, 0, NULL, true); // TODO: silently
    if (v == PURC_VARIANT_INVALID)
        return 0.0;

    double d = purc_variant_numerify(v);
    purc_variant_unref(v);

    return d;
}

static double
numerify_native(purc_variant_t value)
{
    void *native = value->ptr_ptr[0];

    struct purc_native_ops *ops;
    ops = (struct purc_native_ops*)value->ptr_ptr[1];

    if (!ops || !ops->property_getter)
        return 0.0;

    purc_nvariant_method getter = (ops->property_getter)(native, "__number");
    if (!getter)
        return 0.0;

    purc_variant_t v = getter(native, "__number", 0, NULL,
            PCVRT_CALL_FLAG_SILENTLY);
    if (v == PURC_VARIANT_INVALID)
        return 0.0;

    double d = purc_variant_numerify(v);
    purc_variant_unref(v);

    return d;
}

static double
numerify_array(purc_variant_t value)
{
    size_t sz;
    if (!purc_variant_array_size(value, &sz))
        return 0.0;

    double d = 0.0;

    for (size_t i=0; i<sz; ++i) {
        purc_variant_t v = purc_variant_array_get(value, i);
        d += purc_variant_numerify(v);
    }

    return d;
}

static double
numerify_object(purc_variant_t value)
{
    double d = 0.0;

    purc_variant_t v;
    foreach_value_in_variant_object(value, v)
        d += purc_variant_numerify(v);
    end_foreach;

    return d;
}

static double
numerify_set(purc_variant_t value)
{
    double d = 0.0;

    purc_variant_t v;
    foreach_value_in_variant_set_order(value, v)
        d += purc_variant_numerify(v);
    end_foreach;

    return d;
}

static double
numerify_tuple(purc_variant_t value)
{
    purc_variant_t *members;
    size_t sz;

    members = tuple_members(value, &sz);
    assert(members);

    double d = 0;
    for (size_t n = 0; n < sz; n++) {
        d += purc_variant_numerify(members[n]);
    }

    return d;
}

double
purc_variant_numerify(purc_variant_t value)
{
    PC_ASSERT(value != PURC_VARIANT_INVALID);

    const char *s;
    const unsigned char *bs;
    size_t nr;
    enum purc_variant_type type = purc_variant_get_type(value);

    switch (type)
    {
        case PURC_VARIANT_TYPE_UNDEFINED:
            return 0.0;
        case PURC_VARIANT_TYPE_NULL:
            return 0.0;
        case PURC_VARIANT_TYPE_BOOLEAN:
            return value->b ? 1.0 : 0.0;
        case PURC_VARIANT_TYPE_EXCEPTION:
            return value->atom;
        case PURC_VARIANT_TYPE_NUMBER:
            return value->d;
        case PURC_VARIANT_TYPE_LONGINT:
            return value->i64;
        case PURC_VARIANT_TYPE_ULONGINT:
            return value->u64;
        case PURC_VARIANT_TYPE_LONGDOUBLE:
            return value->ld;
        case PURC_VARIANT_TYPE_ATOMSTRING:
            s = purc_variant_get_atom_string_const(value);
            return numerify_str(s);
        case PURC_VARIANT_TYPE_STRING:
            s = purc_variant_get_string_const(value);
            return numerify_str(s);
        case PURC_VARIANT_TYPE_BSEQUENCE:
            bs = purc_variant_get_bytes_const(value, &nr);
            return numerify_bs(bs, nr);
        case PURC_VARIANT_TYPE_DYNAMIC:
            return numerify_dynamic(value);
        case PURC_VARIANT_TYPE_NATIVE:
            return numerify_native(value);
        case PURC_VARIANT_TYPE_OBJECT:
            return numerify_object(value);
        case PURC_VARIANT_TYPE_ARRAY:
            return numerify_array(value);
        case PURC_VARIANT_TYPE_SET:
            return numerify_set(value);
        case PURC_VARIANT_TYPE_TUPLE:
            return numerify_tuple(value);
        default:
            PC_ASSERT(0);
            break;
    }

    return 0;
}

#if 0
static bool
booleanize_str(const char *s)
{
    if (!s || !*s)
        return false;

    return numerify_str(s) != 0.0 ? true : false;
}

static bool
booleanize_bs(const unsigned char *s, size_t nr)
{
    if (!s || nr == 0)
        return false;

    return numerify_bs(s, nr) != 0.0 ? true : false;
}
#endif

static bool
booleanize_dynamic(purc_variant_t value)
{
    purc_dvariant_method getter;
    getter = purc_variant_dynamic_get_getter(value);

    if (!getter)
        return false;

    purc_variant_t v = getter(value, 0, NULL, true); // TODO: silently
    if (v == PURC_VARIANT_INVALID)
        return false;

    bool b = purc_variant_booleanize(v);
    purc_variant_unref(v);

    return b;
}

static bool
booleanize_native(purc_variant_t value)
{
    void *native = value->ptr_ptr[0];

    struct purc_native_ops *ops;
    ops = (struct purc_native_ops*)value->ptr_ptr[1];

    if (!ops || !ops->property_getter)
        return false;

    purc_nvariant_method getter = (ops->property_getter)(native, "__boolean");
    if (!getter)
        return false;

    purc_variant_t v = getter(native, "__boolean", 0, NULL,
            PCVRT_CALL_FLAG_SILENTLY);
    if (v == PURC_VARIANT_INVALID)
        return false;

    bool b = purc_variant_booleanize(v);
    purc_variant_unref(v);

    return b;
}

bool
purc_variant_booleanize(purc_variant_t value)
{
    PC_ASSERT(value != PURC_VARIANT_INVALID);

    size_t nr;
    switch (value->type) {
    case PURC_VARIANT_TYPE_UNDEFINED:
        return false;

    case PURC_VARIANT_TYPE_NULL:
        return false;

    case PURC_VARIANT_TYPE_BOOLEAN:
        return value->b;

    case PURC_VARIANT_TYPE_NUMBER:
        return value->d != 0;

    case PURC_VARIANT_TYPE_LONGINT:
        return value->i64 != 0;

    case PURC_VARIANT_TYPE_ULONGINT:
        return value->u64 != 0;

    case PURC_VARIANT_TYPE_LONGDOUBLE:
        return value->ld != 0;

    case PURC_VARIANT_TYPE_EXCEPTION:
    case PURC_VARIANT_TYPE_ATOMSTRING:
    case PURC_VARIANT_TYPE_STRING:
        purc_variant_get_string_const_ex(value, &nr);
        return (nr != 0);

    case PURC_VARIANT_TYPE_BSEQUENCE:
        purc_variant_get_bytes_const(value, &nr);
        return (nr != 0);

    case PURC_VARIANT_TYPE_OBJECT:
        return purc_variant_object_get_size(value) != 0;
        // return numerify_object(value) != 0.0 ? true : false;

    case PURC_VARIANT_TYPE_ARRAY:
        return purc_variant_array_get_size(value) != 0;
        // return numerify_array(value) != 0.0 ? true : false;

    case PURC_VARIANT_TYPE_SET:
        return purc_variant_set_get_size(value) != 0;
        // return numerify_set(value) != 0.0 ? true : false;

    case PURC_VARIANT_TYPE_TUPLE:
        return purc_variant_tuple_get_size(value) != 0;

    case PURC_VARIANT_TYPE_DYNAMIC:
        return booleanize_dynamic(value);

    case PURC_VARIANT_TYPE_NATIVE:
        return booleanize_native(value);

    default:
        PC_ASSERT(0);
        break;
    }

    return false;
}

struct stringify_arg
{
    void (*cb)(struct stringify_arg *arg, const void *src, size_t len);
    void *arg;
    unsigned int flags;
};

static void
stringify_bs(struct stringify_arg *arg, const unsigned char *bs, size_t nr)
{
    static const char chars[] = "0123456789ABCDEF";

    char buffer[512+1]; // must be an odd number!!!

    char *p = buffer;
    char *end = p + sizeof(buffer) - 1;

    for (size_t i=0; i<nr; ++i) {
        int h = bs[i] >> 4;
        int l = bs[i] & 0x0F;

        *p++ = chars[h];
        *p++ = chars[l];
        if (p == end) {
            *p = '\0';
            arg->cb(arg, buffer, 0);
            p = buffer;
        }
    }

    if (p>buffer) {
        *p = '\0';
        arg->cb(arg, buffer, 0);
    }
}

static void
variant_stringify(struct stringify_arg *arg, purc_variant_t value);

static void
stringify_array(struct stringify_arg *arg, purc_variant_t value)
{
    size_t sz = 0;
    purc_variant_array_size(value, &sz);

    for (size_t i=0; i<sz; ++i) {
        purc_variant_t v = purc_variant_array_get(value, i);
        variant_stringify(arg, v);
        arg->cb(arg, "\n", 0);
    }
}

static void
stringify_kv(struct stringify_arg *arg, const char *key, purc_variant_t val)
{
    arg->cb(arg, key, strlen(key));
    arg->cb(arg, ":", 0);
    variant_stringify(arg, val);
    arg->cb(arg, "\n", 0);
}

static void
stringify_object(struct stringify_arg *arg, purc_variant_t value)
{
    purc_variant_t k, v;
    foreach_key_value_in_variant_object(value, k, v)
        const char *sk = purc_variant_get_string_const(k);
        stringify_kv(arg, sk, v);
    end_foreach;
}

static void
stringify_set(struct stringify_arg *arg, purc_variant_t value)
{
    purc_variant_t v;
    foreach_value_in_variant_set_order(value, v)
        variant_stringify(arg, v);
        arg->cb(arg, "\n", 0);
    end_foreach;
}

static void
stringify_tuple(struct stringify_arg *arg, purc_variant_t value)
{
    purc_variant_t *members;
    size_t sz;

    members = tuple_members(value, &sz);
    assert(members);

    for (size_t n = 0; n < sz; n++) {
        variant_stringify(arg, members[n]);
        arg->cb(arg, "\n", 0);
    }
}

static void
stringify_dynamic(struct stringify_arg *arg, purc_variant_t value)
{
    purc_dvariant_method getter = purc_variant_dynamic_get_getter(value);
    purc_dvariant_method setter = purc_variant_dynamic_get_setter(value);

    char buf[128];
    snprintf(buf, sizeof(buf), "<dynamic: %p, %p>", getter, setter);
    arg->cb(arg, buf, 0);
}

static void
stringify_native(struct stringify_arg *arg, purc_variant_t value)
{
    void *native = purc_variant_native_get_entity(value);

    char buf[128];
    snprintf(buf, sizeof(buf), "<native: %p>", native);
    arg->cb(arg, buf, 0);
}

static void
variant_stringify(struct stringify_arg *arg, purc_variant_t value)
{
    enum purc_variant_type type = purc_variant_get_type(value);
    char buf[128];

    switch (type) {
    case PURC_VARIANT_TYPE_UNDEFINED:
        arg->cb(arg, "undefined", 0);
        break;
    case PURC_VARIANT_TYPE_NULL:
        arg->cb(arg, "null", 0);
        break;
    case PURC_VARIANT_TYPE_BOOLEAN:
        if (value->b) {
            arg->cb(arg, "true", 0);
        } else {
            arg->cb(arg, "false", 0);
        }
        break;
    case PURC_VARIANT_TYPE_NUMBER:
        if (arg->flags & PCVRNT_STRINGIFY_OPT_REAL_BAREBYTES) {
            arg->cb(arg, &value->d, sizeof(double));
        }
        else {
            snprintf(buf, sizeof(buf), "%g", value->d);
            arg->cb(arg, buf, 0);
        }
        break;
    case PURC_VARIANT_TYPE_LONGINT:
        if (arg->flags & PCVRNT_STRINGIFY_OPT_REAL_BAREBYTES) {
            arg->cb(arg, &value->i64, sizeof(int64_t));
        }
        else {
            snprintf(buf, sizeof(buf), "%" PRId64 "", value->i64);
            arg->cb(arg, buf, 0);
        }
        break;

    case PURC_VARIANT_TYPE_ULONGINT:
        if (arg->flags & PCVRNT_STRINGIFY_OPT_REAL_BAREBYTES) {
            arg->cb(arg, &value->u64, sizeof(uint64_t));
        }
        else {
            snprintf(buf, sizeof(buf), "%" PRIu64 "", value->u64);
            arg->cb(arg, buf, 0);
        }
        break;

    case PURC_VARIANT_TYPE_LONGDOUBLE:
        if (arg->flags & PCVRNT_STRINGIFY_OPT_REAL_BAREBYTES) {
            arg->cb(arg, &value->ld, sizeof(long double));
        }
        else {
            snprintf(buf, sizeof(buf), "%Lg", value->ld);
            arg->cb(arg, buf, 0);
        }
        break;

    case PURC_VARIANT_TYPE_EXCEPTION:
    case PURC_VARIANT_TYPE_ATOMSTRING:
    case PURC_VARIANT_TYPE_STRING:
    {
        const char *str;
        size_t len;

        str = purc_variant_get_string_const_ex(value, &len);
        arg->cb(arg, str, len);
        break;
    }

    case PURC_VARIANT_TYPE_BSEQUENCE:
    {
        const unsigned char *bs;
        size_t nr;

        bs = purc_variant_get_bytes_const(value, &nr);
        if (arg->flags & PCVRNT_STRINGIFY_OPT_BSEQUENCE_BAREBYTES) {
            arg->cb(arg, bs, nr);
        }
        else {
            stringify_bs(arg, bs, nr);
        }
        break;
    }

    case PURC_VARIANT_TYPE_DYNAMIC:
        stringify_dynamic(arg, value);
        break;
    case PURC_VARIANT_TYPE_NATIVE:
        stringify_native(arg, value);
        break;
    case PURC_VARIANT_TYPE_OBJECT:
        stringify_object(arg, value);
        break;
    case PURC_VARIANT_TYPE_ARRAY:
        stringify_array(arg, value);
        break;
    case PURC_VARIANT_TYPE_SET:
        stringify_set(arg, value);
        break;
    case PURC_VARIANT_TYPE_TUPLE:
        stringify_tuple(arg, value);
        break;
    default:
        PC_ASSERT(0);
        break;
    }
}

ssize_t
purc_variant_stringify_buff(char *buf, size_t len, purc_variant_t value)
{
    PC_ASSERT(buf);
    PC_ASSERT(len > 0);

    purc_rwstream_t stream;
    stream = purc_rwstream_new_from_mem((void*)buf, len);
    if (!stream)
        return -1;

    unsigned int flags = 0;
    ssize_t sz = purc_variant_stringify(stream, value, flags, NULL);
    purc_rwstream_destroy(stream);

    if (sz == -1)
        return -1;

    PC_ASSERT((size_t)sz < len);
    if ((size_t)sz < len) {
        buf[sz] = '\0';
    }
    else {
        buf[len-1] = '\0';
    }

    return sz;
}

struct stringify_stream {
    purc_rwstream_t           stream;
    unsigned int              flags;
    size_t                    written;
    size_t                    accu;
    int                       err;
};

static void
do_stringify_stream(struct stringify_arg *arg, const void *src, size_t len)
{
    struct stringify_stream *ud;
    ud = (struct stringify_stream*)(arg->arg);

    if (len == 0)
        len = strlen(src);

    if (ud->err == 0 && ud->stream && len > 0) {
        ssize_t sz = purc_rwstream_write(ud->stream, src, len);
        if (sz == -1) {
            ud->err = -1;
        }
        else {
            PC_ASSERT(sz >= 0);
            PC_ASSERT((size_t)sz == len); // FIXME: < len ???
            ud->written += sz;
        }
    }

    ud->accu += len;
}

ssize_t
purc_variant_stringify(purc_rwstream_t stream, purc_variant_t value,
        unsigned int flags, size_t *len_expected)
{
    if (value == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    struct stringify_stream ud = {
        .stream           = stream,
        .flags            = flags,
        .written          = 0,
        .accu             = 0,
        .err              = 0,
    };

    struct stringify_arg arg;
    arg.cb    = do_stringify_stream;
    arg.arg   = &ud;
    arg.flags = flags;

    variant_stringify(&arg, value);

    PC_ASSERT(ud.err == 0);

    if (len_expected)
        *len_expected = ud.accu;

    if (ud.err) {
        if (flags & PCVRNT_STRINGIFY_OPT_IGNORE_ERRORS) {
            purc_clr_error();
            return ud.written;
        }
        return -1;
    }

    return ud.written;
}

ssize_t
purc_variant_stringify_alloc(char **strp, purc_variant_t value)
{
    purc_rwstream_t stream;
    stream = purc_rwstream_new_buffer(0, 0);
    if (!stream)
        return -1;

    unsigned int flags = 0;
    ssize_t sz = purc_variant_stringify(stream, value, flags, NULL);
    if (sz == -1) {
        purc_rwstream_destroy(stream);
        return -1;
    }

    if (!strp) {
        purc_rwstream_destroy(stream);
        return -1;
    }

    size_t sz_content, sz_buff;
    bool res_buff = true;
    char *p;
    p = (char*)purc_rwstream_get_mem_buffer_ex(stream,
            &sz_content, &sz_buff, res_buff);
    purc_rwstream_destroy(stream);

    PC_ASSERT(p);
    PC_ASSERT(sz_buff >= 1);
    PC_ASSERT(sz_content <= sz_buff);
    p[sz_content] = '\0';

    *strp = p;

    return sz_content;
}

ssize_t pcvariant_serialize(char *buf, size_t sz, purc_variant_t val)
{
    PC_ASSERT(val != PURC_VARIANT_INVALID);
    purc_rwstream_t out = purc_rwstream_new_from_mem(buf, sz);
    PC_ASSERT(out); // FIXME:

    size_t len_expected = 0;
    ssize_t n;
    n = purc_variant_serialize(val, out,
            0, PCVRNT_SERIALIZE_OPT_PLAIN,
            &len_expected);
    PC_ASSERT(n > 0); // FIXME:

    // FIXME: add-null-terminator?
    n = purc_rwstream_write(out, "", 1);
    PC_ASSERT(n > 0); // FIXME:

    purc_rwstream_destroy(out);

    return len_expected + 1; // FIXME: len_expected counts null-terminator?
}

char* pcvariant_serialize_alloc(char *buf, size_t sz, purc_variant_t val)
{
    int r = pcvariant_serialize(buf, sz, val);
    PC_ASSERT(r > 0); // FIXME:

    if ((size_t)r < sz)
        return buf;

    char *p = (char*)malloc(r + 1);
    PC_ASSERT(p); // FIXME:
    r = pcvariant_serialize(p, r+1, val);
    PC_ASSERT(r > 0);

    return p;
}

char* pcvariant_to_string(purc_variant_t v)
{
    purc_rwstream_t rws = purc_rwstream_new_buffer(PRINT_MIN_BUFFER,
            PRINT_MAX_BUFFER);
    size_t len = 0;
    purc_variant_serialize(v, rws, 0,
            PCVRNT_SERIALIZE_OPT_PLAIN | PCVRNT_SERIALIZE_OPT_UNIQKEYS,
            &len);
    purc_rwstream_write(rws, "", 1);
    char* buf = (char*)purc_rwstream_get_mem_buffer_ex(rws,
            NULL, NULL, true);
    purc_rwstream_destroy(rws);
    return buf;
}

static int
pcvariant_object_set_va(purc_variant_t obj, size_t nr_kvs, va_list ap)
{
    while (nr_kvs--) {
        const char *key = va_arg(ap, const char*);
        const char *val = va_arg(ap, const char*);
        PC_ASSERT(key);
        PC_ASSERT(val);
        purc_variant_t k = purc_variant_make_string(key, true);
        if (k == PURC_VARIANT_INVALID)
            return -1;
        purc_variant_t v = purc_variant_make_string(val, true);
        if (v == PURC_VARIANT_INVALID) {
            purc_variant_unref(k);
            return -1;
        }
        bool ok = purc_variant_object_set(obj, k, v);
        purc_variant_unref(k);
        purc_variant_unref(v);
        if (!ok)
            return -1;
    }

    return 0;
}

purc_variant_t pcvariant_make_object(size_t nr_kvs, ...)
{
    purc_variant_t obj = purc_variant_make_object_by_static_ckey(0,
                NULL, PURC_VARIANT_INVALID);
    if (obj == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    if (nr_kvs == 0) {
        return obj;
    }

    va_list ap;
    va_start(ap, nr_kvs);
    int r = pcvariant_object_set_va(obj, nr_kvs, ap);
    va_end(ap);
    if (r) {
        purc_variant_unref(obj);
        return PURC_VARIANT_INVALID;
    }

    return obj;
}

purc_variant_t pcvariant_make_with_printf(const char *fmt, ...)
{
    char buf[1024];

    va_list ap, ap1;
    va_start(ap, fmt);
    va_copy(ap1, ap);
    int r = vsnprintf(buf, sizeof(buf), fmt, ap);
    PC_ASSERT(r >= 0);
    va_end(ap);

    if ((size_t)r >= sizeof(buf)) {
        char *p = (char*)malloc(r);
        if (!p) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return PURC_VARIANT_INVALID;
        }
        r = vsnprintf(p, r, fmt, ap1);
        PC_ASSERT(r >= 0);
        purc_variant_t v = purc_variant_make_string(p, true);
        free(p);
        return v;
    }

    return purc_variant_make_string(buf, true);
}

purc_variant_t
pcvariant_object_shallow_copy(purc_variant_t obj)
{
    PC_ASSERT(obj);
    PC_ASSERT(purc_variant_is_object(obj));

    purc_variant_t o;
    o = purc_variant_make_object(0, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);

    if (o == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    purc_variant_t k, v;
    foreach_key_value_in_variant_object(obj, k, v)
        bool ok = purc_variant_object_set(o, k, v);
        if (!ok) {
            purc_variant_unref(o);
            return PURC_VARIANT_INVALID;
        }
    end_foreach;

    return o;
}

bool
purc_variant_is_container(purc_variant_t var)
{
    return IS_CONTAINER(var->type);
}

purc_variant_t
pcvariant_container_clone(purc_variant_t ctnr, bool recursively)
{
    enum purc_variant_type vt = ctnr->type;
    switch (vt) {
        case PURC_VARIANT_TYPE_ARRAY:
            return pcvariant_array_clone(ctnr, recursively);
        case PURC_VARIANT_TYPE_OBJECT:
            return pcvariant_object_clone(ctnr, recursively);
        case PURC_VARIANT_TYPE_SET:
            return pcvariant_set_clone(ctnr, recursively);
        case PURC_VARIANT_TYPE_TUPLE:
            return pcvariant_tuple_clone(ctnr, recursively);
        default:
            return purc_variant_ref(ctnr);
    }
}

purc_variant_t
purc_variant_container_clone(purc_variant_t ctnr)
{
    if (ctnr == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    return pcvariant_container_clone(ctnr, false);
}

purc_variant_t
purc_variant_container_clone_recursively(purc_variant_t ctnr)
{
    if (ctnr == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    return pcvariant_container_clone(ctnr, true);
}

int
pcvariant_diff(purc_variant_t l, purc_variant_t r)
{
    if (0) {
        bool caseless = false;
        bool unify_number = true;
        return pcvar_compare_ex(l, r, caseless, unify_number);
    }
    else {
        enum pcvrnt_compare_method opt;
        opt = PCVRNT_COMPARE_METHOD_CASE;

        return pcvariant_diff_ex(l, r, opt);
    }
}

struct purc_ejson_parsing_tree *
purc_variant_ejson_parse_string(const char *ejson, size_t sz)
{
    struct purc_ejson_parsing_tree *ptree;
    purc_rwstream_t rwstream = purc_rwstream_new_from_mem((void*)ejson, sz);
    if (rwstream == NULL)
        return NULL;

    ptree = purc_variant_ejson_parse_stream(rwstream);
    purc_rwstream_destroy(rwstream);

    return ptree;
}

struct purc_ejson_parsing_tree *
purc_variant_ejson_parse_file(const char *fname)
{
    struct purc_ejson_parsing_tree *ptree;
    purc_rwstream_t rwstream = purc_rwstream_new_from_file(fname, "r");
    if (rwstream == NULL)
        return NULL;

    ptree = purc_variant_ejson_parse_stream(rwstream);
    purc_rwstream_destroy(rwstream);

    return ptree;
}

struct purc_ejson_parsing_tree *
purc_variant_ejson_parse_stream(purc_rwstream_t rws)
{
    struct pcvcm_node* root = NULL;
    struct pcejson* parser = NULL;

    int ret = pcejson_parse(&root, &parser, rws, PCEJSON_DEFAULT_DEPTH);
    if (ret == PCEJSON_SUCCESS) {
        pcejson_destroy(parser);
        return (struct purc_ejson_parsing_tree *)root;
    }

    pcvcm_node_destroy(root);
    pcejson_destroy(parser);
    return NULL;
}

purc_variant_t
purc_ejson_parsing_tree_evalute(struct purc_ejson_parsing_tree *parse_tree,
        purc_cb_get_var fn_get_var, void *ctxt, bool silently)
{
    return pcvcm_eval_ex((struct pcvcm_node*)parse_tree, NULL, fn_get_var,
           ctxt, silently);
}

void
purc_ejson_parsing_tree_destroy(struct purc_ejson_parsing_tree *parse_tree)
{
    pcvcm_node_destroy((struct pcvcm_node *)parse_tree);
}

static int
cmp_by_obj(purc_variant_t l, purc_variant_t r,
        bool caseless, bool unify_number)
{
    int diff;

    variant_obj_t ld, rd;
    ld = (variant_obj_t)l->sz_ptr[1];
    rd = (variant_obj_t)r->sz_ptr[1];
    PC_ASSERT(ld);
    PC_ASSERT(rd);
    struct rb_root *lroot = &ld->kvs;
    struct rb_root *rroot = &rd->kvs;
    struct rb_node *lnode = pcutils_rbtree_first(lroot);
    struct rb_node *rnode = pcutils_rbtree_first(rroot);
    for (;
        lnode && rnode;
        lnode = pcutils_rbtree_next(lnode), rnode = pcutils_rbtree_next(rnode))
    {
        struct obj_node *lo, *ro;
        lo = container_of(lnode, struct obj_node, node);
        ro = container_of(rnode, struct obj_node, node);
        PC_ASSERT(lo->key);
        PC_ASSERT(ro->key);
        const char *lk = purc_variant_get_string_const(lo->key);
        const char *rk = purc_variant_get_string_const(ro->key);
        PC_ASSERT(lk);
        PC_ASSERT(rk);

        // NOTE: ignore caseless for keyname
        diff = strcmp(lk, rk);
        if (diff)
            return diff;

        purc_variant_t lv = lo->val;
        purc_variant_t rv = ro->val;
        PC_ASSERT(lv != PURC_VARIANT_INVALID);
        PC_ASSERT(rv != PURC_VARIANT_INVALID);

        diff = pcvar_compare_ex(lv, rv, caseless, unify_number);
        if (diff)
            return diff;
    }

    if (lnode)
        return 1;
    else if (rnode)
        return -1;
    else
        return 0;
}

static int
cmp_by_arr(purc_variant_t l, purc_variant_t r,
        bool caseless, bool unify_number)
{
    int diff;

    variant_arr_t ld, rd;
    ld = (variant_arr_t)l->sz_ptr[1];
    rd = (variant_arr_t)r->sz_ptr[1];
    PC_ASSERT(ld);
    PC_ASSERT(rd);

    struct pcutils_array_list *la = &ld->al;
    struct pcutils_array_list *ra = &rd->al;

    struct pcutils_array_list_node *lnode = pcutils_array_list_get_first(la);
    struct pcutils_array_list_node *rnode = pcutils_array_list_get_first(ra);

    for (;
        lnode && rnode;
        lnode = pcutils_array_list_get(la, lnode->idx + 1),
        rnode = pcutils_array_list_get(ra, rnode->idx + 1))
    {
        struct arr_node *lo, *ro;
        lo = container_of(lnode, struct arr_node, node);
        ro = container_of(rnode, struct arr_node, node);

        purc_variant_t lv = lo->val;
        purc_variant_t rv = ro->val;
        PC_ASSERT(lv != PURC_VARIANT_INVALID);
        PC_ASSERT(rv != PURC_VARIANT_INVALID);

        diff = pcvar_compare_ex(lv, rv, caseless, unify_number);
        if (diff)
            return diff;
    }

    if (lnode)
        return 1;
    else if (rnode)
        return -1;
    else
        return 0;
}

static int
cmp_by_set(purc_variant_t l, purc_variant_t r,
        bool caseless, bool unify_number)
{
    int diff;

    variant_set_t ld, rd;
    ld = (variant_set_t)l->sz_ptr[1];
    rd = (variant_set_t)r->sz_ptr[1];
    PC_ASSERT(ld);
    PC_ASSERT(rd);

    struct rb_root *lroot = &ld->elems;
    struct rb_root *rroot = &rd->elems;
    struct rb_node *lnode = pcutils_rbtree_first(lroot);
    struct rb_node *rnode = pcutils_rbtree_first(rroot);
    for (;
        lnode && rnode;
        lnode = pcutils_rbtree_next(lnode), rnode = pcutils_rbtree_next(rnode))
    {
        struct set_node *lo, *ro;
        lo = container_of(lnode, struct set_node, rbnode);
        ro = container_of(rnode, struct set_node, rbnode);

        purc_variant_t lv = lo->val;
        purc_variant_t rv = ro->val;
        PC_ASSERT(lv != PURC_VARIANT_INVALID);
        PC_ASSERT(rv != PURC_VARIANT_INVALID);

        diff = pcvar_compare_ex(lv, rv, caseless, unify_number);
        if (diff)
            return diff;
    }

    if (lnode)
        return 1;
    else if (rnode)
        return -1;
    else
        return 0;
}

static int
cmp_by_tuple(purc_variant_t l, purc_variant_t r,
        bool caseless, bool unify_number)
{
    int diff;

    purc_variant_t *members1, *members2;
    size_t sz1, sz2;

    members1 = tuple_members(l, &sz1);
    members2 = tuple_members(r, &sz2);
    assert(members1 && members2);

    for (size_t n = 0; n < MIN(sz1, sz2); n++) {
        purc_variant_t lv = members1[n];
        purc_variant_t rv = members2[n];

        diff = pcvar_compare_ex(lv, rv, caseless, unify_number);
        if (diff)
            return diff;
    }

    if (sz1 > sz2)
        return 1;
    else if (sz1 < sz2)
        return -1;

    return 0;
}

struct comp_ex_data {
    enum pcvrnt_compare_method        opt;

    int  diff;
};

static int
number_diff(double l, double r)
{
    // FIXME: delta compare??? NaN/Inf??? fpclassify???

    if (l < r)
        return -1;

    if (l > r)
        return 1;

    return 0;
}

static int
ld_diff(long double l, long double r)
{
    // FIXME: delta compare??? NaN/Inf??? fpclassify???

    if (l < r)
        return -1;

    if (l > r)
        return 1;

    return 0;
}

static int
i64_diff(int64_t l, int64_t r)
{
    if (l == r)
        return 0;

    return (l < r) ? -1 : 1;
};

static int
u64_diff(uint64_t l, uint64_t r)
{
    if (l == r)
        return 0;

    return (l < r) ? -1 : 1;
};

static int
str_numerify_diff(const char *l, const char *r)
{
    double ld = numerify_str(l);
    double rd = numerify_str(r);

    return number_diff(ld, rd);
}

static int
str_diff(const char *l, const char *r, struct comp_ex_data *data)
{
    const char *ls, *rs;
    ls = l;
    rs = r;

    switch (data->opt) {
        case PCVRNT_COMPARE_METHOD_AUTO:
            return strcmp(ls, rs);

        case PCVRNT_COMPARE_METHOD_NUMBER:
            return str_numerify_diff(ls, rs);

        case PCVRNT_COMPARE_METHOD_CASE:
            return strcmp(ls, rs);

        case PCVRNT_COMPARE_METHOD_CASELESS:
            return pcutils_strcasecmp(ls, rs);

        default:
            PC_ASSERT(0);
            break;
    }

    return 0;
}

static int
atom_diff(purc_variant_t l, purc_variant_t r, struct comp_ex_data *data)
{
    const char *ls, *rs;

    switch (data->opt) {
        case PCVRNT_COMPARE_METHOD_AUTO:
            // FIXME: what if same string in differen BUCKET???
            return l->atom - r->atom;

        case PCVRNT_COMPARE_METHOD_NUMBER:
            ls = purc_atom_to_string(l->atom);
            rs = purc_atom_to_string(r->atom);
            return str_numerify_diff(ls, rs);

        case PCVRNT_COMPARE_METHOD_CASE:
            ls = purc_atom_to_string(l->atom);
            rs = purc_atom_to_string(r->atom);
            return strcmp(ls, rs);

        case PCVRNT_COMPARE_METHOD_CASELESS:
            ls = purc_atom_to_string(l->atom);
            rs = purc_atom_to_string(r->atom);
            return pcutils_strcasecmp(ls, rs);

        default:
            PC_ASSERT(0);
            break;
    }

    return 0;
}

static int
bs_diff(purc_variant_t l, purc_variant_t r)
{
    const unsigned char *lb = (const unsigned char*)l->sz_ptr[1];
    const unsigned char *rb = (const unsigned char*)r->sz_ptr[1];
    size_t ln = l->sz_ptr[0];
    size_t rn = r->sz_ptr[0];

    size_t n = ln < rn ? ln : rn;

    int diff = memcmp(lb, rb, n);
    if (diff)
        return diff;

    if (ln == rn)
        return 0;

    return (ln < rn) ? -1 : 1;
}

static int
dynamic_diff(purc_variant_t l, purc_variant_t r)
{
    // NOTE: compare by addresses
    return memcmp(l->ptr_ptr, r->ptr_ptr, sizeof(void *) * 2);
}

static int
native_diff(purc_variant_t l, purc_variant_t r)
{
    // NOTE: compare by addresses
    return memcmp(l->ptr_ptr, r->ptr_ptr, sizeof(void *) * 2);
}

static int
homo_scalar_diff(purc_variant_t l, purc_variant_t r, struct comp_ex_data *data)
{
    switch (l->type) {
        case PURC_VARIANT_TYPE_UNDEFINED:
            return 0;

        case PURC_VARIANT_TYPE_NULL:
            return 0;

        case PURC_VARIANT_TYPE_BOOLEAN:
            return l->b - r->b;

        case PURC_VARIANT_TYPE_EXCEPTION:
            return atom_diff(l, r, data);

        case PURC_VARIANT_TYPE_NUMBER:
            return number_diff(l->d, r->d);

        case PURC_VARIANT_TYPE_LONGINT:
            return i64_diff(l->i64, r->i64);

        case PURC_VARIANT_TYPE_ULONGINT:
            return u64_diff(l->u64, r->u64);

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            return ld_diff(l->ld, r->ld);

        case PURC_VARIANT_TYPE_ATOMSTRING:
            return atom_diff(l, r, data);

        case PURC_VARIANT_TYPE_STRING:
            return str_diff(purc_variant_get_string_const(l),
                    purc_variant_get_string_const(r), data);

        case PURC_VARIANT_TYPE_BSEQUENCE:
            return bs_diff(l, r);

        case PURC_VARIANT_TYPE_DYNAMIC:
            return dynamic_diff(l, r);

        case PURC_VARIANT_TYPE_NATIVE:
            return native_diff(l, r);

        default:
            PC_ASSERT(0);
            break;
    }

    return 0;
}

static int
numerify_diff(purc_variant_t l, purc_variant_t r)
{
    double ld = purc_variant_numerify(l);
    double rd = purc_variant_numerify(r);

    return number_diff(ld, rd);
}

static const char*
stringify(char *buf, size_t len, purc_variant_t v)
{
    ssize_t nr = 0;

    switch (v->type) {
        case PURC_VARIANT_TYPE_UNDEFINED:
            nr = snprintf(buf, len, "undefined");
            break;

        case PURC_VARIANT_TYPE_NULL:
            nr = snprintf(buf, len, "null");
            break;

        case PURC_VARIANT_TYPE_BOOLEAN:
            nr = snprintf(buf, len, "%s", v->b ? "true" : "false");
            break;

        case PURC_VARIANT_TYPE_EXCEPTION:
            return purc_atom_to_string(v->atom);
            break;

        case PURC_VARIANT_TYPE_NUMBER:
            nr = snprintf(buf, len, "%g", v->d);
            break;

        case PURC_VARIANT_TYPE_LONGINT:
            nr = snprintf(buf, len, "%" PRId64 "", v->i64);
            break;

        case PURC_VARIANT_TYPE_ULONGINT:
            nr = snprintf(buf, len, "%" PRIu64 "", v->u64);
            break;

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            nr = snprintf(buf, len, "%Lg", v->ld);
            break;

        case PURC_VARIANT_TYPE_ATOMSTRING:
            return purc_atom_to_string(v->atom);

        case PURC_VARIANT_TYPE_STRING:
            return purc_variant_get_string_const(v);

        case PURC_VARIANT_TYPE_BSEQUENCE:
            // NOTE: we need not to alloc
            PC_ASSERT(0);
            break;

        case PURC_VARIANT_TYPE_DYNAMIC:
            nr = snprintf(buf, len, "<dynamic: %p, %p>",
                    v->ptr_ptr[0], v->ptr_ptr[1]);
            break;

        case PURC_VARIANT_TYPE_NATIVE:
            nr = snprintf(buf, len, "<native: %p>", v->ptr_ptr[0]);
            break;

        default:
            PC_ASSERT(0);
    }

    PC_ASSERT(nr >= 0 && (size_t)nr < len);
    return buf;
}

static int
stringify_diff(purc_variant_t l, purc_variant_t r, bool caseless)
{
    char lbuf[128], rbuf[128];
    const char *ls, *rs;
    ls = stringify(lbuf, sizeof(lbuf), l);
    rs = stringify(rbuf, sizeof(rbuf), r);

    if (caseless)
        return pcutils_strcasecmp(ls, rs);
    else
        return strcmp(ls, rs);
}

static int
scalar_diff(purc_variant_t l, purc_variant_t r, void *ctxt)
{
    struct comp_ex_data *data;
    data = (struct comp_ex_data*)ctxt;

    if (l == r) {
        data->diff = 0;
        return data->diff;
    }

    if (l == PURC_VARIANT_INVALID) {
        data->diff = -1;
        return data->diff;
    }

    if (r == PURC_VARIANT_INVALID) {
        data->diff = 1;
        return data->diff;
    }

    if (!pcvariant_is_scalar(l) && !pcvariant_is_scalar(r)) {
        data->diff = l->type - r->type;
        PC_ASSERT(data->diff);
        return data->diff;
    }

    if (pcvariant_is_scalar(l) && pcvariant_is_scalar(r)) {
        if (l->type == r->type) {
            data->diff = homo_scalar_diff(l, r, data);
            return data->diff;
        }

        switch (data->opt) {
            case PCVRNT_COMPARE_METHOD_AUTO:
                data->diff = l->type - r->type;
                return data->diff;

            case PCVRNT_COMPARE_METHOD_NUMBER:
                data->diff = numerify_diff(l, r);
                return data->diff;

            case PCVRNT_COMPARE_METHOD_CASE:
                data->diff = stringify_diff(l, r, false);
                return data->diff;

            case PCVRNT_COMPARE_METHOD_CASELESS:
                data->diff = stringify_diff(l, r, true);
                return data->diff;

            default:
                PC_ASSERT(0);
        }
    }

    data->diff = l->type - r->type;
    return data->diff;
}

int pcvariant_diff_ex(purc_variant_t l, purc_variant_t r,
        enum pcvrnt_compare_method opt)
{
    struct comp_ex_data data = {
        .opt           = opt,
        .diff          = 0,
    };

    pcvar_parallel_walk(l, r, &data, scalar_diff);
    return data.diff;
}

int
pcvar_compare_ex(purc_variant_t l, purc_variant_t r,
        bool caseless, bool unify_number)
{
    if (l == r)
        return 0;
    else if (l == PURC_VARIANT_INVALID)
        return -1;
    else if (r == PURC_VARIANT_INVALID)
        return 1;

    if (unify_number &&
            pcvariant_is_of_number(l) &&
            pcvariant_is_of_number(r))
    {
        bool ok;
        bool force = false;
        long double ldl, ldr;
        ok = purc_variant_cast_to_longdouble(l, &ldl, force);
        PC_ASSERT(ok);
        ok = purc_variant_cast_to_longdouble(r, &ldr, force);
        PC_ASSERT(ok);

        // FIXME: Inifinity/NaN
        PC_ASSERT(!isnan(ldl) && !isinf(ldl));
        PC_ASSERT(!isnan(ldr) && !isinf(ldr));

        if (equal_long_doubles(ldl, ldr))
            return 0;

        if (ldl < ldr)
            return -1;
        if (ldl > ldr)
            return 1;

        PC_ASSERT(0);
    }

    int diff;
    const char *ls, *rs;
    const unsigned char *lb, *rb;

    if (l->type != r->type)
        return l->type - r->type;

    switch(l->type) {
        case PURC_VARIANT_TYPE_UNDEFINED:
            return 0;

        case PURC_VARIANT_TYPE_NULL:
            return 0;

        case PURC_VARIANT_TYPE_BOOLEAN:
            return l->b - r->b;

        case PURC_VARIANT_TYPE_EXCEPTION:
            return l->atom - r->atom;

        case PURC_VARIANT_TYPE_NUMBER:
            if (equal_doubles(l->d, r->d))
                return 0;

            return (l->d < r->d) ? -1 : 1;

        case PURC_VARIANT_TYPE_LONGINT:
            if (l->i64 == r->i64)
                return 0;
            return (l->i64 < r->i64) ? -1 : 1;

        case PURC_VARIANT_TYPE_ULONGINT:
            if (l->u64 == r->u64)
                return 0;
            return (l->u64 < r->u64) ? -1 : 1;

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            if (equal_long_doubles(l->ld, r->ld))
                return 0;

            return (l->ld < r->ld) ? -1 : 1;

        case PURC_VARIANT_TYPE_ATOMSTRING:
            return l->atom - r->atom;

        case PURC_VARIANT_TYPE_STRING:
            ls = purc_variant_get_string_const(l);
            rs = purc_variant_get_string_const(r);
            if (caseless)
                return pcutils_strcasecmp(ls, rs);

            return strcmp(ls, rs);

        case PURC_VARIANT_TYPE_BSEQUENCE:
            // NOTE: caseless is ignored
            lb = (const unsigned char*)l->sz_ptr[1];
            rb = (const unsigned char*)r->sz_ptr[1];
            if (l->sz_ptr[0] < r->sz_ptr[0]) {
                diff = memcmp(lb, rb, l->sz_ptr[0]);
                if (diff)
                    return diff;
                return -1;
            }
            else if (l->sz_ptr[0] == r->sz_ptr[0]) {
                return memcmp(lb, rb, l->sz_ptr[0]);
            }
            else {
                diff = memcmp(lb, rb, l->sz_ptr[0]);
                if (diff)
                    return diff;
                return 1;
            }

        case PURC_VARIANT_TYPE_DYNAMIC:
        case PURC_VARIANT_TYPE_NATIVE:
            // NOTE: compare by addresses
            return memcmp(l->ptr_ptr, r->ptr_ptr, sizeof(void *) * 2) == 0;

        case PURC_VARIANT_TYPE_OBJECT:
            return cmp_by_obj(l, r, caseless, unify_number);

        case PURC_VARIANT_TYPE_ARRAY:
            return cmp_by_arr(l, r, caseless, unify_number);

        case PURC_VARIANT_TYPE_SET:
            return cmp_by_set(l, r, caseless, unify_number);

        case PURC_VARIANT_TYPE_TUPLE:
            return cmp_by_tuple(l, r, caseless, unify_number);

        default:
            PC_ASSERT(0);
            break;
    }

    return 0;
}

bool
purc_variant_linear_container_size(purc_variant_t container, size_t *sz)
{
    if (container == PURC_VARIANT_INVALID)
        return false;

    if (container->type == PURC_VARIANT_TYPE_ARRAY) {
        return purc_variant_array_size(container, sz);
    }
    else if (container->type == PURC_VARIANT_TYPE_SET) {
        return purc_variant_set_size(container, sz);
    }
    else if (container->type == PURC_VARIANT_TYPE_TUPLE) {
        return purc_variant_tuple_size(container, sz);
    }

    return false;
}

purc_variant_t
purc_variant_linear_container_get(purc_variant_t container, size_t idx)
{
    if (container == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    if (container->type == PURC_VARIANT_TYPE_ARRAY) {
        return purc_variant_array_get(container, idx);
    }
    else if (container->type == PURC_VARIANT_TYPE_SET) {
        return purc_variant_set_get_by_index(container, idx);
    }
    else if (container->type == PURC_VARIANT_TYPE_TUPLE) {
        return purc_variant_tuple_get(container, idx);
    }

    return PURC_VARIANT_INVALID;
}

bool
purc_variant_linear_container_set(purc_variant_t container,
        size_t idx, purc_variant_t value)
{
    if (container == PURC_VARIANT_INVALID)
        return false;

    if (container->type == PURC_VARIANT_TYPE_ARRAY) {
        return purc_variant_array_set(container, idx, value);
    }
    else if (container->type == PURC_VARIANT_TYPE_SET) {
        return purc_variant_set_set_by_index(container, idx, value);
    }
    else if (container->type == PURC_VARIANT_TYPE_TUPLE) {
        return purc_variant_tuple_set(container, idx, value);
    }

    return false;
}

static void
do_stringify_md5(struct stringify_arg *arg, const void *src, size_t len)
{
    pcutils_md5_ctxt *ud;
    ud = (pcutils_md5_ctxt*)(arg->arg);

    if (len == 0)
        len = strlen(src);

    pcutils_md5_hash(ud, src, len);
}

void pcvariant_md5_ex(char *md5, purc_variant_t val, const char *salt,
    bool caseless, unsigned int serialize_flags)
{
    PC_ASSERT(caseless == false);

    PC_ASSERT(val != PURC_VARIANT_INVALID);

    pcutils_md5_ctxt ud;

    pcutils_md5_begin(&ud);

    struct stringify_arg arg;
    arg.cb    = do_stringify_md5;
    arg.arg   = &ud;
    arg.flags = serialize_flags;

    variant_stringify(&arg, val);

    if (salt)
        pcutils_md5_hash(&ud, salt, strlen(salt));

    unsigned char md5_digest[PCUTILS_MD5_DIGEST_SIZE];
    pcutils_md5_end(&ud, md5_digest);

    bool uppercase = true;
    pcutils_bin2hex(md5_digest, PCUTILS_MD5_DIGEST_SIZE, md5, uppercase);
}

void
pcvariant_md5_by_set(char *md5, purc_variant_t val, purc_variant_t set)
{
    PC_ASSERT(md5);
    PC_ASSERT(val != PURC_VARIANT_INVALID);
    PC_ASSERT(set != PURC_VARIANT_INVALID);

    variant_set_t data = pcvar_set_get_data(set);
    PC_ASSERT(data);

    if (data->unique_key == NULL) {
        pcvariant_md5(md5, val);
        return;
    }

    purc_variant_t undefined = purc_variant_make_undefined();
    PC_ASSERT(undefined);

    if (val->type != PVT(_OBJECT)) {
        pcvariant_md5(md5, undefined);
        purc_variant_unref(undefined);
        return;
    }

    pcutils_md5_ctxt ud;

    pcutils_md5_begin(&ud);

    struct stringify_arg arg;
    arg.cb    = do_stringify_md5;
    arg.arg   = &ud;
    arg.flags = 0;

    for (size_t i=0; i<data->nr_keynames; ++i) {
        purc_variant_t v;
        v = purc_variant_object_get_by_ckey(val, data->keynames[i]);
        if (v == PURC_VARIANT_INVALID) {
            v = undefined;
        }
        stringify_kv(&arg, data->keynames[i], v);
    }

    unsigned char md5_digest[PCUTILS_MD5_DIGEST_SIZE];
    pcutils_md5_end(&ud, md5_digest);

    bool uppercase = true;
    pcutils_bin2hex(md5_digest, PCUTILS_MD5_DIGEST_SIZE, md5, uppercase);

    purc_variant_unref(undefined);
}

int
pcvariant_diff_by_set(const char *md5l, purc_variant_t l,
        const char *md5r, purc_variant_t r, purc_variant_t set)
{
    // TODO: https://gitlab.fmsoft.cn/hvml/hvml-docs/-/blob/master/zh/hvml-spec-v1.0-zh.md#21610-%E9%9B%86%E5%90%88%E5%8F%98%E9%87%8F

    PC_ASSERT(md5l);
    PC_ASSERT(l != PURC_VARIANT_INVALID);
    PC_ASSERT(md5r);
    PC_ASSERT(r != PURC_VARIANT_INVALID);

    PC_ASSERT(set != PURC_VARIANT_INVALID);

    int diff;

    variant_set_t data = pcvar_set_get_data(set);
    PC_ASSERT(data);

    if (data->unique_key == NULL) {
        return pcvariant_diff(l, r);
    }

    diff = strcmp(md5l, md5r);
    if (diff)
        return diff;

    purc_variant_t undefined = purc_variant_make_undefined();
    PC_ASSERT(undefined);

    for (size_t i=0; i<data->nr_keynames; ++i) {
        purc_variant_t vl = PURC_VARIANT_INVALID;
        purc_variant_t vr = PURC_VARIANT_INVALID;

        if (l->type == PVT(_OBJECT))
            vl = purc_variant_object_get_by_ckey(l, data->keynames[i]);
        if (r->type == PVT(_OBJECT))
            vr = purc_variant_object_get_by_ckey(r, data->keynames[i]);

        if (vl == PURC_VARIANT_INVALID)
            vl = undefined;
        if (vr == PURC_VARIANT_INVALID)
            vr = undefined;

        diff = pcvariant_diff(vl, vr);
        if (diff)
            break;
    }

    purc_variant_unref(undefined);

    return diff;
}

bool pcvariant_is_scalar(purc_variant_t v)
{
    switch (v->type) {
        case PURC_VARIANT_TYPE_UNDEFINED:
        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_BOOLEAN:
        case PURC_VARIANT_TYPE_EXCEPTION:
        case PURC_VARIANT_TYPE_NUMBER:
        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_ULONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
        case PURC_VARIANT_TYPE_ATOMSTRING:
        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
        case PURC_VARIANT_TYPE_DYNAMIC:
        case PURC_VARIANT_TYPE_NATIVE:
            return true;

        case PURC_VARIANT_TYPE_OBJECT:
        case PURC_VARIANT_TYPE_ARRAY:
        case PURC_VARIANT_TYPE_SET:
        case PURC_VARIANT_TYPE_TUPLE:
            return false;

        default:
            PC_ASSERT(0);
            break;
    }

    return false;
}

bool pcvariant_is_of_number(purc_variant_t v)
{
    switch (v->type) {
        case PURC_VARIANT_TYPE_UNDEFINED:
        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_BOOLEAN:
        case PURC_VARIANT_TYPE_EXCEPTION:
            return false;

        case PURC_VARIANT_TYPE_NUMBER:
        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_ULONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
            return true;

        case PURC_VARIANT_TYPE_ATOMSTRING:
        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
        case PURC_VARIANT_TYPE_DYNAMIC:
        case PURC_VARIANT_TYPE_NATIVE:
        case PURC_VARIANT_TYPE_OBJECT:
        case PURC_VARIANT_TYPE_ARRAY:
        case PURC_VARIANT_TYPE_SET:
        case PURC_VARIANT_TYPE_TUPLE:
            return false;

        default:
            PC_ASSERT(0);
            break;
    }

    return false;
}

bool pcvariant_is_of_string(purc_variant_t v)
{
    switch (v->type) {
        case PURC_VARIANT_TYPE_UNDEFINED:
        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_BOOLEAN:
        case PURC_VARIANT_TYPE_EXCEPTION:
            return true;

        case PURC_VARIANT_TYPE_NUMBER:
            return false;

        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_ULONGINT:
            return true;

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            return false;

        case PURC_VARIANT_TYPE_ATOMSTRING:
        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            return true;

        case PURC_VARIANT_TYPE_DYNAMIC:
        case PURC_VARIANT_TYPE_NATIVE:
        case PURC_VARIANT_TYPE_OBJECT:
        case PURC_VARIANT_TYPE_ARRAY:
        case PURC_VARIANT_TYPE_SET:
        case PURC_VARIANT_TYPE_TUPLE:
            return false;

        default:
            PC_ASSERT(0);
            break;
    }

    return false;
}

bool
pcvariant_is_linear_container(purc_variant_t v)
{
    if (!v) {
        return false;
    }

    enum purc_variant_type vt = purc_variant_get_type(v);
    return ((vt == PURC_VARIANT_TYPE_ARRAY) ||
              (vt == PURC_VARIANT_TYPE_SET) || (vt == PURC_VARIANT_TYPE_TUPLE));
}
