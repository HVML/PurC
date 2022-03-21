/**
 * @file dvobjs.h
 * @author Geng Yue, Vincent Wei
 * @date 2021/07/02
 * @brief The interface for dynamic variant objects.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
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

#ifndef PURC_PRIVATE_DVOBJS_H
#define PURC_PRIVATE_DVOBJS_H

#include "config.h"
#include "purc-dom.h"
#include "purc-html.h"
#include "purc-rwstream.h"
#include "purc-variant.h"
#include "purc-dvobjs.h"

#include <assert.h>
#include <time.h>

#define PURC_SYS_TZ_FILE    "/etc/localtime"
#if OS(DARWIN)
#define PURC_SYS_TZ_DIR     "/var/db/timezone/zoneinfo/"
#else
#define PURC_SYS_TZ_DIR     "/usr/share/zoneinfo/"
#endif

#define STRING_COMP_MODE_CASELESS   "caseless"
#define STRING_COMP_MODE_CASE       "case"
#define STRING_COMP_MODE_REG        "reg"
#define STRING_COMP_MODE_WILDCARD   "wildcard"
#define STRING_COMP_MODE_NUMBER     "number"
#define STRING_COMP_MODE_AUTO       "auto"
#define STRING_COMP_MODE_ASC        "asc"
#define STRING_COMP_MODE_DESC       "desc"

#define HVML_MAP_APPEND             "append"
#define HVML_MAP_DISPLACE           "displace"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

bool pcdvobjs_is_valid_timezone(const char *timezone) WTF_INTERNAL;

// initialize dvobjs module (once)
void pcdvobjs_init_once(void) WTF_INTERNAL;

struct pcinst;

// initialize the dvobjs module for a PurC instance.
void pcdvobjs_init_instance(struct pcinst* inst) WTF_INTERNAL;

// clean up the dvobjs module for a PurC instance.
void pcdvobjs_cleanup_instance(struct pcinst* inst) WTF_INTERNAL;

struct wildcard_list {
    char * wildcard;
    struct wildcard_list *next;
};

#if 0          /* { */
purc_variant_t
pcdvobjs_make_element_variant(struct pcdom_element *element);

struct pcdom_element*
pcdvobjs_get_element_from_variant(purc_variant_t val);
#endif         /* } */

purc_variant_t
pcdvobjs_make_elements(struct pcdom_element *element);

purc_variant_t
pcdvobjs_elements_by_css(pchtml_html_document_t *doc, const char *css);

struct pcdom_element*
pcdvobjs_get_element_from_elements(purc_variant_t elems, size_t idx);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_DVOBJS_H */

