/*
 * @file page.h
 * @author Vincent Wei
 * @date 2023/10/20
 * @brief The header for Seeker page.
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

#ifndef purc_seeker_page_h
#define purc_seeker_page_h

#include "seeker.h"

/* a page is the client area of a window or widget,
   which is used to render the content. */
struct pcmcth_page {
    /* Since PURCMC-120 */
    purc_page_ostack_t ostack;

    pcmcth_udom *udom;
};

#ifdef __cplusplus
extern "C" {
#endif

int seeker_page_module_init(pcmcth_renderer *rdr);
void seeker_page_module_cleanup(pcmcth_renderer *rdr);

bool seeker_page_content_init(pcmcth_page *page);
void seeker_page_content_cleanup(pcmcth_page *page);

/* Returns the workspace to which the page belongs */
pcmcth_workspace *seeker_page_get_workspace(pcmcth_page *page);

/* Sets uDOM and return the old one */
pcmcth_udom *seeker_page_set_udom(pcmcth_page *page, pcmcth_udom *udom);

#ifdef __cplusplus
}
#endif


#endif  /* purc_seeker_page_h */

