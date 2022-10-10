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

#include <assert.h>

struct purcth_page {
    unsigned rows, cols;
    purcth_udom *udom;
};

int foil_page_module_init(void)
{
    return 0;
}

void foil_page_module_cleanup(void)
{
}

purcth_page *foil_page_new(unsigned rows, unsigned cols)
{
    purcth_page *page = calloc(1, sizeof(*page));

    if (page) {
        page->rows = rows;
        page->cols = cols;
        page->udom = NULL;
    }

    return page;
}

purcth_udom *foil_page_delete(purcth_page *page)
{
    purcth_udom *udom = page->udom;
    free(page);

    return udom;
}

purcth_udom *foil_page_set_udom(purcth_page *page, purcth_udom *udom)
{
    purcth_udom *old_udom = page->udom;
    page->udom = udom;

    return old_udom;
}

unsigned foil_page_rows(const purcth_page *page)
{
    return page->rows;
}

unsigned foil_page_cols(const purcth_page *page)
{
    return page->cols;
}

