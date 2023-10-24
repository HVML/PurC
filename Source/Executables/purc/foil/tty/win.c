/*
   Terminal management xterm and rxvt support

   Copyright (C) 1995-2021
   Free Software Foundation, Inc.

   Written by:
   Andrew Borodin <aborodin@vmail.ru>, 2009.

   This file is part of purc, an HVML interperter with CLI.

   Purc is free software: you can redistribute it
   and/or modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   Purc is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** \file win.c
 *  \brief Source: Terminal management xterm and rxvt support
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "../screen.h"
#include "util.h"           /* is_printable() */
#include "tty-internal.h"
#include "tty.h"                /* tty_gotoyx, tty_print_char */
#include "win.h"

/*** global variables ****************************************************************************/

char *smcup = NULL;
char *rmcup = NULL;

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

static gboolean rxvt_extensions = FALSE;

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/* my own weird protocol base 16 - paul */
static int
rxvt_getc (void)
{
    int r;
    unsigned char c;

    while (read (0, &c, 1) != 1);
    if (c == '\n')
        return -1;
    r = (c - 'A') * 16;
    while (read (0, &c, 1) != 1);
    r += (c - 'A');
    return r;
}

/* --------------------------------------------------------------------------------------------- */

static int
anything_ready (void)
{
    fd_set fds;
    struct timeval tv;

    FD_ZERO (&fds);
    FD_SET (0, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    return select (1, &fds, 0, 0, &tv);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
show_rxvt_contents (int starty, unsigned char y1, unsigned char y2)
{
    unsigned char *k;
    int bytes, i, j, cols = 0;

    y1 += mc_global.keybar_visible != 0 ? 1 : 0;        /* i don't knwo why we need this - paul */
    y2 += mc_global.keybar_visible != 0 ? 1 : 0;
    while (anything_ready ())
        tty_lowlevel_getch ();

    /* my own weird protocol base 26 - paul */
    printf (ESC_STR "CL%c%c%c%c\n", (y1 / 26) + 'A', (y1 % 26) + 'A', (y2 / 26) + 'A',
            (y2 % 26) + 'A');

    bytes = (y2 - y1) * (COLS + 1) + 1; /* *should* be the number of bytes read */
    j = 0;
    k = g_malloc (bytes);
    while (TRUE)
    {
        int c;

        c = rxvt_getc ();
        if (c < 0)
            break;
        if (j < bytes)
            k[j++] = c;
        for (cols = 1;; cols++)
        {
            c = rxvt_getc ();
            if (c < 0)
                break;
            if (j < bytes)
                k[j++] = c;
        }
    }
    for (i = 0; i < j; i++)
    {
        if ((i % cols) == 0)
            tty_gotoyx (starty + (i / cols), 0);
        tty_print_char (is_printable (k[i]) ? k[i] : ' ');
    }
    g_free (k);
}

/* --------------------------------------------------------------------------------------------- */

gboolean
look_for_rxvt_extensions (void)
{
    static gboolean been_called = FALSE;

    if (!been_called)
    {
        const char *e = getenv ("RXVT_EXT");
        rxvt_extensions = ((e != NULL) && (strcmp (e, "1.0") == 0));
        been_called = TRUE;
    }

    if (rxvt_extensions)
        mc_global.tty.console_flag = '\004';

    return rxvt_extensions;
}

/* --------------------------------------------------------------------------------------------- */
