/*
** @file foil.h
** @author Vincent Wei
** @date 2022/09/30
** @brief The global definitions for the renderer Foil.
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

#ifndef purc_foil_h
#define purc_foil_h

#include <time.h>

/* for purc_atom_t */
#include <purc/purc.h>

#include "util/kvlist.h"

#define RDR_FOIL_APP_NAME       "cn.fmsoft.hvml.renderer"
#define RDR_FOIL_RUN_NAME       "foil"

#define RENDERER_FEATURES \
    PCRDR_PURCMC_PROTOCOL_NAME ":" PCRDR_PURCMC_PROTOCOL_VERSION_STRING "\n" \
    "HTML:5.3\n" \
    "workspace:0/tabbedWindow:0/tabbedPage:0/plainWindow:-1/windowLevel:2\n" \
    "windowLevels:normal,topmost"

/* The THREAD renderer */
struct Renderer;
typedef struct Renderer Renderer;

struct Workspace;
typedef struct Workspace Workspace;

struct PlainWin;
typedef struct PlainWin PlainWin;

struct Page;
typedef struct Page Page;

struct Dom;
typedef struct Dom Dom;

/* The THREAD endpoint */
struct Endpoint;
typedef struct Endpoint Endpoint;

struct Session;
typedef struct Session Session;

typedef struct RendererCallbacks {
    int  (*prepare)(Renderer *);
    void (*cleanup)(Renderer *);

    Session *(*create_session)(Renderer *, Endpoint *);
    int (*remove_session)(Session *);

    /* nullable */
    Workspace *(*create_workspace)(Session *,
            const char *name, const char *title, purc_variant_t properties,
            int *retv);
    /* null if create_workspace is null */
    int (*update_workspace)(Session *, Workspace *,
            const char *property, const char *value);
    /* null if create_workspace is null */
    int (*destroy_workspace)(Session *, Workspace *);

    /* nullable */
    int (*set_page_groups)(Session *, Workspace *,
            const char *content, size_t length);
    /* null if set_page_groups is null */
    int (*add_page_groups)(Session *, Workspace *,
            const char *content, size_t length);
    /* null if set_page_groups is null */
    int (*remove_page_group)(Session *, Workspace *,
            const char* gid);

    PlainWin *(*create_plainwin)(Session *, Workspace *,
            const char *request_id, const char *gid, const char *name,
            const char *class_name, const char *title, const char *layout_style,
            purc_variant_t toolkit_style, int *retv);
    int (*update_plainwin)(Session *, Workspace *,
            PlainWin *win, const char *property, purc_variant_t value);
    int (*destroy_plainwin)(Session *, Workspace *,
            PlainWin *win);

    Page *(*get_plainwin_page)(Session *,
            PlainWin *plainWin, int *retv);

    /* nullable */
    Page *(*create_page)(Session *, Workspace *,
            const char *request_id, const char *gid, const char *name,
            const char *class_name, const char *title, const char *layout_style,
            purc_variant_t toolkit_style, int *retv);
    /* null if create_page is null */
    int (*update_page)(Session *, Workspace *,
            Page *page, const char *property, purc_variant_t value);
    /* null if create_page is null */
    int (*destroy_page)(Session *, Workspace *,
            Page *page);

    Dom *(*load)(Session *, Page *,
            int op, const char *op_name, const char *request_id,
            const char *content, size_t length, int *retv);
    Dom *(*write)(Session *, Page *,
            int op, const char *op_name, const char *request_id,
            const char *content, size_t length, int *retv);

    int (*update_dom)(Session *, Dom *,
            int op, const char *op_name, const char *request_id,
            const char* element_type, const char* element_value,
            const char* property, pcrdr_msg_data_type text_type,
            const char *content, size_t length);

    /* nullable */
    purc_variant_t (*call_method_in_session)(Session *,
            pcrdr_msg_target target, uint64_t target_value,
            const char *element_type, const char *element_value,
            const char *property, const char *method, purc_variant_t arg,
            int* retv);
    /* nullable */
    purc_variant_t (*call_method_in_dom)(Session *, const char *,
            Dom *, const char* element_type, const char* element_value,
            const char *method, purc_variant_t arg, int* retv);

    /* nullable */
    purc_variant_t (*get_property_in_session)(Session *,
            pcrdr_msg_target target, uint64_t target_value,
            const char *element_type, const char *element_value,
            const char *property, int *retv);
    /* nullable */
    purc_variant_t (*get_property_in_dom)(Session *, const char *,
            Dom *, const char* element_type, const char* element_value,
            const char *property, int *retv);

    /* nullable */
    purc_variant_t (*set_property_in_session)(Session *,
            pcrdr_msg_target target, uint64_t target_value,
            const char *element_type, const char *element_value,
            const char *property, purc_variant_t value, int *retv);
    /* nullable */
    purc_variant_t (*set_property_in_dom)(Session *, const char *,
            Dom *, const char* element_type, const char* element_value,
            const char *property, purc_variant_t value, int *retv);

    bool (*pend_response)(Session *, const char *operation,
            const char *request_id, void *result_value);
} RendererCallbacks;

struct Renderer {
    unsigned int nr_endpoints;

    time_t t_start;
    time_t t_elapsed;
    time_t t_elapsed_last;

    /* The KV list using endpoint URI as the key, and Endpoint* as the value */
    struct kvlist endpoint_list;

    /* the AVL tree of endpoints sorted by living time */
    struct avl_tree living_avl;

    RendererCallbacks cbs;
};

purc_atom_t foil_init(const char *rdr_uri);

#endif  /* purc_foil_h */

