/*
   Utilities for VFS modules.

   Copyright (C) 1988-2021
   Free Software Foundation, Inc.

   Copyright (C) 1995, 1996 Miguel de Icaza

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

/**
 * \file
 * \brief Source: Utilities for VFS modules
 * \author Miguel de Icaza
 * \date 1995, 1996
 */

#include <config.h>

#include <ctype.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <string.h>

#include "lib/global.h"
#include "lib/unixcompat.h"
#include "lib/util.h"           /* mc_mkstemps() */
#include "lib/widget.h"         /* message() */
#include "lib/strutil.h"        /* INVALID_CONV */

#include "vfs.h"
#include "utilvfs.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

#ifndef TUNMLEN
#define TUNMLEN 256
#endif
#ifndef TGNMLEN
#define TGNMLEN 256
#endif

#define MC_HISTORY_VFS_PASSWORD       "mc.vfs.password"

/*
 * FIXME2, the "-993" is to reduce the chance of a hit on the first lookup.
 */
#define GUID_DEFAULT_CONST -993

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */


/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */
/** Get current username
 *
 * @return g_malloc()ed string with the name of the currently logged in
 *         user ("anonymous" if uid is not registered in the system)
 */

char *
vfs_get_local_username (void)
{
    struct passwd *p_i;

    p_i = getpwuid (geteuid ());

    return (p_i && p_i->pw_name) ? g_strdup (p_i->pw_name) : g_strdup ("anonymous");    /* Unknown UID, strange */
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Look up a user or group name from a uid/gid, maintaining a cache.
 * FIXME, for now it's a one-entry cache.
 * This file should be modified for non-unix systems to do something
 * reasonable.
 */

/* --------------------------------------------------------------------------------------------- */

int
vfs_finduid (const char *uname)
{
    static int saveuid = GUID_DEFAULT_CONST;
    static char saveuname[TUNMLEN] = "\0";

    size_t uname_len;

    uname_len = strlen (uname);

    if (uname[0] != saveuname[0]        /* Quick test w/o proc call */
        || strncmp (uname, saveuname, MIN (uname_len, TUNMLEN - 1)) != 0)
    {
        struct passwd *pw;

        g_strlcpy (saveuname, uname, TUNMLEN);
        pw = getpwnam (uname);
        if (pw)
        {
            saveuid = pw->pw_uid;
        }
        else
        {
            static int my_uid = GUID_DEFAULT_CONST;

            if (my_uid < 0)
                my_uid = getuid ();

            saveuid = my_uid;
        }
    }
    return saveuid;
}

/* --------------------------------------------------------------------------------------------- */

int
vfs_findgid (const char *gname)
{
    static int savegid = GUID_DEFAULT_CONST;
    static char savegname[TGNMLEN] = "\0";

    size_t gname_len;

    gname_len = strlen (gname);

    if (gname[0] != savegname[0]        /* Quick test w/o proc call */
        || strncmp (gname, savegname, MIN (gname_len, TGNMLEN - 1)) != 0)
    {
        struct group *gr;

        g_strlcpy (savegname, gname, TGNMLEN);
        gr = getgrnam (gname);
        if (gr)
        {
            savegid = gr->gr_gid;
        }
        else
        {
            static int my_gid = GUID_DEFAULT_CONST;

            if (my_gid < 0)
                my_gid = getgid ();

            savegid = my_gid;
        }
    }
    return savegid;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Create a temporary file with a name resembling the original.
 * This is needed e.g. for local copies requested by extfs.
 * Some extfs scripts may look at the extension.
 * We also protect stupid scripts agains dangerous names.
 */

int
vfs_mkstemps (vfs_path_t ** pname_vpath, const char *prefix, const char *param_basename)
{
    const char *p;
    GString *suffix;
    int shift;
    int fd;

    /* Strip directories */
    p = strrchr (param_basename, PATH_SEP);
    if (p == NULL)
        p = param_basename;
    else
        p++;

    /* Protection against very long names */
    shift = strlen (p) - (MC_MAXPATHLEN - 16);
    if (shift > 0)
        p += shift;

    suffix = g_string_sized_new (32);

    /* Protection against unusual characters */
    for (; *p != '\0' && *p != '#'; p++)
        if (strchr (".-_@", *p) != NULL || g_ascii_isalnum (*p))
            g_string_append_c (suffix, *p);

    fd = mc_mkstemps (pname_vpath, prefix, suffix->str);
    g_string_free (suffix, TRUE);

    return fd;
}

/* --------------------------------------------------------------------------------------------- */
/**  Extract the hostname and username from the path
 *
 * Format of the path is [user@]hostname:port/remote-dir, e.g.:
 *
 * ftp://sunsite.unc.edu/pub/linux
 * ftp://miguel@sphinx.nuclecu.unam.mx/c/nc
 * ftp://tsx-11.mit.edu:8192/
 * ftp://joe@foo.edu:11321/private
 * ftp://joe:password@foo.se
 *
 * @param path is an input string to be parsed
 * @param default_port is an input default port
 * @param flags are parsing modifier flags (@see vfs_url_flags_t)
 *
 * @return g_malloc()ed url info.
 *         If the user is empty, e.g. ftp://@roxanne/private, and URL_USE_ANONYMOUS
 *         is not set, then the current login name is supplied.
 *         Return value is a g_malloc()ed structure with the pathname relative to the
 *         host.
 */

vfs_path_element_t *
vfs_url_split (const char *path, int default_port, vfs_url_flags_t flags)
{
    vfs_path_element_t *path_element;

    char *pcopy;
    size_t pcopy_len;
    const char *pend;
    char *colon, *at, *rest;

    path_element = g_new0 (vfs_path_element_t, 1);
    path_element->port = default_port;

    pcopy_len = strlen (path);
    pcopy = g_strndup (path, pcopy_len);
    pend = pcopy + pcopy_len;

    if ((flags & URL_NOSLASH) == 0)
    {
        char *dir = pcopy;

        /* locate path component */
        while (!IS_PATH_SEP (*dir) && *dir != '\0')
            dir++;
        if (*dir == '\0')
            path_element->path = g_strdup (PATH_SEP_STR);
        else
        {
            path_element->path = g_strndup (dir, pcopy_len - (size_t) (dir - pcopy));
            *dir = '\0';
        }
    }

    /* search for any possible user */
    at = strrchr (pcopy, '@');

    /* We have a username */
    if (at == NULL)
        rest = pcopy;
    else
    {
        char *inner_colon;

        *at = '\0';
        inner_colon = strchr (pcopy, ':');
        if (inner_colon != NULL)
        {
            *inner_colon = '\0';
            inner_colon++;
            path_element->password = g_strdup (inner_colon);
        }

        if (*pcopy != '\0')
            path_element->user = g_strdup (pcopy);

        if (pend == at + 1)
            rest = at;
        else
            rest = at + 1;
    }

    if ((flags & URL_USE_ANONYMOUS) == 0)
    {
        g_free (path_element->user);
        path_element->user = vfs_get_local_username ();
    }
    /* Check if the host comes with a port spec, if so, chop it */
    if (*rest != '[')
        colon = strchr (rest, ':');
    else
    {
        colon = strchr (++rest, ']');
        if (colon != NULL)
        {
            colon[0] = '\0';
            colon[1] = '\0';
            colon++;
        }
        else
        {
            vfs_path_element_free (path_element);
            g_free (pcopy);
            return NULL;
        }
    }

    if (colon != NULL)
    {
        *colon = '\0';
        /* cppcheck-suppress invalidscanf */
        if (sscanf (colon + 1, "%d", &path_element->port) == 1)
        {
            if (path_element->port <= 0 || path_element->port >= 65536)
                path_element->port = default_port;
        }
        else
            while (*(++colon) != '\0')
            {
                switch (*colon)
                {
                case 'C':
                    path_element->port = 1;
                    break;
                case 'r':
                    path_element->port = 2;
                    break;
                default:
                    break;
                }
            }
    }

    path_element->host = g_strdup (rest);
    g_free (pcopy);
#ifdef HAVE_CHARSET
    path_element->dir.converter = INVALID_CONV;
#endif

    return path_element;
}

/* --------------------------------------------------------------------------------------------- */

void __attribute__ ((noreturn)) vfs_die (const char *m)
{
    message (D_ERROR, _("Internal error:"), "%s", m);
    exit (EXIT_FAILURE);
}

/* --------------------------------------------------------------------------------------------- */

char *
vfs_get_password (const char *msg)
{
    return input_dialog (msg, _("Password:"), MC_HISTORY_VFS_PASSWORD, INPUT_PASSWORD,
                         INPUT_COMPLETE_NONE);
}

/* --------------------------------------------------------------------------------------------- */
