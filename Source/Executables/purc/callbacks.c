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
#include "util/sorted-array.h"

/* handle types */
enum {
    HT_WORKSPACE = 0,
    HT_PLAINWIN,
    HT_TABBEDWIN,
    HT_CONTAINER,
    HT_WIDGET,
    HT_PAGE,
    HT_DOM,
};

struct purcth_workspace {
    /* TODO: manager of grouped plain windows and pages */
    void *layouter;
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

static KVLIST(kv_app_workspace, NULL);

static int foil_prepare(purcth_renderer *rdr)
{
    (void)rdr;
    return 0;
}

static void foil_cleanup(purcth_renderer *rdr)
{
    (void)rdr;
    const char *name;
    void *next, *data;

    kvlist_for_each_safe(&kv_app_workspace, name, next, data) {
        purcth_workspace *workspace = *(purcth_workspace **)data;
        if (workspace->layouter) {
            // TODO: ws_layouter_delete(workspace->layouter);
        }
    }
}

static purcth_workspace *create_or_get_workspace(purcth_endpoint* endpoint)
{
    char host[PURC_LEN_HOST_NAME + 1];
    char app[PURC_LEN_APP_NAME + 1];
    const char *edpt_uri = get_endpoint_uri(endpoint);

    purc_extract_host_name(edpt_uri, host);
    purc_extract_app_name(edpt_uri, app);

    char app_key[PURC_LEN_ENDPOINT_NAME + 1];
    sprintf(app_key, "%s-%s", host, app);

    void *data;
    purcth_workspace *workspace;
    if ((data = kvlist_get(&kv_app_workspace, app_key))) {
        workspace = *(purcth_workspace **)data;
        assert(workspace);
    }
    else {
        workspace = calloc(1, sizeof(purcth_workspace));
        if (workspace) {
            workspace->layouter = NULL;
            kvlist_set(&kv_app_workspace, app_key, &workspace);
        }
    }

    return workspace;
}

static purcth_session *
foil_create_session(purcth_renderer *rdr, purcth_endpoint *edpt)
{
    purcth_session* sess = calloc(1, sizeof(purcth_session));

    sess->workspace = create_or_get_workspace(edpt);
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
        purcth_plainwin *plain_win = *(purcth_plainwin **)data;
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

static purcth_plainwin *foil_create_plainwin(purcth_session *sess,
        purcth_workspace *workspace,
        const char *gid, const char *name,
        const char *class_name, const char *title, const char *layout_style,
        purc_variant_t toolkit_style, int *retv)
{
    (void)class_name;
    (void)title;
    (void)layout_style;
    (void)toolkit_style;

    purcth_plainwin *plain_win = NULL;

    workspace = sess->workspace;

    if (gid == NULL) {
        /* create a ungrouped plain window */
        LOG_DEBUG("creating an ungrouped plain window with name (%s)\n", name);

        if (kvlist_get(&sess->ug_wins, name)) {
            LOG_WARN("Duplicated ungrouped plain window: %s\n", name);
            *retv = PCRDR_SC_CONFLICT;
            goto done;
        }

        /* TODO
        struct ws_widget_info style = { };
        style.flags = WSWS_FLAG_NAME | WSWS_FLAG_TITLE;
        style.name = name;
        style.title = title;
        foil_imp_convert_style(&style, toolkit_style);
        plain_win = foil_imp_create_widget(workspace, sess,
                WS_WIDGET_TYPE_PLAINWINDOW, NULL, NULL, web_view, &style);
        kvlist_set(&sess->ug_wins, name, &plain_win);
        */

    }
    else if (workspace->layouter == NULL) {
        *retv = PCRDR_SC_PRECONDITION_FAILED;
        goto done;
    }
    else {
        LOG_DEBUG("creating a grouped plain window with name (%s/%s)\n",
                gid, name);

        /* TODO: create a plain window in the specified group
        plain_win = ws_layouter_add_plain_window(workspace->layouter, sess,
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
        purcth_plainwin *plain_win, const char *property, purc_variant_t value)
{
    void *data;
    if (!sorted_array_find(sess->all_handles, PTR2U64(plain_win), &data)) {

        if (workspace->layouter) {
            /* TODO
            if (ws_layouter_retrieve_widget(workspace->layouter, plain_win) ==
                    WS_WIDGET_TYPE_PLAINWINDOW) {
                return ws_layouter_update_widget(workspace->layouter, sess,
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
        const char *title = purc_variant_get_string_const(value);
        if (title) {
            /* TODO
            browser_plain_window_set_title(BROWSER_PLAIN_WINDOW(plain_win),
                    title); */
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
        purcth_plainwin *plain_win)
{
    (void)sess;
    (void)workspace;
    (void)plain_win;
    return PCRDR_SC_OK;

    /* TODO
    workspace = sess->workspace;

    return foil_imp_destroy_widget(workspace, sess, plain_win, plain_win,
        WS_WIDGET_TYPE_PLAINWINDOW); */
}

static purcth_page *
foil_get_plainwin_page(purcth_session *sess,
        purcth_plainwin *plain_win, int *retv)
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

static purcth_page *
validate_page(purcth_session *sess, purcth_page *page, int *retv)
{
    void *data;
    if (!sorted_array_find(sess->all_handles, PTR2U64(page), &data)) {
        *retv = PCRDR_SC_NOT_FOUND;
        return NULL;
    }

    if ((uintptr_t)data == HT_PLAINWIN) {
        /* TODO:
        BrowserPane *pane = BROWSER_PANE(page);
        return browser_pane_get_web_view(pane);
        */
    }
    else if ((uintptr_t)data == HT_PAGE) {
        return (purcth_page *)page;
    }
    else {
        *retv = PCRDR_SC_BAD_REQUEST;
        return NULL;
    }

    return (purcth_page *)page;
}

static bool validate_dom(purcth_session *sess, purcth_dom *dom, int *retv)
{
    void *data;
    if (!sorted_array_find(sess->all_handles, PTR2U64(dom), &data)) {
        *retv = PCRDR_SC_NOT_FOUND;
        return false;
    }

    if ((uintptr_t)data == HT_DOM) {
        return true;
    }

    *retv = PCRDR_SC_BAD_REQUEST;
    return false;
}

static purcth_dom *
foil_load(purcth_session *sess, purcth_page *page, uint64_t edom, int *retv)
{
    page = validate_page(sess, page, retv);
    if (page == NULL)
        return NULL;

    /* TODO */
    (void)edom;

    *retv = PCRDR_SC_OK;
    return NULL;
}

static int foil_update_dom(purcth_session *sess, purcth_dom *dom, int op,
        uint64_t element_handle, uint64_t ref_element, const char* property)
{
    (void)sess;
    (void)dom;
    (void)op;
    (void)element_handle;
    (void)ref_element;
    (void)property;

    int retv = PCRDR_SC_OK;

    return retv;
}

static purc_variant_t
foil_call_method_in_dom(purcth_session *sess,
        purcth_dom *dom, uint64_t element_handle,
        const char *method, purc_variant_t arg, int* retv)
{
    (void)element_handle;
    (void)method;
    (void)arg;
    if (!validate_dom(sess, dom, retv)) {
        LOG_ERROR("Bad DOM pointer: %p.\n", dom);
        return PURC_VARIANT_INVALID;
    }

    *retv = PCRDR_SC_OK;
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
foil_get_property_in_dom(purcth_session *sess,
        purcth_dom *dom, uint64_t element_handle,
        const char *property, int *retv)
{
    (void)element_handle;
    (void)property;

    if (!validate_dom(sess, dom, retv)) {
        LOG_ERROR("Bad DOM pointer: %p.\n", dom);
        return PURC_VARIANT_INVALID;
    }

    if (!purc_is_valid_token(property, PURC_LEN_PROPERTY_NAME)) {
        *retv = PCRDR_SC_BAD_REQUEST;
        return PURC_VARIANT_INVALID;
    }

    *retv = PCRDR_SC_OK;
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
foil_set_property_in_dom(purcth_session *sess,
        purcth_dom *dom, uint64_t element_handle,
        const char *property, purc_variant_t value, int *retv)
{
    (void)element_handle;
    (void)property;
    (void)value;

    if (!validate_dom(sess, dom, retv)) {
        LOG_ERROR("Bad DOM pointer: %p.\n", dom);
        return PURC_VARIANT_INVALID;
    }

    if (!purc_is_valid_token(property, PURC_LEN_PROPERTY_NAME)) {
        *retv = PCRDR_SC_BAD_REQUEST;
        return PURC_VARIANT_INVALID;
    }

    *retv = PCRDR_SC_OK;
    return PURC_VARIANT_INVALID;
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
    rdr->cbs.get_plainwin_page = foil_get_plainwin_page;

    rdr->cbs.load = foil_load;
    rdr->cbs.update_dom = foil_update_dom;
    rdr->cbs.call_method_in_dom = foil_call_method_in_dom;
    rdr->cbs.get_property_in_dom = foil_get_property_in_dom;
    rdr->cbs.set_property_in_dom = foil_set_property_in_dom;

    return;
}

