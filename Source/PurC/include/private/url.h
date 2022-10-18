/*
 * @file url.h
 * @author gengyue
 * @date 2021/12/26
 * @brief The header file of URL implementation.
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

#ifndef PURC_PRIVATE_URL_H
#define PURC_PRIVATE_URL_H

#include "purc-utils.h"
#include "purc-variant.h"

#ifdef __cplusplus
extern "C" {
#endif

enum pcutils_url_real_notation {
    PCUTILS_URL_REAL_NOTATION_JSON,
    PCUTILS_URL_REAL_NOTATION_EJSON
};

enum pcutils_url_encode_type {
    PCUTILS_URL_ENCODE_TYPE_RFC1738,
    PCUTILS_URL_ENCODE_TYPE_RFC3986
};

enum pcutils_url_decode_dest {
    PCUTILS_URL_DECODE_DEST_OBJECT,
    PCUTILS_URL_DECODE_DEST_ARRAY
};

enum pcutils_url_decode_opt {
    PCUTILS_URL_DECODE_OPT_AUTO,
    PCUTILS_URL_DECODE_OPT_BINARY,
    PCUTILS_URL_DECODE_OPT_STRING
};

purc_variant_t
pcutils_url_build_query(purc_variant_t v, const char *numeric_prefix,
        char arg_separator, enum pcutils_url_real_notation real_notation,
        enum pcutils_url_encode_type encode_type);

purc_variant_t
pcutils_url_parse_query(const char *query, char arg_separator,
        enum pcutils_url_decode_dest decode_dest,
        enum pcutils_url_decode_opt decode_opt,
        enum pcutils_url_encode_type encode_type);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_PRIVATE_URL_H*/

