/*
   8bit strings utilities

   Copyright (C) 2007-2021
   Free Software Foundation, Inc.

   Written by:
   Rostislav Benes, 2007

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

#include <ctype.h>
#include <stdlib.h>

#include "global.h"
#include "strutil.h"

/* Functions for singlebyte encodings, all characters have width 1
 * using standard system functions.
 * There are only small differences between functions in strutil8bit.c
 * and strutilascii.c.
 */

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*
 * Inlines to equalize 'char' signedness for single 'char' encodings.
 * Instead of writing
 *    isspace ((unsigned char) c);
 * you can write
 *    char_isspace (c);
 */
#define DECLARE_CTYPE_WRAPPER(func_name)       \
static inline int char_##func_name(char c)     \
{                                              \
    return func_name((int)(unsigned char)c);   \
}

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

static const char replch = '?';

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/* *INDENT-OFF* */
DECLARE_CTYPE_WRAPPER (isalnum)
DECLARE_CTYPE_WRAPPER (isdigit)
DECLARE_CTYPE_WRAPPER (isprint)
DECLARE_CTYPE_WRAPPER (ispunct)
DECLARE_CTYPE_WRAPPER (isspace)
DECLARE_CTYPE_WRAPPER (toupper)
DECLARE_CTYPE_WRAPPER (tolower)
/* *INDENT-ON* */

/* --------------------------------------------------------------------------------------------- */

static void
str_8bit_insert_replace_char (GString * buffer)
{
    g_string_append_c (buffer, replch);
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
str_8bit_is_valid_string (const char *text)
{
    (void) text;
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_is_valid_char (const char *ch, size_t size)
{
    (void) ch;
    (void) size;
    return 1;
}

/* --------------------------------------------------------------------------------------------- */

static void
str_8bit_cnext_char (const char **text)
{
    (*text)++;
}

/* --------------------------------------------------------------------------------------------- */

static void
str_8bit_cprev_char (const char **text)
{
    (*text)--;
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_cnext_noncomb_char (const char **text)
{
    if (*text[0] == '\0')
        return 0;

    (*text)++;
    return 1;
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_cprev_noncomb_char (const char **text, const char *begin)
{
    if ((*text) == begin)
        return 0;

    (*text)--;
    return 1;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
str_8bit_isspace (const char *text)
{
    return char_isspace (text[0]) != 0;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
str_8bit_ispunct (const char *text)
{
    return char_ispunct (text[0]) != 0;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
str_8bit_isalnum (const char *text)
{
    return char_isalnum (text[0]) != 0;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
str_8bit_isdigit (const char *text)
{
    return char_isdigit (text[0]) != 0;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
str_8bit_isprint (const char *text)
{
    return char_isprint (text[0]) != 0;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
str_8bit_iscombiningmark (const char *text)
{
    (void) text;
    return FALSE;
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_toupper (const char *text, char **out, size_t * remain)
{
    if (*remain <= 1)
        return FALSE;

    (*out)[0] = char_toupper (text[0]);
    (*out)++;
    (*remain)--;
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
str_8bit_tolower (const char *text, char **out, size_t * remain)
{
    if (*remain <= 1)
        return FALSE;

    (*out)[0] = char_tolower (text[0]);
    (*out)++;
    (*remain)--;
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_length (const char *text)
{
    return strlen (text);
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_length2 (const char *text, int size)
{
    return (size >= 0) ? MIN (strlen (text), (gsize) size) : strlen (text);
}

/* --------------------------------------------------------------------------------------------- */

static gchar *
str_8bit_conv_gerror_message (GError * mcerror, const char *def_msg)
{
    GIConv conv;
    gchar *ret;

    /* glib messages are in UTF-8 charset */
    conv = str_crt_conv_from ("UTF-8");

    if (conv == INVALID_CONV)
        ret = g_strdup (def_msg != NULL ? def_msg : "");
    else
    {
        GString *buf;

        buf = g_string_new ("");

        if (str_convert (conv, mcerror->message, buf) != ESTR_FAILURE)
            ret = g_string_free (buf, FALSE);
        else
        {
            ret = g_strdup (def_msg != NULL ? def_msg : "");
            g_string_free (buf, TRUE);
        }

        str_close_conv (conv);
    }

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

static estr_t
str_8bit_vfs_convert_to (GIConv coder, const char *string, int size, GString * buffer)
{
    estr_t result = ESTR_SUCCESS;

    if (coder == str_cnv_not_convert)
        g_string_append_len (buffer, string, size);
    else
        result = str_nconvert (coder, string, size, buffer);

    return result;
}

/* --------------------------------------------------------------------------------------------- */

static const char *
str_8bit_term_form (const char *text)
{
    static char result[BUF_MEDIUM];
    char *actual;
    size_t remain;
    size_t length;
    size_t pos = 0;

    actual = result;
    remain = sizeof (result);
    length = strlen (text);

    for (; pos < length && remain > 1; pos++, actual++, remain--)
        actual[0] = char_isprint (text[pos]) ? text[pos] : '.';

    actual[0] = '\0';
    return result;
}

/* --------------------------------------------------------------------------------------------- */

static const char *
str_8bit_fit_to_term (const char *text, int width, align_crt_t just_mode)
{
    static char result[BUF_MEDIUM];
    char *actual;
    size_t remain;
    int ident = 0;
    size_t length;
    size_t pos = 0;

    length = strlen (text);
    actual = result;
    remain = sizeof (result);

    if ((int) length <= width)
    {
        switch (HIDE_FIT (just_mode))
        {
        case J_CENTER_LEFT:
        case J_CENTER:
            ident = (width - length) / 2;
            break;
        case J_RIGHT:
            ident = width - length;
            break;
        default:
            break;
        }

        if ((int) remain <= ident)
            goto finally;
        memset (actual, ' ', ident);
        actual += ident;
        remain -= ident;

        for (; pos < length && remain > 1; pos++, actual++, remain--)
            actual[0] = char_isprint (text[pos]) ? text[pos] : '.';

        if (width - length - ident > 0)
        {
            if (remain <= width - length - ident)
                goto finally;
            memset (actual, ' ', width - length - ident);
            actual += width - length - ident;
        }
    }
    else if (IS_FIT (just_mode))
    {
        for (; pos + 1 <= (gsize) width / 2 && remain > 1; actual++, pos++, remain--)
            actual[0] = char_isprint (text[pos]) ? text[pos] : '.';

        if (remain <= 1)
            goto finally;
        actual[0] = '~';
        actual++;
        remain--;

        pos += length - width + 1;
        for (; pos < length && remain > 1; pos++, actual++, remain--)
            actual[0] = char_isprint (text[pos]) ? text[pos] : '.';
    }
    else
    {
        switch (HIDE_FIT (just_mode))
        {
        case J_CENTER:
            ident = (length - width) / 2;
            break;
        case J_RIGHT:
            ident = length - width;
            break;
        default:
            break;
        }

        pos += ident;
        for (; pos < (gsize) (ident + width) && remain > 1; pos++, actual++, remain--)
            actual[0] = char_isprint (text[pos]) ? text[pos] : '.';
    }

  finally:
    if (actual >= result + sizeof (result))
        actual = result + sizeof (result) - 1;
    actual[0] = '\0';
    return result;
}

/* --------------------------------------------------------------------------------------------- */

static const char *
str_8bit_term_trim (const char *text, int width)
{
    static char result[BUF_MEDIUM];
    size_t remain;
    char *actual;
    size_t length;

    length = strlen (text);
    actual = result;
    remain = sizeof (result);

    if (width > 0)
    {
        size_t pos;

        if (width >= (int) length)
        {
            for (pos = 0; pos < length && remain > 1; pos++, actual++, remain--)
                actual[0] = char_isprint (text[pos]) ? text[pos] : '.';
        }
        else if (width <= 3)
        {
            memset (actual, '.', width);
            actual += width;
        }
        else
        {
            memset (actual, '.', 3);
            actual += 3;
            remain -= 3;

            for (pos = length - width + 3; pos < length && remain > 1; pos++, actual++, remain--)
                actual[0] = char_isprint (text[pos]) ? text[pos] : '.';
        }
    }

    actual[0] = '\0';
    return result;
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_term_width2 (const char *text, size_t length)
{
    return (length != (size_t) (-1)) ? MIN (strlen (text), length) : strlen (text);
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_term_width1 (const char *text)
{
    return str_8bit_term_width2 (text, (size_t) (-1));
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_term_char_width (const char *text)
{
    (void) text;
    return 1;
}

/* --------------------------------------------------------------------------------------------- */

static const char *
str_8bit_term_substring (const char *text, int start, int width)
{
    static char result[BUF_MEDIUM];
    size_t remain;
    char *actual;
    size_t length;

    actual = result;
    remain = sizeof (result);
    length = strlen (text);

    if (start < (int) length)
    {
        size_t pos;

        for (pos = start; pos < length && width > 0 && remain > 1;
             pos++, width--, actual++, remain--)
            actual[0] = char_isprint (text[pos]) ? text[pos] : '.';
    }

    for (; width > 0 && remain > 1; actual++, remain--, width--)
        actual[0] = ' ';

    actual[0] = '\0';
    return result;
}

/* --------------------------------------------------------------------------------------------- */

static const char *
str_8bit_trunc (const char *text, int width)
{
    static char result[MC_MAXPATHLEN];
    int remain;
    char *actual;
    size_t pos = 0;
    size_t length;

    actual = result;
    remain = sizeof (result);
    length = strlen (text);

    if ((int) length > width)
    {
        for (; pos + 1 <= (gsize) width / 2 && remain > 1; actual++, pos++, remain--)
            actual[0] = char_isprint (text[pos]) ? text[pos] : '.';

        if (remain <= 1)
            goto finally;
        actual[0] = '~';
        actual++;
        remain--;

        pos += length - width + 1;
        for (; pos < length && remain > 1; pos++, actual++, remain--)
            actual[0] = char_isprint (text[pos]) ? text[pos] : '.';
    }
    else
    {
        for (; pos < length && remain > 1; pos++, actual++, remain--)
            actual[0] = char_isprint (text[pos]) ? text[pos] : '.';
    }

  finally:
    actual[0] = '\0';
    return result;
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_offset_to_pos (const char *text, size_t length)
{
    (void) text;
    return (int) length;
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_column_to_pos (const char *text, size_t pos)
{
    (void) text;
    return (int) pos;
}

/* --------------------------------------------------------------------------------------------- */

static char *
str_8bit_create_search_needle (const char *needle, gboolean case_sen)
{
    (void) case_sen;
    return (char *) needle;
}

/* --------------------------------------------------------------------------------------------- */

static void
str_8bit_release_search_needle (char *needle, gboolean case_sen)
{
    (void) case_sen;
    (void) needle;
}

/* --------------------------------------------------------------------------------------------- */

static char *
str_8bit_strdown (const char *str)
{
    char *rets, *p;

    if (str == NULL)
        return NULL;

    rets = g_strdup (str);

    for (p = rets; *p != '\0'; p++)
        *p = char_tolower (*p);

    return rets;
}

/* --------------------------------------------------------------------------------------------- */

static const char *
str_8bit_search_first (const char *text, const char *search, gboolean case_sen)
{
    char *fold_text;
    char *fold_search;
    const char *match;

    fold_text = case_sen ? (char *) text : str_8bit_strdown (text);
    fold_search = case_sen ? (char *) search : str_8bit_strdown (search);

    match = g_strstr_len (fold_text, -1, fold_search);
    if (match != NULL)
    {
        size_t offset;

        offset = match - fold_text;
        match = text + offset;
    }

    if (!case_sen)
    {
        g_free (fold_text);
        g_free (fold_search);
    }

    return match;
}

/* --------------------------------------------------------------------------------------------- */

static const char *
str_8bit_search_last (const char *text, const char *search, gboolean case_sen)
{
    char *fold_text;
    char *fold_search;
    const char *match;

    fold_text = case_sen ? (char *) text : str_8bit_strdown (text);
    fold_search = case_sen ? (char *) search : str_8bit_strdown (search);

    match = g_strrstr_len (fold_text, -1, fold_search);
    if (match != NULL)
    {
        size_t offset;

        offset = match - fold_text;
        match = text + offset;
    }

    if (!case_sen)
    {
        g_free (fold_text);
        g_free (fold_search);
    }

    return match;
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_compare (const char *t1, const char *t2)
{
    return strcmp (t1, t2);
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_ncompare (const char *t1, const char *t2)
{
    return strncmp (t1, t2, MIN (strlen (t1), strlen (t2)));
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_casecmp (const char *s1, const char *s2)
{
    /* code from GLib */

#ifdef HAVE_STRCASECMP
    g_return_val_if_fail (s1 != NULL, 0);
    g_return_val_if_fail (s2 != NULL, 0);

    return strcasecmp (s1, s2);
#else
    gint c1, c2;

    g_return_val_if_fail (s1 != NULL, 0);
    g_return_val_if_fail (s2 != NULL, 0);

    while (*s1 != '\0' && *s2 != '\0')
    {
        /* According to A. Cox, some platforms have islower's that
         * don't work right on non-uppercase
         */
        c1 = isupper ((guchar) * s1) ? tolower ((guchar) * s1) : *s1;
        c2 = isupper ((guchar) * s2) ? tolower ((guchar) * s2) : *s2;
        if (c1 != c2)
            return (c1 - c2);
        s1++;
        s2++;
    }

    return (((gint) (guchar) * s1) - ((gint) (guchar) * s2));
#endif
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_ncasecmp (const char *s1, const char *s2)
{
    size_t n;

    g_return_val_if_fail (s1 != NULL, 0);
    g_return_val_if_fail (s2 != NULL, 0);

    n = MIN (strlen (s1), strlen (s2));

    /* code from GLib */

#ifdef HAVE_STRNCASECMP
    return strncasecmp (s1, s2, n);
#else
    gint c1, c2;

    while (n != 0 && *s1 != '\0' && *s2 != '\0')
    {
        n -= 1;
        /* According to A. Cox, some platforms have islower's that
         * don't work right on non-uppercase
         */
        c1 = isupper ((guchar) * s1) ? tolower ((guchar) * s1) : *s1;
        c2 = isupper ((guchar) * s2) ? tolower ((guchar) * s2) : *s2;
        if (c1 != c2)
            return (c1 - c2);
        s1++;
        s2++;
    }

    if (n == 0)
        return 0;

    return (((gint) (guchar) * s1) - ((gint) (guchar) * s2));

#endif
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_prefix (const char *text, const char *prefix)
{
    int result;

    for (result = 0; text[result] != '\0' && prefix[result] != '\0'
         && text[result] == prefix[result]; result++);

    return result;
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_caseprefix (const char *text, const char *prefix)
{
    int result;

    for (result = 0; text[result] != '\0' && prefix[result] != '\0'
         && char_toupper (text[result]) == char_toupper (prefix[result]); result++);

    return result;
}

/* --------------------------------------------------------------------------------------------- */

static void
str_8bit_fix_string (char *text)
{
    (void) text;
}

/* --------------------------------------------------------------------------------------------- */

static char *
str_8bit_create_key (const char *text, gboolean case_sen)
{
    return case_sen ? (char *) text : str_8bit_strdown (text);
}

/* --------------------------------------------------------------------------------------------- */

static int
str_8bit_key_collate (const char *t1, const char *t2, gboolean case_sen)
{
    return case_sen ? strcmp (t1, t2) : strcoll (t1, t2);
}

/* --------------------------------------------------------------------------------------------- */

static void
str_8bit_release_key (char *key, gboolean case_sen)
{
    if (!case_sen)
        g_free (key);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

struct str_class
str_8bit_init (void)
{
    struct str_class result;

    result.conv_gerror_message = str_8bit_conv_gerror_message;
    result.vfs_convert_to = str_8bit_vfs_convert_to;
    result.insert_replace_char = str_8bit_insert_replace_char;
    result.is_valid_string = str_8bit_is_valid_string;
    result.is_valid_char = str_8bit_is_valid_char;
    result.cnext_char = str_8bit_cnext_char;
    result.cprev_char = str_8bit_cprev_char;
    result.cnext_char_safe = str_8bit_cnext_char;
    result.cprev_char_safe = str_8bit_cprev_char;
    result.cnext_noncomb_char = str_8bit_cnext_noncomb_char;
    result.cprev_noncomb_char = str_8bit_cprev_noncomb_char;
    result.char_isspace = str_8bit_isspace;
    result.char_ispunct = str_8bit_ispunct;
    result.char_isalnum = str_8bit_isalnum;
    result.char_isdigit = str_8bit_isdigit;
    result.char_isprint = str_8bit_isprint;
    result.char_iscombiningmark = str_8bit_iscombiningmark;
    result.char_toupper = str_8bit_toupper;
    result.char_tolower = str_8bit_tolower;
    result.length = str_8bit_length;
    result.length2 = str_8bit_length2;
    result.length_noncomb = str_8bit_length;
    result.fix_string = str_8bit_fix_string;
    result.term_form = str_8bit_term_form;
    result.fit_to_term = str_8bit_fit_to_term;
    result.term_trim = str_8bit_term_trim;
    result.term_width2 = str_8bit_term_width2;
    result.term_width1 = str_8bit_term_width1;
    result.term_char_width = str_8bit_term_char_width;
    result.term_substring = str_8bit_term_substring;
    result.trunc = str_8bit_trunc;
    result.offset_to_pos = str_8bit_offset_to_pos;
    result.column_to_pos = str_8bit_column_to_pos;
    result.create_search_needle = str_8bit_create_search_needle;
    result.release_search_needle = str_8bit_release_search_needle;
    result.search_first = str_8bit_search_first;
    result.search_last = str_8bit_search_last;
    result.compare = str_8bit_compare;
    result.ncompare = str_8bit_ncompare;
    result.casecmp = str_8bit_casecmp;
    result.ncasecmp = str_8bit_ncasecmp;
    result.prefix = str_8bit_prefix;
    result.caseprefix = str_8bit_caseprefix;
    result.create_key = str_8bit_create_key;
    result.create_key_for_filename = str_8bit_create_key;
    result.key_collate = str_8bit_key_collate;
    result.release_key = str_8bit_release_key;

    return result;
}

/* --------------------------------------------------------------------------------------------- */
