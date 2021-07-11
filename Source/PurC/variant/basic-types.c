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

#include "purc-utils.h"
#include "purc-variant.h"
#include "private/variant.h"
#include "variant-internals.h"

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

#ifndef MAX
#define MAX(a, b)   (a) > (b)? (a) : (b)
#endif

// API for variant
purc_variant_t purc_variant_make_undefined (void)
{
    struct pcinst * instance = pcinst_current ();
    purc_variant_t value = &(instance->variant_heap.v_null);

    purc_variant_ref (value);

    return value;
}

purc_variant_t purc_variant_make_null (void)
{
    struct pcinst * instance = pcinst_current ();
    purc_variant_t value = &(instance->variant_heap.v_undefined);

    purc_variant_ref (value);

    return value;
}

purc_variant_t purc_variant_make_boolean (bool b)
{
    purc_variant_t value = NULL;
    struct pcinst * instance = pcinst_current ();

    if (b)
        value = &(instance->variant_heap.v_true);
    else
        value = &(instance->variant_heap.v_false);

    purc_variant_ref (value);

    return value;
}

purc_variant_t purc_variant_make_number (double d)
{
    purc_variant_t value = pcvariant_get (PURC_VARIANT_TYPE_NUMBER);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_NUMBER;
    value->size = 0;
    value->flags = 0;
    value->refc = 1;
    value->d = d;

    return value;
}

purc_variant_t purc_variant_make_longuint (uint64_t u64)
{
    purc_variant_t value = pcvariant_get (PURC_VARIANT_TYPE_LONGINT);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_LONGINT;
    value->size = 0;
    value->flags = 0;
    value->refc = 1;
    value->u64 = u64;

    return value;
}

purc_variant_t purc_variant_make_longint (int64_t u64)
{
    purc_variant_t value = pcvariant_get (PURC_VARIANT_TYPE_LONGINT);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_LONGINT;
    value->flags = 0;
    value->size = 8;            // marked, if size == 8, it is signed long int
    value->refc = 1;
    value->u64 = u64;

    return value;
}

purc_variant_t purc_variant_make_longdouble (long double lf)
{
    purc_variant_t value = pcvariant_get (PURC_VARIANT_TYPE_LONGDOUBLE);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_LONGDOUBLE;
    value->size = 0;
    value->flags = 0;
    value->refc = 1;
    value->ld = lf;

    return value;
}


static bool purc_variant_string_check_utf8 (const char* str_utf8)
{
    size_t i = 0;
    int nBytes = 0;
    unsigned char ch = 0;
    int length = strlen(str_utf8);

    while ((int)i < length) {
        ch = *(str_utf8 + i);
        if (nBytes == 0) {
            if ((ch & 0x80) != 0) {
                while ((ch & 0x80) != 0) {
                    ch <<= 1;
                    nBytes ++;
                }
                if ((nBytes < 2) || (nBytes > 6)) {
                    return false;
                }
                nBytes --;
            }
        }
        else {
            if ((ch & 0xc0) != 0x80) 
                return false;
            nBytes --;
        }
        i ++;
    }

    return (nBytes == 0);
}

purc_variant_t
purc_variant_make_string (const char* str_utf8, bool check_encoding)
{
    int str_size = strlen (str_utf8);
    int real_size = MAX (sizeof(long double), sizeof(void*) * 2);
    purc_variant_t value = NULL;

    if (check_encoding) {
        if (!purc_variant_string_check_utf8 (str_utf8)) {
            pcinst_set_error (PCVARIANT_STRING_NOT_UTF8);
            return PURC_VARIANT_INVALID;
        }
    }

    value = pcvariant_get (PURC_VARIANT_TYPE_STRING);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_STRING;
    value->flags = 0;
    value->refc = 1;

    if (str_size < (real_size - 1)) {
        memcpy (value->bytes, str_utf8, str_size + 1);
        value->size = str_size;
    }
    else {
        value->flags |= PCVARIANT_FLAG_EXTRA_SIZE;
        value->sz_ptr[1] = (uintptr_t)malloc (str_size + 1);
        if(value->sz_ptr[1] == 0) {
            pcvariant_put (value);
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PURC_VARIANT_INVALID;
        }

        value->sz_ptr[0] = (uintptr_t)str_size;
        memcpy ((void *)value->sz_ptr[1], str_utf8, str_size);
        pcvariant_stat_extra_memory (value, true, str_size);
    }

    return value;
}

const char* purc_variant_get_string_const (purc_variant_t string)
{
    const char * str_str = NULL;

    PC_ASSERT(string);

    if (purc_variant_is_type (string, PURC_VARIANT_TYPE_STRING)) {
        if (string->flags & PCVARIANT_FLAG_EXTRA_SIZE)
            str_str = (char *)string->sz_ptr[1];
        else
            str_str = (char *)string->bytes;
    }
    else
        pcinst_set_error (PCVARIANT_INVALID_TYPE);

    return str_str;
}

size_t purc_variant_string_length (const purc_variant_t string)
{
    size_t str_size = 0;

    PC_ASSERT(string);

    if (purc_variant_is_type(string, PURC_VARIANT_TYPE_STRING)) {
        if (string->flags & PCVARIANT_FLAG_EXTRA_SIZE)
            str_size = (size_t)string->sz_ptr[0];
        else
            str_size = string->size;
    }
    else
        pcinst_set_error (PCVARIANT_INVALID_TYPE);

    return str_size;
}

void pcvariant_string_release (purc_variant_t string)
{
    PC_ASSERT(string);

    if (purc_variant_is_type (string, PURC_VARIANT_TYPE_STRING)) {
        if (string->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
            pcvariant_stat_extra_memory (string, false,
                                        (size_t)string->sz_ptr[0]);
            free ((void *)string->sz_ptr[1]);
        }
    }
    else
        pcinst_set_error (PCVARIANT_INVALID_TYPE);
}

purc_variant_t
purc_variant_make_atom_string (const char* str_utf8, bool check_encoding)
{
    PC_ASSERT(str_utf8);

    purc_variant_t value = NULL;

    if (check_encoding) {
        if (!purc_variant_string_check_utf8 (str_utf8)) {
            pcinst_set_error (PCVARIANT_STRING_NOT_UTF8);
            return PURC_VARIANT_INVALID;
        }
    }

    purc_atom_t atom = purc_atom_from_string (str_utf8);
    if (atom == 0) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value = pcvariant_get (PURC_VARIANT_TYPE_ATOM_STRING);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_ATOM_STRING;
    value->size = strlen (str_utf8);
    value->flags |= PCVARIANT_FLAG_EXTRA_SIZE;
    value->refc = 1;
    value->sz_ptr[0] = atom;

    return value;
}

purc_variant_t
purc_variant_make_atom_string_static (const char* str_utf8, 
                                            bool check_encoding)
{
    PC_ASSERT(str_utf8);

    purc_variant_t value = NULL;

    if (check_encoding) {
        if (!purc_variant_string_check_utf8 (str_utf8)) {
            pcinst_set_error (PCVARIANT_STRING_NOT_UTF8);
            return PURC_VARIANT_INVALID;
        }
    }

    purc_atom_t atom = purc_atom_from_static_string (str_utf8);
    if (atom == 0) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    value = pcvariant_get (PURC_VARIANT_TYPE_ATOM_STRING);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_ATOM_STRING;
    value->size = strlen (str_utf8);
    value->flags = 0;
    value->refc = 1;
    value->sz_ptr[0] = atom;

    return value;
}

const char* purc_variant_get_atom_string_const (purc_variant_t atom_string)
{
    const char * str_str = NULL;

    PC_ASSERT(atom_string);

    if (purc_variant_is_type (atom_string, PURC_VARIANT_TYPE_ATOM_STRING))
        str_str = purc_atom_to_string(atom_string->sz_ptr[0]);
    else
        pcinst_set_error (PCVARIANT_INVALID_TYPE);

    return str_str;
}


void pcvariant_atom_string_release(purc_variant_t atom_string)
{
    UNUSED_PARAM(atom_string);
}

purc_variant_t purc_variant_make_byte_sequence (const unsigned char* bytes,
        size_t nr_bytes)
{
    PC_ASSERT(bytes);

    int real_size = MAX (sizeof(long double), sizeof(void*) * 2);
    purc_variant_t value = pcvariant_get (PURC_VARIANT_TYPE_SEQUENCE); 

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_SEQUENCE;
    value->flags = 0;
    value->refc = 1;

    if((int)nr_bytes <= real_size)
    {
        value->size = nr_bytes;
        memcpy (value->bytes, bytes, nr_bytes);
    }
    else
    {
        value->flags |= PCVARIANT_FLAG_EXTRA_SIZE;
        value->sz_ptr[1] = (uintptr_t) malloc (nr_bytes);
        if (value->sz_ptr[1] == 0) {
            pcvariant_put (value);
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PURC_VARIANT_INVALID;
        }

        value->sz_ptr[0] = nr_bytes;
        memcpy ((void *)value->sz_ptr[1], bytes, nr_bytes);
        pcvariant_stat_extra_memory (value, true, nr_bytes);
    }

    return value;
}

const unsigned char *
purc_variant_get_bytes_const (purc_variant_t sequence, size_t* nr_bytes)
{
    const unsigned char * bytes = NULL;

    PC_ASSERT(sequence);

    if (purc_variant_is_type(sequence, PURC_VARIANT_TYPE_SEQUENCE)) {
        if (sequence->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
            bytes = (unsigned char *)sequence->sz_ptr[1];
            * nr_bytes = (size_t)sequence->sz_ptr[0];
        }
        else {
            bytes = sequence->bytes;
            * nr_bytes = sequence->size;
        }
    }
    else
        pcinst_set_error (PCVARIANT_INVALID_TYPE);

    return bytes;
}

size_t purc_variant_sequence_length(const purc_variant_t sequence)
{
    size_t nr_bytes = 0;

    PC_ASSERT(sequence);

    if (purc_variant_is_type (sequence, PURC_VARIANT_TYPE_SEQUENCE)) {
        if (sequence->flags & PCVARIANT_FLAG_EXTRA_SIZE)
            nr_bytes = (size_t)sequence->sz_ptr[0];
        else
            nr_bytes = sequence->size;
    }
    else
        pcinst_set_error (PCVARIANT_INVALID_TYPE);

    return nr_bytes;
}

void pcvariant_sequence_release(purc_variant_t sequence)
{
    PC_ASSERT(sequence);

    if (purc_variant_is_type(sequence, PURC_VARIANT_TYPE_STRING)) {
        if (sequence->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
            pcvariant_stat_extra_memory (sequence, false,
                                        (size_t)sequence->sz_ptr[0]);
            free((void *)sequence->sz_ptr[1]);
        }
    }
    else
        pcinst_set_error (PCVARIANT_INVALID_TYPE);
}

purc_variant_t purc_variant_make_dynamic_value (purc_dvariant_method getter,
        purc_dvariant_method setter)
{
    // getter and setter can be NULL
    purc_variant_t value = pcvariant_get (PURC_VARIANT_TYPE_DYNAMIC);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_DYNAMIC;
    value->size = 0;
    value->flags = 0;
    value->refc = 1;
    value->ptr2[0] = getter;
    value->ptr2[1] = setter;

    return value;
}


purc_variant_t purc_variant_make_native (void *native_obj, 
                                            purc_nvariant_releaser releaser)
{
    purc_variant_t value = pcvariant_get (PURC_VARIANT_TYPE_NATIVE);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_NATIVE;
    value->size = 0;
    value->flags = 0;
    value->refc = 1;
    value->ptr2[0] = native_obj;
    value->ptr2[1] = releaser;

    return value;
}
