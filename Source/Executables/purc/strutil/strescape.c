/*
   Functions for escaping and unescaping strings

   Copyright (C) 2009-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2009;
   Patrick Winnertz <winnie@debian.org>, 2009

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

#include <config.h>

#include "foil.h"
#include "strutil.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

static const char ESCAPE_SHELL_CHARS[] = " !#$%()&{}[]`?|<>;*\\\"'";
static const char ESCAPE_REGEX_CHARS[] = "^!#$%()&{}[]`?|<>;*+.\\";
static const char ESCAPE_GLOB_CHARS[] = "$*\\?";

/*** file scope functions ************************************************************************/

/*** public functions ****************************************************************************/

char *
strutils_escape (const char *src, gsize src_len, const char *escaped_chars,
                 gboolean escape_non_printable)
{
    GString *ret;
    gsize curr_index;
    /* do NOT break allocation semantics */
    if (src == NULL)
        return NULL;

    if (*src == '\0')
        return strdup ("");

    ret = g_string_new ("");

    if (src_len == (gsize) (-1))
        src_len = strlen (src);

    for (curr_index = 0; curr_index < src_len; curr_index++)
    {
        if (escape_non_printable)
        {
            switch (src[curr_index])
            {
            case '\n':
                g_string_append (ret, "\\n");
                continue;
            case '\t':
                g_string_append (ret, "\\t");
                continue;
            case '\b':
                g_string_append (ret, "\\b");
                continue;
            case '\0':
                g_string_append (ret, "\\0");
                continue;
            default:
                break;
            }
        }

        if (strchr (escaped_chars, (int) src[curr_index]))
            g_string_append_c (ret, '\\');

        g_string_append_c (ret, src[curr_index]);
    }
    return g_string_free (ret, FALSE);
}

/* --------------------------------------------------------------------------------------------- */
char *
strutils_unescape (const char *src, gsize src_len, const char *unescaped_chars,
                   gboolean unescape_non_printable)
{
    GString *ret;
    gsize curr_index;

    if (src == NULL)
        return NULL;

    if (*src == '\0')
        return strdup ("");

    ret = g_string_sized_new (16);

    if (src_len == (gsize) (-1))
        src_len = strlen (src);
    src_len--;

    for (curr_index = 0; curr_index < src_len; curr_index++)
    {
        if (src[curr_index] != '\\')
        {
            g_string_append_c (ret, src[curr_index]);
            continue;
        }

        curr_index++;

        if (unescaped_chars == ESCAPE_SHELL_CHARS && src[curr_index] == '$')
        {
            /* special case: \$ is used to disallow variable substitution */
            g_string_append_c (ret, '\\');
        }
        else
        {
            if (unescape_non_printable)
            {
                switch (src[curr_index])
                {
                case 'n':
                    g_string_append_c (ret, '\n');
                    continue;
                case 't':
                    g_string_append_c (ret, '\t');
                    continue;
                case 'b':
                    g_string_append_c (ret, '\b');
                    continue;
                case '0':
                    g_string_append_c (ret, '\0');
                    continue;
                default:
                    break;
                }
            }

            if (strchr (unescaped_chars, (int) src[curr_index]) == NULL)
                g_string_append_c (ret, '\\');
        }

        g_string_append_c (ret, src[curr_index]);
    }
    g_string_append_c (ret, src[curr_index]);

    return g_string_free (ret, FALSE);
}

/* --------------------------------------------------------------------------------------------- */

/**
 * To be compatible with the general posix command lines we have to escape
 * strings for the command line
 *
 * @param src string for escaping
 *
 * @return escaped string (which needs to be freed later) or NULL when NULL string is passed.
 */

char *
strutils_shell_escape (const char *src)
{
    return strutils_escape (src, -1, ESCAPE_SHELL_CHARS, FALSE);
}

/* --------------------------------------------------------------------------------------------- */

char *
strutils_glob_escape (const char *src)
{
    return strutils_escape (src, -1, ESCAPE_GLOB_CHARS, TRUE);
}

/* --------------------------------------------------------------------------------------------- */

char *
strutils_regex_escape (const char *src)
{
    return strutils_escape (src, -1, ESCAPE_REGEX_CHARS, TRUE);
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Unescape paths or other strings for e.g the internal cd
 * shell-unescape within a given buffer (writing to it!)
 *
 * @param text string for unescaping
 *
 * @return unescaped string (which needs to be freed)
 */

char *
strutils_shell_unescape (const char *text)
{
    return strutils_unescape (text, -1, ESCAPE_SHELL_CHARS, TRUE);
}

/* --------------------------------------------------------------------------------------------- */

char *
strutils_glob_unescape (const char *text)
{
    return strutils_unescape (text, -1, ESCAPE_GLOB_CHARS, TRUE);
}

/* --------------------------------------------------------------------------------------------- */
char *
strutils_regex_unescape (const char *text)
{
    return strutils_unescape (text, -1, ESCAPE_REGEX_CHARS, TRUE);
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Check if char in pointer contain escape'd chars
 *
 * @param start string for checking
 * @param current pointer to checked character
 *
 * @return TRUE if string contain escaped chars otherwise return FALSE
 */

gboolean
strutils_is_char_escaped (const char *start, const char *current)
{
    int num_esc = 0;

    if (start == NULL || current == NULL || current <= start)
        return FALSE;

    current--;
    while (current >= start && *current == '\\')
    {
        num_esc++;
        current--;
    }
    return (gboolean) num_esc % 2;
}

/* --------------------------------------------------------------------------------------------- */
