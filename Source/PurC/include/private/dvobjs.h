/**
 * @file dvobjs.h
 * @author Geng Yue, Vincent Wei
 * @date 2021/07/02
 * @brief The interface for dynamic variant objects.
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

#ifndef PURC_PRIVATE_DVOBJS_H
#define PURC_PRIVATE_DVOBJS_H

#include "purc-document.h"
#include "purc-rwstream.h"
#include "purc-variant.h"
#include "purc-dvobjs.h"

#include "config.h"

#include "private/utils.h"

#include <assert.h>
#include <time.h>

#define PURC_SYS_TZ_FILE    "/etc/localtime"
#if OS(DARWIN)
#define PURC_SYS_TZ_DIR     "/var/db/timezone/zoneinfo/"
#else
#define PURC_SYS_TZ_DIR     "/usr/share/zoneinfo/"
#endif

#define PURC_TIMEZONE_UTC   "UTC"

#define LEN_INI_PRINT_BUF       128
#define LEN_MAX_PRINT_BUF       0   // without limitation

#define LEN_INI_SERIALIZE_BUF   128
#define LEN_MAX_SERIALIZE_BUF   0   // without limitation

#define MAX_LEN_TIMEZONE    128
#define MAX_LEN_KEYWORD     64

#define PURC_KEYWORD_true   "true"
#define PURC_KEYWORD_false  "false"

enum {
    PURC_K_KW_FIRST = 0,
#define PURC_KW_caseless    "caseless"
    PURC_K_KW_caseless = PURC_K_KW_FIRST,
#define PURC_KW_case        "case"
    PURC_K_KW_case,
#define PURC_KW_regexp      "regexp"
    PURC_K_KW_regexp,
#define PURC_KW_wildcard    "wildcard"
    PURC_K_KW_wildcard,
#define PURC_KW_number      "number"
    PURC_K_KW_number,
#define PURC_KW_auto        "auto"
    PURC_K_KW_auto,
#define PURC_KW_asc         "asc"
    PURC_K_KW_asc,
#define PURC_KW_desc        "desc"
    PURC_K_KW_desc,
#define PURC_KW_i8          "i8"
    PURC_K_KW_i8,
#define PURC_KW_i16         "i16"
    PURC_K_KW_i16,
#define PURC_KW_i32         "i32"
    PURC_K_KW_i32,
#define PURC_KW_i64         "i64"
    PURC_K_KW_i64,
#define PURC_KW_i16le       "i16le"
    PURC_K_KW_i16le,
#define PURC_KW_i32le       "i32le"
    PURC_K_KW_i32le,
#define PURC_KW_i64le       "i64le"
    PURC_K_KW_i64le,
#define PURC_KW_i16be       "i16be"
    PURC_K_KW_i16be,
#define PURC_KW_i32be       "i32be"
    PURC_K_KW_i32be,
#define PURC_KW_i64be       "i64be"
    PURC_K_KW_i64be,
#define PURC_KW_u8          "u8"
    PURC_K_KW_u8,
#define PURC_KW_u16         "u16"
    PURC_K_KW_u16,
#define PURC_KW_u32         "u32"
    PURC_K_KW_u32,
#define PURC_KW_u64         "u64"
    PURC_K_KW_u64,
#define PURC_KW_u16le       "u16le"
    PURC_K_KW_u16le,
#define PURC_KW_u32le       "u32le"
    PURC_K_KW_u32le,
#define PURC_KW_u64le       "u64le"
    PURC_K_KW_u64le,
#define PURC_KW_u16be       "u16be"
    PURC_K_KW_u16be,
#define PURC_KW_u32be       "u32be"
    PURC_K_KW_u32be,
#define PURC_KW_u64be       "u64be"
    PURC_K_KW_u64be,
#define PURC_KW_f16         "f16"
    PURC_K_KW_f16,
#define PURC_KW_f32         "f32"
    PURC_K_KW_f32,
#define PURC_KW_f64         "f64"
    PURC_K_KW_f64,
#define PURC_KW_f96         "f96"
    PURC_K_KW_f96,
#define PURC_KW_f128        "f128"
    PURC_K_KW_f128,
#define PURC_KW_f16le       "f16le"
    PURC_K_KW_f16le,
#define PURC_KW_f32le       "f32le"
    PURC_K_KW_f32le,
#define PURC_KW_f64le       "f64le"
    PURC_K_KW_f64le,
#define PURC_KW_f96le       "f96le"
    PURC_K_KW_f96le,
#define PURC_KW_f128le      "f128le"
    PURC_K_KW_f128le,
#define PURC_KW_f16be       "f16be"
    PURC_K_KW_f16be,
#define PURC_KW_f32be       "f32be"
    PURC_K_KW_f32be,
#define PURC_KW_f64be       "f64be"
    PURC_K_KW_f64be,
#define PURC_KW_f96be       "f96be"
    PURC_K_KW_f96be,
#define PURC_KW_f128be      "f128be"
    PURC_K_KW_f128be,
#define PURC_KW_bytes       "bytes"
    PURC_K_KW_bytes,
#define PURC_KW_utf8        "utf8"
    PURC_K_KW_utf8,
#define PURC_KW_utf16       "utf16"
    PURC_K_KW_utf16,
#define PURC_KW_utf32       "utf32"
    PURC_K_KW_utf32,
#define PURC_KW_utf16le     "utf16le"
    PURC_K_KW_utf16le,
#define PURC_KW_utf32le     "utf32le"
    PURC_K_KW_utf32le,
#define PURC_KW_utf16be     "utf16be"
    PURC_K_KW_utf16be,
#define PURC_KW_utf32be     "utf32be"
    PURC_K_KW_utf32be,
#define PURC_KW_padding     "padding"
    PURC_K_KW_padding,
#define PURC_KW_binary      "binary"
    PURC_K_KW_binary,
#define PURC_KW_string      "string"
    PURC_K_KW_string,
#define PURC_KW_uppercase   "uppercase"
    PURC_K_KW_uppercase,
#define PURC_KW_lowercase   "lowercase"
    PURC_K_KW_lowercase,
#define PURC_KW_longint     "longint"
    PURC_K_KW_longint,
#define PURC_KW_ulongint    "ulongint"
    PURC_K_KW_ulongint,
#define PURC_KW_longdouble  "longdouble"
    PURC_K_KW_longdouble,
#define PURC_KW_object      "object"
    PURC_K_KW_object,
#define PURC_KW_local       "local"
    PURC_K_KW_local,
#define PURC_KW_global      "global"
    PURC_K_KW_global,
#define PURC_KW_rfc1738      "rfc1738"
    PURC_K_KW_rfc1738,
#define PURC_KW_rfc3986      "rfc3986"
    PURC_K_KW_rfc3986,

    /* XXX: change this when a new keyword appended */
    PURC_K_KW_LAST = PURC_K_KW_rfc3986,
};

#define PURC_GLOBAL_KEYWORD_NR  (PURC_K_KW_LAST - PURC_K_KW_FIRST + 1)

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* return the quantity in a format string like `u16be:12`,
   -1 for bad quantity. */
ssize_t
pcdvobjs_quantity_in_format(const char *format, size_t *format_length);

/* return -1 for bad keyword */
int pcdvobjs_global_keyword_id(const char *keyword, size_t length);

bool pcdvobjs_is_valid_timezone(const char *timezone) WTF_INTERNAL;
bool pcdvobjs_get_current_timezone(char *buff, size_t sz_buff) WTF_INTERNAL;

struct pcinst;

struct wildcard_list {
    char * wildcard;
    struct wildcard_list *next;
};

int32_t pcdvobjs_get_random(void) WTF_INTERNAL;

bool
pcdvobjs_is_elements(purc_variant_t v);

purc_variant_t
pcdvobjs_make_elements(purc_document_t doc, pcdoc_element_t element);

purc_variant_t
pcdvobjs_elements_by_css(purc_document_t doc, const char *css);

pcdoc_element_t
pcdvobjs_get_element_from_elements(purc_variant_t elems, size_t idx);

/* return the number of left characters cannot be decoded */
size_t pcdvobj_url_decode_in_place(char *string, size_t length, int rfc);

int pcdvobj_url_encode(struct pcutils_mystring *mystr,
        const unsigned char *bytes, size_t nr_bytes, int rfc);
int pcdvobj_url_decode(struct pcutils_mystring *mystr,
        const char *string, size_t length, int rfc, bool silently);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_DVOBJS_H */

