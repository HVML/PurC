/*
** @file udom.c
** @author Vincent Wei
** @date 2022/10/06
** @brief The implementation of uDOM (the rendering tree).
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

#include "udom.h"
#include "page.h"
#include "widget.h"
#include "util/sorted-array.h"
#include "util/list.h"

#include <assert.h>

int seeker_udom_module_init(pcmcth_renderer *rdr)
{
    (void)rdr;
    return 0;
}

void seeker_udom_module_cleanup(pcmcth_renderer *rdr)
{
    (void)rdr;
}

pcmcth_udom *seeker_udom_new(pcmcth_page *page)
{
    pcmcth_udom* udom = calloc(1, sizeof(pcmcth_udom));
    if (udom) {
        udom->page = page;
    }

    return udom;
}

void seeker_udom_delete(pcmcth_udom *udom)
{
    free(udom);
}

pcmcth_udom *
seeker_udom_load_edom(pcmcth_page *page, purc_variant_t edom, int *retv)
{
    purc_document_t edom_doc;
    pcmcth_udom *udom = NULL;

    edom_doc = purc_variant_native_get_entity(edom);
    assert(edom_doc);

    udom = seeker_udom_new(page);
    if (udom == NULL) {
        *retv = PCRDR_SC_INSUFFICIENT_STORAGE;
    }
    else {
        udom->doc = edom_doc;
    }

    return udom;
}

