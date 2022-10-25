/*
 * @file screen.c
 * @author Vincent Wei
 * @date 2022/10/02
 * @brief The built-in text-mode renderer.
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

#include "screen.h"

#include "tty/tty.h"
#include "strutil/strutil.h"

mc_global_t mc_global;

int foil_test(void)
{
    tty_check_term(FALSE);
    str_init_strings("utf8");

    tty_init(FALSE, FALSE);
    tty_gotoyx(1, 1);
    tty_printf("This is a test line");
    tty_refresh();
    tty_shutdown();
    return 0;
}

void mc_refresh(void)
{
    return;
}

int
vfs_timeouts(void)
{
    return 10;
}

void
vfs_timeout_handler(void)
{
    return;
}

