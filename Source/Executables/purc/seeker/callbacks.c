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
#include "util/sorted-array.h"

#include <purc/purc-utils.h>
#include <assert.h>
#include <unistd.h>

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
    const char *name;

    // number of tabbed windows in this workspace
    int nr_tabbedwins;

    // number of plain windows in this workspace
    int nr_plainwins;

    // page identifier (plainwin:hello@main) -> owners;
    struct pcutils_kvlist   *page_owners;

    // widget group name (main) -> tabbedwindows;
    struct pcutils_kvlist   *group_tabbedwin;
};

struct pcmcth_rdr_data {
    // all available workspaces
    int nr_workspaces;
    int active_workspace;
};

struct pcmcth_session {
    pcmcth_renderer *rdr;
    pcmcth_endpoint *edpt;

    /* the sorted array of all valid handles */
    struct sorted_array *all_handles;
};

static int create_workspace(pcmcth_renderer *rdr, const char *name)
{
    pcmcth_workspace *workspace = calloc(1, sizeof(pcmcth_workspace));
    if (workspace) {
        workspace->page_owners = pcutils_kvlist_new(NULL);
        workspace->group_tabbedwin = pcutils_kvlist_new(NULL);
        workspace->name = kvlist_set_ex(&rdr->workspace_list, name, &workspace);
        return 0;
    }

    return -1;
}

static int delete_each_ostack(void *ctxt, const char *name, void *data)
{
    (void)name;

    struct pcmcth_workspace *workspace = ctxt;
    purc_page_ostack_t ostack = *(purc_page_ostack_t *)data;
    purc_page_ostack_delete(workspace->page_owners, ostack);
    return 0;
}

static void destroy_workspace(pcmcth_renderer *rdr,
        struct pcmcth_workspace *workspace)
{
    /* TODO: generate window and/or widget destroyed events */
    pcutils_kvlist_for_each_safe(workspace->page_owners, workspace,
            delete_each_ostack);

    pcutils_kvlist_delete(workspace->page_owners);
    pcutils_kvlist_delete(workspace->group_tabbedwin);

    kvlist_delete(&rdr->workspace_list, workspace->name);
}

static int seeker_prepare(pcmcth_renderer *rdr)
{
    rdr->impl = calloc(1, sizeof(*rdr->impl));
    if (rdr->impl) {
        /* create the default workspace */
        create_workspace(rdr, PCRDR_DEFAULT_WORKSPACE);
        rdr->impl->nr_workspaces = 1;
    }

    return 0;
}

static int
seeker_handle_event(pcmcth_renderer *rdr, unsigned long long timeout_usec)
{
    (void)rdr;
    usleep(timeout_usec);
    return 0;
}

static void seeker_cleanup(pcmcth_renderer *rdr)
{
    const char *name;
    void *next, *data;
    kvlist_for_each_safe(&rdr->workspace_list, name, next, data) {
        pcmcth_workspace *workspace = *(pcmcth_workspace **)data;
        destroy_workspace(rdr, workspace);
    }

    kvlist_free(&rdr->workspace_list);
    free(rdr->impl);
}

static pcmcth_session *
seeker_create_session(pcmcth_renderer *rdr, pcmcth_endpoint *edpt)
{
    pcmcth_session* sess = calloc(1, sizeof(pcmcth_session));
    if (sess) {
        sess->rdr = rdr;
        sess->edpt = edpt;
    }

    sess->all_handles = sorted_array_create(SAFLAG_DEFAULT, 8, NULL, NULL);
    if (sess->all_handles == NULL) {
        goto failed;
    }

    return sess;

failed:
    free(sess);
    return NULL;
}

static int delete_ostack_of_session(void *ctxt, const char *name, void *data)
{
    (void)name;

    pcmcth_session *sess = ctxt;
    purc_page_ostack_t ostack = *(purc_page_ostack_t *)data;

    struct purc_page_owner to_reload;
    to_reload = purc_page_ostack_revoke_session(ostack, ctxt);
    if (to_reload.corh) {
        assert(to_reload.sess);
        // TODO: send reloadPage request to another endpoint
    }

    pcmcth_page *page = purc_page_ostack_get_page(ostack);
    if (sorted_array_find(sess->all_handles, PTR2U64(page), NULL) >= 0) {
        /* TODO: delete page */
    }

    return 0;
}

static int seeker_remove_session(pcmcth_session *sess)
{
    const char *name;
    void *data;

    kvlist_for_each(&sess->rdr->workspace_list, name,  data) {
        pcmcth_workspace *workspace = *(pcmcth_workspace **)data;
        pcutils_kvlist_for_each_safe(workspace->page_owners,
                sess, delete_ostack_of_session);
    }

    sorted_array_destroy(sess->all_handles);
    free(sess);
    return PCRDR_SC_OK;
}

void seeker_set_renderer_callbacks(pcmcth_renderer *rdr)
{
    memset(&rdr->cbs, 0, sizeof(rdr->cbs));

    rdr->cbs.prepare = seeker_prepare;
    rdr->cbs.handle_event = seeker_handle_event;
    rdr->cbs.cleanup = seeker_cleanup;
    rdr->cbs.create_session = seeker_create_session;
    rdr->cbs.remove_session = seeker_remove_session;

#if 0
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

