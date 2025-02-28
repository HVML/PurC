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
        if (term_enc == NULL || strcasecmp(term_enc, "UTF-8")) {
            LOG_ERROR("The terminal encoding must be UTF-8, but it is %s\n",
                    term_enc ? term_enc : "NULL");
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

    foil_wsp_module_cleanup(rdr);
    free(rdr->impl);
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
    return sess;

failed:

    if (sess->all_handles)
        sorted_array_destroy(sess->all_handles);

    free(sess);
    return NULL;
}

static int on_each_ostack(void *ctxt, const char *name, void *data)
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
    LOG_DEBUG("removing page %p\n", page);
    if (sorted_array_find(sess->all_handles, PTR2U64(page), NULL) >= 0) {
        pcmcth_udom *udom = foil_page_delete(page);
        if (udom) {
            foil_udom_delete(udom);
        }

        sorted_array_remove(sess->all_handles, PTR2U64(page));
        LOG_DEBUG("page %p removed\n", page);
    }

    purc_page_ostack_delete(sess->workspace->page_owners, ostack);
    return 0;
}

static int foil_remove_session(pcmcth_session *sess)
{
    LOG_DEBUG("removing session (%p)...\n", sess);

    LOG_DEBUG("destroy all windows/widgets created by this session...\n");
    pcutils_kvlist_for_each_safe(sess->workspace->page_owners, sess,
            on_each_ostack);

    LOG_DEBUG("destroy sorted array for all handles...\n");
    sorted_array_destroy(sess->all_handles);

    LOG_DEBUG("free session...\n");
    free(sess);

    LOG_DEBUG("done\n");
    return PCRDR_SC_OK;
}

#define STR_STYLE_SEPARATOR ";"
#define STR_PAIR_SEPARATOR  ":"

/* use `rows` and `columns` for the size of the off-screen plain window,
   for example, "rows:25;columns:80". */
static void parse_layout_style_for_off_screen(const char *layout_style,
        foil_rect *rc)
{
    char *styles = strdup(layout_style);
    char *str1;
    char *style;
    char *saveptr1;

    LOG_DEBUG("layout style: %s\n", layout_style);
    for (str1 = styles; ; str1 = NULL) {
        style = strtok_r(str1, STR_STYLE_SEPARATOR, &saveptr1);
        if (style == NULL) {
            goto done;
        }

        char *key, *value;
        char *saveptr2;
        key = strtok_r(style, STR_PAIR_SEPARATOR, &saveptr2);
        if (key == NULL) {
            goto done;
        }

        value = strtok_r(NULL, STR_PAIR_SEPARATOR, &saveptr2);
        if (value == NULL) {
            goto done;
        }

        int v = (int)strtol(value, NULL, 10);
        if (strcmp(key, "rows") == 0) {
            LOG_DEBUG("height of the off-screen window was overwritten: %d\n",
                    v);
            rc->bottom = rc->top + v;
        }
        else if (strcmp(key, "columns") == 0) {
            LOG_DEBUG("width of the off-screen window was overwritten: %d\n",
                    v);
            rc->right = rc->left + v;
        }
        else {
            goto done;
        }
    }

done:
    free(styles);
    return;
}

/* TODO: always returns the page for 'plainwin:main' currently */
static pcmcth_page *foil_get_special_plainwin(pcmcth_session *sess,
        pcmcth_workspace *workspace, const char *group,
        pcrdr_resname_page_k page_type)
{
    (void)group;
    (void)page_type;

    if (workspace == NULL)
        workspace = sess->workspace;

    void *data;
    data = pcutils_kvlist_get(workspace->page_owners, "plainwin:main");
    if (data != NULL) {
        struct purc_page_ostack *ostack = *(struct purc_page_ostack **)data;
        return purc_page_ostack_get_page(ostack);
    }

    return NULL;
}

static pcmcth_page *foil_find_page(pcmcth_session *sess,
        pcmcth_workspace *workspace, const char *page_id)
{
    if (workspace == NULL)
        workspace = sess->workspace;

    void *data;
    data = pcutils_kvlist_get(workspace->page_owners, page_id);
    if (data != NULL) {
        struct purc_page_ostack *ostack = *(struct purc_page_ostack **)data;
        return purc_page_ostack_get_page(ostack);
    }

    return NULL;
}

static pcmcth_page *foil_create_plainwin(pcmcth_session *sess,
        pcmcth_workspace *workspace,
        const char *page_id, const char *group, const char *name,
        const char *class_name, const char *title, const char *layout_style,
        purc_variant_t toolkit_style, int *retv)
{
    pcmcth_page *plain_win = NULL;

    workspace = sess->workspace;
    if (pcutils_kvlist_get(workspace->page_owners, page_id)) {
        LOG_WARN("Duplicated page identifier: %s\n", page_id);
        *retv = PCRDR_SC_CONFLICT;
        goto done;
    }

    if (group == NULL) {
        /* TODO: use workspace to maintain the names of plain windows */
        /* create a ungrouped plain window */

        if (class_name &&
                strcmp(class_name, WSP_WIDGET_CLASS_OFF_SCREEN) == 0) {
            LOG_DEBUG("creating an off-screen window with name (%s)\n", name);
            foil_rect rc;
            foil_rect_set(&rc, 0, 0,
                    workspace->rdr->impl->cols, workspace->rdr->impl->rows);
            parse_layout_style_for_off_screen(layout_style, &rc);
            foil_widget *plainwin = foil_widget_new(
                    WSP_WIDGET_TYPE_OFFSCREEN, WSP_WIDGET_BORDER_NONE,
                    name, title, &rc);
            plainwin->user_data = workspace;    /* an orphan widget */
            plain_win = &plainwin->page;
        }
        else {
            LOG_DEBUG("creating an ungrouped plain window with name (%s)\n",
                    name);

            struct foil_widget_info style = { };
            style.flags = WSP_WIDGET_FLAG_NAME | WSP_WIDGET_FLAG_TITLE;
            style.name = name;
            style.title = title;
            foil_wsp_convert_style(workspace, sess, &style, toolkit_style);
            plain_win = foil_wsp_create_widget(workspace, sess,
                    WSP_WIDGET_TYPE_PLAINWINDOW, NULL, NULL, NULL, &style);
        }

        plain_win->ostack =
            purc_page_ostack_new(workspace->page_owners, page_id, plain_win);
    }
    else if (workspace->layouter == NULL) {
        *retv = PCRDR_SC_PRECONDITION_FAILED;
        goto done;
    }
    else {
        LOG_DEBUG("creating a grouped plain window with name (%s@%s)\n",
                name, group);

        /* TODO: create a plain window in the specified group
        plain_win = wsp_layouter_add_plain_window(workspace->layouter, sess,
                group, name, class_name, title, layout_style, toolkit_style,
                web_view, retv);
        */
        *retv = PCRDR_SC_NOT_IMPLEMENTED;
        goto done;
    }

    if (plain_win) {
        sorted_array_add(sess->all_handles, PTR2U64(plain_win),
                INT2PTR(HT_PLAINWIN));
        *retv = PCRDR_SC_OK;
    }
    else {
        LOG_ERROR("Failed to create a plain window: %s@%s\n", name, group);
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
    void *data;
    if (sorted_array_find(sess->all_handles, PTR2U64(plain_win), &data) < 0) {
        sorted_array_remove(sess->all_handles, PTR2U64(plain_win));

        workspace = sess->workspace;
        return foil_wsp_destroy_widget(workspace, sess, plain_win, plain_win,
            WSP_WIDGET_TYPE_PLAINWINDOW);
    }

    return PCRDR_SC_BAD_REQUEST;
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
        uint64_t crtn, char *buff, size_t sz_buff, int *retv)
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

        struct purc_page_owner owner = { sess, crtn }, suppressed;
        suppressed = purc_page_ostack_register(page->ostack, owner);
        if (suppressed.corh) {
            if (suppressed.sess == sess) {
                snprintf(buff, sz_buff,
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
foil_register_crtn(pcmcth_session *sess, pcmcth_page *page,
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
foil_revoke_crtn(pcmcth_session *sess, pcmcth_page *page,
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
foil_call_method_in_session(pcmcth_session *sess,
            pcrdr_msg_target target, uint64_t target_value,
            pcrdr_msg_element_type element_type, const char *element_value,
            const char *property, const char *method, purc_variant_t arg,
            int* retv)
{
    (void)target;
    (void)target_value;
    (void)property;

    purc_variant_t result = PURC_VARIANT_INVALID;

    LOG_DEBUG("element: %s; property: %s; method: %s\n",
            element_value, property, method);
    if (target != PCRDR_MSG_TARGET_WORKSPACE ||
            (void *)(uintptr_t)target_value != 0) {
        *retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    /* use element to specify the widget. */
    if (element_type != PCRDR_MSG_ELEMENT_TYPE_ID || element_value == NULL) {
        *retv = PCRDR_SC_BAD_REQUEST;
        goto failed;
    }

    foil_widget *widget;
    widget = foil_wsp_find_widget(sess->workspace, sess, element_value);
    if (widget == NULL) {
        *retv = PCRDR_SC_NOT_FOUND;
        goto failed;
    }

    result = foil_widget_call_method(widget, method, arg);
    if (result) {
        *retv = PCRDR_SC_OK;
    }
    else {
        *retv = PCRDR_SC_INTERNAL_SERVER_ERROR;
    }

failed:
    return result;
}

static purc_variant_t
foil_call_method_in_udom(pcmcth_session *sess, pcmcth_udom *udom,
        pcrdr_msg_element_type element_type, const char *element_value,
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

    uint64_t element_handle = 0;
    if (element_type != PCRDR_MSG_ELEMENT_TYPE_HANDLE ||
            element_value == NULL) {
        *retv = PCRDR_SC_BAD_REQUEST;
        return PURC_VARIANT_INVALID;
    }

    element_handle = strtoull(element_value, NULL, 16);

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

void foil_set_renderer_callbacks(pcmcth_renderer *rdr)
{
    memset(&rdr->cbs, 0, sizeof(rdr->cbs));

    rdr->cbs.prepare = foil_prepare;
    rdr->cbs.handle_event = foil_handle_event;
    rdr->cbs.cleanup = foil_cleanup;
    rdr->cbs.create_session = foil_create_session;
    rdr->cbs.remove_session = foil_remove_session;

    rdr->cbs.find_page = foil_find_page;
    rdr->cbs.get_special_plainwin = foil_get_special_plainwin;
    rdr->cbs.create_plainwin = foil_create_plainwin;
    rdr->cbs.update_plainwin = foil_update_plainwin;
    rdr->cbs.destroy_plainwin = foil_destroy_plainwin;

    rdr->cbs.load_edom = foil_load_edom;
    rdr->cbs.register_crtn = foil_register_crtn;
    rdr->cbs.revoke_crtn = foil_revoke_crtn;
    rdr->cbs.update_udom = foil_update_udom;
    rdr->cbs.call_method_in_udom = foil_call_method_in_udom;
    rdr->cbs.call_method_in_session = foil_call_method_in_session;
    rdr->cbs.get_property_in_udom = foil_get_property_in_udom;
    rdr->cbs.set_property_in_udom = foil_set_property_in_udom;

    return;
}

