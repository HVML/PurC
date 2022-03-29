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

#include "private/variant.h"
#include "private/instance.h"
#include "private/ejson.h"
#include "private/vcm.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/dvobjs.h"
#include "private/stringbuilder.h"
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
};


/* Make sure the number of error messages matches the number of error codes */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(msgs,
        PCA_TABLESIZE(variant_err_msgs) == PCVARIANT_ERROR_NR);

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

void pcvariant_init_once(void)
{
    // register error message
    pcinst_register_error_message_segment(&_variant_err_msgs_seg);

    pcvariant_move_heap_init_once();
    atexit(pcvariant_move_heap_cleanup_once);

    // initialize others
    pcvariant_atom_grow = purc_atom_from_static_string("grow");
    pcvariant_atom_shrink = purc_atom_from_static_string("shrink");
    pcvariant_atom_change = purc_atom_from_static_string("change");
    // pcvariant_atom_reference = purc_atom_from_static_string("reference");
    // pcvariant_atom_unreference = purc_atom_from_static_string("unreference");
}

void pcvariant_init_instance(struct pcinst *inst)
{
    inst->variant_heap = calloc(1, sizeof(*inst->variant_heap));
    if (inst->variant_heap == NULL)
        return;

    inst->org_vrt_heap = inst->variant_heap;

    // initialize const values in instance
    inst->variant_heap->v_undefined.type = PURC_VARIANT_TYPE_UNDEFINED;
    inst->variant_heap->v_undefined.refc = 0;
    inst->variant_heap->v_undefined.flags = PCVARIANT_FLAG_NOFREE;
    // INIT_LIST_HEAD(&inst->variant_heap->v_undefined.listeners);

    inst->variant_heap->v_null.type = PURC_VARIANT_TYPE_NULL;
    inst->variant_heap->v_null.refc = 0;
    inst->variant_heap->v_null.flags = PCVARIANT_FLAG_NOFREE;
    // INIT_LIST_HEAD(&inst->variant_heap->v_null.listeners);

    inst->variant_heap->v_false.type = PURC_VARIANT_TYPE_BOOLEAN;
    inst->variant_heap->v_false.refc = 0;
    inst->variant_heap->v_false.flags = PCVARIANT_FLAG_NOFREE;
    inst->variant_heap->v_false.b = false;
    // INIT_LIST_HEAD(&inst->variant_heap->v_false.listeners);

    inst->variant_heap->v_true.type = PURC_VARIANT_TYPE_BOOLEAN;
    inst->variant_heap->v_true.refc = 0;
    inst->variant_heap->v_true.flags = PCVARIANT_FLAG_NOFREE;
    inst->variant_heap->v_true.b = true;
    // INIT_LIST_HEAD(&inst->variant_heap->v_true.listeners);

    inst->variant_heap->gc = NULL;

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
}

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
};

/* Make sure the number of variant types matches the size of `type_names` */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(types, PCA_TABLESIZE(typenames) == PURC_VARIANT_TYPE_NR);
#undef _COMPILE_TIME_ASSERT

const char* purc_variant_typename(enum purc_variant_type type)
{
    assert(type >= 0 && type < PURC_VARIANT_TYPE_NR);
    return typenames[type];
}

// experiment
struct gc_slot {
    purc_variant_t      *vals;
    size_t               sz;
    size_t               nr;
};

struct pcvariant_gc {
    struct gc_slot *slots;
    size_t          sz;
    size_t          nr;
};

static void
gc_slot_release(struct gc_slot *slot)
{
    if (slot) {
        for (size_t i=0; i<slot->nr; ++i) {
            purc_variant_t val = slot->vals[i];
            if (val != PURC_VARIANT_INVALID) {
                purc_variant_unref(val);
            }
            slot->vals[i] = PURC_VARIANT_INVALID;
        }
        free(slot->vals);
        slot->vals  = NULL;
        slot->sz    = 0;
        slot->nr    = 0;
    }
}

static void
gc_release(struct pcvariant_gc *gc)
{
    if (gc) {
        for (size_t i=0; i<gc->sz; ++i) {
            struct gc_slot *slot;
            slot = gc->slots + i;
            gc_slot_release(slot);
        }
        free(gc->slots);
        gc->slots = NULL;
        gc->sz    = 0;
        gc->nr    = 0;
    }
}

static void
gc_destroy(struct pcvariant_gc *gc)
{
    if (gc) {
        gc_release(gc);
        free(gc);
    }
}

void pcvariant_cleanup_instance(struct pcinst *inst)
{
    struct pcvariant_heap *heap = inst->variant_heap;

    if (inst->variables) {
        pcvarmgr_destroy(inst->variables);
        inst->variables = NULL;
    }

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

    if (heap->gc) {
        gc_destroy(heap->gc);
        heap->gc = NULL;
    }

    PC_ASSERT(heap->v_undefined.refc == 0);
    PC_ASSERT(heap->v_null.refc == 0);
    PC_ASSERT(heap->v_true.refc == 0);
    PC_ASSERT(heap->v_false.refc == 0);

    free(heap);
    inst->variant_heap = NULL;
    inst->org_vrt_heap = NULL;
}

bool purc_variant_is_type(purc_variant_t value, enum purc_variant_type type)
{
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
            return true;
        default:
            return false;
    }
}

static inline void
referenced(purc_variant_t value)
{
    // pcvariant_on_post_fired(value, pcvariant_atom_reference, 0, NULL);
    UNUSED_PARAM(value);
}

static inline void
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
    if (value->refc == 0 && !(value->flags & PCVARIANT_FLAG_NOFREE)) {
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

struct purc_variant_stat * purc_variant_usage_stat(void)
{
    struct pcinst *inst = pcinst_current();
    if (inst == NULL) {
        /* VWNOTE: if you have the instance, directly set the errcode. */
        inst->errcode = PURC_ERROR_NO_INSTANCE;
        return NULL;
    }

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

    if (value->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
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
static inline bool equal_doubles(double a, double b)
{
    double max_val = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
    return (fabs(a - b) <= max_val * DBL_EPSILON);
}

/* securely comparison of floating-point variables */
static inline bool equal_long_doubles(long double a, long double b)
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
    struct purc_variant_set_iterator* it;
    purc_variant_t m1, m2;

    if (sz1 != sz2)
        return false;

    it = purc_variant_set_make_iterator_begin(v2);
    foreach_value_in_variant_set_order(v1, m1)

        m2 = purc_variant_set_iterator_get_value(it);
        if (!purc_variant_is_equal_to(m1, m2))
            goto not_equal;

        purc_variant_set_iterator_next(it);
    end_foreach;

    purc_variant_set_release_iterator(it);
    return true;

not_equal:
    purc_variant_set_release_iterator(it);
    return false;
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
            if (v1->flags & PCVARIANT_FLAG_STRING_STATIC) {
                str1 = (const char*)v1->sz_ptr[1];
                len1 = v1->sz_ptr[0];
            }
            else if (v1->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
                str1 = (const char*)v1->sz_ptr[1];
                len1 = v1->sz_ptr[0];
            }
            else {
                str1 = (const char*)v1->bytes;
                len1 = v1->size;
            }

            if (v2->flags & PCVARIANT_FLAG_STRING_STATIC) {
                str2 = (const char*)v2->sz_ptr[1];
                len2 = v2->sz_ptr[0];
            }
            else if (v2->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
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

            if (isinf(v->d) == -1 || v->d <= INT32_MIN) {
                if (force)
                    *i32 = INT32_MIN;
                else
                    break;
            }
            else if (isinf(v->d) == 1 || v->d >= INT32_MAX) {
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

            if (isinf(v->d) == -1 || v->ld <= INT32_MIN) {
                if (force)
                    *i32 = INT32_MIN;
                else
                    break;
            }
            else if (isinf(v->d) == 1 || v->ld >= INT32_MAX) {
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

            if (v->flags & PCVARIANT_FLAG_STRING_STATIC) {
                bytes = (void*)v->sz_ptr[1];
                sz = strlen((const char*)bytes);
            }
            else if (v->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
                bytes = (void*)v->sz_ptr[1];
                sz = v->sz_ptr[0];
            }
            else {
                bytes = (void*)v->bytes;
                sz = v->size;
            }
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

            if (isinf(v->d) == -1 || v->d <= 0) {
                if (force)
                    *u32 = 0;
                else
                    break;
            }
            else if (isinf(v->d) == 1 || v->d >= UINT32_MAX) {
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
            else if (isinf(v->ld) == 1 || v->ld >= UINT32_MAX) {
                *u32 = UINT32_MAX;
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

            if (v->flags & PCVARIANT_FLAG_STRING_STATIC) {
                bytes = (void*)v->sz_ptr[1];
                sz = strlen((const char*)bytes);
            }
            else if (v->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
                bytes = (void*)v->sz_ptr[1];
                sz = v->sz_ptr[0];
            }
            else {
                bytes = (void*)v->bytes;
                sz = v->size;
            }
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

            if (isinf(v->d) == -1 || v->d <= INT64_MIN) {
                if (force)
                    *i64 = INT64_MIN;
                else
                    break;
            }
            else if (isinf(v->d) == 1 || v->d >= INT64_MAX) {
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

            if (isinf(v->d) == -1 || v->ld <= INT64_MIN) {
                if (force)
                    *i64 = INT64_MIN;
                else
                    break;
            }
            else if (isinf(v->d) == 1 || v->ld >= INT64_MAX) {
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

            if (v->flags & PCVARIANT_FLAG_STRING_STATIC) {
                bytes = (void*)v->sz_ptr[1];
                sz = strlen((const char*)bytes);
            }
            else if (v->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
                bytes = (void*)v->sz_ptr[1];
                sz = v->sz_ptr[0];
            }
            else {
                bytes = (void*)v->bytes;
                sz = v->size;
            }
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

            if (v->flags & PCVARIANT_FLAG_STRING_STATIC) {
                bytes = (void*)v->sz_ptr[1];
                sz = strlen((const char*)bytes);
            }
            else if (v->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
                bytes = (void*)v->sz_ptr[1];
                sz = v->sz_ptr[0];
            }
            else {
                bytes = (void*)v->bytes;
                sz = v->size;
            }
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

            if (v->flags & PCVARIANT_FLAG_STRING_STATIC) {
                bytes = (void*)v->sz_ptr[1];
                sz = strlen((const char*)bytes);
            }
            else if (v->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
                bytes = (void*)v->sz_ptr[1];
                sz = v->sz_ptr[0];
            }
            else {
                bytes = (void*)v->bytes;
                sz = v->size;
            }
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

            if (v->flags & PCVARIANT_FLAG_STRING_STATIC) {
                bytes = (void*)v->sz_ptr[1];
                sz = strlen((const char*)bytes);
            }
            else if (v->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
                bytes = (void*)v->sz_ptr[1];
                sz = v->sz_ptr[0];
            }
            else {
                bytes = (void*)v->bytes;
                sz = v->size;
            }
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
                    v->flags & PCVARIANT_FLAG_STRING_STATIC) {
                *bytes = (void*)v->sz_ptr[1];
                *sz = v->sz_ptr[0]; // strlen((const char*)*bytes) + 1;
            }
            else if (v->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
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
    double number1 = purc_variant_numberify (v1);
    double number2 = purc_variant_numberify (v2);

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
        purc_vrtcmp_opt_t opt)
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

    if (opt == PCVARIANT_COMPARE_OPT_CASE)
        compare = (double)strcmp (buf1, buf2);
    else
        compare = (double)strcasecmp (buf1, buf2);

    if (buf1 != stackbuf1)
        free (buf1);
    if (buf2 != stackbuf2)
        free (buf2);

    return compare;
}

int purc_variant_compare_ex (purc_variant_t v1,
        purc_variant_t v2, purc_vrtcmp_opt_t opt)
{
    int compare = 0;

    PC_ASSERT(v1);
    PC_ASSERT(v2);

    if ((opt == PCVARIANT_COMPARE_OPT_CASELESS) ||
            (opt == PCVARIANT_COMPARE_OPT_CASE))
        compare = compare_string_method (v1, v2, opt);
    else if (opt == PCVARIANT_COMPARE_OPT_NUMBER)
        compare = compare_number_method (v1, v2);
    else if (opt == PCVARIANT_COMPARE_OPT_AUTO) {
        if (v1 && ((v1->type == PURC_VARIANT_TYPE_NUMBER) ||
                (v1->type == PURC_VARIANT_TYPE_LONGINT) ||
                (v1->type == PURC_VARIANT_TYPE_ULONGINT) ||
                (v1->type == PURC_VARIANT_TYPE_LONGDOUBLE)))
            compare = compare_number_method (v1, v2);
        else
            compare = compare_string_method (v1, v2, opt);
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

#if OS(LINUX) || OS(UNIX) || OS(MAC_OS_X)
    const char *ext = ".so";
#if OS(MAC_OS_X)
    ext = ".dylib";
#endif
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
            library_handle = dlopen(so, RTLD_LAZY);
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
                library_handle = dlopen(so, RTLD_LAZY);
                if (library_handle) {
                    PC_INFO("Loaded DVObj from %s\n", so);
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
            library_handle = dlopen(so, RTLD_LAZY);
            if (library_handle) {
                PC_INFO("Loaded DVObj from %s\n", so);
                break;
            }
        }

    } while (0);

    if (!library_handle) {
        purc_set_error_with_info(PURC_ERROR_BAD_SYSTEM_CALL,
                "failed to load: %s", so);
        PC_DEBUGX("failed to load: %s", so);
        return PURC_VARIANT_INVALID;
    }
    PC_DEBUGX("loaded: %s", so);

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
        if (dlclose((void *)u64) == 0) {
            purc_variant_unref (dvobj);
        } else {
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

static inline double
numberify_str(const char *s)
{
    if (!s || !*s)
        return 0.0;

    return strtod(s, NULL);
}

static inline double
numberify_bs(const unsigned char *s, size_t nr)
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

static inline double
numberify_dynamic(purc_variant_t value)
{
    purc_dvariant_method getter;
    getter = purc_variant_dynamic_get_getter(value);

    if (!getter)
        return 0.0;

    purc_variant_t v = getter(value, 0, NULL, true); // TODO: silently
    if (v == PURC_VARIANT_INVALID)
        return 0.0;

    double d = purc_variant_numberify(v);
    purc_variant_unref(v);

    return d;
}

static inline double
numberify_native(purc_variant_t value)
{
    void *native = value->ptr_ptr[0];

    struct purc_native_ops *ops;
    ops = (struct purc_native_ops*)value->ptr_ptr[1];

    if (!ops || !ops->property_getter)
        return 0.0;

    purc_nvariant_method getter = (ops->property_getter)("__number");
    if (!getter)
        return 0.0;

    purc_variant_t v = getter(native, 0, NULL, true);  // TODO: silently
    if (v == PURC_VARIANT_INVALID)
        return 0.0;

    double d = purc_variant_numberify(v);
    purc_variant_unref(v);

    return d;
}

static inline double
numberify_array(purc_variant_t value)
{
    size_t sz;
    if (!purc_variant_array_size(value, &sz))
        return 0.0;

    double d = 0.0;

    for (size_t i=0; i<sz; ++i) {
        purc_variant_t v = purc_variant_array_get(value, i);
        d += purc_variant_numberify(v);
    }

    return d;
}

static inline double
numberify_object(purc_variant_t value)
{
    double d = 0.0;

    purc_variant_t v;
    foreach_value_in_variant_object(value, v)
        d += purc_variant_numberify(v);
    end_foreach;

    return d;
}

static inline double
numberify_set(purc_variant_t value)
{
    double d = 0.0;

    purc_variant_t v;
    foreach_value_in_variant_set_order(value, v)
        d += purc_variant_numberify(v);
    end_foreach;

    return d;
}

double
purc_variant_numberify(purc_variant_t value)
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
            return numberify_str(s);
        case PURC_VARIANT_TYPE_STRING:
            s = purc_variant_get_string_const(value);
            return numberify_str(s);
        case PURC_VARIANT_TYPE_BSEQUENCE:
            bs = purc_variant_get_bytes_const(value, &nr);
            return numberify_bs(bs, nr);
        case PURC_VARIANT_TYPE_DYNAMIC:
            return numberify_dynamic(value);
        case PURC_VARIANT_TYPE_NATIVE:
            return numberify_native(value);
        case PURC_VARIANT_TYPE_OBJECT:
            return numberify_object(value);
        case PURC_VARIANT_TYPE_ARRAY:
            return numberify_array(value);
        case PURC_VARIANT_TYPE_SET:
            return numberify_set(value);
        default:
            PC_ASSERT(0);
            break;
    }
}

#if 0
static inline bool
booleanize_str(const char *s)
{
    if (!s || !*s)
        return false;

    return numberify_str(s) != 0.0 ? true : false;
}

static inline bool
booleanize_bs(const unsigned char *s, size_t nr)
{
    if (!s || nr == 0)
        return false;

    return numberify_bs(s, nr) != 0.0 ? true : false;
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

    purc_nvariant_method getter = (ops->property_getter)("__boolean");
    if (!getter)
        return false;

    purc_variant_t v = getter(native, 0, NULL, true);  // TODO: silently
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
        // return numberify_object(value) != 0.0 ? true : false;

    case PURC_VARIANT_TYPE_ARRAY:
        return purc_variant_array_get_size(value) != 0;
        // return numberify_array(value) != 0.0 ? true : false;

    case PURC_VARIANT_TYPE_SET:
        return purc_variant_set_get_size(value) != 0;
        // return numberify_set(value) != 0.0 ? true : false;

    case PURC_VARIANT_TYPE_DYNAMIC:
        return booleanize_dynamic(value);

    case PURC_VARIANT_TYPE_NATIVE:
        return booleanize_native(value);

    default:
        PC_ASSERT(0);
        break;
    }
}

struct stringify_arg
{
    void (*cb)(void *arg, const char *src);
    void *arg;
};

struct stringify_buffer
{
    char                 *buf;
    const size_t          len;

    size_t                curr;
};

static inline void
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
            arg->cb(arg->arg, buffer);
            p = buffer;
        }
    }

    if (p>buffer) {
        *p = '\0';
        arg->cb(arg->arg, buffer);
    }
}

static inline void
variant_stringify(struct stringify_arg *arg, purc_variant_t value);

static inline void
stringify_array(struct stringify_arg *arg, purc_variant_t value)
{
    size_t sz = 0;
    purc_variant_array_size(value, &sz);

    for (size_t i=0; i<sz; ++i) {
        purc_variant_t v = purc_variant_array_get(value, i);
        variant_stringify(arg, v);
        arg->cb(arg->arg, "\n");
    }
    arg->cb(arg->arg, "");
}

static inline void
stringify_object(struct stringify_arg *arg, purc_variant_t value)
{
    purc_variant_t k, v;
    foreach_key_value_in_variant_object(value, k, v)
        variant_stringify(arg, k);
        arg->cb(arg->arg, ":");
        variant_stringify(arg, v);
        arg->cb(arg->arg, "\n");
    end_foreach;
    arg->cb(arg->arg, "");
}

static inline void
stringify_set(struct stringify_arg *arg, purc_variant_t value)
{
    purc_variant_t v;
    foreach_value_in_variant_set_order(value, v)
        variant_stringify(arg, v);
        arg->cb(arg->arg, "\n");
    end_foreach;
    arg->cb(arg->arg, "");
}

static inline void
stringify_dynamic(struct stringify_arg *arg, purc_variant_t value)
{
    purc_dvariant_method getter = purc_variant_dynamic_get_getter(value);
    purc_dvariant_method setter = purc_variant_dynamic_get_setter(value);

    char buf[128];
    snprintf(buf, sizeof(buf), "<dynamic: %p, %p>", getter, setter);
    arg->cb(arg->arg, buf);
}

static inline void
stringify_native(struct stringify_arg *arg, purc_variant_t value)
{
    void *native = purc_variant_native_get_entity(value);

    char buf[128];
    snprintf(buf, sizeof(buf), "<native: %p>", native);
    arg->cb(arg->arg, buf);
}

static inline void
variant_stringify(struct stringify_arg *arg, purc_variant_t value)
{
    const unsigned char *bs;
    size_t nr;
    enum purc_variant_type type = purc_variant_get_type(value);
    char buf[128];

    switch (type)
    {
        case PURC_VARIANT_TYPE_UNDEFINED:
            arg->cb(arg->arg, "undefined");
            break;
        case PURC_VARIANT_TYPE_NULL:
            arg->cb(arg->arg, "null");
            break;
        case PURC_VARIANT_TYPE_BOOLEAN:
            if (value->b) {
                arg->cb(arg->arg, "true");
            } else {
                arg->cb(arg->arg, "false");
            }
            break;
        case PURC_VARIANT_TYPE_EXCEPTION:
            arg->cb(arg->arg,
                    purc_variant_get_exception_string_const(value));
            break;
        case PURC_VARIANT_TYPE_NUMBER:
            snprintf(buf, sizeof(buf), "%g", value->d);
            arg->cb(arg->arg, buf);
            break;
        case PURC_VARIANT_TYPE_LONGINT:
            snprintf(buf, sizeof(buf), "%" PRId64 "", value->i64);
            arg->cb(arg->arg, buf);
            break;
        case PURC_VARIANT_TYPE_ULONGINT:
            snprintf(buf, sizeof(buf), "%" PRIu64 "", value->u64);
            arg->cb(arg->arg, buf);
            break;
        case PURC_VARIANT_TYPE_LONGDOUBLE:
            snprintf(buf, sizeof(buf), "%Lg", value->ld);
            arg->cb(arg->arg, buf);
            break;
        case PURC_VARIANT_TYPE_ATOMSTRING:
            arg->cb(arg->arg,
                    purc_variant_get_atom_string_const(value));
            break;
        case PURC_VARIANT_TYPE_STRING:
            arg->cb(arg->arg,
                    purc_variant_get_string_const(value));
            break;
        case PURC_VARIANT_TYPE_BSEQUENCE:
            bs = purc_variant_get_bytes_const(value, &nr);
            stringify_bs(arg, bs, nr);
            break;
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
        default:
            PC_ASSERT(0);
            break;
    }
}

static inline void
do_stringify_buffer(void *arg, const char *src)
{
    struct stringify_buffer *buffer;
    buffer = (struct stringify_buffer *)arg;

    char *p;
    size_t len;
    int n;

    if (buffer->curr < buffer->len) {
        p = buffer->buf + buffer->curr;
        len = buffer->len - buffer->curr;
        n = snprintf(p, len, "%s", src);
        PC_ASSERT(n >= 0);
    }
    else {
        n = snprintf(NULL, 0, "%s", src);
    }
    buffer->curr += n;
}

ssize_t
purc_variant_stringify_buff(char *buf, size_t len, purc_variant_t value)
{
    struct stringify_buffer buffer = {
        .buf           = buf,
        .len           = len,
    };

    buffer.curr          = 0;

    struct stringify_arg arg;
    arg.cb    = do_stringify_buffer;
    arg.arg   = &buffer;

    variant_stringify(&arg, value);

    return buffer.curr;
}

static inline void
do_stringify_stringbuilder(void *arg, const char *src)
{
    struct pcutils_stringbuilder *sb;
    sb = (struct pcutils_stringbuilder*)arg;

    pcutils_stringbuilder_snprintf(sb, "%s", src);
}

ssize_t
purc_variant_stringify_alloc(char **strp, purc_variant_t value)
{
    struct pcutils_stringbuilder sb;
    pcutils_stringbuilder_init(&sb, 1024);

    struct stringify_arg arg;
    arg.cb    = do_stringify_stringbuilder;
    arg.arg   = &sb;

    variant_stringify(&arg, value);

    ssize_t total = sb.total;

    if (sb.oom) {
        total = -1;
    } else {
        if (strp) {
            *strp = pcutils_stringbuilder_build(&sb);
            if (!*strp)
                total = -1;
        }
    }

    pcutils_stringbuilder_reset(&sb);
    return total;
}

// experiment
static void
gc_push(struct pcvariant_gc *gc)
{
    PC_ASSERT(gc->nr <= gc->sz);
    if (gc->nr == gc->sz) {
        gc->sz += 16;
        gc->slots = (struct gc_slot*)realloc(gc->slots,
                gc->sz * sizeof(*gc->slots));
        PC_ASSERT(gc->slots);
        for (size_t i=gc->nr; i<gc->sz; ++i) {
            struct gc_slot *slot = &gc->slots[i];
            slot->vals = NULL;
            slot->sz   = 0;
            slot->nr   = 0;
        }
    }
    ++gc->nr;
}

static void
gc_pop(struct pcvariant_gc *gc)
{
    PC_ASSERT(gc);
    PC_ASSERT(gc->nr <= gc->sz);
    PC_ASSERT(gc->nr > 0);
    struct gc_slot *slot = &gc->slots[--gc->nr];
    for (size_t i=0; i<slot->nr; ++i) {
        purc_variant_t val = slot->vals[i];
        if (val != PURC_VARIANT_INVALID) {
            purc_variant_unref(val);
            slot->vals[i] = PURC_VARIANT_INVALID;
        }
    }
    slot->nr = 0;
}

void pcvariant_push_gc(void)
{
    struct pcinst *instance = pcinst_current();
    struct pcvariant_heap *heap = instance->variant_heap;
    if (heap->gc == NULL) {
        heap->gc = (struct pcvariant_gc*)calloc(1, sizeof(*heap->gc));
        PC_ASSERT(heap->gc);
    }
    gc_push(heap->gc);
}

void pcvariant_pop_gc(void)
{
    struct pcinst *instance = pcinst_current();
    struct pcvariant_heap *heap = instance->variant_heap;
    PC_ASSERT(heap->gc);
    gc_pop(heap->gc);
}

static void
gc_slot_add_variant(struct gc_slot *slot, purc_variant_t val)
{
    PC_ASSERT(slot->nr <= slot->sz);
    if (slot->nr == slot->sz) {
        slot->sz += 16;
        slot->vals = (purc_variant_t*)realloc(slot->vals,
                slot->sz * sizeof(*slot->vals));
        PC_ASSERT(slot->vals);
    }
    slot->vals[slot->nr++] = val;
    if (val != PURC_VARIANT_INVALID)
        purc_variant_ref(val);
}

static void
gc_add_variant(struct pcvariant_gc *gc, purc_variant_t val)
{
    PC_ASSERT(gc->slots);
    PC_ASSERT(gc->nr < gc->sz);
    struct gc_slot *slot = &gc->slots[gc->nr-1];
    gc_slot_add_variant(slot, val);
}

void pcvariant_gc_add(purc_variant_t val)
{
    PC_ASSERT(val != PURC_VARIANT_INVALID);
    struct pcinst *instance = pcinst_current();
    struct pcvariant_heap *heap = instance->variant_heap;
    PC_ASSERT(heap->gc);
    gc_add_variant(heap->gc, val);
}

void pcvariant_gc_mov(purc_variant_t val)
{
    PC_ASSERT(val != PURC_VARIANT_INVALID);
    pcvariant_gc_add(val);
    purc_variant_unref(val);
}

ssize_t pcvariant_serialize(char *buf, size_t sz, purc_variant_t val)
{
    PC_ASSERT(val != PURC_VARIANT_INVALID);
    purc_rwstream_t out = purc_rwstream_new_from_mem(buf, sz);
    PC_ASSERT(out); // FIXME:

    size_t len_expected = 0;
    ssize_t n;
    n = purc_variant_serialize(val, out,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN,
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
            PCVARIANT_SERIALIZE_OPT_PLAIN | PCVARIANT_SERIALIZE_OPT_UNIQKEYS,
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

int
purc_variant_is_mutable(purc_variant_t var, bool *is_mutable)
{
    if (var == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if (is_mutable)
        *is_mutable = IS_CONTAINER(var->type);

    return 0;
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
    PC_ASSERT(l != PURC_VARIANT_INVALID);
    PC_ASSERT(r != PURC_VARIANT_INVALID);
    int diff;
    diff = purc_variant_compare_ex(l, r, PCVARIANT_COMPARE_OPT_AUTO);
    return diff;
}

struct purc_ejson_parse_tree *
purc_variant_ejson_parse_string(const char *ejson, size_t sz)
{
    struct purc_ejson_parse_tree *ptree;
    purc_rwstream_t rwstream = purc_rwstream_new_from_mem((void*)ejson, sz);
    if (rwstream == NULL)
        return NULL;

    ptree = purc_variant_ejson_parse_stream(rwstream);
    purc_rwstream_destroy(rwstream);

    return ptree;
}

struct purc_ejson_parse_tree *
purc_variant_ejson_parse_file(const char *fname)
{
    struct purc_ejson_parse_tree *ptree;
    purc_rwstream_t rwstream = purc_rwstream_new_from_file(fname, "r");
    if (rwstream == NULL)
        return NULL;

    ptree = purc_variant_ejson_parse_stream(rwstream);
    purc_rwstream_destroy(rwstream);

    return ptree;
}

struct purc_ejson_parse_tree *
purc_variant_ejson_parse_stream(purc_rwstream_t rws)
{
    struct pcvcm_node* root = NULL;
    struct pcejson* parser = NULL;

    int ret = pcejson_parse(&root, &parser, rws, PCEJSON_DEFAULT_DEPTH);
    if (ret == PCEJSON_SUCCESS) {
        pcejson_destroy(parser);
        return (struct purc_ejson_parse_tree *)root;
    }

    pcvcm_node_destroy(root);
    pcejson_destroy(parser);
    return NULL;
}

purc_variant_t
purc_variant_ejson_parse_tree_evalute(struct purc_ejson_parse_tree *parse_tree,
        purc_cb_get_var fn_get_var, void *ctxt, bool silently)
{
    return pcvcm_eval_ex((struct pcvcm_node*)parse_tree, fn_get_var,
           ctxt, silently);
}

void
purc_variant_ejson_parse_tree_destroy(struct purc_ejson_parse_tree *parse_tree)
{
    pcvcm_node_destroy((struct pcvcm_node *)parse_tree);
}

