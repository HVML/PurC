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
#include "private/dvobjs.h"

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
        size_t len;

        if (rfc == PURC_K_KW_rfc1738 && bytes[i] == ' ') {
            encoded[0] = '+';
            len = 1;
        }
        else if (purc_isalnum(bytes[i]) ||
                bytes[i] == '-' || bytes[i] == '_' || bytes[i] == '.') {
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
        purc_set_error(PURC_ERROR_INVALID_VALUE);
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

purc_variant_t purc_dvobj_url_new(void)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;

    static struct purc_dvobj_method methods [] = {
        { "encode",     encode_getter,      NULL },
        { "decode",     decode_getter,      NULL },
    };

    retv = purc_dvobj_make_from_methods(methods, PCA_TABLESIZE(methods));
    return retv;
}
