/*
** @file workspace.c
** @author Vincent Wei
** @date 2023/10/20
** @brief The implementation of workspace of Seeker.
**
** Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
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

#undef NDEBUG

#include "workspace.h"
#include "purcmc-thread.h"
#include "endpoint.h"
#include "page.h"
#include "udom.h"
#include "util/sorted-array.h"

#include <assert.h>

int seeker_wsp_module_init(pcmcth_renderer *rdr)
{
    kvlist_init(&rdr->workspace_list, NULL);

    return seeker_page_module_init(rdr);
}

static pcmcth_workspace *workspace_new(pcmcth_renderer *rdr,
        const char *name)
{
    pcmcth_workspace *workspace = calloc(1, sizeof(pcmcth_workspace));
    if (workspace) {
        workspace->page_owners = pcutils_kvlist_new(NULL);
        if (workspace->page_owners == NULL)
            goto failed;

        workspace->rdr = rdr;
        workspace->root = seeker_widget_new(WSP_WIDGET_TYPE_ROOT,
                "root", NULL);
        if (workspace->root == NULL)
            goto failed;

        /* we use user_data of root to store the pointer to the workspace */
        workspace->root->user_data = workspace;
        workspace->name = kvlist_set_ex(&rdr->workspace_list, name, &workspace);
    }

    return workspace;

failed:
    if (workspace->root)
        seeker_widget_delete(workspace->root);
    if (workspace->page_owners)
        pcutils_kvlist_delete(workspace->page_owners);
    free(workspace);
    return NULL;
}

static void workspace_delete(pcmcth_workspace *workspace)
{
    assert(workspace->root);

    LOG_DEBUG("destroy page owners map...\n");
    pcutils_kvlist_delete(workspace->page_owners);

    seeker_widget_delete_deep(workspace->root);

    free(workspace);
}

void seeker_wsp_module_cleanup(pcmcth_renderer *rdr)
{
    (void)rdr;
    const char *name;
    void *next, *data;

    kvlist_for_each_safe(&rdr->workspace_list, name, next, data) {
        pcmcth_workspace *workspace = *(pcmcth_workspace **)data;
        workspace_delete(workspace);
    }

    kvlist_free(&rdr->workspace_list);
    seeker_page_module_cleanup(rdr);
}

pcmcth_workspace *seeker_wsp_create_or_get_workspace(pcmcth_renderer *rdr,
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

void seeker_wsp_convert_style(void *workspace, void *session,
        struct seeker_widget_info *style, purc_variant_t toolkit_style)
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
        void *init_arg, const struct seeker_widget_info *style)
{
    (void)sess;
    (void)init_arg;

    struct seeker_widget *plainwin;
    plainwin = seeker_widget_new(WSP_WIDGET_TYPE_PLAINWINDOW,
            style->name, style->title);
    if (plainwin) {
        seeker_widget_append_child(workspace->root, plainwin);
        return &plainwin->page;
    }

    return NULL;
}

void *seeker_wsp_create_widget(void *workspace, void *session,
        seeker_widget_type_k type, void *window,
        void *parent, void *init_arg, const struct seeker_widget_info *style)
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
        seeker_widget *plainwin)
{
    (void)workspace;
    (void)sess;

    seeker_widget_delete(plainwin);
    return PCRDR_SC_OK;
}

int seeker_wsp_destroy_widget(void *workspace, void *session,
        void *window, void *widget, seeker_widget_type_k type)
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

void seeker_wsp_update_widget(void *workspace, void *session,
        void *widget, seeker_widget_type_k type,
        const struct seeker_widget_info *style)
{
    (void)workspace;
    (void)session;
    (void)widget;
    (void)type;
    (void)style;
}

pcmcth_udom *seeker_wsp_load_edom_in_page(void *workspace, void *session,
        pcmcth_page *page, purc_variant_t edom, int *retv)
{
    (void)workspace;
    (void)session;

    if (page->udom) {
        page->udom = NULL;
    }

    pcmcth_udom *udom = seeker_udom_load_edom(page, edom, retv);
    return udom;
}

seeker_widget *seeker_wsp_find_widget(void *workspace, void *session,
        const char *id)
{
    (void)session;

    seeker_widget* widget = NULL;
    pcmcth_workspace *wsp = workspace;
    void *data = pcutils_kvlist_get(wsp->page_owners, id);
    if (data == NULL)
        goto done;

    purc_page_ostack_t ostack = *(purc_page_ostack_t *)data;
    widget = purc_page_ostack_get_page(ostack);

done:
    return widget;
}


