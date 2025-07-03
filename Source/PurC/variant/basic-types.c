/*
 * @file basic-types.c
 * @author Geng Yue, Vincent Wei
 * @date 2021/07/02
 * @brief The implementation of variant.
 *
 * Copyright (C) 2021 ~ 2025 FMSoft <https://www.fmsoft.cn>
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
    purc_variant_t value =
        (purc_variant_t)&(instance->variant_heap->v_undefined);

    value->refc++;

    return value;
}

purc_variant_t purc_variant_make_null (void)
{
    struct pcinst * instance = pcinst_current ();
    purc_variant_t value =
        (purc_variant_t)&(instance->variant_heap->v_null);

    value->refc++;

    return value;
}

purc_variant_t purc_variant_make_boolean (bool b)
{
    purc_variant_t value = NULL;
    struct pcinst * instance = pcinst_current ();

    if (b)
        value = (purc_variant_t)&(instance->variant_heap->v_true);
    else
        value = (purc_variant_t)&(instance->variant_heap->v_false);

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
    return value;
}

const char* purc_variant_get_exception_string_const(purc_variant_t v)
{
    PCVRNT_CHECK_FAIL_RET(v, NULL);

    const char * str_str = NULL;

    if (IS_TYPE (v, PURC_VARIANT_TYPE_EXCEPTION))
        str_str = purc_atom_to_string(v->atom);
    else
        pcinst_set_error(PCVRNT_ERROR_INVALID_TYPE);

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
    PCVRNT_CHECK_FAIL_RET(str_utf8, PURC_VARIANT_INVALID);
    return purc_variant_make_string_ex(str_utf8, strlen(str_utf8),
            check_encoding);
}

purc_variant_t
purc_variant_make_string_ex(const char* str_utf8, size_t len,
        bool check_encoding)
{
    PCVRNT_CHECK_FAIL_RET(str_utf8, PURC_VARIANT_INVALID);

    static const size_t sz_in_space = NR_BYTES_IN_WRAPPER;
    purc_variant_t value = NULL;

    if (check_encoding) {
        if (!pcutils_string_check_utf8_len(str_utf8, len, NULL, NULL)) {
            pcinst_set_error(PURC_ERROR_BAD_ENCODING);
            return PURC_VARIANT_INVALID;
        }
    }
    else {
        const char *end;
        pcutils_string_check_utf8_len(str_utf8, len, NULL, &end);
        len = end - str_utf8;
    }

    value = pcvariant_get(PURC_VARIANT_TYPE_STRING);
    if (value == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_STRING;
    value->flags = 0;
    value->refc = 1;

    if ((len + 1) < sz_in_space) {
        memcpy(value->bytes, str_utf8, len);
        value->bytes[len] = '\0';
        value->size = len + 1;
    }
    else {
        char* new_buf;
        new_buf = malloc(len + 1);
        if (new_buf == NULL) {
            pcvariant_put(value);
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PURC_VARIANT_INVALID;
        }

        value->flags = PCVRNT_FLAG_EXTRA_SIZE;
        value->len = len + 1;
        value->ptr2 = new_buf;
        memcpy(new_buf, str_utf8, len);
        new_buf[len] = '\0';
        pcvariant_stat_set_extra_size(value, len + 1);
    }

    return value;
}

purc_variant_t purc_variant_make_string_reuse_buff(char* str_utf8,
        size_t sz_buff, bool check_encoding)
{
    PCVRNT_CHECK_FAIL_RET(str_utf8, PURC_VARIANT_INVALID);

    purc_variant_t value = NULL;
    size_t len;
    const char *end;

    if (check_encoding) {
        if (!pcutils_string_check_utf8(str_utf8, -1, NULL, &end)) {
            pcinst_set_error (PURC_ERROR_BAD_ENCODING);
            return PURC_VARIANT_INVALID;
        }
    }
    else {
        pcutils_string_check_utf8_len(str_utf8, sz_buff, NULL, &end);
    }
    len = end - str_utf8;
    str_utf8[len] = '\0'; /* make sure the string is null-terminated */

    if (sz_buff <= len) {
        PC_WARN("%s() called with a bad buffer size.\n", __func__);
        sz_buff = len + 1;
    }
    else if (sz_buff > len + 1 && sz_buff > 32) {
        /* shrink the buffer to release not used space. */
        sz_buff = len + 1;
        str_utf8 = realloc(str_utf8, sz_buff);
    }

    value = pcvariant_get(PURC_VARIANT_TYPE_STRING);

    if (value == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_STRING;
    value->flags = PCVRNT_FLAG_EXTRA_SIZE;
    value->refc = 1;

    value->len = len + 1;
    value->ptr2 = str_utf8;
    pcvariant_stat_set_extra_size(value, sz_buff);

    return value;
}


purc_variant_t purc_variant_make_string_static(const char* str_utf8,
        bool check_encoding)
{
    PCVRNT_CHECK_FAIL_RET(str_utf8, PURC_VARIANT_INVALID);

    purc_variant_t value = NULL;

    if (check_encoding) {
        if (!pcutils_string_check_utf8(str_utf8, -1, NULL, NULL)) {
            pcinst_set_error (PURC_ERROR_BAD_ENCODING);
            return PURC_VARIANT_INVALID;
        }
    }

    value = pcvariant_get(PURC_VARIANT_TYPE_STRING);

    if (value == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_STRING;
    value->flags = PCVRNT_FLAG_STATIC_DATA;
    value->refc = 1;
    value->len = strlen(str_utf8) + 1;
    value->ptr2 = (void *)str_utf8;

    return value;
}

const char* purc_variant_get_string_const_ex(purc_variant_t string,
        size_t *str_len)
{
    size_t len = 0;

    PCVRNT_CHECK_FAIL_RET(string, NULL);

    const char *str_str = NULL;

    if (IS_TYPE(string, PURC_VARIANT_TYPE_STRING)) {
        if ((string->flags & PCVRNT_FLAG_EXTRA_SIZE) ||
                (string->flags & PCVRNT_FLAG_STATIC_DATA)) {
            str_str = (const char *)string->ptr2;
            len = (size_t)string->len;
        }
        else {
            str_str = (const char *)string->bytes;
            len = string->size;
        }
    }
    else if (IS_TYPE(string, PURC_VARIANT_TYPE_ATOMSTRING) ||
            IS_TYPE(string, PURC_VARIANT_TYPE_EXCEPTION)) {
        str_str = purc_atom_to_string(string->atom);
        len = strlen(str_str) + 1;
    }
    else {
        pcinst_set_error(PCVRNT_ERROR_INVALID_TYPE);
    }

    if (str_str && str_len) {
        *str_len = len - 1;
    }

    return str_str;
}

bool purc_variant_string_bytes(purc_variant_t string, size_t *length)
{
    PC_ASSERT(string && length);

    if (IS_TYPE(string, PURC_VARIANT_TYPE_STRING)) {
        if ((string->flags & PCVRNT_FLAG_EXTRA_SIZE) ||
                (string->flags & PCVRNT_FLAG_STATIC_DATA))
            *length = (size_t)string->len;
        else
            *length = string->size;

        return true;
    }

    pcinst_set_error(PCVRNT_ERROR_INVALID_TYPE);
    return false;
}

bool purc_variant_string_chars(purc_variant_t string, size_t *nr_chars)
{
    PC_ASSERT(string && nr_chars);

    if (IS_TYPE(string, PURC_VARIANT_TYPE_STRING)) {
        const char *str_str;
        if ((string->flags & PCVRNT_FLAG_EXTRA_SIZE) ||
                (string->flags & PCVRNT_FLAG_STATIC_DATA)) {
            str_str = (const char *)string->ptr2;
        }
        else {
            str_str = (const char *)string->bytes;
        }

        *nr_chars = pcutils_string_utf8_chars(str_str, -1);
    }
    else if (IS_TYPE(string, PURC_VARIANT_TYPE_ATOMSTRING) ||
            IS_TYPE(string, PURC_VARIANT_TYPE_EXCEPTION)) {
        *nr_chars = pcutils_string_utf8_chars(
                purc_atom_to_string(string->atom), -1);
    }
    else {
        pcinst_set_error(PCVRNT_ERROR_INVALID_TYPE);
        return false;
    }

    return true;
}

void pcvariant_string_release(purc_variant_t string)
{
    PC_ASSERT(string);

    if (IS_TYPE(string, PURC_VARIANT_TYPE_STRING)) {
        if (string->flags & PCVRNT_FLAG_EXTRA_SIZE) {
            free(string->ptr2);
            pcvariant_stat_set_extra_size(string, 0);
        }
    }
    else
        pcinst_set_error(PCVRNT_ERROR_INVALID_TYPE);
}

purc_variant_t
purc_variant_make_atom(purc_atom_t atom)
{
    PCVRNT_CHECK_FAIL_RET(atom, PURC_VARIANT_INVALID);

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
    PCVRNT_CHECK_FAIL_RET(str_utf8, PURC_VARIANT_INVALID);

    purc_variant_t value = NULL;

    if (check_encoding) {
        if (!pcutils_string_check_utf8(str_utf8, -1, NULL, NULL)) {
            pcinst_set_error (PURC_ERROR_BAD_ENCODING);
            return PURC_VARIANT_INVALID;
        }
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

    return value;
}

purc_variant_t
purc_variant_make_atom_string_static(const char* str_utf8,
        bool check_encoding)
{
    PCVRNT_CHECK_FAIL_RET(str_utf8, PURC_VARIANT_INVALID);

    purc_variant_t value = NULL;

    if (check_encoding) {
        if (!pcutils_string_check_utf8(str_utf8, -1, NULL, NULL)) {
            pcinst_set_error (PURC_ERROR_BAD_ENCODING);
            return PURC_VARIANT_INVALID;
        }
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
    value->flags = PCVRNT_FLAG_STATIC_DATA;
    value->refc = 1;
    value->atom = atom;

    return value;
}

const char* purc_variant_get_atom_string_const (purc_variant_t atom_string)
{
    PCVRNT_CHECK_FAIL_RET(atom_string, NULL);

    const char * str_str = NULL;

    if (IS_TYPE (atom_string, PURC_VARIANT_TYPE_ATOMSTRING))
        str_str = purc_atom_to_string(atom_string->atom);
    else
        pcinst_set_error (PCVRNT_ERROR_INVALID_TYPE);

    return str_str;
}

purc_variant_t purc_variant_make_byte_sequence(const void* bytes,
        size_t nr_bytes)
{
    // VWNOTE: check nr_bytes is not zero.
    PCVRNT_CHECK_FAIL_RET((bytes != NULL && nr_bytes > 0),
        PURC_VARIANT_INVALID);

    static const size_t sz_in_space = NR_BYTES_IN_WRAPPER;
    purc_variant_t value = pcvariant_get(PURC_VARIANT_TYPE_BSEQUENCE);

    if (value == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_BSEQUENCE;
    value->flags = 0;
    value->refc = 1;

    if (nr_bytes <= sz_in_space) {
        value->size = nr_bytes;
        memcpy(value->bytes, bytes, nr_bytes);
    }
    else {
        value->flags = PCVRNT_FLAG_EXTRA_SIZE;
        value->ptr2 = malloc(nr_bytes);
        if (value->ptr2 == NULL) {
            pcvariant_put(value);
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return PURC_VARIANT_INVALID;
        }

        value->len = nr_bytes;
        memcpy(value->ptr2, bytes, nr_bytes);
        pcvariant_stat_set_extra_size(value, nr_bytes);
    }

    return value;
}

purc_variant_t purc_variant_make_byte_sequence_static(const void* bytes,
        size_t nr_bytes)
{
    // VWNOTE: check nr_bytes is not zero.
    PCVRNT_CHECK_FAIL_RET((bytes != NULL && nr_bytes > 0),
        PURC_VARIANT_INVALID);

    purc_variant_t value = pcvariant_get(PURC_VARIANT_TYPE_BSEQUENCE);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_BSEQUENCE;
    value->flags = PCVRNT_FLAG_STATIC_DATA;
    value->refc = 1;
    value->len = nr_bytes;
    value->ptr2 = (void *)bytes;

    return value;
}

purc_variant_t purc_variant_make_byte_sequence_reuse_buff(void* bytes,
        size_t nr_bytes, size_t sz_buff)
{
    // VWNOTE: check nr_bytes is not zero.
    PCVRNT_CHECK_FAIL_RET((bytes != NULL && nr_bytes > 0 &&
                nr_bytes <= sz_buff), PURC_VARIANT_INVALID);

    purc_variant_t value = pcvariant_get(PURC_VARIANT_TYPE_BSEQUENCE);

    if (value == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_BSEQUENCE;
    value->flags = PCVRNT_FLAG_EXTRA_SIZE;
    value->refc = 1;

#if 0
    /* we keep the buffer unchanged since 0.9.22. */
    if (nr_bytes < sz_buff) {
        /* shrink the buffer to release not used space. */
        bytes = realloc(bytes, nr_bytes);
    }
#endif

    value->len = nr_bytes;
    value->ptr2 = bytes;
    pcvariant_stat_set_extra_size(value, sz_buff);

    return value;
}

purc_variant_t purc_variant_make_byte_sequence_empty(void)
{
    static const size_t sz_in_space = NR_BYTES_IN_WRAPPER;
    purc_variant_t value = pcvariant_get(PURC_VARIANT_TYPE_BSEQUENCE);

    if (value == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_BSEQUENCE;
    value->flags = 0;
    value->refc = 1;
    value->size = 0;
    memset(value->bytes, 0, sz_in_space);

    return value;
}

/* Since 0.9.22 */
purc_variant_t purc_variant_make_byte_sequence_empty_ex(size_t sz_buf)
{
    static const size_t sz_in_space = NR_BYTES_IN_WRAPPER;
    purc_variant_t value = pcvariant_get(PURC_VARIANT_TYPE_BSEQUENCE);

    if (value == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_BSEQUENCE;
    value->refc = 1;

    if (sz_buf > sz_in_space) {
        void *buf = calloc(1, sz_buf);
        if (buf == NULL) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return PURC_VARIANT_INVALID;
        }

        value->flags = PCVRNT_FLAG_EXTRA_SIZE;
        value->len = 0;
        value->ptr2 = buf;
        pcvariant_stat_set_extra_size(value, sz_buf);
    }
    else {
        value->flags = 0;
        value->size = 0;
        memset(value->bytes, 0, sz_in_space);
    }

    return value;
}

/* Since 0.9.22 */
unsigned char *
purc_variant_bsequence_buffer(purc_variant_t sequence, size_t *nr_bytes,
        size_t *sz_buf)
{
    unsigned char *bytes = NULL;

    if (IS_TYPE(sequence, PURC_VARIANT_TYPE_BSEQUENCE)) {
        if (sequence->flags & PCVRNT_FLAG_STATIC_DATA) {
            goto done;
        }

        if (sequence->flags & PCVRNT_FLAG_EXTRA_SIZE) {
            bytes = sequence->ptr2;
            *nr_bytes = sequence->len;
            *sz_buf = sequence->extra_size;
        }
        else {
            bytes = sequence->bytes;
            *nr_bytes = sequence->size;
            *sz_buf = NR_BYTES_IN_WRAPPER;
        }

    }

done:
    return bytes;
}

bool
purc_variant_bsequence_set_bytes(purc_variant_t sequence, size_t nr_bytes)
{
    bool retv = false;

    if (IS_TYPE(sequence, PURC_VARIANT_TYPE_BSEQUENCE)) {
        if (sequence->flags & PCVRNT_FLAG_STATIC_DATA) {
            goto done;
        }

        if (sequence->flags & PCVRNT_FLAG_EXTRA_SIZE) {
            if (nr_bytes <= sequence->extra_size) {
                sequence->len = (uintptr_t)nr_bytes;
                retv = true;
            }
        }
        else {
            if (nr_bytes <= NR_BYTES_IN_WRAPPER) {
                sequence->size = nr_bytes;
                retv = true;
            }
        }
    }

done:
    return retv;
}

/* Since 0.9.22 */
bool purc_variant_bsequence_append(purc_variant_t sequence,
        const unsigned char *bytes, size_t nr_bytes)
{
    unsigned char *buf = NULL;
    size_t curr_bytes, sz_buf;

    PCVRNT_CHECK_FAIL_RET(sequence && nr_bytes, false);

    if (IS_TYPE(sequence, PURC_VARIANT_TYPE_BSEQUENCE)) {
        if (sequence->flags & PCVRNT_FLAG_STATIC_DATA) {
            PC_ERROR("Attempt to append data to a static bsequence.\n");
            pcinst_set_error(PURC_ERROR_ACCESS_DENIED);
            goto error;
        }

        if (sequence->flags & PCVRNT_FLAG_EXTRA_SIZE) {
            buf = sequence->ptr2;
            curr_bytes = sequence->len;
            sz_buf = sequence->extra_size;
        }
        else {
            buf = sequence->bytes;
            curr_bytes = sequence->size;
            sz_buf = NR_BYTES_IN_WRAPPER;
        }
    }
    else {
        goto error;
    }

    if (sz_buf < nr_bytes + curr_bytes) {
        goto error;
    }

    memcpy(buf + curr_bytes, bytes, nr_bytes);
    curr_bytes += nr_bytes;

    if (sequence->flags & PCVRNT_FLAG_EXTRA_SIZE) {
        sequence->len = curr_bytes;
    }
    else {
        sequence->size = curr_bytes;
    }

    /* TODO: trigger an event for the change of the byte sequence. */
    return true;

error:
    return false;
}

/* Since 0.9.22 */
ssize_t purc_variant_bsequence_roll(purc_variant_t sequence, ssize_t offset)
{
    unsigned char *buf = NULL;
    size_t curr_bytes, sz_buf;
    ssize_t nr_copied = 0;

    if (IS_TYPE(sequence, PURC_VARIANT_TYPE_BSEQUENCE)) {
        if (sequence->flags & PCVRNT_FLAG_STATIC_DATA) {
            PC_ERROR("Attempt to change a static bsequence.\n");
            pcinst_set_error(PURC_ERROR_ACCESS_DENIED);
            goto error;
        }

        if (sequence->flags & PCVRNT_FLAG_EXTRA_SIZE) {
            buf = sequence->ptr2;
            curr_bytes = sequence->len;
            sz_buf = sequence->extra_size;
        }
        else {
            buf = sequence->bytes;
            curr_bytes = sequence->size;
            sz_buf = NR_BYTES_IN_WRAPPER;
        }
    }
    else {
        pcinst_set_error(PURC_ERROR_NOT_DESIRED_ENTITY);
        goto error;
    }

    if (offset < 0) {
        if (sequence->flags & PCVRNT_FLAG_EXTRA_SIZE) {
            sequence->len = 0;
        }
        else {
            sequence->size = 0;
        }
        memset(buf, 0, sz_buf);
    }
    else if ((size_t)offset >= curr_bytes) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }
    else {
        nr_copied = curr_bytes - offset;
        if (offset != 0) {
            memmove(buf, buf + offset, nr_copied);

            if (sequence->flags & PCVRNT_FLAG_EXTRA_SIZE) {
                sequence->len = nr_copied;
            }
            else {
                sequence->size = nr_copied;
            }
        }
        memset(buf + nr_copied, 0, sz_buf - nr_copied);
    }

    /* TODO: trigger an event for the change of the byte sequence. */
    return nr_copied;

error:
    return -1;
}

const unsigned char *purc_variant_get_bytes_const(purc_variant_t sequence,
        size_t* nr_bytes)
{
    PCVRNT_CHECK_FAIL_RET(sequence && nr_bytes, NULL);

    const unsigned char * bytes = NULL;

    if (IS_TYPE(sequence, PURC_VARIANT_TYPE_BSEQUENCE)) {
        if ((sequence->flags & PCVRNT_FLAG_EXTRA_SIZE) ||
                    (sequence->flags & PCVRNT_FLAG_STATIC_DATA)) {
            bytes = sequence->ptr2;
            *nr_bytes = sequence->len;
        }
        else {
            bytes = sequence->bytes;
            *nr_bytes = sequence->size;
        }
    }
    else if (IS_TYPE(sequence, PURC_VARIANT_TYPE_STRING)) {
        if ((sequence->flags & PCVRNT_FLAG_EXTRA_SIZE) ||
                (sequence->flags & PCVRNT_FLAG_STATIC_DATA)) {
            bytes = sequence->ptr2;
            *nr_bytes = sequence->len;
        }
        else {
            bytes = sequence->bytes;
            *nr_bytes = sequence->size;
        }
    }
    else
        pcinst_set_error(PCVRNT_ERROR_INVALID_TYPE);

    return bytes;
}

bool purc_variant_bsequence_bytes(purc_variant_t sequence, size_t *length)
{
    PC_ASSERT(sequence && length);

    if (IS_TYPE (sequence, PURC_VARIANT_TYPE_BSEQUENCE)) {
        if ((sequence->flags & PCVRNT_FLAG_EXTRA_SIZE) ||
                    (sequence->flags & PCVRNT_FLAG_STATIC_DATA))
            *length = sequence->len;
        else
            *length = sequence->size;

        return true;
    }

    pcinst_set_error (PCVRNT_ERROR_INVALID_TYPE);
    return false;
}

void pcvariant_sequence_release(purc_variant_t sequence)
{
    PC_ASSERT(sequence);

    if (IS_TYPE(sequence, PURC_VARIANT_TYPE_BSEQUENCE)) {
        if (sequence->flags & PCVRNT_FLAG_EXTRA_SIZE) {
            free(sequence->ptr2);
            pcvariant_stat_set_extra_size(sequence, 0);
        }
    }
    else
        pcinst_set_error(PCVRNT_ERROR_INVALID_TYPE);
}

purc_variant_t purc_variant_make_dynamic(purc_dvariant_method getter,
        purc_dvariant_method setter)
{
    // VWNOTE: check getter is not NULL.
    PCVRNT_CHECK_FAIL_RET((getter != NULL), PURC_VARIANT_INVALID);

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
    value->ptr = getter;
    value->ptr2 = setter;

    return value;
}

purc_dvariant_method purc_variant_dynamic_get_getter(purc_variant_t dynamic)
{
    PC_ASSERT(dynamic);

    purc_dvariant_method fn = NULL;
    if (IS_TYPE(dynamic, PURC_VARIANT_TYPE_DYNAMIC)) {
        fn = dynamic->ptr;
    }
    else
        pcinst_set_error (PCVRNT_ERROR_INVALID_TYPE);

    return fn;
}

purc_dvariant_method purc_variant_dynamic_get_setter(purc_variant_t dynamic)
{
    PC_ASSERT(dynamic);

    purc_dvariant_method fn = NULL;
    if (IS_TYPE(dynamic, PURC_VARIANT_TYPE_DYNAMIC)) {
        fn = dynamic->ptr2;
    }
    else
        pcinst_set_error (PCVRNT_ERROR_INVALID_TYPE);

    return fn;
}

purc_variant_t purc_variant_make_native_entity(void *native_entity,
    struct purc_native_ops *ops, const char *name)
{
    // VWNOTE: check entity is not NULL.
    PCVRNT_CHECK_FAIL_RET((native_entity != NULL), PURC_VARIANT_INVALID);

    purc_variant_t value = pcvariant_get (PURC_VARIANT_TYPE_NATIVE);

    if (value == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    value->type = PURC_VARIANT_TYPE_NATIVE;
    value->size = 0;
    value->flags = 0;
    value->refc = 1;
    value->ptr = native_entity;
    value->ptr2 = ops;
    value->extra_data = name ? (void *)name : (void *)"anonymous";

    return value;
}

void pcvariant_native_release(purc_variant_t value)
{
    if (value->type == PURC_VARIANT_TYPE_NATIVE) {
        struct purc_native_ops *ops;
        ops = value->ptr2;
        if (ops && ops->on_release) {
            ops->on_release(value->ptr);
        }
    }
}

void *purc_variant_native_get_entity(purc_variant_t native)
{
    PC_ASSERT(native);

    void * ret = NULL;

    if (IS_TYPE(native, PURC_VARIANT_TYPE_NATIVE)) {
        ret = native->ptr;
    }
    else
        pcinst_set_error(PCVRNT_ERROR_INVALID_TYPE);

    return ret;
}

struct purc_native_ops *purc_variant_native_get_ops(purc_variant_t native)
{
    PC_ASSERT(native);

    struct purc_native_ops *ret = NULL;

    if (IS_TYPE(native, PURC_VARIANT_TYPE_NATIVE)) {
        ret = native->ptr2;
    }
    else
        pcinst_set_error(PCVRNT_ERROR_INVALID_TYPE);

    return ret;
}

const char *purc_variant_native_get_name(purc_variant_t native)
{
    PC_ASSERT(native);

    const char *name = NULL;

    if (IS_TYPE(native, PURC_VARIANT_TYPE_NATIVE)) {
        name = native->extra_data;
    }
    else
        pcinst_set_error(PCVRNT_ERROR_INVALID_TYPE);

    return name;
}

struct purc_native_ops *
purc_variant_native_set_ops(purc_variant_t native, struct purc_native_ops *ops)
{
    PC_ASSERT(native);

    struct purc_native_ops *ret = NULL;

    if (IS_TYPE(native, PURC_VARIANT_TYPE_NATIVE)) {
        ret = native->ptr2;
        native->ptr2 = ops;
    }
    else
        pcinst_set_error(PCVRNT_ERROR_INVALID_TYPE);

    return ret;
}

