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
#include "workspace.h"
#include "session.h"
#include "timer.h"
#include "finder.h"
#include "util/sorted-array.h"

#include <purc/purc-utils.h>
#include <purc/purc-helpers.h>
#include <assert.h>
#include <unistd.h>

/* handle types */
enum {
    HT_WORKSPACE = 0,
    HT_PLAINWIN,
    HT_TABBEDWIN,
    HT_CONTAINER,
    HT_WIDGET,
    HT_UDOM,
};

static int prepare(pcmcth_renderer *rdr)
{
    rdr->impl = calloc(1, sizeof(*rdr->impl));
    if (rdr->impl) {
        rdr->impl->def_wsp = seeker_wsp_module_init(rdr);
        if (rdr->impl->def_wsp == NULL) {
            free(rdr->impl);
            rdr->impl = NULL;
            return -1;
        }
    }

    pcmcth_timer_new(rdr, SEEKER_UNIX_FINDER_NAME,
            seeker_look_for_local_renderer, SEEKER_UNIX_FINDER_INTERVAL, rdr);

#if PCA_ENABLE_DNSSD
    rdr->impl->dnssd = purc_dnssd_connect(NULL,
            seeker_dnssd_on_service_discovered, rdr);
    if (rdr->impl->dnssd == NULL) {
        LOG_WARN("Failed to connect to mDNS Responder\n");
    }
    else {
        rdr->impl->browsing_handle = purc_dnssd_start_browsing(rdr->impl->dnssd,
                "_purcmc._tcp", NULL);
        if (rdr->impl->browsing_handle == NULL) {
            LOG_WARN("Failed to start browsing\n");
            purc_dnssd_disconnect(rdr->impl->dnssd);
            rdr->impl->dnssd = NULL;
        }
        else {
            pcmcth_timer_new(rdr, SEEKER_NET_FINDER_NAME,
                    seeker_look_for_local_renderer, SEEKER_FINDER_INTERVAL, rdr);
        }
    }
#endif
    return 0;
}

static int
handle_event(pcmcth_renderer *rdr, unsigned long long timeout_usec)
{
#if PCA_ENABLE_DNSSD
    if (rdr->impl->dnssd) {
        int fd = purc_dnssd_fd(rdr->impl->dnssd);
        assert(fd >= 0);

        fd_set select_set;
        FD_ZERO(&select_set);
        FD_SET(fd, &select_set); /* Add stdin */

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = timeout_usec;
        int v = select(fd + 1, &select_set, NULL, NULL, &timeout);

        if (v > 0 && FD_ISSET(fd, &select_set)) {
            purc_dnssd_process_result(rdr->impl->dnssd);
        }
    }
    else {
        usleep(timeout_usec);
    }
#else
    (void)rdr;
    usleep(timeout_usec);
#endif
    return 0;
}

static void cleanup(pcmcth_renderer *rdr)
{
#if PCA_ENABLE_DNSSD
    if (rdr->impl->browsing_handle) {
        purc_dnssd_stop_browsing(rdr->impl->dnssd,
                rdr->impl->browsing_handle);
        purc_dnssd_disconnect(rdr->impl->dnssd);
    }
#endif

    seeker_wsp_module_cleanup(rdr);
    free(rdr->impl);
}

static pcmcth_session *
create_session(pcmcth_renderer *rdr, pcmcth_endpoint *edpt)
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

static int remove_session(pcmcth_session *sess)
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

static pcmcth_workspace *
create_workspace(pcmcth_session *sess,
            const char *name, const char *title, purc_variant_t properties,
            int *retv)
{
    (void)properties;
    pcmcth_workspace *wsp = NULL;

    if (kvlist_get(&sess->rdr->workspace_list, name)) {
        *retv = PCRDR_SC_CONFLICT;
    }
    else {
        wsp = seeker_wsp_new(sess->rdr, name, title);
        if (wsp == NULL) {
            *retv = PCRDR_SC_INSUFFICIENT_STORAGE;
        }
        else {
            *retv = PCRDR_SC_OK;
        }
    }

    return wsp;
}

static pcmcth_workspace *
validate_workspace(pcmcth_renderer *rdr, pcmcth_workspace *workspace)
{
    const char *name;
    void *data;

    if (workspace == NULL) {
        workspace = rdr->impl->def_wsp;
        return workspace;
    }

    kvlist_for_each(&rdr->workspace_list, name, data) {
        if (*(pcmcth_workspace **)data == workspace) {
            return workspace;
        }
    }

    return NULL;
}

static int
update_workspace(pcmcth_session *sess, pcmcth_workspace *workspace,
            const char *property, const char *value)
{
    (void)property;
    (void)value;

    if (validate_workspace(sess->rdr, workspace) == NULL) {
        return PCRDR_SC_NOT_FOUND;
    }

    return PCRDR_SC_OK;
}

static int
destroy_workspace(pcmcth_session *sess, pcmcth_workspace *workspace)
{
    workspace = validate_workspace(sess->rdr, workspace);

    if (workspace == NULL) {
        return PCRDR_SC_NOT_FOUND;
    }

    if (strcmp(workspace->name, PCRDR_DEFAULT_WORKSPACE) == 0)
        return PCRDR_SC_FORBIDDEN;

    if (workspace->root->first) {
        return PCRDR_SC_METHOD_NOT_ALLOWED;
    }

    seeker_wsp_delete(sess->rdr, workspace);
    return PCRDR_SC_OK;
}

static pcmcth_workspace *
find_workspace(pcmcth_session *sess, const char *name)
{
    pcmcth_workspace *workspace = NULL;

    void *data;
    if ((data = kvlist_get(&sess->rdr->workspace_list, name))) {
        workspace = *(pcmcth_workspace **)data;
    }

    return workspace;
}

static pcmcth_workspace *get_last_workspace(pcmcth_renderer *rdr)
{
    const char *name;
    void *data;
    pcmcth_workspace *last = NULL;

    kvlist_for_each(&rdr->workspace_list, name,  data) {
        last = *(pcmcth_workspace **)data;
    }

    return last;
}

static pcmcth_workspace *
get_special_workspace(pcmcth_session *sess,
            pcrdr_resname_workspace_k v)
{
    switch (v) {
        case PCRDR_K_RESNAME_WORKSPACE_default:
        case PCRDR_K_RESNAME_WORKSPACE_active:
        case PCRDR_K_RESNAME_WORKSPACE_first:
            return sess->rdr->impl->def_wsp;
        case PCRDR_K_RESNAME_WORKSPACE_last:
            return get_last_workspace(sess->rdr);
    }

    return NULL;
}

static int
set_page_groups(pcmcth_session *sess, pcmcth_workspace *workspace,
            const char *content, size_t length)
{
    if (validate_workspace(sess->rdr, workspace) == NULL) {
        return PCRDR_SC_NOT_FOUND;
    }

    (void)content;
    (void)length;
    return PCRDR_SC_OK;
}

static int
add_page_groups(pcmcth_session *sess, pcmcth_workspace *workspace,
            const char *content, size_t length)
{
    if (validate_workspace(sess->rdr, workspace) == NULL) {
        return PCRDR_SC_NOT_FOUND;
    }

    (void)content;
    (void)length;
    return PCRDR_SC_OK;
}

static int
remove_page_group(pcmcth_session *sess, pcmcth_workspace *workspace,
            const char* gid)
{
    if (validate_workspace(sess->rdr, workspace) == NULL) {
        return PCRDR_SC_NOT_FOUND;
    }

    (void)gid;
    return PCRDR_SC_OK;
}

struct prefix_group {
    const char *prefix;
    const char *group;
    struct purc_page_ostack *found;

    size_t prefix_len;
    pcrdr_resname_page_k page_type;
};

static int filter_via_prefix_and_group(void *ctxt, const char *name, void *data)
{
    struct prefix_group *cond = ctxt;
    bool matched = false;

    if (strncmp(name, cond->prefix, cond->prefix_len) == 0) {
        const char *group = NULL;
        const char *at = strrchr(name, '@');
        if (at) {
            group = at + 1;
            const char *slash = strchr(group, '/');
            if (slash)
                group = slash + 1;
        }

        if (cond->group == NULL && group == NULL) {
            matched = true;
        }
        else if (cond->group && group && strcmp(cond->group, group) == 0) {
            matched = true;
        }
    }

    if (matched) {
        cond->found = *(purc_page_ostack_t *)data;
        switch (cond->page_type) {
            case PCRDR_K_RESNAME_PAGE_active:
            case PCRDR_K_RESNAME_PAGE_first:
                return 1;

            case PCRDR_K_RESNAME_PAGE_last:
                break;
        }
    }

    return 0;
}

static pcmcth_page *get_special_plainwin(pcmcth_session *sess,
        pcmcth_workspace *workspace, const char *group,
        pcrdr_resname_page_k page_type)
{
    workspace = validate_workspace(sess->rdr, workspace);
    if (workspace == NULL) {
        return NULL;
    }

    char buf[PURC_LEN_APP_NAME + sizeof(PURC_PREFIX_PLAINWIN)];
    const char *edpt = get_endpoint_uri(sess->edpt);
    purc_extract_app_name(edpt, buf);
    strcat(buf, "/");
    strcat(buf, PURC_PREFIX_PLAINWIN);

    struct prefix_group ctxt = { buf, group, NULL, strlen(buf), page_type };
    pcutils_kvlist_for_each(workspace->page_owners, &ctxt,
            filter_via_prefix_and_group);

    if (ctxt.found) {
        return purc_page_ostack_get_page(ctxt.found);
    }

    return NULL;
}

static pcmcth_page *find_page(pcmcth_session *sess,
        pcmcth_workspace *workspace, const char *page_id)
{
    workspace = validate_workspace(sess->rdr, workspace);
    if (workspace == NULL) {
        return NULL;
    }

    char buf[PURC_LEN_APP_NAME + strlen(page_id) + 2];
    const char *edpt = get_endpoint_uri(sess->edpt);
    purc_extract_app_name(edpt, buf);
    strcat(buf, "/");
    strcat(buf, page_id);

    void *data;
    data = pcutils_kvlist_get(workspace->page_owners, buf);
    if (data != NULL) {
        struct purc_page_ostack *ostack = *(struct purc_page_ostack **)data;
        return purc_page_ostack_get_page(ostack);
    }

    return NULL;
}

static pcmcth_page *create_plainwin(pcmcth_session *sess,
        pcmcth_workspace *workspace,
        const char *page_id, const char *group, const char *name,
        const char *class_name, const char *title, const char *layout_style,
        purc_variant_t toolkit_style, int *retv)
{
    (void)class_name;
    (void)layout_style;
    (void)toolkit_style;

    pcmcth_page *plainwin = NULL;
    char my_pageid[PURC_LEN_APP_NAME + strlen(page_id) + 2];

    workspace = validate_workspace(sess->rdr, workspace);
    if (workspace == NULL) {
        *retv = PCRDR_SC_NOT_FOUND;
        goto done;
    }

    const char *edpt = get_endpoint_uri(sess->edpt);
    purc_extract_app_name(edpt, my_pageid);
    strcat(my_pageid, "/");
    strcat(my_pageid, page_id);

    if (pcutils_kvlist_get(workspace->page_owners, my_pageid)) {
        LOG_WARN("Duplicated page identifier: %s\n", my_pageid);
        *retv = PCRDR_SC_CONFLICT;
        goto done;
    }

    struct seeker_widget_info style = { };
    style.flags = WSP_WIDGET_FLAG_NAME | WSP_WIDGET_FLAG_TITLE;
    style.name = name;
    style.title = title;

    plainwin = seeker_wsp_create_widget(workspace, sess,
            WSP_WIDGET_TYPE_PLAINWINDOW, NULL, NULL, NULL, &style);

    if (plainwin) {
        plainwin->ostack = purc_page_ostack_new(workspace->page_owners,
                    my_pageid, plainwin);
        sorted_array_add(sess->all_handles, PTR2U64(plainwin),
                INT2PTR(HT_PLAINWIN));
        *retv = PCRDR_SC_OK;
    }
    else {
        LOG_ERROR("Failed to create a plain window: %s@%s\n", name, group);
        *retv = PCRDR_SC_INSUFFICIENT_STORAGE;
    }

done:
    return plainwin;
}

static int
update_plainwin(pcmcth_session *sess, pcmcth_workspace *workspace,
        pcmcth_page *plain_win, const char *property, purc_variant_t value)
{
    workspace = validate_workspace(sess->rdr, workspace);
    if (workspace == NULL) {
        return PCRDR_SC_NOT_FOUND;
    }

    void *data;
    if (sorted_array_find(sess->all_handles, PTR2U64(plain_win), &data) < 0) {
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
        struct seeker_widget_info info = { };

        info.title = purc_variant_get_string_const(value);
        if (info.title) {
            info.flags = WSP_WIDGET_FLAG_TITLE;
            seeker_wsp_update_widget(workspace, sess,
                    plain_win, WSP_WIDGET_TYPE_PLAINWINDOW, &info);
        }
        else {
            return PCRDR_SC_BAD_REQUEST;
        }
    }
    else if (strcmp(property, "layoutStyle") == 0) {
        /* do nothing */
    }
    else if (strcmp(property, "toolkitStyle") == 0) {
        /* do nothing */
    }
    else {
        return PCRDR_SC_BAD_REQUEST;
    }

    return PCRDR_SC_OK;
}

static int
destroy_plainwin(pcmcth_session *sess, pcmcth_workspace *workspace,
        pcmcth_page *plain_win)
{
    workspace = validate_workspace(sess->rdr, workspace);
    if (workspace == NULL) {
        return PCRDR_SC_NOT_FOUND;
    }

    void *data;
    if (sorted_array_find(sess->all_handles, PTR2U64(plain_win), &data) < 0) {
        return PCRDR_SC_NOT_FOUND;
    }

    if ((uintptr_t)data != HT_PLAINWIN) {
        return PCRDR_SC_BAD_REQUEST;
    }

    return seeker_wsp_destroy_widget(workspace, sess, plain_win, plain_win,
        WSP_WIDGET_TYPE_PLAINWINDOW);
}

#define PREFIX_TABBEDWIN           "tabbedwin:"

static seeker_widget *
create_or_get_tabbedwin(pcmcth_session *sess, pcmcth_workspace *workspace,
        const char *group)
{
    char tabwinid[PURC_LEN_APP_NAME + sizeof(PREFIX_TABBEDWIN) + strlen(group) + 2];
    const char *edpt = get_endpoint_uri(sess->edpt);
    purc_extract_app_name(edpt, tabwinid);
    strcat(tabwinid, "/");
    strcat(tabwinid, PREFIX_TABBEDWIN);
    strcat(tabwinid, ":");
    strcat(tabwinid, group);

    void *data = pcutils_kvlist_get(workspace->group_tabbedwin, tabwinid);
    if (data) {
        return *(seeker_widget**)data;
    }

    struct seeker_widget_info style = { };
    style.flags = WSP_WIDGET_FLAG_NAME | WSP_WIDGET_FLAG_TITLE;
    style.name = group;
    style.title = "Untitled";

    seeker_widget *tabbedwin;
    tabbedwin = seeker_wsp_create_widget(workspace, sess,
            WSP_WIDGET_TYPE_TABBEDWINDOW, NULL, NULL, NULL, &style);

    return tabbedwin;
}

static pcmcth_page *
create_widget(pcmcth_session *sess, pcmcth_workspace *workspace,
            const char *page_id, const char *group, const char *name,
            const char *class_name, const char *title, const char *layout_style,
            purc_variant_t toolkit_style, int *retv)
{
    (void)class_name;
    (void)layout_style;
    (void)toolkit_style;

    pcmcth_page *widget = NULL;
    char my_pageid[PURC_LEN_APP_NAME + strlen(page_id) + 2];

    workspace = validate_workspace(sess->rdr, workspace);
    if (workspace == NULL) {
        *retv = PCRDR_SC_NOT_FOUND;
        goto done;
    }

    const char *edpt = get_endpoint_uri(sess->edpt);
    purc_extract_app_name(edpt, my_pageid);
    strcat(my_pageid, "/");
    strcat(my_pageid, page_id);

    if (pcutils_kvlist_get(workspace->page_owners, my_pageid)) {
        LOG_WARN("Duplicated page identifier for widget: %s\n", my_pageid);
        *retv = PCRDR_SC_CONFLICT;
        goto done;
    }

    seeker_widget *tabbedwin;
    tabbedwin = create_or_get_tabbedwin(sess, workspace, group);
    if (tabbedwin == NULL) {
        *retv = PCRDR_SC_INSUFFICIENT_STORAGE;
        goto done;
    }

    struct seeker_widget_info style = { };
    style.flags = WSP_WIDGET_FLAG_NAME | WSP_WIDGET_FLAG_TITLE;
    style.name = name;
    style.title = title;

    widget = seeker_wsp_create_widget(workspace, sess,
            WSP_WIDGET_TYPE_TABBEDPAGE, tabbedwin, tabbedwin, NULL, &style);

    if (widget) {
        widget->ostack = purc_page_ostack_new(workspace->page_owners,
                    my_pageid, widget);
        sorted_array_add(sess->all_handles, PTR2U64(widget),
                INT2PTR(HT_WIDGET));
        *retv = PCRDR_SC_OK;
    }
    else {
        LOG_ERROR("Failed to create a widget: %s@%s\n", name, group);
        *retv = PCRDR_SC_INSUFFICIENT_STORAGE;
    }

done:
    return widget;
}

static int
update_widget(pcmcth_session *sess, pcmcth_workspace *workspace,
            pcmcth_page *page, const char *property, purc_variant_t value)
{
    workspace = validate_workspace(sess->rdr, workspace);
    if (workspace == NULL) {
        return PCRDR_SC_NOT_FOUND;
    }

    void *data;
    if (sorted_array_find(sess->all_handles, PTR2U64(page), &data) < 0) {
        return PCRDR_SC_NOT_FOUND;
    }

    if ((uintptr_t)data != HT_WIDGET) {
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
        struct seeker_widget_info info = { };

        info.title = purc_variant_get_string_const(value);
        if (info.title) {
            info.flags = WSP_WIDGET_FLAG_TITLE;
            seeker_wsp_update_widget(workspace, sess,
                    page, WSP_WIDGET_TYPE_TABBEDPAGE, &info);
        }
        else {
            return PCRDR_SC_BAD_REQUEST;
        }
    }
    else if (strcmp(property, "layoutStyle") == 0) {
        /* do nothing */
    }
    else if (strcmp(property, "toolkitStyle") == 0) {
        /* do nothing */
    }
    else {
        return PCRDR_SC_BAD_REQUEST;
    }

    return PCRDR_SC_OK;
}

static int
destroy_widget(pcmcth_session *sess, pcmcth_workspace *workspace,
            pcmcth_page *page)
{
    workspace = validate_workspace(sess->rdr, workspace);
    if (workspace == NULL) {
        return PCRDR_SC_NOT_FOUND;
    }

    void *data;
    if (sorted_array_find(sess->all_handles, PTR2U64(page), &data) < 0) {
        return PCRDR_SC_NOT_FOUND;
    }

    if ((uintptr_t)data != HT_WIDGET) {
        return PCRDR_SC_BAD_REQUEST;
    }

    return seeker_wsp_destroy_widget(workspace, sess, page, page,
        WSP_WIDGET_TYPE_TABBEDPAGE);
}

static pcmcth_page *
get_special_widget(pcmcth_session *sess, pcmcth_workspace *workspace,
        const char *group, pcrdr_resname_page_k v)
{
    workspace = validate_workspace(sess->rdr, workspace);
    if (workspace == NULL) {
        return NULL;
    }

    char tabwinid[PURC_LEN_APP_NAME + sizeof(PREFIX_TABBEDWIN) + strlen(group) + 2];
    const char *edpt = get_endpoint_uri(sess->edpt);
    purc_extract_app_name(edpt, tabwinid);
    strcat(tabwinid, "/");
    strcat(tabwinid, PREFIX_TABBEDWIN);
    strcat(tabwinid, ":");
    strcat(tabwinid, group);

    void *data = pcutils_kvlist_get(workspace->group_tabbedwin, tabwinid);
    if (data == NULL) {
        return NULL;
    }

    seeker_widget *tabbedwin = *(seeker_widget**)data;
    switch (v) {
        case PCRDR_K_RESNAME_PAGE_active:
        case PCRDR_K_RESNAME_PAGE_first:
            return &tabbedwin->first->page;

        case PCRDR_K_RESNAME_PAGE_last:
            return &tabbedwin->last->page;
    }

    return NULL;
}

static pcmcth_page *
validate_page(pcmcth_session *sess, pcmcth_page *page, int *retv)
{
    void *data;
    if (sorted_array_find(sess->all_handles, PTR2U64(page), &data) < 0) {
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

static pcmcth_udom *
load_edom(pcmcth_session *sess, pcmcth_page *page, purc_variant_t edom,
        uint64_t crtn, char *buff, int *retv)
{
    page = validate_page(sess, page, retv);
    if (page == NULL)
        return NULL;

    pcmcth_udom *udom = seeker_wsp_load_edom_in_page(page, edom, retv);

    if (udom) {
        sorted_array_add(sess->all_handles, PTR2U64(udom), INT2PTR(HT_UDOM));
        *retv = PCRDR_SC_OK;
        seeker_page_set_udom(page, udom);

        struct purc_page_owner owner = { sess, crtn }, suppressed;
        suppressed = purc_page_ostack_register(page->ostack, owner);
        if (suppressed.corh) {
            if (suppressed.sess == sess) {
                sprintf(buff,
                        "%llx", (unsigned long long int)suppressed.corh);
            }
            else {
                /* TODO: send suppressPage request to another endpoint */
            }
        }
        else {
            buff[0] = 0;
        }
    }
    else
        *retv = PCRDR_SC_INTERNAL_SERVER_ERROR;

    return udom;
}

static uint64_t
register_crtn(pcmcth_session *sess, pcmcth_page *page,
        uint64_t crtn, int *retv)
{
    page = validate_page(sess, page, retv);
    if (page == NULL) {
        return 0;
    }

    struct purc_page_owner owner = { sess, crtn }, suppressed;
    suppressed = purc_page_ostack_register(page->ostack, owner);
    if (suppressed.corh && suppressed.sess != sess) {
        suppressed.corh = 0;
        /* TODO: send suppressPage request to another endpoint */
    }

    *retv = PCRDR_SC_OK;
    return suppressed.corh;
}

static uint64_t
revoke_crtn(pcmcth_session *sess, pcmcth_page *page,
        uint64_t crtn, int *retv)
{
    page = validate_page(sess, page, retv);
    if (page == NULL) {
        return 0;
    }

    struct purc_page_owner owner = { sess, crtn }, to_reload;
    to_reload = purc_page_ostack_revoke(page->ostack, owner);
    if (to_reload.corh && to_reload.sess != sess) {
        to_reload.corh = 0;
        /* TODO: send reloadPage request to another endpoint */
    }

    *retv = PCRDR_SC_OK;
    return to_reload.corh;
}

static pcmcth_udom *
validate_udom(pcmcth_session *sess, pcmcth_udom *udom, int *retv)
{
    void *data;
    if (sorted_array_find(sess->all_handles, PTR2U64(udom), &data) < 0) {
        *retv = PCRDR_SC_NOT_FOUND;
        return NULL;
    }

    if ((uintptr_t)data == HT_UDOM) {
        return udom;
    }

    *retv = PCRDR_SC_BAD_REQUEST;
    return NULL;
}

static int
update_udom(pcmcth_session *sess, pcmcth_udom *udom,
        int op, uint64_t element_handle, const char* property,
        purc_variant_t ref_info)
{
    (void)op;
    (void)element_handle;
    (void)property;
    (void)ref_info;

    int retv;
    udom = validate_udom(sess, udom, &retv);
    if (udom == NULL) {
        LOG_ERROR("Bad uDOM: %p.\n", udom);
        goto failed;
    }

    retv = PCRDR_SC_OK;

failed:
    return retv;
}

static purc_variant_t
call_method_in_session(pcmcth_session *sess,
            pcrdr_msg_target target, uint64_t target_value,
            pcrdr_msg_element_type element_type, const char *element_value,
            const char *property, const char *method, purc_variant_t arg,
            int* retv)
{
    (void)property;
    (void)method;
    (void)arg;

    LOG_DEBUG("element: %s; property: %s; method: %s\n",
            element_value, property, method);
    if (target != PCRDR_MSG_TARGET_WORKSPACE ||
            (void *)(uintptr_t)target_value != 0) {
        *retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    pcmcth_workspace *workspace = (pcmcth_workspace *)(uintptr_t)target_value;
    workspace = validate_workspace(sess->rdr, workspace);

    /* use element to specify the widget. */
    if (element_type != PCRDR_MSG_ELEMENT_TYPE_ID || element_value == NULL) {
        *retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    seeker_widget *widget;
    widget = seeker_wsp_find_widget(workspace, sess, element_value);
    if (widget == NULL) {
        *retv = PCRDR_SC_NOT_FOUND;
        goto failed;
    }

    *retv = PCRDR_SC_OK;
    return purc_variant_make_null();

failed:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
call_method_in_udom(pcmcth_session *sess,
        pcmcth_udom *udom, uint64_t element_handle,
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

    (void)element_handle;
    (void)method;
    (void)arg;

    *retv = PCRDR_SC_OK;
    return purc_variant_make_null();
}

static purc_variant_t
get_property_in_udom(pcmcth_session *sess,
        pcmcth_udom *udom, uint64_t element_handle,
        const char *property, int *retv)
{
    udom = validate_udom(sess, udom, retv);
    if (udom == NULL) {
        LOG_ERROR("Bad uDOM: %p.\n", udom);
        return PURC_VARIANT_INVALID;
    }

    if (!purc_is_valid_token(property, PURC_LEN_PROPERTY_NAME)) {
        *retv = PCRDR_SC_BAD_REQUEST;
        return PURC_VARIANT_INVALID;
    }

    (void)element_handle;
    (void)property;

    *retv = PCRDR_SC_OK;
    return purc_variant_make_null();
}

static purc_variant_t
set_property_in_udom(pcmcth_session *sess,
        pcmcth_udom *udom, uint64_t element_handle,
        const char *property, purc_variant_t value, int *retv)
{
    udom = validate_udom(sess, udom, retv);
    if (udom == NULL) {
        LOG_ERROR("Bad uDOM: %p.\n", udom);
        return PURC_VARIANT_INVALID;
    }

    (void)element_handle;
    (void)property;
    (void)value;

    *retv = PCRDR_SC_OK;
    return purc_variant_make_null();
}

void seeker_set_renderer_callbacks(pcmcth_renderer *rdr)
{
    memset(&rdr->cbs, 0, sizeof(rdr->cbs));

    rdr->cbs.prepare = prepare;
    rdr->cbs.handle_event = handle_event;
    rdr->cbs.cleanup = cleanup;
    rdr->cbs.create_session = create_session;
    rdr->cbs.remove_session = remove_session;

    rdr->cbs.create_workspace = create_workspace;
    rdr->cbs.update_workspace = update_workspace;
    rdr->cbs.destroy_workspace = destroy_workspace;
    rdr->cbs.find_workspace = find_workspace;
    rdr->cbs.get_special_workspace = get_special_workspace;

    rdr->cbs.set_page_groups = set_page_groups;
    rdr->cbs.add_page_groups = add_page_groups;
    rdr->cbs.remove_page_group = remove_page_group;

    rdr->cbs.find_page = find_page;
    rdr->cbs.get_special_plainwin = get_special_plainwin;
    rdr->cbs.create_plainwin = create_plainwin;
    rdr->cbs.update_plainwin = update_plainwin;
    rdr->cbs.destroy_plainwin = destroy_plainwin;

    rdr->cbs.create_widget = create_widget;
    rdr->cbs.update_widget = update_widget;
    rdr->cbs.destroy_widget = destroy_widget;
    rdr->cbs.get_special_widget = get_special_widget;

    rdr->cbs.load_edom = load_edom;
    rdr->cbs.register_crtn = register_crtn;
    rdr->cbs.revoke_crtn = revoke_crtn;
    rdr->cbs.update_udom = update_udom;
    rdr->cbs.call_method_in_udom = call_method_in_udom;
    rdr->cbs.call_method_in_session = call_method_in_session;
    rdr->cbs.get_property_in_udom = get_property_in_udom;
    rdr->cbs.set_property_in_udom = set_property_in_udom;

    return;
}

