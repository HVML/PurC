/*
 * @file url.c
 * @author Vincent Wei
 * @date 2022/03/31
 * @brief The implementation of URL dynamic variant object.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

#include "purc-variant.h"
#include "purc-errors.h"
#include "purc-helpers.h"

#include "private/utils.h"
#include "private/url.h"
#include "private/dvobjs.h"
#include "private/atom-buckets.h"
#include <stdbool.h>

enum {
#define _KW_real_json       "real-json"
    K_KW_real_json,
#define _KW_real_ejson      "real-ejson"
    K_KW_real_ejson,
#define _KW_rfc1738         "rfc1738"
    K_KW_rfc1738,
#define _KW_rfc3986         "rfc3986"
    K_KW_rfc3986,
};

static struct keyword_to_atom {
    const char *    keyword;
    unsigned int    flag;
    purc_atom_t     atom;
} keywords2atoms [] = {
    { _KW_real_json,        PCUTILS_URL_OPT_REAL_JSON,  0 },
    { _KW_real_ejson,       PCUTILS_URL_OPT_REAL_EJSON, 0 },
    { _KW_rfc1738,          PCUTILS_URL_OPT_RFC1738,    0 },
    { _KW_rfc3986,          PCUTILS_URL_OPT_RFC3986,    0 },
};

size_t pcdvobj_url_decode_in_place(char *string, size_t length, int rfc)
{
    size_t nr_decoded = 0;
    size_t left = length;
    unsigned char *dest = (unsigned char *)string;

    while (left > 0) {
        unsigned char decoded;

        if (rfc == PURC_K_KW_rfc1738 && *string == '+') {
            decoded = ' ';
        }
        else if (purc_isalnum(*string) ||
                *string == '-' || *string == '_' || *string == '.') {
            decoded = (unsigned char)*string;
        }
        else {
            if (*string == '%') {
                if (left > 2) {
                    if (pcutils_hex2byte(string + 1, &decoded)) {
                        goto bad_encoding;
                    }

                    string += 2;
                    left -= 2;
                }
                else {
                    goto bad_encoding;
                }
            }
            else {
                goto bad_encoding;
            }
        }

        dest[nr_decoded] = decoded;
        nr_decoded++;

        left--;
        string++;
    }

bad_encoding:
    dest[nr_decoded] = 0;
    return left;
}

int pcdvobj_url_encode(struct pcutils_mystring *mystr,
        const unsigned char *bytes, size_t nr_bytes, int rfc)
{
    for (size_t i = 0; i < nr_bytes; i++) {
        unsigned char encoded[4];
        size_t len = 0;

        if (rfc == PURC_K_KW_rfc1738 && bytes[i] == ' ') {
            encoded[0] = '+';
            len = 1;
        }
        else if (purc_isalnum(bytes[i]) ||
                bytes[i] == '-' || bytes[i] == '_' || bytes[i] == '.' ||
                (rfc == PURC_K_KW_rfc3986 && bytes[i] == '~')) {
            encoded[0] = bytes[i];
            len = 1;
        }
        else {
            encoded[0] = '%';
            pcutils_bin2hex(bytes + i, 1, (char *)encoded + 1, true);
            len = 3;
        }

        if (pcutils_mystring_append_mchar(mystr, encoded, len))
            return -1;
    }

    return 0;
}

int pcdvobj_url_decode(struct pcutils_mystring *mystr,
        const char *string, size_t length, int rfc, bool silently)
{
    size_t left = length;

    while (left > 0) {
        unsigned char decoded;

        if (rfc == PURC_K_KW_rfc1738 && *string == '+') {
            decoded = ' ';
        }
        else if (purc_isalnum(*string) ||
                *string == '-' || *string == '_' || *string == '.' ||
                (rfc == PURC_K_KW_rfc3986 && *string == '~')) {
            decoded = (unsigned char)*string;
        }
        else {
            if (*string == '%') {
                if (left > 2) {
                    if (pcutils_hex2byte(string + 1, &decoded)) {
                        goto bad_encoding;
                    }

                    string += 2;
                    left -= 2;
                }
                else {
                    goto bad_encoding;
                }
            }
            else {
                goto bad_encoding;
            }
        }

        if (pcutils_mystring_append_mchar(mystr, &decoded, 1))
            return -1;

        left--;
        string++;
    }

    return 0;

bad_encoding:
    if (silently)
        return 0;
    return 1;
}

static purc_variant_t
encode_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const void *bytes;
    size_t nr_bytes;
    int rfc = PURC_K_KW_rfc1738;

    bytes = purc_variant_get_bytes_const(argv[0], &nr_bytes);
    if (bytes == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (purc_variant_is_string(argv[0])) {
        assert(nr_bytes > 0);
        nr_bytes--; // do not encode the terminating null byte.
    }

    if (nr_args > 1) {
        const char *encoding;
        size_t len;

        encoding = purc_variant_get_string_const_ex(argv[1], &len);
        if (encoding == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        encoding = pcutils_trim_spaces(encoding, &len);
        if (len == 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        rfc = pcdvobjs_global_keyword_id(encoding, len);
        if (rfc != PURC_K_KW_rfc1738 && rfc != PURC_K_KW_rfc3986) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    if (nr_bytes == 0) {
        return purc_variant_make_string_static("", false);
    }

    DECL_MYSTRING(mystr);
    if (pcdvobj_url_encode(&mystr, bytes, nr_bytes, rfc) ||
            pcutils_mystring_done(&mystr)) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    return purc_variant_make_string_reuse_buff(mystr.buff,
            mystr.sz_space, false);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_string_static("", false);

fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
decode_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    bool silently = call_flags & PCVRT_CALL_FLAG_SILENTLY;
    const void *string;
    size_t length;
    int rtt = PURC_K_KW_string;     // return type
    int rfc = PURC_K_KW_rfc1738;    // encoding type

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    string = purc_variant_get_string_const_ex(argv[0], &length);
    if (string == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (nr_args > 1) {
        const char *rettype;
        size_t len;

        rettype = purc_variant_get_string_const_ex(argv[1], &len);
        if (rettype == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        rettype = pcutils_trim_spaces(rettype, &len);
        if (len == 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        rtt = pcdvobjs_global_keyword_id(rettype, len);
        if (rtt != PURC_K_KW_string && rtt != PURC_K_KW_binary) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        if (nr_args > 2) {
            const char *encoding;
            size_t len;

            encoding = purc_variant_get_string_const_ex(argv[2], &len);
            if (encoding == NULL) {
                purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                goto failed;
            }

            encoding = pcutils_trim_spaces(encoding, &len);
            if (len == 0) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            rfc = pcdvobjs_global_keyword_id(encoding, len);
            if (rfc != PURC_K_KW_rfc1738 && rfc != PURC_K_KW_rfc3986) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }
        }
    }

    if (length == 0) {
        if (rtt == PURC_K_KW_string)
            return purc_variant_make_string_static("", false);

        return purc_variant_make_byte_sequence_empty();
    }

    DECL_MYSTRING(mystr);
    int ret = pcdvobj_url_decode(&mystr, string, length, rfc, silently);
    if (ret > 0) {
        pcutils_mystring_free(&mystr);
        purc_set_error(PURC_ERROR_BAD_ENCODING);
        goto failed;
    }
    else if (ret < 0) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    if (rtt == PURC_K_KW_string) {
        if (pcutils_mystring_done(&mystr)) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto fatal;
        }

        return purc_variant_make_string_reuse_buff(mystr.buff,
                mystr.sz_space, !silently);
    }

    return purc_variant_make_byte_sequence_reuse_buff(mystr.buff,
            mystr.nr_bytes, mystr.sz_space);


failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        if (rtt == PURC_K_KW_binary)    // the default value may be overridden
            return purc_variant_make_byte_sequence_empty();

        return purc_variant_make_string_static("", false);
    }

fatal:
    return PURC_VARIANT_INVALID;
}

/*
$URL.build_query(
    < object | array $query_data >
    [, < string $numeric_prefix = '': `The numeric prefix for the argument names if $query_data is an array.` >
        [, <'[real-json | real-ejson] || [rfc1738 | rfc3986]' $opts = 'real-json rfc1738':
        - 'real-json':    `Use JSON notation for real numbers, i.e., treat all real numbers (number, longint, ulongint, and longdouble) as JSON numbers.`
        - 'real-ejson':   `Use eJSON notation for longint, ulongint, and longdouble, e.g., 100L, 999UL, and 100FL.`
        - 'rfc1738':      `Encoding is performed per RFC 1738 and the 'application/x-www-form-urlencoded' media type, which implies that spaces are encoded as plus (+) signs.`
        - 'rfc3986':      `Encoding is performed according to RFC 3986, and spaces will be percent encoded (%20).`
            [, <string $arg_separator = '&': `The character used to separate the arguments. `>
            ]
        ]
    ]
) string | false
 */
static purc_variant_t
build_query_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *numeric_prefix = NULL;
    const char *options = NULL;
    size_t options_len;
    char arg_separator = '&';
    unsigned int flags = 0;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (nr_args > 1) {
        size_t len = 0;

        if (purc_variant_is_null(argv[1])) {
            numeric_prefix = NULL;
        }
        else if (purc_variant_is_string(argv[1])) {
            numeric_prefix = purc_variant_get_string_const_ex(argv[1], &len);
            if (numeric_prefix == NULL) {
                purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                goto failed;
            }
        }
        else {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        if (nr_args > 2) {
            options = purc_variant_get_string_const_ex(argv[2], &options_len);
            if (!options) {
                purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                goto failed;
            }

            options = pcutils_trim_spaces(options, &options_len);
            if (len == 0) {
                options = NULL;
            }
        }

        if (nr_args > 3) {
            const char *separator;
            size_t len = 0;

            separator = purc_variant_get_string_const_ex(argv[3], &len);
            if (separator == NULL || len > 1) {
                purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                goto failed;
            }

            arg_separator = separator[0];
        }

    }

    if (options) {
        size_t length = 0;
        const char *option = pcutils_get_next_token_len(options, options_len,
                PURC_KW_DELIMITERS, &length);

        do {

            if (length > 0 || length <= MAX_LEN_KEYWORD) {
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
                            flags |= keywords2atoms[i].flag;
                        }
                    }
                }
            }

            if (options_len <= length)
                break;

            options_len -= length;
            option = pcutils_get_next_token_len(option + length, options_len,
                    PURC_KW_DELIMITERS, &length);
        } while (option);
    }

    return pcutils_url_build_query(argv[0], numeric_prefix, arg_separator, flags);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_string_static("", false);
    }

    return PURC_VARIANT_INVALID;
}

/*
 $URL.parse_query(
    < string $query_string >
    [, <'[array | object] || [string | binary | auto] || [rfc1738 | rfc3986]' $opts = 'object auto rfc1738':
        - 'array':    `construct an array with the query string; this will ignore the argument names in the query string.`
        - 'object':   `construct an object with the query string.`
        - 'auto':     `The argument values will be decoded as strings first; if failed, decoded into binary sequences.`
        - 'binary':   `The argument values will be decoded as binary sequences.`
        - 'string':   `The argument values will be decoded as strings.` >
        - 'rfc1738':  `The query string is encoded per RFC 1738 and the 'application/x-www-form-urlencoded' media type, which implies that spaces are encoded as plus (+) signs.`
        - 'rfc3986':  `The query string is encoded according to RFC 3986, and spaces will be percent encoded (%20).`
        [, <string $arg_separator = '&': `The character used to separate the arguments. >]
    ]
) object | array | false
 */
static purc_variant_t
parse_query_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    int ec = PURC_ERROR_OK;
    purc_variant_t result = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto failed;
    }

    const char *query_str;
    size_t query_len;
    query_str = purc_variant_get_string_const_ex(argv[0], &query_len);
    if (query_str == NULL) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto failed;
    }

    // Default options
    bool as_array = false;
    int decode_type = PURC_K_KW_auto;
    int rfc = PURC_K_KW_rfc1738;
    char arg_separator = '&';

    // Process option parameters
    if (nr_args > 1) {
        const char *options;
        size_t options_len;
        
        options = purc_variant_get_string_const_ex(argv[1], &options_len);
        if (options) {
            options = pcutils_trim_spaces(options, &options_len);
            if (options_len > 0) {
                size_t length = 0;
                const char *option = pcutils_get_next_token_len(options, options_len,
                        PURC_KW_DELIMITERS, &length);

                do {
                    if (length > 0 && length <= MAX_LEN_KEYWORD) {
                        char tmp[length + 1];
                        strncpy(tmp, option, length);
                        tmp[length] = '\0';

                        if (strcmp(tmp, "array") == 0)
                            as_array = true;
                        else if (strcmp(tmp, "object") == 0)
                            as_array = false;
                        else if (strcmp(tmp, "auto") == 0)
                            decode_type = PURC_K_KW_auto;
                        else if (strcmp(tmp, "binary") == 0)
                            decode_type = PURC_K_KW_binary;
                        else if (strcmp(tmp, "string") == 0)
                            decode_type = PURC_K_KW_string;
                        else if (strcmp(tmp, "rfc1738") == 0)
                            rfc = PURC_K_KW_rfc1738;
                        else if (strcmp(tmp, "rfc3986") == 0)
                            rfc = PURC_K_KW_rfc3986;
                        else {
                            ec = PURC_ERROR_INVALID_VALUE;
                            goto failed;
                        }
                    }

                    if (options_len <= length)
                        break;

                    options_len -= length;
                    option = pcutils_get_next_token_len(option + length, options_len,
                            PURC_KW_DELIMITERS, &length);
                } while (option);
            }
        }

        // Process separator parameter
        if (nr_args > 2) {
            const char *separator;
            size_t len;
            separator = purc_variant_get_string_const_ex(argv[2], &len);
            if (separator && len == 1) {
                arg_separator = separator[0];
            }
        }
    }

    // Create return value container
    if (as_array) {
        result = purc_variant_make_array_0();
    }
    else {
        result = purc_variant_make_object_0();
    }

    if (!result) {
        ec = PURC_ERROR_OUT_OF_MEMORY;
        goto failed;
    }

    // Return empty container for empty query string
    if (query_len == 0) {
        return result;
    }

    // Parse query string
    const char *p = query_str;
    const char *end = p + query_len;
    
    while (p < end) {
        // Find key-value pair separator
        const char *eq = memchr(p, '=', end - p);
        if (!eq) break;

        // Find argument separator
        const char *next = memchr(eq + 1, arg_separator, end - (eq + 1));
        if (!next) next = end;

        // Decode key
        DECL_MYSTRING(key);
        int ret = pcdvobj_url_decode(&key, p, eq - p, rfc, false);
        if (ret) {
            pcutils_mystring_free(&key);
            ec = (ret < 0) ? PURC_ERROR_OUT_OF_MEMORY : PURC_ERROR_BAD_ENCODING;
            goto failed;
        }

        // Decode value
        DECL_MYSTRING(value);
        ret = pcdvobj_url_decode(&value, eq + 1, next - (eq + 1), rfc, false);
        if (ret) {
            pcutils_mystring_free(&key);
            pcutils_mystring_free(&value);
            ec = (ret < 0) ? PURC_ERROR_OUT_OF_MEMORY : PURC_ERROR_BAD_ENCODING;
            goto failed;
        }

        // Create value variant based on decode type
        purc_variant_t val = PURC_VARIANT_INVALID;
        if (decode_type == PURC_K_KW_binary) {
            val = purc_variant_make_byte_sequence_reuse_buff(value.buff,
                    value.nr_bytes, value.sz_space);
        }
        else if (decode_type == PURC_K_KW_string) {
            if (pcutils_mystring_done(&value) == 0) {
                val = purc_variant_make_string_reuse_buff(value.buff,
                        value.sz_space, true);
            }
        }
        else { // auto
            if (pcutils_mystring_done(&value) == 0) {
                val = purc_variant_make_string_reuse_buff(value.buff,
                        value.sz_space, true);
                if (val == PURC_VARIANT_INVALID) {
                    if (purc_get_last_error() == PURC_ERROR_BAD_ENCODING) {
                        val = purc_variant_make_byte_sequence_reuse_buff(value.buff,
                                value.nr_bytes - 1, value.sz_space);
                    }
                    else {
                        ec = PURC_ERROR_OUT_OF_MEMORY;
                        goto failed;
                    }
                }
            }
        }

        if (!val) {
            pcutils_mystring_free(&key);
            pcutils_mystring_free(&value);
            ec = PURC_ERROR_OUT_OF_MEMORY;
            goto failed;
       }

        // Add key-value pair to result container
        if (as_array) {
            if (!purc_variant_array_append(result, val)) {
                pcutils_mystring_free(&key);
                purc_variant_unref(val);
                ec = PURC_ERROR_OUT_OF_MEMORY;
                goto failed;
            }
        }
        else {
            if (pcutils_mystring_done(&key) == 0) {
                if (!purc_variant_object_set_by_ckey(result, key.buff, val)) {
                    pcutils_mystring_free(&key);
                    purc_variant_unref(val);
                    ec = PURC_ERROR_OUT_OF_MEMORY;
                    goto failed;
                }
            }
        }

        pcutils_mystring_free(&key);
        purc_variant_unref(val);

        p = next + 1;
        if (p >= end) break;
    }

    return result;

failed:
    if (ec != PURC_ERROR_OK)
        purc_set_error(ec);

    if (result)
        purc_variant_unref(result);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_boolean(false);
    }

    return PURC_VARIANT_INVALID;
}

purc_variant_t purc_dvobj_url_new(void)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;

    static struct purc_dvobj_method methods [] = {
        { "encode",     encode_getter,      NULL },
        { "decode",     decode_getter,      NULL },
        { "build_query", build_query_getter, NULL },
        { "parse_query", parse_query_getter, NULL },
        // { "parse",       parse_getter,      NULL },
        // { "assembly",    assembly_getter,   NULL },
    };

    if (keywords2atoms[0].atom == 0) {
        for (size_t i = 0; i < PCA_TABLESIZE(keywords2atoms); i++) {
            keywords2atoms[i].atom =
                purc_atom_from_static_string_ex(ATOM_BUCKET_DVOBJ,
                    keywords2atoms[i].keyword);
        }
    }

    retv = purc_dvobj_make_from_methods(methods, PCA_TABLESIZE(methods));
    return retv;
}
