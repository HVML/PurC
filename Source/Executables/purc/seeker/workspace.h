/*
 * @file workspace.h
 * @author Vincent Wei
 * @date 2023/10/20
 * @brief The global definitions for Seeker workspace.
 *
 * Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
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

#ifndef purc_seeker_workspace_h
#define purc_seeker_workspace_h

#include "seeker.h"
#include "widget.h"

struct pcmcth_workspace {
    const pcmcth_renderer *rdr;

    /* the workspace name */
    const char *name;

    /* the workspace title */
    char *title;

    /* the root window in the workspace */
    struct seeker_widget *root;

    /* page identifier (<app_name>/plainwin:main@group) -> owners */
    pcutils_kvlist_t        page_owners;

    /* widget group name (<app_name>/group) -> tabbedwindow */
    pcutils_kvlist_t        group_tabbedwin;
};

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize workspace module and return the default workspace. */
pcmcth_workspace *seeker_wsp_module_init(pcmcth_renderer *rdr);

/* Clean up the workspace module */
void seeker_wsp_module_cleanup(pcmcth_renderer *rdr);

pcmcth_workspace *seeker_wsp_new(pcmcth_renderer *rdr,
        const char *name, const char *title);
void seeker_wsp_delete(pcmcth_renderer *rdr, pcmcth_workspace *workspace);

void *seeker_wsp_create_widget(void *workspace, void *session,
        seeker_widget_type_k type, void *window,
        void *parent, void *init_arg, const struct seeker_widget_info *style);

int seeker_wsp_destroy_widget(void *workspace, void *session,
        void *window, void *widget, seeker_widget_type_k type);

void seeker_wsp_update_widget(void *workspace, void *session,
        void *widget, seeker_widget_type_k type,
        const struct seeker_widget_info *style);

pcmcth_udom *seeker_wsp_load_edom_in_page(pcmcth_page *page,
        purc_variant_t edom, int *retv);

seeker_widget *seeker_wsp_find_widget(pcmcth_workspace *workspace,
        pcmcth_session *session, const char *page_id);

#ifdef __cplusplus
}
#endif

#endif  /* purc_seeker_workspace_h */

