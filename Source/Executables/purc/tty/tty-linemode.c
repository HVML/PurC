/*
** @file tty-linemode.c
** @author Vincent Wei
** @date 2022/10/20
** @brief Interface to the terminal controlling under line-mode.
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

#include "config.h"

#include "tty-linemode.h"
#include "strutil/strutil.h"

#include <stdio.h>
#include <unistd.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <termios.h>

#include "screen.h"
#include "tty-internal.h"

static void
tty_setup_sigwinch (void (*handler) (int))
{
#ifdef SIGWINCH
    struct sigaction act, oact;

    memset (&act, 0, sizeof (act));
    act.sa_handler = handler;
    sigemptyset (&act.sa_mask);
#ifdef SA_RESTART
    act.sa_flags = SA_RESTART;
#endif /* SA_RESTART */
    sigaction (SIGWINCH, &act, &oact);
#endif /* SIGWINCH */

    tty_create_winch_pipe();
}

/* --------------------------------------------------------------------------------------------- */

static void
sigwinch_handler(int dummy)
{
    ssize_t n = 0;

    (void) dummy;

    n = write (sigwinch_pipe[1], "", 1);
    (void) n;
}

const char *
tty_linemode_init(int *rows, int *cols)
{
    int rc = -1;
    const char *termencoding = NULL;

#if defined(TIOCGWINSZ)
    struct winsize winsz;

    winsz.ws_col = winsz.ws_row = 0;

    /* Ioctl on the STDIN_FILENO */
    ioctl(fileno(stdout), TIOCGWINSZ, &winsz);
    if (winsz.ws_col != 0 && winsz.ws_row != 0) {
        *rows = winsz.ws_row;
        *cols = winsz.ws_col;
        rc = 0;
    }
#else
#endif /* defined(TIOCGWINSZ) */

    if (rc == 0) {
        termencoding = str_detect_termencoding();
        tty_setup_sigwinch(sigwinch_handler);
    }

    return termencoding;
}

/* --------------------------------------------------------------------------------------------- */

void
tty_linemode_shutdown(void)
{
    tty_destroy_winch_pipe();
}

