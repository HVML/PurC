/*
** @file callbacks.h
** @author Vincent Wei
** @date 2022/10/03
** @brief The implementation indepedent definitions of PURCMC thread callbacks.
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

#ifndef purc_foil_callbacks_h_
#define purc_foil_callbacks_h_

#include <purc/purc-variant.h>
#include <purc/purc-pcrdr.h>

/* The renderer */
struct purcth_renderer;
typedef struct purcth_renderer purcth_renderer;

/* The endpoint */
struct purcth_endpoint;
typedef struct purcth_endpoint purcth_endpoint;

/* The session for a specific endpoint */
struct purcth_session;
typedef struct purcth_session purcth_session;

/* The workspace for a specific app */
struct purcth_workspace;
typedef struct purcth_workspace purcth_workspace;

/* The page (a plain window or a widget) containing the ultimate DOM (uDOM) */
struct purcth_page;
typedef struct purcth_page purcth_page;

/* The ultimate DOM */
struct purcth_udom;
typedef struct purcth_udom purcth_udom;

typedef struct purcth_rdr_cbs {
    int  (*prepare)(purcth_renderer *);
    int  (*handle_event)(purcth_renderer *, unsigned long long timeout_usec);
    void (*cleanup)(purcth_renderer *);

    purcth_session *(*create_session)(purcth_renderer *, purcth_endpoint *);
    int (*remove_session)(purcth_session *);

    /* nullable */
    purcth_workspace *(*create_workspace)(purcth_session *,
            const char *name, const char *title, purc_variant_t properties,
            int *retv);
    /* null if create_workspace is null */
    int (*update_workspace)(purcth_session *, purcth_workspace *,
            const char *property, const char *value);
    /* null if create_workspace is null */
    int (*destroy_workspace)(purcth_session *, purcth_workspace *);

    /* nullable */
    int (*set_page_groups)(purcth_session *, purcth_workspace *,
            const char *content, size_t length);
    /* null if set_page_groups is null */
    int (*add_page_groups)(purcth_session *, purcth_workspace *,
            const char *content, size_t length);
    /* null if set_page_groups is null */
    int (*remove_page_group)(purcth_session *, purcth_workspace *,
            const char* gid);

    purcth_page *(*create_plainwin)(purcth_session *, purcth_workspace *,
            const char *gid, const char *name,
            const char *class_name, const char *title, const char *layout_style,
            purc_variant_t toolkit_style, int *retv);
    int (*update_plainwin)(purcth_session *, purcth_workspace *,
            purcth_page *win, const char *property, purc_variant_t value);
    int (*destroy_plainwin)(purcth_session *, purcth_workspace *,
            purcth_page *win);

    /* nullable */
    purcth_page *(*create_widget)(purcth_session *, purcth_workspace *,
            const char *gid, const char *name,
            const char *class_name, const char *title, const char *layout_style,
            purc_variant_t toolkit_style, int *retv);
    /* null if create_widget is null */
    int (*update_widget)(purcth_session *, purcth_workspace *,
            purcth_page *page, const char *property, purc_variant_t value);
    /* null if create_widget is null */
    int (*destroy_widget)(purcth_session *, purcth_workspace *,
            purcth_page *page);

    /* no write method */
    purcth_udom *(*load_edom)(purcth_session *, purcth_page *,
            purc_variant_t edom, int *retv);

    int (*update_udom)(purcth_session *, purcth_udom *, int op,
            uint64_t element_handle, const char* property,
            purc_variant_t ref_info);

    /* nullable */
    purc_variant_t (*call_method_in_session)(purcth_session *,
            pcrdr_msg_target target, uint64_t target_value,
            const char *element_type, const char *element_value,
            const char *property, const char *method, purc_variant_t arg,
            int* retv);
    /* nullable */
    purc_variant_t (*call_method_in_udom)(purcth_session *,
            purcth_udom *, uint64_t element_handle,
            const char *method, purc_variant_t arg, int* retv);

    /* nullable */
    purc_variant_t (*get_property_in_session)(purcth_session *,
            pcrdr_msg_target target, uint64_t target_value,
            const char *element_type, const char *element_value,
            const char *property, int *retv);
    /* nullable */
    purc_variant_t (*get_property_in_udom)(purcth_session *,
            purcth_udom *, uint64_t element_handle,
            const char *property, int *retv);

    /* nullable */
    purc_variant_t (*set_property_in_session)(purcth_session *,
            pcrdr_msg_target target, uint64_t target_value,
            const char *element_type, const char *element_value,
            const char *property, purc_variant_t value, int *retv);
    /* nullable */
    purc_variant_t (*set_property_in_udom)(purcth_session *,
            purcth_udom *, uint64_t element_handle,
            const char *property, purc_variant_t value, int *retv);
} purcth_rdr_cbs;

#ifdef __cplusplus
extern "C" {
#endif

void set_renderer_callbacks(purcth_renderer *rdr);

#ifdef __cplusplus
}
#endif

#endif /* !purc_foil_callbacks_h_ */

