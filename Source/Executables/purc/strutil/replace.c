/*
   Functions for replacing substrings in strings.

   Copyright (C) 2013-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2013;

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

#include "global.h"
#include "strutil.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static GString *
str_ptr_array_join (const GPtrArray * str_splints)
{
    GString *return_str;
    guint i;

    return_str = g_string_sized_new (32);
    for (i = 0; i < str_splints->len; i++)
        g_string_append (return_str, g_ptr_array_index (str_splints, i));

    return return_str;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */
/**
 * Replace all substrings 'needle' in string 'haystack' by 'replacement'.
 * If the 'needle' in the 'haystack' will be escaped by backslash,
 * then this occurrence isn't be replaced.
 *
 * @param haystack    string contains substrings for replacement
 * @param needle      string for search
 * @param replacement string for replace
 * @return newly allocated string with replaced substrings
 */

char *
str_replace_all (const char *haystack, const char *needle, const char *replacement)
{
    size_t needle_len;
    GPtrArray *str_splints;
    GString *return_str;

    needle_len = strlen (needle);

    str_splints = g_ptr_array_new_with_free_func (g_free);

    while (TRUE)
    {
        char *needle_in_str;

        needle_in_str = strstr (haystack, needle);
        if (needle_in_str == NULL)
        {
            if (*haystack != '\0')
                g_ptr_array_add (str_splints, g_strdup (haystack));
            break;
        }

        if (strutils_is_char_escaped (haystack, needle_in_str))
        {
            char *backslash = needle_in_str - 1;

            if (haystack != backslash)
                g_ptr_array_add (str_splints, g_strndup (haystack, backslash - haystack));

            g_ptr_array_add (str_splints, g_strndup (backslash + 1, needle_in_str - backslash));
            haystack = needle_in_str + 1;
            continue;
        }
        if (needle_in_str - haystack > 0)
            g_ptr_array_add (str_splints, g_strndup (haystack, needle_in_str - haystack));
        g_ptr_array_add (str_splints, g_strdup (replacement));
        haystack = needle_in_str + needle_len;
    }
    return_str = str_ptr_array_join (str_splints);

    g_ptr_array_free (str_splints, TRUE);

    return g_string_free (return_str, FALSE);
}

/* --------------------------------------------------------------------------------------------- */
