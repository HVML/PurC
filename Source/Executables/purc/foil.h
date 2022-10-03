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

#define FOIL_APP_NAME           "cn.fmsoft.hvml.renderer"
#define FOIL_RUN_NAME           "foil"

#define FOIL_RDR_NAME           "Foil"

#define FOIL_RDR_FEATURES \
    PCRDR_PURCMC_PROTOCOL_NAME ":" PCRDR_PURCMC_PROTOCOL_VERSION_STRING "\n" \
    FOIL_RDR_NAME ":" PURC_VERSION_STRING "\n" \
    "HTML:5.3\n" \
    "workspace:0/tabbedWindow:-1/plainWindow:-1/widgetInTabbedWindow:8\n" \
    "DOMElementSelectors:handle,handles"

/* The PURCTH renderer */
struct purcth_renderer;
typedef struct purcth_renderer purcth_renderer;

/* The PURCTH endpoint */
struct purcth_endpoint;
typedef struct purcth_endpoint purcth_endpoint;

struct purcth_session;
typedef struct purcth_session purcth_session;

struct purcth_workspace;
typedef struct purcth_workspace purcth_workspace;

struct purcth_plainwin;
typedef struct purcth_plainwin purcth_plainwin;

struct purcth_page;
typedef struct purcth_page purcth_page;

struct purcth_dom;
typedef struct purcth_dom purcth_dom;

typedef struct purcth_rdr_cbs {
    int  (*prepare)(purcth_renderer *);
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

    purcth_plainwin *(*create_plainwin)(purcth_session *, purcth_workspace *,
            const char *request_id, const char *gid, const char *name,
            const char *class_name, const char *title, const char *layout_style,
            purc_variant_t toolkit_style, int *retv);
    int (*update_plainwin)(purcth_session *, purcth_workspace *,
            purcth_plainwin *win, const char *property, purc_variant_t value);
    int (*destroy_plainwin)(purcth_session *, purcth_workspace *,
            purcth_plainwin *win);

    purcth_page *(*get_plainwin_page)(purcth_session *,
            purcth_plainwin *plainWin, int *retv);

    /* nullable */
    purcth_page *(*create_page)(purcth_session *, purcth_workspace *,
            const char *request_id, const char *gid, const char *name,
            const char *class_name, const char *title, const char *layout_style,
            purc_variant_t toolkit_style, int *retv);
    /* null if create_page is null */
    int (*update_page)(purcth_session *, purcth_workspace *,
            purcth_page *page, const char *property, purc_variant_t value);
    /* null if create_page is null */
    int (*destroy_page)(purcth_session *, purcth_workspace *,
            purcth_page *page);

    purcth_dom *(*load)(purcth_session *, purcth_page *,
            int op, const char *op_name, const char *request_id,
            const char *content, size_t length, int *retv);
    purcth_dom *(*write)(purcth_session *, purcth_page *,
            int op, const char *op_name, const char *request_id,
            const char *content, size_t length, int *retv);

    int (*update_dom)(purcth_session *, purcth_dom *,
            int op, const char *op_name, const char *request_id,
            const char* element_type, const char* element_value,
            const char* property, pcrdr_msg_data_type text_type,
            const char *content, size_t length);

    /* nullable */
    purc_variant_t (*call_method_in_session)(purcth_session *,
            pcrdr_msg_target target, uint64_t target_value,
            const char *element_type, const char *element_value,
            const char *property, const char *method, purc_variant_t arg,
            int* retv);
    /* nullable */
    purc_variant_t (*call_method_in_dom)(purcth_session *, const char *,
            purcth_dom *, const char* element_type, const char* element_value,
            const char *method, purc_variant_t arg, int* retv);

    /* nullable */
    purc_variant_t (*get_property_in_session)(purcth_session *,
            pcrdr_msg_target target, uint64_t target_value,
            const char *element_type, const char *element_value,
            const char *property, int *retv);
    /* nullable */
    purc_variant_t (*get_property_in_dom)(purcth_session *, const char *,
            purcth_dom *, const char* element_type, const char* element_value,
            const char *property, int *retv);

    /* nullable */
    purc_variant_t (*set_property_in_session)(purcth_session *,
            pcrdr_msg_target target, uint64_t target_value,
            const char *element_type, const char *element_value,
            const char *property, purc_variant_t value, int *retv);
    /* nullable */
    purc_variant_t (*set_property_in_dom)(purcth_session *, const char *,
            purcth_dom *, const char* element_type, const char* element_value,
            const char *property, purc_variant_t value, int *retv);

    bool (*pend_response)(purcth_session *, const char *operation,
            const char *request_id, void *result_value);
} purcth_rdr_cbs;

struct purcth_renderer {
    purc_atom_t     master_rid;
    unsigned int    nr_endpoints;

    time_t t_start;
    time_t t_elapsed;
    time_t t_elapsed_last;

    char  *features;

    /* The KV list using endpoint URI as the key, and purcth_endpoint* as the value */
    struct kvlist endpoint_list;

    /* the AVL tree of endpoints sorted by living time */
    struct avl_tree living_avl;

    purcth_rdr_cbs cbs;
};

purc_atom_t foil_init(const char *rdr_uri);

#endif  /* purc_foil_h */

