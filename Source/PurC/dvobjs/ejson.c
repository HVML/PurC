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

#define LEN_INI_SERIALIZE_BUF   128
#define LEN_MAX_SERIALIZE_BUF   4096

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

#define _KW_DELIMITERS  " \t\n\v\f\r"

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

    const char *options = NULL;
    size_t options_len;
    unsigned int flags = PCVARIANT_SERIALIZE_OPT_PLAIN;

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

            if (length > 0 || length <= MAX_LEN_KEYWORD) {
                purc_atom_t atom;

                /* TODO: use strndupa if it is available */
                char *tmp = strndup(option, length);
                atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);
                free(tmp);

                if (atom > 0) {
                    size_t i;
                    for (i = 0; i < PCA_TABLESIZE(keywords2atoms); i++) {
                        if (atom == keywords2atoms[i].atom) {
                            if (keywords2atoms[i].flag &
                                    PCVARIANT_SERIALIZE_OPT_BSEQUENCE_MASK) {
                                // clear the byte sequence mask
                                flags &= ~PCVARIANT_SERIALIZE_OPT_BSEQUENCE_MASK;
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
        bool silently)
{
    UNUSED_PARAM(root);

    if (nr_args < 2) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *string;
    size_t length;
    string = purc_variant_get_string_const_ex(argv[1], &length);
    if (string == NULL) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    struct purc_ejson_parse_tree *ptree;

    ptree = purc_variant_ejson_parse_string(string, length);
    if (ptree == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    purc_variant_t retv;
    retv = purc_variant_ejson_parse_tree_evalute(ptree, NULL, NULL, silently);
    purc_variant_ejson_parse_tree_destroy(ptree);
    return retv;

failed:
    if (silently)
        return purc_variant_make_undefined();

fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
isequal_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    if (nr_args < 2) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    bool v = purc_variant_is_equal_to(argv[0], argv[1]);
    return purc_variant_make_boolean(v);

failed:
    if (silently)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
compare_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);

    const char *option = NULL;
    size_t option_len;
    unsigned int flag = PCVARIANT_COMPARE_OPT_AUTO;

    if (nr_args < 2) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (nr_args >= 3) {
        option = purc_variant_get_string_const_ex(argv[2], &option_len);
        if (option == NULL) {
            pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        if (option_len == 0) {
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        option = pcutils_trim_spaces(option, &option_len);
        int cmp_id = pcdvobjs_global_keyword_id(option, option_len);
        switch (cmp_id) {
        case PURC_K_KW_auto:
            flag = PCVARIANT_COMPARE_OPT_AUTO;
            break;

        case PURC_K_KW_number:
            flag = PCVARIANT_COMPARE_OPT_NUMBER;
            break;

        case PURC_K_KW_caseless:
            flag = PCVARIANT_COMPARE_OPT_CASELESS;
            break;

        case PURC_K_KW_case:
            flag = PCVARIANT_COMPARE_OPT_CASE;
            break;

        default:
            if (!silently) {
                pcinst_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }
            break;
        }
    }

    double result = 0.0;
    result = purc_variant_compare_ex(argv[0], argv[1], flag);
    return purc_variant_make_number(result);

failed:
    if (silently)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
fetchstr_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    const char *encoding = NULL;
    size_t encoding_len;
    uint64_t length;
    int64_t offset = 0;

    if (nr_args < 3) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (!purc_variant_is_bsequence(argv[0])) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    encoding = purc_variant_get_string_const_ex(argv[1], &encoding_len);
    if (encoding == NULL) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (encoding_len == 0) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (!purc_variant_cast_to_ulongint(argv[2], &length, false)) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (nr_args >= 3) {
        if (!purc_variant_cast_to_longint(argv[3], &offset, false)) {
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    const unsigned char *bytes;
    size_t nr_bytes;
    bytes = purc_variant_get_bytes_const(argv[0], &nr_bytes);

    if (offset > 0 && (uint64_t)offset >= nr_bytes) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (offset < 0 &&  (uint64_t)-offset > nr_bytes) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (offset < 0)
        offset = nr_bytes + offset;

    if (offset + length > nr_bytes) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (length == 0)
        return purc_variant_make_string_static("", false);

    bytes += offset;
    char *str = NULL;
    size_t str_len;

    encoding = pcutils_trim_spaces(encoding, &encoding_len);
    int encoding_id = pcdvobjs_global_keyword_id(encoding, encoding_len);
    switch (encoding_id) {
    case PURC_K_KW_utf8:
        return purc_variant_make_string_ex((const char *)bytes, length, !silently);

    case PURC_K_KW_utf16:
        str = pcutils_string_decode_utf16(bytes, length, &str_len, silently);
        break;

    case PURC_K_KW_utf32:
        str = pcutils_string_decode_utf32(bytes, length, &str_len, silently);
        break;

    case PURC_K_KW_utf16le:
        str = pcutils_string_decode_utf16le(bytes, length, &str_len, silently);
        break;

    case PURC_K_KW_utf32le:
        str = pcutils_string_decode_utf32le(bytes, length, &str_len, silently);
        break;

    case PURC_K_KW_utf16be:
        str = pcutils_string_decode_utf16be(bytes, length, &str_len, silently);
        break;

    case PURC_K_KW_utf32be:
        str = pcutils_string_decode_utf32be(bytes, length, &str_len, silently);
        break;

    default:
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (str == NULL) {
        pcinst_set_error(PURC_ERROR_BAD_ENCODING);
        goto failed;
    }

    return purc_variant_make_string_reuse_buff(str, str_len, !silently);

failed:
    if (silently)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

typedef purc_real_t (*fn_fetch_real)(const unsigned char *bytes);

static const struct real_info {
    uint8_t         length;         // unit length in bytes
    uint8_t         real_type;      // EJSON real type
    fn_fetch_real   fetcher;        // fetcher
} real_info[] = {
    { 1,  PURC_VARIANT_TYPE_LONGINT,    purc_fetch_i8       },  // "i8"
    { 2,  PURC_VARIANT_TYPE_LONGINT,    purc_fetch_i16      },  // "i16"
    { 4,  PURC_VARIANT_TYPE_LONGINT,    purc_fetch_i32      },  // "i32"
    { 8,  PURC_VARIANT_TYPE_LONGINT,    purc_fetch_i64      },  // "i64"
    { 2,  PURC_VARIANT_TYPE_LONGINT,    purc_fetch_i16le    },  // "i16le"
    { 4,  PURC_VARIANT_TYPE_LONGINT,    purc_fetch_i32le    },  // "i32le"
    { 6,  PURC_VARIANT_TYPE_LONGINT,    purc_fetch_i64le    },  // "i64le"
    { 2,  PURC_VARIANT_TYPE_LONGINT,    purc_fetch_i16be    },  // "i16be"
    { 4,  PURC_VARIANT_TYPE_LONGINT,    purc_fetch_i32be    },  // "i32be"
    { 8,  PURC_VARIANT_TYPE_LONGINT,    purc_fetch_i64be    },  // "i64be"
    { 1,  PURC_VARIANT_TYPE_ULONGINT,   purc_fetch_u8       },  // "u8"
    { 2,  PURC_VARIANT_TYPE_ULONGINT,   purc_fetch_u16      },  // "u16"
    { 4,  PURC_VARIANT_TYPE_ULONGINT,   purc_fetch_u32      },  // "u32"
    { 8,  PURC_VARIANT_TYPE_ULONGINT,   purc_fetch_u64      },  // "u64"
    { 2,  PURC_VARIANT_TYPE_ULONGINT,   purc_fetch_u16le    },  // "u16le"
    { 4,  PURC_VARIANT_TYPE_ULONGINT,   purc_fetch_u32le    },  // "u32le"
    { 8,  PURC_VARIANT_TYPE_ULONGINT,   purc_fetch_u64le    },  // "u64le"
    { 2,  PURC_VARIANT_TYPE_ULONGINT,   purc_fetch_u16be    },  // "u16be"
    { 4,  PURC_VARIANT_TYPE_ULONGINT,   purc_fetch_u32be    },  // "u32be"
    { 8,  PURC_VARIANT_TYPE_ULONGINT,   purc_fetch_u64be    },  // "u64be"
    { 2,  PURC_VARIANT_TYPE_NUMBER,     purc_fetch_f16      },  // "f16"
    { 4,  PURC_VARIANT_TYPE_NUMBER,     purc_fetch_f32      },  // "f32"
    { 8,  PURC_VARIANT_TYPE_NUMBER,     purc_fetch_f64      },  // "f64"
    { 12, PURC_VARIANT_TYPE_LONGDOUBLE, purc_fetch_f96      },  // "f96"
    { 16, PURC_VARIANT_TYPE_LONGDOUBLE, purc_fetch_f128     },  // "f128"
    { 2,  PURC_VARIANT_TYPE_NUMBER,     purc_fetch_f16le    },  // "f16le"
    { 4,  PURC_VARIANT_TYPE_NUMBER,     purc_fetch_f32le    },  // "f32le"
    { 8,  PURC_VARIANT_TYPE_NUMBER,     purc_fetch_f64le    },  // "f64le"
    { 12, PURC_VARIANT_TYPE_LONGDOUBLE, purc_fetch_f96le    },  // "f96le"
    { 16, PURC_VARIANT_TYPE_LONGDOUBLE, purc_fetch_f128le   },  // "f128le"
    { 2,  PURC_VARIANT_TYPE_NUMBER,     purc_fetch_f16be    },  // "f16be"
    { 4,  PURC_VARIANT_TYPE_NUMBER,     purc_fetch_f32be    },  // "f32be"
    { 8,  PURC_VARIANT_TYPE_NUMBER,     purc_fetch_f64be    },  // "f64be"
    { 12, PURC_VARIANT_TYPE_LONGDOUBLE, purc_fetch_f96be    },  // "f96be"
    { 16, PURC_VARIANT_TYPE_LONGDOUBLE, purc_fetch_f128be   },  // "f128be"
};

static purc_variant_t
fetchreal_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    const char *format = NULL;
    size_t format_len;
    size_t length = 0;
    int64_t offset = 0;

    if (nr_args < 2) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const unsigned char *bytes;
    size_t nr_bytes;
    bytes = purc_variant_get_bytes_const(argv[0], &nr_bytes);
    if (bytes == NULL) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (nr_bytes == 0) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    format = purc_variant_get_string_const_ex(argv[1], &format_len);
    if (format == NULL) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (format_len == 0) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    // parse format and get the length of real number.
    format = pcutils_trim_spaces(format, &format_len);
    int format_id = pcdvobjs_global_keyword_id(format, format_len);
    assert(format_id >= PURC_K_KW_i8 && format_id <= PURC_K_KW_f128be);

    format_id -= PURC_K_KW_i8;
    length = real_info[format_id].length;
    if (nr_args >= 2) {
        if (!purc_variant_cast_to_longint(argv[3], &offset, false)) {
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    if (offset > 0 && (uint64_t)offset >= nr_bytes) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (offset < 0 &&  (uint64_t)-offset > nr_bytes) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (offset < 0)
        offset = nr_bytes + offset;

    if (offset + length > nr_bytes) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    purc_real_t real = real_info[format_id].fetcher(bytes);
    switch (real_info[format_id].real_type) {
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

failed:
    if (silently)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
shuffle_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    if (nr_args < 1) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (!(purc_variant_is_array(argv[0]) || purc_variant_is_set(argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    // TODO

    return purc_variant_make_boolean(true);

failed:
    return purc_variant_make_boolean(false);
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

purc_variant_t purc_dvobj_ejson_new(void)
{
    static struct purc_dvobj_method method [] = {
        { "type",       type_getter, NULL },
        { "count",      count_getter, NULL },
        { "numberify",  numberify_getter, NULL },
        { "booleanize", booleanize_getter, NULL },
        { "stringify",  stringify_getter, NULL },
        { "serialize",  serialize_getter, NULL },
        { "parse",      parse_getter, NULL },
        { "isequal",    isequal_getter, NULL },
        { "compare",    compare_getter, NULL },
        { "fetchstr",   fetchstr_getter, NULL },
        { "fetchreal",  fetchreal_getter, NULL },
        { "shuffle",    shuffle_getter, NULL },
        { "sort",       sort_getter, NULL },
    };

    if (keywords2atoms[0].atom == 0) {
        for (size_t i = 0; i < PCA_TABLESIZE(keywords2atoms); i++) {
            keywords2atoms[i].atom =
                purc_atom_from_static_string_ex(ATOM_BUCKET_DVOBJ,
                    keywords2atoms[i].keyword);
        }
    }

    return purc_dvobj_make_from_methods(method, PCA_TABLESIZE(method));
}
