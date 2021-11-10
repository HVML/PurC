/*
   Provides a functions for working with shell.

   Copyright (C) 2006-2021
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

/** \file shell.c
 *  \brief Source: provides a functions for working with shell.
 */

#include <config.h>

#include <pwd.h>                /* for username in xterm title */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "util.h"


/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

static char rp_shell[PATH_MAX];

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */
/**
 * Get a system shell.
 *
 * @return newly allocated mc_shell_t object with shell name
 */

static mc_shell_t *
mc_shell_get_installed_in_system (void)
{
    mc_shell_t *mc_shell;

    mc_shell = g_new0 (mc_shell_t, 1);

    /* 3rd choice: look for existing shells supported as MC subshells.  */
    if (access ("/bin/bash", X_OK) == 0)
        mc_shell->path = g_strdup ("/bin/bash");
    else if (access ("/bin/ash", X_OK) == 0)
        mc_shell->path = g_strdup ("/bin/ash");
    else if (access ("/bin/dash", X_OK) == 0)
        mc_shell->path = g_strdup ("/bin/dash");
    else if (access ("/bin/busybox", X_OK) == 0)
        mc_shell->path = g_strdup ("/bin/busybox");
    else if (access ("/bin/zsh", X_OK) == 0)
        mc_shell->path = g_strdup ("/bin/zsh");
    else if (access ("/bin/tcsh", X_OK) == 0)
        mc_shell->path = g_strdup ("/bin/tcsh");
    else if (access ("/bin/csh", X_OK) == 0)
        mc_shell->path = g_strdup ("/bin/csh");
    /* No fish as fallback because it is so much different from other shells and
     * in a way exotic (even though user-friendly by name) that we should not
     * present it as a subshell without the user's explicit intention. We rather
     * will not use a subshell but just a command line.
     * else if (access("/bin/fish", X_OK) == 0)
     *     mc_global.tty.shell = g_strdup ("/bin/fish");
     */
    else
        /* Fallback and last resort: system default shell */
        mc_shell->path = g_strdup ("/bin/sh");

    return mc_shell;
}

/* --------------------------------------------------------------------------------------------- */

static char *
mc_shell_get_name_env (void)
{
    const char *shell_env;
    char *shell_name = NULL;

    shell_env = g_getenv ("SHELL");
    if ((shell_env == NULL) || (shell_env[0] == '\0'))
    {
        /* 2nd choice: user login shell */
        struct passwd *pwd;

        pwd = getpwuid (geteuid ());
        if (pwd != NULL)
            shell_name = g_strdup (pwd->pw_shell);
    }
    else
        /* 1st choice: SHELL environment variable */
        shell_name = g_strdup (shell_env);

    return shell_name;
}

/* --------------------------------------------------------------------------------------------- */

static mc_shell_t *
mc_shell_get_from_env (void)
{
    mc_shell_t *mc_shell = NULL;

    char *shell_name;

    shell_name = mc_shell_get_name_env ();

    if (shell_name != NULL)
    {
        mc_shell = g_new0 (mc_shell_t, 1);
        mc_shell->path = shell_name;
    }

    return mc_shell;
}

/* --------------------------------------------------------------------------------------------- */

static void
mc_shell_recognize_real_path (mc_shell_t * mc_shell)
{
    if (strstr (mc_shell->path, "/zsh") != NULL || strstr (mc_shell->real_path, "/zsh") != NULL
        || getenv ("ZSH_VERSION") != NULL)
    {
        /* Also detects ksh symlinked to zsh */
        mc_shell->type = SHELL_ZSH;
        mc_shell->name = "zsh";
    }
    else if (strstr (mc_shell->path, "/tcsh") != NULL
             || strstr (mc_shell->real_path, "/tcsh") != NULL)
    {
        /* Also detects csh symlinked to tcsh */
        mc_shell->type = SHELL_TCSH;
        mc_shell->name = "tcsh";
    }
    else if (strstr (mc_shell->path, "/csh") != NULL
             || strstr (mc_shell->real_path, "/csh") != NULL)
    {
        mc_shell->type = SHELL_TCSH;
        mc_shell->name = "csh";
    }
    else if (strstr (mc_shell->path, "/fish") != NULL
             || strstr (mc_shell->real_path, "/fish") != NULL)
    {
        mc_shell->type = SHELL_FISH;
        mc_shell->name = "fish";
    }
    else if (strstr (mc_shell->path, "/dash") != NULL
             || strstr (mc_shell->real_path, "/dash") != NULL)
    {
        /* Debian ash (also found if symlinked to by ash/sh) */
        mc_shell->type = SHELL_DASH;
        mc_shell->name = "dash";
    }
    else if (strstr (mc_shell->real_path, "/busybox") != NULL)
    {
        /* If shell is symlinked to busybox, assume it is an ash, even though theoretically
         * it could also be a hush (a mini shell for non-MMU systems deactivated by default).
         * For simplicity's sake we assume that busybox always contains an ash, not a hush.
         * On embedded platforms or on server systems, /bin/sh often points to busybox.
         * Sometimes even bash is symlinked to busybox (CONFIG_FEATURE_BASH_IS_ASH option),
         * so we need to check busybox symlinks *before* checking for the name "bash"
         * in order to avoid that case. */
        mc_shell->type = SHELL_ASH_BUSYBOX;
        mc_shell->name = mc_shell->path;
    }
    else
        mc_shell->type = SHELL_NONE;
}

/* --------------------------------------------------------------------------------------------- */

static void
mc_shell_recognize_path (mc_shell_t * mc_shell)
{
    /* If shell is not symlinked to busybox, it is safe to assume it is a real shell */
    if (strstr (mc_shell->path, "/bash") != NULL || getenv ("BASH") != NULL)
    {
        mc_shell->type = SHELL_BASH;
        mc_shell->name = "bash";
    }
    else if (strstr (mc_shell->path, "/sh") != NULL || getenv ("SH") != NULL)
    {
        mc_shell->type = SHELL_SH;
        mc_shell->name = "sh";
    }
    else if (strstr (mc_shell->path, "/ash") != NULL || getenv ("ASH") != NULL)
    {
        mc_shell->type = SHELL_ASH_BUSYBOX;
        mc_shell->name = "ash";
    }
    else
        mc_shell->type = SHELL_NONE;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
mc_shell_init (void)
{
    mc_shell_t *mc_shell;

    mc_shell = mc_shell_get_from_env ();

    if (mc_shell == NULL)
        mc_shell = mc_shell_get_installed_in_system ();

    mc_shell->real_path = mc_realpath (mc_shell->path, rp_shell);

    /* Find out what type of shell we have. Also consider real paths (resolved symlinks)
     * because e.g. csh might point to tcsh, ash to dash or busybox, sh to anything. */

    if (mc_shell->real_path != NULL)
        mc_shell_recognize_real_path (mc_shell);

    if (mc_shell->type == SHELL_NONE)
        mc_shell_recognize_path (mc_shell);

    if (mc_shell->type == SHELL_NONE)
        mc_global.tty.use_subshell = FALSE;

    mc_global.shell = mc_shell;
}

/* --------------------------------------------------------------------------------------------- */

void
mc_shell_deinit (void)
{
    if (mc_global.shell != NULL)
    {
        g_free (mc_global.shell->path);
        MC_PTR_FREE (mc_global.shell);
    }
}

/* --------------------------------------------------------------------------------------------- */
