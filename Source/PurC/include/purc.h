/**
 * @file purc.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/02
 * @brief The main header file of PurC.
 *
 * Copyright (C) 2021, 2022 FMSoft <https://www.fmsoft.cn>
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
#include "purc-document.h"
#include "purc-helpers.h"
#include "purc-pcrdr.h"
#include "purc-dvobjs.h"
#include "purc-executor.h"
#include "purc-runloop.h"
#include "purc-fetcher.h"

/**
 * purc_instance_extra_info:
 *
 * The structure defines the extra information for a new PurC instance.
 */
typedef struct purc_instance_extra_info {
    /**
     * The renderer protocol, one of the following values:
     *  - PURC_RDRCOMM_HEADLESS:
     *      No renderer.
     *  - PURC_RDRCOMM_THREAD:
     *      The renderer runs as a thread within the current process.
     *  - PURC_RDRCOMM_SOCKET:
     *      The renderer runs as a server and uses socket
     *      (local socket or WebSocket) as the communication method.
     *  - PURC_RDRCOMM_HBDBUS:
     *      The renderer runs as a HBDBus endpoint and uses HBDBus.
     */
    purc_rdrcomm_k  renderer_comm;

    /**
     * When using an HEADLESS renderer, you should specify a file
     * or a named pipe (FIFO), like `file:///var/tmp/purc-foo-bar-msgs.log`.
     *
     * When using a THREAD renderer, you should specify the endpoint name
     * of the renderer like `edpt://localhost/<app_name>/<runner_name>`.
     * The endpoint name will be used to communicate between the renderer
     * and the interperter instances.
     *
     * When using a SOCKET renderer, you can specify a UNIX domain socket
     * or a URI of WebSocket:
     *
     *      - UNIX domain socket: `unix:///var/tmp/xxx.sock`
     *      - WebSocket: `ws://foo.bar.com:8877`
     *      - Secured WebSocket: `wss://foo.bar.com:8877`
     */
    const char      *renderer_uri;

    /** The SSL certification if using Secured WebSocket. */
    const char      *ssl_cert;

    /** The SSL key if using Secured WebSocket. */
    const char      *ssl_key;

    /** The default workspace of this instance. */
    const char      *workspace_name;

    /** The title of the workspace. */
    const char      *workspace_title;

    /**
     * The HTML contents defining the layout of the windows or widgets which
     * will render the uDOMs in the default workspace.
     */
    const char      *workspace_layout;

    /**
     * Whether allow switching the renderer (Since 0.9.18).
     */
    unsigned int    allow_switching_rdr:1;

    /**
     * Whether allow scaling by density.
     */
    unsigned int    allow_scaling_by_density:1;

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
#define PURC_HAVE_PCRDR         0x0100
#define PURC_HAVE_FETCHER       0x0200
#define PURC_HAVE_FETCHER_R     0x0400
#define PURC_HAVE_ALL (                 \
            PURC_HAVE_UTILS         |   \
            PURC_HAVE_DOM           |   \
            PURC_HAVE_HTML          |   \
            PURC_HAVE_XML           |   \
            PURC_HAVE_VARIANT       |   \
            PURC_HAVE_EJSON         |   \
            PURC_HAVE_XGML          |   \
            PURC_HAVE_HVML          |   \
            PURC_HAVE_PCRDR         |   \
            PURC_HAVE_FETCHER       |   \
            PURC_HAVE_FETCHER_R)

#define PURC_MODULE_UTILS      (PURC_HAVE_UTILS)
#define PURC_MODULE_DOM        (PURC_MODULE_UTILS    | PURC_HAVE_DOM)
#define PURC_MODULE_HTML       (PURC_MODULE_DOM      | PURC_HAVE_HTML)
#define PURC_MODULE_XML        (PURC_MODULE_DOM      | PURC_HAVE_XML)
#define PURC_MODULE_VARIANT    (PURC_MODULE_UTILS    | PURC_HAVE_VARIANT)
#define PURC_MODULE_EJSON      (PURC_MODULE_VARIANT  | PURC_HAVE_EJSON)
#define PURC_MODULE_XGML       (PURC_MODULE_EJSON    | PURC_HAVE_XGML)
#define PURC_MODULE_PCRDR      (PURC_MODULE_EJSON    | PURC_HAVE_PCRDR)
#define PURC_MODULE_HVML       (PURC_MODULE_PCRDR    | PURC_HAVE_HVML | \
            PURC_HAVE_FETCHER)
#define PURC_MODULE_ALL         0xFFFF

/**
 * purc_init_ex:
 *
 * @modules: The modules will be initialized, can be OR'd with one or more of
 *      the following values:
 *  - %PURC_MODULE_UTILS: Helpers and utilities.
 *  - %PURC_MODULE_DOM: DOM construction.
 *  - %PURC_MODULE_HTML: HTML Parser.
 *  - %PURC_MODULE_XML: XML Parser (not implemented so far).
 *  - %PURC_MODULE_VARIANT: Variant.
 *  - %PURC_MODULE_EJSON: eJSON parser.
 *  - %PURC_MODULE_XGML: XGML Parser (not implemented so far).
 *  - %PURC_MODULE_PCRDR: Communication with renderer.
 *  - %PURC_MODULE_ALL: All modules including HVML parser and interpreter.
 * @app_name (nullable): A pointer to a null-terminated string contains
 *      the app name. If this argument is null, the executable program name of
 *      the command line will be used for the app name.
 * @runner_name (nullable): A pointer to a null-terminated string contains
 *      the runner name. If this argument is null, `unknown` will be used for
 *      the runner name.
 * @extra_info (nullable): A pointer to #purc_instance_extra_info structure
 *      which defines the extra information for the new PurC instance, e.g.,
 *      the URI of the renderer.
 *
 * Initializes individual PurC modules or a new complete PurC instance for
 * the current system thread, and creates a new renderer session for this
 * PurC instance if %PURC_MODULE_PCRDR is specified.
 *
 * Returns: the error code:
 *  - %PURC_ERROR_OK: success
 *  - %PURC_ERROR_DUPLICATED: duplicated call of this function.
 *  - %PURC_ERROR_OUT_OF_MEMORY: Out of memory.
 *
 * Note that this function is the only one which returns the error code
 * directly. Because if it fails, there is no any space to store
 * the error code.
 *
 * Since 0.0.2
 */
PCA_EXPORT int
purc_init_ex(unsigned int modules,
        const char *app_name, const char *runner_name,
        const purc_instance_extra_info *extra_info);

/**
 * purc_init:
 *
 * @app_name (nullable): A pointer to a null-terminated string contains
 *      the app name. If this argument is null, the executable program name
 *      of the command line will be used for the app name.
 * @runner_name (nullable): A pointer to a null-terminated string contains
 *      the runner name. If this argument is null, `unknown` will be used for
 *      the runner name.
 * @extra_info (nullable): A pointer to #purc_instance_extra_info structure
 *      which defines the extra information for the new PurC instance, e.g.,
 *      the URI of the renderer.
 *
 * Initializes a new PurC instance for the current thread, and creates a new
 * renderer session for this PurC instance.
 *
 * Returns: the error code:
 *  - %PURC_ERROR_OK: success
 *  - %PURC_ERROR_DUPLICATED: duplicated call of this function.
 *  - %PURC_ERROR_OUT_OF_MEMORY: Out of memory.
 *
 * Note that this function is the only one which returns the error code
 * directly. Because if it fails, there is no any space to store
 * the error code.
 *
 * Since 0.0.1
 */
static inline int purc_init(const char *app_name, const char *runner_name,
        const purc_instance_extra_info *extra_info)
{
    return purc_init_ex(PURC_MODULE_ALL, app_name, runner_name, extra_info);
}

/**
 * purc_cleanup:
 *
 * Cleans up the PurC instance attached to the current thread.
 *
 * Returns: %true for success; %false for no PurC instance for
 *      the current thread.
 *
 * Since 0.0.1
 */
PCA_EXPORT bool
purc_cleanup(void);

/**
 * purc_get_endpoint:
 *
 * @atom (nullable): A pointer to a purc_atom_t buffer to receive the endpoint
 *      atom of the current PurC instance.

 * Gets the endpoint name and its atom value of the current PurC instance.
 *
 * Returns: The endpoint name for success; %NULL for no PurC instance for
 *      the current thread.
 *
 * Since 0.2.0
 */
PCA_EXPORT const char *
purc_get_endpoint(purc_atom_t *atom);

#define PURC_LDNAME_RANDOM_DATA     "random-data"
#define PURC_LDNAME_FORMAT_DOUBLE   "format-double"
#define PURC_LDNAME_FORMAT_LDOUBLE  "format-long-double"
#define PURC_LDNAME_PARSE_ERROR     "parse-error"

typedef void (*cb_free_local_data) (void *key, void *local_data);

/**
 * purc_set_local_data:
 *
 * @data_name: The name of the local data.
 * @local_data: The value of the local data.
 * @cb_free: A callback function which will be called automatically
 *  by PurC to free the local data when the PurC instance is being destroyed
 *  or the local data is being removed or overwritten. If it is %NULL,
 *  the system does nothing to the local data.
 *
 * This function sets the local data as @local_data which is bound with the
 * name @data_name for the current PurC instance. If you passed a non-NULL
 * function pointer for @cb_free, the system will call this function to free
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
 *     a pointer to a static string), which will be used to serilize a
 *     variant of long double type. If not defined, use the default format
 *     (%.17Lg).
 *
 * Returns: %true for success; %false on error.
 *
 * Since 0.0.1
 */
PCA_EXPORT bool
purc_set_local_data(const char* data_name, uintptr_t local_data,
        cb_free_local_data cb_free);

/**
 * purc_remove_local_data:
 *
 * @data_name The name for the local data.
 *
 * Removes the local data bound with the given name.
 *
 * This function removes the local data which is bound with the
 * name given by @data_name for the current PurC instance. When you pass %NULL
 * for @data_name, this function will remove all local data bound with the
 * current PurC instance.
 *
 * Note that this function will call the callback procedure for releasing
 * the local data, if you had set it, when removing the local data.
 *
 * Returns: The number of entries removed, 0 means no entry found. -1 on error.
 *
 * Since 0.0.1
 */
PCA_EXPORT ssize_t
purc_remove_local_data(const char *data_name);

/**
 * purc_get_local_data:
 *
 * @data_name: The name for the local data.
 * @local_data: The pointer to a uinptr_t variable to return
 *  the local data.
 * @cb_free: The pointer to a cb_free_local_data variable to return
 *  the pointer to the callback function which is used to free the local data.
 *
 * Retrieves the local data bound with a name.
 *
 * This function retrieves the local data which is bound with the
 * name given by @data_name for the current PurC instance.
 *
 * Returns: 1 for success; 0 for no entry found; -1 on error.
 *
 * Since 0.0.1
 */
PCA_EXPORT int
purc_get_local_data(const char *data_name, uintptr_t *local_data,
        cb_free_local_data *cb_free);

/**
 * purc_bind_runner_variable:
 *
 * @name: The pointer to the string contains the name for the variable.
 * @variant: The variant.
 *
 * Binds a variant value as the runner-level variable for the current PurC
 * instance.
 *
 * Returns: %true for success; %false for failure.
 *
 * Since 0.0.1
 */
PCA_EXPORT bool
purc_bind_runner_variable(const char *name, purc_variant_t variant);

/**
 * purc_get_runner_variable:
 *
 * @name: The pointer to the string contains the name for the variable.
 *
 * Retrieves a runner-level variable of the current PurC instance.
 *
 * Returns: The variant on success or %PURC_VARIANT_INVALID for failure.
 *
 * Since 0.2.0
 */
PCA_EXPORT purc_variant_t
purc_get_runner_variable(const char *name);

struct pcvdom_document;
typedef struct pcvdom_document *purc_vdom_t;

/**
 * purc_load_hvml_from_string:
 *
 * @string: The pointer to a null-terminated string which contains
 *      an HVML program.
 *
 * Loads an HVML program from a string.
 *
 * Returns: A valid pointer to the vDOM tree for success; %NULL for failure.
 *
 * Since 0.0.1
 */
PCA_EXPORT purc_vdom_t
purc_load_hvml_from_string(const char *string);

/**
 * purc_load_hvml_from_file:
 *
 * @file: The pointer to a null-terminated string which contains the file name.
 *
 * Loads an HVML program from a file.
 *
 * Returns: A valid pointer to the vDOM tree for success; %NULL for failure.
 *
 * Since 0.0.1
 */
PCA_EXPORT purc_vdom_t
purc_load_hvml_from_file(const char *file);

/**
 * purc_load_hvml_from_url:
 *
 * @url: The pointer to a null-terminated string which contains the URL.
 *
 * Loads an HVML program from the speicifed URL.
 *
 * Returns: A valid pointer to the vDOM tree for success; %NULL for failure.
 *
 * Since 0.0.1
 */
PCA_EXPORT purc_vdom_t
purc_load_hvml_from_url(const char *url);

/**
 * purc_load_hvml_from_rwstream:
 *
 * @stream: A purc_rwstream object.
 *
 * Loads an HVML program from the specified #purc_rwstream object.
 *
 * Returns: A valid pointer to the vDOM tree for success; %NULL for failure.
 *
 * Since 0.0.1
 */
PCA_EXPORT purc_vdom_t
purc_load_hvml_from_rwstream(purc_rwstream_t stream);

/**
 * purc_get_conn_to_renderer:
 *
 * Retrieves the connection to the renderer of the current PurC instance.
 *
 * Returns: the pointer to the connection structure; %NULL for failure.
 *
 * Since 0.1.0
 */
PCA_EXPORT struct pcrdr_conn *
purc_get_conn_to_renderer(void);

/**
 * purc_renderer_extra_info:
 *
 * The extra renderer information.
 */
typedef struct purc_renderer_extra_info {
    /** The class for layout of the widget */
    const char *klass;

    /** The title of the widget */
    const char *title;

    /**
     * The layout styles of the page. For a standalone (not-grouped) page
     * (a plain window), we can use `window-size` and `window-position` to
     * specify the size and the position like `background-size` and
     * `background-position` in CSS:
     *
     * `window-size:50% 480px;window-position:center;`
     *
     * For a grouped page (a plain window or a widget), we use the standard
     * CSS styles, for example: `width:200px; height:auto;`.
     */
    const char *layout_style;

    /**
     * The transition styles of the page. For a standalone (not-grouped) page
     * (a plain window), we can use `transition-style` specify the transition
     * like `transition` in CSS:
     *
     * `window-transition-move: linear 200`
     *
     * Since: 0.9.18
     */
    const char *transition_style;

    /**
     * The toolkit style of the page (an object variant).
     * The definition of this field depends on the renderer.
     */
    purc_variant_t toolkit_style;

    /** The page groups to add to the layout DOM */
    const char *page_groups;

    /**
     * The keep contents flag which used to inform the renderer to
     * preserve the page content.
     *
     * Since: 0.9.22
     */
    purc_variant_t keep_contents;
} purc_renderer_extra_info;

/**
 * pcrdr_page_type_k:
 *
 * The rendere page type.
 */
typedef enum pcrdr_page_type {
    PCRDR_PAGE_TYPE_first = 0,

#define PCRDR_PAGE_TYPE_NAME_NULL       "null"
    /** Do not create or use any page for the HVML coroutine. */
    PCRDR_PAGE_TYPE_NULL = PCRDR_PAGE_TYPE_first,

#define PCRDR_PAGE_TYPE_NAME_INHERIT    "inherit"
    /** Use the document and page of curator. */
    PCRDR_PAGE_TYPE_INHERIT,

#define PCRDR_PAGE_TYPE_NAME_SELF       "self"
    /** Use the page of curator. */
    PCRDR_PAGE_TYPE_SELF,

#define PCRDR_PAGE_TYPE_NAME_PLAINWIN   "plainwin"
    /** Create a new plain window in the specified page group. */
    PCRDR_PAGE_TYPE_PLAINWIN,

#define PCRDR_PAGE_TYPE_NAME_WIDGET     "widget"
    /** Create a new widget in the specified page group. */
    PCRDR_PAGE_TYPE_WIDGET,

    PCRDR_PAGE_TYPE_last = PCRDR_PAGE_TYPE_WIDGET,
} pcrdr_page_type_k;

#define PCRDR_PAGE_TYPE_nr \
    (PCRDR_PAGE_TYPE_last - PCRDR_PAGE_TYPE_first + 1)

struct pcintr_coroutine;
typedef struct pcintr_coroutine *purc_coroutine_t;

/**
 * purc_schedule_vdom:
 *
 * @vdom: The vDOM entity returned by @purc_load_hvml_from_rwstream or
 *  its brother functions.
 * @curator: The curator of the new coroutine, i.e., the coroutine which is
 *  waiting for the the execute result of the new coroutine; 0 for no curator.
 * @request: The request variant for the new coroutine.
 * @page_type: the target renderer page type.
 * @target_workspace: The name of the target renderer workspace.
 * @target_group: The identifier of the target group (nullable) in the layout
 *  HTML contents. When %NULL given, the renderer will create an ungrouped
 *  plain window for this coroutine.
 * @page_name: The page name which is used to show the contents (nullable).
 *  When %NULL given, use `main` as the default one.
 * @extra_info: The extra renderer information.
 * @body_id: The identifier of the `body` element as the entry in @vdom.
 * @user_data: The pointer to the initial user data.
 *
 * Creates a new coroutine to run the specified vDOM.
 * If success, the new coroutine will be in READY state.
 *
 * Returns: The pointer to the new coroutine, %NULL for error.
 *
 * Since 0.2.0
 */
PCA_EXPORT purc_coroutine_t
purc_schedule_vdom(purc_vdom_t vdom,
        purc_atom_t curator, purc_variant_t request,
        pcrdr_page_type_k page_type, const char *target_workspace,
        const char *target_group, const char *page_name,
        purc_renderer_extra_info *extra_info, const char *body_id,
        void *user_data);

static inline purc_coroutine_t
purc_schedule_vdom_null(purc_vdom_t vdom)
{
    return purc_schedule_vdom(vdom, 0, PURC_VARIANT_INVALID,
            PCRDR_PAGE_TYPE_NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/**
 * purc_coroutine_set_user_data:
 *
 * @cor: The pointer to a coroutine structure which representing a coroutine.
 * @user_data: The pointer to the user data.
 *
 * Sets the user data of the specific coroutine and returns the old one.
 *
 * Returns: The pointer to the old user data.
 *
 * Since 0.2.0
 */
PCA_EXPORT void *
purc_coroutine_set_user_data(purc_coroutine_t cor, void *user_data);

/**
 * purc_coroutine_get_user_data:
 *
 * @cor: The pointer to a coroutine structure which representing a coroutine.
 *
 * Gets the user data of the specific coroutine and returns the old one.
 * Binds a variant as the coroutine-level variable of the specified
 * coroutine.
 *
 * Gets the user data of the specific coroutine.
 *
 * Returns: The pointer to the user data attached to the coroutine.
 *
 * Since 0.2.0
 */
PCA_EXPORT void *
purc_coroutine_get_user_data(purc_coroutine_t cor);

/**
 * purc_coroutine_identifier:
 *
 * @cor: The pointer to a coroutine structure which representing a coroutine.
 *
 * Gets the coroutine identifier (cid) of the specified coroutine.
 *
 * Returns: The cid of the coroutine.
 *
 * Since 0.2.0
 */
PCA_EXPORT purc_atom_t
purc_coroutine_identifier(purc_coroutine_t cor);

/**
 * purc_coroutine_bind_variable:
 *
 * @cor: The pointer to a coroutine structure which representing a coroutine.
 * @name: The pointer to the string contains the name for the variable.
 * @variant: The variant.
 *
 * Binds a variant as the coroutine-level variable of the specified
 * coroutine.
 *
 * Returns: %true for success; %false for failure.
 *
 * Since 0.2.0
 */
PCA_EXPORT bool
purc_coroutine_bind_variable(purc_coroutine_t cor, const char *name,
        purc_variant_t variant);

/**
 * purc_coroutine_unbind_variable:
 *
 * @cor: The pointer to a coroutine structure which representing a coroutine.
 * @name: The pointer to the string contains the name for the variable.
 *
 * Unbinds a coroutine-level variable from the specified coroutine.
 *
 * Returns: %true for success; %false for failure.
 *
 * Since 0.2.0
 */
PCA_EXPORT bool
purc_coroutine_unbind_variable(purc_coroutine_t cor, const char *name);

/**
 * purc_coroutine_get_variable:
 *
 * @cor: The pointer to a coroutine structure which representing a coroutine.
 * @name: The pointer to the string contains the name for the variable.
 *
 * Retrieves a coroutine-level variable from the specified coroutine.
 *
 * Returns: The variant value on success; %PURC_VARIANT_INVALID for failure.
 *
 * Since 0.2.0
 */
PCA_EXPORT purc_variant_t
purc_coroutine_get_variable(purc_coroutine_t cor, const char *name);

/**
 * purc_coroutine_dump_stack:
 *
 * @cor: The pointer to a coroutine structure which representing a coroutine.
 * @stm: The stream to dump the stack frames.
 *
 * Dumps all stack frames (from bottommost to topmost) of
 * the specified coroutine to a stream.
 *
 * Returns: 0 for success, -1 for failure.
 *
 * Since 0.8.1
 */
PCA_EXPORT int
purc_coroutine_dump_stack(purc_coroutine_t cor, purc_rwstream_t stm);

struct purc_cor_run_info {
    unsigned long   run_idx;
    purc_variant_t  result;
    purc_document_t doc;
};

struct purc_cor_exit_info {
    purc_variant_t  result;
    purc_document_t doc;
};

struct purc_cor_term_info {
    purc_atom_t     except;
    purc_document_t doc;
};

/** The PurC instance conditions */
typedef enum purc_cond {
    /**
     * Indicating that the instance has started.
     * In the condition handler, `arg` is the atom value of the instance,
     * `data` is the pointer to the struct purc_instance_extra_info,
     * in which contains the extra information to start the instance.
     */
    PURC_COND_STARTED = 0,

    /**
     * Indicating that the instance has stopped.
     * In the condition handler, `arg` is the atom value of the instance,
     * `data` is NULL.
     */
    PURC_COND_STOPPED,

    /**
     * Indicating that there is no coroutine scheduled.
     * In the condition handler, `arg` and `data` both are NULL.
     */
    PURC_COND_NOCOR,

    /**
     * Indicating that there is no coroutine in ready state.
     * In the condition handler, `arg` and `data` both are NULL.
     */
    PURC_COND_IDLE,

    /**
     * Indicating that there is a new coroutine created.
     * In the condition handler, `arg` is the pointer to the coroutine structure,
     * `data` is the coroutine indentifier.
     */
    PURC_COND_COR_CREATED,

    /**
     * Indicating that a coroutine finished a round of run.
     * In the condition handler, `arg` is the pointer to the coroutine structure,
     * `data` is the pointer to a `struct purc_cor_run_info` which contains
     * the index of current run, the current executed result and
     * the target document genenerated by the HVML coroutine.
     */
    PURC_COND_COR_ONE_RUN,

    /**
     * Indicating that a coroutine exited.
     * In the condition handler, `arg` is the pointer to the coroutine structure,
     * `data` is the pointer to a `struct purc_cor_exit_info` which contains
     * the execute result and the target document genenerated by the
     * HVML coroutine.
     */
    PURC_COND_COR_EXITED,

    /**
     * Indicating that a coroutine terminated due to an exception or error.
     * In the condition handler, `arg` is the pointer to the coroutine structure,
     * `data` is the pointer to a `struct purc_cor_term_info` which contains
     * the termination information and the target document genenerated by the
     * HVML coroutine.
     */
    PURC_COND_COR_TERMINATED,

    /**
     * Indicating that PurC is destroying a coroutine.
     * In the condition handler, `arg` is the pointer to the coroutine structure,
     * `data` is the user data bound to the corontine.
     */
    PURC_COND_COR_DESTROYED,

    /**
     * Indicating that the PurC instance got an unknown request message.
     * In the condition handler, `arg` is the pointer to the request message,
     * `data` is the pointer to a response message initialized as
     * a `void` message.
     */
    PURC_COND_UNK_REQUEST,

    /**
     * Indicating that the PurC instance got an unknown event message.
     * In the condition handler, `arg` is the pointer to the event message,
     * `data` is NULL.
     */
    PURC_COND_UNK_EVENT,

    /**
     * Indicating that the PurC instance has been asked by another intance to
     * to shutdown.
     *
     * In the condition handler, `arg` is the pointer to the request message,
     * `data` is NULL. If allowed, the condition handler should returns 0.
     */
    PURC_COND_SHUTDOWN_ASKED,

} purc_cond_k;

typedef int (*purc_cond_handler)(purc_cond_k event, void *arg, void *data);

/**
 * purc_get_cond_handler:
 *
 * Returns: The pointer to the current condition handler;
 *      %PURC_INVPTR for error.
 *
 * Since 0.2.0
 */
PCA_EXPORT purc_cond_handler
purc_get_cond_handler(void);

/**
 * purc_set_cond_handler:
 *
 * @handler: The pointer to a call-back function which handles
 *      the runner events.
 *
 * Sets the condition handler of the current PurC instance, and returns
 * the old condition handler.
 *
 * Returns: The pointer to the old condition handler; %PURC_INVPTR for error.
 *
 * Since 0.2.0
 */
PCA_EXPORT purc_cond_handler
purc_set_cond_handler(purc_cond_handler handler);

/**
 * purc_run:
 *
 * @handler: The pointer to a callback function which handles
 *      the runner events.
 *
 * Enters the event loop and runs all HVML coroutines which are ready in
 * the current PurC instance.
 *
 * Returns: 0 for success; -1 for failure.
 *
 * Since 0.0.1
 */
PCA_EXPORT int
purc_run(purc_cond_handler handler);

/**
 * purc_get_rid_by_cid:
 *
 * @cid: A coroutine identifier.
 *
 * Gets the runner identifier (rid) of a specific coroutine.
 *
 * Returns: the runner identifier or zero for failure.
 *
 * Since 0.2.0
 */
PCA_EXPORT purc_atom_t
purc_get_rid_by_cid(purc_atom_t cid);

/**
 * purc_get_instmgr_rid:
 *
 * Gets the runner identifier of the instance manager.
 *
 * Returns: the rid of the instance manager or zero for failure.
 *
 * Since 0.2.0
 */
PCA_EXPORT purc_atom_t
purc_get_instmgr_rid(void);

/**
 * purc_get_app_manifest:
 *
 * Gets the app manifest of the current instance.
 *
 * Returns: a variant of the manifest.
 * This function returns PURC_VARIANT_INVALID if there is no any PurC instance.
 *
 * Since 0.9.18
 */
PCA_EXPORT purc_variant_t
purc_get_app_manifest(void);

/**
 * purc_get_app_label:
 *
 * Gets the app label for a specific locale of the current instance.
 *
 * @locale (nullable): A pointer to the string contains the desired locale.
 *      The locale must have format like `zh_CN`. If it is NULL, or has a bad
 *      format, this function will use `en_US` as the default locale.
 *
 * Returns: A string variant which contains the app label. This function
 *  returns PURC_VARIANT_INVALID if there is no PurC instance.
 *
 * Since 0.9.18
 */
PCA_EXPORT purc_variant_t
purc_get_app_label(const char *locale);

/**
 * purc_get_app_description:
 *
 * Gets the app description for a specific locale of the current instance.
 *
 * @locale (nullable): A pointer to the string contains the desired locale.
 *      The locale must have format like `zh_CN`. If it is NULL, or has a bad
 *      format, this function will use `en_US` as the default locale.
 *
 * Returns: A string variant which contains the app description. This function
 *  returns PURC_VARIANT_INVALID if there is no PurC instance.
 *
 * Since 0.9.18
 */
PCA_EXPORT purc_variant_t
purc_get_app_description(const char *locale);

/**
 * purc_get_app_icon_url:
 *
 * Gets the URL of the app icon for the specific display density and
 * locale of the current instance.
 *
 * @display_density (nullable): A pointer to the string contains the desired
 *      display (screen) density. The display density must be one of `ldpi`,
 *      `mdpi`, `hdpi`, `xhdpi`, or `xxhdpi`. If it is NULL or not one of the
 *      available ones, this function will use `hdpi` as the default value.
 * @locale (nullable): A pointer to the string contains the desired locale.
 *      The locale must have format like `zh_CN`. If it is NULL, or has a bad
 *      format, this function will use `en_US` as the default locale.
 *
 * Returns: A string variant which contains the URL of the app icon.
 *  This function returns PURC_VARIANT_INVALID if there is no PurC instance.
 *
 * Note: you must call purc_variant_unref() after using the returned variant.
 *
 * Since 0.9.18
 */
PCA_EXPORT purc_variant_t
purc_get_app_icon_url(const char *display_density, const char *locale);

/**
 * purc_inst_create_or_get:
 *
 * @app_name (nullable): A pointer to the string contains the app name.
 *      If this argument is null, the executable program name of the command
 *      line will be used for the app name.
 * @runner_name (nullable): A pointer to the string contains the runner name.
 *      If this argument is null, `unknown` will be used for the runner name.
 * @cond_handler (nullable): A pointer to the condition handler for the new instance.
 * @extra_info (nullable): A pointer to the extra information for the new
 *      PurC instance, e.g., the type and the URI of the renderer.
 *
 * Creates a new PurC instance or gets the atom value of the existing
 * PurC instance.
 *
 * Returns: The atom representing the new PurC instance, 0 for error.
 *
 * Since 0.2.0
 */
PCA_EXPORT purc_atom_t
purc_inst_create_or_get(const char *app_name, const char *runner_name,
        purc_cond_handler cond_handler,
        const purc_instance_extra_info *extra_info);

/**
 * purc_inst_ask_to_shutdown:
 *
 * @inst: The atom representing the target PurC instance differs
 *  from the current instance.
 *
 * Asks the specified instance to shutdown. This function send a
 * `shutdownInstance` request to the target instance.
 *
 * Returns: The return code of the request; -1 on failure to send the request.
 *
 * Since 0.2.0
 */
PCA_EXPORT int
purc_inst_ask_to_shutdown(purc_atom_t inst);

/**
 * purc_inst_schedule_vdom:
 *
 * @inst: The atom representing the target PurC instance differs
 *  from the current instance.
 * @vdom: The vDOM entity returned by @purc_load_hvml_from_rwstream or
 *  its brother functions.
 * @curator: The curator of the new coroutine, i.e., the coroutine which is
 *  waiting for the the execute result of the new coroutine; 0 for no curator.
 * @page_type: the target renderer page type.
 * @target_workspace: The name of the target renderer workspace.
 * @target_group: The identifier of the target group (nullable) in the layout
 *  HTML contents. When %NULL given, the renderer will create an ungrouped
 *  plainw window for this coroutine.
 * @page_name: The page name (nullable). When %NULL is given, the page will be
 *  assigned with an auto-generated page name like `page-10`.
 * @extra_rdr_info: The extra renderer information.
 * @entry: The identifier of the `body` element as the entry in @vdom.
 *         When %NULL is given, use the first `body` element as the entry.
 * @request: The variant which will be used as the request data.
 *
 * Creates a new coroutine to run the specified vDOM in the specific instances.
 * If success, the new coroutine will be in READY state.
 *
 * Returns: The atom representing the new coroutine in the PurC instance,
 *      0 for error.
 *
 * Since 0.2.0
 */
PCA_EXPORT purc_atom_t
purc_inst_schedule_vdom(purc_atom_t inst, purc_vdom_t vdom,
        purc_atom_t curator, purc_variant_t request,
        pcrdr_page_type_k page_type, const char *target_workspace,
        const char *target_group, const char *page_name,
        purc_renderer_extra_info *extra_rdr_info,
        const char *entry);

#define PURC_EVENT_TARGET_SELF          0
#define PURC_EVENT_TARGET_BROADCAST     ((purc_atom_t)-1)

/**
 * purc_inst_post_event:
 *
 * @inst: An atom representing the target PurC instance differs
 *      from the current instance.
 * @msg: A pointer to a pcrdr_msg structure which represents the event.
 *
 * Posts an event message to a target instance.
 *
 * Returns: -1 for error; zero means everything is ok.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
purc_inst_post_event(purc_atom_t inst_to, pcrdr_msg *msg);

typedef enum {
    PURC_INST_SIGNAL_CANCEL,
    PURC_INST_SIGNAL_KILL,
} purc_inst_signal_k;

/**
 * purc_inst_emit_signal:
 *
 * @inst: An atom representing the target PurC instance differs
 *      from the current instance.
 * @signal: The signal will be emitted to the instance.
 *      - PURC_INST_SIGNAL_CANCEL
 *      - PURC_INST_SIGNAL_KILL
 *      - ...
 *
 * Emit a signal to the specified PurC instance.
 *
 * Returns: the error code:
 *  - %PURC_ERROR_OK: success
 *
 * Proposal; not implemented.
 */
PCA_EXPORT int
purc_inst_emit_signal(purc_atom_t inst, purc_inst_signal_k signal);

/**
 * purc_connect_to_renderer:
 *
 * @extra_info : A pointer to #purc_instance_extra_info structure
 *      which defines the information for the new renderer, e.g.,
 *      the URI of the renderer.
 *
 * Connect to the renderer.
 *
 * Returns: The unique id for the renderer,
 *      %NULL for error.
 *
 * Since 0.9.20
 */
PCA_EXPORT const char *
purc_connect_to_renderer(purc_instance_extra_info *extra_info);

/**
 * purc_disconnect_from_renderer:
 *
 * @id : The unique id for the renderer.
 *
 * Disconnect the renderer with the specified id
 *
 * Returns: 0 for success, other for failure.
 *
 * Since 0.9.20
 */
PCA_EXPORT int
purc_disconnect_from_renderer(const char *id);



PCA_EXTERN_C_END

#endif /* not defined PURC_PURC_H */

