/**
 * @file purc-dvobjs.h
 * @date 2021/03/15
 * @brief This file declares APIs for the built-in Dynmaic Variant Objects.
 *
 * Copyright (c) 2022 FMSoft (http://www.fmsoft.cn)
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
#include <ctype.h>
#include <time.h>

#include "purc-macros.h"
#include "purc-variant.h"
#include "purc-dom.h"

struct purc_broken_down_url {
    char *schema;
    char *user;
    char *passwd;
    char *host;
    char *path;
    char *query;
    char *fragment;
    unsigned int port;
};

struct purc_hvml_ctrl_props {
    struct purc_broken_down_url base_url;

    uint64_t            max_iteration_count;
    unsigned short      max_recursion_depth;
    unsigned short      max_embedded_levels;

    struct timespec     timeout;
};

struct purc_dvobj_method {
    const char          *name;
    purc_dvariant_method getter;
    purc_dvariant_method setter;
};

PCA_EXTERN_C_BEGIN

/**
 * @defgroup DVObjs Functions for Dynamic Variant Objects
 * @{
 */

PCA_EXPORT purc_variant_t
purc_dvobj_make_from_methods(const struct purc_dvobj_method *method,
        size_t size);

PCA_EXPORT purc_variant_t
purc_dvobj_system_new(void);

PCA_EXPORT purc_variant_t
purc_dvobj_session_new(void);

PCA_EXPORT purc_variant_t
purc_dvobj_datetime_new(void);

PCA_EXPORT purc_variant_t
purc_dvobj_hvml_new(struct purc_hvml_ctrl_props **ctrl_props);

PCA_EXPORT purc_variant_t
purc_dvobj_doc_new(struct pcdom_document *doc);

PCA_EXPORT purc_variant_t
purc_dvobj_ejson_new(void);

PCA_EXPORT purc_variant_t
purc_dvobj_logical_new(void);

PCA_EXPORT purc_variant_t
purc_dvobj_text_new(void);

PCA_EXPORT purc_variant_t
purc_dvobj_string_new(void);

PCA_EXPORT purc_variant_t
purc_dvobj_url_new(void);

/**@}*/

PCA_EXTERN_C_END

#endif /* !PURC_PURC_DVOBJS_H */

