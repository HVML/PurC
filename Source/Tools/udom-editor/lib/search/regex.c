/*
   Search text engine.
   Regex search

   Copyright (C) 2009-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2009, 2010, 2011, 2013
   Vitaliy Filippov <vitalif@yourcmc.ru>, 2011
   Andrew Borodin <aborodin@vmail.ru>, 2013-2015

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

#include "lib/global.h"
#include "lib/strutil.h"
#include "lib/search.h"
#include "lib/strescape.h"
#include "lib/util.h"           /* MC_PTR_FREE */

#include "internal.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

#define REPLACE_PREPARE_T_NOTHING_SPECIAL -1
#define REPLACE_PREPARE_T_REPLACE_FLAG    -2
#define REPLACE_PREPARE_T_ESCAPE_SEQ      -3

/*** file scope type declarations ****************************************************************/

typedef enum
{
    REPLACE_T_NO_TRANSFORM = 0,
    REPLACE_T_UPP_TRANSFORM_CHAR = 1,
    REPLACE_T_LOW_TRANSFORM_CHAR = 2,
    REPLACE_T_UPP_TRANSFORM = 4,
    REPLACE_T_LOW_TRANSFORM = 8
} replace_transform_type_t;


/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

static gboolean
mc_search__regex_str_append_if_special (GString * copy_to, const GString * regex_str,
                                        gsize * offset)
{
    const char *special_chars[] = {
        "\\s", "\\S",
        "\\d", "\\D",
        "\\b", "\\B",
        "\\w", "\\W",
        "\\t", "\\n",
        "\\r", "\\f",
        "\\a", "\\e",
        "\\x", "\\X",
        "\\c", "\\C",
        "\\l", "\\L",
        "\\u", "\\U",
        "\\E", "\\Q",
        NULL
    };

    char *tmp_regex_str;
    const char **spec_chr;

    tmp_regex_str = &(regex_str->str[*offset]);

    for (spec_chr = special_chars; *spec_chr != NULL; spec_chr++)
    {
        gsize spec_chr_len;

        spec_chr_len = strlen (*spec_chr);

        if (strncmp (tmp_regex_str, *spec_chr, spec_chr_len) == 0
            && !strutils_is_char_escaped (regex_str->str, tmp_regex_str))
        {
            if (strncmp ("\\x", *spec_chr, spec_chr_len) == 0)
            {
                if (tmp_regex_str[spec_chr_len] != '{')
                    spec_chr_len += 2;
                else
                {
                    while ((spec_chr_len < regex_str->len - *offset)
                           && tmp_regex_str[spec_chr_len] != '}')
                        spec_chr_len++;
                    if (tmp_regex_str[spec_chr_len] == '}')
                        spec_chr_len++;
                }
            }
            g_string_append_len (copy_to, tmp_regex_str, spec_chr_len);
            *offset += spec_chr_len;
            return TRUE;
        }
    }

    return FALSE;
}

/* --------------------------------------------------------------------------------------------- */

static void
mc_search__cond_struct_new_regex_hex_add (const char *charset, GString * str_to,
                                          const char *one_char, gsize str_len)
{
    GString *upp, *low;
    gsize loop;

    upp = mc_search__toupper_case_str (charset, one_char, str_len);
    low = mc_search__tolower_case_str (charset, one_char, str_len);

    for (loop = 0; loop < upp->len; loop++)
    {
        gchar tmp_str[10 + 1];  /* longest content is "[\\x%02X\\x%02X]" */
        gint tmp_len;

        if (loop >= low->len || upp->str[loop] == low->str[loop])
            tmp_len =
                g_snprintf (tmp_str, sizeof (tmp_str), "\\x%02X", (unsigned char) upp->str[loop]);
        else
            tmp_len =
                g_snprintf (tmp_str, sizeof (tmp_str), "[\\x%02X\\x%02X]",
                            (unsigned char) upp->str[loop], (unsigned char) low->str[loop]);

        g_string_append_len (str_to, tmp_str, tmp_len);
    }

    g_string_free (upp, TRUE);
    g_string_free (low, TRUE);
}

/* --------------------------------------------------------------------------------------------- */

static void
mc_search__cond_struct_new_regex_accum_append (const char *charset, GString * str_to,
                                               GString * str_from)
{
    GString *recoded_part;
    gsize loop = 0;

    recoded_part = g_string_sized_new (32);

    while (loop < str_from->len)
    {
        gchar *one_char;
        gsize one_char_len;
        gboolean just_letters;

        one_char =
            mc_search__get_one_symbol (charset, &(str_from->str[loop]),
                                       MIN (str_from->len - loop, 6), &just_letters);
        one_char_len = strlen (one_char);

        if (one_char_len == 0)
            loop++;
        else
        {
            loop += one_char_len;

            if (just_letters)
                mc_search__cond_struct_new_regex_hex_add (charset, recoded_part, one_char,
                                                          one_char_len);
            else
                g_string_append_len (recoded_part, one_char, one_char_len);
        }

        g_free (one_char);
    }

    g_string_append_len (str_to, recoded_part->str, recoded_part->len);
    g_string_free (recoded_part, TRUE);
    g_string_set_size (str_from, 0);
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Creates a case-insensitive version of a regex pattern.
 *
 * For example (assuming ASCII charset): given "\\bHello!\\xAB", returns
 * "\\b[Hh][Ee][Ll][Ll][Oo]!\\xAB" (this example is for easier reading; in
 * reality hex codes are used instead of letters).
 *
 * This function knows not to ruin special regex symbols.
 *
 * This function is used when working with non-UTF-8 charsets: GLib's
 * regex engine doesn't understand such charsets and therefore can't do
 * this job itself.
 */
static GString *
mc_search__cond_struct_new_regex_ci_str (const char *charset, const GString * astr)
{
    GString *accumulator, *spec_char, *ret_str;
    gsize loop;

    ret_str = g_string_sized_new (64);
    accumulator = g_string_sized_new (64);
    spec_char = g_string_sized_new (64);
    loop = 0;

    while (loop <= astr->len)
    {
        if (mc_search__regex_str_append_if_special (spec_char, astr, &loop))
        {
            mc_search__cond_struct_new_regex_accum_append (charset, ret_str, accumulator);
            g_string_append_len (ret_str, spec_char->str, spec_char->len);
            g_string_set_size (spec_char, 0);
            continue;
        }

        if (astr->str[loop] == '[' && !strutils_is_char_escaped (astr->str, &(astr->str[loop])))
        {
            mc_search__cond_struct_new_regex_accum_append (charset, ret_str, accumulator);

            while (loop < astr->len && !(astr->str[loop] == ']'
                                         && !strutils_is_char_escaped (astr->str,
                                                                       &(astr->str[loop]))))
            {
                g_string_append_c (ret_str, astr->str[loop]);
                loop++;
            }

            g_string_append_c (ret_str, astr->str[loop]);
            loop++;
            continue;
        }
        /*
           TODO: handle [ and ]
         */
        g_string_append_c (accumulator, astr->str[loop]);
        loop++;
    }
    mc_search__cond_struct_new_regex_accum_append (charset, ret_str, accumulator);

    g_string_free (accumulator, TRUE);
    g_string_free (spec_char, TRUE);

    return ret_str;
}

/* --------------------------------------------------------------------------------------------- */

#ifdef SEARCH_TYPE_GLIB
/* A thin wrapper above g_regex_match_full that makes sure the string passed
 * to it is valid UTF-8 (unless G_REGEX_RAW compile flag was set), as it is a
 * requirement by glib and it might crash otherwise. See: mc ticket 3449.
 * Be careful: there might be embedded NULs in the strings. */
static gboolean
mc_search__g_regex_match_full_safe (const GRegex * regex,
                                    const gchar * string,
                                    gssize string_len,
                                    gint start_position,
                                    GRegexMatchFlags match_options,
                                    GMatchInfo ** match_info, GError ** error)
{
    char *string_safe, *p, *end;
    gboolean ret;

    if (string_len < 0)
        string_len = strlen (string);

    if ((g_regex_get_compile_flags (regex) & G_REGEX_RAW)
        || g_utf8_validate (string, string_len, NULL))
    {
        return g_regex_match_full (regex, string, string_len, start_position, match_options,
                                   match_info, error);
    }

    /* Correctly handle embedded NULs while copying */
    p = string_safe = g_malloc (string_len + 1);
    memcpy (string_safe, string, string_len);
    string_safe[string_len] = '\0';
    end = p + string_len;

    while (p < end)
    {
        gunichar c = g_utf8_get_char_validated (p, -1);
        if (c != (gunichar) (-1) && c != (gunichar) (-2))
        {
            p = g_utf8_next_char (p);
        }
        else
        {
            /* U+FFFD would be the proper choice, but then we'd have to
               maintain mapping between old and new offsets.
               So rather do a byte by byte replacement. */
            *p++ = '\0';
        }
    }

    ret =
        g_regex_match_full (regex, string_safe, string_len, start_position, match_options,
                            match_info, error);
    g_free (string_safe);
    return ret;
}
#endif /* SEARCH_TYPE_GLIB */

/* --------------------------------------------------------------------------------------------- */

static mc_search__found_cond_t
mc_search__regex_found_cond_one (mc_search_t * lc_mc_search, mc_search_regex_t * regex,
                                 GString * search_str)
{
#ifdef SEARCH_TYPE_GLIB
    GError *mcerror = NULL;

    if (!mc_search__g_regex_match_full_safe
        (regex, search_str->str, search_str->len, 0, G_REGEX_MATCH_NEWLINE_ANY,
         &lc_mc_search->regex_match_info, &mcerror))
    {
        g_match_info_free (lc_mc_search->regex_match_info);
        lc_mc_search->regex_match_info = NULL;
        if (mcerror != NULL)
        {
            lc_mc_search->error = MC_SEARCH_E_REGEX;
            g_free (lc_mc_search->error_str);
            lc_mc_search->error_str =
                str_conv_gerror_message (mcerror, _("Regular expression error"));
            g_error_free (mcerror);
            return COND__FOUND_ERROR;
        }
        return COND__NOT_FOUND;
    }
    lc_mc_search->num_results = g_match_info_get_match_count (lc_mc_search->regex_match_info);
#else /* SEARCH_TYPE_GLIB */
    lc_mc_search->num_results = pcre_exec (regex, lc_mc_search->regex_match_info,
                                           search_str->str, search_str->len, 0, 0,
                                           lc_mc_search->iovector, MC_SEARCH__NUM_REPLACE_ARGS);
    if (lc_mc_search->num_results < 0)
    {
        return COND__NOT_FOUND;
    }
#endif /* SEARCH_TYPE_GLIB */
    return COND__FOUND_OK;

}

/* --------------------------------------------------------------------------------------------- */

static mc_search__found_cond_t
mc_search__regex_found_cond (mc_search_t * lc_mc_search, GString * search_str)
{
    gsize loop1;

    for (loop1 = 0; loop1 < lc_mc_search->conditions->len; loop1++)
    {
        mc_search_cond_t *mc_search_cond;
        mc_search__found_cond_t ret;

        mc_search_cond = (mc_search_cond_t *) g_ptr_array_index (lc_mc_search->conditions, loop1);

        if (!mc_search_cond->regex_handle)
            continue;

        ret =
            mc_search__regex_found_cond_one (lc_mc_search, mc_search_cond->regex_handle,
                                             search_str);
        if (ret != COND__NOT_FOUND)
            return ret;
    }
    return COND__NOT_ALL_FOUND;
}

/* --------------------------------------------------------------------------------------------- */

static int
mc_search_regex__get_max_num_of_replace_tokens (const gchar * str, gsize len)
{
    int max_token = 0;
    gsize loop;
    for (loop = 0; loop < len - 1; loop++)
    {
        if (str[loop] == '\\' && g_ascii_isdigit (str[loop + 1]))
        {
            if (strutils_is_char_escaped (str, &str[loop]))
                continue;
            if (max_token < str[loop + 1] - '0')
                max_token = str[loop + 1] - '0';
            continue;
        }
        if (str[loop] == '$' && str[loop + 1] == '{')
        {
            gsize tmp_len;

            if (strutils_is_char_escaped (str, &str[loop]))
                continue;

            for (tmp_len = 0;
                 loop + tmp_len + 2 < len && (str[loop + 2 + tmp_len] & (char) 0xf0) == 0x30;
                 tmp_len++);

            if (str[loop + 2 + tmp_len] == '}')
            {
                int tmp_token;
                char *tmp_str;

                tmp_str = g_strndup (&str[loop + 2], tmp_len);
                tmp_token = atoi (tmp_str);
                if (max_token < tmp_token)
                    max_token = tmp_token;
                g_free (tmp_str);
            }
        }
    }
    return max_token;
}

/* --------------------------------------------------------------------------------------------- */

static char *
mc_search_regex__get_token_by_num (const mc_search_t * lc_mc_search, gsize lc_index)
{
    int fnd_start = 0, fnd_end = 0;

#ifdef SEARCH_TYPE_GLIB
    g_match_info_fetch_pos (lc_mc_search->regex_match_info, lc_index, &fnd_start, &fnd_end);
#else /* SEARCH_TYPE_GLIB */
    fnd_start = lc_mc_search->iovector[lc_index * 2 + 0];
    fnd_end = lc_mc_search->iovector[lc_index * 2 + 1];
#endif /* SEARCH_TYPE_GLIB */

    if (fnd_end == fnd_start)
        return g_strdup ("");

    return g_strndup (lc_mc_search->regex_buffer->str + fnd_start, fnd_end - fnd_start);

}

/* --------------------------------------------------------------------------------------------- */

static gboolean
mc_search_regex__replace_handle_esc_seq (const GString * replace_str, const gsize current_pos,
                                         gsize * skip_len, int *ret)
{
    char *curr_str = &(replace_str->str[current_pos]);
    char c = curr_str[1];

    if (replace_str->len > current_pos + 2)
    {
        if (c == '{')
        {
            for (*skip_len = 2; /* \{ */
                 current_pos + *skip_len < replace_str->len && curr_str[*skip_len] >= '0'
                 && curr_str[*skip_len] <= '7'; (*skip_len)++)
                ;

            if (current_pos + *skip_len < replace_str->len && curr_str[*skip_len] == '}')
            {
                (*skip_len)++;
                *ret = REPLACE_PREPARE_T_ESCAPE_SEQ;
                return FALSE;
            }
            else
            {
                *ret = REPLACE_PREPARE_T_NOTHING_SPECIAL;
                return TRUE;
            }
        }

        if (c == 'x')
        {
            *skip_len = 2;      /* \x */
            c = curr_str[2];
            if (c == '{')
            {
                for (*skip_len = 3;     /* \x{ */
                     current_pos + *skip_len < replace_str->len
                     && g_ascii_isxdigit ((guchar) curr_str[*skip_len]); (*skip_len)++)
                    ;

                if (current_pos + *skip_len < replace_str->len && curr_str[*skip_len] == '}')
                {
                    (*skip_len)++;
                    *ret = REPLACE_PREPARE_T_ESCAPE_SEQ;
                    return FALSE;
                }
                else
                {
                    *ret = REPLACE_PREPARE_T_NOTHING_SPECIAL;
                    return TRUE;
                }
            }
            else if (!g_ascii_isxdigit ((guchar) c))
            {
                *skip_len = 2;  /* \x without number behind */
                *ret = REPLACE_PREPARE_T_NOTHING_SPECIAL;
                return FALSE;
            }
            else
            {
                c = curr_str[3];
                if (!g_ascii_isxdigit ((guchar) c))
                    *skip_len = 3;      /* \xH */
                else
                    *skip_len = 4;      /* \xHH */
                *ret = REPLACE_PREPARE_T_ESCAPE_SEQ;
                return FALSE;
            }
        }
    }

    if (strchr ("ntvbrfa", c) != NULL)
    {
        *skip_len = 2;
        *ret = REPLACE_PREPARE_T_ESCAPE_SEQ;
        return FALSE;
    }
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

static int
mc_search_regex__process_replace_str (const GString * replace_str, const gsize current_pos,
                                      gsize * skip_len, replace_transform_type_t * replace_flags)
{
    int ret = -1;
    const char *curr_str = &(replace_str->str[current_pos]);

    if (current_pos > replace_str->len)
        return REPLACE_PREPARE_T_NOTHING_SPECIAL;

    *skip_len = 0;

    if (replace_str->len > current_pos + 2 && curr_str[0] == '$' && curr_str[1] == '{'
        && (curr_str[2] & (char) 0xf0) == 0x30)
    {
        char *tmp_str;

        if (strutils_is_char_escaped (replace_str->str, curr_str))
        {
            *skip_len = 1;
            return REPLACE_PREPARE_T_NOTHING_SPECIAL;
        }

        for (*skip_len = 0;
             current_pos + *skip_len + 2 < replace_str->len
             && (curr_str[2 + *skip_len] & (char) 0xf0) == 0x30; (*skip_len)++)
            ;

        if (curr_str[2 + *skip_len] != '}')
            return REPLACE_PREPARE_T_NOTHING_SPECIAL;

        tmp_str = g_strndup (curr_str + 2, *skip_len);
        if (tmp_str == NULL)
            return REPLACE_PREPARE_T_NOTHING_SPECIAL;

        ret = atoi (tmp_str);
        g_free (tmp_str);

        *skip_len += 3;         /* ${} */
        return ret;             /* capture buffer index >= 0 */
    }

    if (curr_str[0] == '\\' && replace_str->len > current_pos + 1)
    {
        if (strutils_is_char_escaped (replace_str->str, curr_str))
        {
            *skip_len = 1;
            return REPLACE_PREPARE_T_NOTHING_SPECIAL;
        }

        if (g_ascii_isdigit (curr_str[1]))
        {
            ret = g_ascii_digit_value (curr_str[1]);    /* capture buffer index >= 0 */
            *skip_len = 2;      /* \\ and one digit */
            return ret;
        }

        if (!mc_search_regex__replace_handle_esc_seq (replace_str, current_pos, skip_len, &ret))
            return ret;

        ret = REPLACE_PREPARE_T_REPLACE_FLAG;
        *skip_len += 2;

        switch (curr_str[1])
        {
        case 'U':
            *replace_flags |= REPLACE_T_UPP_TRANSFORM;
            *replace_flags &= ~REPLACE_T_LOW_TRANSFORM;
            break;
        case 'u':
            *replace_flags |= REPLACE_T_UPP_TRANSFORM_CHAR;
            break;
        case 'L':
            *replace_flags |= REPLACE_T_LOW_TRANSFORM;
            *replace_flags &= ~REPLACE_T_UPP_TRANSFORM;
            break;
        case 'l':
            *replace_flags |= REPLACE_T_LOW_TRANSFORM_CHAR;
            break;
        case 'E':
            *replace_flags = REPLACE_T_NO_TRANSFORM;
            break;
        default:
            ret = REPLACE_PREPARE_T_NOTHING_SPECIAL;
            break;
        }
    }
    return ret;
}

/* --------------------------------------------------------------------------------------------- */

static void
mc_search_regex__process_append_str (GString * dest_str, const char *from, gsize len,
                                     replace_transform_type_t * replace_flags)
{
    gsize loop;
    gsize char_len;

    if (len == (gsize) (-1))
        len = strlen (from);

    if (*replace_flags == REPLACE_T_NO_TRANSFORM)
    {
        g_string_append_len (dest_str, from, len);
        return;
    }

    for (loop = 0; loop < len; loop += char_len)
    {
        GString *tmp_string = NULL;
        char *tmp_str;

        tmp_str = mc_search__get_one_symbol (NULL, from + loop, len - loop, NULL);
        char_len = strlen (tmp_str);

        if ((*replace_flags & REPLACE_T_UPP_TRANSFORM_CHAR) != 0)
        {
            *replace_flags &= ~REPLACE_T_UPP_TRANSFORM_CHAR;
            tmp_string = mc_search__toupper_case_str (NULL, tmp_str, char_len);
            g_string_append_len (dest_str, tmp_string->str, tmp_string->len);
            g_string_free (tmp_string, TRUE);
        }
        else if ((*replace_flags & REPLACE_T_LOW_TRANSFORM_CHAR) != 0)
        {
            *replace_flags &= ~REPLACE_T_LOW_TRANSFORM_CHAR;
            tmp_string = mc_search__tolower_case_str (NULL, tmp_str, char_len);
            g_string_append_len (dest_str, tmp_string->str, tmp_string->len);
            g_string_free (tmp_string, TRUE);
        }
        else if ((*replace_flags & REPLACE_T_UPP_TRANSFORM) != 0)
        {
            tmp_string = mc_search__toupper_case_str (NULL, tmp_str, char_len);
            g_string_append_len (dest_str, tmp_string->str, tmp_string->len);
            g_string_free (tmp_string, TRUE);
        }
        else if ((*replace_flags & REPLACE_T_LOW_TRANSFORM) != 0)
        {
            tmp_string = mc_search__tolower_case_str (NULL, tmp_str, char_len);
            g_string_append_len (dest_str, tmp_string->str, tmp_string->len);
            g_string_free (tmp_string, TRUE);
        }

        g_free (tmp_str);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
mc_search_regex__process_escape_sequence (GString * dest_str, const char *from, gsize len,
                                          replace_transform_type_t * replace_flags,
                                          gboolean is_utf8)
{
    gsize i = 0;
    unsigned int c = 0;
    char b;

    if (len == (gsize) (-1))
        len = strlen (from);
    if (len == 0)
        return;

    if (from[i] == '{')
        i++;
    if (i >= len)
        return;

    if (from[i] == 'x')
    {
        i++;
        if (i < len && from[i] == '{')
            i++;
        for (; i < len; i++)
        {
            if (from[i] >= '0' && from[i] <= '9')
                c = c * 16 + from[i] - '0';
            else if (from[i] >= 'a' && from[i] <= 'f')
                c = c * 16 + 10 + from[i] - 'a';
            else if (from[i] >= 'A' && from[i] <= 'F')
                c = c * 16 + 10 + from[i] - 'A';
            else
                break;
        }
    }
    else if (from[i] >= '0' && from[i] <= '7')
        for (; i < len && from[i] >= '0' && from[i] <= '7'; i++)
            c = c * 8 + from[i] - '0';
    else
    {
        switch (from[i])
        {
        case 'n':
            c = '\n';
            break;
        case 't':
            c = '\t';
            break;
        case 'v':
            c = '\v';
            break;
        case 'b':
            c = '\b';
            break;
        case 'r':
            c = '\r';
            break;
        case 'f':
            c = '\f';
            break;
        case 'a':
            c = '\a';
            break;
        default:
            mc_search_regex__process_append_str (dest_str, from, len, replace_flags);
            return;
        }
    }

    if (c < 0x80 || !is_utf8)
        g_string_append_c (dest_str, (char) c);
    else if (c < 0x800)
    {
        b = 0xC0 | (c >> 6);
        g_string_append_c (dest_str, b);
        b = 0x80 | (c & 0x3F);
        g_string_append_c (dest_str, b);
    }
    else if (c < 0x10000)
    {
        b = 0xE0 | (c >> 12);
        g_string_append_c (dest_str, b);
        b = 0x80 | ((c >> 6) & 0x3F);
        g_string_append_c (dest_str, b);
        b = 0x80 | (c & 0x3F);
        g_string_append_c (dest_str, b);
    }
    else if (c < 0x10FFFF)
    {
        b = 0xF0 | (c >> 16);
        g_string_append_c (dest_str, b);
        b = 0x80 | ((c >> 12) & 0x3F);
        g_string_append_c (dest_str, b);
        b = 0x80 | ((c >> 6) & 0x3F);
        g_string_append_c (dest_str, b);
        b = 0x80 | (c & 0x3F);
        g_string_append_c (dest_str, b);
    }
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
mc_search__cond_struct_new_init_regex (const char *charset, mc_search_t * lc_mc_search,
                                       mc_search_cond_t * mc_search_cond)
{
    if (lc_mc_search->whole_words && !lc_mc_search->is_entire_line)
    {
        /* NOTE: \b as word boundary doesn't allow search
         * whole words with non-ASCII symbols.
         * Update: Is it still true nowadays? Probably not. #2396, #3524 */
        g_string_prepend (mc_search_cond->str, "(?<![\\p{L}\\p{N}_])");
        g_string_append (mc_search_cond->str, "(?![\\p{L}\\p{N}_])");
    }

    {
#ifdef SEARCH_TYPE_GLIB
        GError *mcerror = NULL;
        GRegexCompileFlags g_regex_options = G_REGEX_OPTIMIZE | G_REGEX_DOTALL;

        if (str_isutf8 (charset) && mc_global.utf8_display)
        {
            if (!lc_mc_search->is_case_sensitive)
                g_regex_options |= G_REGEX_CASELESS;
        }
        else
        {
            g_regex_options |= G_REGEX_RAW;

            if (!lc_mc_search->is_case_sensitive)
            {
                GString *tmp;

                tmp = mc_search_cond->str;
                mc_search_cond->str = mc_search__cond_struct_new_regex_ci_str (charset, tmp);
                g_string_free (tmp, TRUE);

            }
        }

        mc_search_cond->regex_handle =
            g_regex_new (mc_search_cond->str->str, g_regex_options, 0, &mcerror);

        if (mcerror != NULL)
        {
            lc_mc_search->error = MC_SEARCH_E_REGEX_COMPILE;
            g_free (lc_mc_search->error_str);
            lc_mc_search->error_str =
                str_conv_gerror_message (mcerror, _("Regular expression error"));
            g_error_free (mcerror);
            return;
        }
#else /* SEARCH_TYPE_GLIB */
        const char *error;
        int erroffset;
        int pcre_options = PCRE_EXTRA | PCRE_MULTILINE;

        if (str_isutf8 (charset) && mc_global.utf8_display)
        {
            pcre_options |= PCRE_UTF8;
            if (!lc_mc_search->is_case_sensitive)
                pcre_options |= PCRE_CASELESS;
        }
        else
        {
            if (!lc_mc_search->is_case_sensitive)
            {
                GString *tmp;

                tmp = mc_search_cond->str;
                mc_search_cond->str = mc_search__cond_struct_new_regex_ci_str (charset, tmp);
                g_string_free (tmp, TRUE);
            }
        }

        mc_search_cond->regex_handle =
            pcre_compile (mc_search_cond->str->str, pcre_options, &error, &erroffset, NULL);
        if (mc_search_cond->regex_handle == NULL)
        {
            mc_search_set_error (lc_mc_search, MC_SEARCH_E_REGEX_COMPILE, "%s", error);
            return;
        }
        lc_mc_search->regex_match_info = pcre_study (mc_search_cond->regex_handle, 0, &error);
        if (lc_mc_search->regex_match_info == NULL && error != NULL)
        {
            mc_search_set_error (lc_mc_search, MC_SEARCH_E_REGEX_COMPILE, "%s", error);
            MC_PTR_FREE (mc_search_cond->regex_handle);
            return;
        }
#endif /* SEARCH_TYPE_GLIB */
    }

    lc_mc_search->is_utf8 = str_isutf8 (charset);
}

/* --------------------------------------------------------------------------------------------- */

gboolean
mc_search__run_regex (mc_search_t * lc_mc_search, const void *user_data,
                      gsize start_search, gsize end_search, gsize * found_len)
{
    mc_search_cbret_t ret = MC_SEARCH_CB_NOTFOUND;
    gsize current_pos, virtual_pos;
    gint start_pos;
    gint end_pos;

    if (lc_mc_search->regex_buffer != NULL)
        g_string_set_size (lc_mc_search->regex_buffer, 0);
    else
        lc_mc_search->regex_buffer = g_string_sized_new (64);

    virtual_pos = current_pos = start_search;
    while (virtual_pos <= end_search)
    {
        g_string_set_size (lc_mc_search->regex_buffer, 0);
        lc_mc_search->start_buffer = current_pos;

        if (lc_mc_search->search_fn != NULL)
        {
            while (TRUE)
            {
                int current_chr = '\n'; /* stop search symbol */

                ret = lc_mc_search->search_fn (user_data, current_pos, &current_chr);

                if (ret == MC_SEARCH_CB_ABORT)
                    break;

                if (ret == MC_SEARCH_CB_INVALID)
                    continue;

                current_pos++;

                if (ret == MC_SEARCH_CB_SKIP)
                    continue;

                virtual_pos++;

                g_string_append_c (lc_mc_search->regex_buffer, (char) current_chr);

                if ((char) current_chr == '\n' || virtual_pos > end_search)
                    break;
            }
        }
        else
        {
            /* optimization for standard case (for search from file manager)
             *  where there is no MC_SEARCH_CB_INVALID or MC_SEARCH_CB_SKIP
             *  return codes, so we can copy line at regex buffer all at once
             */
            while (TRUE)
            {
                const char current_chr = ((const char *) user_data)[current_pos];

                if (current_chr == '\0')
                    break;

                current_pos++;

                if (current_chr == '\n' || current_pos > end_search)
                    break;
            }

            /* use virtual_pos as index of start of current chunk */
            g_string_append_len (lc_mc_search->regex_buffer, (const char *) user_data + virtual_pos,
                                 current_pos - virtual_pos);
            virtual_pos = current_pos;
        }

        switch (mc_search__regex_found_cond (lc_mc_search, lc_mc_search->regex_buffer))
        {
        case COND__FOUND_OK:
#ifdef SEARCH_TYPE_GLIB
            g_match_info_fetch_pos (lc_mc_search->regex_match_info, 0, &start_pos, &end_pos);
#else /* SEARCH_TYPE_GLIB */
            start_pos = lc_mc_search->iovector[0];
            end_pos = lc_mc_search->iovector[1];
#endif /* SEARCH_TYPE_GLIB */
            if (found_len != NULL)
                *found_len = end_pos - start_pos;
            lc_mc_search->normal_offset = lc_mc_search->start_buffer + start_pos;
            return TRUE;
        case COND__NOT_ALL_FOUND:
            break;
        default:
            g_string_free (lc_mc_search->regex_buffer, TRUE);
            lc_mc_search->regex_buffer = NULL;
            return FALSE;
        }

        if ((lc_mc_search->update_fn != NULL) &&
            ((lc_mc_search->update_fn) (user_data, current_pos) == MC_SEARCH_CB_ABORT))
            ret = MC_SEARCH_CB_ABORT;

        if (ret == MC_SEARCH_CB_ABORT || ret == MC_SEARCH_CB_NOTFOUND)
            break;
    }

    g_string_free (lc_mc_search->regex_buffer, TRUE);
    lc_mc_search->regex_buffer = NULL;

    MC_PTR_FREE (lc_mc_search->error_str);
    lc_mc_search->error = ret == MC_SEARCH_CB_ABORT ? MC_SEARCH_E_ABORT : MC_SEARCH_E_NOTFOUND;

    return FALSE;
}

/* --------------------------------------------------------------------------------------------- */

GString *
mc_search_regex_prepare_replace_str (mc_search_t * lc_mc_search, GString * replace_str)
{
    GString *ret;

    int num_replace_tokens;
    gsize loop;
    gsize prev = 0;
    replace_transform_type_t replace_flags = REPLACE_T_NO_TRANSFORM;

    num_replace_tokens =
        mc_search_regex__get_max_num_of_replace_tokens (replace_str->str, replace_str->len);

    if (lc_mc_search->num_results < 0)
        return mc_g_string_dup (replace_str);

    if (num_replace_tokens > lc_mc_search->num_results - 1
        || num_replace_tokens > MC_SEARCH__NUM_REPLACE_ARGS)
    {
        mc_search_set_error (lc_mc_search, MC_SEARCH_E_REGEX_REPLACE, "%s",
                             _(STR_E_RPL_NOT_EQ_TO_FOUND));
        return NULL;
    }

    ret = g_string_sized_new (64);

    for (loop = 0; loop < replace_str->len - 1; loop++)
    {
        int lc_index;
        gchar *tmp_str;
        gsize len = 0;

        lc_index = mc_search_regex__process_replace_str (replace_str, loop, &len, &replace_flags);

        if (lc_index == REPLACE_PREPARE_T_NOTHING_SPECIAL)
        {
            if (len != 0)
            {
                mc_search_regex__process_append_str (ret, replace_str->str + prev, loop - prev,
                                                     &replace_flags);
                mc_search_regex__process_append_str (ret, replace_str->str + loop + 1, len - 1,
                                                     &replace_flags);
                prev = loop + len;
                loop = prev - 1;        /* prepare to loop++ */
            }

            continue;
        }

        if (lc_index == REPLACE_PREPARE_T_REPLACE_FLAG)
        {
            if (loop != 0)
                mc_search_regex__process_append_str (ret, replace_str->str + prev, loop - prev,
                                                     &replace_flags);
            prev = loop + len;
            loop = prev - 1;    /* prepare to loop++ */
            continue;
        }

        /* escape sequence */
        if (lc_index == REPLACE_PREPARE_T_ESCAPE_SEQ)
        {
            mc_search_regex__process_append_str (ret, replace_str->str + prev, loop - prev,
                                                 &replace_flags);
            /* call process_escape_sequence without starting '\\' */
            mc_search_regex__process_escape_sequence (ret, replace_str->str + loop + 1, len - 1,
                                                      &replace_flags, lc_mc_search->is_utf8);
            prev = loop + len;
            loop = prev - 1;    /* prepare to loop++ */
            continue;
        }

        /* invalid capture buffer number */
        if (lc_index > lc_mc_search->num_results)
        {
            g_string_free (ret, TRUE);
            mc_search_set_error (lc_mc_search, MC_SEARCH_E_REGEX_REPLACE,
                                 _(STR_E_RPL_INVALID_TOKEN), lc_index);
            return NULL;
        }

        tmp_str = mc_search_regex__get_token_by_num (lc_mc_search, lc_index);

        if (loop != 0)
            mc_search_regex__process_append_str (ret, replace_str->str + prev, loop - prev,
                                                 &replace_flags);

        mc_search_regex__process_append_str (ret, tmp_str, -1, &replace_flags);
        g_free (tmp_str);

        prev = loop + len;
        loop = prev - 1;        /* prepare to loop++ */
    }

    mc_search_regex__process_append_str (ret, replace_str->str + prev, replace_str->len - prev,
                                         &replace_flags);

    return ret;
}
