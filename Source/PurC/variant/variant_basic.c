/**
 * @file purc-variant.h
 * @author 
 * @date 2021/07/02
 * @brief The API for variant.
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


#include "purc_variant.h"
#include "variant_types.h"

static struct purc_variant pcvariant_undefined = { PCVARIANT_TYPE_UNDEFINED, 0, 0, PCVARIANT_FLAG_NOFREE };
static struct purc_variant pcvariant_null = { PCVARIANT_TYPE_NULL, 0, 0, PCVARIANT_FLAG_NOFREE };
static struct purc_variant pcvariant_false = { PCVARIANT_TYPE_BOOLEAN, 0, 0, PCVARIANT_FLAG_NOFREE, { b:0 } };
static struct purc_variant pcvariant_true = { PCVARIANT_TYPE_BOOLEAN, 0, 0, PCVARIANT_FLAG_NOFREE, { b:1 } };

purc_variant_t purc_variant_make_undefined (void)
{
    return &pcvariant_undefined;
}

purc_variant_t purc_variant_make_null (void)
{
    return &pcvariant_null;
}

purc_variant_t purc_variant_make_boolean (bool b)
{
    purc_variant_t purc_variant_bool = NULL;

    if(b)
        purc_variant_bool = &pcvariant_true;
    else
        purc_variant_bool = &pcvariant_false;

    return purc_variant_bool;
}

purc_variant_t purc_variant_make_number (double d)
{
    purc_variant_t purc_variant_number = (purc_variant_t)malloc(sizeof(purc_variant));

    if(purc_variant_number == NULL)
        return PURC_VARIANT_INVALID;

    purc_variant_number->type = VARIANT_TYPE_NUMBER;
    purc_variant_number->size = 0;
    purc_variant_number->flags = 0;
    purc_variant_number->refc = 1;
    purc_variant_number->d = d;

    return purc_variant_number;
}

purc_variant_t purc_variant_make_longuint (uint64_t u64)
{
    purc_variant_t purc_variant_longuint = (purc_variant_t)malloc(sizeof(purc_variant));

    if(purc_variant_longuint == NULL)
        return PURC_VARIANT_INVALID;

    purc_variant_longuint->type = VARIANT_TYPE_LONGINT;
    purc_variant_longuint->size = 0;
    purc_variant_longuint->flags = 0;
    purc_variant_longuint->refc = 1;
    purc_variant_longuint->u64 = u64;

    return purc_variant_longuint;
}

purc_variant_t purc_variant_make_longint (uint64_t u64)
{
    purc_variant_t purc_variant_longint = (purc_variant_t)malloc(sizeof(purc_variant));

    if(purc_variant_longint == NULL)
        return PURC_VARIANT_INVALID;

    purc_variant_longint->type = VARIANT_TYPE_LONGINT;
    purc_variant_longint->size = 0;
    purc_variant_longint->flags = 0;
    purc_variant_longint->refc = 1;
    purc_variant_longint->u64 = i64;

    return purc_variant_longint;
}

purc_variant_t purc_variant_make_longdouble (long double lf)
{
    purc_variant_t purc_variant_longdouble = (purc_variant_t)malloc(sizeof(purc_variant));

    if(purc_variant_longdouble == NULL)
        return PURC_VARIANT_INVALID;

    purc_variant_longdouble->type = VARIANT_TYPE_LONGDOUBLE;
    purc_variant_longdouble->size = 0;
    purc_variant_longdouble->flags = 0;
    purc_variant_longdouble->refc = 1;
    purc_variant_longdouble->ld = lf;

    return purc_variant_longdouble;
}

purc_variant_t purc_variant_make_string (const char* str_utf8)
{
    int str_size = strlen(str_utf8);
    int real_size = MAX(sizeof(long double), sizeof(void*) * 2);
    purc_variant_t purc_variant_string = NULL;
    
    if(str_size < real_size)
        real_size = 0;
    else
        real_size = str_size + 1;
    
    purc_variant_string = (purc_variant_t)malloc(sizeof(purc_variant) + real_size);

    if(purc_variant_string == NULL)
        return PURC_VARIANT_INVALID;

    purc_variant_string->type = VARIANT_TYPE_STRING;
    purc_variant_string->size = str_size;
    purc_variant_string->flags = 0;
    purc_variant_string->refc = 1;

    memcpy(purc_variant_string->bytes, str_utf8, str_size + 1);

    return purc_variant_string;

}

static bool purc_variant_string_check_utf8(const char* str_utf8)
{
    // todo

    return true;
}

purc_variant_t purc_variant_make_string_with_check (const char* str_utf8)
{
    purc_variant_t purc_variant_string = NULL;
    bool b_check = purc_variant_string_check_utf8(str_utf8);

    if(b_check)
        purc_variant_string = purc_variant_make_string(str_utf8)
    else
        purc_variant_string = VARIANT_TYPE_STRING;

    return purc_variant_string;
}

const char* purc_variant_get_string_const (purc_variant_t value)
{
    const char * str_str = NULL;

    if(purc_variant_is_type(value, VARIANT_TYPE_STRING))
        str_str = value->bytes;

    return str_str;
}

size_t purc_variant_string_length(const purc_variant_t value)
{
    size_t str_size = 0;

    if(purc_variant_is_type(value, VARIANT_TYPE_STRING))
        str_size = value->size;

    return size;
}

purc_variant_t purc_variant_make_byte_sequence (const unsigned char* bytes, size_t nr_bytes)
{
    int real_size = MAX(sizeof(long double), sizeof(void*) * 2);
    purc_variant_t purc_variant_sequence = NULL;
    
    if(nr_bytes <= real_size)
        real_size = 0;
    else
        real_size = nr_bytes;
    
    purc_variant_sequence = (purc_variant_t)malloc(sizeof(purc_variant) + real_size);

    if(purc_variant_sequence == NULL)
        return PURC_VARIANT_INVALID;

    purc_variant_sequence->type = VARIANT_TYPE_SEQUENCE;
    purc_variant_sequence->size = nr_bytes;
    purc_variant_sequence->flags = 0;
    purc_variant_sequence->refc = 1;

    memcpy(purc_variant_sequence->bytes, bytes, nr_bytes);

    return purc_variant_sequence;
}

const unsigned char* purc_variant_get_bytes_const (purc_variant_t value, size_t* nr_bytes)
{
    const unsigned char * bytes = NULL;

    if(purc_variant_is_type(value, VARIANT_TYPE_SEQUENCE))
        bytes = value->bytes;

    return bytes;
}

size_t purc_variant_sequence_length(const purc_variant_t sequence)
{
    size_t nr_bytes = 0;

    if(purc_variant_is_type(value, VARIANT_TYPE_SEQUENCE))
        nr_bytes = value->size;

    return nr_bytes;
}
