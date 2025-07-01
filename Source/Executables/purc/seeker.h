/*
** @file seeker.h
** @author Vincent Wei
** @date 2023/10/20
** @brief The global definitions for the renderer Seeker.
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

#ifndef purc_seeker_h
#define purc_seeker_h

#include <time.h>

#include <purc/purc.h>

#include "purcmc-thread.h"

#define SEEKER_APP_NAME           "cn.fmsoft.hvml.renderer"
#define SEEKER_RUN_NAME           "seeker"
#define SEEKER_RDR_NAME           "Seeker"
#define SEEKER_RDR_URI            \
    PURC_EDPT_SCHEME "localhost/" SEEKER_APP_NAME "/" SEEKER_RUN_NAME

#define __STRING(x) #x

#define SEEKER_RDR_FEATURES \
    PCRDR_PURCMC_PROTOCOL_NAME ":" PCRDR_PURCMC_PROTOCOL_VERSION_STRING "\n" \
    SEEKER_RDR_NAME ":" PURC_VERSION_STRING "\n"                             \
    "HTML:5.3\n"                                                             \
    "workspace:-1/tabbedWindow:-1/widgetInTabbedWindow:-1/plainWindow:-1"    \
    "DOMElementSelectors:handle"

#ifdef __cplusplus
extern "C" {
#endif

/* Starts Seeker renderer */
purc_atom_t seeker_start(const char *rdr_uri);

/* Returns the pointer to the Seeker renderer. */
pcmcth_renderer *seeker_get_renderer(void);

void seeker_set_renderer_callbacks(pcmcth_renderer *rdr);

/* Wait for Seeker renderer to exit synchronously */
void seeker_sync_exit(void);

#ifdef __cplusplus
}
#endif

#endif  /* purc_seeker_h */

