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

// #undef NDEBUG

#include "purcmc-thread.h"
#include "endpoint.h"
#include "workspace.h"
#include "udom.h"
#include "util/sorted-array.h"
#include "tty/tty.h"
#include "tty/tty-linemode.h"

#include <assert.h>

/* handle types */
enum {
    HT_WORKSPACE = 0,
    HT_PLAINWIN,
    HT_TABBEDWIN,
    HT_CONTAINER,
    HT_WIDGET,
    HT_UDOM,
};

struct pcmcth_session {
    pcmcth_renderer *rdr;
    pcmcth_endpoint *edpt;

    /* ungrouped plain windows */
    struct kvlist ug_wins;

    /* the sorted array of all valid handles */
    struct sorted_array *all_handles;

    /* the only workspace for all sessions of current app */
    pcmcth_workspace *workspace;
};

static int foil_prepare(pcmcth_renderer *rdr)
{
    rdr->impl = calloc(1, sizeof(*rdr->impl));
    if (rdr->impl) {
        rdr->impl->term_mode = FOIL_TERM_MODE_LINE;

        const char *term_enc;
        term_enc = tty_linemode_init(&rdr->impl->rows, &rdr->impl->cols);
        if (strcasecmp(term_enc, "UTF-8")) {
            LOG_ERROR("The terminal encoding must be UTF-8, but it is %s\n",
                    term_enc);
            goto failed;
        }

        LOG_INFO("The terminal info: encoding (%s), size (%d x %d)\n",
                term_enc, rdr->impl->rows, rdr->impl->cols);

        return foil_wsp_module_init(rdr);
    }

failed:
    return -1;
}

static int
foil_handle_event(pcmcth_renderer *rdr, unsigned long long timeout_usec)
{
    if (rdr->impl->term_mode == FOIL_TERM_MODE_LINE) {
        if (tty_got_winch(timeout_usec)) {
            // TODO: handle change of terminal size
        }
    }

    return 0;
}

static void foil_cleanup(pcmcth_renderer *rdr)
{
    if (rdr->impl->term_mode == FOIL_TERM_MODE_LINE) {
        tty_linemode_shutdown();
    }

    free(rdr->impl);

    foil_wsp_module_cleanup(rdr);
}

static pcmcth_session *
foil_create_session(pcmcth_renderer *rdr, pcmcth_endpoint *edpt)
{
    pcmcth_session* sess = calloc(1, sizeof(pcmcth_session));

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

static int foil_remove_session(pcmcth_session *sess)
{
    const char *name;
    void *next, *data;

    LOG_DEBUG("removing session (%p)...\n", sess);

    LOG_DEBUG("destroy all ungrouped plain windows...\n");
    kvlist_for_each_safe(&sess->ug_wins, name, next, data) {
        pcmcth_page *page = *(pcmcth_page **)data;
        foil_udom_delete(page->udom);
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

static pcmcth_page *foil_create_plainwin(pcmcth_session *sess,
        pcmcth_workspace *workspace,
        const char *gid, const char *name,
        const char *class_name, const char *title, const char *layout_style,
        purc_variant_t toolkit_style, int *retv)
{
    (void)class_name;
    (void)layout_style;
    (void)toolkit_style;

    pcmcth_page *plain_win = NULL;

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
foil_update_plainwin(pcmcth_session *sess, pcmcth_workspace *workspace,
        pcmcth_page *plain_win, const char *property, purc_variant_t value)
{
    void *data;
    if (sorted_array_find(sess->all_handles, PTR2U64(plain_win), &data) < 0) {

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
foil_destroy_plainwin(pcmcth_session *sess, pcmcth_workspace *workspace,
        pcmcth_page *plain_win)
{
    workspace = sess->workspace;
    return foil_wsp_destroy_widget(workspace, sess, plain_win, plain_win,
        WSP_WIDGET_TYPE_PLAINWINDOW);
}

#if 0
static pcmcth_page *
foil_get_plainwin_page(pcmcth_session *sess,
        pcmcth_page *plain_win, int *retv)
{
    void *data;
    if (sorted_array_find(sess->all_handles, PTR2U64(plain_win), &data) < 0) {
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
    return (pcmcth_page *)browser_plain_window_get_view(
            BROWSER_PLAIN_WINDOW(plain_win)); */
}
#endif

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
foil_load_edom(pcmcth_session *sess, pcmcth_page *page, purc_variant_t edom,
        int *retv)
{
    page = validate_page(sess, page, retv);
    if (page == NULL)
        return NULL;

    pcmcth_udom *udom = foil_wsp_load_edom_in_page(sess->workspace, sess,
            page, edom, retv);

    if (udom) {
        sorted_array_add(sess->all_handles, PTR2U64(udom), INT2PTR(HT_UDOM));
        *retv = PCRDR_SC_OK;
        foil_page_set_udom(page, udom);
    }
    else
        *retv = PCRDR_SC_INTERNAL_SERVER_ERROR;

    return udom;
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

static int foil_update_udom(pcmcth_session *sess, pcmcth_udom *udom,
        int op, uint64_t element_handle, const char* property,
        purc_variant_t ref_info)
{
    int retv;

    udom = validate_udom(sess, udom, &retv);
    if (udom == NULL) {
        LOG_ERROR("Bad uDOM: %p.\n", udom);
        goto failed;
    }

    foil_rdrbox *rdrbox = foil_udom_find_rdrbox(udom, element_handle);
    if (rdrbox == NULL) {
        LOG_WARN("Not found rdrbox for element: %p\n", INT2PTR(element_handle));
        retv = PCRDR_SC_NOT_FOUND;
        goto failed;
    }

    retv = foil_udom_update_rdrbox(udom, rdrbox, op, property, ref_info);

failed:
    return retv;
}

static purc_variant_t
foil_call_method_in_udom(pcmcth_session *sess,
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
foil_get_property_in_udom(pcmcth_session *sess,
        pcmcth_udom *udom, uint64_t element_handle,
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
foil_set_property_in_udom(pcmcth_session *sess,
        pcmcth_udom *udom, uint64_t element_handle,
        const char *property, purc_variant_t value, int *retv)
{
    udom = validate_udom(sess, udom, retv);
    if (udom == NULL) {
        LOG_ERROR("Bad uDOM: %p.\n", udom);
        return PURC_VARIANT_INVALID;
    }

    foil_rdrbox *rdrbox = foil_udom_find_rdrbox(udom, element_handle);
    if (rdrbox == NULL) {
        LOG_DEBUG("Not rdrbox for the element handle: %p\n",
                INT2PTR(element_handle));
        *retv = PCRDR_SC_NOT_FOUND;
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

void pcmcth_set_renderer_callbacks(pcmcth_renderer *rdr)
{
    memset(&rdr->cbs, 0, sizeof(rdr->cbs));

    rdr->cbs.prepare = foil_prepare;
    rdr->cbs.handle_event = foil_handle_event;
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

