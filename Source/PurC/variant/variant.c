/*
 * @file variant-public.c
 * @author 
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

// TODO: initialize the table here
static pcvariant_release_fn pcvariant_releasers[PURC_VARIANT_TYPE_MAX] = {
    NULL,                           // PURC_VARIANT_TYPE_UNDEFINED
    NULL,                           // PURC_VARIANT_TYPE_NULL
    NULL,                           // PURC_VARIANT_TYPE_BOOLEAN
    NULL,                           // PURC_VARIANT_TYPE_NUMBER
    NULL,                           // PURC_VARIANT_TYPE_LONGINT
    NULL,                           // PURC_VARIANT_TYPE_LONGDOUBLE
    pcvariant_string_release,       // PURC_VARIANT_TYPE_STRING
    pcvariant_atom_string_release,  // PURC_VARIANT_TYPE_ATOM_STRING
    pcvariant_sequence_release,     // PURC_VARIANT_TYPE_SEQUENCE
    NULL,                           // PURC_VARIANT_TYPE_DYNAMIC
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

void pcvariant_init (void)
{
    // register error message
    pcinst_register_error_message_segment (&_variant_err_msgs_seg);

    // initialize others
}

void pcvariant_init_instance(struct pcinst* inst)
{
    /* VWNOTE: do not call this when initializing the instance
    pcinst_register_error_message_segment(&_variant_err_msgs_seg);
    */

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
    struct purc_variant_stat * stat = &(inst->variant_heap.stat);
    stat->nr_values[PURC_VARIANT_TYPE_NULL] = 1;
    stat->sz_mem[PURC_VARIANT_TYPE_NULL] = sizeof(purc_variant);
    stat->nr_values[PURC_VARIANT_TYPE_UNDEFINED] = 1;
    stat->sz_mem[PURC_VARIANT_TYPE_UNDEFINED] = sizeof(purc_variant);
    stat->nr_values[PURC_VARIANT_TYPE_BOOLEAN] = 2;
    stat->sz_mem[PURC_VARIANT_TYPE_BOOLEAN] = sizeof(purc_variant) * 2;
    stat->nr_total_values = 4;
    stat->sz_total_mem = 4 * sizeof(purc_variant);

    // initialize others
}

void pcvariant_cleanup_instance(struct pcinst* inst)
{
    // TODO: release reserved values here.
    UNUSED_PARAM(inst);
}

bool purc_variant_is_type (const purc_variant_t value,
                            enum purc_variant_type type)
{
    return (value->type == type);
}

enum purc_variant_type purc_variant_get_type (const purc_variant_t value)
{
    return value->type;
}

unsigned int purc_variant_ref (purc_variant_t value)
{
    PCVARIANT_ALWAYS_ASSERT(value);

    purc_variant_t variant = NULL;
    switch ((int)value->type) {
        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_UNDEFINED:
        case PURC_VARIANT_TYPE_BOOLEAN:
        case PURC_VARIANT_TYPE_NUMBER:
        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_SEQUENCE:
        case PURC_VARIANT_TYPE_DYNAMIC:
        case PURC_VARIANT_TYPE_NATIVE:
            value->refc++;
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            foreach_value_in_variant_object (value, variant)
                purc_variant_ref (variant);
            end_foreach;
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            foreach_value_in_variant_array(value, variant)
                purc_variant_ref (variant);
            end_foreach;
            break;

        case PURC_VARIANT_TYPE_SET:
            PCVARIANT_ALWAYS_ASSERT(0);
            foreach_value_in_variant_set (value, variant)
                purc_variant_ref (variant);
            end_foreach;
            break;

        default:
            break;
    }
    return value->refc;
}

unsigned int purc_variant_unref (purc_variant_t value)
{
    PCVARIANT_ALWAYS_ASSERT(value);

    purc_variant_t variant = NULL;

    /* this should not occur */
    if (value->refc == 0) {
        PC_ASSERT (0);
        return 0;
    }

    switch ((int)value->type) {
        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_UNDEFINED:
        case PURC_VARIANT_TYPE_BOOLEAN:
        case PURC_VARIANT_TYPE_NUMBER:
        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_ATOM_STRING:
        case PURC_VARIANT_TYPE_SEQUENCE:
        case PURC_VARIANT_TYPE_DYNAMIC:
        case PURC_VARIANT_TYPE_NATIVE:
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            foreach_value_in_variant_object (value, variant)
                purc_variant_unref (variant);
            end_foreach;
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            foreach_value_in_variant_array (value, variant)
                purc_variant_unref (variant);
            end_foreach;
            break;

        case PURC_VARIANT_TYPE_SET:
            PCVARIANT_ALWAYS_ASSERT(0);
            foreach_value_in_variant_set (value, variant)
                purc_variant_unref (variant);
            end_foreach;
            break;

        default:
            break;
    }

    value->refc--;

    if (value->refc == 0) {
        // release resource occupied by variant
        pcvariant_release_fn release = pcvariant_releasers[value->type];
        if (release)
            release (value);

        if (value->flags & PCVARIANT_FLAG_NOFREE) {
            if (value->type > PURC_VARIANT_TYPE_BOOLEAN) {
                struct pcinst * instance = pcinst_current ();
                PCVARIANT_ALWAYS_ASSERT(instance);

                struct purc_variant_stat * stat = &(instance->variant_heap.stat);

                stat->nr_reserved ++;
            }
        }
        else
        {
            // release variant
            if (value->type > PURC_VARIANT_TYPE_BOOLEAN) {
                pcvariant_put (value);
                return 0;
            }
        }
    }

    return value->refc;
}

bool purc_variant_usage_stat (struct purc_variant_stat* stat)
{
    PCVARIANT_ALWAYS_ASSERT(stat);

    struct pcinst * instance = pcinst_current ();
    PCVARIANT_ALWAYS_ASSERT(instance);

    memcpy(stat, &(instance->variant_heap.stat), sizeof(struct purc_variant_stat));

    return true;
}

// todo
purc_variant_t purc_variant_make_from_json_string (const char* json, size_t sz)
{
    UNUSED_PARAM(json);
    UNUSED_PARAM(sz);

    purc_variant_t value = pcvariant_get (PURC_VARIANT_TYPE_STRING);

    return value;
}

// todo
purc_variant_t purc_variant_load_from_json_file (const char* file)
{
    PCVARIANT_ALWAYS_ASSERT(file);

    purc_rwstream_t rwstream = purc_rwstream_new_from_file (file, "r");
    if (rwstream == NULL)
        return PURC_VARIANT_INVALID;

    // how to get file size? use new rwstream type?
    size_t size = 100;
    size_t read_size = 0;
    unsigned char * buf = malloc(size);

    read_size = purc_rwstream_read (rwstream, buf, size);
    if (read_size == 0)
        return PURC_VARIANT_INVALID;

    purc_variant_t value =
            purc_variant_make_from_json_string ((const char *)buf, size);

    free (buf);
    purc_rwstream_close (rwstream);

    return value;
}

#if 0
purc_variant_t purc_variant_dynamic_value_load_from_so (const char* so_name,
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

#if 0
purc_variant_t purc_variant_load_from_json_stream (purc_rwstream_t stream)
{
}

size_t purc_variant_serialize (purc_variant_t value, purc_rwstream_t stream, \
                                                            unsigned int opts)
{
}

int purc_variant_compare (purc_variant_t v1, purc_variant v2)
{
}
#endif

#if HAVE(GLIB)
static inline void * pcvariant_alloc_mem(size_t size)
                { return (void *)g_slice_alloc((gsize)size); }
static inline void * pcvariant_alloc_mem_0(size_t size)
                { return (void *)g_slice_alloc0((gsize)size); }
static inline void pcvariant_free_mem(size_t size, void *ptr)
                { return g_slice_free1((gsize)size, (gpointer)ptr); }
#else
static inline void * pcvariant_alloc_mem(size_t size)
                { return malloc(size); }
static inline void * pcvariant_alloc_mem_0(size_t size)
                { return (void *)calloc(size); }
static inline void pcvariant_free_mem(size_t size, void *ptr)
                { return free(ptr); }
#endif


// set statistic for additional memory for one variant
void pcvariant_stat_additional_memory (purc_variant_t value, bool add)
{
    struct pcinst * instance = pcinst_current ();

    PCVARIANT_ALWAYS_ASSERT(value);
    PCVARIANT_ALWAYS_ASSERT(instance);

    struct purc_variant_stat * stat = &(instance->variant_heap.stat);
    int type = value->type;

    switch (type)
    {
        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_SEQUENCE:
            if (value->flags & PCVARIANT_FLAG_LONG) {
                if (add) {
                    stat->sz_mem[type] += (size_t)value->sz_ptr[1];
                    stat->sz_total_mem += (size_t)value->sz_ptr[1];
                }
                else {
                    stat->sz_mem[type] -= (size_t)value->sz_ptr[1];
                    stat->sz_total_mem -= (size_t)value->sz_ptr[1];
                }
            }
            break;

        case PURC_VARIANT_TYPE_ATOM_STRING:
            if (!(value->flags & PCVARIANT_FLAG_ATOM_STATIC)) {
                if (add) {
                    stat->sz_mem[type] += (size_t)value->size;
                    stat->sz_total_mem += (size_t)value->size;
                }
                else {
                    // whether invoke it, depend on atom string release
                    stat->sz_mem[type] -= (size_t)value->size;
                    stat->sz_total_mem -= (size_t)value->size;
                }
            }
            break;
    }
}

static void
    pcvariant_set_stat (enum purc_variant_type type, bool reserved, bool direct)
{
    struct pcinst * instance = pcinst_current ();
    PCVARIANT_ALWAYS_ASSERT(instance);

    struct purc_variant_stat * stat = &(instance->variant_heap.stat);

    switch (type)
    {
        // do not set null, undefined, boolean type
        case PURC_VARIANT_TYPE_NUMBER:
        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
        case PURC_VARIANT_TYPE_DYNAMIC:
        case PURC_VARIANT_TYPE_NATIVE:
        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_SEQUENCE:
        case PURC_VARIANT_TYPE_ATOM_STRING:
        case PURC_VARIANT_TYPE_OBJECT:
        case PURC_VARIANT_TYPE_ARRAY:
        case PURC_VARIANT_TYPE_SET:
            if (direct) {
                stat->nr_values[type] ++ ;
                stat->nr_total_values ++ ;
                if (!reserved) {
                    stat->sz_mem[type] += sizeof(purc_variant);
                    stat->sz_total_mem += sizeof(purc_variant);
                }
            }
            else {
                stat->nr_values[type] -- ;
                stat->nr_total_values -- ;
                if (!reserved) {
                    stat->sz_mem[type] -= sizeof(purc_variant);
                    stat->sz_total_mem -= sizeof(purc_variant);
                }
            }
            break;

        default:
            break;
    }
}

purc_variant_t pcvariant_get (enum purc_variant_type type)
{
    purc_variant_t value = NULL;
    struct pcinst * instance = pcinst_current ();
    struct pcvariant_heap * heap = &(instance->variant_heap);

    // it is empty
    if (heap->headpos == heap->tailpos) {
        value = (purc_variant_t)pcvariant_alloc_mem_0 (sizeof(purc_variant));
        if (value == NULL)
            value = PURC_VARIANT_INVALID;
        else
            pcvariant_set_stat (type, false, true);
    }
    else {
        value = heap->nr_reserved[heap->tailpos];
        heap->tailpos = (heap->tailpos + 1) % MAX_RESERVED_VARIANTS;

        if (value == NULL) {
            value = (purc_variant_t)pcvariant_alloc_mem_0 (sizeof(purc_variant));
            if (value == NULL)
                value = PURC_VARIANT_INVALID;
            else
                pcvariant_set_stat (type, false, true);
        }
        else
            pcvariant_set_stat (type, true, true);
    }

    return value;
}

void pcvariant_put (purc_variant_t value)
{
    PCVARIANT_ALWAYS_ASSERT(value);

    struct pcinst * instance = pcinst_current ();
    struct pcvariant_heap * heap = &(instance->variant_heap);

    if ((heap->headpos + 1) % MAX_RESERVED_VARIANTS == heap->tailpos) {
        pcvariant_free_mem (sizeof(struct purc_variant), value);
        pcvariant_set_stat (value->type, false, false);
    }
    else {
        heap->nr_reserved[heap->headpos] = value;
        heap->headpos = (heap->headpos + 1) % MAX_RESERVED_VARIANTS;
        pcvariant_set_stat (value->type, true, false);
    }
}

