/**
 * @file purc-dvobjs.h
 * @date 2021/03/15
 * @brief This file declares APIs for the built-in Dynmaic Variant Objects.
 *
 * Copyright (C) 2022 FMSoft (http://www.fmsoft.cn)
 *
 * Authors:
 *  Vincent Wei (<https://github.com/VincentWei>), 2022
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
 *
 */

#ifndef PURC_PURC_DVOBJS_H
#define PURC_PURC_DVOBJS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>

#include "purc-macros.h"
#include "purc-variant.h"
#include "purc-document.h"
#include "purc-utils.h"

#define PURC_TFORMAT_PREFIX_UTC         "{UTC}"

/* runner-level variables */
#define PURC_PREDEF_VARNAME_SYS         "SYS"
#define PURC_PREDEF_VARNAME_RUNNER      "RUNNER"
#define PURC_PREDEF_VARNAME_L           "L"
#define PURC_PREDEF_VARNAME_STR         "STR"
#define PURC_PREDEF_VARNAME_DATA        "DATA"
#define PURC_PREDEF_VARNAME_STREAM      "STREAM"
#define PURC_PREDEF_VARNAME_SOCKET      "SOCKET"
#define PURC_PREDEF_VARNAME_DATETIME    "DATETIME"
#define PURC_PREDEF_VARNAME_URL         "URL"
#define PURC_PREDEF_VARNAME_RDR         "RDR"

#define PURC_PREDEF_VARNAME_SYS_ZH      "系统"
#define PURC_PREDEF_VARNAME_RUNNER_ZH   "行者"
#define PURC_PREDEF_VARNAME_L_ZH        "逻辑"
#define PURC_PREDEF_VARNAME_STR_ZH      "字符串"
#define PURC_PREDEF_VARNAME_DATA_ZH     "数据"
#define PURC_PREDEF_VARNAME_STREAM_ZH   "流"
#define PURC_PREDEF_VARNAME_SOCKET_ZH   "套接字"
#define PURC_PREDEF_VARNAME_DATETIME_ZH "时间"

/* coroutine-level variables */
#define PURC_PREDEF_VARNAME_T           "T"
#define PURC_PREDEF_VARNAME_REQ         "REQ"
#define PURC_PREDEF_VARNAME_DOC         "DOC"
#define PURC_PREDEF_VARNAME_CRTN        "CRTN"
#define PURC_PREDEF_VARNAME_TIMERS      "TIMERS"

#define PURC_PREDEF_VARNAME_T_ZH        "文本"
#define PURC_PREDEF_VARNAME_REQ_ZH      "请求"
#define PURC_PREDEF_VARNAME_DOC_ZH      "文档"
#define PURC_PREDEF_VARNAME_CRTN_ZH     "协程"
#define PURC_PREDEF_VARNAME_TIMERS_ZH   "定时器"

/* external variables */
#define PURC_PREDEF_VARNAME_FS          "FS"
#define PURC_PREDEF_VARNAME_FILE        "FILE"
#define PURC_PREDEF_VARNAME_MATH        "MATH"

#define PURC_PREDEF_VARNAME_FS_ZH       "文件系统"
#define PURC_PREDEF_VARNAME_FILE_ZH     "文件"
#define PURC_PREDEF_VARNAME_MATH_ZH     "数学"

/** The structure defining a method of a dynamic variant object. */
struct purc_dvobj_method {
    /* The method name. */
    const char          *name;
    /* The getter of the method. */
    purc_dvariant_method getter;
    /* The setter of the method. */
    purc_dvariant_method setter;
};

struct pcintr_coroutine;

PCA_EXTERN_C_BEGIN

/**
 * @defgroup DVObjs Functions for Dynamic Variant Objects
 * @{
 */

/**
 * Make a dynamic variant object by using information in the array of
 * `struct purc_dvobj_method`.
 */
PCA_EXPORT purc_variant_t
purc_dvobj_make_from_methods(const struct purc_dvobj_method *method,
        size_t size);

/** Make a dynamic variant object for built-in `$SYS` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_system_new(void);

/** Make a dynamic variant object for built-in `$RUNNER` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_runner_new(void);

/** Make a dynamic variant object for built-in `$DATETIME` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_datetime_new(void);

/** Make a dynamic variant object for built-in `$CRTN` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_coroutine_new(struct pcintr_coroutine* cor);

/** Make a dynamic variant object for built-in `$DOC` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_doc_new(purc_document_t doc);

/** Make a dynamic variant object for built-in `$DATA` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_data_new(void);

/** Make a dynamic variant object for built-in `$L` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_logical_new(void);

/** Make a dynamic variant object for built-in `$T` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_text_new(void);

/** Make a dynamic variant object for built-in `$STR` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_string_new(void);

/** Make a dynamic variant object for built-in `$URL` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_url_new(void);

/** Make a dynamic variant object for built-in `$STREAM` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_stream_new(void);

/** Make a dynamic variant object for built-in `$SOCKET` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_socket_new(void);

/** Make a dynamic variant object for built-in `$RDR` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_rdr_new(void);

/** Parse format string and return the format identifier and quantity.
  * Return -1 on error. Quantity will be 0 if not specified. */
PCA_EXPORT int
purc_dvobj_parse_format(const char *format, size_t format_len,
        size_t *quantity);

struct pcdvobj_bytes_buff {
    uint8_t *bytes;
    size_t   nr_bytes;
    size_t   sz_allocated;
};

/** Pack a real or a real array to the byte buffer */
PCA_EXPORT int
purc_dvobj_pack_real(struct pcdvobj_bytes_buff *buff, purc_variant_t item,
        int format_id, size_t quantity, bool silently);

/** Pack a string to the byte buffer */
PCA_EXPORT int
purc_dvobj_pack_string(struct pcdvobj_bytes_buff *buff, purc_variant_t item,
        int format_id, size_t length);

/** Pack variant values to the byte buffer. */
PCA_EXPORT int
purc_dvobj_pack_variants(struct pcdvobj_bytes_buff *buff,
        purc_variant_t *argv, size_t nr_args,
        const char *formats, size_t formats_len, bool silently);

/** Unpack real number(s) and make a real variant or an array from
  * the byte sequence specified.
  * Return undefined for invalid arguments or an invalid variant
  * for fatal error. */
PCA_EXPORT purc_variant_t
purc_dvobj_unpack_real(const unsigned char *bytes, size_t nr_bytes,
        int format_id, size_t quantity);

/** Unpack a string in the specified encoding identifier from
  * the byte sequence specified.
  * Return undefined for bad encodings, or an invalid variant
  * for fatal error. */
PCA_EXPORT purc_variant_t
purc_dvobj_unpack_string(const unsigned char *bytes, size_t nr_bytes,
        size_t *consumed, int format_id, bool silently);

/** Unpack a byte sequence and returns an array.
  * Return an empty array for invalid arguments when @silently is true,
  * or an invalid variant for any error. */
PCA_EXPORT purc_variant_t
purc_dvobj_unpack_bytes(const uint8_t *bytes, size_t nr_bytes,
        const char *formats, size_t formats_len, bool silently);

/** Read struct from stream and returns an array.
  * Return an empty array for invalid arguments when @silently is true,
  * or an invalid variant for any error. */
PCA_EXPORT purc_variant_t
purc_dvobj_read_struct(purc_rwstream_t stream, const char *formats,
        size_t formats_left, size_t *nr_total_read, bool silently);

/** Match the wildcard specified by @pattern
    with multiple string candidants specified by @strs.
    This function returns the mached strings in a 32-bit integer,
    one bit per string; -1 on failure.
    Note that the @nr_strs should be less than 32. */
int pcdvobjs_wildcard_cmp_ex(const char *pattern,
        const char *strs[], int nr_strs);

/** Match the regualr expression specified by @pattern
    with multiple string candidants specified by @strs.
    This function returns the mached strings in a 32-bit integer,
    one bit per string; -1 on failure.
    Note that the @nr_strs should be less than 32. */
int pcdvobjs_regex_cmp_ex(const char *pattern,
        const char *strs[], int nr_strs);

/** Match the event pattern specified by @main_pattern and @sub_pattern
    with multiple event candidants specified by @events.
    This function returns the mached events in a 32-bit integer,
    one bit per event; -1 on failure.
    Note that the @nr_events should be less than 32. */
int pcdvobjs_match_events(const char *main_pattern, const char *sub_pattern,
        const char *events[], int nr_events);

/** Cast a number to a time value (struct timeval). */
bool pcdvobjs_cast_to_timeval(struct timeval *timeval, purc_variant_t t);

struct pcdvobjs_option_to_atom {
    const char *option;
    purc_atom_t atom;
    int flag;
};

/** Parse options specified by @vrt according to @single_keywords
    and @composite_keywords. */
int pcdvobjs_parse_options(purc_variant_t vrt,
        const struct pcdvobjs_option_to_atom *single_keywords, size_t nr_skw,
        const struct pcdvobjs_option_to_atom *composite_keywords, size_t nr_ckw,
        int flags4null, int flags4failed);

/**@}*/

PCA_EXTERN_C_END

#endif /* !PURC_PURC_DVOBJS_H */

