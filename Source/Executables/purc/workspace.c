/*
** @file workspace.c
** @author Vincent Wei
** @date 2022/10/05
** @brief The implementation of workspace of Foil.
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

#include <assert.h>

#include "callbacks.h"
#include "endpoint.h"
#include "workspace.h"

static KVLIST(kv_app_workspace, NULL);

int foil_wsp_init(purcth_renderer *rdr)
{
    (void)rdr;
    return 0;
}

void foil_wsp_cleanup(purcth_renderer *rdr)
{
    (void)rdr;
    const char *name;
    void *next, *data;

    kvlist_for_each_safe(&kv_app_workspace, name, next, data) {
        purcth_workspace *workspace = *(purcth_workspace **)data;
        if (workspace->layouter) {
            // TODO: ws_layouter_delete(workspace->layouter);
        }
    }
}

purcth_workspace *foil_wsp_create_or_get_workspace(purcth_endpoint* endpoint)
{
    char host[PURC_LEN_HOST_NAME + 1];
    char app[PURC_LEN_APP_NAME + 1];
    const char *edpt_uri = get_endpoint_uri(endpoint);

    purc_extract_host_name(edpt_uri, host);
    purc_extract_app_name(edpt_uri, app);

    char app_key[PURC_LEN_ENDPOINT_NAME + 1];
    sprintf(app_key, "%s-%s", host, app);

    void *data;
    purcth_workspace *workspace;
    if ((data = kvlist_get(&kv_app_workspace, app_key))) {
        workspace = *(purcth_workspace **)data;
        assert(workspace);
    }
    else {
        workspace = calloc(1, sizeof(purcth_workspace));
        if (workspace) {
            workspace->layouter = NULL;
            kvlist_set(&kv_app_workspace, app_key, &workspace);
        }
    }

    return workspace;
}

