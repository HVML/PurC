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

#undef NDEBUG

#include "seeker.h"
#include "endpoint.h"

#include <purc/purc-utils.h>
#include <assert.h>

struct tabbedwin_info {
    // the group identifier of the tabbedwin
    const char *group;

    // handle of this tabbedWindow; NULL for not used slot.
    void *handle;

    // number of widgets in this tabbedWindow
    int nr_widgets;

    // the active widget in this tabbedWindow
    int active_widget;

    // handles of all widgets in this tabbedWindow
    void *widgets[SEEKER_NR_WIDGETS];

    // handles of all DOM documents in all widgets.
    void *domdocs[SEEKER_NR_WIDGETS];
};

struct pcmcth_workspace {
    // handle of this workspace; NULL for not used slot
    void *handle;

    // name of the workspace
    char *name;

    // number of tabbed windows in this workspace
    int nr_tabbedwins;

    // number of plain windows in this workspace
    int nr_plainwins;

    // index of the active plain window in this workspace
    int active_plainwin;

    // information of all tabbed windows in this workspace.
    struct tabbedwin_info tabbedwins[SEEKER_NR_TABBEDWINDOWS];

    // handles of all plain windows in this workspace.
    void *plainwins[SEEKER_NR_PLAINWINDOWS];

    // handles of DOM documents in all plain windows.
    void *domdocs[SEEKER_NR_PLAINWINDOWS];

    // page identifier (plainwin:hello@main) -> owners;
    struct pcutils_kvlist   *page_owners;

    // widget group name (main) -> tabbedwindows;
    struct pcutils_kvlist   *group_tabbedwin;
};

struct pcmcth_rdr_data {
    // all available workspaces
    struct pcmcth_workspace workspaces[SEEKER_NR_WORKSPACES];
    int nr_workspaces;
    int active_workspace;
};

struct pcmcth_session {
    pcmcth_renderer *rdr;
    pcmcth_endpoint *edpt;
};

static int create_workspace(pcmcth_renderer *rdr, size_t slot,
        const char *name)
{
    struct pcmcth_workspace *workspace = rdr->impl->workspaces + slot;
    workspace->handle = &workspace->handle;
    workspace->name = strdup(name);

    workspace->page_owners = pcutils_kvlist_new(NULL);
    workspace->group_tabbedwin = pcutils_kvlist_new(NULL);
    return 0;
}

static int delete_each_ostack(void *ctxt, const char *name, void *data)
{
    (void)name;

    struct pcmcth_workspace *workspace = ctxt;
    purc_page_ostack_t ostack = *(purc_page_ostack_t *)data;
    purc_page_ostack_delete(workspace->page_owners, ostack);
    return 0;
}

static void destroy_workspace(pcmcth_renderer *rdr, size_t slot)
{
    struct pcmcth_workspace *workspace = rdr->impl->workspaces + slot;
    assert(workspace->handle);

    /* TODO: generate window and/or widget destroyed events */
    pcutils_kvlist_for_each_safe(workspace->page_owners, workspace,
            delete_each_ostack);

    pcutils_kvlist_delete(workspace->page_owners);
    pcutils_kvlist_delete(workspace->group_tabbedwin);

    free(workspace->name);
    memset(workspace, 0, sizeof(struct pcmcth_workspace));
}

static int seeker_prepare(pcmcth_renderer *rdr)
{
    rdr->impl = calloc(1, sizeof(*rdr->impl));
    if (rdr->impl) {
        /* create the default workspace */
        create_workspace(rdr, 0, PCRDR_DEFAULT_WORKSPACE);
        rdr->impl->nr_workspaces = 1;
        rdr->impl->active_workspace = 0;
    }

    return -1;
}

void seeker_set_renderer_callbacks(pcmcth_renderer *rdr)
{
    memset(&rdr->cbs, 0, sizeof(rdr->cbs));

    rdr->cbs.prepare = seeker_prepare;
#if 0
    rdr->cbs.handle_event = seeker_handle_event;
    rdr->cbs.cleanup = seeker_cleanup;
    rdr->cbs.create_session = seeker_create_session;
    rdr->cbs.remove_session = seeker_remove_session;

    rdr->cbs.find_page = seeker_find_page;
    rdr->cbs.get_special_plainwin = seeker_get_special_plainwin;
    rdr->cbs.create_plainwin = seeker_create_plainwin;
    rdr->cbs.update_plainwin = seeker_update_plainwin;
    rdr->cbs.destroy_plainwin = seeker_destroy_plainwin;

    rdr->cbs.load_edom = seeker_load_edom;
    rdr->cbs.register_crtn = seeker_register_crtn;
    rdr->cbs.revoke_crtn = seeker_revoke_crtn;
    rdr->cbs.update_udom = seeker_update_udom;
    rdr->cbs.call_method_in_udom = seeker_call_method_in_udom;
    rdr->cbs.call_method_in_session = seeker_call_method_in_session;
    rdr->cbs.get_property_in_udom = seeker_get_property_in_udom;
    rdr->cbs.set_property_in_udom = seeker_set_property_in_udom;
#endif

    return;
}

