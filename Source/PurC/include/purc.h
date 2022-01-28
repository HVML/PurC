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
#include "purc-ports.h"
#include "purc-utils.h"
#include "purc-rwstream.h"
#include "purc-variant.h"
#include "purc-dom.h"
#include "purc-html.h"

typedef struct purc_instance_extra_info {
    const char *renderer_uri;
    bool enable_remote_fetcher;
} purc_instance_extra_info;

PCA_EXTERN_C_BEGIN

#define PURC_HAVE_UTILS         0x0001
#define PURC_HAVE_DOM           0x0002
#define PURC_HAVE_HTML          0x0004
#define PURC_HAVE_XML           0x0008
#define PURC_HAVE_VARIANT       0x0010
#define PURC_HAVE_EJSON         0x0020
#define PURC_HAVE_XGML          0x0040
#define PURC_HAVE_HVML          0x0080

#define PURC_MODULE_UTILS      (PURC_HAVE_UTILS)
#define PURC_MODULE_DOM        (PURC_MODULE_UTILS    | PURC_HAVE_DOM)
#define PURC_MODULE_HTML       (PURC_MODULE_DOM      | PURC_HAVE_HTML)
#define PURC_MODULE_XML        (PURC_MODULE_DOM      | PURC_HAVE_XML)
#define PURC_MODULE_VARIANT    (PURC_MODULE_UTILS    | PURC_HAVE_VARIANT)
#define PURC_MODULE_EJSON      (PURC_MODULE_VARIANT  | PURC_HAVE_EJSON)
#define PURC_MODULE_XGML       (PURC_MODULE_EJSON    | PURC_HAVE_XGML)
#define PURC_MODULE_HVML       (PURC_MODULE_EJSON    | PURC_HAVE_HVML)
#define PURC_MODULE_ALL         0xFFFF

/**
 * purc_init_ex:
 *
 * @modules: The modules will be initialized, can be OR'd with one or more
 *      the following values:
 *  - @PURC_MODULE_UTILS: Helpers and utilities.
 *  - @PURC_MODULE_DOM: DOM construction.
 *  - @PURC_MODULE_HTML: HTML Parser.
 *  - @PURC_MODULE_XML: XML Parser.
 *  - @PURC_MODULE_VARIANT: Variant.
 *  - @PURC_MODULE_EJSON: eJSON parser.
 *  - @PURC_MODULE_XGML: XGML Parser (not implemented).
 *  - @PURC_MODULE_ALL: All modules including HVML parser and interpreter.
 * @app_name: a pointer to the string contains the app name.
 *      If this argument is null, the executable program name of the command
 *      line will be used for the app name.
 * @runner_name: a pointer to the string contains the runner name.
 *      If this argument is null, `unknown` will be used for the runner name.
 * @extra_info: a pointer (nullable) to the extra information for the new
 *      PurC instance, e.g., the URI of the renderer.
 *
 * Initializes individual PurC modules and/or a new PurC instance for
 * the current thread, and creates a new renderer session for this PurC instance.
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
 * Since 0.0.2
 */
PCA_EXPORT int
purc_init_ex(unsigned int modules,
        const char* app_name, const char* runner_name,
        const purc_instance_extra_info* extra_info);

/**
 * purc_init:
 *
 * @app_name: a pointer to the string contains the app name.
 *      If this argument is null, the executable program name of the command
 *      line will be used for the app name.
 * @runner_name: a pointer to the string contains the runner name.
 *      If this argument is null, `unknown` will be used for the runner name.
 * @extra_info: a pointer (nullable) to the extra information for the new
 *      PurC instance, e.g., the URI of the renderer.
 *
 * Initializes a new PurC instance for the current thread, and creates a new
 * renderer session for this PurC instance.
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
static inline int purc_init(const char* app_name, const char* runner_name,
        const purc_instance_extra_info* extra_info)
{
    return purc_init_ex(PURC_MODULE_ALL, app_name, runner_name, extra_info);
}

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
purc_set_local_data(const char* data_name, uintptr_t locale_data,
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
 * Returns: The number of entries removed, 0 means no entry found. -1 on error.
 *
 * Since 0.0.1
 */
PCA_EXPORT ssize_t
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
 * Returns: 1 for success; 0 for no entry found; -1 on error.
 *
 * Since 0.0.1
 */
PCA_EXPORT int
purc_get_local_data(const char* data_name, uintptr_t *local_data,
        cb_free_local_data* cb_free);

/**
 * purc_bind_variable:
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
purc_bind_variable(const char* name, purc_variant_t variant);

struct purc_vdom;
typedef struct purc_vdom  purc_vdom;
typedef struct purc_vdom* purc_vdom_t;

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
PCA_EXPORT purc_vdom_t
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
PCA_EXPORT purc_vdom_t
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
PCA_EXPORT purc_vdom_t
purc_load_hvml_from_url(const char* url);

/**
 * purc_load_hvml_from_rwstream:
 *
 * @stream: The purc_rwstream object.
 *
 * Loads a HVML program from the specified purc_rwstream object.
 *
 * Returns: A valid pointer to the vDOM tree for success; @NULL for failure.
 *
 * Since 0.0.1
 */
PCA_EXPORT purc_vdom_t
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
purc_bind_document_variable(purc_vdom_t vdom, const char* name,
        purc_variant_t variant);

typedef struct purc_window_info {
    const char *classes;
    const char *box_styles;
} purc_window_info;

/**
 * purc_attach_vdom_to_renderer:
 *
 * @vdom: The vDOM entity returned by @purc_load_hvml_from_rwstream or
 *      its brother functions.
 *
 * Attaches a vDOM tree to a plain window or a tabbed page in
 * the connected renderer.
 *
 * Returns: the error code:
 *  - @PURC_ERROR_OK: success
 *  - TODO
 *
 * Since 0.0.1
 */
PCA_EXPORT bool
purc_attach_vdom_to_renderer(purc_vdom_t vdom,
        const char *target_workspace, const char* workspace_name,
        const char *target_window, const char *window_name,
        const char *target_page, const char *page_name,
        purc_window_info *window_info);

typedef int (*purc_event_handler)(purc_vdom_t vdom, purc_variant_t event);

/**
 * purc_run:
 *
 * @request: The variant which will be used as the request data.
 * @handler: The pointer to a call-back function which handles
 *      the session events.
 *
 * Runs all HVML programs which are ready in the current PurC instance.
 *
 * Returns: @true for success; @false for failure.
 *
 * Since 0.0.1
 */
PCA_EXPORT bool
purc_run(purc_variant_t request, purc_event_handler handler);

PCA_EXTERN_C_END

#endif /* not defined PURC_PURC_H */

