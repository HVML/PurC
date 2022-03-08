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

#define PCRDR_TIME_DEF_EXPECTED         5

/* the capabilities of a renderer */
struct renderer_capabilities {
    /* the protocol name */
    char   *prot_name;

    /* the HTML version if supported, else NULL */
    char   *html_version;
    /* the XGML version if supported, else NULL */
    char   *xgml_version;
    /* the XML version if supported, else NULL */
    char   *xml_version;

    /* the protocol version number */
    long int    prot_version;

    /* the max number of workspaces;
       0 for not supported, -1 for unlimited */
    long int    workspace;
    /* the max number of tabbed windows;
       0 for not supported, -1 for unlimited */
    long int    tabbedWindow;
    /* the max number of tabbed pages in one tabbed window;
       0 for not supported, -1 for unlimited */
    long int    tabbedPage;
    /* the max number of plain windows;
       0 for not supported, -1 for unlimited */
    long int    plainWindow;
    /* the max number of window levels; 0 or <0 for not supported */
    long int    windowLevel;

    /* the names of window levels */
    char  **window_levels;

    /* the session handle */
    uintptr_t   session_handle;
};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void pcrdr_init_once(void) WTF_INTERNAL;

int pcrdr_init_instance(struct pcinst* inst,
        const purc_instance_extra_info *extra_info) WTF_INTERNAL;

void pcrdr_cleanup_instance(struct pcinst* inst) WTF_INTERNAL;

struct renderer_capabilities *
pcrdr_parse_renderer_capabilities(const char *data) WTF_INTERNAL;

void pcrdr_release_renderer_capabilities(
        struct renderer_capabilities *rdr_caps) WTF_INTERNAL;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_PCRDR_H */

