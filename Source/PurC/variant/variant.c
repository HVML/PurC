/*
 * @file variant.c
 * @author Geng Yue
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
#include "private/utils.h"
#include "variant-internals.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#if OS(LINUX) || OS(UNIX)
    #include <dlfcn.h>
#endif

#if HAVE(GLIB)
    #include <gmodule.h>
#endif

static pcvariant_release_fn variant_releasers[PURC_VARIANT_TYPE_MAX] = {
    NULL,                           // PURC_VARIANT_TYPE_UNDEFINED
    NULL,                           // PURC_VARIANT_TYPE_NULL
    NULL,                           // PURC_VARIANT_TYPE_BOOLEAN
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


static const char* variant_err_msgs[] = {
    /* PCVARIANT_INVALID_TYPE */
    "Invalid variant type",
    /* PCVARIANT_STRING_NOT_UTF8 */
    "Input string is not in UTF-8 encoding",
};

static struct err_msg_seg _variant_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_FIRST_VARIANT,
    PURC_ERROR_FIRST_VARIANT + PCA_TABLESIZE(variant_err_msgs) - 1,
    variant_err_msgs
};

/*
 * VWNOTE:
 * The author should change the condition mannually to
 *      #if 0
 * in order to compile the statements in other branch
 * to find the potential errors in advance.
 */
#if HAVE(GLIB)
static inline UNUSED_FUNCTION purc_variant *alloc_variant(void) {
    return (purc_variant *)g_slice_alloc(sizeof(purc_variant));
}

static inline purc_variant *alloc_variant_0(void) {
    return (purc_variant *)g_slice_alloc0(sizeof(purc_variant));
}

static inline void free_variant(purc_variant *v) {
    return g_slice_free1(sizeof(purc_variant), (gpointer)v);
}
#else
/*
 * VWNOTE:
 *  - Use UNUSED_FUNCTION for unused inline functions to avoid warnings.
 *  - Use UNUSED_PARAM to avoid compilation warnings.
 */
static inline UNUSED_FUNCTION purc_variant *alloc_variant(void) {
    return (purc_variant *)malloc(sizeof(purc_variant));
}

static inline purc_variant *alloc_variant_0(void) {
    return (purc_variant *)calloc(1, sizeof(purc_variant));
}

static inline void free_variant(purc_variant *v) {
    return free(v);
}
#endif

void pcvariant_init_once(void)
{
    // register error message
    pcinst_register_error_message_segment(&_variant_err_msgs_seg);

    // initialize others
}

void pcvariant_init_instance(struct pcinst *inst)
{
    // initialize const values in instance
    inst->variant_heap.v_undefined.type = PURC_VARIANT_TYPE_UNDEFINED;
    inst->variant_heap.v_undefined.refc = 0;
    inst->variant_heap.v_undefined.flags = PCVARIANT_FLAG_NOFREE;

    inst->variant_heap.v_null.type = PURC_VARIANT_TYPE_NULL;
    inst->variant_heap.v_null.refc = 0;
    inst->variant_heap.v_null.flags = PCVARIANT_FLAG_NOFREE;

    inst->variant_heap.v_false.type = PURC_VARIANT_TYPE_BOOLEAN;
    inst->variant_heap.v_false.refc = 0;
    inst->variant_heap.v_false.flags = PCVARIANT_FLAG_NOFREE;
    inst->variant_heap.v_false.b = false;

    inst->variant_heap.v_true.type = PURC_VARIANT_TYPE_BOOLEAN;
    inst->variant_heap.v_true.refc = 0;
    inst->variant_heap.v_true.flags = PCVARIANT_FLAG_NOFREE;
    inst->variant_heap.v_true.b = true;

    /* VWNOTE: there are two values of boolean.  */
    struct purc_variant_stat *stat = &(inst->variant_heap.stat);
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

    // VWNOTE: this is redundant
    // memset(inst->variant_heap.v_reserved, 0,
    //         sizeof(inst->variant_heap.v_reserved));
    // inst->variant_heap.headpos = 0;
    // inst->variant_heap.tailpos = 0;

    // initialize others
}

void pcvariant_cleanup_instance(struct pcinst *inst)
{
    struct pcvariant_heap *heap = &(inst->variant_heap);
    int i = 0;

    /* VWNOTE: do not try to release the extra memory here. */
    for (i = 0; i < MAX_RESERVED_VARIANTS; i++) {
        if (heap->v_reserved[i]) {
            free_variant(heap->v_reserved[i]);
            heap->v_reserved[i] = NULL;
        }
    }

    heap->headpos = 0;
    heap->tailpos = 0;
}

bool purc_variant_is_type(const purc_variant_t value,
        enum purc_variant_type type)
{
    return (value->type == type);
}

enum purc_variant_type purc_variant_get_type(const purc_variant_t value)
{
    return value->type;
}

unsigned int purc_variant_ref (purc_variant_t value)
{
    PC_ASSERT(value);

    /* this should not occur */
    if (value->refc == 0) {
        PC_ASSERT(0);
        return 0;
    }

    value->refc++;

    return value->refc;
}

unsigned int purc_variant_unref(purc_variant_t value)
{
    PC_ASSERT(value);

    /* this should not occur */
    if (value->refc == 0) {
        PC_ASSERT(0);
        return 0;
    }

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

    purc_variant_t value = &(inst->variant_heap.v_undefined);
    inst->variant_heap.stat.nr_values[PURC_VARIANT_TYPE_UNDEFINED] = value->refc;

    value = &(inst->variant_heap.v_null);
    inst->variant_heap.stat.nr_values[PURC_VARIANT_TYPE_NULL] = value->refc;


    value = &(inst->variant_heap.v_true);
    inst->variant_heap.stat.nr_values[PURC_VARIANT_TYPE_BOOLEAN] = value->refc;
    value = &(inst->variant_heap.v_false);
    inst->variant_heap.stat.nr_values[PURC_VARIANT_TYPE_BOOLEAN] += value->refc;

    return &inst->variant_heap.stat;
}

void pcvariant_stat_set_extra_size(purc_variant_t value, size_t extra_size)
{
    struct pcinst *instance = pcinst_current();

    PC_ASSERT(value);
    PC_ASSERT(instance);

    struct purc_variant_stat *stat = &(instance->variant_heap.stat);
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
    struct pcvariant_heap *heap = &(instance->variant_heap);
    struct purc_variant_stat *stat = &(heap->stat);

    if (heap->headpos == heap->tailpos) {
        // no reserved, allocate one
        value = alloc_variant_0();
        if (value == NULL)
            return PURC_VARIANT_INVALID;

        stat->sz_mem[type] += sizeof(purc_variant);
        stat->sz_total_mem += sizeof(purc_variant);
    }
    else {
        value = heap->v_reserved[heap->tailpos];
        // must have a value.
        PC_ASSERT(value);

        // VWNOTE: set the slot as NULL
        heap->v_reserved[heap->tailpos] = NULL;
        heap->tailpos = (heap->tailpos + 1) % MAX_RESERVED_VARIANTS;

        /* VWNOTE: do not forget to set nr_reserved. */
        stat->nr_reserved--;
    }

    // set stat information
    stat->nr_values[type]++;
    stat->nr_total_values++;

    return value;
}

void pcvariant_put(purc_variant_t value)
{
    struct pcinst *instance = pcinst_current();
    struct pcvariant_heap *heap = &(instance->variant_heap);
    struct purc_variant_stat *stat = &(heap->stat);

    PC_ASSERT(value);

    // set stat information
    stat->nr_values[value->type]--;
    stat->nr_total_values--;

    if ((heap->headpos + 1) % MAX_RESERVED_VARIANTS == heap->tailpos) {
        stat->sz_mem[value->type] -= sizeof(purc_variant);
        stat->sz_total_mem -= sizeof(purc_variant);

        free_variant(value);
    }
    else {
        heap->v_reserved[heap->headpos] = value;
        heap->headpos = (heap->headpos + 1) % MAX_RESERVED_VARIANTS;

        /* VWNOTE: do not forget to set nr_reserved. */
        stat->nr_reserved++;
    }
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

static int compare_objects(purc_variant_t v1, purc_variant_t v2)
{
    int diff;
    purc_variant_t key;
    purc_variant_t m1, m2;
    size_t sz1 = purc_variant_object_get_size(v1);
    size_t sz2 = purc_variant_object_get_size(v2);

    if (sz1 != sz2)
        return (int)(sz1 - sz2);

    foreach_key_value_in_variant_object(v1, key, m1)
        m2 = purc_variant_object_get(v2, key);
        diff = purc_variant_compare(m1, m2);
        if (diff != 0)
            return diff;
    end_foreach;

    return 0;
}

static int compare_arrays(purc_variant_t v1, purc_variant_t v2)
{
    int diff;
    size_t idx;
    size_t sz1 = purc_variant_array_get_size(v1);
    size_t sz2 = purc_variant_array_get_size(v2);
    purc_variant_t m1, m2;

    if (sz1 != sz2)
        return (int)(sz1 - sz2);

    idx = 0;
    foreach_value_in_variant_array(v1, m1)

        m2 = purc_variant_array_get(v2, idx);
        diff = purc_variant_compare(m1, m2);
        if (diff != 0)
            return diff;

        idx++;
    end_foreach;

    return 0;
}

static int compare_sets(purc_variant_t v1, purc_variant_t v2)
{
    int diff;
    size_t sz1 = purc_variant_set_get_size(v1);
    size_t sz2 = purc_variant_set_get_size(v2);
    struct purc_variant_set_iterator* it;
    purc_variant_t m1, m2;

    if (sz1 != sz2)
        return (int)(sz1 - sz2);

    it = purc_variant_set_make_iterator_begin(v2);
    foreach_value_in_variant_set(v1, m1)

        m2 = purc_variant_set_iterator_get_value(it);
        diff = purc_variant_compare(m1, m2);
        if (diff != 0)
            goto ret;

    purc_variant_set_iterator_next(it);
    end_foreach;

    purc_variant_set_release_iterator(it);
    return 0;

ret:
    purc_variant_set_release_iterator(it);
    return diff;
}

bool
purc_variant_cast_to_longint(purc_variant_t v, int64_t *i64, bool parse_str)
{
    const char *bytes;
    size_t sz;

    PC_ASSERT(v);

    switch (v->type) {
        case PURC_VARIANT_TYPE_NULL:
            *i64 = 0;
            return true;

        case PURC_VARIANT_TYPE_BOOLEAN:
            *i64 = (int64_t)v->b;
            return true;

        case PURC_VARIANT_TYPE_NUMBER:
            if (v->d <= INT64_MIN) {
                *i64 = INT64_MIN;
            }
            else if (v->d >= INT64_MAX) {
                *i64 = INT64_MAX;
            }
            else {
                *i64 = (int64_t)v->d;
            }
            return true;

        case PURC_VARIANT_TYPE_LONGINT:
            *i64 = v->i64;
            return true;

        case PURC_VARIANT_TYPE_ULONGINT:
            if (v->u64 >= INT64_MAX)
                *i64 = INT64_MAX;
            else
                *i64 = (int64_t)v->u64;
            return true;

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            if (v->ld <= INT64_MIN) {
                *i64 = INT64_MIN;
            }
            else if (v->ld >= INT64_MAX) {
                *i64 = INT64_MAX;
            }
            else {
                *i64 = (int64_t)v->ld;
            }
            return true;

        case PURC_VARIANT_TYPE_ATOMSTRING:
            if (!parse_str)
                break;

            bytes = purc_atom_to_string(v->sz_ptr[1]);
            sz = strlen(bytes);
            if (pcutils_parse_int64(bytes, sz, i64) != 0) {
                *i64 = 0;
            }
            return true;

        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (!parse_str)
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
purc_variant_cast_to_ulongint(purc_variant_t v, uint64_t *u64, bool parse_str)
{
    const char *bytes;
    size_t sz;

    PC_ASSERT(v);

    switch (v->type) {
        case PURC_VARIANT_TYPE_NULL:
            *u64 = 0;
            return true;

        case PURC_VARIANT_TYPE_BOOLEAN:
            *u64 = (uint64_t)v->b;
            return true;

        case PURC_VARIANT_TYPE_NUMBER:
            if (v->d <= 0) {
                *u64 = 0;
            }
            else if (v->d >= UINT64_MAX) {
                *u64 = UINT64_MAX;
            }
            else {
                *u64 = (uint64_t)v->d;
            }
            return true;

        case PURC_VARIANT_TYPE_LONGINT:
            if (v->i64 < 0)
                *u64 = 0;
            else
                *u64 = (uint64_t)v->i64;
            return true;

        case PURC_VARIANT_TYPE_ULONGINT:
            *u64 = v->u64;
            return true;

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            if (v->ld < 0) {
                *u64 = 0;
            }
            else if (v->ld >= UINT64_MAX) {
                *u64 = UINT64_MAX;
            }
            else {
                *u64 = (uint64_t)v->ld;
            }
            return true;

        case PURC_VARIANT_TYPE_ATOMSTRING:
            if (!parse_str)
                break;

            bytes = purc_atom_to_string(v->sz_ptr[1]);
            sz = strlen(bytes);
            if (pcutils_parse_uint64(bytes, sz, u64) != 0) {
                *u64 = 0;
            }
            return true;

        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (!parse_str)
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

bool purc_variant_cast_to_number(purc_variant_t v, double *d, bool parse_str)
{
    const char *bytes;
    size_t sz;

    PC_ASSERT(v);

    switch (v->type) {
        case PURC_VARIANT_TYPE_NULL:
            *d = 0;
            return true;

        case PURC_VARIANT_TYPE_BOOLEAN:
            *d = (double)v->b;
            return true;

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
            if (!parse_str)
                break;

            bytes = purc_atom_to_string(v->sz_ptr[1]);
            sz = strlen(bytes);
            if (pcutils_parse_double(bytes, sz, d) != 0) {
                *d = 0;
            }
            return true;

        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (!parse_str)
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
purc_variant_cast_to_long_double(purc_variant_t v, long double *d,
        bool parse_str)
{
    const char *bytes;
    size_t sz;

    PC_ASSERT(v);

    switch (v->type) {
        case PURC_VARIANT_TYPE_NULL:
            *d = 0;
            return true;

        case PURC_VARIANT_TYPE_BOOLEAN:
            *d = (long double)v->b;
            return true;

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
            if (!parse_str)
                break;

            bytes = purc_atom_to_string(v->sz_ptr[1]);
            sz = strlen(bytes);
            if (pcutils_parse_long_double(bytes, sz, d) != 0) {
                *d = 0;
            }
            return true;

        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (!parse_str)
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
            *bytes = purc_atom_to_string(v->sz_ptr[1]);
            *sz = strlen(*bytes);
            return true;

        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (v->flags & PCVARIANT_FLAG_STRING_STATIC) {
                *bytes = (void*)v->sz_ptr[1];
                *sz = strlen((const char*)*bytes);
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

int purc_variant_compare(purc_variant_t v1, purc_variant_t v2)
{
    int i;
    const char *str1, *str2;
    size_t len1, len2;

    if (v1 == NULL)
        return v2 ? 1 : 0;
    if (v2 == NULL)
        return v1 ? 1 : 0;

    if (v1->type == v2->type) {
    switch (v1->type) {
        case PURC_VARIANT_TYPE_UNDEFINED:
        case PURC_VARIANT_TYPE_NULL:
            return 0;

        case PURC_VARIANT_TYPE_BOOLEAN:
            return (int)v1->b - (int)v2->b;

        case PURC_VARIANT_TYPE_NUMBER:
            if (equal_doubles(v1->d, v2->d))
                return 0;
            // VWNOTE: this may get zero because of too small difference:
            // return (int)(v1->d - v2->d);
            if (v1->d > v2->d)
                return 1;
            return -1;

        case PURC_VARIANT_TYPE_LONGINT:
            return (int)(v1->i64 - v2->i64);

        case PURC_VARIANT_TYPE_ULONGINT:
            return (int)(v1->u64 - v2->u64);

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            if (equal_long_doubles(v1->ld, v2->ld))
                return 0;
            // VWNOTE: this may get zero because of too small difference:
            // return (int)(v1->d - v2->d);
            if (v1->ld > v2->ld)
                return 1;
            return -1;

        case PURC_VARIANT_TYPE_ATOMSTRING:
            str1 = purc_atom_to_string(v1->sz_ptr[1]);
            str2 = purc_atom_to_string(v2->sz_ptr[1]);
            return strcmp(str1, str2);

        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (v1->flags & PCVARIANT_FLAG_STRING_STATIC) {
                str1 = (const char*)v1->sz_ptr[1];
                len1 = strlen(str1);
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
                len2 = strlen(str2);
            }
            else if (v2->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
                str2 = (const char*)v2->sz_ptr[1];
                len2 = v2->sz_ptr[0];
            }
            else {
                str2 = (const char*)v2->bytes;
                len2 = v2->size;
            }

            if (v1->type == PURC_VARIANT_TYPE_STRING)
                return strcmp(str1, str2);

            i = memcmp(str1, str2, (len1 > len2)?len2:len1);
            if (i == 0 && len1 != len2) {
                return (int)(len1 - len2);
            }
            return i;

        case PURC_VARIANT_TYPE_DYNAMIC:
        case PURC_VARIANT_TYPE_NATIVE:
            return memcmp(v1->ptr_ptr, v2->ptr_ptr, sizeof(void *) * 2);

        case PURC_VARIANT_TYPE_OBJECT:
            return compare_objects(v1, v2);

        case PURC_VARIANT_TYPE_ARRAY:
            return compare_arrays(v1, v2);

        case PURC_VARIANT_TYPE_SET:
            return compare_sets(v1, v2);

        default:
            PC_ASSERT(0);
            break;
    }
    }
    else {
        long double ld1, ld2;
        if (purc_variant_cast_to_long_double(v1, &ld1, false) &&
                purc_variant_cast_to_long_double(v2, &ld2, false)) {
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

        static const char* type_names[PURC_VARIANT_TYPE_MAX] = {
            "undefined",        // PURC_VARIANT_TYPE_UNDEFINED
            "null",             // PURC_VARIANT_TYPE_NULL
            "boolean",          // PURC_VARIANT_TYPE_BOOLEAN
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

    return 0;
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

    value = pcvcm_eval (root, NULL);

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

#if 0
purc_variant_t purc_variant_dynamic_value_load_from_so(const char* so_name,
                                                        const char* var_name)
{
    PCVARIANT_ALWAYS_ASSERT(so_name);
    PCVARIANT_ALWAYS_ASSERT(var_name);

    purc_variant_t value = PURC_VARIANT_INVALID;

// temporarily disable to make sure test cases available
#if OS(LINUX) || OS(UNIX)
    void * library_handle = NULL;

    library_handle = dlopen(so_name, RTLD_LAZY);
    if(!library_handle)
        return PURC_VARIANT_INVALID;

    purc_variant_t (* get_variant_by_name)(const char *);

    get_variant_by_name = (purc_variant_t (*) (const char *))dlsym(library_handle, "get_variant_by_name");
    if(dlerror() != NULL)
    {
        dlclose(library_handle);
        return PURC_VARIANT_INVALID;
    }

    value = get_variant_by_name(var_name);
    if(value == NULL)
    {
        dlclose(library_handle);
        return PURC_VARIANT_INVALID;
    }

    // ??? long string, sequence, atom string, dynamic, native..... can not dlclose....
#else // 0
    UNUSED_PARAM(so_name);
    UNUSED_PARAM(var_name);
#endif // !0
    return value;

}

#endif

purc_variant_t purc_variant_load_dvobj_from_so (const char *so_name,
        const char *var_name)
{
    purc_variant_t value = PURC_VARIANT_INVALID;

#if OS(LINUX) || OS(UNIX)
    purc_variant_t val = PURC_VARIANT_INVALID;
    int ver_code;

    void *library_handle = NULL;

    library_handle = dlopen(so_name, RTLD_LAZY);
    if(!library_handle) {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
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
#endif

    return value;
}

bool purc_variant_unload_dvobj (purc_variant_t dvobj)
{
    bool ret = true;

    if (dvobj == PURC_VARIANT_INVALID)  {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return false;
    }

    if (!purc_variant_is_type (dvobj, PURC_VARIANT_TYPE_OBJECT)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return false;
    }

    uint64_t u64 = 0;
    purc_variant_t val = purc_variant_object_get_by_ckey (dvobj,
            EXOBJ_LOAD_HANDLE_KEY);
    if (val == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return false;
    }

    if (!purc_variant_cast_to_ulongint (val, &u64, false)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
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
