/*
 * @file foil.h
 * @author Vincent Wei
 * @date 2022/09/30
 * @brief The global definitions for the renderer Foil.
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

#ifndef purc_foil_h
#define purc_foil_h

/* for purc_atom_t */
#include <purc/purc.h>

#define RDR_FOIL_APP_NAME       "cn.fmsoft.hvml.renderer"
#define RDR_FOIL_RUN_NAME       "foil"

purc_atom_t foil_init(const char *rdr_uri);

#endif  /* purc_foil_h */

