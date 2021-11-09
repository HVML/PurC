/*
   Search text engine.
   HEX-style pattern matching

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

#include <stdio.h>

#include "lib/global.h"
#include "lib/strutil.h"
#include "lib/search.h"
#include "lib/strescape.h"

#include "internal.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

typedef enum
{
    MC_SEARCH_HEX_E_OK,
    MC_SEARCH_HEX_E_NUM_OUT_OF_RANGE,
    MC_SEARCH_HEX_E_INVALID_CHARACTER,
    MC_SEARCH_HEX_E_UNMATCHED_QUOTES
} mc_search_hex_parse_error_t;

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

static GString *
mc_search__hex_translate_to_regex (const GString * astr, mc_search_hex_parse_error_t * error_ptr,
                                   int *error_pos_ptr)
{
    GString *buff;
    const char *str;
    gsize str_len;
    gsize loop = 0;
    mc_search_hex_parse_error_t error = MC_SEARCH_HEX_E_OK;

    buff = g_string_sized_new (64);
    str = astr->str;
    str_len = astr->len;

    while (loop < str_len && error == MC_SEARCH_HEX_E_OK)
    {
        unsigned int val;
        int ptr;

        if (g_ascii_isspace (str[loop]))
        {
            /* Eat-up whitespace between tokens. */
            while (g_ascii_isspace (str[loop]))
                loop++;
        }
        /* cppcheck-suppress invalidscanf */
        else if (sscanf (str + loop, "%x%n", &val, &ptr) == 1)
        {
            if (val > 255)
                error = MC_SEARCH_HEX_E_NUM_OUT_OF_RANGE;
            else
            {
                g_string_append_printf (buff, "\\x%02X", val);
                loop += ptr;
            }
        }
        else if (str[loop] == '"')
        {
            gsize loop2;

            loop2 = loop + 1;

            while (loop2 < str_len)
            {
                if (str[loop2] == '"')
                    break;
                if (str[loop2] == '\\' && loop2 + 1 < str_len)
                    loop2++;
                g_string_append_c (buff, str[loop2]);
                loop2++;
            }

            if (str[loop2] == '\0')
                error = MC_SEARCH_HEX_E_UNMATCHED_QUOTES;
            else
                loop = loop2 + 1;
        }
        else
            error = MC_SEARCH_HEX_E_INVALID_CHARACTER;
    }

    if (error != MC_SEARCH_HEX_E_OK)
    {
        g_string_free (buff, TRUE);
        if (error_ptr != NULL)
            *error_ptr = error;
        if (error_pos_ptr != NULL)
            *error_pos_ptr = loop;
        return NULL;
    }

    return buff;
}

/*** public functions ****************************************************************************/

void
mc_search__cond_struct_new_init_hex (const char *charset, mc_search_t * lc_mc_search,
                                     mc_search_cond_t * mc_search_cond)
{
    GString *tmp;
    mc_search_hex_parse_error_t error = MC_SEARCH_HEX_E_OK;
    int error_pos = 0;

    /*
     * We may be searching in binary data, which is often invalid UTF-8.
     *
     * We have to create a non UTF-8 regex (that is, G_REGEX_RAW) or else, as
     * the data is invalid UTF-8, both GLib's PCRE and our
     * mc_search__g_regex_match_full_safe() are going to fail us. The former by
     * not finding all bytes, the latter by overwriting the supposedly invalid
     * UTF-8 with NULs.
     *
     * To do this, we specify "ASCII" as the charset.
     *
     * In fact, we can specify any charset other than "UTF-8": any such charset
     * will trigger G_REGEX_RAW (see [1]). The output of [2] will be the same
     * for all charsets because it skips the \xXX symbols
     * mc_search__hex_translate_to_regex() outputs.
     *
     * But "ASCII" is the best choice because a hex pattern may contain a
     * quoted string: this way we know [2] will ignore any characters outside
     * ASCII letters range (these ignored chars will be copied verbatim to the
     * output and will match as-is; in other words, in a case-sensitive manner;
     * If the user is interested in case-insensitive searches of international
     * text, he shouldn't be using hex search in the first place.)
     *
     * Switching out of UTF-8 has another advantage:
     *
     * When doing case-insensitive searches, GLib treats \xXX symbols as normal
     * letters and therefore matches both "a" and "A" for the hex pattern
     * "0x61". When we switch out of UTF-8, we're switching to using [2], which
     * doesn't have this issue.
     *
     * [1] mc_search__cond_struct_new_init_regex
     * [2] mc_search__cond_struct_new_regex_ci_str
     */
    if (str_isutf8 (charset))
        charset = "ASCII";

    tmp = mc_search__hex_translate_to_regex (mc_search_cond->str, &error, &error_pos);
    if (tmp != NULL)
    {
        g_string_free (mc_search_cond->str, TRUE);
        mc_search_cond->str = tmp;
        mc_search__cond_struct_new_init_regex (charset, lc_mc_search, mc_search_cond);
    }
    else
    {
        const char *desc;

        switch (error)
        {
        case MC_SEARCH_HEX_E_NUM_OUT_OF_RANGE:
            desc =
                _
                ("Number out of range (should be in byte range, 0 <= n <= 0xFF, expressed in hex)");
            break;
        case MC_SEARCH_HEX_E_INVALID_CHARACTER:
            desc = _("Invalid character");
            break;
        case MC_SEARCH_HEX_E_UNMATCHED_QUOTES:
            desc = _("Unmatched quotes character");
            break;
        default:
            desc = "";
        }

        lc_mc_search->error = MC_SEARCH_E_INPUT;
        lc_mc_search->error_str =
            g_strdup_printf (_("Hex pattern error at position %d:\n%s."), error_pos + 1, desc);
    }
}

/* --------------------------------------------------------------------------------------------- */

gboolean
mc_search__run_hex (mc_search_t * lc_mc_search, const void *user_data,
                    gsize start_search, gsize end_search, gsize * found_len)
{
    return mc_search__run_regex (lc_mc_search, user_data, start_search, end_search, found_len);
}

/* --------------------------------------------------------------------------------------------- */

GString *
mc_search_hex_prepare_replace_str (mc_search_t * lc_mc_search, GString * replace_str)
{
    (void) lc_mc_search;

    return mc_g_string_dup (replace_str);
}

/* --------------------------------------------------------------------------------------------- */
