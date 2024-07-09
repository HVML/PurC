/**
 * @file pcrdr.h
 * @date 2022/02/21
 * @brief The internal interfaces for PCRDR module.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * Authors:
 *  Vincent Wei (<https://github.com/VincentWei>), 2021, 2022
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PURC_PRIVATE_PCRDR_H
#define PURC_PRIVATE_PCRDR_H

#include "instance.h"
#include "atom-buckets.h"

#define PCRDR_TIME_DEF_EXPECTED         5
#define PCRDR_TIME_AUTH_EXPECTED        10

enum {
#define PCRDR_SELECTOR_ID       "id"
    PCRDR_K_SELECTOR_ID_b        = 0x01,
#define PCRDR_SELECTOR_HANDLE   "handle"
    PCRDR_K_SELECTOR_HANDLE_b   = 0x02,
#define PCRDR_SELECTOR_HANDLES  "handles"
    PCRDR_K_SELECTOR_HANDLES_b  = 0x04,
#define PCRDR_SELECTOR_CSS      "css"
    PCRDR_K_SELECTOR_CSS_b      = 0x08,
#define PCRDR_SELECTOR_XPATH    "xpath"
    PCRDR_K_SELECTOR_XPATH_b    = 0x10,
};

/* the capabilities of a renderer */
struct renderer_capabilities {
#define NR_RDRCAP_STRINGS   9
    char   *_strings[0];

    /* the protocol name */
    char   *prot_name;

    /* the renderer name */
    char   *rdr_name;
    /* the renderer version */
    char   *rdr_version;

    /* the HTML version if supported, else NULL */
    char   *html_version;
    /* the XGML version if supported, else NULL */
    char   *xgml_version;
    /* the XML version if supported, else NULL */
    char   *xml_version;

    /* Since v160 for authentication */
    char   *challenge_code;
    char   *locale;
    char   *display_density;

    /* the protocol version number */
    long int    prot_version;

    /* the max number of workspaces;
       0 for not supported, -1 for unlimited */
    int    workspace;
    /* the max number of tabbed windows;
       0 for not supported, -1 for unlimited */
    int    tabbedWindow;
    /* the max number of widgets in one tabbed window;
       0 for not supported, -1 for unlimited */
    int    widgetInTabbedWindow;
    /* the max number of plain windows;
       0 for not supported, -1 for unlimited */
    int    plainWindow;

    /* the element selectors supported */
    unsigned    selectors;

    /* the session handle */
    uint64_t    session_handle;
    /* the default workspace handle */
    uint64_t    workspace_handle;
};

struct pcrdr_conn;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

struct renderer_capabilities *
pcrdr_parse_renderer_capabilities(const char *data) WTF_INTERNAL;

void pcrdr_release_renderer_capabilities(
        struct renderer_capabilities *rdr_caps) WTF_INTERNAL;

static inline purc_atom_t
pcrdr_check_operation(const char *op)
{
    return purc_atom_try_string_ex(ATOM_BUCKET_RDROP, op);
}

int pcrdr_switch_renderer(struct pcinst *inst, const char *comm,
        const char *uri);

/* rdrState:newDuplicate,  rdrState:lostDuplicate, $RUNNER.duplicateRenderers */
purc_variant_t pcrdr_data(pcrdr_conn *conn);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_PCRDR_H */

