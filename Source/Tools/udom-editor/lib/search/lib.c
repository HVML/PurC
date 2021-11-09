/*
   Search text engine.
   Common share code for module.

   Copyright (C) 2009-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2009, 2011
   Andrew Borodin <aborodin@vmail.ru>, 2013

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

#include <stdlib.h>
#include <sys/types.h>

#include "lib/global.h"
#include "lib/strutil.h"
#include "lib/search.h"
#ifdef HAVE_CHARSET
#include "lib/charsets.h"
#endif

#include "internal.h"

/*** global variables ****************************************************************************/

const char *STR_E_NOTFOUND = N_("Search string not found");
const char *STR_E_UNKNOWN_TYPE = N_("Not implemented yet");
const char *STR_E_RPL_NOT_EQ_TO_FOUND =
N_("Num of replace tokens not equal to num of found tokens");
const char *STR_E_RPL_INVALID_TOKEN = N_("Invalid token number %d");

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

/*** public functions ****************************************************************************/

gchar *
mc_search__recode_str (const char *str, gsize str_len,
                       const char *charset_from, const char *charset_to, gsize * bytes_written)
{
    gchar *ret = NULL;

    if (charset_from != NULL && charset_to != NULL
        && g_ascii_strcasecmp (charset_to, charset_from) != 0)
    {
        GIConv conv;

        conv = g_iconv_open (charset_to, charset_from);
        if (conv != INVALID_CONV)
        {
            gsize bytes_read;

            ret = g_convert_with_iconv (str, str_len, conv, &bytes_read, bytes_written, NULL);
            g_iconv_close (conv);
        }
    }

    if (ret == NULL)
    {
        *bytes_written = str_len;
        ret = g_strndup (str, str_len);
    }

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

gchar *
mc_search__get_one_symbol (const char *charset, const char *str, gsize str_len,
                           gboolean * just_letters)
{
    gchar *converted_str;
    const gchar *next_char;

    gsize tmp_len;
#ifdef HAVE_CHARSET
    gsize converted_str_len;
    gchar *converted_str2;

    if (charset == NULL)
        charset = cp_source;

    converted_str = mc_search__recode_str (str, str_len, charset, cp_display, &converted_str_len);
#else
    (void) charset;

    converted_str = g_strndup (str, str_len);
#endif

    next_char = str_cget_next_char (converted_str);

    tmp_len = next_char - converted_str;

    converted_str[tmp_len] = '\0';

#ifdef HAVE_CHARSET
    converted_str2 =
        mc_search__recode_str (converted_str, tmp_len, cp_display, charset, &converted_str_len);
#endif
    if (just_letters != NULL)
        *just_letters = str_isalnum (converted_str) && !str_isdigit (converted_str);
#ifdef HAVE_CHARSET
    g_free (converted_str);
    return converted_str2;
#else
    return converted_str;
#endif
}

/* --------------------------------------------------------------------------------------------- */

GString *
mc_search__tolower_case_str (const char *charset, const char *str, gsize str_len)
{
    GString *ret;
#ifdef HAVE_CHARSET
    gchar *converted_str, *tmp_str1, *tmp_str2, *tmp_str3;
    gsize converted_str_len;
    gsize tmp_len;

    if (charset == NULL)
        charset = cp_source;

    tmp_str2 = converted_str =
        mc_search__recode_str (str, str_len, charset, cp_display, &converted_str_len);

    tmp_len = converted_str_len + 1;

    tmp_str3 = tmp_str1 = g_strdup (converted_str);

    while (str_tolower (tmp_str1, &tmp_str2, &tmp_len))
        tmp_str1 += str_length_char (tmp_str1);

    g_free (tmp_str3);
    tmp_str2 =
        mc_search__recode_str (converted_str, converted_str_len, cp_display, charset, &tmp_len);
    g_free (converted_str);

    ret = g_string_new_len (tmp_str2, tmp_len);
    g_free (tmp_str2);
    return ret;
#else
    const gchar *tmp_str1 = str;
    gchar *converted_str, *tmp_str2;
    gsize converted_str_len = str_len + 1;

    (void) charset;

    tmp_str2 = converted_str = g_strndup (str, str_len);

    while (str_tolower (tmp_str1, &tmp_str2, &converted_str_len))
        tmp_str1 += str_length_char (tmp_str1);

    ret = g_string_new_len (converted_str, str_len);
    g_free (converted_str);
    return ret;
#endif
}

/* --------------------------------------------------------------------------------------------- */

GString *
mc_search__toupper_case_str (const char *charset, const char *str, gsize str_len)
{
    GString *ret;
#ifdef HAVE_CHARSET
    gchar *converted_str, *tmp_str1, *tmp_str2, *tmp_str3;
    gsize converted_str_len;
    gsize tmp_len;

    if (charset == NULL)
        charset = cp_source;

    tmp_str2 = converted_str =
        mc_search__recode_str (str, str_len, charset, cp_display, &converted_str_len);

    tmp_len = converted_str_len + 1;

    tmp_str3 = tmp_str1 = g_strdup (converted_str);

    while (str_toupper (tmp_str1, &tmp_str2, &tmp_len))
        tmp_str1 += str_length_char (tmp_str1);

    g_free (tmp_str3);

    tmp_str2 =
        mc_search__recode_str (converted_str, converted_str_len, cp_display, charset, &tmp_len);
    g_free (converted_str);

    ret = g_string_new_len (tmp_str2, tmp_len);
    g_free (tmp_str2);
    return ret;
#else

    const gchar *tmp_str1 = str;
    gchar *converted_str, *tmp_str2;
    gsize converted_str_len = str_len + 1;

    (void) charset;

    tmp_str2 = converted_str = g_strndup (str, str_len);

    while (str_toupper (tmp_str1, &tmp_str2, &converted_str_len))
        tmp_str1 += str_length_char (tmp_str1);

    ret = g_string_new_len (converted_str, str_len);
    g_free (converted_str);
    return ret;
#endif
}

/* --------------------------------------------------------------------------------------------- */

gchar **
mc_search_get_types_strings_array (size_t * num)
{
    gchar **ret;
    int lc_index;
    size_t n;

    const mc_search_type_str_t *type_str;
    const mc_search_type_str_t *types_str = mc_search_types_list_get (&n);

    ret = g_try_new0 (char *, n + 1);
    if (ret == NULL)
        return NULL;

    for (lc_index = 0, type_str = types_str; type_str->str != NULL; type_str++, lc_index++)
        ret[lc_index] = g_strdup (type_str->str);

    /* don't count last NULL item */
    if (num != NULL)
        *num = (size_t) lc_index;

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
