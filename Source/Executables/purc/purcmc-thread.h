/*
** @file purcmc-thread.h
** @author Vincent Wei
** @date 2022/10/03
** @brief The implementation indepedent definitions for
**      thread-based PURCMC renderers.
**
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of purc, which is an HVML interpreter with
** a command line interface (CLI).
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef purc_purcmc_thread_h_
#define purc_purcmc_thread_h_

#include <purc/purc-variant.h>
#include <purc/purc-pcrdr.h>

#include "util/kvlist.h"

#define LDNAME_RENDERER   "renderer"

#ifdef NDEBUG
#   define LOG_DEBUG(x, ...)
#else
#   define LOG_DEBUG(x, ...)   \
    purc_log_debug("%s: " x, __func__, ##__VA_ARGS__)
#endif /* not defined NDEBUG */

#ifdef LOG_ERROR
#   undef LOG_ERROR
#endif

#define LOG_ERROR(x, ...)   \
    purc_log_error("%s: " x, __func__, ##__VA_ARGS__)

#define LOG_WARN(x, ...)    \
    purc_log_warn("%s: " x, __func__, ##__VA_ARGS__)

#define LOG_INFO(x, ...)    \
    purc_log_info("%s: " x, __func__, ##__VA_ARGS__)

#ifndef MIN
#   define MIN(x, y)   (((x) > (y)) ? (y) : (x))
#endif

#ifndef MAX
#   define MAX(x, y)   (((x) < (y)) ? (y) : (x))
#endif

/* round n to multiple of m */
#define ROUND_TO_MULTIPLE(n, m) (((n) + (((m) - 1))) & ~((m) - 1))

#if defined(_WIN64)
#   define SIZEOF_PTR   8
#   define SIZEOF_HPTR  4
#elif defined(__LP64__)
#   define SIZEOF_PTR   8
#   define SIZEOF_HPTR  4
#else
#   define SIZEOF_PTR   4
#   define SIZEOF_HPTR  2
#endif

/* The renderer */
struct pcmcth_renderer;
typedef struct pcmcth_renderer pcmcth_renderer;

/* The endpoint */
struct pcmcth_endpoint;
typedef struct pcmcth_endpoint pcmcth_endpoint;

/* The session for a specific endpoint */
struct pcmcth_session;
typedef struct pcmcth_session pcmcth_session;

/* The workspace for a specific app */
struct pcmcth_workspace;
typedef struct pcmcth_workspace pcmcth_workspace;

/* The page (a plain window or a widget) containing the ultimate DOM (uDOM) */
struct pcmcth_page;
typedef struct pcmcth_page pcmcth_page;

/* The ultimate DOM */
struct pcmcth_udom;
typedef struct pcmcth_udom pcmcth_udom;

typedef struct pcmcth_rdr_cbs {
    int  (*prepare)(pcmcth_renderer *);
    int  (*handle_event)(pcmcth_renderer *, unsigned long long timeout_usec);
    void (*cleanup)(pcmcth_renderer *);

    pcmcth_session *(*create_session)(pcmcth_renderer *, pcmcth_endpoint *);
    int (*remove_session)(pcmcth_session *);

    /* nullable */
    pcmcth_workspace *(*create_workspace)(pcmcth_session *,
            const char *name, const char *title, purc_variant_t properties,
            int *retv);
    /* null if create_workspace is null */
    int (*update_workspace)(pcmcth_session *, pcmcth_workspace *,
            const char *property, const char *value);
    /* null if create_workspace is null */
    int (*destroy_workspace)(pcmcth_session *, pcmcth_workspace *);

    /* Since PURMC-120; null if create_workspace is null */
    pcmcth_workspace *(*find_workspace)(pcmcth_session *sess,
            const char *name);
    /* Since PURMC-120; null if create_workspace is null */
    pcmcth_workspace *(*get_special_workspace)(pcmcth_session *sess,
            pcrdr_resname_workspace_k v);

    /* nullable */
    int (*set_page_groups)(pcmcth_session *, pcmcth_workspace *,
            const char *content, size_t length);
    /* null if set_page_groups is null */
    int (*add_page_groups)(pcmcth_session *, pcmcth_workspace *,
            const char *content, size_t length);
    /* null if set_page_groups is null */
    int (*remove_page_group)(pcmcth_session *, pcmcth_workspace *,
            const char* gid);

    /* Since PURMC-120 */
    pcmcth_page *(*find_page)(pcmcth_session *sess,
            pcmcth_workspace *workspace, const char *page_id);

    pcmcth_page *(*create_plainwin)(pcmcth_session *, pcmcth_workspace *,
            const char *page_id, const char *group, const char *name,
            const char *class_name, const char *title, const char *layout_style,
            purc_variant_t toolkit_style, int *retv);
    int (*update_plainwin)(pcmcth_session *, pcmcth_workspace *,
            pcmcth_page *win, const char *property, purc_variant_t value);
    int (*destroy_plainwin)(pcmcth_session *, pcmcth_workspace *,
            pcmcth_page *win);
    /* Since PURMC-120; nullable */
    pcmcth_page *(*get_special_plainwin)(pcmcth_session *sess,
            pcmcth_workspace *workspace, const char *group,
            pcrdr_resname_page_k v);

    /* nullable */
    pcmcth_page *(*create_widget)(pcmcth_session *, pcmcth_workspace *,
            const char *page_id, const char *group, const char *name,
            const char *class_name, const char *title, const char *layout_style,
            purc_variant_t toolkit_style, int *retv);
    /* null if create_widget is null */
    int (*update_widget)(pcmcth_session *, pcmcth_workspace *,
            pcmcth_page *page, const char *property, purc_variant_t value);
    /* null if create_widget is null */
    int (*destroy_widget)(pcmcth_session *, pcmcth_workspace *,
            pcmcth_page *page);
    /* Since PURMC-120; null if create_widget is null */
    pcmcth_page *(*get_special_widget)(pcmcth_session *sess,
            pcmcth_workspace *workspace, const char *group,
            pcrdr_resname_page_k v);

    /* no writeXXX methods */
    pcmcth_udom *(*load_edom)(pcmcth_session *, pcmcth_page *,
            purc_variant_t edom, uint64_t crtn, char *suppressed, int *retv);
    /* Since PURMC-120 */
    uint64_t (*register_crtn)(pcmcth_session *sess, pcmcth_page *,
            uint64_t crtn, int *retv);
    /* Since PURMC-120 */
    uint64_t (*revoke_crtn)(pcmcth_session *sess, pcmcth_page *,
            uint64_t crtn, int *retv);

    int (*update_udom)(pcmcth_session *, pcmcth_udom *, int op,
            uint64_t element_handle, const char* property,
            purc_variant_t ref_info);

    /* nullable */
    purc_variant_t (*call_method_in_session)(pcmcth_session *,
            pcrdr_msg_target target, uint64_t target_value,
            pcrdr_msg_element_type element_type, const char *element_value,
            const char *property, const char *method, purc_variant_t arg,
            int* retv);
    /* nullable */
    purc_variant_t (*call_method_in_udom)(pcmcth_session *,
            pcmcth_udom *, uint64_t element_handle,
            const char *method, purc_variant_t arg, int* retv);

    /* nullable */
    purc_variant_t (*get_property_in_session)(pcmcth_session *,
            pcrdr_msg_target target, uint64_t target_value,
            pcrdr_msg_element_type element_type, const char *element_value,
            const char *property, int *retv);
    /* nullable */
    purc_variant_t (*get_property_in_udom)(pcmcth_session *,
            pcmcth_udom *, uint64_t element_handle,
            const char *property, int *retv);

    /* nullable */
    purc_variant_t (*set_property_in_session)(pcmcth_session *,
            pcrdr_msg_target target, uint64_t target_value,
            pcrdr_msg_element_type element_type, const char *element_value,
            const char *property, purc_variant_t value, int *retv);
    /* nullable */
    purc_variant_t (*set_property_in_udom)(pcmcth_session *,
            pcmcth_udom *, uint64_t element_handle,
            const char *property, purc_variant_t value, int *retv);
} pcmcth_rdr_cbs;

struct pcmcth_rdr_data;

struct pcmcth_renderer {
    const char     *features;
    const char     *name;
    unsigned int    len_features;
    unsigned int    nr_endpoints;
    unsigned int    nr_timers;
    purc_atom_t     master_rid;

    time_t t_start;
    time_t t_elapsed;
    time_t t_elapsed_last;

    /* The KV list using app name as the key,
       and pcmcth_workspace* as the value */
    struct kvlist workspace_list;

    /* The KV list using endpoint URI as the key,
       and pcmcth_endpoint* as the value */
    struct kvlist endpoint_list;

    /* the AVL tree of endpoints sorted by living time */
    struct avl_tree living_avl;

    /* The KV list using timer identifier as the key,
       and pcmcth_timer_t as the value */
    struct kvlist timer_list;

    /* the AVL tree of timers sorted by expired time */
    struct avl_tree timer_avl;

    /* the data for the renderer implementation */
    struct pcmcth_rdr_data *impl;

    pcmcth_rdr_cbs cbs;
};

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* !purc_purcmc_thread_h_ */

