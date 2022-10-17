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

#include "foil.h"
#include "widget.h"

struct purcth_workspace {
    /* the root window in the workspace */
    struct foil_widget *root;

    /* TODO: manager of grouped plain windows and pages */
    void *layouter;
};

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize workspace module */
int foil_wsp_module_init(purcth_renderer *rdr);
/* Clean up the workspace module */
void foil_wsp_module_cleanup(purcth_renderer *rdr);

/* Create or get a workspace for an endpoint */
purcth_workspace *foil_wsp_create_or_get_workspace(purcth_renderer *rdr,
        purcth_endpoint* endpoint);

void foil_wsp_convert_style(void *workspace, void *session,
        struct foil_widget_info *style, purc_variant_t toolkit_style);

void *foil_wsp_create_widget(void *workspace, void *session,
        foil_widget_type_t type, void *window,
        void *parent, void *init_arg, const struct foil_widget_info *style);

int foil_wsp_destroy_widget(void *workspace, void *session,
        void *window, void *widget, foil_widget_type_t type);

void foil_wsp_update_widget(void *workspace, void *session,
        void *widget, foil_widget_type_t type,
        const struct foil_widget_info *style);

purcth_udom *foil_wsp_load_edom_in_page(void *workspace, void *session,
        purcth_page *page, purc_variant_t edom, int *retv);

#ifdef __cplusplus
}
#endif

#endif  /* purc_foil_workspace_h */

