/*
 * @file workspace.h
 * @author Vincent Wei
 * @date 2022/10/02
 * @brief The global definitions for terminal workspace.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of purc, which is an HVML interpreter with
 * a command line interface (CLI).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef purc_foil_workspace_h
#define purc_foil_workspace_h

typedef enum {
    WSP_WIDGET_TYPE_NONE  = 0,       /* not-existing */
    WSP_WIDGET_TYPE_PLAINWINDOW,     /* a plain main window */
    WSP_WIDGET_TYPE_TABBEDWINDOW,    /* a tabbed main window */
    WSP_WIDGET_TYPE_CONTAINER,       /* A layout container widget */
    WSP_WIDGET_TYPE_PANEHOST,        /* the container of pan widgets */
    WSP_WIDGET_TYPE_TABHOST,         /* the container of tab pages */
    WSP_WIDGET_TYPE_PANEDPAGE,       /* a plain page for a webview */
    WSP_WIDGET_TYPE_TABBEDPAGE,      /* a tabbed page for a webview */
} wsp_widget_type_t;

#define WSP_WIGET_FLAG_NAME      0x00000001
#define WSP_WIGET_FLAG_TITLE     0x00000002
#define WSP_WIGET_FLAG_GEOMETRY  0x00000004
#define WSP_WIGET_FLAG_TOOLKIT   0x00000008

struct wsp_widget_info {
    unsigned int flags;

    const char *name;
    const char *title;
    const char *klass;

    /* other styles */
    const char *backgroundColor;
    bool        darkMode;
    bool        fullScreen;
    bool        withToolbar;

    int         x, y;
    unsigned    w, h;

    int         ml, mt, mr, mb; /* margins */
    int         pl, pt, pr, pb; /* paddings */
    float       bl, bt, br, bb; /* borders */
    float       brlt, brtr, brrb, brbl; /* border radius */

    float       opacity;
};

struct purcth_workspace {
    /* TODO: manager of grouped plain windows and pages */
    void *layouter;
};

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize workspace module */
int foil_wsp_init(purcth_renderer *rdr);
/* Clean up the workspace module */
void foil_wsp_cleanup(purcth_renderer *rdr);

/* Create or get a workspace for an endpoint */
purcth_workspace *foil_wsp_create_or_get_workspace(purcth_endpoint* endpoint);

void foil_wsp_convert_style(struct wsp_widget_info *style,
        purc_variant_t toolkit_style);

void *foil_wsp_create_widget(void *workspace, void *session,
        wsp_widget_type_t type, void *window,
        void *parent, void *init_arg, const struct wsp_widget_info *style);

int  foil_wsp_destroy_widget(void *workspace, void *session,
        void *window, void *widget, wsp_widget_type_t type);

void foil_wsp_update_widget(void *workspace, void *session, void *widget,
        wsp_widget_type_t type, const struct wsp_widget_info *style);

#ifdef __cplusplus
}
#endif

#endif  /* purc_foil_workspace_h */

