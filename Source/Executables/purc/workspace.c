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

#include <assert.h>

#include "callbacks.h"
#include "endpoint.h"
#include "workspace.h"
#include "udom.h"
#include "util/sorted-array.h"

static KVLIST(kv_app_workspace, NULL);

int foil_wsp_init(purcth_renderer *rdr)
{
    (void)rdr;
    return 0;
}

void foil_wsp_cleanup(purcth_renderer *rdr)
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

purcth_workspace *foil_wsp_create_or_get_workspace(purcth_endpoint* endpoint)
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

void foil_wsp_convert_style(void *workspace, void *session,
        struct wsp_widget_info *style, purc_variant_t toolkit_style)
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

static purcth_page *
create_plainwin(purcth_workspace *workspace, purcth_session *sess,
        void *init_arg, const struct wsp_widget_info *style)
{
    (void)workspace;
    (void)sess;
    (void)init_arg;
    (void)style;

    struct purcth_page *plainwin = NULL;

    /* TODO */
    return plainwin;
}

void *foil_wsp_create_widget(void *workspace, void *session,
        wsp_widget_type_t type, void *window,
        void *parent, void *init_arg, const struct wsp_widget_info *style)
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
destroy_plainwin(purcth_workspace *workspace, purcth_session *sess,
        purcth_page *plain_win)
{
    (void)workspace;
    (void)sess;
    (void)plain_win;

    /* TODO */
    return PCRDR_SC_OK;
}

int foil_wsp_destroy_widget(void *workspace, void *session,
        void *window, void *widget, wsp_widget_type_t type)
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
        void *widget, wsp_widget_type_t type,
        const struct wsp_widget_info *style)
{
    (void)workspace;
    (void)session;
    (void)widget;
    (void)type;
    (void)style;
}

purcth_udom *foil_wsp_load_edom_in_page(void *workspace, void *session,
        purcth_page *page, purc_variant_t edom)
{
    (void)workspace;
    (void)session;

    purcth_udom *udom = foil_udom_new(page);

    purcth_rdrbox *rdrbox;
    if (udom) {
        rdrbox = foil_udom_load_edom(udom, NULL, NULL);
    }

    if (rdrbox == NULL) {
        foil_udom_delete(udom);
        udom = NULL;
    }

    return udom;
}

