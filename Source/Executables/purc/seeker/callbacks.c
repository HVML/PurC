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
#include "util/sorted-array.h"

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

void seeker_set_renderer_callbacks(pcmcth_renderer *rdr)
{
    memset(&rdr->cbs, 0, sizeof(rdr->cbs));

#if 0
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
#endif

    return;
}

