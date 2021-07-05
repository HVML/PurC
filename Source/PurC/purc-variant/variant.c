/**
 * @file variant.h
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


#include "purc-rwstream.h"
#include "purc-variant.h"
#include "config.h"

#include "private/variant.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define PCVARIANT_ASSERT(s) assert(s)

struct pc_var_object;
typedef struct pc_var_object pc_var_object;
typedef struct pc_var_object* pc_var_object_t;
struct pc_var_object_kv;
typedef struct pc_var_object_kv pc_var_object_kv;
typedef struct pc_var_object_kv* pc_var_object_kv_t;

struct pc_var_set;
typedef struct pc_var_set pc_var_set;
typedef struct pc_var_set* pc_var_set_t;
struct pc_var_set_v;
typedef struct pc_var_set_v pc_var_set_v;
typedef struct pc_var_set_v* pc_var_set_v_t;

struct pc_var_array;
typedef struct pc_var_array pc_var_array;
typedef struct pc_var_array* pc_var_array_t;
struct pc_var_array_v;
typedef struct pc_var_array_v pc_var_array_v;
typedef struct pc_var_array_v* pc_var_array_v_t;


struct pc_var_object {
    pc_var_object_kv_t          head;
    pc_var_object_kv_t          tail;
};

struct pc_var_object_kv {
    char               *key;
    purc_variant_t      val;

    pc_var_object_kv_t  prev;
    pc_var_object_kv_t  next;

    unsigned int        refc;
    unsigned int        zombie:1;
};

struct pc_var_set {
    // better use hash, but list now
    pc_var_set_v_t           head;
    pc_var_set_v_t           tail;
};

struct pc_var_set_v {
    purc_variant_t      val;
    pc_var_set_v_t      next;
    pc_var_set_v_t      prev;

    unsigned int        refc;
    unsigned int        zombie:1;
};

struct purc_variant_object_iterator;


static struct purc_variant pcvariant_undefined = { PCVARIANT_TYPE_UNDEFINED, 0, PCVARIANT_FLAG_NOFREE, 0, {0} };
static struct purc_variant pcvariant_null      = { PCVARIANT_TYPE_NULL,      0, PCVARIANT_FLAG_NOFREE, 0, {0} };
static struct purc_variant pcvariant_false     = { PCVARIANT_TYPE_BOOLEAN,   0, PCVARIANT_FLAG_NOFREE, 0, { b:0 } };
static struct purc_variant pcvariant_true      = { PCVARIANT_TYPE_BOOLEAN,   0, PCVARIANT_FLAG_NOFREE, 0, { b:1 } };

static void purc_variant_free (purc_variant_t value)
{
    if (!value) return;

    switch (value->type)
    {
        case PCVARIANT_TYPE_NUMBER:
        case PCVARIANT_TYPE_LONGINT:
        case PCVARIANT_TYPE_LONGUINT:
        case PCVARIANT_TYPE_LONGDOUBLE:
            break;
        case PCVARIANT_TYPE_STRING:
        case PCVARIANT_TYPE_BYTESEQ:
            if (value->flags & PCVARIANT_FLAG_LONG) {
                if (value->u.sz_ptr[1]) {
                    free((void*)value->u.sz_ptr[1]);
                }
            }
            break;
        case PCVARIANT_TYPE_DYNAMIC:
            PCVARIANT_ASSERT(0); // not implemented yet
            break;
        case PCVARIANT_TYPE_NATIVE:
            if (value->u.sz_ptr[1]) {
                void *native = (void*)value->u.sz_ptr[0];
                purc_nvariant_releaser releaser = (purc_nvariant_releaser)value->u.sz_ptr[1];
                releaser(native);
            }
            break;
        case PCVARIANT_TYPE_OBJECT:
        case PCVARIANT_TYPE_ARRAY:
        case PCVARIANT_TYPE_SET:
            PCVARIANT_ASSERT(0); // not implemented yet
            break;
        default:
            PCVARIANT_ASSERT(0); // internal logic error
            break;
    }
    free(value);
}

unsigned int purc_variant_ref (purc_variant_t value)
{
    PCVARIANT_ASSERT(value);
    return ++value->refc;
}

unsigned int purc_variant_unref (purc_variant_t value)
{
    PCVARIANT_ASSERT(value);
    unsigned int refc = --value->refc;
    do {
        PCVARIANT_ASSERT(refc>=0);
        if (refc) break;
        if (value->flags & PCVARIANT_FLAG_NOFREE) break;
        purc_variant_free(value);
    } while (0);

    return refc;
}

purc_variant_t purc_variant_make_undefined (void)
{
    purc_variant_t var = &pcvariant_undefined;
    purc_variant_ref(var);
    return var;
}

purc_variant_t purc_variant_make_null (void)
{
    purc_variant_t var = &pcvariant_null;
    purc_variant_ref(var);
    return var;
}

purc_variant_t purc_variant_make_boolean (bool b)
{
    purc_variant_t var = b ? &pcvariant_true : &pcvariant_false;
    purc_variant_ref(var);
    return var;
}

purc_variant_t purc_variant_make_number (double d)
{
    purc_variant_t var = (purc_variant_t)calloc(1, sizeof(*var));

    if(var== NULL) return PURC_VARIANT_INVALID;

    var->type = PCVARIANT_TYPE_NUMBER;
    var->refc = 1;
    var->u.d  = d;

    return var;
}

purc_variant_t purc_variant_make_longuint (uint64_t u64)
{
    purc_variant_t var = (purc_variant_t)calloc(1, sizeof(*var));

    if(var== NULL) return PURC_VARIANT_INVALID;

    var->type   = PCVARIANT_TYPE_LONGUINT;
    var->refc   = 1;
    var->u.u64  = u64;

    return var;
}

purc_variant_t purc_variant_make_longint (int64_t i64)
{
    purc_variant_t var = (purc_variant_t)calloc(1, sizeof(*var));

    if(var== NULL) return PURC_VARIANT_INVALID;

    var->type   = PCVARIANT_TYPE_LONGINT;
    var->refc   = 1;
    var->u.i64  = i64;

    return var;
}

purc_variant_t purc_variant_make_longdouble (long double lf)
{
    purc_variant_t var = (purc_variant_t)calloc(1, sizeof(*var));

    if(var== NULL) return PURC_VARIANT_INVALID;

    var->type   = PCVARIANT_TYPE_LONGDOUBLE;
    var->refc   = 1;
    var->u.ld   = lf;

    return var;
}

purc_variant_t purc_variant_make_string (const char* str_utf8)
{
    PCVARIANT_ASSERT(str_utf8);

    purc_variant_t var = NULL;
    static const size_t short_space_limit = sizeof(((purc_variant_t)0)->u);

    var = (purc_variant_t)malloc(sizeof(*var));
    if (var==NULL) return PURC_VARIANT_INVALID;

    size_t str_size = strlen(str_utf8);
    if (str_size<short_space_limit) {
        var->type       = PCVARIANT_TYPE_STRING;
        var->refc       = 1;
        var->size       = str_size;
        strcpy((char*)var->u.bytes, str_utf8);
        return var;
    } else {
        var->type        = PCVARIANT_TYPE_STRING;
        var->refc        = 1;
        var->flags       = PCVARIANT_FLAG_LONG;
        var->u.sz_ptr[0] = str_size;
        var->u.sz_ptr[1] = (uintptr_t)strdup(str_utf8);
        if (!var->u.sz_ptr[1]) {
            free(var);
            return PURC_VARIANT_INVALID;
        }
        return var;
    }
}

static bool purc_variant_string_check_utf8(const char* str_utf8)
{
    // todo:
    UNUSED_PARAM(str_utf8);

    return true;
}

purc_variant_t purc_variant_make_string_with_check (const char* str_utf8)
{
    if (!purc_variant_string_check_utf8(str_utf8)) {
        return PURC_VARIANT_INVALID;
    }

    return purc_variant_make_string(str_utf8);
}

const char* purc_variant_get_string_const (purc_variant_t value)
{
    PCVARIANT_ASSERT(value);
    PCVARIANT_ASSERT(value->type==PCVARIANT_TYPE_STRING);

    if (value->flags & PCVARIANT_FLAG_LONG) {
        return (const char*)value->u.sz_ptr[1];
    } else {
        return (const char*)value->u.bytes;
    }
}

size_t purc_variant_string_length(const purc_variant_t value)
{
    PCVARIANT_ASSERT(value);
    PCVARIANT_ASSERT(value->type==PCVARIANT_TYPE_STRING);

    if (value->flags & PCVARIANT_FLAG_LONG) {
        return (size_t)value->u.sz_ptr[0];
    } else {
        return (size_t)value->size;
    }
}

purc_variant_t purc_variant_make_byte_sequence (const unsigned char* bytes, size_t nr_bytes)
{
    PCVARIANT_ASSERT(bytes);
    PCVARIANT_ASSERT(nr_bytes>=0);

    purc_variant_t var = NULL;
    static const size_t short_space_limit = sizeof(((purc_variant_t)0)->u);

    var = (purc_variant_t)malloc(sizeof(*var));
    if (var==NULL) return PURC_VARIANT_INVALID;

    if (nr_bytes<=short_space_limit) {
        var->type       = PCVARIANT_TYPE_BYTESEQ;
        var->refc       = 1;
        var->size       = nr_bytes;
        memcpy((char*)var->u.bytes, bytes, nr_bytes);
        return var;
    } else {
        var->type        = PCVARIANT_TYPE_BYTESEQ;
        var->refc        = 1;
        var->flags       = PCVARIANT_FLAG_LONG;
        var->u.sz_ptr[0] = nr_bytes;
        var->u.sz_ptr[1] = (uintptr_t)malloc(nr_bytes+1); // one-more-null-terminator, in case user print it
        if (!var->u.sz_ptr[1]) {
            free(var);
            return PURC_VARIANT_INVALID;
        }
        memcpy((void*)var->u.sz_ptr[1], bytes, nr_bytes);
        ((char*)var->u.sz_ptr[1])[nr_bytes] = '\0';
        return var;
    }
}

const unsigned char* purc_variant_get_bytes_const (purc_variant_t value, size_t* nr_bytes)
{
    PCVARIANT_ASSERT(value);
    PCVARIANT_ASSERT(value->type==PCVARIANT_TYPE_BYTESEQ);

    if (value->flags & PCVARIANT_FLAG_LONG) {
        if (nr_bytes) *nr_bytes = (size_t)value->u.sz_ptr[0];
        return (const unsigned char*)value->u.sz_ptr[1];
    } else {
        if (nr_bytes) *nr_bytes = value->size;
        return (const unsigned char*)value->u.bytes;
    }
}

size_t purc_variant_sequence_length(const purc_variant_t sequence)
{
    PCVARIANT_ASSERT(sequence);
    PCVARIANT_ASSERT(sequence->type==PCVARIANT_TYPE_BYTESEQ);

    if (sequence->flags & PCVARIANT_FLAG_LONG) {
        return (size_t)sequence->u.sz_ptr[0];
    } else {
        return (size_t)sequence->size;
    }
}

