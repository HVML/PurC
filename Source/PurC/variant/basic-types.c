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

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "private/tls.h"

#include "purc-variant.h"
#include "private/variant.h"
#include "variant_internals.h"

#include <stdlib.h>
#include <string.h>

#if 0
static pcvariant_to_json_string_fn pcvariant_serialize[PURC_VARIANT_TYPE_MAX] = 
{
    pcvariant_null_to_json_string,        // PURC_VARIANT_TYPE_NULL
    pcvariant_undefined_to_json_string,   // PURC_VARIANT_TYPE_UNDEFINED
    pcvariant_boolean_to_json_string,     // PURC_VARIANT_TYPE_BOOLEAN
    pcvariant_number_to_json_string,      // PURC_VARIANT_TYPE_NUMBER
    pcvariant_longint_to_json_string,     // PURC_VARIANT_TYPE_LONGINT
    pcvariant_longdouble_to_json_string,  // PURC_VARIANT_TYPE_LONGDOUBLE
    pcvariant_string_to_json_string,      // PURC_VARIANT_TYPE_STRING
    pcvariant_atom_string_to_json_string, // PURC_VARIANT_TYPE_ATOM_STRING
    pcvariant_sequence_to_json_string,    // PURC_VARIANT_TYPE_SEQUENCE
    pcvariant_dynamic_to_json_string,     // PURC_VARIANT_TYPE_DYNAMIC
    pcvariant_native_to_json_string,      // PURC_VARIANT_TYPE_NATIVE
    pcvariant_object_to_json_string,      // PURC_VARIANT_TYPE_OBJECT
    pcvariant_array_to_json_string,       // PURC_VARIANT_TYPE_ARRAY
    pcvariant_set_to_json_string,         // PURC_VARIANT_TYPE_SET
};

int pcvariant_undefined_to_json_string(purc_variant_t value, purc_rwstream * rw, int level, int flags)
{
}

int pcvariant_null_to_json_string(purc_variant_t value, purc_rwstream * rw, int level, int flags)
{
}

int pcvariant_boolean_to_json_string(purc_variant_t value, purc_rwstream * rw, int level, int flags)
{
}

int pcvariant_number_to_json_string(purc_variant_t value, purc_rwstream * rw, int level, int flags)
{
}

int pcvariant_longint_to_json_string(purc_variant_t value, purc_rwstream * rw, int level, int flags)
{
}

int pcvariant_longdouble_to_json_string(purc_variant_t value, purc_rwstream * rw, int level, int flags)
{
}

int pcvariant_string_to_json_string(purc_variant_t value, purc_rwstream * rw, int level, int flags)
{
}

int pcvariant_sequence_to_json_string(purc_variant_t value, purc_rwstream * rw, int level, int flags)
{
}

int pcvariant_dynamic_to_json_string(purc_variant_t value, purc_rwstream * rw, int level, int flags)
{
}

int pcvariant_native_to_json_string(purc_variant_t value, purc_rwstream * rw, int level, int flags)
{
}

int pcvariant_object_to_json_string(purc_variant_t value, purc_rwstream * rw, int level, int flags)
{
}

int pcvariant_array_to_json_string(purc_variant_t value, purc_rwstream * rw, int level, int flags)
{
}

int pcvariant_set_to_json_string(purc_variant_t value, purc_rwstream * rw, int level, int flags)
{
}

#endif

// API for variant
purc_variant_t purc_variant_make_undefined (void)
{
    struct pcinst * instance = pcinst_current();
    purc_variant_t variant_undefined = &(instance->variant_heap.v_null);

    purc_variant_ref(variant_undefined);
    return variant_undefined;
}

purc_variant_t purc_variant_make_null (void)
{
    struct pcinst * instance = pcinst_current();
    purc_variant_t variant_null = &(instance->variant_heap.v_undefined);

    purc_variant_ref(variant_null);
    return variant_null;
}

purc_variant_t purc_variant_make_boolean (bool b)
{
    purc_variant_t variant_bool = NULL;
    struct pcinst * instance = pcinst_current();

    if(b)
        variant_bool = &(instance->variant_heap.v_true);
    else
        variant_bool = &(instance->variant_heap.v_false);

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

purc_variant_t purc_variant_make_longint (int64_t u64)
{
    purc_variant_t variant_longint = (purc_variant_t)pcvariant_alloc_mem_0 \
                                                (sizeof(struct purc_variant));

    if(variant_longint == NULL)
        return PURC_VARIANT_INVALID;

    variant_longint->type = PURC_VARIANT_TYPE_LONGINT;
    variant_longint->flags = 0;
    variant_longint->flags |= PCVARIANT_FLAG_SIGNED;
    variant_longint->refc = 1;
    variant_longint->u64 = u64;

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


static bool purc_variant_string_check_utf8(const char* str_utf8)
{
    size_t i = 0;
    int nBytes = 0;
    unsigned char ch = 0;
    int length = strlen(str_utf8);

    while((int)i < length)
    {
        ch = *(str_utf8 + i);
        if(nBytes == 0)
        {
            if((ch & 0x80) != 0)
            {
                while((ch & 0x80) != 0)
                {
                    ch <<= 1;
                    nBytes ++;
                }
                if((nBytes < 2) || (nBytes > 6))
                {
                    return false;
                }
                nBytes --;
            }
        }
        else
        {
            if((ch & 0xc0) != 0x80)
            {
                return false;
            }
            nBytes --;
        }
        i ++;
    }

    return (nBytes == 0);
}

purc_variant_t purc_variant_make_string (const char* str_utf8, bool check_encoding)
{
    int str_size = strlen(str_utf8);
    int real_size = MAX(sizeof(long double), sizeof(void*) * 2);
    purc_variant_t variant_string = NULL;
    
    
    if(check_encoding)
    {
        if(!purc_variant_string_check_utf8(str_utf8))
            return PURC_VARIANT_INVALID;
    }
    
    variant_string = (purc_variant_t)pcvariant_alloc_mem_0(sizeof(struct purc_variant));
    
    if(variant_string == NULL)
        return PURC_VARIANT_INVALID;

    variant_string->type = PURC_VARIANT_TYPE_STRING;
    variant_string->flags = 0;
    variant_string->refc = 1;

    if(str_size < (real_size - 1))
    {
        memcpy(variant_string->bytes, str_utf8, str_size + 1);
        variant_string->size = str_size;
    }
    else
    {
        variant_string->flags |= PCVARIANT_FLAG_LONG;
        variant_string->sz_ptr[0] = (uintptr_t)pcvariant_alloc_mem_0(str_size + 1);
        if(variant_string->sz_ptr[0] == 0)
        {
            pcvariant_free_mem(sizeof(struct purc_variant), variant_string);
            return PURC_VARIANT_INVALID;
        }

        variant_string->sz_ptr[1] = (uintptr_t)str_size;
        memcpy((void *)variant_string->sz_ptr[0], str_utf8, str_size);
    }

    return variant_string;

}

const char* purc_variant_get_string_const (purc_variant_t string)
{
    const char * str_str = NULL;

    PURC_VARIANT_ASSERT(string);

    if(purc_variant_is_type(string, PURC_VARIANT_TYPE_STRING))
    {
        if(string->flags & PCVARIANT_FLAG_LONG)
            str_str = (char *)string->sz_ptr[0];
        else
            str_str = (char *)string->bytes;
    }

    return str_str;
}

size_t purc_variant_string_length(const purc_variant_t string)
{
    size_t str_size = 0;

    PURC_VARIANT_ASSERT(string);

    if(purc_variant_is_type(string, PURC_VARIANT_TYPE_STRING))
    {
        if(string->flags & PCVARIANT_FLAG_LONG)
            str_size = (size_t)string->sz_ptr[1];
        else
            str_size = string->size;
    }

    return str_size;
}

void pcvariant_string_release(purc_variant_t string)
{
    PURC_VARIANT_ASSERT(string);

    if(purc_variant_is_type(string, PURC_VARIANT_TYPE_STRING))
    {
        if(string->flags & PCVARIANT_FLAG_LONG)
        {
            pcvariant_free_mem((size_t)string->sz_ptr[1], (void *)string->sz_ptr[0]);
        }
    }
}

// todo
purc_variant_t purc_variant_make_atom_string (const char* str_utf8, bool check_encoding)
{
    int str_size = strlen(str_utf8);
    int real_size = MAX(sizeof(long double), sizeof(void*) * 2);
    purc_variant_t variant_string = (purc_variant_t)pcvariant_alloc_mem_0 \
                                                (sizeof(struct purc_variant));
    
    check_encoding = check_encoding;

    if(variant_string == NULL)
        return PURC_VARIANT_INVALID;

    variant_string->type = PURC_VARIANT_TYPE_STRING;
    variant_string->flags = 0;
    variant_string->refc = 1;

    if(str_size < (real_size - 1))
    {
        memcpy(variant_string->bytes, str_utf8, str_size + 1);
        variant_string->size = str_size;
    }
    else
    {
        variant_string->flags |= PCVARIANT_FLAG_LONG;
        variant_string->sz_ptr[0] = (uintptr_t)pcvariant_alloc_mem_0(str_size + 1);
        if(variant_string->sz_ptr[0] == 0)
        {
            pcvariant_free_mem(sizeof(struct purc_variant), variant_string);
            return PURC_VARIANT_INVALID;
        }

        variant_string->sz_ptr[1] = (uintptr_t)str_size;
        memcpy((void *)variant_string->sz_ptr[0], str_utf8, str_size);
    }

    return variant_string;
}

// todo
purc_variant_t purc_variant_make_atom_string_static (const char* str_utf8, \
                                                        bool check_encoding)
{
    purc_variant_t variant_string = NULL;
    bool b_check = purc_variant_string_check_utf8(str_utf8);

    check_encoding = check_encoding;

    if(b_check)
        variant_string = purc_variant_make_string(str_utf8, check_encoding);
    else
        variant_string = PURC_VARIANT_INVALID;

    return variant_string;
}

// todo
const char* purc_variant_get_atom_string_const (purc_variant_t string)
{
    const char * str_str = NULL;

    PURC_VARIANT_ASSERT(string);

    if(purc_variant_is_type(string, PURC_VARIANT_TYPE_STRING))
    {
        if(string->flags & PCVARIANT_FLAG_LONG)
            str_str = (char *)string->sz_ptr[0];
        else
            str_str = (char *)string->bytes;
    }

    return str_str;
}

void pcvariant_atom_string_release(purc_variant_t string)
{
    PURC_VARIANT_ASSERT(string);

    if(purc_variant_is_type(string, PURC_VARIANT_TYPE_ATOM_STRING))
    {
    }
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

    variant_sequence->type = PURC_VARIANT_TYPE_SEQUENCE;
    variant_sequence->flags = 0;
    variant_sequence->refc = 1;

    if((int)nr_bytes <= real_size)
    {
        variant_sequence->size = nr_bytes;
        memcpy(variant_sequence->bytes, bytes, nr_bytes);
    }
    else
    {
        variant_sequence->flags |= PCVARIANT_FLAG_LONG;
        variant_sequence->sz_ptr[0] = (uintptr_t)pcvariant_alloc_mem_0(nr_bytes);
        if(variant_sequence->sz_ptr[0] == 0)
        {
            free(variant_sequence);
            return PURC_VARIANT_INVALID;
        }

        variant_sequence->sz_ptr[1] = nr_bytes;
        memcpy((void *)variant_sequence->sz_ptr[0], bytes, nr_bytes);
    }

    return variant_sequence;
}

const unsigned char* purc_variant_get_bytes_const (purc_variant_t sequence, \
                                                            size_t* nr_bytes)
{
    const unsigned char * bytes = NULL;

    PURC_VARIANT_ASSERT(sequence);

    if(purc_variant_is_type(sequence, PURC_VARIANT_TYPE_SEQUENCE))
    {
        if(sequence->flags & PCVARIANT_FLAG_LONG)
        {
            bytes = (unsigned char *)sequence->sz_ptr[0];
            * nr_bytes = (size_t)sequence->sz_ptr[1];
        }
        else
        {
            bytes = sequence->bytes;
            * nr_bytes = sequence->size;
        }
    }

    return bytes;
}

size_t purc_variant_sequence_length(const purc_variant_t sequence)
{
    size_t nr_bytes = 0;

    PURC_VARIANT_ASSERT(sequence);

    if(purc_variant_is_type(sequence, PURC_VARIANT_TYPE_SEQUENCE))
    {
        if(sequence->flags & PCVARIANT_FLAG_LONG)
            nr_bytes = (size_t)sequence->sz_ptr[1];
        else
            nr_bytes = sequence->size;
    }

    return nr_bytes;
}

void pcvariant_sequence_release(purc_variant_t sequence)
{
    PURC_VARIANT_ASSERT(sequence);

    if(purc_variant_is_type(sequence, PURC_VARIANT_TYPE_STRING))
    {
        if(sequence->flags & PCVARIANT_FLAG_LONG)
        {
            pcvariant_free_mem((size_t)sequence->sz_ptr[1], (void *)sequence->sz_ptr[0]);
        }
    }
}

purc_variant_t purc_variant_make_dynamic_value (purc_dvariant_method getter, \
                                                   purc_dvariant_method setter)
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
