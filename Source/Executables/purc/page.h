/*
 * @file page.h
 * @author Vincent Wei
 * @date 2022/10/10
 * @brief The header for page.
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

#ifndef purc_foil_page_h
#define purc_foil_page_h

#include "foil.h"

#ifdef __cplusplus
extern "C" {
#endif

int foil_page_module_init(void);
void foil_page_module_cleanup(void);

purcth_page *foil_page_new(unsigned rows, unsigned cols);

/* return the uDOM set for this page */
purcth_udom *foil_page_delete(purcth_page *page);

/* set uDOM and return the old one */
purcth_udom *foil_page_set_udom(purcth_page *page, purcth_udom *udom);

unsigned foil_page_rows(const purcth_page *page);
unsigned foil_page_cols(const purcth_page *page);

#ifdef __cplusplus
}
#endif

#endif  /* purc_foil_page_h */

