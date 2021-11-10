/*
   Proxy functions for getting access to public variables into 'filemanager' module.

   Copyright (C) 2015-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2015.

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

#include <config.h>

#include <signal.h>             /* kill() */
#include <sys/types.h>
#include <sys/wait.h>           /* waitpid() */

#include "lib/global.h"

#include "lib/vfs/vfs.h"        /* vfs_get_raw_current_dir() */

#include "src/setup.h"          /* quit */
#include "src/consaver/cons.saver.h"    /* handle_console() */

#include "internal.h"

/*** global variables ****************************************************************************/

/* path to X clipboard utility */

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

const vfs_path_t *
subshell_get_cwd (void)
{
#if 0 // VW: TODO
    if (mc_global.mc_run_mode == MC_RUN_FULL)
        return current_panel->cwd_vpath;
#endif

    return vfs_get_raw_current_dir ();
}

/* --------------------------------------------------------------------------------------------- */

void
subshell_handle_cons_saver (void)
{
#ifdef __linux__
    int status;
    pid_t pid;

    pid = waitpid (cons_saver_pid, &status, WUNTRACED | WNOHANG);

    if (pid == cons_saver_pid)
    {

        if (WIFSTOPPED (status))
            /* Someone has stopped cons.saver - restart it */
            kill (pid, SIGCONT);
        else
        {
            /* cons.saver has died - disable console saving */
            handle_console (CONSOLE_DONE);
            mc_global.tty.console_flag = '\0';
        }

    }
#endif /* __linux__ */
}

/* --------------------------------------------------------------------------------------------- */

int
subshell_get_mainloop_quit (void)
{
    return quit;
}

/* --------------------------------------------------------------------------------------------- */

void
subshell_set_mainloop_quit (const int param_quit)
{
    quit = param_quit;
}

/* --------------------------------------------------------------------------------------------- */
