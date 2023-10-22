/*
** @file workspace.c
** @author Vincent Wei
** @date 2023/10/20
** @brief The implementation of workspace of Seeker.
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

#include "seeker.h"
#include "finder.h"
#include "purcmc-thread.h"
#include "endpoint.h"

#include <assert.h>

int seeker_look_for_renderer(const char *name, void *ctxt)
{
    pcmcth_renderer *rdr = ctxt;
    LOG_WARN("It is time to find a new renderer: %s for rdr: %p\n",
            name, rdr);
    return 0;
}

