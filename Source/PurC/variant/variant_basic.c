/*
 * @file variant-basic.c
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

#include <stdlib.h>
#include <string.h>

#include "purc_variant.h"
#include "variant_types.h"

static struct purc_variant pcvariant_null = { PURC_VARIANT_TYPE_NULL, 0, 0, PCVARIANT_FLAG_NOFREE };
static struct purc_variant pcvariant_undefined = { PURC_VARIANT_TYPE_UNDEFINED, 0, 0, PCVARIANT_FLAG_NOFREE };
static struct purc_variant pcvariant_false = { PURC_VARIANT_TYPE_BOOLEAN, 0, 0, PCVARIANT_FLAG_NOFREE, { b:0 } };
static struct purc_variant pcvariant_true = { PURC_VARIANT_TYPE_BOOLEAN, 0, 0, PCVARIANT_FLAG_NOFREE, { b:1 } };

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

    purc_variant_number->type = PURC_VARIANT_TYPE_NUMBER;
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

    purc_variant_longuint->type = PURC_VARIANT_TYPE_LONGINT;
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

    purc_variant_longint->type = PURC_VARIANT_TYPE_LONGINT;
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

    purc_variant_longdouble->type = PURC_VARIANT_TYPE_LONGDOUBLE;
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

    purc_variant_string->type = PURC_VARIANT_TYPE_STRING;
    purc_variant_string->size = str_size;
    purc_variant_string->flags = 0;
    purc_variant_string->refc = 1;

    if(real_size)
        memcpy(purc_variant_string->sz_ptr, str_utf8, str_size + 1);
    else
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
        purc_variant_string = PURC_VARIANT_TYPE_STRING;

    return purc_variant_string;
}

const char* purc_variant_get_string_const (purc_variant_t value)
{
    const char * str_str = NULL;
    int real_size = MAX(sizeof(long double), sizeof(void*) * 2);

    if(purc_variant_is_type(value, PURC_VARIANT_TYPE_STRING))
    {
        if(value->size < real_size)
            str_str = value->bytes;
        else
            str_str = value->sz_ptr;
    }

    return str_str;
}

size_t purc_variant_string_length(const purc_variant_t value)
{
    size_t str_size = 0;

    if(purc_variant_is_type(value, PURC_VARIANT_TYPE_STRING))
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

    purc_variant_sequence->type = PURC_VARIANT_TYPE_SEQUENCE;
    purc_variant_sequence->size = nr_bytes;
    purc_variant_sequence->flags = 0;
    purc_variant_sequence->refc = 1;

    if(real_size)
        memcpy(purc_variant_sequence->sz_ptr, bytes, nr_bytes);
    else
        memcpy(purc_variant_sequence->bytes, bytes, nr_bytes);

    return purc_variant_sequence;
}

const unsigned char* purc_variant_get_bytes_const (purc_variant_t value, size_t* nr_bytes)
{
    const unsigned char * bytes = NULL;
    int real_size = MAX(sizeof(long double), sizeof(void*) * 2);

    if(purc_variant_is_type(value, PURC_VARIANT_TYPE_SEQUENCE))
    {
        if(value->size <= real_size)
            bytes = value->bytes;
        else
            bytes = value->sz_ptr;
    }

    return bytes;
}

size_t purc_variant_sequence_length(const purc_variant_t sequence)
{
    size_t nr_bytes = 0;

    if(purc_variant_is_type(value, PURC_VARIANT_TYPE_SEQUENCE))
        nr_bytes = value->size;

    return nr_bytes;
}

purc_variant_t purc_variant_make_dynamic_value (CB_DYNAMIC_VARIANT getter, CB_DYNAMIC_VARIANT setter)
{
    purc_variant_t purc_variant_dynamic = (purc_variant_t)malloc(sizeof(purc_variant));

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

purc_variant_t purc_variant_make_native (void *native_obj, purc_nvariant_releaser releaser)
{
    purc_variant_t purc_variant_native = (purc_variant_t)malloc(sizeof(purc_variant));

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

unsigned int purc_variant_ref (purc_variant_t value)
{
    enum purc_variant_type type = purc_variant_get_type(value);

    if((type != PURC_VARIANT_TYPE_NULL) && (type != PURC_VARIANT_TYPE_UNDEFINED) && \
       (type != PURC_VARIANT_TYPE_BOOLEAN))
        value->refc ++;

    return value->refc;
}


bool purc_variant_is_type(const purc_variant_t value, enum purc_variant_type type)
{
    return (value->type == type);
}


enum purc_variant_type purc_variant_get_type(const purc_variant_t value)
{
    return value->type;
}


// todo
unsigned int purc_variant_ref (purc_variant_t value)
{
    enum purc_variant_type type = purc_variant_get_type(value);

    if((type != PURC_VARIANT_TYPE_NULL) && (type != PURC_VARIANT_TYPE_UNDEFINED) && \
       (type != PURC_VARIANT_TYPE_BOOLEAN))
        value->refc ++;

    return value->refc;
}

// todo
unsigned int purc_variant_unref (purc_variant_t value)
{
    enum purc_variant_type type = purc_variant_get_type(value);

    if((type == PURC_VARIANT_TYPE_NULL) || (type == PURC_VARIANT_TYPE_UNDEFINED) || \
       (type == PURC_VARIANT_TYPE_BOOLEAN))
        return 0;

    if(value->refc == 0)
    {
        if(value->flags & PCVARIANT_FLAG_NOFREE)
        {
        }
        else
        {
            free(value);
            return 0;
        }
    }
    else
        value->refc --;

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
