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
#include "purc-dom.h"
#include "purc-utils.h"

#define PURC_TFORMAT_PREFIX_UTC       "{UTC}"

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
typedef struct pcintr_coroutine pcintr_coroutine;
typedef struct pcintr_coroutine *pcintr_coroutine_t;

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

/** Make a dynamic variant object for built-in `$SYSTEM` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_system_new(void);

/** Make a dynamic variant object for built-in `$SESSION` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_session_new(void);

/** Make a dynamic variant object for built-in `$DATETIME` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_datetime_new(void);

/** Make a dynamic variant object for built-in `$HVML` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_hvml_new(pcintr_coroutine_t cor);

/** Make a dynamic variant object for built-in `$DOC` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_doc_new(struct pcdom_document *doc);

/** Make a dynamic variant object for built-in `$EJSON` variable. */
PCA_EXPORT purc_variant_t
purc_dvobj_ejson_new(void);

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
purc_dvobj_stream_new(purc_atom_t cid);

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
purc_dvobj_read_struct(purc_rwstream_t stream,
        const char *formats, size_t formats_left, bool silently);

/**@}*/

PCA_EXTERN_C_END

#endif /* !PURC_PURC_DVOBJS_H */

