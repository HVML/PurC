/*
 * @file data.c
 * @author Geng Yue, Vincent Wei
 * @date 2021/07/02
 * @brief The implementation of DATA dynamic variant object.
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

#include "purc-variant.h"
#include "purc-utils.h"

#include "private/variant.h"
#include "private/errors.h"
#include "private/atom-buckets.h"
#include "private/dvobjs.h"
#include "private/utils.h"
#include "private/utf8.h"
#include "helper.h"

#include <assert.h>
#include <stdlib.h>

static purc_variant_t
type_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

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
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

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
        case PURC_VARIANT_TYPE_SET:
        case PURC_VARIANT_TYPE_TUPLE:
            count = purc_variant_linear_container_get_size(argv[0]);
            break;
        }
    }

    return purc_variant_make_ulongint(count);
}

static purc_variant_t
arith_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 3) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *op;
    size_t op_len;
    op = purc_variant_get_string_const_ex(argv[0], &op_len);
    if (op == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    op = pcutils_trim_spaces(op, &op_len);
    if (op_len != 1) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    int64_t l_operand, r_operand;
    if (!purc_variant_cast_to_longint(argv[1], &l_operand, true) ||
            !purc_variant_cast_to_longint(argv[2], &r_operand, true)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    int64_t result = 0;
    switch (op[0]) {
    case '+':
        result = l_operand + r_operand;
        break;

    case '-':
        result = l_operand - r_operand;
        break;

    case '*':
        result = l_operand * r_operand;
        break;

    case '/':
        if (r_operand == 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        result = l_operand / r_operand;
        break;

    case '%':
        if (r_operand == 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        result = l_operand % r_operand;
        break;

    case '^':
        if (r_operand < 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        result = 1;
        while (r_operand) {
            result *= l_operand;
            r_operand--;
        }
        break;

    default:
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
        break;
    }

    return purc_variant_make_longint(result);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
bitwise_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *op;
    size_t op_len;
    op = purc_variant_get_string_const_ex(argv[0], &op_len);
    if (op == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    op = pcutils_trim_spaces(op, &op_len);
    if (op_len != 1) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    uint64_t l_operand, r_operand;
    if (!purc_variant_cast_to_ulongint(argv[1], &l_operand, true)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (op[0] == '~') {
        r_operand = 0;
    }
    else if (nr_args < 3) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }
    else if (!purc_variant_cast_to_ulongint(argv[2], &r_operand, true)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    uint64_t result = 0;
    switch (op[0]) {
        case '~':
            result = ~l_operand;
            break;

        case '&':
            result = l_operand & r_operand;
            break;

        case '|':
            result = l_operand | r_operand;
            break;

        case '^':
            result = l_operand ^ r_operand;
            break;

        case '<':
            result = l_operand << r_operand;
            break;

        case '>':
            result = l_operand >> r_operand;
            break;

        default:
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
            break;
    }

    return purc_variant_make_ulongint(result);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
numerify_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    double number;
    if (nr_args == 0) {
        // treat as undefined
        number = 0.0;
    }
    else {
        assert(argv[0]);
        number = purc_variant_numerify(argv[0]);
    }

    return purc_variant_make_number(number);
}

static purc_variant_t
booleanize_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

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
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

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
            str_static = purc_variant_typename(PURC_VARIANT_TYPE_UNDEFINED);
            break;

        case PURC_VARIANT_TYPE_NULL:
            str_static = purc_variant_typename(PURC_VARIANT_TYPE_NULL);
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
        case PURC_VARIANT_TYPE_TUPLE:
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
                buff = malloc(n+1);
                if (buff == NULL) {
                    purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                    goto fatal;
                }
                memcpy(buff, str, n);
                buff[n] = '\0';
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
            n = purc_variant_stringify_buff(buff_in_stack, sizeof(buff_in_stack),
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

#define _KW_DELIMITERS  " \t\n\v\f\r"

static struct keyword_to_atom {
    const char *    keyword;
    unsigned int    flag;
    purc_atom_t     atom;
} keywords2atoms [] = {
    { _KW_real_json,        PCVRNT_SERIALIZE_OPT_REAL_JSON, 0 },
    { _KW_real_ejson,       PCVRNT_SERIALIZE_OPT_REAL_EJSON, 0 },
    { _KW_runtime_null,     PCVRNT_SERIALIZE_OPT_RUNTIME_NULL, 0 },
    { _KW_runtime_string,   PCVRNT_SERIALIZE_OPT_RUNTIME_STRING, 0 },
    { _KW_plain,            PCVRNT_SERIALIZE_OPT_PLAIN, 0 },
    { _KW_spaced,           PCVRNT_SERIALIZE_OPT_SPACED, 0 },
    { _KW_pretty,           PCVRNT_SERIALIZE_OPT_PRETTY, 0 },
    { _KW_pretty_tab,       PCVRNT_SERIALIZE_OPT_PRETTY_TAB, 0 },
    { _KW_bseq_hex_string,  PCVRNT_SERIALIZE_OPT_BSEQUENCE_HEX_STRING, 0 },
    { _KW_bseq_hex,         PCVRNT_SERIALIZE_OPT_BSEQUENCE_HEX, 0 },
    { _KW_bseq_bin,         PCVRNT_SERIALIZE_OPT_BSEQUENCE_BIN, 0 },
    { _KW_bseq_bin_dots,    PCVRNT_SERIALIZE_OPT_BSEQUENCE_BIN_DOT, 0 },
    { _KW_bseq_base64,      PCVRNT_SERIALIZE_OPT_BSEQUENCE_BASE64, 0 },
    { _KW_no_trailing_zero, PCVRNT_SERIALIZE_OPT_NOZERO, 0 },
    { _KW_no_slash_escape,  PCVRNT_SERIALIZE_OPT_NOSLASHESCAPE, 0 },
};

static purc_variant_t
serialize_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    const char *options = NULL;
    size_t options_len;
    unsigned int flags = PCVRNT_SERIALIZE_OPT_PLAIN;

    purc_variant_t vrt;

    if (nr_args == 0) {
        vrt = purc_variant_make_undefined();
        if (vrt == PURC_VARIANT_INVALID) {
            goto fatal;
        }
    }
    else {
        vrt = argv[0];

        if (nr_args > 1) {
            options = purc_variant_get_string_const_ex(argv[1], &options_len);
            if (options) {
                options = pcutils_trim_spaces(options, &options_len);
                if (options_len == 0) {
                    options = NULL;
                }
            }
        }
    }

    if (options) {
        size_t length = 0;
        const char *option = pcutils_get_next_token_len(options, options_len,
                _KW_DELIMITERS, &length);

        do {

            if (length > 0 && length <= MAX_LEN_KEYWORD) {
                purc_atom_t atom;

#if 0
                /* TODO: use strndupa if it is available */
                char *tmp = strndup(option, length);
                atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);
                free(tmp);
#else
                char tmp[length + 1];
                strncpy(tmp, option, length);
                tmp[length]= '\0';
                atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);
#endif

                if (atom > 0) {
                    size_t i;
                    for (i = 0; i < PCA_TABLESIZE(keywords2atoms); i++) {
                        if (atom == keywords2atoms[i].atom) {
                            if (keywords2atoms[i].flag &
                                    PCVRNT_SERIALIZE_OPT_BSEQUENCE_MASK) {
                                // clear the byte sequence mask
                                flags &= ~PCVRNT_SERIALIZE_OPT_BSEQUENCE_MASK;
                            }

                            flags |= keywords2atoms[i].flag;
                        }
                    }
                }
            }

            if (options_len <= length)
                break;

            options_len -= length;
            option = pcutils_get_next_token_len(option + length, options_len,
                    _KW_DELIMITERS, &length);
        } while (option);
    }

    purc_rwstream_t my_stream;
    ssize_t n;

    my_stream = purc_rwstream_new_buffer(LEN_INI_SERIALIZE_BUF,
            LEN_MAX_SERIALIZE_BUF);
    n = purc_variant_serialize(vrt, my_stream, 0, flags, NULL);
    if (nr_args == 0)
        purc_variant_unref(vrt);

    if (n == -1) {
        goto fatal;
    }

    purc_rwstream_write(my_stream, "\0", 1);

    char *buf = NULL;
    size_t sz_content, sz_buffer;
    buf = purc_rwstream_get_mem_buffer_ex(my_stream,
            &sz_content, &sz_buffer, true);
    purc_rwstream_destroy(my_stream);

    return purc_variant_make_string_reuse_buff(buf, sz_buffer, false);

fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
parse_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *string;
    size_t length;
    string = purc_variant_get_string_const_ex(argv[0], &length);
    if (string == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    struct purc_ejson_parsing_tree *ptree;
    ptree = purc_variant_ejson_parse_string(string, length);
    if (ptree == NULL) {
        goto failed;
    }

    purc_variant_t retv;
    retv = purc_ejson_parsing_tree_evalute(ptree, NULL, NULL,
            (call_flags & PCVRT_CALL_FLAG_SILENTLY));
    purc_ejson_parsing_tree_destroy(ptree);
    return retv;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
isequal_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    bool v = purc_variant_is_equal_to(argv[0], argv[1]);
    return purc_variant_make_boolean(v);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
compare_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *option = NULL;
    size_t option_len;
    unsigned int flag = PCVRNT_COMPARE_METHOD_AUTO;

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (nr_args >= 3) {
        option = purc_variant_get_string_const_ex(argv[2], &option_len);
        if (option == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        option = pcutils_trim_spaces(option, &option_len);
        if (option_len == 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        int cmp_id = pcdvobjs_global_keyword_id(option, option_len);
        switch (cmp_id) {
        case PURC_K_KW_auto:
            flag = PCVRNT_COMPARE_METHOD_AUTO;
            break;

        case PURC_K_KW_number:
            flag = PCVRNT_COMPARE_METHOD_NUMBER;
            break;

        case PURC_K_KW_caseless:
            flag = PCVRNT_COMPARE_METHOD_CASELESS;
            break;

        case PURC_K_KW_case:
            flag = PCVRNT_COMPARE_METHOD_CASE;
            break;

        default:
            if (!(call_flags & PCVRT_CALL_FLAG_SILENTLY)) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }
            break;
        }
    }

    double result = 0.0;
    result = purc_variant_compare_ex(argv[0], argv[1], flag);
    return purc_variant_make_number(result);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

int purc_dvobj_parse_format(const char *format, size_t format_len,
        size_t *_quantity)
{
    size_t keyword_len = format_len;
    ssize_t quantity = pcdvobjs_quantity_in_format(format, &keyword_len);
    if (quantity < 0 || keyword_len == 0) {
        return -1;
    }

    *_quantity = (size_t)quantity;

    return pcdvobjs_global_keyword_id(format, keyword_len);
}

static purc_variant_t
fetchstr_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *encoding = NULL;
    size_t encoding_len;
    size_t length;
    ssize_t offset = 0;

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const unsigned char *bytes;
    size_t nr_bytes;
    bytes = purc_variant_get_bytes_const(argv[0], &nr_bytes);
    if (bytes == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    encoding = purc_variant_get_string_const_ex(argv[1], &encoding_len);
    if (encoding == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    encoding = pcutils_trim_spaces(encoding, &encoding_len);
    if (encoding_len == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    int encoding_id = purc_dvobj_parse_format(encoding, encoding_len, &length);
    if (encoding_id < 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (length == 0) {
        length = nr_bytes;
    }

    if (nr_args > 2 && !purc_variant_is_null(argv[2])) {
        uint64_t tmp;
        if (!purc_variant_cast_to_ulongint(argv[2], &tmp, false)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
        length = (size_t)tmp;
    }

    if (nr_args > 3) {
        int64_t tmp;
        if (!purc_variant_cast_to_longint(argv[3], &tmp, false)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
        offset = (ssize_t)tmp;
    }

    if (offset > 0 && (size_t)offset >= nr_bytes) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (offset < 0 && (size_t)-offset > nr_bytes) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (offset < 0)
        offset = nr_bytes + offset;

    if (nr_args > 2 && !purc_variant_is_null(argv[2])) {
        if (offset + length > nr_bytes) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    if (length >= nr_bytes - offset) {
        length = nr_bytes - offset;
    }

    if (length == 0)
        return purc_variant_make_string_static("", false);

    size_t consumed;
    purc_variant_t retv;
    retv = purc_dvobj_unpack_string(bytes + offset, length, &consumed,
            encoding_id, (call_flags & PCVRT_CALL_FLAG_SILENTLY));

    if (retv == PURC_VARIANT_INVALID) {
        goto fatal;
    }
    else if (purc_variant_is_undefined(retv)) {
        purc_variant_unref(retv);
        goto failed;
    }

    return retv;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_string_static("", false);

fatal:
    return PURC_VARIANT_INVALID;
}

typedef purc_real_t (*fn_fetch_real)(const unsigned char *bytes);
typedef bool (*fn_dump_real)(unsigned char *dst, purc_real_t real, bool force);

static const struct real_info {
    uint8_t         length;         // unit length in bytes
    uint8_t         real_type;      // EJSON real type
    fn_fetch_real   fetcher;        // fetcher
    fn_dump_real    dumper;         // dumper
} real_info[] = {
    { 1,  PURC_VARIANT_TYPE_LONGINT,
        purc_fetch_i8,              purc_dump_i8       },  // "i8"
    { 2,  PURC_VARIANT_TYPE_LONGINT,
        purc_fetch_i16,             purc_dump_i16      },  // "i16"
    { 4,  PURC_VARIANT_TYPE_LONGINT,
        purc_fetch_i32,             purc_dump_i32      },  // "i32"
    { 8,  PURC_VARIANT_TYPE_LONGINT,
        purc_fetch_i64,             purc_dump_i64      },  // "i64"
    { 2,  PURC_VARIANT_TYPE_LONGINT,
        purc_fetch_i16le,           purc_dump_i16le    },  // "i16le"
    { 4,  PURC_VARIANT_TYPE_LONGINT,
        purc_fetch_i32le,           purc_dump_i32le    },  // "i32le"
    { 8,  PURC_VARIANT_TYPE_LONGINT,
        purc_fetch_i64le,           purc_dump_i64le    },  // "i64le"
    { 2,  PURC_VARIANT_TYPE_LONGINT,
        purc_fetch_i16be,           purc_dump_i16be    },  // "i16be"
    { 4,  PURC_VARIANT_TYPE_LONGINT,
        purc_fetch_i32be,           purc_dump_i32be    },  // "i32be"
    { 8,  PURC_VARIANT_TYPE_LONGINT,
        purc_fetch_i64be,           purc_dump_i64be    },  // "i64be"
    { 1,  PURC_VARIANT_TYPE_ULONGINT,
        purc_fetch_u8,              purc_dump_u8       },  // "u8"
    { 2,  PURC_VARIANT_TYPE_ULONGINT,
        purc_fetch_u16,             purc_dump_u16      },  // "u16"
    { 4,  PURC_VARIANT_TYPE_ULONGINT,
        purc_fetch_u32,             purc_dump_u32      },  // "u32"
    { 8,  PURC_VARIANT_TYPE_ULONGINT,
        purc_fetch_u64,             purc_dump_u64      },  // "u64"
    { 2,  PURC_VARIANT_TYPE_ULONGINT,
        purc_fetch_u16le,           purc_dump_u16le    },  // "u16le"
    { 4,  PURC_VARIANT_TYPE_ULONGINT,
        purc_fetch_u32le,           purc_dump_u32le    },  // "u32le"
    { 8,  PURC_VARIANT_TYPE_ULONGINT,
        purc_fetch_u64le,           purc_dump_u64le    },  // "u64le"
    { 2,  PURC_VARIANT_TYPE_ULONGINT,
        purc_fetch_u16be,           purc_dump_u16be    },  // "u16be"
    { 4,  PURC_VARIANT_TYPE_ULONGINT,
        purc_fetch_u32be,           purc_dump_u32be    },  // "u32be"
    { 8,  PURC_VARIANT_TYPE_ULONGINT,
        purc_fetch_u64be,           purc_dump_u64be    },  // "u64be"
    { 2,  PURC_VARIANT_TYPE_NUMBER,
        purc_fetch_f16,             purc_dump_f16      },  // "f16"
    { 4,  PURC_VARIANT_TYPE_NUMBER,
        purc_fetch_f32,             purc_dump_f32      },  // "f32"
    { 8,  PURC_VARIANT_TYPE_NUMBER,
        purc_fetch_f64,             purc_dump_f64      },  // "f64"
    { 12, PURC_VARIANT_TYPE_LONGDOUBLE,
        purc_fetch_f96,             purc_dump_f96      },  // "f96"
    { 16, PURC_VARIANT_TYPE_LONGDOUBLE,
        purc_fetch_f128,            purc_dump_f128     },  // "f128"
    { 2,  PURC_VARIANT_TYPE_NUMBER,
        purc_fetch_f16le,           purc_dump_f16le    },  // "f16le"
    { 4,  PURC_VARIANT_TYPE_NUMBER,
        purc_fetch_f32le,           purc_dump_f32le    },  // "f32le"
    { 8,  PURC_VARIANT_TYPE_NUMBER,
        purc_fetch_f64le,           purc_dump_f64le    },  // "f64le"
    { 12, PURC_VARIANT_TYPE_LONGDOUBLE,
        purc_fetch_f96le,           purc_dump_f96le    },  // "f96le"
    { 16, PURC_VARIANT_TYPE_LONGDOUBLE,
        purc_fetch_f128le,          purc_dump_f128le   },  // "f128le"
    { 2,  PURC_VARIANT_TYPE_NUMBER,
        purc_fetch_f16be,           purc_dump_f16be    },  // "f16be"
    { 4,  PURC_VARIANT_TYPE_NUMBER,
        purc_fetch_f32be,           purc_dump_f32be    },  // "f32be"
    { 8,  PURC_VARIANT_TYPE_NUMBER,
        purc_fetch_f64be,           purc_dump_f64be    },  // "f64be"
    { 12, PURC_VARIANT_TYPE_LONGDOUBLE,
        purc_fetch_f96be,           purc_dump_f96be    },  // "f96be"
    { 16, PURC_VARIANT_TYPE_LONGDOUBLE,
        purc_fetch_f128be,          purc_dump_f128be   },  // "f128be"
};

static purc_variant_t
fetchreal_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *format = NULL;
    size_t format_len = 0;
    size_t length = 0;
    ssize_t offset = 0;

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const unsigned char *bytes;
    size_t nr_bytes;
    bytes = purc_variant_get_bytes_const(argv[0], &nr_bytes);
    if (bytes == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (nr_bytes == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    format = purc_variant_get_string_const_ex(argv[1], &format_len);
    if (format == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    // parse format and get the length of real number.
    format = pcutils_trim_spaces(format, &format_len);
    if (format_len == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    size_t quantity;
    int format_id = purc_dvobj_parse_format(format, format_len, &quantity);
    if (format_id < PURC_K_KW_i8 || format_id > PURC_K_KW_f128be) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (quantity == 0)
        quantity = 1;

    length = real_info[format_id - PURC_K_KW_i8].length * quantity;
    if (nr_args > 2) {
        int64_t tmp;
        if (!purc_variant_cast_to_longint(argv[2], &tmp, false)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
        offset = (ssize_t)tmp;
    }

    if (offset > 0 && (size_t)offset >= nr_bytes) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (offset < 0 && (size_t)-offset > nr_bytes) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (offset < 0) {
        offset = nr_bytes + offset;
    }

    if (offset + length > nr_bytes) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    bytes += offset;
    nr_bytes -= offset;

    purc_variant_t retv;
    retv = purc_dvobj_unpack_real(bytes, nr_bytes, format_id, quantity);
    if (retv == PURC_VARIANT_INVALID) {
        goto fatal;
    }
    else if (purc_variant_is_undefined(retv)) {
        purc_variant_unref(retv);
        goto failed;
    }

    return retv;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

fatal:
    return PURC_VARIANT_INVALID;
}

typedef size_t (*fn_encode_str)(const char *str, size_t len, size_t nr_chars,
        unsigned char *dst, size_t max_bytes);

static size_t dump_utf8_string(const char *str, size_t len, size_t nr_chars,
        unsigned char *dst, size_t max_bytes)
{

    if (max_bytes > len) {
        memcpy(dst, str, len);
        dst[len] = 0;
        return len + 1;
    }

    size_t n = 0;
    const char *p = str;
    while (nr_chars > 0 && *p) {
        const char *next;
        size_t len;

        next = pcutils_utf8_next_char(p);
        len = next - p;
        if (max_bytes < n + len)
            break;

        memcpy(dst, p, len);
        dst += len;
        n += len;

        p = next;
        nr_chars--;
    }

    if (max_bytes > n) {
        *dst = 0;   // if there still is room.
    }

    return n;
}

purc_variant_t
purc_dvobj_unpack_real(const unsigned char *bytes, size_t nr_bytes,
        int format_id, size_t quantity)
{
    int real_id = format_id - PURC_K_KW_i8;

    if (real_info[real_id].length * quantity > nr_bytes) {
        return purc_variant_make_undefined();
    }

    if (quantity == 1) {
        purc_real_t real = real_info[real_id].fetcher(bytes);
        switch (real_info[real_id].real_type) {
            case PURC_VARIANT_TYPE_LONGINT:
                return purc_variant_make_longint(real.i64);
            case PURC_VARIANT_TYPE_ULONGINT:
                return purc_variant_make_ulongint(real.u64);
            case PURC_VARIANT_TYPE_NUMBER:
                return purc_variant_make_ulongint(real.d);
            case PURC_VARIANT_TYPE_LONGDOUBLE:
                return purc_variant_make_ulongint(real.ld);
            default:
                assert(0);
                break;
        }
    }
    else {
        purc_variant_t retv;

        retv = purc_variant_make_array(0, PURC_VARIANT_INVALID);
        if (retv == PURC_VARIANT_INVALID) {
            goto fatal;
        }

        for (size_t i = 0; i < quantity; i++) {
            purc_variant_t vrt = PURC_VARIANT_INVALID;
            purc_real_t real = real_info[real_id].fetcher(bytes);
            switch (real_info[real_id].real_type) {
                case PURC_VARIANT_TYPE_LONGINT:
                    vrt = purc_variant_make_longint(real.i64);
                    break;
                case PURC_VARIANT_TYPE_ULONGINT:
                    vrt = purc_variant_make_ulongint(real.u64);
                    break;
                case PURC_VARIANT_TYPE_NUMBER:
                    vrt = purc_variant_make_ulongint(real.d);
                    break;
                case PURC_VARIANT_TYPE_LONGDOUBLE:
                    vrt = purc_variant_make_ulongint(real.ld);
                    break;
                default:
                    assert(0);
                    break;
            }

            if (vrt == PURC_VARIANT_INVALID)
                goto fatal;

            bool ok = purc_variant_array_append(retv, vrt);
            purc_variant_unref(vrt);
            if (!ok) {
                purc_variant_unref(retv);
                goto fatal;
            }

            bytes += real_info[real_id].length;
        }

        return retv;
    }

fatal:
    return PURC_VARIANT_INVALID;
}

purc_variant_t
purc_dvobj_unpack_string(const unsigned char *bytes, size_t nr_bytes,
        size_t *consumed, int format_id, bool silently)
{
    char *str = NULL;
    size_t len;

    switch (format_id) {
    case PURC_K_KW_utf8:
        {
            purc_variant_t retv;
            retv = purc_variant_make_string_ex((const char *)bytes, nr_bytes,
                !silently);
            if (retv) {
                purc_variant_string_bytes(retv, consumed);
            }
            return retv;
        }

    case PURC_K_KW_utf16:
        str = pcutils_string_decode_utf16(bytes, nr_bytes, &len, consumed,
                silently);
        break;

    case PURC_K_KW_utf32:
        str = pcutils_string_decode_utf32(bytes, nr_bytes, &len, consumed,
                silently);
        break;

    case PURC_K_KW_utf16le:
        str = pcutils_string_decode_utf16le(bytes, nr_bytes, &len, consumed,
                silently);
        break;

    case PURC_K_KW_utf32le:
        str = pcutils_string_decode_utf32le(bytes, nr_bytes, &len, consumed,
                silently);
        break;

    case PURC_K_KW_utf16be:
        str = pcutils_string_decode_utf16be(bytes, nr_bytes, &len, consumed,
                silently);
        break;

    case PURC_K_KW_utf32be:
        str = pcutils_string_decode_utf32be(bytes, nr_bytes, &len, consumed,
                silently);
        break;

    default:
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (str == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }
    else if (str == (char *)-1) {
        purc_set_error(PURC_ERROR_BAD_ENCODING);
        goto failed;
    }

    return purc_variant_make_string_reuse_buff(str, len, !silently);

failed:
    if (silently)
        return purc_variant_make_string_static("", false);
    return purc_variant_make_undefined();

fatal:
    return PURC_VARIANT_INVALID;
}

purc_variant_t
purc_dvobj_unpack_bytes(const uint8_t *bytes, size_t nr_bytes,
        const char *formats, size_t formats_left, bool silently)
{
    purc_variant_t retv = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    purc_variant_t item = PURC_VARIANT_INVALID;

    if (retv == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    do {
        const char *format;
        size_t format_len, consumed;

        format = pcutils_get_next_token_len(formats, formats_left,
                _KW_DELIMITERS, &format_len);
        if (format == NULL) {
            break;
        }

        formats += format_len;
        formats_left -= format_len;

        int format_id;
        size_t quantity;
        format_id = purc_dvobj_parse_format(format, format_len, &quantity);
        if (format_id < 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        if (format_id >= PURC_K_KW_i8 && format_id <= PURC_K_KW_f128be) {

            if (quantity == 0)
                quantity = 1;

            int real_id = format_id - PURC_K_KW_i8;
            consumed = real_info[real_id].length * quantity;
            if (consumed > nr_bytes) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            item = purc_dvobj_unpack_real(bytes, nr_bytes, format_id, quantity);
        }
        else if (format_id == PURC_K_KW_bytes) {
            if (quantity == 0 || quantity > nr_bytes) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            item = purc_variant_make_byte_sequence(bytes, quantity);
            consumed = quantity;
        }
        else if (format_id == PURC_K_KW_padding) {
            if (quantity == 0 || quantity > nr_bytes) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            item = purc_variant_make_undefined();
            consumed = quantity;
        }
        else if (format_id >= PURC_K_KW_utf8 &&
                format_id <= PURC_K_KW_utf32be) {

            if (quantity > nr_bytes) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            if (quantity == 0)
                quantity = nr_bytes;

            item = purc_dvobj_unpack_string(bytes, quantity, &consumed,
                    format_id, silently);
        }

        if (item == PURC_VARIANT_INVALID) {
            goto fatal;
        }
        else if (purc_variant_is_undefined(item)) {
            purc_variant_unref(item);
            goto failed;
        }
        else if (!purc_variant_array_append(retv, item)) {
            goto fatal;
        }
        purc_variant_unref(item);

        if (consumed >= nr_bytes)
            break;

        bytes += consumed;
        nr_bytes -= consumed;

    } while (true);

    /* if there is only one member, return the member instead of the array */
    if (purc_variant_array_get_size(retv) == 1) {
        item = purc_variant_ref(item);
        purc_variant_unref(retv);
        return item;
    }
    return retv;

failed:
    if (silently)
        return retv;

fatal:
    if (item)
        purc_variant_unref(item);
    if (retv)
        purc_variant_unref(retv);

    return PURC_VARIANT_INVALID;
}

static
const uint8_t *rwstream_read_bytes(purc_rwstream_t in, purc_rwstream_t buff,
        size_t count, size_t *nr_read)
{
    purc_rwstream_seek(buff, 0L, SEEK_SET);
    ssize_t nr = purc_rwstream_dump_to_another(in, buff, count);
    if (nr == -1) {
        if (nr_read) {
            *nr_read = 0;
        }
        return NULL;
    }
    if (nr_read) {
        *nr_read = nr;
    }
    return purc_rwstream_get_mem_buffer(buff, NULL);
}

static
const uint8_t *rwstream_read_string(purc_rwstream_t in, purc_rwstream_t buff,
        int format_id, size_t *nr_read)
{
    UNUSED_PARAM(format_id);
    int nr_null = 0;
    switch (format_id) {
    case PURC_K_KW_utf8:
        nr_null = 1;
        break;
    case PURC_K_KW_utf16:
    case PURC_K_KW_utf16le:
    case PURC_K_KW_utf16be:
        nr_null = 2;
        break;
    case PURC_K_KW_utf32:
    case PURC_K_KW_utf32le:
    case PURC_K_KW_utf32be:
        nr_null = 4;
        break;
    }
    purc_rwstream_seek(buff, 0L, SEEK_SET);
    int read_len = 0;
    int nr_write = 0;
    uint32_t uc = 0;
    while ((read_len = purc_rwstream_read(in, &uc, nr_null)) > 0) {
        nr_write += purc_rwstream_write(buff, &uc, read_len);
        if (uc == 0) {
            break;
        }
    }
    if (nr_read) {
        *nr_read = nr_write;
    }
    return purc_rwstream_get_mem_buffer(buff, NULL);
}

purc_variant_t
purc_dvobj_read_struct(purc_rwstream_t stream,
        const char *formats, size_t formats_left, bool silently)
{
    const uint8_t *bytes = NULL;
    size_t nr_bytes = 0;
    purc_rwstream_t rws = NULL;
    purc_variant_t retv = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    purc_variant_t item = PURC_VARIANT_INVALID;

    if (retv == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    rws = purc_rwstream_new_buffer(LEN_INI_SERIALIZE_BUF,
            LEN_MAX_SERIALIZE_BUF);
    if (rws == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    do {
        const char *format;
        size_t format_len, consumed;

        format = pcutils_get_next_token_len(formats, formats_left,
                _KW_DELIMITERS, &format_len);
        if (format == NULL) {
            break;
        }

        formats += format_len;
        formats_left -= format_len;

        int format_id;
        size_t quantity;
        format_id = purc_dvobj_parse_format(format, format_len, &quantity);
        if (format_id < 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        if (format_id >= PURC_K_KW_i8 && format_id <= PURC_K_KW_f128be) {

            if (quantity == 0)
                quantity = 1;

            int real_id = format_id - PURC_K_KW_i8;
            consumed = real_info[real_id].length * quantity;

            bytes = rwstream_read_bytes(stream, rws, consumed, &nr_bytes);
            if (consumed > nr_bytes) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            item = purc_dvobj_unpack_real(bytes, nr_bytes, format_id, quantity);
        }
        else if (format_id == PURC_K_KW_bytes) {
            if (quantity == 0) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            bytes = rwstream_read_bytes(stream, rws, quantity, &nr_bytes);
            if (bytes == NULL ||  quantity > nr_bytes) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            item = purc_variant_make_byte_sequence(bytes, quantity);
            consumed = quantity;
        }
        else if (format_id == PURC_K_KW_padding) {
            if (quantity == 0) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            bytes = rwstream_read_bytes(stream, rws, quantity, &nr_bytes);
            if (bytes == NULL ||  quantity > nr_bytes) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            item = purc_variant_make_undefined();
            consumed = quantity;
        }
        else if (format_id >= PURC_K_KW_utf8 &&
                format_id <= PURC_K_KW_utf32be) {

            if (quantity > 0) {
                bytes = rwstream_read_bytes(stream, rws, quantity, &nr_bytes);
                if (quantity > nr_bytes) {
                    purc_set_error(PURC_ERROR_INVALID_VALUE);
                    goto failed;
                }
            }
            else if (quantity == 0) {
                bytes = rwstream_read_string(stream, rws, format_id, &nr_bytes);
                if (nr_bytes == 0) {
                    purc_set_error(PURC_ERROR_INVALID_VALUE);
                    goto failed;
                }
                quantity = nr_bytes;
            }

            item = purc_dvobj_unpack_string(bytes, quantity, &consumed,
                    format_id, silently);
        }

        if (item == PURC_VARIANT_INVALID) {
            goto fatal;
        }
        else if (purc_variant_is_undefined(item)) {
            purc_variant_unref(item);
            goto failed;
        }
        else if (!purc_variant_array_append(retv, item)) {
            goto fatal;
        }
        purc_variant_unref(item);
    } while (true);

    if (rws) {
        purc_rwstream_destroy(rws);
        rws = NULL;
    }

    /* if there is only one member, return the member instead of the array */
    if (purc_variant_array_get_size(retv) == 1) {
        item = purc_variant_ref(item);
        purc_variant_unref(retv);
        return item;
    }
    return retv;

failed:
    if (silently) {
        if (rws) {
            purc_rwstream_destroy(rws);
        }
        return retv;
    }

fatal:
    if (item)
        purc_variant_unref(item);
    if (retv)
        purc_variant_unref(retv);
    if (rws)
        purc_rwstream_destroy(rws);

    return PURC_VARIANT_INVALID;
}

int
purc_dvobj_pack_real(struct pcdvobj_bytes_buff *bf, purc_variant_t item,
        int format_id, size_t quantity, bool silently)
{
    if (quantity == 0)
        quantity = 1;

    int real_id = format_id - PURC_K_KW_i8;
    bf->sz_allocated += real_info[real_id].length * quantity;
    bf->bytes = realloc(bf->bytes, bf->sz_allocated);
    if (bf->bytes == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    enum purc_variant_type vt = purc_variant_get_type(item);
    bool is_linear_container = ((vt == PURC_VARIANT_TYPE_ARRAY) ||
            (vt == PURC_VARIANT_TYPE_SET) || (vt == PURC_VARIANT_TYPE_TUPLE));
    for (size_t n = 0; n < quantity; n++) {
        purc_variant_t real_item;

        if (is_linear_container) {
            real_item = purc_variant_linear_container_get(item, n);
            if (real_item == PURC_VARIANT_INVALID) {
                purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
                goto failed;
            }
        }
        else {
            // just repeat the item.
            real_item = item;
        }

        purc_real_t real;
        bool ret;
        switch (real_info[real_id].real_type) {
            case PURC_VARIANT_TYPE_LONGINT:
                ret = purc_variant_cast_to_longint(real_item,
                        &real.i64, false);
                break;
            case PURC_VARIANT_TYPE_ULONGINT:
                ret = purc_variant_cast_to_ulongint(real_item,
                        &real.u64, false);
                break;
            case PURC_VARIANT_TYPE_NUMBER:
                ret = purc_variant_cast_to_number(real_item,
                        &real.d, false);
                break;
            case PURC_VARIANT_TYPE_LONGDOUBLE:
                ret = purc_variant_cast_to_longdouble(real_item,
                        &real.ld, false);
                break;
            default:
                ret = false;
                break;
        }

        if (!ret) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        if (!real_info[real_id].dumper(bf->bytes + bf->nr_bytes, real,
                    silently)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        bf->nr_bytes += real_info[real_id].length;
    }

    return 0;

failed:
    return -1;
}

int
purc_dvobj_pack_string(struct pcdvobj_bytes_buff *bf, purc_variant_t item,
        int format_id, size_t length)
{
    const char *this_str;
    size_t len_this, nr_chars;

    this_str = purc_variant_get_string_const_ex(item,
            &len_this);
    if (this_str == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    fn_encode_str encoder = NULL;
    purc_variant_string_chars(item, &nr_chars);
    switch (format_id) {
        case PURC_K_KW_utf8:
            encoder = dump_utf8_string;
            if (length == 0)
                length = len_this + 1;
            break;

        case PURC_K_KW_utf16:
            encoder = pcutils_string_encode_utf16;
            if (length == 0)
                length = (nr_chars + 1) * 2;
            break;

        case PURC_K_KW_utf16le:
            encoder = pcutils_string_encode_utf16le;
            if (length == 0)
                length = (nr_chars + 1) * 2;
            break;

        case PURC_K_KW_utf16be:
            encoder = pcutils_string_encode_utf16be;
            if (length == 0)
                length = (nr_chars + 1) * 2;
            break;

        case PURC_K_KW_utf32:
            encoder = pcutils_string_encode_utf32;
            if (length == 0)
                length = (nr_chars + 1) * 4;
            break;

        case PURC_K_KW_utf32le:
            encoder = pcutils_string_encode_utf32le;
            if (length == 0)
                length = (nr_chars + 1) * 4;
            break;

        case PURC_K_KW_utf32be:
            encoder = pcutils_string_encode_utf32be;
            if (length == 0)
                length = (nr_chars + 1) * 4;
            break;

        default:
            break;
    }

    if (encoder == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    bf->sz_allocated += length;
    bf->bytes = realloc(bf->bytes, bf->sz_allocated);
    if (bf->bytes == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    bf->nr_bytes += encoder(this_str, len_this, nr_chars,
            bf->bytes + bf->nr_bytes, length);
    return 0;

failed:
    return -1;
}

int
purc_dvobj_pack_variants(struct pcdvobj_bytes_buff *bf,
        purc_variant_t *argv, size_t nr_args,
        const char *formats, size_t formats_left, bool silently)
{
    size_t item_idx = 0, nr_items;
    bool items_in_linear_container = (nr_args == 1) &&
        purc_variant_linear_container_size(argv[0], &nr_items);

    if (!items_in_linear_container) {
        nr_items = nr_args;
    }

    do {
        const char *format;
        size_t format_len;

        format = pcutils_get_next_token_len(formats, formats_left,
                _KW_DELIMITERS, &format_len);
        if (format == NULL) {
            break;
        }

        formats += format_len;
        formats_left -= format_len;

        if (item_idx >= nr_items) {
            purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
            goto failed;
        }

        purc_variant_t item;
        if (items_in_linear_container) {
            item = purc_variant_linear_container_get(argv[0], item_idx);
            assert(item);   // item must be valid
        }
        else {
            item = argv[item_idx];
        }
        item_idx++;

        size_t quantity;
        int format_id = purc_dvobj_parse_format(format, format_len, &quantity);
        if (format_id < 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        if (format_id >= PURC_K_KW_i8 && format_id <= PURC_K_KW_f128be) {

            if (quantity == 0)
                quantity = 1;

            if (purc_dvobj_pack_real(bf, item, format_id, quantity,
                        silently)) {
                goto failed;
            }
        }
        else if (format_id == PURC_K_KW_bytes) {
            const unsigned char *this_bytes;
            size_t nr_this;
            this_bytes = purc_variant_get_bytes_const(item, &nr_this);

            if (this_bytes == NULL) {
                purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                goto failed;
            }

            if ((size_t)quantity > nr_this) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            if (quantity == 0) {
                quantity = nr_this;
            }

            bf->sz_allocated += quantity;
            bf->bytes = realloc(bf->bytes, bf->sz_allocated);
            if (bf->bytes == NULL) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                goto failed;
            }

            memcpy(bf->bytes + bf->nr_bytes, this_bytes, quantity);
            bf->nr_bytes += quantity;
        }
        else if (format_id == PURC_K_KW_padding) {
            bf->sz_allocated += quantity;
            bf->bytes = realloc(bf->bytes, bf->sz_allocated);
            if (bf->bytes == NULL) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                goto failed;
            }

            memset(bf->bytes + bf->nr_bytes, 0, quantity);
            bf->nr_bytes += quantity;
        }
        else if (format_id >= PURC_K_KW_utf8 &&
                format_id <= PURC_K_KW_utf32be) {

            if (purc_dvobj_pack_string(bf, item, format_id, quantity)) {
                goto failed;
            }
        }

    } while (true);

    return 0;

failed:
    return -1;
}

static purc_variant_t
pack_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    bool silently = call_flags & PCVRT_CALL_FLAG_SILENTLY;
    struct pcdvobj_bytes_buff bf = { NULL, 0, 0 };

    const char *formats = NULL;
    size_t formats_left = 0;
    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    formats = purc_variant_get_string_const_ex(argv[0], &formats_left);
    if (formats == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    formats = pcutils_trim_spaces(formats, &formats_left);
    if (formats_left == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (purc_dvobj_pack_variants(&bf, argv + 1, nr_args - 1, formats,
                formats_left, (call_flags & PCVRT_CALL_FLAG_SILENTLY))) {
        if (bf.bytes == NULL)
            goto fatal;

        goto failed;
    }

    silently = true;    // fall through

failed:
    if (silently) {
        if (bf.bytes)
            return purc_variant_make_byte_sequence_reuse_buff(bf.bytes,
                    bf.nr_bytes, bf.sz_allocated);
        return purc_variant_make_byte_sequence_empty();
    }

fatal:
    if (bf.bytes)
        free(bf.bytes);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
unpack_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *formats = NULL;
    size_t formats_left = 0;
    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    formats = purc_variant_get_string_const_ex(argv[0], &formats_left);
    if (formats == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    formats = pcutils_trim_spaces(formats, &formats_left);
    if (formats_left == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    const unsigned char *bytes = NULL;
    size_t nr_bytes = 0;
    bytes = purc_variant_get_bytes_const(argv[1], &nr_bytes);
    if (nr_bytes > 0) {
        return purc_dvobj_unpack_bytes(bytes, nr_bytes,
                formats, formats_left,
                (call_flags & PCVRT_CALL_FLAG_SILENTLY));
    }
    else {
        return purc_variant_make_array(0, PURC_VARIANT_INVALID);
    }

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_array(0, PURC_VARIANT_INVALID);
    }

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
shuffle_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (purc_variant_is_array(argv[0])) {
        ssize_t sz = purc_variant_array_get_size(argv[0]);

        if (sz > 1) {
            struct pcutils_array_list *al = variant_array_get_data(argv[0]);
            struct pcutils_array_list_node *p;
            array_list_for_each(al, p) {

                size_t new_idx;
                if (sz < RAND_MAX) {
                    new_idx = (size_t)pcdvobjs_get_random() % sz;
                }
                else {
                    new_idx = (size_t)pcdvobjs_get_random();
                    new_idx = new_idx * sz / RAND_MAX;
                }

                if (new_idx != p->idx)
                    pcutils_array_list_swap(al, p->idx, new_idx);
            }
        }
    }
    else if (purc_variant_is_set(argv[0])) {
        ssize_t sz = purc_variant_set_get_size(argv[0]);

        if (sz > 1) {
            variant_set_t data = (variant_set_t)argv[0]->sz_ptr[1];
            struct pcutils_array_list *al;
            al = &data->al;
            size_t nr = pcutils_array_list_length(al);

            for (size_t idx = 0; idx < nr; idx++) {

                size_t new_idx;
                if (sz < RAND_MAX) {
                    new_idx = (size_t)pcdvobjs_get_random() % sz;
                }
                else {
                    new_idx = (size_t)pcdvobjs_get_random();
                    new_idx = new_idx * sz / RAND_MAX;
                }

                if (new_idx != idx)
                    pcutils_array_list_swap(al, idx, new_idx);
            }
        }
    }
    else {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    return purc_variant_ref(argv[0]);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

#if 0
struct sort_opt {
    pcvrnt_compare_method_k method;
    bool asc;
    pcutils_map *map;
};

static int my_array_sort (purc_variant_t v1, purc_variant_t v2, void *ud)
{
    int ret = 0;
    char *p1 = NULL;
    char *p2 = NULL;
    pcutils_map_entry *entry = NULL;

    struct sort_opt *sort_arg = (struct sort_opt *)ud;
    entry = pcutils_map_find(sort_arg->map, v1);
    p1 = (char *)entry->val;
    entry = NULL;
    entry = pcutils_map_find(sort_arg->map, v2);
    p2 = (char *)entry->val;

    if (sort_arg->method == PURC_K_KW_caseless)
        ret = pcutils_strcasecmp(p1, p2);
    else
        ret = strcmp(p1, p2);

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

    sort_arg.map = pcutils_map_create(map_copy_key, map_free_key,
            map_copy_val, map_free_val, map_comp_key, false);

    pcutils_map_destroy(sort_arg.map);
#endif

static purc_variant_t
sort_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_variant_t val = PURC_VARIANT_INVALID;
    size_t totalsize = 0;

    uintptr_t sort_opt = PCVRNT_SORT_ASC;

    if (nr_args == 0) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (purc_variant_is_array(argv[0])) {
        totalsize = purc_variant_array_get_size(argv[0]);
        if (totalsize > 1) {
            val = purc_variant_array_get(argv[0], 0);
        }
    }
    else if (purc_variant_is_set(argv[0])) {
        totalsize = purc_variant_set_get_size(argv[0]);
        if (totalsize > 1) {
            val = purc_variant_set_get_by_index(argv[0], 0);
        }
    }
    else {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (totalsize < 2) {
        // no need to sort
        goto done;
    }

    // get sort order: asc, desc
    if (nr_args >= 2) {
        const char *order;
        size_t order_len;
        order = purc_variant_get_string_const_ex(argv[1], &order_len);
        if (order == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        order = pcutils_trim_spaces(order, &order_len);
        if (order_len == 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        int order_id = pcdvobjs_global_keyword_id(order, order_len);
        if (order_id == PURC_K_KW_asc) {
            sort_opt = PCVRNT_SORT_ASC;
        }
        else if (order_id == PURC_K_KW_desc) {
            sort_opt = PCVRNT_SORT_DESC;
        }
        else {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    // get sort option: auto, number, case, caseless
    if (nr_args >= 3) {
        const char *option;
        size_t option_len;
        option = purc_variant_get_string_const_ex(argv[2], &option_len);
        if (option == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        option = pcutils_trim_spaces(option, &option_len);
        if (option_len == 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        int option_id = pcdvobjs_global_keyword_id(option, option_len);
        if (option_id == PURC_K_KW_auto) {
            double number;
            if (purc_variant_cast_to_number(val, &number, false))
                sort_opt |= PCVRNT_COMPARE_METHOD_NUMBER;
            else
                sort_opt |= PCVRNT_COMPARE_METHOD_CASE;
        }
        else if (option_id == PURC_K_KW_number) {
            sort_opt |= PCVRNT_COMPARE_METHOD_NUMBER;
        }
        else if (option_id == PURC_K_KW_case) {
            sort_opt |= PCVRNT_COMPARE_METHOD_CASE;
        }
        else if (option_id == PURC_K_KW_caseless) {
            sort_opt |= PCVRNT_COMPARE_METHOD_CASELESS;
        }
        else {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    /* use the default variant comparison function */
    if (purc_variant_is_array(argv[0])) {
        pcvariant_array_sort(argv[0], (void *)sort_opt, NULL);
    }
    else {
        pcvariant_set_sort(argv[0], (void *)sort_opt, NULL);
    }

done:
    return purc_variant_ref(argv[0]);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static struct crc32algo_to_atom {
    const char *    algo;
    purc_atom_t     atom;
} crc32algo2atoms[] = {
    { PURC_ALGO_CRC32,          0 }, // "CRC-32"
    { PURC_ALGO_CRC32_BZIP2,    0 }, // "CRC-32/BZIP2"
    { PURC_ALGO_CRC32_MPEG2,    0 }, // "CRC-32/MPEG-2"
    { PURC_ALGO_CRC32_POSIX,    0 }, // "CRC-32/POSIX"
    { PURC_ALGO_CRC32_XFER,     0 }, // "CRC-32/XFER"
    { PURC_ALGO_CRC32_ISCSI,    0 }, // "CRC-32/ISCSI"
    { PURC_ALGO_CRC32C,         0 }, // "CRC-32C"
    { PURC_ALGO_CRC32_BASE91_D, 0 }, // "CRC-32/BASE91-D"
    { PURC_ALGO_CRC32D,         0 }, // "CRC-32D"
    { PURC_ALGO_CRC32_JAMCRC,   0 }, // "CRC-32/JAMCRC"
    { PURC_ALGO_CRC32_AIXM,     0 }, // "CRC-32/AIXM"
    { PURC_ALGO_CRC32Q,         0 }, // "CRC-32Q"
};

static ssize_t cb_calc_crc32(void *ctxt, const void *buf, size_t count)
{
    pcutils_crc32_update(ctxt, buf, count);
    return count;
}

static purc_variant_t
crc32_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_rwstream_t stream = NULL;

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    purc_crc32_algo_t algo = PURC_K_ALGO_CRC32_UNKNOWN;
    if (nr_args == 1 || purc_variant_is_null(argv[1])) {
        algo = PURC_K_ALGO_CRC32;
    }
    else {
        const char *option;
        size_t option_len;
        option = purc_variant_get_string_const_ex(argv[1], &option_len);
        if (option == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        option = pcutils_trim_spaces(option, &option_len);
        if (option_len == 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        purc_atom_t atom;
        char tmp[option_len + 1];
        strncpy(tmp, option, option_len);
        tmp[option_len]= '\0';
        atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);

        for (size_t i = 0; i < PCA_TABLESIZE(crc32algo2atoms); i++) {
            if (atom == crc32algo2atoms[i].atom) {
                algo = PURC_K_ALGO_CRC32 + i;
                break;
            }
        }

        if (algo == PURC_K_ALGO_CRC32_UNKNOWN) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    int ret_type = PURC_K_KW_ulongint;
    if (nr_args > 2) {
        const char *option;
        size_t option_len;
        option = purc_variant_get_string_const_ex(argv[2], &option_len);
        if (option == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        option = pcutils_trim_spaces(option, &option_len);
        if (option_len == 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        ret_type = pcdvobjs_global_keyword_id(option, option_len);
    }

    pcutils_crc32_ctxt ctxt;
    stream = purc_rwstream_new_for_dump(&ctxt, cb_calc_crc32);
    if (stream == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    pcutils_crc32_begin(&ctxt, algo);

    if (purc_variant_stringify(stream, argv[0],
            PCVRNT_STRINGIFY_OPT_BSEQUENCE_BAREBYTES, NULL) < 0) {
        goto fatal;
    }

    purc_rwstream_destroy(stream);

    uint32_t crc32;
    pcutils_crc32_end(&ctxt, &crc32);
    purc_log_info("%08x\n", crc32);

    switch (ret_type) {
        case PURC_K_KW_ulongint:
        default:
            return purc_variant_make_ulongint((uint64_t)crc32);
        case PURC_K_KW_binary:
            return purc_variant_make_byte_sequence(&crc32, sizeof(crc32));
        case PURC_K_KW_uppercase:
        case PURC_K_KW_lowercase:
        {
            char hex[sizeof(crc32) * 2 + 1];
            pcutils_bin2hex((unsigned char *)&crc32, sizeof(crc32), hex,
                    ret_type == PURC_K_KW_uppercase);
            return purc_variant_make_string(hex, false);
        }
    }

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

fatal:
    if (stream)
        purc_rwstream_destroy(stream);
    return PURC_VARIANT_INVALID;
}

static ssize_t cb_calc_md5(void *ctxt, const void *buf, size_t count)
{
    pcutils_md5_ctxt *md5_ctxt = ctxt;
    pcutils_md5_hash(md5_ctxt, buf, count);
    return count;
}

static purc_variant_t
md5_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_rwstream_t stream = NULL;

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    int ret_type = PURC_K_KW_binary;
    if (nr_args > 1) {
        const char *option;
        size_t option_len;
        option = purc_variant_get_string_const_ex(argv[1], &option_len);
        if (option == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        option = pcutils_trim_spaces(option, &option_len);
        if (option_len == 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        ret_type = pcdvobjs_global_keyword_id(option, option_len);
    }

    pcutils_md5_ctxt md5_ctxt;
    stream = purc_rwstream_new_for_dump(&md5_ctxt, cb_calc_md5);
    if (stream == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    pcutils_md5_begin(&md5_ctxt);
    if (purc_variant_stringify(stream, argv[0],
            PCVRNT_STRINGIFY_OPT_BSEQUENCE_BAREBYTES, NULL) < 0) {
        goto fatal;
    }

    purc_rwstream_destroy(stream);

    unsigned char md5[PCUTILS_MD5_DIGEST_SIZE];
    pcutils_md5_end(&md5_ctxt, md5);

    switch (ret_type) {
        case PURC_K_KW_binary:  // fallthrough
        default:
            return purc_variant_make_byte_sequence(md5, sizeof(md5));
        case PURC_K_KW_uppercase:
        case PURC_K_KW_lowercase:
        {
            char hex[sizeof(md5) * 2 + 1];
            pcutils_bin2hex(md5, sizeof(md5), hex,
                    ret_type == PURC_K_KW_uppercase);
            return purc_variant_make_string(hex, false);
        }
    }

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

fatal:
    if (stream)
        purc_rwstream_destroy(stream);
    return PURC_VARIANT_INVALID;
}

static ssize_t cb_calc_sha1(void *ctxt, const void *buf, size_t count)
{
    pcutils_sha1_ctxt *sha1_ctxt = ctxt;
    pcutils_sha1_hash(sha1_ctxt, buf, count);
    return count;
}

static purc_variant_t
sha1_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_rwstream_t stream = NULL;

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    int ret_type = PURC_K_KW_binary;
    if (nr_args > 1) {
        const char *option;
        size_t option_len;
        option = purc_variant_get_string_const_ex(argv[1], &option_len);
        if (option == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        option = pcutils_trim_spaces(option, &option_len);
        if (option_len == 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        ret_type = pcdvobjs_global_keyword_id(option, option_len);
    }

    pcutils_sha1_ctxt sha1_ctxt;
    stream = purc_rwstream_new_for_dump(&sha1_ctxt, cb_calc_sha1);
    if (stream == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    pcutils_sha1_begin(&sha1_ctxt);
    if (purc_variant_stringify(stream, argv[0],
            PCVRNT_STRINGIFY_OPT_BSEQUENCE_BAREBYTES, NULL) < 0) {
        goto fatal;
    }

    purc_rwstream_destroy(stream);

    unsigned char sha1[PCUTILS_SHA1_DIGEST_SIZE];
    pcutils_sha1_end(&sha1_ctxt, sha1);

    switch (ret_type) {
        case PURC_K_KW_binary:  // fallthrough
        default:
            return purc_variant_make_byte_sequence(sha1, sizeof(sha1));

        case PURC_K_KW_uppercase:
        case PURC_K_KW_lowercase:
        {
            char hex[sizeof(sha1) * 2 + 1];
            pcutils_bin2hex(sha1, sizeof(sha1), hex,
                    ret_type == PURC_K_KW_uppercase);
            return purc_variant_make_string(hex, false);
        }
    }

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

fatal:
    if (stream)
        purc_rwstream_destroy(stream);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
bin2hex_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const unsigned char *bytes = NULL;
    size_t nr_bytes = 0;

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (purc_variant_is_string(argv[0])) {
        bytes = (const unsigned char *)
            purc_variant_get_string_const_ex(argv[0], &nr_bytes);
    }
    else if (purc_variant_is_bsequence(argv[0])) {
        bytes = purc_variant_get_bytes_const(argv[0], &nr_bytes);
    }

    if (bytes == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (nr_bytes == 0) {
        return purc_variant_make_string_static("", false);
    }

    int opt_case = PURC_K_KW_lowercase;
    if (nr_args > 1) {
        const char *option;
        size_t option_len;
        option = purc_variant_get_string_const_ex(argv[1], &option_len);
        if (option == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        option = pcutils_trim_spaces(option, &option_len);
        if (option_len == 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        opt_case = pcdvobjs_global_keyword_id(option, option_len);
        if (opt_case != PURC_K_KW_lowercase &&
                opt_case != PURC_K_KW_uppercase) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    char *buff = malloc(nr_bytes * 2 + 1);
    if (buff == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    pcutils_bin2hex(bytes, nr_bytes, buff, opt_case == PURC_K_KW_uppercase);

    return purc_variant_make_string_reuse_buff(buff, nr_bytes * 2 + 1, false);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_string_static("", false);

fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
hex2bin_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *string = NULL;
    size_t len = 0;

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    string = purc_variant_get_string_const_ex(argv[0], &len);
    if (string == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (len < 2) {
        purc_set_error(PURC_ERROR_BAD_ENCODING);
        goto failed;
    }

    size_t expected = len / 2;
    unsigned char *bytes = malloc(expected+1);
    if (bytes == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    size_t converted;
    if (pcutils_hex2bin(string, bytes, &converted) < 0 ||
            converted < expected) {
        free(bytes);
        purc_set_error(PURC_ERROR_BAD_ENCODING);
        goto failed;
    }

    return purc_variant_make_byte_sequence_reuse_buff(bytes, converted,
            expected);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_undefined();
    }

fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
base64_encode_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const unsigned char *bytes = NULL;
    size_t nr_bytes = 0;

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (purc_variant_is_string(argv[0])) {
        bytes = (const unsigned char *)
            purc_variant_get_string_const_ex(argv[0], &nr_bytes);
    }
    else if (purc_variant_is_bsequence(argv[0])) {
        bytes = purc_variant_get_bytes_const(argv[0], &nr_bytes);
    }

    if (bytes == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (nr_bytes == 0) {
        return purc_variant_make_string_static("", false);
    }

    size_t sz_buff = pcutils_b64_encoded_length(nr_bytes);
    char *buff = malloc(sz_buff);
    if (buff == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    pcutils_b64_encode(bytes, nr_bytes, buff, sz_buff);

    return purc_variant_make_string_reuse_buff(buff, sz_buff, false);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_string_static("", false);

fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
base64_decode_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *string = NULL;
    size_t len = 0;

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    string = purc_variant_get_string_const_ex(argv[0], &len);
    if (string == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (len < 4) {
        purc_set_error(PURC_ERROR_BAD_ENCODING);
        goto failed;
    }

    size_t expected = pcutils_b64_decoded_length(len);
    unsigned char *bytes = malloc(expected);
    if (bytes == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    ssize_t converted = pcutils_b64_decode(string, bytes, expected);
    if (converted < 0) {
        free(bytes);
        purc_set_error(PURC_ERROR_BAD_ENCODING);
        goto failed;
    }

    return purc_variant_make_byte_sequence_reuse_buff(bytes, converted,
            expected);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_byte_sequence_empty();
    }

fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
isdivisible_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    int64_t l_operand, r_operand;
    if (!purc_variant_cast_to_longint(argv[0], &l_operand, true) ||
            !purc_variant_cast_to_longint(argv[1], &r_operand, true)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (r_operand == 0) {
        purc_set_error(PURC_ERROR_DIVBYZERO);
        goto failed;
    }

    bool ret = (l_operand % r_operand == 0);
    return purc_variant_make_boolean(ret);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

enum {
    RETURNS_INDEXES = 0,
    RETURNS_VALUES,
    RETURNS_KEYS,
    RETURNS_KV_PAIRS
};

enum {
    MATCHING_EXACT = PCVRNT_COMPARE_METHOD_CASELESS + 1,
    MATCHING_WILDCARD,
    MATCHING_REGEXP,
};

static purc_variant_t
match_members_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *options = NULL;
    size_t options_len;
    int matching = MATCHING_EXACT;
    int returns = RETURNS_INDEXES;
    size_t sz;

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (!purc_variant_linear_container_size(argv[0], &sz)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (nr_args >= 3) {
        options = purc_variant_get_string_const_ex(argv[2], &options_len);
        if (options) {
            options = pcutils_trim_spaces(options, &options_len);
            if (options_len == 0) {
                options = NULL;
            }
        }
    }

    if (options) {
        size_t opt_len = 0;
        const char *option = pcutils_get_next_token_len(options, options_len,
                _KW_DELIMITERS, &opt_len);

        do {

            if (opt_len == 0 || opt_len > MAX_LEN_KEYWORD) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            int cmp_id = pcdvobjs_global_keyword_id(option, opt_len);
            switch (cmp_id) {
            case PURC_K_KW_exact:
                matching = MATCHING_EXACT;
                break;

            case PURC_K_KW_auto:
                matching = PCVRNT_COMPARE_METHOD_AUTO;
                break;

            case PURC_K_KW_number:
                matching = PCVRNT_COMPARE_METHOD_NUMBER;
                break;

            case PURC_K_KW_caseless:
                matching = PCVRNT_COMPARE_METHOD_CASELESS;
                break;

            case PURC_K_KW_wildcard:
                matching = MATCHING_WILDCARD;
                break;

            case PURC_K_KW_regexp:
                matching = MATCHING_REGEXP;
                break;

            case PURC_K_KW_indexes:
                returns = RETURNS_INDEXES;
                break;

            case PURC_K_KW_values:
                returns = RETURNS_VALUES;
                break;

            default:
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
                break;
            }

            if (options_len <= opt_len)
                break;

            options_len -= opt_len;
            option = pcutils_get_next_token_len(option + opt_len, options_len,
                    _KW_DELIMITERS, &opt_len);
        } while (option);
    }

    return purc_variant_make_array_0();

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
match_properties_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *options = NULL;
    size_t options_len;
    int matching = MATCHING_EXACT;
    int returns = RETURNS_KEYS;

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (!purc_variant_is_object(argv[0])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

#if 0
    struct pcvrnt_object_iterator *it;
    it = pcvrnt_object_iterator_create_begin(argv[0]);
#endif

    if (nr_args >= 3) {
        options = purc_variant_get_string_const_ex(argv[2], &options_len);
        if (options) {
            options = pcutils_trim_spaces(options, &options_len);
            if (options_len == 0) {
                options = NULL;
            }
        }
    }

    if (options) {
        size_t opt_len = 0;
        const char *option = pcutils_get_next_token_len(options, options_len,
                _KW_DELIMITERS, &opt_len);

        do {

            if (opt_len == 0 || opt_len > MAX_LEN_KEYWORD) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            int cmp_id = pcdvobjs_global_keyword_id(option, opt_len);
            switch (cmp_id) {
            case PURC_K_KW_exact:
                matching = MATCHING_EXACT;
                break;

            case PURC_K_KW_auto:
                matching = PCVRNT_COMPARE_METHOD_AUTO;
                break;

            case PURC_K_KW_number:
                matching = PCVRNT_COMPARE_METHOD_NUMBER;
                break;

            case PURC_K_KW_caseless:
                matching = PCVRNT_COMPARE_METHOD_CASELESS;
                break;

            case PURC_K_KW_wildcard:
                matching = MATCHING_WILDCARD;
                break;

            case PURC_K_KW_regexp:
                matching = MATCHING_REGEXP;
                break;

            case PURC_K_KW_keys:
                returns = RETURNS_KEYS;
                break;

            case PURC_K_KW_values:
                returns = RETURNS_VALUES;
                break;

            case PURC_K_KW_kv_pairs:
                returns = RETURNS_KV_PAIRS;
                break;

            default:
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
                break;
            }

            if (options_len <= opt_len)
                break;

            options_len -= opt_len;
            option = pcutils_get_next_token_len(option + opt_len, options_len,
                    _KW_DELIMITERS, &opt_len);
        } while (option);
    }

    return purc_variant_make_array_0();

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

purc_variant_t purc_dvobj_data_new(void)
{
    static struct purc_dvobj_method method [] = {
        { "type",       type_getter, NULL },
        { "count",      count_getter, NULL },
        { "arith",      arith_getter, NULL },
        { "bitwise",    bitwise_getter, NULL },
        { "numerify",   numerify_getter, NULL },
        { "booleanize", booleanize_getter, NULL },
        { "stringify",  stringify_getter, NULL },
        { "serialize",  serialize_getter, NULL },
        { "parse",      parse_getter, NULL },
        { "isequal",    isequal_getter, NULL },
        { "compare",    compare_getter, NULL },
        { "fetchstr",   fetchstr_getter, NULL },
        { "fetchreal",  fetchreal_getter, NULL },
        { "pack",       pack_getter, NULL },
        { "unpack",     unpack_getter, NULL },
        { "shuffle",    shuffle_getter, NULL },
        { "sort",       sort_getter, NULL },
        { "crc32",      crc32_getter, NULL },
        { "md5",        md5_getter, NULL },
        { "sha1",       sha1_getter, NULL },
        { "bin2hex",    bin2hex_getter, NULL },
        { "hex2bin",    hex2bin_getter, NULL },
        { "base64_encode",  base64_encode_getter, NULL },
        { "base64_decode",  base64_decode_getter, NULL },
        { "isdivisible",    isdivisible_getter, NULL },
        { "match_members",      match_members_getter, NULL },
        { "match_properties",   match_properties_getter, NULL },
    };

    if (keywords2atoms[0].atom == 0) {
        for (size_t i = 0; i < PCA_TABLESIZE(keywords2atoms); i++) {
            keywords2atoms[i].atom =
                purc_atom_from_static_string_ex(ATOM_BUCKET_DVOBJ,
                    keywords2atoms[i].keyword);
        }
    }

    if (crc32algo2atoms[0].atom == 0) {
        for (size_t i = 0; i < PCA_TABLESIZE(crc32algo2atoms); i++) {
            crc32algo2atoms[i].atom =
                purc_atom_from_static_string_ex(ATOM_BUCKET_DVOBJ,
                    crc32algo2atoms[i].algo);
        }
    }

    return purc_dvobj_make_from_methods(method, PCA_TABLESIZE(method));
}
