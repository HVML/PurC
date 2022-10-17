/*
** @file callbacks.c
** @author Vincent Wei
** @date 2022/10/03
** @brief The implementation of PURCTH callbacks.
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

#include <assert.h>

#include "callbacks.h"
#include "endpoint.h"
#include "workspace.h"
#include "udom.h"
#include "util/sorted-array.h"

/* handle types */
enum {
    HT_WORKSPACE = 0,
    HT_PLAINWIN,
    HT_TABBEDWIN,
    HT_CONTAINER,
    HT_WIDGET,
    HT_UDOM,
};

struct purcth_session {
    purcth_renderer *rdr;
    purcth_endpoint *edpt;

    /* ungrouped plain windows */
    struct kvlist ug_wins;

    /* the sorted array of all valid handles */
    struct sorted_array *all_handles;

    /* the only workspace for all sessions of current app */
    purcth_workspace *workspace;
};

static int foil_prepare(purcth_renderer *rdr)
{
    return foil_wsp_module_init(rdr);
}

static void foil_cleanup(purcth_renderer *rdr)
{
    foil_wsp_module_cleanup(rdr);
}

static purcth_session *
foil_create_session(purcth_renderer *rdr, purcth_endpoint *edpt)
{
    purcth_session* sess = calloc(1, sizeof(purcth_session));

    sess->workspace = foil_wsp_create_or_get_workspace(rdr, edpt);
    if (sess->workspace == NULL) {
        goto failed;
    }

    sess->all_handles = sorted_array_create(SAFLAG_DEFAULT, 8, NULL, NULL);
    if (sess->all_handles == NULL) {
        goto failed;
    }

    sess->rdr = rdr;
    sess->edpt = edpt;

    kvlist_init(&sess->ug_wins, NULL);
    return sess;

failed:

    if (sess->all_handles)
        sorted_array_destroy(sess->all_handles);

    free(sess);
    return NULL;
}

static int foil_remove_session(purcth_session *sess)
{
    const char *name;
    void *next, *data;

    LOG_DEBUG("removing session (%p)...\n", sess);

    LOG_DEBUG("destroy all ungrouped plain windows...\n");
    kvlist_for_each_safe(&sess->ug_wins, name, next, data) {
        /* TODO
        purcth_page *plain_win = *(purcth_page **)data;
        */
    }

    LOG_DEBUG("destroy kvlist for ungrouped plain windows...\n");
    kvlist_free(&sess->ug_wins);

    LOG_DEBUG("destroy sorted array for all handles...\n");
    sorted_array_destroy(sess->all_handles);

    LOG_DEBUG("free session...\n");
    free(sess);

    LOG_DEBUG("done\n");
    return PCRDR_SC_OK;
}

static purcth_page *foil_create_plainwin(purcth_session *sess,
        purcth_workspace *workspace,
        const char *gid, const char *name,
        const char *class_name, const char *title, const char *layout_style,
        purc_variant_t toolkit_style, int *retv)
{
    (void)class_name;
    (void)title;
    (void)layout_style;
    (void)toolkit_style;

    purcth_page *plain_win = NULL;

    workspace = sess->workspace;

    if (gid == NULL) {
        /* create a ungrouped plain window */
        LOG_DEBUG("creating an ungrouped plain window with name (%s)\n", name);

        if (kvlist_get(&sess->ug_wins, name)) {
            LOG_WARN("Duplicated ungrouped plain window: %s\n", name);
            *retv = PCRDR_SC_CONFLICT;
            goto done;
        }

        struct foil_widget_info style = { };
        style.flags = WSP_WIDGET_FLAG_NAME | WSP_WIDGET_FLAG_TITLE;
        style.name = name;
        style.title = title;
        foil_wsp_convert_style(workspace, sess, &style, toolkit_style);
        plain_win = foil_wsp_create_widget(workspace, sess,
                WSP_WIDGET_TYPE_PLAINWINDOW, NULL, NULL, NULL, &style);
        kvlist_set(&sess->ug_wins, name, &plain_win);

    }
    else if (workspace->layouter == NULL) {
        *retv = PCRDR_SC_PRECONDITION_FAILED;
        goto done;
    }
    else {
        LOG_DEBUG("creating a grouped plain window with name (%s/%s)\n",
                gid, name);

        /* TODO: create a plain window in the specified group
        plain_win = wsp_layouter_add_plain_window(workspace->layouter, sess,
                gid, name, class_name, title, layout_style, toolkit_style,
                web_view, retv);
        */
    }

    if (plain_win) {
        sorted_array_add(sess->all_handles, PTR2U64(plain_win),
                INT2PTR(HT_PLAINWIN));
        /* TODO sorted_array_add(sess->all_handles, PTR2U64(web_view),
                INT2PTR(HT_PAGE)); */
        *retv = PCRDR_SC_OK;
    }
    else {
        LOG_ERROR("Failed to create a plain window: %s/%s\n", gid, name);
        *retv = PCRDR_SC_INSUFFICIENT_STORAGE;
    }

done:
    return plain_win;
}

static int
foil_update_plainwin(purcth_session *sess, purcth_workspace *workspace,
        purcth_page *plain_win, const char *property, purc_variant_t value)
{
    void *data;
    if (!sorted_array_find(sess->all_handles, PTR2U64(plain_win), &data)) {

        if (workspace->layouter) {
            /* TODO
            if (wsp_layouter_retrieve_widget(workspace->layouter, plain_win) ==
                    WSP_WIDGET_TYPE_PLAINWINDOW) {
                return wsp_layouter_update_widget(workspace->layouter, sess,
                        plain_win, property, value);
            }
            */
        }

        return PCRDR_SC_NOT_FOUND;
    }

    if ((uintptr_t)data != HT_PLAINWIN) {
        return PCRDR_SC_BAD_REQUEST;
    }

    if (strcmp(property, "name") == 0) {
        /* Forbid to change name of a plain window */
        return PCRDR_SC_FORBIDDEN;
    }
    else if (strcmp(property, "class") == 0) {
        /* Not acceptable to change class of a plain window */
        return PCRDR_SC_NOT_ACCEPTABLE;
    }
    else if (strcmp(property, "title") == 0) {
        struct foil_widget_info info = { };

        info.title = purc_variant_get_string_const(value);
        if (info.title) {
            info.flags = WSP_WIDGET_FLAG_TITLE;
            foil_wsp_update_widget(workspace, sess,
                    plain_win, WSP_WIDGET_TYPE_PLAINWINDOW, &info);
        }
        else {
            return PCRDR_SC_BAD_REQUEST;
        }
    }
    else if (strcmp(property, "layoutStyle") == 0) {
        /* TODO */
    }
    else if (strcmp(property, "toolkitStyle") == 0) {
        /* TODO */
    }

    return PCRDR_SC_OK;
}

static int
foil_destroy_plainwin(purcth_session *sess, purcth_workspace *workspace,
        purcth_page *plain_win)
{
    workspace = sess->workspace;
    return foil_wsp_destroy_widget(workspace, sess, plain_win, plain_win,
        WSP_WIDGET_TYPE_PLAINWINDOW);
}

#if 0
static purcth_page *
foil_get_plainwin_page(purcth_session *sess,
        purcth_page *plain_win, int *retv)
{
    void *data;
    if (!sorted_array_find(sess->all_handles, PTR2U64(plain_win), &data)) {
        *retv = PCRDR_SC_NOT_FOUND;
        return NULL;
    }

    if ((uintptr_t)data != HT_PLAINWIN) {
        *retv = PCRDR_SC_BAD_REQUEST;
        return NULL;
    }

    *retv = PCRDR_SC_OK;
    return NULL;
    /* TODO
    return (purcth_page *)browser_plain_window_get_view(
            BROWSER_PLAIN_WINDOW(plain_win)); */
}
#endif

static purcth_page *
validate_page(purcth_session *sess, purcth_page *page, int *retv)
{
    void *data;
    if (!sorted_array_find(sess->all_handles, PTR2U64(page), &data)) {
        *retv = PCRDR_SC_NOT_FOUND;
        return NULL;
    }

    if ((uintptr_t)data == HT_PLAINWIN ||
            (uintptr_t)data == HT_WIDGET) {
        return page;
    }

    *retv = PCRDR_SC_BAD_REQUEST;
    return NULL;
}

static purcth_udom *
foil_load_edom(purcth_session *sess, purcth_page *page, purc_variant_t edom,
        int *retv)
{
    page = validate_page(sess, page, retv);
    if (page == NULL)
        return NULL;

    purcth_udom *udom = foil_wsp_load_edom_in_page(sess->workspace, sess,
            page, edom, retv);

    if (udom) {
        sorted_array_add(sess->all_handles, PTR2U64(udom),
                INT2PTR(HT_UDOM));
        *retv = PCRDR_SC_OK;
    }
    else
        *retv = PCRDR_SC_INTERNAL_SERVER_ERROR;

    return udom;
}

static purcth_udom *
validate_udom(purcth_session *sess, purcth_udom *udom, int *retv)
{
    void *data;
    if (!sorted_array_find(sess->all_handles, PTR2U64(udom), &data)) {
        *retv = PCRDR_SC_NOT_FOUND;
        return NULL;
    }

    if ((uintptr_t)data == HT_UDOM) {
        return udom;
    }

    *retv = PCRDR_SC_BAD_REQUEST;
    return NULL;
}

static int foil_update_udom(purcth_session *sess, purcth_udom *udom,
        int op, uint64_t element_handle, const char* property,
        purc_variant_t ref_info)
{
    int retv;

    udom = validate_udom(sess, udom, &retv);
    if (udom == NULL) {
        LOG_ERROR("Bad uDOM: %p.\n", udom);
        goto failed;
    }

    if (property != NULL &&
            !purc_is_valid_token(property, PURC_LEN_PROPERTY_NAME)) {
        retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    foil_rdrbox *rdrbox = foil_udom_find_rdrbox(udom, element_handle);
    if (rdrbox == NULL) {
        retv = PCRDR_SC_NOT_FOUND;
        goto failed;
    }

    retv = foil_udom_update_rdrbox(udom, rdrbox, op, property, ref_info);

failed:
    return retv;
}

static purc_variant_t
foil_call_method_in_udom(purcth_session *sess,
        purcth_udom *udom, uint64_t element_handle,
        const char *method, purc_variant_t arg, int* retv)
{
    udom = validate_udom(sess, udom, retv);
    if (udom == NULL) {
        LOG_ERROR("Bad uDOM: %p.\n", udom);
        return PURC_VARIANT_INVALID;
    }

    if (!purc_is_valid_token(method, PURC_LEN_PROPERTY_NAME)) {
        *retv = PCRDR_SC_BAD_REQUEST;
        return PURC_VARIANT_INVALID;
    }

    foil_rdrbox *rdrbox = foil_udom_find_rdrbox(udom, element_handle);
    if (rdrbox == NULL) {
        *retv = PCRDR_SC_NOT_FOUND;
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t result = foil_udom_call_method(udom, rdrbox, method, arg);
    if (result) {
        *retv = PCRDR_SC_OK;
    }
    else {
        *retv = PCRDR_SC_INTERNAL_SERVER_ERROR;
    }

    return result;
}

static purc_variant_t
foil_get_property_in_udom(purcth_session *sess,
        purcth_udom *udom, uint64_t element_handle,
        const char *property, int *retv)
{
    udom = validate_udom(sess, udom, retv);
    if (udom == NULL) {
        LOG_ERROR("Bad uDOM: %p.\n", udom);
        return PURC_VARIANT_INVALID;
    }

    foil_rdrbox *rdrbox = foil_udom_find_rdrbox(udom, element_handle);
    if (rdrbox == NULL) {
        *retv = PCRDR_SC_NOT_FOUND;
        return PURC_VARIANT_INVALID;
    }

    if (!purc_is_valid_token(property, PURC_LEN_PROPERTY_NAME)) {
        *retv = PCRDR_SC_BAD_REQUEST;
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t result;
    result = foil_udom_get_property(udom, rdrbox, property);
    if (result) {
        *retv = PCRDR_SC_OK;
    }
    else {
        *retv = PCRDR_SC_INTERNAL_SERVER_ERROR;
    }

    return result;
}

static purc_variant_t
foil_set_property_in_udom(purcth_session *sess,
        purcth_udom *udom, uint64_t element_handle,
        const char *property, purc_variant_t value, int *retv)
{
    udom = validate_udom(sess, udom, retv);
    if (udom == NULL) {
        LOG_ERROR("Bad uDOM: %p.\n", udom);
        return PURC_VARIANT_INVALID;
    }

    foil_rdrbox *rdrbox = foil_udom_find_rdrbox(udom, element_handle);
    if (rdrbox == NULL) {
        *retv = PCRDR_SC_NOT_FOUND;
        return PURC_VARIANT_INVALID;
    }

    if (!purc_is_valid_token(property, PURC_LEN_PROPERTY_NAME)) {
        *retv = PCRDR_SC_BAD_REQUEST;
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t result;
    result = foil_udom_set_property(udom, rdrbox, property, value);
    if (result) {
        *retv = PCRDR_SC_OK;
    }
    else {
        *retv = PCRDR_SC_INTERNAL_SERVER_ERROR;
    }

    return result;
}

void set_renderer_callbacks(purcth_renderer *rdr)
{
    memset(&rdr->cbs, 0, sizeof(rdr->cbs));

    rdr->cbs.prepare = foil_prepare;
    rdr->cbs.cleanup = foil_cleanup;
    rdr->cbs.create_session = foil_create_session;
    rdr->cbs.remove_session = foil_remove_session;
    rdr->cbs.create_plainwin = foil_create_plainwin;
    rdr->cbs.update_plainwin = foil_update_plainwin;
    rdr->cbs.destroy_plainwin = foil_destroy_plainwin;

    rdr->cbs.load_edom = foil_load_edom;
    rdr->cbs.update_udom = foil_update_udom;
    rdr->cbs.call_method_in_udom = foil_call_method_in_udom;
    rdr->cbs.get_property_in_udom = foil_get_property_in_udom;
    rdr->cbs.set_property_in_udom = foil_set_property_in_udom;

    return;
}

