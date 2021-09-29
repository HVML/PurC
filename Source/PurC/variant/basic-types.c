/*
 * @file variant-basic.c
 * @author Geng Yue
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

#ifndef MAX
#define MAX(a, b)   (a) > (b)? (a) : (b)
#endif

// API for variant
purc_variant_t purc_variant_make_undefined (void)
{
    struct pcinst * instance = pcinst_current ();
    purc_variant_t value = &(instance->variant_heap.v_undefined);

    value->refc++;

    return value;
}

purc_variant_t purc_variant_make_null (void)
{
    struct pcinst * instance = pcinst_current ();
    purc_variant_t value = &(instance->variant_heap.v_null);

    value->refc++;

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

    value->refc++;

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

purc_variant_t purc_variant_make_ulongint (uint64_t u64)
{
    purc_variant_t value = pcvariant_get (PURC_VARIANT_TYPE_ULONGINT);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_ULONGINT;
    value->size = 0;
    value->flags = 0;
    value->refc = 1;
    value->u64 = u64;

    return value;
}

purc_variant_t purc_variant_make_longint (int64_t i64)
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
    value->i64 = i64;

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
    PCVARIANT_CHECK_FAIL_RET(str_utf8, PURC_VARIANT_INVALID);

    size_t str_size = strlen (str_utf8);
    size_t real_size = MAX (sizeof(long double), sizeof(void*) * 2);
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
        // VWNOTE: use strcpy instead of memcpy
        strcpy ((char *)value->bytes, str_utf8);
        // VWNOTE: always store the size including the terminating null byte.
        value->size = str_size + 1;
    }
    else {
        char* new_buf;
        new_buf = malloc (str_size + 1);
        if(new_buf == NULL) {
            pcvariant_put (value);
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PURC_VARIANT_INVALID;
        }

        value->flags = PCVARIANT_FLAG_EXTRA_SIZE;
        value->sz_ptr[1] = (uintptr_t)new_buf;
        // VWNOTE: sz_ptr[0] will be set in pcvariant_stat_set_extra_size
        // VWNOTE: use strcpy instead of memcpy
        //value->sz_ptr[0] = (uintptr_t)str_size;
        strcpy (new_buf, str_utf8);
        // VWNOTE: always store the size including the terminating null byte.
        pcvariant_stat_set_extra_size (value, str_size + 1);
    }

    return value;
}


purc_variant_t
purc_variant_make_string_reuse_buff (char* str_utf8, size_t sz_buff,
                                                    bool check_encoding)
{
    PCVARIANT_CHECK_FAIL_RET(str_utf8, PURC_VARIANT_INVALID);

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
    value->flags = PCVARIANT_FLAG_EXTRA_SIZE;
    value->refc = 1;

    value->sz_ptr[1] = (uintptr_t)(str_utf8);
    pcvariant_stat_set_extra_size (value, sz_buff);

    return value;
}


purc_variant_t
purc_variant_make_string_static (const char* str_utf8, bool check_encoding)
{
    PCVARIANT_CHECK_FAIL_RET(str_utf8, PURC_VARIANT_INVALID);

    size_t str_size = strlen (str_utf8);
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
    value->flags = PCVARIANT_FLAG_STRING_STATIC;
    value->refc = 1;
    value->sz_ptr[0] = str_size;
    value->sz_ptr[1] = (uintptr_t)str_utf8;

    return value;
}

const char* purc_variant_get_string_const (purc_variant_t string)
{
    PCVARIANT_CHECK_FAIL_RET(string, NULL);

    const char * str_str = NULL;

    if (purc_variant_is_type (string, PURC_VARIANT_TYPE_STRING)) {
        if ((string->flags & PCVARIANT_FLAG_EXTRA_SIZE) ||
                (string->flags & PCVARIANT_FLAG_STRING_STATIC))
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
        if ((string->flags & PCVARIANT_FLAG_EXTRA_SIZE) ||
                (string->flags & PCVARIANT_FLAG_STRING_STATIC))
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
            // VWNOTE: sz_ptr[0] will be set in pcvariant_stat_set_extra_size
            pcvariant_stat_set_extra_size (string, 0);
            free ((void *)string->sz_ptr[1]);
        }
    }
    else
        pcinst_set_error (PCVARIANT_INVALID_TYPE);
}

purc_variant_t
purc_variant_make_atom_string (const char* str_utf8, bool check_encoding)
{
    PCVARIANT_CHECK_FAIL_RET(str_utf8, PURC_VARIANT_INVALID);

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

    value = pcvariant_get (PURC_VARIANT_TYPE_ATOMSTRING);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    /* VWNOTE: for atomstring, only store the atom value */
    value->type = PURC_VARIANT_TYPE_ATOMSTRING;
    value->size = 0;
    value->flags = 0;
    value->refc = 1;
    value->sz_ptr[1] = atom;

    return value;
}

purc_variant_t
purc_variant_make_atom_string_static (const char* str_utf8,
        bool check_encoding)
{
    PCVARIANT_CHECK_FAIL_RET(str_utf8, PURC_VARIANT_INVALID);

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

    value = pcvariant_get (PURC_VARIANT_TYPE_ATOMSTRING);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    /* VWNOTE: for atomstring, only store the atom value */
    value->type = PURC_VARIANT_TYPE_ATOMSTRING;
    value->size = 0;
    value->flags = PCVARIANT_FLAG_STRING_STATIC;
    value->refc = 1;
    value->sz_ptr[1] = atom;

    return value;
}

const char* purc_variant_get_atom_string_const (purc_variant_t atom_string)
{
    PCVARIANT_CHECK_FAIL_RET(atom_string, NULL);

    const char * str_str = NULL;

    if (purc_variant_is_type (atom_string, PURC_VARIANT_TYPE_ATOMSTRING))
        str_str = purc_atom_to_string(atom_string->sz_ptr[1]);
    else
        pcinst_set_error (PCVARIANT_INVALID_TYPE);

    return str_str;
}

purc_variant_t purc_variant_make_byte_sequence (const void* bytes,
        size_t nr_bytes)
{
    // VWNOTE: check nr_bytes is not zero.
    PCVARIANT_CHECK_FAIL_RET((bytes != NULL && nr_bytes > 0),
        PURC_VARIANT_INVALID);

    size_t real_size = MAX (sizeof(long double), sizeof(void*) * 2);
    purc_variant_t value = pcvariant_get (PURC_VARIANT_TYPE_BSEQUENCE);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_BSEQUENCE;
    value->flags = 0;
    value->refc = 1;

    if (nr_bytes <= real_size) {
        value->size = nr_bytes;
        memcpy (value->bytes, bytes, nr_bytes);
    }
    else {
        value->flags = PCVARIANT_FLAG_EXTRA_SIZE;
        value->sz_ptr[1] = (uintptr_t) malloc (nr_bytes);
        if (value->sz_ptr[1] == 0) {
            pcvariant_put (value);
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PURC_VARIANT_INVALID;
        }

        // VWNOTE: sz_ptr[0] will be set in pcvariant_stat_set_extra_size
        // value->sz_ptr[0] = nr_bytes;
        memcpy ((void *)value->sz_ptr[1], bytes, nr_bytes);
        pcvariant_stat_set_extra_size (value, nr_bytes);
    }

    return value;
}

purc_variant_t purc_variant_make_byte_sequence_static (const void* bytes,
        size_t nr_bytes)
{
    // VWNOTE: check nr_bytes is not zero.
    PCVARIANT_CHECK_FAIL_RET((bytes != NULL && nr_bytes > 0),
        PURC_VARIANT_INVALID);

    purc_variant_t value = pcvariant_get (PURC_VARIANT_TYPE_BSEQUENCE);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_BSEQUENCE;
    value->flags = PCVARIANT_FLAG_STRING_STATIC;
    value->refc = 1;
    value->sz_ptr[0] = nr_bytes;
    value->sz_ptr[1] = (uintptr_t)bytes;

    return value;
}

purc_variant_t purc_variant_make_byte_sequence_reuse_buff (void* bytes,
                                        size_t nr_bytes, size_t sz_buff)
{
    // VWNOTE: check nr_bytes is not zero.
    PCVARIANT_CHECK_FAIL_RET((bytes != NULL && nr_bytes > 0),
        PURC_VARIANT_INVALID);

    purc_variant_t value = pcvariant_get (PURC_VARIANT_TYPE_BSEQUENCE);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_BSEQUENCE;
    value->flags = PCVARIANT_FLAG_EXTRA_SIZE;
    value->refc = 1;

    value->sz_ptr[1] = (uintptr_t) bytes;
    pcvariant_stat_set_extra_size (value, sz_buff);

    return value;
}


const unsigned char *
purc_variant_get_bytes_const (purc_variant_t sequence, size_t* nr_bytes)
{
    PCVARIANT_CHECK_FAIL_RET(sequence && nr_bytes, NULL);

    const unsigned char * bytes = NULL;

    if (purc_variant_is_type(sequence, PURC_VARIANT_TYPE_BSEQUENCE)) {
        if ((sequence->flags & PCVARIANT_FLAG_EXTRA_SIZE) ||
                    (sequence->flags & PCVARIANT_FLAG_STRING_STATIC)) {
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

    if (purc_variant_is_type (sequence, PURC_VARIANT_TYPE_BSEQUENCE)) {
        if ((sequence->flags & PCVARIANT_FLAG_EXTRA_SIZE) ||
                    (sequence->flags & PCVARIANT_FLAG_STRING_STATIC))
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

    if (purc_variant_is_type(sequence, PURC_VARIANT_TYPE_BSEQUENCE)) {
        if (sequence->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
            // VWNOTE: sz_ptr[0] will be set in pcvariant_stat_set_extra_size
            pcvariant_stat_set_extra_size (sequence, 0);
            free((void *)sequence->sz_ptr[1]);
        }
    }
    else
        pcinst_set_error (PCVARIANT_INVALID_TYPE);
}

purc_variant_t purc_variant_make_dynamic (purc_dvariant_method getter,
        purc_dvariant_method setter)
{
    // VWNOTE: check getter is not NULL.
    PCVARIANT_CHECK_FAIL_RET((getter != NULL), PURC_VARIANT_INVALID);

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
    value->ptr_ptr[0] = getter;
    value->ptr_ptr[1] = setter;

    return value;
}

purc_dvariant_method
purc_variant_dynamic_get_getter(const purc_variant_t dynamic)
{
    PC_ASSERT(dynamic);

    purc_dvariant_method fn = NULL;
    if (purc_variant_is_type(dynamic, PURC_VARIANT_TYPE_DYNAMIC)) {
        fn = dynamic->ptr_ptr[0];
    }
    else
        pcinst_set_error (PCVARIANT_INVALID_TYPE);

    return fn;
}

purc_dvariant_method
purc_variant_dynamic_get_setter(const purc_variant_t dynamic)
{
    PC_ASSERT(dynamic);

    purc_dvariant_method fn = NULL;
    if (purc_variant_is_type(dynamic, PURC_VARIANT_TYPE_DYNAMIC)) {
        fn = dynamic->ptr_ptr[1];
    }
    else
        pcinst_set_error (PCVARIANT_INVALID_TYPE);

    return fn;
}

purc_variant_t purc_variant_make_native (void *native_entity,
    const struct purc_native_ops *ops)
{
    // VWNOTE: check entity is not NULL.
    PCVARIANT_CHECK_FAIL_RET((native_entity != NULL), PURC_VARIANT_INVALID);

    purc_variant_t value = pcvariant_get (PURC_VARIANT_TYPE_NATIVE);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_NATIVE;
    value->size = 0;
    value->flags = 0;
    value->refc = 1;
    value->ptr_ptr[0] = native_entity;
    value->ptr_ptr[1] = (void*)ops; // FIXME: globally available ?

    return value;
}

void pcvariant_native_release(purc_variant_t value)
{
    if (value->type == PURC_VARIANT_TYPE_NATIVE) {
        struct purc_native_ops *ops;
        ops = (struct purc_native_ops*)value->ptr_ptr[1];
        if (ops && ops->eraser) {
            ops->eraser(value->ptr_ptr[0]);
        }
    }
}

void * purc_variant_native_get_entity(const purc_variant_t native)
{
    PC_ASSERT(native);

    void * ret = NULL;

    if (purc_variant_is_type(native, PURC_VARIANT_TYPE_NATIVE)) {
        ret = native->ptr_ptr[0];
    }
    else
        pcinst_set_error (PCVARIANT_INVALID_TYPE);

    return ret;
}
