/*
** @file session.h
** @author Vincent Wei
** @date 2022/10/02
** @brief The interface of Endpoint (copied from PurC Midnight Commander).
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

#ifndef purc_foil_session_h_
#define purc_foil_session_h_

#include <time.h>
#include <stdbool.h>
#include <string.h>

#include "foil.h"

Session *start_session(Renderer* rdr, Endpoint* session, int *retv);
int remove_session(Renderer* rdr, Endpoint* session);

#endif /* !purc_foil_session_h_ */

