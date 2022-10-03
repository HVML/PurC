/*
** @file callbacks.h
** @author Vincent Wei
** @date 2022/10/03
** @brief The interface of PURCTH callbacks.
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

#ifndef purc_foil_callbacks_h_
#define purc_foil_callbacks_h_

#include "foil.h"

#ifdef __cplusplus
extern "C" {
#endif

void set_renderer_callbacks(purcth_renderer *rdr);

#ifdef __cplusplus
}
#endif

#endif /* !purc_foil_callbacks_h_ */

