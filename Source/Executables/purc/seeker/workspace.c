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
#include "session.h"
#include "util/sorted-array.h"

#include <assert.h>

pcmcth_workspace *seeker_wsp_new(pcmcth_renderer *rdr, const char *name,
        const char *title)
{
    pcmcth_workspace *workspace = calloc(1, sizeof(pcmcth_workspace));
    if (workspace) {
        workspace->page_owners = pcutils_kvlist_new(NULL);
        if (workspace->page_owners == NULL)
            goto failed;

        workspace->group_tabbedwin = pcutils_kvlist_new(NULL);
        if (workspace->group_tabbedwin == NULL)
            goto failed;

        workspace->rdr = rdr;
        workspace->root = seeker_widget_new(WSP_WIDGET_TYPE_ROOT,
                "root", "The root window");
        if (workspace->root == NULL) {
            LOG_ERROR("Failed to create root widget for workspace: %s\n", name);
            goto failed;
        }

        /* we use user_data of root to store the pointer to the workspace */
        workspace->root->user_data = workspace;
        workspace->name = kvlist_set_ex(&rdr->workspace_list, name, &workspace);
        workspace->title = strdup(title ? title : "Untitled");
    }

    return workspace;

failed:
    if (workspace->root)
        seeker_widget_delete(workspace->root);
    if (workspace->group_tabbedwin)
        pcutils_kvlist_delete(workspace->group_tabbedwin);
    if (workspace->page_owners)
        pcutils_kvlist_delete(workspace->page_owners);
    free(workspace);
    return NULL;
}

void seeker_wsp_delete(pcmcth_renderer *rdr, pcmcth_workspace *workspace)
{
    assert(workspace->root);

    seeker_widget_delete_deep(workspace->root);

    pcutils_kvlist_delete(workspace->page_owners);
    pcutils_kvlist_delete(workspace->group_tabbedwin);

    kvlist_delete(&rdr->workspace_list, workspace->name);

    free(workspace->title);
    free(workspace);
}

pcmcth_workspace *seeker_wsp_module_init(pcmcth_renderer *rdr)
{
    if (seeker_page_module_init(rdr))
        return NULL;

    kvlist_init(&rdr->workspace_list, NULL);

    /* create and return the default workspace */
    return seeker_wsp_new(rdr, PCRDR_DEFAULT_WORKSPACE, NULL);
}

void seeker_wsp_module_cleanup(pcmcth_renderer *rdr)
{
    (void)rdr;
    const char *name;
    void *next, *data;

    kvlist_for_each_safe(&rdr->workspace_list, name, next, data) {
        pcmcth_workspace *workspace = *(pcmcth_workspace **)data;
        seeker_wsp_delete(rdr, workspace);
    }

    kvlist_free(&rdr->workspace_list);
    seeker_page_module_cleanup(rdr);
}

static pcmcth_page *
create_window(pcmcth_workspace *workspace, pcmcth_session *sess,
        seeker_widget_type_k type,
        void *init_arg, const struct seeker_widget_info *style)
{
    (void)sess;
    (void)init_arg;

    struct seeker_widget *window;
    window = seeker_widget_new(type, style->name, style->title);
    if (window) {
        seeker_widget_append_child(workspace->root, window);
        return &window->page;
    }

    return NULL;
}

static pcmcth_page *
create_widget(pcmcth_workspace *workspace, pcmcth_session *sess,
        seeker_widget *parent, seeker_widget_type_k type,
        void *init_arg, const struct seeker_widget_info *style)
{
    (void)workspace;
    (void)sess;
    (void)init_arg;

    struct seeker_widget *widget;
    widget = seeker_widget_new(type, style->name, style->title);
    if (widget) {
        seeker_widget_append_child(parent, widget);
        return &widget->page;
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
    case WSP_WIDGET_TYPE_TABBEDWINDOW:
        return create_window(workspace, session, type, init_arg, style);

    case WSP_WIDGET_TYPE_ROOT:
        break;

    default:
        assert(parent);
        return create_widget(workspace, session, parent, type, init_arg, style);
        break;
    }

    return NULL;
}

static int
destroy_window(pcmcth_workspace *workspace, pcmcth_session *sess,
        seeker_widget *window)
{
    (void)workspace;
    (void)sess;

    seeker_widget_delete_deep(window);
    return PCRDR_SC_OK;
}

static int
destroy_widget(pcmcth_workspace *workspace, pcmcth_session *sess,
        seeker_widget *window, seeker_widget *widget)
{
    (void)workspace;
    (void)sess;
    (void)window;

    seeker_widget_delete_deep(widget);
    return PCRDR_SC_OK;
}

int seeker_wsp_destroy_widget(void *workspace, void *session,
        void *window, void *widget, seeker_widget_type_k type)
{
    (void)window;
    switch (type) {
    case WSP_WIDGET_TYPE_PLAINWINDOW:
    case WSP_WIDGET_TYPE_TABBEDWINDOW:
        return destroy_window(workspace, session, widget);

    case WSP_WIDGET_TYPE_ROOT:
        break;

    default:
        if (((seeker_widget *)widget)->parent == window) {
            return destroy_widget(workspace, session, window, widget);
        }
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

pcmcth_udom *seeker_wsp_load_edom_in_page(pcmcth_page *page,
        purc_variant_t edom, int *retv)
{
    pcmcth_udom *udom = seeker_udom_load_edom(page, edom, retv);
    return udom;
}

seeker_widget *seeker_wsp_find_widget(pcmcth_workspace *workspace,
        pcmcth_session *session, const char *page_id)
{
    seeker_widget *widget = NULL;

    char my_pageid[PURC_LEN_APP_NAME + strlen(page_id) + 2];
    const char *edpt = get_endpoint_uri(session->edpt);
    purc_extract_app_name(edpt, my_pageid);
    strcat(my_pageid, "/");
    strcat(my_pageid, page_id);

    void *data = pcutils_kvlist_get(workspace->page_owners, my_pageid);
    if (data == NULL)
        goto done;

    purc_page_ostack_t ostack = *(purc_page_ostack_t *)data;
    pcmcth_page *page = purc_page_ostack_get_page(ostack);
    widget = seeker_widget_from_page(page);

done:
    return widget;
}


