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
#include "purc-features.h"
#include "purc-errors.h"
#include "purc-rwstream.h"
#include "purc-variant.h"
#include "purc-ports.h"
#include "purc-utils.h"

typedef struct purc_instance_extra_info {
    int foo;
    int bar;
} purc_instance_extra_info;

typedef struct purc_rdr_extra_info {
    int foo;
    int bar;
} purc_rdr_extra_info;

PCA_EXTERN_C_BEGIN

/**
 * purc_init:
 *
 * @app_name: a pointer to the string contains the app name.
 *      If this argument is null, the executable program name of the command
 *      line will be used for the app name.
 * @runner_name: a pointer to the string contains the runner name.
 *      If this argument is null, `unknown` will be used for the runner name.
 * @extra_info: a pointer (nullable) to the extra information for
 *      the new PurC instance.
 *
 * Initializes a new PurC instance for the current thread.
 *
 * Returns: the error code:
 *  - @PURC_ERROR_OK: success
 *  - @PURC_ERROR_DUPLICATED: duplicated call of this function.
 *  - @PURC_ERROR_OUT_OF_MEMORY: Out of memory.
 *
 * Note that this function is the only one which returns the error code
 * directly. Because if it fails, there is no any space to store
 * the error code.
 *
 * Since 0.0.1
 */
PCA_EXPORT int
purc_init(const char* app_name, const char* runner_name,
        const purc_instance_extra_info* extra_info);

/**
 * purc_cleanup:
 *
 * Cleans up the PurC instance attached to the current thread.
 *
 * Returns: @true for success; @false for no PurC instance for
 *      the current thread.
 *
 * Since 0.0.1
 */
PCA_EXPORT bool
purc_cleanup(void);

typedef void (*cb_free_local_data) (void *local_data);

/**
 * purc_set_local_data:
 *
 * @param data_name: The name of the local data.
 * @param local_data: The value of the local data.
 * @param cb_free: A callback function which will be called automatically
 *  by PurC to free the local data when the PurC instance is being destroyed
 *  or the local data is being removed or overwritten. If it is NULL,
 *  the system does nothing to the local data.
 *
 * This function sets the local data as @local_data which is bound with the
 * name @data_name for the current PurC instance. If you passed a non-NULL
 * function pointer for \a cb_free, the system will call this function to free
 * the local data when you clean up the instance, remove the local data, or
 * when you call this function to overwrite the old local data for the name.
 *
 * PurC uses the following local data for some functions:
 *
 *  - `format-double`: This local data contains the format (should be a
 *     pointer to a static string) which will be used to serilize a variant
 *     of double (number) type. If not defined, use the default format
 *     (`%.17g`).
 *  - `format-long-double`: This local data contains the format (should be
 *     a pointer to a static string), which will be  used to serilize a
 *     variant of long double type. If not defined, use the default format
 *     (%.17Lg).
 *
 * Returns: @true for success; @false on error.
 *
 * Since 0.0.1
 */
PCA_EXPORT bool
purc_set_local_data(const char* data_name, void *locale_data,
        cb_free_local_data cb_free);

/**
 * purc_remove_local_data:
 *
 * Remove the local data bound with a name.
 *
 * \param data_name The name for the local data.
 *
 * This function removes the local data which is bound with the
 * name \a data_name for the current PurC instance. When you pass NULL
 * for \a data_name, this function will remove all local data of it.
 * Note that this function will call the callback procedure for releasing
 * the local data, if you had set it, when removing the local data.
 *
 * Returns: @true for success; @false on error.
 *
 * Since 0.0.1
 */
PCA_EXPORT bool
purc_remove_local_data(const char* data_name);

/**
 * purc_get_local_data:
 *
 * Retrieve the local data bound with a name.
 *
 * This function retrieves the local data which is bound with the
 * name \a data_name for the current PurC instance.
 *
 * @param data_name: The name for the local data.
 * @param local_data: The pointer to a uinptr_t variable to return
 *  the local data.
 * @param cb_free: The pointer to a cb_free_local_data variable to return
 *  the pointer to the callback function which is used to free the local data.
 *
 * Returns: @true for success; @false on error.
 *
 * Since 0.0.1
 */
PCA_EXPORT bool
purc_get_local_data(const char* data_name, void **local_data,
        cb_free_local_data* cb_free);

/**
 * purc_bind_session_variable:
 *
 * @name: The pointer to the string contains the name for the variable.
 * @variant: The variant.
 *
 * Binds a variant value as the session-level variable.
 *
 * Returns: @true for success; @false for failure.
 *
 * Since 0.0.1
 */
PCA_EXPORT bool
purc_bind_session_variable(const char* name, purc_variant_t variant);

struct pcvdom_tree;
typedef struct pcvdom_tree  pcvdom_tree;
typedef struct pcvdom_tree* pcvdom_tree_t;

/**
 * purc_load_hvml_from_string:
 *
 * @string: The pointer to the string contains the HVML docment.
 *
 * Loads a HVML program from a string.
 *
 * Returns: A valid pointer to the vDOM tree for success; @NULL for failure.
 *
 * Since 0.0.1
 */
PCA_EXPORT pcvdom_tree_t
purc_load_hvml_from_string(const char* string);

/**
 * purc_load_hvml_from_file:
 *
 * @file: The pointer to the string contains the file name.
 *
 * Loads a HVML program from a file.
 *
 * Returns: A valid pointer to the vDOM tree for success; @NULL for failure.
 *
 * Since 0.0.1
 */
PCA_EXPORT pcvdom_tree_t
purc_load_hvml_from_file(const char* file);

/**
 * purc_load_hvml_from_url:
 *
 * @url: The pointer to the string contains the URL.
 *
 * Loads a HVML program from the speicifed URL.
 *
 * Returns: A valid pointer to the vDOM tree for success; @NULL for failure.
 *
 * Since 0.0.1
 */
PCA_EXPORT pcvdom_tree_t
purc_load_hvml_from_url(const char* url);

/**
 * purc_load_hvml_from_rwstream:
 *
 * @stream: The RWSTream object.
 *
 * Loads a HVML program from the specified RWStream object.
 *
 * Returns: A valid pointer to the vDOM tree for success; @NULL for failure.
 *
 * Since 0.0.1
 */
PCA_EXPORT pcvdom_tree_t
purc_load_hvml_from_rwstream(purc_rwstream_t stream);

/**
 * purc_bind_document_variable:
 *
 * @name: The pointer to the string contains the name for the variable.
 * @variant: The variant.
 *
 * Binds a variant value as the document-level variable of the specified vDOM.
 *
 * Returns: @true for success; @false for failure.
 *
 * Since 0.0.1
 */
PCA_EXPORT bool
purc_bind_document_variable(pcvdom_tree_t vdom, const char* name, purc_variant_t variant);

/**
 * purc_connnect_vdom_to_renderer:
 *
 * @vdom: The pointer to the string contains the name for the variable.
 * @type: The pointer to the string contains the type of the expected renderer.
 * @classes: The pointer to the string contains the classes of the expected renderer.
 * @name: The pointer to the string contains the name of the expected renderer.
 * @extra_info: The structure pointer to the extra information of the expected renderer.
 *
 * Connects a vDOM tree to an renderer.
 *
 * Returns: @true for success; @false for failure.
 *
 * Since 0.0.1
 */
PCA_EXPORT bool
purc_connnect_vdom_to_renderer(pcvdom_tree_t vdom,
        const char* type, const char* name,
        const purc_rdr_extra_info* extra_info);

typedef int (*purc_event_handler)(pcvdom_tree_t vdom, purc_variant_t event);

/**
 * purc_run:
 *
 * @request: The variant which will be used for request data.
 * @handler: The pointer to a call-back function which handles
 *      the session events.
 *
 * Runs all HVML programs which has connected to a renderer in
 * the current PurC instance.
 *
 * Returns: @true for success; @false for failure.
 *
 * Since 0.0.1
 */
PCA_EXPORT bool
purc_run(purc_variant_t request, purc_event_handler handler);

PCA_EXTERN_C_END

#endif /* not defined PURC_PURC_H */
