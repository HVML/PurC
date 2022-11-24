/*
 * @file variant-basic.c
 * @author Geng Yue, Vincent Wei
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

#include "config.h"

#include "purc-utils.h"
#include "purc-variant.h"

#include "private/instance.h"
#include "private/errors.h"
#include "private/tls.h"
#include "private/variant.h"
#include "private/utf8.h"

#include "variant-internals.h"

#include <stdlib.h>
#include <string.h>

#ifndef MAX
#define MAX(a, b)   (a) > (b)? (a) : (b)
#endif

#define IS_TYPE(v, t)   (v->type == t)

// API for variant
purc_variant_t purc_variant_make_undefined (void)
{
    struct pcinst * instance = pcinst_current ();
    purc_variant_t value = &(instance->variant_heap->v_undefined);

    value->refc++;

    return value;
}

purc_variant_t purc_variant_make_null (void)
{
    struct pcinst * instance = pcinst_current ();
    purc_variant_t value = &(instance->variant_heap->v_null);

    value->refc++;

    return value;
}

purc_variant_t purc_variant_make_boolean (bool b)
{
    purc_variant_t value = NULL;
    struct pcinst * instance = pcinst_current ();

    if (b)
        value = &(instance->variant_heap->v_true);
    else
        value = &(instance->variant_heap->v_false);

    value->refc++;

    return value;
}

purc_variant_t purc_variant_make_exception(purc_atom_t except_atom)
{
    if (!purc_is_except_atom(except_atom)) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t value = pcvariant_get(PURC_VARIANT_TYPE_EXCEPTION);
    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_EXCEPTION;
    value->size = 0;
    value->flags = 0;
    value->refc = 1;
    value->atom = except_atom;
    value->extra_size = pcutils_string_utf8_chars(
            purc_atom_to_string(except_atom), -1);
    return value;
}

const char* purc_variant_get_exception_string_const (purc_variant_t v)
{
    PCVARIANT_CHECK_FAIL_RET(v, NULL);

    const char * str_str = NULL;

    if (IS_TYPE (v, PURC_VARIANT_TYPE_EXCEPTION))
        str_str = purc_atom_to_string(v->atom);
    else
        pcinst_set_error (PCVARIANT_ERROR_INVALID_TYPE);

    return str_str;
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

purc_variant_t
purc_variant_make_string(const char* str_utf8, bool check_encoding)
{
    PCVARIANT_CHECK_FAIL_RET(str_utf8, PURC_VARIANT_INVALID);
    return purc_variant_make_string_ex(str_utf8, strlen(str_utf8),
            check_encoding);
}

purc_variant_t
purc_variant_make_string_ex(const char* str_utf8, size_t len,
        bool check_encoding)
{
    PCVARIANT_CHECK_FAIL_RET(str_utf8, PURC_VARIANT_INVALID);

    static const size_t sz_bytes = MAX(sizeof(long double), sizeof(void*) * 2);
    purc_variant_t value = NULL;
    size_t nr_chars;

    if (check_encoding) {
        if (!pcutils_string_check_utf8_len(str_utf8, len, &nr_chars, NULL)) {
            pcinst_set_error(PURC_ERROR_BAD_ENCODING);
            return PURC_VARIANT_INVALID;
        }
    }
    else {
        const char *end;
        pcutils_string_check_utf8_len(str_utf8, len, &nr_chars, &end);
        len = end - str_utf8;
    }

    value = pcvariant_get (PURC_VARIANT_TYPE_STRING);
    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_STRING;
    value->flags = 0;
    value->refc = 1;
    value->extra_size = nr_chars;

    if (len < sz_bytes) {
        memcpy(value->bytes, str_utf8, len);
        value->bytes[len] = '\0';
        // VWNOTE: always store the size including the terminating null byte.
        value->size = len + 1;
    }
    else {
        char* new_buf;
        new_buf = malloc(len + 1);
        if(new_buf == NULL) {
            pcvariant_put (value);
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PURC_VARIANT_INVALID;
        }

        value->flags = PCVARIANT_FLAG_EXTRA_SIZE;
        // VWNOTE: sz_ptr[0] will be set in pcvariant_stat_set_extra_size
        value->sz_ptr[1] = (uintptr_t)new_buf;
        memcpy(new_buf, str_utf8, len);
        new_buf[len] = '\0';
        // VWNOTE: always store the size including the terminating null byte.
        pcvariant_stat_set_extra_size (value, len + 1);
    }

    return value;
}

purc_variant_t purc_variant_make_string_reuse_buff(char* str_utf8,
        size_t sz_buff, bool check_encoding)
{
    PCVARIANT_CHECK_FAIL_RET(str_utf8, PURC_VARIANT_INVALID);

    purc_variant_t value = NULL;
    size_t nr_chars;
    size_t len;
    const char *end;

    if (check_encoding) {
        if (!pcutils_string_check_utf8(str_utf8, -1, &nr_chars, &end)) {
            pcinst_set_error (PURC_ERROR_BAD_ENCODING);
            return PURC_VARIANT_INVALID;
        }
    }
    else {
        pcutils_string_check_utf8_len(str_utf8, sz_buff, &nr_chars, &end);
    }
    len = end - str_utf8;
    str_utf8[len] = '\0'; /* make sure the string is null-terminated */
    len++;

    if (len < sz_buff) {
        /* shrink the buffer to release not used space. */
        str_utf8 = realloc(str_utf8, len);
    }

    value = pcvariant_get(PURC_VARIANT_TYPE_STRING);

    if (value == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_STRING;
    value->flags = PCVARIANT_FLAG_EXTRA_SIZE;
    value->refc = 1;
    value->extra_size = nr_chars;

    value->sz_ptr[1] = (uintptr_t)(str_utf8);
    pcvariant_stat_set_extra_size(value, len);

    return value;
}


purc_variant_t purc_variant_make_string_static(const char* str_utf8,
        bool check_encoding)
{
    PCVARIANT_CHECK_FAIL_RET(str_utf8, PURC_VARIANT_INVALID);

    size_t nr_chars;
    purc_variant_t value = NULL;

    if (check_encoding) {
        if (!pcutils_string_check_utf8(str_utf8, -1, &nr_chars, NULL)) {
            pcinst_set_error (PURC_ERROR_BAD_ENCODING);
            return PURC_VARIANT_INVALID;
        }
    }
    else {
        nr_chars = pcutils_string_utf8_chars(str_utf8, -1);
    }

    value = pcvariant_get(PURC_VARIANT_TYPE_STRING);

    if (value == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_STRING;
    value->flags = PCVARIANT_FLAG_STRING_STATIC;
    value->refc = 1;
    value->extra_size = nr_chars;
    value->sz_ptr[0] = (uintptr_t)strlen(str_utf8) + 1;
    value->sz_ptr[1] = (uintptr_t)str_utf8;

    return value;
}

const char* purc_variant_get_string_const_ex(purc_variant_t string,
        size_t *str_len)
{
    size_t len = 0;

    PCVARIANT_CHECK_FAIL_RET(string, NULL);

    const char *str_str = NULL;

    if (IS_TYPE(string, PURC_VARIANT_TYPE_STRING)) {
        if ((string->flags & PCVARIANT_FLAG_EXTRA_SIZE) ||
                (string->flags & PCVARIANT_FLAG_STRING_STATIC)) {
            str_str = (const char *)string->sz_ptr[1];
            len = (size_t)string->sz_ptr[0] - 1;
        }
        else {
            str_str = (const char *)string->bytes;
            len = string->size - 1;
        }
    }
    else if (IS_TYPE(string, PURC_VARIANT_TYPE_ATOMSTRING) ||
            IS_TYPE(string, PURC_VARIANT_TYPE_EXCEPTION)) {
        str_str = purc_atom_to_string(string->atom);
        len = strlen(str_str);
    }
    else {
        pcinst_set_error(PCVARIANT_ERROR_INVALID_TYPE);
    }

    if (str_str && str_len) {
        *str_len = len;
    }

    return str_str;
}

bool purc_variant_string_bytes(purc_variant_t string, size_t *length)
{
    PC_ASSERT(string && length);

    if (IS_TYPE(string, PURC_VARIANT_TYPE_STRING)) {
        if ((string->flags & PCVARIANT_FLAG_EXTRA_SIZE) ||
                (string->flags & PCVARIANT_FLAG_STRING_STATIC))
            *length = (size_t)string->sz_ptr[0];
        else
            *length = string->size;

        return true;
    }

    pcinst_set_error (PCVARIANT_ERROR_INVALID_TYPE);
    return false;
}

// TODO
bool purc_variant_string_chars(purc_variant_t string, size_t *nr_chars)
{
    PC_ASSERT(string && nr_chars);

    if (IS_TYPE(string, PURC_VARIANT_TYPE_STRING) ||
        IS_TYPE(string, PURC_VARIANT_TYPE_ATOMSTRING) ||
        IS_TYPE(string, PURC_VARIANT_TYPE_EXCEPTION)) {

        *nr_chars = string->extra_size;
        return true;
    }

    pcinst_set_error (PCVARIANT_ERROR_INVALID_TYPE);
    return false;
}

void pcvariant_string_release (purc_variant_t string)
{
    PC_ASSERT(string);

    if (IS_TYPE (string, PURC_VARIANT_TYPE_STRING)) {
        if (string->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
            // VWNOTE: sz_ptr[0] will be set in pcvariant_stat_set_extra_size
            pcvariant_stat_set_extra_size (string, 0);
            free ((void *)string->sz_ptr[1]);
        }
    }
    else
        pcinst_set_error (PCVARIANT_ERROR_INVALID_TYPE);
}

purc_variant_t
purc_variant_make_atom(purc_atom_t atom)
{
    PCVARIANT_CHECK_FAIL_RET(atom, PURC_VARIANT_INVALID);

    purc_variant_t value = NULL;
    value = pcvariant_get(PURC_VARIANT_TYPE_ATOMSTRING);
    if (value == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    /* VWNOTE: for atomstring, only store the atom value */
    value->type = PURC_VARIANT_TYPE_ATOMSTRING;
    value->size = 0;
    value->flags = 0;
    value->refc = 1;
    value->atom = atom;

    return value;
}

purc_variant_t
purc_variant_make_atom_string (const char* str_utf8, bool check_encoding)
{
    PCVARIANT_CHECK_FAIL_RET(str_utf8, PURC_VARIANT_INVALID);

    purc_variant_t value = NULL;
    size_t nr_chars;

    if (check_encoding) {
        if (!pcutils_string_check_utf8(str_utf8, -1, &nr_chars, NULL)) {
            pcinst_set_error (PURC_ERROR_BAD_ENCODING);
            return PURC_VARIANT_INVALID;
        }
    }
    else {
        // XXX: the string must be enconded in UTF-8 correctly.
        nr_chars = pcutils_string_utf8_chars(str_utf8, -1);
    }

    purc_atom_t atom = purc_atom_from_string(str_utf8);
    if (atom == 0) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value = pcvariant_get(PURC_VARIANT_TYPE_ATOMSTRING);
    if (value == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    /* VWNOTE: for atomstring, only store the atom value */
    value->type = PURC_VARIANT_TYPE_ATOMSTRING;
    value->size = 0;
    value->flags = 0;
    value->refc = 1;
    value->atom = atom;
    value->extra_size = nr_chars;

    return value;
}

purc_variant_t
purc_variant_make_atom_string_static(const char* str_utf8,
        bool check_encoding)
{
    PCVARIANT_CHECK_FAIL_RET(str_utf8, PURC_VARIANT_INVALID);

    purc_variant_t value = NULL;
    size_t nr_chars;

    if (check_encoding) {
        if (!pcutils_string_check_utf8(str_utf8, -1, &nr_chars, NULL)) {
            pcinst_set_error (PURC_ERROR_BAD_ENCODING);
            return PURC_VARIANT_INVALID;
        }
    }
    else {
        // XXX: the string must be enconded in UTF-8 correctly.
        nr_chars = pcutils_string_utf8_chars(str_utf8, -1);
    }

    purc_atom_t atom = purc_atom_from_static_string(str_utf8);
    if (atom == 0) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    value = pcvariant_get(PURC_VARIANT_TYPE_ATOMSTRING);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    /* VWNOTE: for atomstring, only store the atom value */
    value->type = PURC_VARIANT_TYPE_ATOMSTRING;
    value->size = 0;
    value->flags = PCVARIANT_FLAG_STRING_STATIC;
    value->refc = 1;
    value->atom = atom;
    value->extra_size = nr_chars;

    return value;
}

const char* purc_variant_get_atom_string_const (purc_variant_t atom_string)
{
    PCVARIANT_CHECK_FAIL_RET(atom_string, NULL);

    const char * str_str = NULL;

    if (IS_TYPE (atom_string, PURC_VARIANT_TYPE_ATOMSTRING))
        str_str = purc_atom_to_string(atom_string->atom);
    else
        pcinst_set_error (PCVARIANT_ERROR_INVALID_TYPE);

    return str_str;
}

purc_variant_t purc_variant_make_byte_sequence(const void* bytes,
        size_t nr_bytes)
{
    // VWNOTE: check nr_bytes is not zero.
    PCVARIANT_CHECK_FAIL_RET((bytes != NULL && nr_bytes > 0),
        PURC_VARIANT_INVALID);

    static const size_t sz_bytes = MAX(sizeof(long double), sizeof(void*) * 2);
    purc_variant_t value = pcvariant_get(PURC_VARIANT_TYPE_BSEQUENCE);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_BSEQUENCE;
    value->flags = 0;
    value->refc = 1;

    if (nr_bytes <= sz_bytes) {
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

purc_variant_t purc_variant_make_byte_sequence_static(const void* bytes,
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

purc_variant_t purc_variant_make_byte_sequence_reuse_buff(void* bytes,
        size_t nr_bytes, size_t sz_buff)
{
    // VWNOTE: check nr_bytes is not zero.
    PCVARIANT_CHECK_FAIL_RET((bytes != NULL && nr_bytes > 0 &&
                nr_bytes <= sz_buff), PURC_VARIANT_INVALID);

    purc_variant_t value = pcvariant_get (PURC_VARIANT_TYPE_BSEQUENCE);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_BSEQUENCE;
    value->flags = PCVARIANT_FLAG_EXTRA_SIZE;
    value->refc = 1;

    if (nr_bytes < sz_buff) {
        /* shrink the buffer to release not used space. */
        bytes = realloc(bytes, nr_bytes);
    }
    value->sz_ptr[1] = (uintptr_t) bytes;
    pcvariant_stat_set_extra_size (value, nr_bytes);

    return value;
}

purc_variant_t purc_variant_make_byte_sequence_empty(void)
{
    static const size_t sz_bytes = MAX(sizeof(long double), sizeof(void*) * 2);
    purc_variant_t value = pcvariant_get (PURC_VARIANT_TYPE_BSEQUENCE);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_BSEQUENCE;
    value->flags = 0;
    value->refc = 1;
    value->size = 0;
    memset(value->bytes, 0, sz_bytes);

    return value;
}

const unsigned char *purc_variant_get_bytes_const(purc_variant_t sequence,
        size_t* nr_bytes)
{
    PCVARIANT_CHECK_FAIL_RET(sequence && nr_bytes, NULL);

    const unsigned char * bytes = NULL;

    if (IS_TYPE(sequence, PURC_VARIANT_TYPE_BSEQUENCE)) {
        if ((sequence->flags & PCVARIANT_FLAG_EXTRA_SIZE) ||
                    (sequence->flags & PCVARIANT_FLAG_STRING_STATIC)) {
            bytes = (const unsigned char *)sequence->sz_ptr[1];
            *nr_bytes = (size_t)sequence->sz_ptr[0];
        }
        else {
            bytes = sequence->bytes;
            *nr_bytes = sequence->size;
        }
    }
    else if (IS_TYPE(sequence, PURC_VARIANT_TYPE_STRING)) {
        if ((sequence->flags & PCVARIANT_FLAG_EXTRA_SIZE) ||
                (sequence->flags & PCVARIANT_FLAG_STRING_STATIC)) {
            bytes = (const unsigned char *)sequence->sz_ptr[1];
            *nr_bytes = (size_t)sequence->sz_ptr[0];
        }
        else {
            bytes = (const unsigned char *)sequence->bytes;
            *nr_bytes = sequence->size;
        }
    }
    else
        pcinst_set_error (PCVARIANT_ERROR_INVALID_TYPE);

    return bytes;
}

bool purc_variant_bsequence_bytes(purc_variant_t sequence, size_t *length)
{
    PC_ASSERT(sequence && length);

    if (IS_TYPE (sequence, PURC_VARIANT_TYPE_BSEQUENCE)) {
        if ((sequence->flags & PCVARIANT_FLAG_EXTRA_SIZE) ||
                    (sequence->flags & PCVARIANT_FLAG_STRING_STATIC))
            *length = (size_t)sequence->sz_ptr[0];
        else
            *length = sequence->size;

        return true;
    }

    pcinst_set_error (PCVARIANT_ERROR_INVALID_TYPE);
    return false;
}

void pcvariant_sequence_release(purc_variant_t sequence)
{
    PC_ASSERT(sequence);

    if (IS_TYPE(sequence, PURC_VARIANT_TYPE_BSEQUENCE)) {
        if (sequence->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
            // VWNOTE: sz_ptr[0] will be set in pcvariant_stat_set_extra_size
            pcvariant_stat_set_extra_size (sequence, 0);
            free((void *)sequence->sz_ptr[1]);
        }
    }
    else
        pcinst_set_error (PCVARIANT_ERROR_INVALID_TYPE);
}

purc_variant_t purc_variant_make_dynamic(purc_dvariant_method getter,
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

purc_dvariant_method purc_variant_dynamic_get_getter(purc_variant_t dynamic)
{
    PC_ASSERT(dynamic);

    purc_dvariant_method fn = NULL;
    if (IS_TYPE(dynamic, PURC_VARIANT_TYPE_DYNAMIC)) {
        fn = dynamic->ptr_ptr[0];
    }
    else
        pcinst_set_error (PCVARIANT_ERROR_INVALID_TYPE);

    return fn;
}

purc_dvariant_method purc_variant_dynamic_get_setter(purc_variant_t dynamic)
{
    PC_ASSERT(dynamic);

    purc_dvariant_method fn = NULL;
    if (IS_TYPE(dynamic, PURC_VARIANT_TYPE_DYNAMIC)) {
        fn = dynamic->ptr_ptr[1];
    }
    else
        pcinst_set_error (PCVARIANT_ERROR_INVALID_TYPE);

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
        if (ops && ops->on_release) {
            ops->on_release(value->ptr_ptr[0]);
        }
    }
}

void *purc_variant_native_get_entity(purc_variant_t native)
{
    PC_ASSERT(native);

    void * ret = NULL;

    if (IS_TYPE(native, PURC_VARIANT_TYPE_NATIVE)) {
        ret = native->ptr_ptr[0];
    }
    else
        pcinst_set_error (PCVARIANT_ERROR_INVALID_TYPE);

    return ret;
}

struct purc_native_ops *purc_variant_native_get_ops(purc_variant_t native)
{
    PC_ASSERT(native);

    struct purc_native_ops *ret = NULL;

    if (IS_TYPE(native, PURC_VARIANT_TYPE_NATIVE)) {
        ret = native->ptr_ptr[1];
    }
    else
        pcinst_set_error (PCVARIANT_ERROR_INVALID_TYPE);

    return ret;
}

