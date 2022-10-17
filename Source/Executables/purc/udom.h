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

#ifndef purc_foil_udom_h
#define purc_foil_udom_h

#include "foil.h"
#include "rdrbox.h"

#ifdef __cplusplus
extern "C" {
#endif

int foil_udom_module_init(void);
void foil_udom_module_cleanup(void);

purcth_udom *foil_udom_new(purcth_page *page);
void foil_udom_delete(purcth_udom *udom);

foil_rdrbox *foil_udom_find_rdrbox(purcth_udom *udom,
        uint64_t element_handle);

purcth_udom *foil_udom_load_edom(purcth_page *page,
        purc_variant_t edom, int *retv);

int foil_udom_update_rdrbox(purcth_udom *udom, foil_rdrbox *rdrbox,
        int op, const char *property, purc_variant_t ref_info);

purc_variant_t foil_udom_call_method(purcth_udom *udom, foil_rdrbox *rdrbox,
        const char *method, purc_variant_t arg);

purc_variant_t foil_udom_get_property(purcth_udom *udom, foil_rdrbox *rdrbox,
        const char *property);

purc_variant_t foil_udom_set_property(purcth_udom *udom, foil_rdrbox *rdrbox,
        const char *property, purc_variant_t value);

#ifdef __cplusplus
}
#endif

#endif  /* purc_foil_udom_h */

