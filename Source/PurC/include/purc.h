/**
 * @file purc.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/02
 * @brief The main header file of PurC.
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

#ifndef PURC_PURC_H
#define PURC_PURC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "purc-macros.h"
#include "purc-version.h"
#include "purc-errors.h"
#include "purc-variant.h"
#include "purc-rwstream.h"

typedef struct purc_instance_extra_info {
    int foo;
    int bar;
} purc_instance_extra_info;

typedef struct purc_hvml_extra_info {
    int foo;
    int bar;
} purc_hvml_extra_info;

PCA_EXTERN_C_BEGIN

PCA_EXPORT int
purc_init (const char* app_name, const char* runner_name,
        const purc_instance_extra_info* extra_info);

PCA_EXPORT bool
purc_cleanup (void);

PCA_EXPORT bool
purc_bind_session_variable (const char* name, purc_variant_t variant);

struct pcvdom_tree;
typedef struct pcvdom_tree  pcvdom_tree;
typedef struct pcvdom_tree* pcvdom_tree_t;

PCA_EXPORT pcvdom_tree_t
purc_load_hvml_from_string (const char* string);

PCA_EXPORT pcvdom_tree_t
purc_load_hvml_from_file (const char* file);

PCA_EXPORT pcvdom_tree_t
purc_load_hvml_from_url (const char* url);

PCA_EXPORT pcvdom_tree_t
purc_load_hvml_from_rwstream (purc_rwstream_t stream);

PCA_EXPORT bool
purc_bind_document_variable (pcvdom_tree_t vdom, const char* name, purc_variant_t variant);

PCA_EXPORT bool
purc_register_hvml_to_renderer (pcvdom_tree_t vdom,
        const char* type, const char* name,
        const purc_hvml_extra_info* extra_info);

typedef int (*purc_event_handler) (pcvdom_tree_t vdom, purc_variant_t event);

PCA_EXPORT bool
purc_run (purc_variant_t request, purc_event_handler handler);

PCA_EXTERN_C_END

#endif /* not defined PURC_PURC_H */
