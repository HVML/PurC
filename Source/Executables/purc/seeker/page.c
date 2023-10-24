/*
** @file page.c
** @author Vincent Wei
** @date 2023/10/20
** @brief The implementation of Seeker page.
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

#include "page.h"
#include "widget.h"
#include "udom.h"

#include <assert.h>

int seeker_page_module_init(pcmcth_renderer *rdr)
{
    (void)rdr;
    return 0;
}

void seeker_page_module_cleanup(pcmcth_renderer *rdr)
{
    (void)rdr;
}

bool seeker_page_content_init(pcmcth_page *page)
{
    (void)page;
    return true;
}

void seeker_page_content_cleanup(pcmcth_page *page)
{
    if (page->udom)
        seeker_udom_delete(page->udom);
    page->udom = NULL;
}

pcmcth_udom *seeker_page_set_udom(pcmcth_page *page, pcmcth_udom *udom)
{
    pcmcth_udom *old_udom = page->udom;
    seeker_udom_delete(page->udom);
    page->udom = udom;

    return old_udom;
}

pcmcth_workspace *seeker_page_get_workspace(pcmcth_page *page)
{
    pcmcth_workspace *workspace = NULL;

    seeker_widget *widget = seeker_widget_from_page(page);

    seeker_widget *root = seeker_widget_get_root(widget);
    if (root) {
        workspace = (pcmcth_workspace *)root->user_data;
    }
    else {
        /* For anonymous pages */
        workspace = (pcmcth_workspace *)widget->user_data;
    }

    return workspace;
}

