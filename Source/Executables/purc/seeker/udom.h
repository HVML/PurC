/*
 * @file udom.h
 * @author Vincent Wei
 * @date 2022/10/06
 * @brief The global definitions for the ultimate DOM.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

#ifndef purc_seeker_udom_h
#define purc_seeker_udom_h

#include "seeker.h"
#include "util/sorted-array.h"
#include "util/list.h"

#include <purc/purc-document.h>

struct pcmcth_udom {
    /* the page in which the uDOM located */
    pcmcth_page *page;

    /* purc_document */
    purc_document_t doc;
};

#ifdef __cplusplus
extern "C" {
#endif

int seeker_udom_module_init(pcmcth_renderer *rdr);
void seeker_udom_module_cleanup(pcmcth_renderer *rdr);

pcmcth_udom *seeker_udom_new(pcmcth_page *page);
void seeker_udom_delete(pcmcth_udom *udom);

pcmcth_udom *seeker_udom_load_edom(pcmcth_page *page,
        purc_variant_t edom, int *retv);

#ifdef __cplusplus
}
#endif

#endif  /* purc_seeker_udom_h */

