/*
 * @file finder.h
 * @author Vincent Wei
 * @date 2023/10/22
 * @brief The global definitions for Seeker renderer finder.
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

#ifndef purc_seeker_finder_h
#define purc_seeker_finder_h

#include "seeker.h"

#define SEEKER_FINDER_INTERVAL  1000

#ifdef __cplusplus
extern "C" {
#endif

int seeker_look_for_renderer(const char *name, void *ctxt);

#ifdef __cplusplus
}
#endif

#endif  /* purc_seeker_finder_h */

