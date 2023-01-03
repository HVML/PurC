/*
** @file workspace.c
** @author Vincent Wei
** @date 2022/10/05
** @brief The implementation of workspace of Foil.
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

#include "workspace.h"
#include "purcmc-thread.h"
#include "endpoint.h"
#include "udom.h"
#include "page.h"
#include "util/sorted-array.h"

#include <assert.h>

int foil_wsp_module_init(pcmcth_renderer *rdr)
{
    kvlist_init(&rdr->workspace_list, NULL);

    return foil_page_module_init(rdr);
}

static pcmcth_workspace *workspace_new(pcmcth_renderer *rdr,
        const char *app_key)
{
    pcmcth_workspace *workspace = calloc(1, sizeof(pcmcth_workspace));
    if (workspace) {
        workspace->cols = rdr->impl->cols;
        workspace->rows = rdr->impl->rows;
        workspace->layouter = NULL;
        foil_rect rc;
        foil_rect_set(&rc, 0, 0, workspace->cols, workspace->rows);
        workspace->root = foil_widget_new(
                WSP_WIDGET_TYPE_ROOT, WSP_WIDGET_BORDER_NONE,
                "root", NULL, &rc);
        if (workspace->root == NULL) {
            free(workspace);
            workspace = NULL;
            goto done;
        }

        kvlist_set(&rdr->workspace_list, app_key, &workspace);
    }

done:
    return workspace;
}

static void workspace_delete(pcmcth_workspace *workspace)
{
    assert(workspace->root);

    if (workspace->layouter) {
        // TODO: ws_layouter_delete(workspace->layouter);
    }

    foil_widget_delete_deep(workspace->root);

    free(workspace);
}

void foil_wsp_module_cleanup(pcmcth_renderer *rdr)
{
    (void)rdr;
    const char *name;
    void *next, *data;

    kvlist_for_each_safe(&rdr->workspace_list, name, next, data) {
        pcmcth_workspace *workspace = *(pcmcth_workspace **)data;
        workspace_delete(workspace);
    }

    kvlist_free(&rdr->workspace_list);
    foil_page_module_cleanup(rdr);
}

pcmcth_workspace *foil_wsp_create_or_get_workspace(pcmcth_renderer *rdr,
        pcmcth_endpoint* endpoint)
{
    char host[PURC_LEN_HOST_NAME + 1];
    char app[PURC_LEN_APP_NAME + 1];
    const char *edpt_uri = get_endpoint_uri(endpoint);

    purc_extract_host_name(edpt_uri, host);
    purc_extract_app_name(edpt_uri, app);

    char app_key[PURC_LEN_ENDPOINT_NAME + 1];
    strcpy(app_key, host);
    strcat(app_key, "-");
    strcat(app_key, app);

    void *data;
    pcmcth_workspace *workspace;
    if ((data = kvlist_get(&rdr->workspace_list, app_key))) {
        workspace = *(pcmcth_workspace **)data;
        assert(workspace);
    }
    else {
        workspace = workspace_new(rdr, app_key);
    }

    return workspace;
}

void foil_wsp_convert_style(void *workspace, void *session,
        struct foil_widget_info *style, purc_variant_t toolkit_style)
{
    (void)workspace;
    (void)session;
    style->backgroundColor = NULL;

    if (toolkit_style == PURC_VARIANT_INVALID)
        return;

    purc_variant_t tmp;
    if ((tmp = purc_variant_object_get_by_ckey(toolkit_style, "darkMode")) &&
            purc_variant_is_true(tmp)) {
        style->darkMode = true;
    }

    if ((tmp = purc_variant_object_get_by_ckey(toolkit_style, "fullScreen")) &&
            purc_variant_is_true(tmp)) {
        style->fullScreen = true;
    }

    if ((tmp = purc_variant_object_get_by_ckey(toolkit_style, "withToolbar")) &&
            purc_variant_is_true(tmp)) {
        style->withToolbar = true;
    }

    if ((tmp = purc_variant_object_get_by_ckey(toolkit_style,
                    "backgroundColor"))) {
        const char *value = purc_variant_get_string_const(tmp);
        if (value) {
            style->backgroundColor = value;
        }
    }

    style->flags |= WSP_WIDGET_FLAG_TOOLKIT;
}

static pcmcth_page *
create_plainwin(pcmcth_workspace *workspace, pcmcth_session *sess,
        void *init_arg, const struct foil_widget_info *style)
{
    (void)sess;
    (void)init_arg;

    struct foil_widget *plainwin;
    foil_rect rc;
    foil_rect_set(&rc, 0, 0, workspace->cols, workspace->rows);
    plainwin = foil_widget_new(
            WSP_WIDGET_TYPE_PLAINWINDOW, WSP_WIDGET_BORDER_NONE,
            style->name, style->title, &rc);
    if (plainwin) {
        foil_widget_append_child(workspace->root, plainwin);
        return &plainwin->page;
    }

    return NULL;
}

void *foil_wsp_create_widget(void *workspace, void *session,
        foil_widget_type_k type, void *window,
        void *parent, void *init_arg, const struct foil_widget_info *style)
{
    (void)window;
    (void)parent;

    switch (type) {
    case WSP_WIDGET_TYPE_PLAINWINDOW:
        return create_plainwin(workspace, session, init_arg, style);

    default:
        /* TODO */
        break;
    }

    return NULL;
}

static int
destroy_plainwin(pcmcth_workspace *workspace, pcmcth_session *sess,
        foil_widget *plainwin)
{
    (void)workspace;
    (void)sess;

    foil_widget_delete(plainwin);
    return PCRDR_SC_OK;
}

int foil_wsp_destroy_widget(void *workspace, void *session,
        void *window, void *widget, foil_widget_type_k type)
{
    (void)window;
    switch (type) {
    case WSP_WIDGET_TYPE_PLAINWINDOW:
        return destroy_plainwin(workspace, session, widget);

    default:
        /* TODO */
        break;
    }

    return PCRDR_SC_BAD_REQUEST;
}

void foil_wsp_update_widget(void *workspace, void *session,
        void *widget, foil_widget_type_k type,
        const struct foil_widget_info *style)
{
    (void)workspace;
    (void)session;
    (void)widget;
    (void)type;
    (void)style;
}

pcmcth_udom *foil_wsp_load_edom_in_page(void *workspace, void *session,
        pcmcth_page *page, purc_variant_t edom, int *retv)
{
    (void)workspace;
    (void)session;

    pcmcth_udom *udom = foil_udom_load_edom(page, edom, retv);
    return udom;
}

