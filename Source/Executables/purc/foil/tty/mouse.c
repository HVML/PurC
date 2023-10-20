/*
   Mouse managing

   Copyright (C) 1994-2021
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

/** \file mouse.c
 *  \brief Source: mouse managing
 *
 *  Events received by clients of this library have their coordinates 0 based
 */

#include <config.h>

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "../screen.h"

#include "tty.h"
#include "tty-internal.h"       /* mouse_enabled */
#include "mouse.h"
#include "key.h"                /* define sequence */

/*** global variables ****************************************************************************/

Mouse_Type use_mouse_p = MOUSE_NONE;
gboolean mouse_enabled = FALSE;
int mouse_fd = -1;              /* for when gpm_fd changes to < 0 and the old one must be cleared from select_set */
const char *xmouse_seq;
const char *xmouse_extended_seq;

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
show_mouse_pointer (int x, int y)
{
#ifdef HAVE_LIBGPM
    if (use_mouse_p == MOUSE_GPM)
        Gpm_DrawPointer (x, y, gpm_consolefd);
#else
    (void) x;
    (void) y;
#endif /* HAVE_LIBGPM */
}

/* --------------------------------------------------------------------------------------------- */

void
init_mouse (void)
{
    switch (use_mouse_p)
    {
#ifdef HAVE_LIBGPM
    case MOUSE_NONE:
        use_mouse_p = MOUSE_GPM;
        break;
#endif /* HAVE_LIBGPM */

    case MOUSE_XTERM_NORMAL_TRACKING:
    case MOUSE_XTERM_BUTTON_EVENT_TRACKING:
        if (xmouse_seq != NULL)
            define_sequence (MCKEY_MOUSE, xmouse_seq, MCKEY_NOACTION);
        if (xmouse_extended_seq != NULL)
            define_sequence (MCKEY_EXTENDED_MOUSE, xmouse_extended_seq, MCKEY_NOACTION);
        break;

    default:
        break;
    }

    enable_mouse ();
}

/* --------------------------------------------------------------------------------------------- */

void
enable_mouse (void)
{
    if (mouse_enabled)
        return;

    switch (use_mouse_p)
    {
#ifdef HAVE_LIBGPM
    case MOUSE_GPM:
        {
            Gpm_Connect conn;

            conn.eventMask = ~GPM_MOVE;
            conn.defaultMask = GPM_MOVE;
            conn.minMod = 0;
            conn.maxMod = 0;

            mouse_fd = Gpm_Open (&conn, 0);
            if (mouse_fd == -1)
            {
                use_mouse_p = MOUSE_NONE;
                return;
            }
            mouse_enabled = TRUE;
        }
        break;
#endif /* HAVE_LIBGPM */

    case MOUSE_XTERM_NORMAL_TRACKING:
        /* save old highlight mouse tracking */
        printf (ESC_STR "[?1001s");

        /* enable mouse tracking */
        printf (ESC_STR "[?1000h");

        /* enable SGR extended mouse reporting */
        printf (ESC_STR "[?1006h");

        fflush (stdout);
        mouse_enabled = TRUE;
        break;

    case MOUSE_XTERM_BUTTON_EVENT_TRACKING:
        /* save old highlight mouse tracking */
        printf (ESC_STR "[?1001s");

        /* enable mouse tracking */
        printf (ESC_STR "[?1002h");

        /* enable SGR extended mouse reporting */
        printf (ESC_STR "[?1006h");

        fflush (stdout);
        mouse_enabled = TRUE;
        break;

    default:
        break;
    }
}

/* --------------------------------------------------------------------------------------------- */

void
disable_mouse (void)
{
    if (!mouse_enabled)
        return;

    mouse_enabled = FALSE;

    switch (use_mouse_p)
    {
#ifdef HAVE_LIBGPM
    case MOUSE_GPM:
        Gpm_Close ();
        break;
#endif
    case MOUSE_XTERM_NORMAL_TRACKING:
        /* disable SGR extended mouse reporting */
        printf (ESC_STR "[?1006l");

        /* disable mouse tracking */
        printf (ESC_STR "[?1000l");

        /* restore old highlight mouse tracking */
        printf (ESC_STR "[?1001r");

        fflush (stdout);
        break;
    case MOUSE_XTERM_BUTTON_EVENT_TRACKING:
        /* disable SGR extended mouse reporting */
        printf (ESC_STR "[?1006l");

        /* disable mouse tracking */
        printf (ESC_STR "[?1002l");

        /* restore old highlight mouse tracking */
        printf (ESC_STR "[?1001r");

        fflush (stdout);
        break;
    default:
        break;
    }
}

/* --------------------------------------------------------------------------------------------- */
