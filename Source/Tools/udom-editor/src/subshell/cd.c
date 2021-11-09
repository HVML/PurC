/*
   cd_to() function.

   Copyright (C) 1995-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2013
   Andrew Borodin <aborodin@vmail.ru>, 2020

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

/** \file cd.c
 *  \brief Source: cd_to() function
 */

#include <config.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "lib/global.h"
#include "lib/vfs/vfs.h"
#include "lib/widget.h"
#include "lib/strescape.h"      /* strutils_shell_unescape() */
#include "lib/util.h"           /* whitespace() */

#include "subshell.h"
#include "cd.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/**
 * Expand the argument to "cd" and change directory.  First try tilde
 * expansion, then variable substitution.  If the CDPATH variable is set
 * (e.g. CDPATH=".:~:/usr"), try all the paths contained there.
 * We do not support such rare substitutions as ${var:-value} etc.
 * No quoting is implemented here, so ${VAR} and $VAR will be always
 * substituted.  Wildcards are not supported either.
 * Advanced users should be encouraged to use "\cd" instead of "cd" if
 * they want the behavior they are used to in the shell.
 *
 * @param _path string to examine
 * @return newly allocated string
 */

static GString *
examine_cd (const char *_path)
{
    /* *INDENT-OFF* */
    typedef enum
    {
        copy_sym,
        subst_var
    } state_t;
    /* *INDENT-ON* */

    state_t state = copy_sym;
    GString *q;
    char *path_tilde, *path;
    char *p;

    /* Tilde expansion */
    path = strutils_shell_unescape (_path);
    path_tilde = tilde_expand (path);
    g_free (path);

    q = g_string_sized_new (32);

    /* Variable expansion */
    for (p = path_tilde; *p != '\0';)
    {
        switch (state)
        {
        case copy_sym:
            if (p[0] == '\\' && p[1] == '$')
            {
                g_string_append_c (q, '$');
                p += 2;
            }
            else if (p[0] != '$' || p[1] == '[' || p[1] == '(')
            {
                g_string_append_c (q, *p);
                p++;
            }
            else
                state = subst_var;
            break;

        case subst_var:
            {
                char *s = NULL;
                char c;
                const char *t = NULL;

                /* skip dollar */
                p++;

                if (p[0] == '{')
                {
                    p++;
                    s = strchr (p, '}');
                }
                if (s == NULL)
                    s = strchr (p, PATH_SEP);
                if (s == NULL)
                    s = strchr (p, '\0');
                c = *s;
                *s = '\0';
                t = getenv (p);
                *s = c;
                if (t == NULL)
                {
                    g_string_append_c (q, '$');
                    if (p[-1] != '$')
                        g_string_append_c (q, '{');
                }
                else
                {
                    g_string_append (q, t);
                    p = s;
                    if (*s == '}')
                        p++;
                }

                state = copy_sym;
                break;
            }

        default:
            break;
        }
    }

    g_free (path_tilde);

    return q;
}

/* --------------------------------------------------------------------------------------------- */

/* CDPATH handling */
static gboolean
handle_cdpath (const char *path)
{
    gboolean result = FALSE;

    /* CDPATH handling */
    if (!IS_PATH_SEP (*path))
    {
        char *cdpath, *p;
        char c;

        cdpath = g_strdup (getenv ("CDPATH"));
        p = cdpath;
        c = (p == NULL) ? '\0' : ':';

        while (!result && c == ':')
        {
            char *s;

            s = strchr (p, ':');
            if (s == NULL)
                s = strchr (p, '\0');
            c = *s;
            *s = '\0';
            if (*p != '\0')
            {
                vfs_path_t *r_vpath;

                r_vpath = vfs_path_build_filename (p, path, (char *) NULL);
                result = subshell_cd (r_vpath, cd_parse_command);
                vfs_path_free (r_vpath, TRUE);
            }
            *s = c;
            p = s + 1;
        }
        g_free (cdpath);
    }

    return result;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/** Execute the cd command to specified path
 *
 * @param path path to cd
 */

void
cd_to (const char *path)
{
    char *p;

    /* Remove leading whitespaces. */
    /* Any final whitespace should be removed here (to see why, try "cd fred "). */
    /* NOTE: I think we should not remove the extra space,
       that way, we can cd into hidden directories */
    /* FIXME: what about interpreting quoted strings like the shell.
       so one could type "cd <tab> M-a <enter>" and it would work. */
    p = g_strstrip (g_strdup (path));

    GString *s_path;
    vfs_path_t *q_vpath;
    gboolean ok;

    s_path = examine_cd (p);

    if (s_path->len == 0)
        q_vpath = vfs_path_from_str (mc_config_get_home_dir ());
    else
        q_vpath = vfs_path_from_str_flags (s_path->str, VPF_NO_CANON);

    ok = subshell_cd (q_vpath, cd_parse_command);
    if (!ok)
        ok = handle_cdpath (s_path->str);
    if (!ok)
    {
        char *d;

        d = vfs_path_to_str_flags (q_vpath, 0, VPF_STRIP_PASSWORD);
        message (D_ERROR, MSG_ERROR, _("Cannot chdir to \"%s\"\n%s"), d,
                 unix_error_string (errno));
        g_free (d);
    }

    vfs_path_free (q_vpath, TRUE);
    g_string_free (s_path, TRUE);

    g_free (p);
}

/* --------------------------------------------------------------------------------------------- */
