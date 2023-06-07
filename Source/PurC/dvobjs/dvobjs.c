/*
 * @file dvobjs.c
 * @author Geng Yue, Vincent Wei
 * @date 2021/07/02
 * @brief The interface of dynamic variant objects.
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

// #undef NDEBUG

#include "private/instance.h"
#include "private/errors.h"
#include "private/interpreter.h"
#include "private/dvobjs.h"
#include "private/atom-buckets.h"
#include "purc-variant.h"

#include <time.h>
#include <limits.h>

#define DVOBJS_SEPERATOR_COLON      ':'

ssize_t
pcdvobjs_quantity_in_format(const char *format, size_t *format_length)
{
    const char *seperator;

    long int quantity;

    if ((seperator = strchr(format, ':'))) {
        quantity = strtol(seperator + 1, NULL, 10);
        *format_length = seperator - format;
    }
    else {
        quantity = 0;
    }

    return (ssize_t)quantity;
}

static struct keyword_to_atom {
    const char *    keyword;
    purc_atom_t     atom;
} keywords2atoms [] = {
    { PURC_KW_caseless, 0 },    // "caseless"
    { PURC_KW_case,     0 },    // "case"
    { PURC_KW_regexp,   0 },    // "regexp"
    { PURC_KW_wildcard, 0 },    // "wildcard"
    { PURC_KW_number,   0 },    // "number"
    { PURC_KW_exact,    0 },    // "exact"
    { PURC_KW_auto,     0 },    // "auto"
    { PURC_KW_asc,      0 },    // "asc"
    { PURC_KW_desc,     0 },    // "desc"
    { PURC_KW_indexes,  0 },    // "indexes"
    { PURC_KW_values,   0 },    // "values"
    { PURC_KW_keys,     0 },    // "keys"
    { PURC_KW_kv_pairs, 0 },    // "kv-pairs"
    { PURC_KW_iv_pairs, 0 },    // "iv-pairs"
    { PURC_KW_i8,       0 },    // "i8"
    { PURC_KW_i16,      0 },    // "i16"
    { PURC_KW_i32,      0 },    // "i32"
    { PURC_KW_i64,      0 },    // "i64"
    { PURC_KW_i16le,    0 },    // "i16le"
    { PURC_KW_i32le,    0 },    // "i32le"
    { PURC_KW_i64le,    0 },    // "i64le"
    { PURC_KW_i16be,    0 },    // "i16be"
    { PURC_KW_i32be,    0 },    // "i32be"
    { PURC_KW_i64be,    0 },    // "i64be"
    { PURC_KW_u8,       0 },    // "u8"
    { PURC_KW_u16,      0 },    // "u16"
    { PURC_KW_u32,      0 },    // "u32"
    { PURC_KW_u64,      0 },    // "u64"
    { PURC_KW_u16le,    0 },    // "u16le"
    { PURC_KW_u32le,    0 },    // "u32le"
    { PURC_KW_u64le,    0 },    // "u64le"
    { PURC_KW_u16be,    0 },    // "u16be"
    { PURC_KW_u32be,    0 },    // "u32be"
    { PURC_KW_u64be,    0 },    // "u64be"
    { PURC_KW_f16,      0 },    // "f16"
    { PURC_KW_f32,      0 },    // "f32"
    { PURC_KW_f64,      0 },    // "f64"
    { PURC_KW_f96,      0 },    // "f96"
    { PURC_KW_f128,     0 },    // "f128"
    { PURC_KW_f16le,    0 },    // "f16le"
    { PURC_KW_f32le,    0 },    // "f32le"
    { PURC_KW_f64le,    0 },    // "f64le"
    { PURC_KW_f96le,    0 },    // "f96le"
    { PURC_KW_f128le,   0 },    // "f128le"
    { PURC_KW_f16be,    0 },    // "f16be"
    { PURC_KW_f32be,    0 },    // "f32be"
    { PURC_KW_f64be,    0 },    // "f64be"
    { PURC_KW_f96be,    0 },    // "f96be"
    { PURC_KW_f128be,   0 },    // "f128be"
    { PURC_KW_bytes,    0 },    // "bytes"
    { PURC_KW_utf8,     0 },    // "utf8"
    { PURC_KW_utf16,    0 },    // "utf16"
    { PURC_KW_utf32,    0 },    // "utf32"
    { PURC_KW_utf16le,  0 },    // "utf16le"
    { PURC_KW_utf32le,  0 },    // "utf32le"
    { PURC_KW_utf16be,  0 },    // "utf16be"
    { PURC_KW_utf32be,  0 },    // "utf32be"
    { PURC_KW_padding,  0 },    // "padding"
    { PURC_KW_binary,   0 },    // "binary"
    { PURC_KW_string,   0 },    // "string"
    { PURC_KW_uppercase,0 },    // "uppercase"
    { PURC_KW_lowercase,0 },    // "lowercase"
    { PURC_KW_longint,  0 },    // "longint"
    { PURC_KW_ulongint, 0 },    // "ulongint"
    { PURC_KW_longdouble,   0 },// "longdouble"
    { PURC_KW_object,   0 },    // "object"
    { PURC_KW_local,   0 },     // "local"
    { PURC_KW_global,  0 },     // "global"
    { PURC_KW_rfc1738,  0 },    // "rfc1738"
    { PURC_KW_rfc3986,  0 },    // "rfc3986"
};

/* Make sure the number of keywords2atoms matches the number of keywords */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(atoms,
        PCA_TABLESIZE(keywords2atoms) == PURC_GLOBAL_KEYWORD_NR);
#undef _COMPILE_TIME_ASSERT

int pcdvobjs_global_keyword_id(const char *keyword, size_t length)
{
    if (LIKELY(length > 0 || length <= MAX_LEN_KEYWORD)) {
        purc_atom_t atom;
#if 0
        /* TODO: use strndupa if it is available */
        char *tmp = strndup(keyword, length);
        atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);
        free(tmp);
#else
        char tmp[length + 1];
        strncpy(tmp, keyword, length);
        tmp[length]= '\0';
        atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);
#endif

        assert(keywords2atoms[PURC_K_KW_FIRST].atom);

        /* We can expect the atoms in keywords2atoms are increasing one by one. */
        if (atom >= keywords2atoms[PURC_K_KW_FIRST].atom &&
                atom <= keywords2atoms[PURC_K_KW_LAST].atom) {
            return (int)(atom - keywords2atoms[PURC_K_KW_FIRST].atom);
        }
    }

    return -1;
}


static int _init_instance(struct pcinst* inst,
        const purc_instance_extra_info* extra_info)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(extra_info);
    srand(time(NULL));

    return 0;
}

static void _cleanup_instance(struct pcinst* inst)
{
    UNUSED_PARAM(inst);
}

static int _init_once(void)
{
    for (size_t i = 0; i < PCA_TABLESIZE(keywords2atoms); i++) {
        keywords2atoms[i].atom =
            purc_atom_from_static_string_ex(ATOM_BUCKET_DVOBJ,
                    keywords2atoms[i].keyword);

        /* We expect the atoms are increasing one by one. */
        if (keywords2atoms[i].atom - keywords2atoms[0].atom != i)
            return -1;
    }
    // initialize others
    return 0;
}

struct pcmodule _module_dvobjs = {
    .id              = PURC_HAVE_EJSON,
    .module_inited   = 0,

    .init_once          = _init_once,
    .init_instance      = _init_instance,
    .cleanup_instance   = _cleanup_instance,
};

