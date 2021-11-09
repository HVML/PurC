/*
   Search text engine.
   Plain search

   Copyright (C) 2009-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2009.

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

#include "lib/global.h"
#include "lib/strutil.h"
#include "lib/search.h"

#include "internal.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

static GString *
mc_search__normal_translate_to_regex (const GString * astr)
{
    const char *str = astr->str;
    GString *buff;
    gsize loop;

    buff = g_string_sized_new (32);

    for (loop = 0; loop < astr->len; loop++)
        switch (str[loop])
        {
        case '*':
        case '?':
        case ',':
        case '{':
        case '}':
        case '[':
        case ']':
        case '\\':
        case '+':
        case '.':
        case '$':
        case '(':
        case ')':
        case '^':
        case '-':
        case '|':
            g_string_append_c (buff, '\\');
            MC_FALLTHROUGH;
        default:
            g_string_append_c (buff, str[loop]);
            break;
        }

    return buff;
}

/*** public functions ****************************************************************************/

void
mc_search__cond_struct_new_init_normal (const char *charset, mc_search_t * lc_mc_search,
                                        mc_search_cond_t * mc_search_cond)
{
    GString *tmp;

    tmp = mc_search__normal_translate_to_regex (mc_search_cond->str);
    g_string_free (mc_search_cond->str, TRUE);

    mc_search_cond->str = tmp;
    mc_search__cond_struct_new_init_regex (charset, lc_mc_search, mc_search_cond);
}

/* --------------------------------------------------------------------------------------------- */

gboolean
mc_search__run_normal (mc_search_t * lc_mc_search, const void *user_data,
                       gsize start_search, gsize end_search, gsize * found_len)
{
    return mc_search__run_regex (lc_mc_search, user_data, start_search, end_search, found_len);
}

/* --------------------------------------------------------------------------------------------- */
GString *
mc_search_normal_prepare_replace_str (mc_search_t * lc_mc_search, GString * replace_str)
{
    (void) lc_mc_search;

    return mc_g_string_dup (replace_str);
}
