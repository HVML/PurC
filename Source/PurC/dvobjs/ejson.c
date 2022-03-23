/*
 * @file ejson.c
 * @author Geng Yue, Vincent Wei
 * @date 2021/07/02
 * @brief The implementation of EJSON dynamic variant object.
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

#undef NDEBUG

#include "private/instance.h"
#include "private/errors.h"
#include "private/atom-buckets.h"
#include "private/dvobjs.h"
#include "purc-variant.h"
#include "helper.h"

#include <assert.h>
#include <stdlib.h>

static purc_variant_t
type_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    const char *type;
    if (nr_args == 0) {
        // treat as undefined
        type = purc_variant_typename(PURC_VARIANT_TYPE_UNDEFINED);
    }
    else {
        assert(argv[0] != PURC_VARIANT_INVALID);
        type = purc_variant_typename(purc_variant_get_type(argv[0]));
    }

    return purc_variant_make_string_static(type, false);
}

static purc_variant_t
count_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    size_t count;

    if (nr_args == 0) {
        count = 0;  // treat as undefined
    }
    else {
        switch (purc_variant_get_type(argv[0])) {
        case PURC_VARIANT_TYPE_UNDEFINED:
            count = 0;
            break;

        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_BOOLEAN:
        case PURC_VARIANT_TYPE_EXCEPTION:
        case PURC_VARIANT_TYPE_NUMBER:
        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_ULONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
        case PURC_VARIANT_TYPE_ATOMSTRING:
        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
        case PURC_VARIANT_TYPE_DYNAMIC:
        case PURC_VARIANT_TYPE_NATIVE:
            count = 1;
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            count = purc_variant_object_get_size(argv[0]);
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            count = purc_variant_array_get_size(argv[0]);
            break;

        case PURC_VARIANT_TYPE_SET:
            count = purc_variant_set_get_size(argv[0]);
            break;
        }
    }

    return purc_variant_make_ulongint(count);
}

static purc_variant_t
numberify_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    double number;
    if (nr_args == 0) {
        // treat as undefined
        number = 0.0;
    }
    else {
        assert(argv[0]);
        number = purc_variant_numberify(argv[0]);
    }

    return purc_variant_make_number(number);
}

static purc_variant_t
booleanize_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    bool retv;
    if (nr_args == 0) {
        retv = false;
    }
    else {
        assert(argv[0]);
        if (purc_variant_booleanize(argv[0]))
            retv = true;
        else
            retv = false;
    }

    return purc_variant_make_boolean(retv);
}

static purc_variant_t
stringify_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    const char *str_static = NULL;
    char buff_in_stack[128];
    char *buff = NULL;
    size_t n = 0;

    if (nr_args == 0) {
        str_static = purc_variant_typename(PURC_VARIANT_TYPE_UNDEFINED);
    }
    else {
        switch (purc_variant_get_type (argv[0])) {
        case PURC_VARIANT_TYPE_UNDEFINED:
        case PURC_VARIANT_TYPE_NULL:
            str_static = purc_variant_typename(PURC_VARIANT_TYPE_UNDEFINED);
            break;

        case PURC_VARIANT_TYPE_BOOLEAN:
            if (purc_variant_is_true(argv[0]))
                str_static = PURC_KEYWORD_true;
            else
                str_static = PURC_KEYWORD_false;
            break;

        case PURC_VARIANT_TYPE_BSEQUENCE:
        case PURC_VARIANT_TYPE_OBJECT:
        case PURC_VARIANT_TYPE_ARRAY:
        case PURC_VARIANT_TYPE_SET:
            n = purc_variant_stringify_alloc(&buff, argv[0]);
            if (n == (size_t)-1) {
                // Keep the error code set by purc_variant_stringify_alloc.
                goto fatal;
            }
            break;

        case PURC_VARIANT_TYPE_EXCEPTION:
        case PURC_VARIANT_TYPE_ATOMSTRING:
        case PURC_VARIANT_TYPE_STRING:
        {
            const char *str = purc_variant_get_string_const_ex(argv[0], &n);
            assert(str);

            if (n > 0) {
                buff = malloc(n);
                if (buff == NULL) {
                    purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                    goto fatal;
                }
                memcpy(buff, str, n);
            }
            else
                str_static = "";
            break;
        }

        case PURC_VARIANT_TYPE_NUMBER:
        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_ULONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
        case PURC_VARIANT_TYPE_DYNAMIC:
        case PURC_VARIANT_TYPE_NATIVE:
            n = purc_variant_stringify(buff_in_stack, sizeof(buff_in_stack),
                    argv[0]);
            if (n == (size_t)-1 || n >= sizeof(buff_in_stack)) {
                purc_set_error(PURC_ERROR_TOO_SMALL_BUFF);
                goto fatal;
            }
            buff = buff_in_stack;
            break;
        }
    }

    if (str_static) {
        return purc_variant_make_string_static(str_static, false);
    }
    else if (buff == buff_in_stack) {
        return purc_variant_make_string(buff, false);
    }
    else if (buff != NULL) {
        return purc_variant_make_string_reuse_buff(buff, n, false);
    }
    else {
        assert(0);
        return purc_variant_make_string_static("", false);
    }

fatal:
    return PURC_VARIANT_INVALID;
}

enum {
#define _KW_real_json       "real-json"
    K_KW_real_json,
#define _KW_real_ejson      "real-ejson"
    K_KW_real_ejson,
#define _KW_runtime_null    "runtime-null"
    K_KW_runtime_null,
#define _KW_runtime_string  "runtime-string"
    K_KW_runtime_string,
#define _KW_plain           "plain"
    K_KW_plain,
#define _KW_spaced          "spaced"
    K_KW_spaced,
#define _KW_pretty          "pretty"
    K_KW_pretty,
#define _KW_pretty_tab      "pretty-tab"
    K_KW_pretty_tab,
#define _KW_bseq_hex_string "bseq-hex-string"
    K_KW_bseq_hex_string,
#define _KW_bseq_hex        "bseq-hex"
    K_KW_bseq_hex,
#define _KW_bseq_bin        "bseq-bin"
    K_KW_bseq_bin,
#define _KW_bseq_bin_dots   "bseq-bin-dots"
    K_KW_bseq_bin_dots,
#define _KW_bseq_base64     "bseq-base64"
    K_KW_bseq_base64,
#define _KW_no_trailing_zero    "no-trailing-zero"
    K_KW_no_trailing_zero,
#define _KW_no_slash_escape     "no-slash-escape"
    K_KW_no_slash_escape,
};

static struct keyword_to_atom {
    const char *    keyword;
    unsigned int    flag;
    purc_atom_t     atom;
} keywords2atoms [] = {
    { _KW_real_json,        PCVARIANT_SERIALIZE_OPT_REAL_JSON, 0 },
    { _KW_real_ejson,       PCVARIANT_SERIALIZE_OPT_REAL_EJSON, 0 },
    { _KW_runtime_null,     PCVARIANT_SERIALIZE_OPT_RUNTIME_NULL, 0 },
    { _KW_runtime_string,   PCVARIANT_SERIALIZE_OPT_RUNTIME_STRING, 0 },
    { _KW_plain,            PCVARIANT_SERIALIZE_OPT_PLAIN, 0 },
    { _KW_spaced,           PCVARIANT_SERIALIZE_OPT_SPACED, 0 },
    { _KW_pretty,           PCVARIANT_SERIALIZE_OPT_PRETTY, 0 },
    { _KW_pretty_tab,       PCVARIANT_SERIALIZE_OPT_PRETTY_TAB, 0 },
    { _KW_bseq_hex_string,  PCVARIANT_SERIALIZE_OPT_BSEQUENCE_HEX_STRING, 0 },
    { _KW_bseq_hex,         PCVARIANT_SERIALIZE_OPT_BSEQUENCE_HEX, 0 },
    { _KW_bseq_bin,         PCVARIANT_SERIALIZE_OPT_BSEQUENCE_BIN, 0 },
    { _KW_bseq_bin_dots,    PCVARIANT_SERIALIZE_OPT_BSEQUENCE_BIN_DOT, 0 },
    { _KW_bseq_base64,      PCVARIANT_SERIALIZE_OPT_BSEQUENCE_BASE64, 0 },
    { _KW_no_trailing_zero, PCVARIANT_SERIALIZE_OPT_NOZERO, 0 },
    { _KW_no_slash_escape,  PCVARIANT_SERIALIZE_OPT_NOSLASHESCAPE, 0 },
};

static purc_variant_t
serialize_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t serialize = NULL;
    char *buf = NULL;
    size_t sz_stream = 0;

    if (nr_args == 0) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    serialize = purc_rwstream_new_buffer (32, STREAM_SIZE);
    purc_variant_serialize (argv[0], serialize, 3, 0, &sz_stream);
    buf = purc_rwstream_get_mem_buffer (serialize, &sz_stream);
    purc_rwstream_destroy (serialize);

    ret_var = purc_variant_make_string_reuse_buff (buf, sz_stream + 1, false);

    return ret_var;
}

typedef struct __dvobjs_ejson_arg {
    bool asc;
    bool caseless;
    pcutils_map *map;
} dvobjs_ejson_arg;

static int my_array_sort (purc_variant_t v1, purc_variant_t v2, void *ud)
{
    int ret = 0;
    char *p1 = NULL;
    char *p2 = NULL;
    pcutils_map_entry *entry = NULL;

    dvobjs_ejson_arg *sort_arg = (dvobjs_ejson_arg *)ud;
    entry = pcutils_map_find (sort_arg->map, v1);
    p1 = (char *)entry->val;
    entry = NULL;
    entry = pcutils_map_find (sort_arg->map, v2);
    p2 = (char *)entry->val;

    if (sort_arg->caseless)
        ret = strcasecmp (p1, p2);
    else
        ret = strcmp (p1, p2);

    if (!sort_arg->asc)
        ret = -1 * ret;

    if (ret != 0)
        ret = ret > 0? 1: -1;
    return ret;
}

static void * map_copy_key(const void *key)
{
    return (void *)key;
}

static void map_free_key(void *key)
{
    UNUSED_PARAM(key);
}

static void *map_copy_val(const void *val)
{
    return (void *)val;
}

static int map_comp_key(const void *key1, const void *key2)
{
    int ret = 0;
    if (key1 != key2)
        ret = key1 > key2? 1: -1;
    return ret;
}

static void map_free_val(void *val)
{
    if (val)
        free (val);
}

static purc_variant_t
sort_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    size_t i = 0;
    size_t totalsize = 0;
    const char *option = NULL;
    const char *order = NULL;
    dvobjs_ejson_arg sort_arg;
    char *buf = NULL;

    sort_arg.asc = true;
    sort_arg.caseless = false;
    sort_arg.map = NULL;

    if ((argv == NULL) || (nr_args < 2)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (!(purc_variant_is_array (argv[0]) || purc_variant_is_set (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    // get sort order: asc, desc
    if ((argv[1] == PURC_VARIANT_INVALID) ||
                (!purc_variant_is_string (argv[1]))) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    order = purc_variant_get_string_const (argv[1]);
    if (strcasecmp (order, STRING_COMP_MODE_DESC) == 0)
        sort_arg.asc = false;

    // get sort option: case, caseless
    if ((nr_args == 3) && (argv[2] != PURC_VARIANT_INVALID) &&
                (purc_variant_is_string (argv[2]))) {
        option = purc_variant_get_string_const (argv[2]);
        if (strcasecmp (option, STRING_COMP_MODE_CASELESS) == 0)
            sort_arg.caseless = true;
    }

    sort_arg.map = pcutils_map_create (map_copy_key, map_free_key,
            map_copy_val, map_free_val, map_comp_key, false);

    // it is the array
    if (purc_variant_is_array (argv[0])) {
        totalsize = purc_variant_array_get_size (argv[0]);

        for (i = 0; i < totalsize; ++i) {
            val = purc_variant_array_get(argv[0], i);
            purc_variant_stringify_alloc (&buf, val);
            pcutils_map_find_replace_or_insert (sort_arg.map, val, buf, NULL);
        }
        pcvariant_array_sort (argv[0], (void *)&sort_arg, my_array_sort);
    }
    else {    // it is the set
        pcvariant_set_sort (argv[0]);
    }

    pcutils_map_destroy (sort_arg.map);

    ret_var = argv[0];

    return ret_var;
}

static purc_variant_t
compare_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    const char *option = NULL;
    double compare = 0.0L;
    unsigned int flag = PCVARIANT_COMPARE_OPT_AUTO;

    if ((argv == NULL) || (nr_args < 3)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[2] != PURC_VARIANT_INVALID) &&
         (!purc_variant_is_string (argv[2]))) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    option = purc_variant_get_string_const (argv[2]);

    if (strcasecmp (option, STRING_COMP_MODE_CASELESS) == 0)
        flag = PCVARIANT_COMPARE_OPT_CASELESS;
    else if (strcasecmp (option, STRING_COMP_MODE_CASE) == 0)
        flag = PCVARIANT_COMPARE_OPT_CASE;
    else if (strcasecmp (option, STRING_COMP_MODE_NUMBER) == 0)
        flag = PCVARIANT_COMPARE_OPT_NUMBER;
    else
        flag = PCVARIANT_COMPARE_OPT_AUTO;

    compare = purc_variant_compare_ex (argv[0], argv[1], flag);

    ret_var = purc_variant_make_number (compare);

    return ret_var;
}

// only for test now.
purc_variant_t purc_dvobj_ejson_new (void)
{
    static struct purc_dvobj_method method [] = {
        {"type",        type_getter, NULL},
        {"count",       count_getter, NULL},
        {"numberify",   numberify_getter, NULL},
        {"booleanize",  booleanize_getter, NULL},
        {"stringify",   stringify_getter, NULL},
        {"serialize",   serialize_getter, NULL},
        {"sort",        sort_getter, NULL},
        {"compare",     compare_getter, NULL}
    };

    if (keywords2atoms[0].atom == 0) {
        for (size_t i = 0; i < PCA_TABLESIZE(keywords2atoms); i++) {
            keywords2atoms[i].atom =
                purc_atom_from_static_string_ex(ATOM_BUCKET_DVOBJ,
                    keywords2atoms[i].keyword);
        }

    }

    return purc_dvobj_make_from_methods (method, PCA_TABLESIZE(method));
}
