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
#include "private/errors.h"
#include "private/debug.h"
#include "variant-internals.h"

#include <stdlib.h>
#include <string.h>

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
    NULL,                           // PURC_VARIANT_TYPE_LONGUINT
    NULL,                           // PURC_VARIANT_TYPE_LONGDOUBLE
    NULL,                           // PURC_VARIANT_TYPE_ATOM_STRING
    pcvariant_string_release,       // PURC_VARIANT_TYPE_STRING
    pcvariant_sequence_release,     // PURC_VARIANT_TYPE_SEQUENCE
    NULL,                           // PURC_VARIANT_TYPE_DYNAMIC
    // VWNOTE (ERROR): Please define a releaser for PURC_VARIANT_TYPE_NATIVE
    NULL,                           // PURC_VARIANT_TYPE_NATIVE
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

void pcvariant_init_instance(struct pcinst* inst)
{
    // initialize const values in instance
    inst->variant_heap.v_null.type = PURC_VARIANT_TYPE_NULL;
    inst->variant_heap.v_null.refc = 1;
    inst->variant_heap.v_null.flags = PCVARIANT_FLAG_NOFREE;

    inst->variant_heap.v_undefined.type = PURC_VARIANT_TYPE_UNDEFINED;
    inst->variant_heap.v_undefined.refc = 1;
    inst->variant_heap.v_undefined.flags = PCVARIANT_FLAG_NOFREE;

    inst->variant_heap.v_false.type = PURC_VARIANT_TYPE_UNDEFINED;
    inst->variant_heap.v_false.refc = 1;
    inst->variant_heap.v_false.flags = PCVARIANT_FLAG_NOFREE;
    inst->variant_heap.v_false.b = false;

    inst->variant_heap.v_true.type = PURC_VARIANT_TYPE_UNDEFINED;
    inst->variant_heap.v_true.refc = 1;
    inst->variant_heap.v_true.flags = PCVARIANT_FLAG_NOFREE;
    inst->variant_heap.v_true.b = true;

    /* VWNOTE: there are two values of boolean.  */
    struct purc_variant_stat *stat = &(inst->variant_heap.stat);
    stat->nr_values[PURC_VARIANT_TYPE_NULL] = 1;
    stat->sz_mem[PURC_VARIANT_TYPE_NULL] = sizeof(purc_variant);
    stat->nr_values[PURC_VARIANT_TYPE_UNDEFINED] = 1;
    stat->sz_mem[PURC_VARIANT_TYPE_UNDEFINED] = sizeof(purc_variant);
    stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN] = 2;
    stat->sz_mem[PURC_VARIANT_TYPE_BOOLEAN] = sizeof(purc_variant) * 2;
    stat->nr_total_values = 4;
    stat->sz_total_mem = 4 * sizeof(purc_variant);

    stat->nr_reserved = 0;
    stat->nr_max_reserved = MAX_RESERVED_VARIANTS;

    // VWNOTE: this is redundant
    memset(inst->variant_heap.v_reserved, 0,
            sizeof(inst->variant_heap.v_reserved));
    inst->variant_heap.headpos = 0;
    inst->variant_heap.tailpos = 0;

    // initialize others
}

void pcvariant_cleanup_instance(struct pcinst* inst)
{
    struct pcvariant_heap * heap = &(inst->variant_heap);
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

    purc_variant_t variant = NULL;

    value->refc++;
    switch (value->type) {
        case PURC_VARIANT_TYPE_OBJECT:
            foreach_value_in_variant_object(value, variant)
                purc_variant_ref(variant);
            end_foreach;
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            foreach_value_in_variant_array(value, variant)
                purc_variant_ref(variant);
            end_foreach;
            break;

        case PURC_VARIANT_TYPE_SET:
            PC_ASSERT(0);
            foreach_value_in_variant_set(value, variant)
                purc_variant_ref(variant);
            end_foreach;
            break;

        default:
            break;
    }
    return value->refc;
}

unsigned int purc_variant_unref(purc_variant_t value)
{
    PC_ASSERT(value);

    purc_variant_t variant = NULL;

    /* this should not occur */
    if (value->refc == 0) {
        PC_ASSERT(0);
        return 0;
    }

    switch ((int)value->type) {
        case PURC_VARIANT_TYPE_OBJECT:
            foreach_value_in_variant_object(value, variant)
                purc_variant_unref(variant);
            end_foreach;
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            foreach_value_in_variant_array(value, variant)
                purc_variant_unref(variant);
            end_foreach;
            break;

        case PURC_VARIANT_TYPE_SET:
            PC_ASSERT(0);
            foreach_value_in_variant_set(value, variant)
                purc_variant_unref(variant);
            end_foreach;
            break;

        default:
            break;
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
    struct pcinst * inst = pcinst_current();
    if (inst == NULL) {
        /* VWNOTE: if you have the instance, directly set the errcode. */
        inst->errcode = PURC_ERROR_NO_INSTANCE;
        return NULL;
    }

    return &inst->variant_heap.stat;
}

void pcvariant_stat_set_extra_size(purc_variant_t value, size_t extra_size)
{
    struct pcinst * instance = pcinst_current();

    PC_ASSERT(value);
    PC_ASSERT(instance);

    struct purc_variant_stat * stat = &(instance->variant_heap.stat);
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
    struct pcinst * instance = pcinst_current();
    struct pcvariant_heap * heap = &(instance->variant_heap);
    struct purc_variant_stat * stat = &(heap->stat);

    PC_ASSERT(value);

    if ((heap->headpos + 1) % MAX_RESERVED_VARIANTS == heap->tailpos) {
        free_variant(value);

        stat->sz_mem[value->type] -= sizeof(purc_variant);
        stat->sz_total_mem -= sizeof(purc_variant);
    }
    else {
        heap->v_reserved[heap->headpos] = value;
        heap->headpos = (heap->headpos + 1) % MAX_RESERVED_VARIANTS;

        /* VWNOTE: do not forget to set nr_reserved. */
        stat->nr_reserved++;
    }

    // set stat information
    stat->nr_values[value->type]--;
    stat->nr_total_values--;
}

int purc_variant_compare(purc_variant_t v1, purc_variant_t v2)
{
    UNUSED_PARAM(v1);
    UNUSED_PARAM(v2);
    PC_ASSERT(0);
    return -1;
}

purc_variant_t purc_variant_load_from_json_stream(purc_rwstream_t stream)
{
    // TODO
    UNUSED_PARAM(stream);
    return PURC_VARIANT_INVALID;
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

