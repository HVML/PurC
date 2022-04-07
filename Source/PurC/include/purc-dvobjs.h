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

/** The structure defining the control properties of a HVML program */
struct purc_hvml_ctrl_props {
    /** The base URL as a null-terminated string. */
    char *base_url_string;

    /** The base URL broken down. */
    struct purc_broken_down_url base_url_broken_down;

    /** The maximal iteration count. */
    uint64_t            max_iteration_count;
    /** The maximal recursion depth. */
    uint64_t            max_recursion_depth;
    /** The maximal embedded levels of a EJSON container. */
    uint64_t            max_embedded_levels;

    /** The timeout value for a remote request. */
    struct timespec     timeout;
};

/** The structure defining a method of a dynamic variant object. */
struct purc_dvobj_method {
    /* The method name. */
    const char          *name;
    /* The getter of the method. */
    purc_dvariant_method getter;
    /* The setter of the method. */
    purc_dvariant_method setter;
};

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
purc_dvobj_hvml_new(const struct purc_hvml_ctrl_props **ctrl_props);

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

/**@}*/

PCA_EXTERN_C_END

#endif /* !PURC_PURC_DVOBJS_H */

