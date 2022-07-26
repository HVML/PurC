/*
** @file page.c
** @author Vincent Wei
** @date 2022/10/10
** @brief The implementation of page (the view for showing the rendered tree).
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

#include "page.h"
#include "udom.h"
#include "workspace.h"

#include <assert.h>

int foil_page_module_init(pcmcth_renderer *rdr)
{
    return foil_udom_module_init(rdr);
}

void foil_page_module_cleanup(pcmcth_renderer *rdr)
{
    foil_udom_module_cleanup(rdr);
}

pcmcth_page *foil_page_new(int rows, int cols)
{
    pcmcth_page *page = calloc(1, sizeof(*page));

    if (page) {
        page->rows = rows;
        page->cols = cols;
        page->udom = NULL;
    }

    return page;
}

pcmcth_udom *foil_page_delete(pcmcth_page *page)
{
    pcmcth_udom *udom = page->udom;
    free(page);

    return udom;
}

pcmcth_udom *foil_page_set_udom(pcmcth_page *page, pcmcth_udom *udom)
{
    pcmcth_udom *old_udom = page->udom;
    page->udom = udom;

    return old_udom;
}

int foil_page_rows(const pcmcth_page *page)
{
    return page->rows;
}

int foil_page_cols(const pcmcth_page *page)
{
    return page->cols;
}

