/*
   Internal stuff of the terminal controlling library.

   Copyright (C) 2019-2021
   Free Software Foundation, Inc.

   Written by:
   Andrew Borodin <aborodin@vmail.ru>, 2019.

   This file is part of the Midnight Commander.

   The Midnight Commander is free software: you can redistribute it
   and/or modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   The Midnight Commander is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** \file
 *  \brief Source: internal stuff of the terminal controlling library.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "global.h"

#include <glib-unix.h>

#include "tty-internal.h"

/*** global variables ****************************************************************************/

/* pipe to handle SIGWINCH */
int sigwinch_pipe[2];

/*** file scope macro definitions ****************************************************************/

/*** global variables ****************************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
tty_create_winch_pipe (void)
{
    GError *mcerror = NULL;

    if (!g_unix_open_pipe (sigwinch_pipe, FD_CLOEXEC, &mcerror))
    {
        fprintf (stderr, _("\nCannot create pipe for SIGWINCH: %s (%d)\n"),
                 mcerror->message, mcerror->code);
        g_error_free (mcerror);
        exit (EXIT_FAILURE);
    }

    /* If we read from an empty pipe, then read(2) will block until data is available.
     * If we write to a full pipe, then write(2) blocks until sufficient data has been read
     * from the pipe to allow the write to complete..
     * Therefore, use nonblocking I/O.
     */
    if (!g_unix_set_fd_nonblocking (sigwinch_pipe[0], TRUE, &mcerror))
    {
        fprintf (stderr, _("\nCannot configure write end of SIGWINCH pipe: %s (%d)\n"),
                 mcerror->message, mcerror->code);
        g_error_free (mcerror);
        tty_destroy_winch_pipe ();
        exit (EXIT_FAILURE);
    }

    if (!g_unix_set_fd_nonblocking (sigwinch_pipe[1], TRUE, &mcerror))
    {
        fprintf (stderr, _("\nCannot configure read end of SIGWINCH pipe: %s (%d)\n"),
                 mcerror->message, mcerror->code);
        g_error_free (mcerror);
        tty_destroy_winch_pipe ();
        exit (EXIT_FAILURE);
    }
}

/* --------------------------------------------------------------------------------------------- */

void
tty_destroy_winch_pipe (void)
{
    (void) close (sigwinch_pipe[0]);
    (void) close (sigwinch_pipe[1]);
}

/* --------------------------------------------------------------------------------------------- */
