/*
 * @file variant-basic.c
 * @author 
 * @date 2021/07/02
 * @brief The implementation of variant.
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

#include <stdlib.h>
#include <string.h>

#include "purc-variant.h"
#include "variant.h"

// for release the resource in a variant
static void pcvariant_string_release  (struct purc_variant_t value);
static void pcvariant_sequence_release(struct purc_variant_t value);
static void pcvariant_dynamic_release (struct purc_variant_t value);
static void pcvariant_native_release  (struct purc_variant_t value);
static void pcvariant_object_release  (struct purc_variant_t value);
static void pcvariant_array_release   (struct purc_variant_t value);
static void pcvariant_set_release     (struct purc_variant_t value);

// for serialize a variant
static int pcvariant_undefined_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags);
static int pcvariant_null_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags);
static int pcvariant_boolean_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags);
static int pcvariant_number_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags);
static int pcvariant_longint_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags);
static int pcvariant_longdouble_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags);
static int pcvariant_string_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags);
static int pcvariant_sequence_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags);
static int pcvariant_dynamic_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags);
static int pcvariant_native_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags);
static int pcvariant_object_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags);
static int pcvariant_array_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags);
static int pcvariant_set_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags);


static pcvariant_release_fn pcvariant_release[PURC_VARIANT_TYPE_MAX] = 
{
    NULL,                           // PURC_VARIANT_TYPE_UNDEFINED
    NULL,                           // PURC_VARIANT_TYPE_NULL
    NULL,                           // PURC_VARIANT_TYPE_BOOLEAN
    NULL,                           // PURC_VARIANT_TYPE_NUMBER
    NULL,                           // PURC_VARIANT_TYPE_LONGINT
    NULL,                           // PURC_VARIANT_TYPE_LONGDOUBLE
    pcvariant_string_release,       // PURC_VARIANT_TYPE_STRING
    pcvariant_sequence_release,     // PURC_VARIANT_TYPE_SEQUENCE
    pcvariant_dynamic_release,      // PURC_VARIANT_TYPE_DYNAMIC
    pcvariant_native_release,       // PURC_VARIANT_TYPE_NATIVE
    pcvariant_object_release,       // PURC_VARIANT_TYPE_OBJECT
    pcvariant_array_release,        // PURC_VARIANT_TYPE_ARRAY
    pcvariant_set_release,          // PURC_VARIANT_TYPE_SET
};

static pcvariant_to_json_string_fn pcvariant_serialize[PURC_VARIANT_TYPE_MAX] = 
{
    pcvariant_null_to_json_string,        // PURC_VARIANT_TYPE_NULL
    pcvariant_undefined_to_json_string,   // PURC_VARIANT_TYPE_UNDEFINED
    pcvariant_boolean_to_json_string,     // PURC_VARIANT_TYPE_BOOLEAN
    pcvariant_number_to_json_string,      // PURC_VARIANT_TYPE_NUMBER
    pcvariant_longint_to_json_string,     // PURC_VARIANT_TYPE_LONGINT
    pcvariant_longdouble_to_json_string,  // PURC_VARIANT_TYPE_LONGDOUBLE
    pcvariant_string_to_json_string,      // PURC_VARIANT_TYPE_STRING
    pcvariant_sequence_to_json_string,    // PURC_VARIANT_TYPE_SEQUENCE
    pcvariant_dynamic_to_json_string,     // PURC_VARIANT_TYPE_DYNAMIC
    pcvariant_native_to_json_string,      // PURC_VARIANT_TYPE_NATIVE
    pcvariant_object_to_json_string,      // PURC_VARIANT_TYPE_OBJECT
    pcvariant_array_to_json_string,       // PURC_VARIANT_TYPE_ARRAY
    pcvariant_set_to_json_string,         // PURC_VARIANT_TYPE_SET
};

static void pcvariant_string_release(struct purc_variant_t string)
{
    PURC_VARIANT_ASSERT(string);

    if(purc_variant_is_type(string, PURC_VARIANT_TYPE_STRING))
    {
        if(string->size == PCVARIANT_FLAG_LONG)
        {
            pcvariant_free_mem((size_t)string->sz_ptr[1], string->sz_ptr[0]);
        }
    }
}

static void pcvariant_sequence_release(struct purc_variant_t sequence)
{
    PURC_VARIANT_ASSERT(sequence);

    if(purc_variant_is_type(sequence, PURC_VARIANT_TYPE_STRING))
    {
        if(sequence->size == PCVARIANT_FLAG_LONG)
        {
            pcvariant_free_mem((size_t)sequence->sz_ptr[1], sequence->sz_ptr[0]);
        }
    }
}

static void pcvariant_dynamic_release (struct purc_variant_t value)
{
    // do nothing now
}

static void pcvariant_native_release  (struct purc_variant_t value)
{
    // do nothing now
}

static void pcvariant_object_release  (struct purc_variant_t value)
{
}

static void pcvariant_array_release   (struct purc_variant_t value)
{
}

static void pcvariant_set_release     (struct purc_variant_t value)
{
}

static int pcvariant_undefined_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)
{
}

static int pcvariant_null_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)
{
}

static int pcvariant_boolean_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)
{
}

static int pcvariant_number_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)
{
}

static int pcvariant_longint_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)
{
}

static int pcvariant_longdouble_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)
{
}

static int pcvariant_string_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)
{
}

static int pcvariant_sequence_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)
{
}

static int pcvariant_dynamic_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)
{
}

static int pcvariant_native_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)
{
}

static int pcvariant_object_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)
{
}

static int pcvariant_array_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)
{
}

static int pcvariant_set_to_json_string(struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)
{
}


// API for variant
purc_variant_t purc_variant_make_undefined (void)
{
    struct pcinst * pcinstance = pcinst_current();
    purc_variant_t variant_undefined = pcinstance->variant_const->pcvariant_undefined;

    purc_variant_ref(variant_undefined);
    return variant_undefined;
}

purc_variant_t purc_variant_make_null (void)
{
    struct pcinst * pcinstance = pcinst_current();
    purc_variant_t variant_null = pcinstance->variant_const->pcvariant_null;

    purc_variant_ref(variant_null);
    return variant_null;
}

purc_variant_t purc_variant_make_boolean (bool b)
{
    purc_variant_t variant_bool = NULL;
    struct pcinst * pcinstance = pcinst_current();

    if(b)
        variant_bool = pcinstance->variant_const->pcvariant_true;
    else
        variant_bool = pcinstance->variant_const->pcvariant_false;

    purc_variant_ref(variant_bool);
    return variant_bool;
}

purc_variant_t purc_variant_make_number (double d)
{
    purc_variant_t variant_number = (purc_variant_t)pcvariant_alloc_mem_0 \
                                                (sizeof(struct purc_variant));

    if(variant_number == NULL)
        return PURC_VARIANT_INVALID;

    variant_number->type = PURC_VARIANT_TYPE_NUMBER;
    variant_number->size = 0;
    variant_number->flags = 0;
    variant_number->refc = 1;
    variant_number->d = d;

    return variant_number;
}

purc_variant_t purc_variant_make_longuint (uint64_t u64)
{
    purc_variant_t variant_longuint = (purc_variant_t)pcvariant_alloc_mem_0 \
                                                (sizeof(struct purc_variant));

    if(variant_longuint == NULL)
        return PURC_VARIANT_INVALID;

    variant_longuint->type = PURC_VARIANT_TYPE_LONGINT;
    variant_longuint->size = 0;
    variant_longuint->flags = 0;
    variant_longuint->refc = 1;
    variant_longuint->u64 = u64;

    return variant_longuint;
}

purc_variant_t purc_variant_make_longint (uint64_t u64)
{
    purc_variant_t variant_longint = (purc_variant_t)pcvariant_alloc_mem_0 \
                                                (sizeof(struct purc_variant));

    if(variant_longint == NULL)
        return PURC_VARIANT_INVALID;

    variant_longint->type = PURC_VARIANT_TYPE_LONGINT;
    variant_longint->size = PCVARIANT_FLAG_SIGNED;
    variant_longint->flags = 0;
    variant_longint->refc = 1;
    variant_longint->u64 = i64;

    return variant_longint;
}

purc_variant_t purc_variant_make_longdouble (long double lf)
{
    purc_variant_t variant_longdouble = (purc_variant_t)pcvariant_alloc_mem_0 \
                                                (sizeof(struct purc_variant));

    if(variant_longdouble == NULL)
        return PURC_VARIANT_INVALID;

    variant_longdouble->type = PURC_VARIANT_TYPE_LONGDOUBLE;
    variant_longdouble->size = 0;
    variant_longdouble->flags = 0;
    variant_longdouble->refc = 1;
    variant_longdouble->ld = lf;

    return variant_longdouble;
}

purc_variant_t purc_variant_make_string (const char* str_utf8)
{
    int str_size = strlen(str_utf8);
    int real_size = MAX(sizeof(long double), sizeof(void*) * 2);
    purc_variant_t variant_string = (purc_variant_t)pcvariant_alloc_mem_0 \
                                                (sizeof(struct purc_variant));
    
    if(variant_string == NULL)
        return PURC_VARIANT_INVALID;

    if(str_size < (real_size - 1))
    {
        memcpy(variant_string->bytes, str_utf8, str_size + 1);
        variant_string->size = str_size;
    }
    else
    {
        variant_string->size = PCVARIANT_FLAG_LONG;
        variant_string->sz_ptr[0] = pcvariant_alloc_mem_0(str_size + 1);
        if(variant_string->sz_ptr[0] == NULL)
        {
            pcvariant_free_mem(sizeof(struct purc_variant), variant_string);
            return PURC_VARIANT_INVALID;
        }

        variant_string->sz_ptr[1] = (uintptr_t)str_size;
        memcpy(variant_string->sz_ptr[0], str_utf8, str_size);
    }

    variant_string->type = PURC_VARIANT_TYPE_STRING;
    variant_string->flags = 0;
    variant_string->refc = 1;

    return variant_string;

}

static bool purc_variant_string_check_utf8(const char* str_utf8)
{
    // todo

    return true;
}

purc_variant_t purc_variant_make_string_with_check (const char* str_utf8)
{
    purc_variant_t variant_string = NULL;
    bool b_check = purc_variant_string_check_utf8(str_utf8);

    if(b_check)
        variant_string = purc_variant_make_string(str_utf8)
    else
        variant_string = PURC_VARIANT_TYPE_STRING;

    return variant_string;
}

const char* purc_variant_get_string_const (purc_variant_t string)
{
    const char * str_str = NULL;

    PURC_VARIANT_ASSERT(string);

    if(purc_variant_is_type(string, PURC_VARIANT_TYPE_STRING))
    {
        if(string->size < PCVARIANT_FLAG_LONG)
            str_str = string->bytes;
        else
            str_str = string->sz_ptr[0];
    }

    return str_str;
}

size_t purc_variant_string_length(const purc_variant_t string)
{
    size_t str_size = 0;

    PURC_VARIANT_ASSERT(string);

    if(purc_variant_is_type(string, PURC_VARIANT_TYPE_STRING))
    {
        if(string->size < PCVARIANT_FLAG_LONG)
            str_size = string->size;
        else
            str_size = (size_t)string->sz_ptr[1];
    }

    return str_size;
}

purc_variant_t purc_variant_make_byte_sequence (const unsigned char* bytes, \
                                                            size_t nr_bytes)
{
    PURC_VARIANT_ASSERT(byte);

    int real_size = MAX(sizeof(long double), sizeof(void*) * 2);
    purc_variant_t variant_sequence = (purc_variant_t)pcvariant_alloc_mem_0 \
                                                (sizeof(struct purc_variant)); 
    
    if(variant_sequence == NULL)
        return PURC_VARIANT_INVALID;

    if(nr_bytes <= real_size)
    {
        variant_sequence->size = nr_bytes;
        memcpy(variant_sequence->bytes, bytes, nr_bytes);
    }
    else
    {
        variant_sequence->size = PCVARIANT_FLAG_LONG;
        variant_sequence->sz_ptr[0] = pcvariant_alloc_mem_0(nr_bytes);
        if(variant_sequence->sz_ptr[0] == NULL)
        {
            free(variant_sequence);
            return PURC_VARIANT_INVALID;
        }

        variant_sequence->sz_ptr[1] = nr_bytes;
        memcpy(variant_sequence->sz_ptr[0], bytes, nr_bytes);
    }

    variant_sequence->type = PURC_VARIANT_TYPE_SEQUENCE;
    variant_sequence->flags = 0;
    variant_sequence->refc = 1;

    return variant_sequence;
}

const unsigned char* purc_variant_get_bytes_const (purc_variant_t sequence, \
                                                            size_t* nr_bytes)
{
    const unsigned char * bytes = NULL;

    PURC_VARIANT_ASSERT(sequence);

    if(purc_variant_is_type(sequence, PURC_VARIANT_TYPE_SEQUENCE))
    {
        if(sequence->size < PCVARIANT_FLAG_LONG)
            bytes = sequence->bytes;
        else
            bytes = sequence->sz_ptr[0];
    }

    return bytes;
}

size_t purc_variant_sequence_length(const purc_variant_t sequence)
{
    size_t nr_bytes = 0;

    PURC_VARIANT_ASSERT(sequence);

    if(purc_variant_is_type(sequence, PURC_VARIANT_TYPE_SEQUENCE))
    {
        if(sequence->size < PCVARIANT_FLAG_LONG)
            nr_bytes = sequence->size;
        else
            nr_bytes = (size_t)sequence->sz_ptr[1];
    }

    return nr_bytes;
}

purc_variant_t purc_variant_make_dynamic_value (CB_DYNAMIC_VARIANT getter, \
                                                        CB_DYNAMIC_VARIANT setter)
{
    // getter and setter can be NULL
    purc_variant_t purc_variant_dynamic = (purc_variant_t)pcvariant_alloc_mem_0 \
                                                        (sizeof(purc_variant));

    if(purc_variant_dynamic == NULL)
        return PURC_VARIANT_INVALID;

    purc_variant_dynamic->type = PURC_VARIANT_TYPE_DYNAMIC;
    purc_variant_dynamic->size = 0;
    purc_variant_dynamic->flags = 0;
    purc_variant_dynamic->refc = 1;
    purc_variant_dynamic->ptr2[0] = getter;
    purc_variant_dynamic->ptr2[1] = setter;

    return purc_variant_dynamic;
}

purc_variant_t purc_variant_make_native (void *native_obj, \
                                            purc_nvariant_releaser releaser)
{
    purc_variant_t purc_variant_native = (purc_variant_t)pcvariant_alloc_mem_0 \
                                                        (sizeof(purc_variant));

    if(purc_variant_native == NULL)
        return PURC_VARIANT_INVALID;

    purc_variant_native->type = PURC_VARIANT_TYPE_NATIVE;
    purc_variant_native->size = 0;
    purc_variant_native->flags = 0;
    purc_variant_native->refc = 1;
    purc_variant_native->ptr2[0] = native_obj;
    purc_variant_native->ptr2[1] = releaser;

    return purc_variant_native;
}

bool purc_variant_is_type(const purc_variant_t value, enum purc_variant_type type)
{
    return (value->type == type);
}


enum purc_variant_type purc_variant_get_type(const purc_variant_t value)
{
    return value->type;
}


unsigned int purc_variant_ref (purc_variant_t value)
{
    PURC_VARIANT_ASSERT(value);

    enum purc_variant_type type = purc_variant_get_type(value);
    purc_variant_t variant = NULL;

    switch((int)value->type)
    {
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
            value->refc ++;
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            foreach_value_in_variant_object(value, variant)
                purc_variant_ref(variant);
            end_foreach
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            foreach_value_in_variant_array(value, variant)
                purc_variant_ref(variant);
            end_foreach
            break;

        case PURC_VARIANT_TYPE_SET:
            foreach_value_in_variant_set(value, variant)
                purc_variant_ref(variant);
            end_foreach
            break;

        default:
            break;
    }
    return value->refc;
}

unsigned int purc_variant_unref (purc_variant_t value)
{
    PURC_VARIANT_ASSERT(value);
    PURC_VARIANT_ASSERT(value->refc);

    enum purc_variant_type type = purc_variant_get_type(value);
    purc_variant_t variant = NULL;

    switch((int)value->type)
    {
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
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            foreach_value_in_variant_object(value, variant)
                purc_variant_unref(variant);
            end_foreach
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            foreach_value_in_variant_array(value, variant)
                purc_variant_unref(variant);
            end_foreach
            break;

        case PURC_VARIANT_TYPE_SET:
            foreach_value_in_variant_set(value, variant)
                purc_variant_unref(variant);
            end_foreach
            break;

        default:
            break;
    }

    value->refc --;

    if(value->refc == 0)
    {
        if(value->flags & PCVARIANT_FLAG_NOFREE)
        {
            // keep the resource, do not remove it
        }
        else
        {
            // release resource occupied by variant
            pcvariant_release_fn release = pcvariant_release[value->type];
            if(release) 
                release(value);

            // release variant
            pcvariant_free_mem(sizeof(struct purc_variant), value);
            return 0;
        }
    }
    else if(value->refc < 0)
        value->refc = 0;

    return value->refc;
}

purc_variant_t purc_variant_make_from_json_string (const char* json, size_t sz)
{
}

purc_variant_t purc_variant_load_from_json_file (const char* file)
{
}

purc_variant_t purc_variant_load_from_json_stream (purc_rwstream_t stream)
{
}

purc_variant_t purc_variant_dynamic_value_load_from_so (const char* so_name, \
                                                        const char* var_name)
{
}

size_t purc_variant_serialize (purc_variant_t value, purc_rwstream_t stream, \
                                                            unsigned int opts)
{
}

int purc_variant_compare (purc_variant_t v1, purc_variant v2)
{
}

