/*
   Editor syntax highlighting.

   Copyright (C) 1996-2021
   Free Software Foundation, Inc.

   Written by:
   Paul Sheer, 1998
   Leonard den Ottolander <leonard den ottolander nl>, 2005, 2006
   Egmont Koblinger <egmont@gmail.com>, 2010
   Slava Zanko <slavazanko@gmail.com>, 2013
   Andrew Borodin <aborodin@vmail.ru>, 2013, 2014, 2021

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

/** \file
 *  \brief Source: editor syntax highlighting
 *  \author Paul Sheer
 *  \date 1996, 1997
 *  \author Mikhail Pobolovets
 *  \date 2010
 *
 *  Mispelled words are flushed from the syntax highlighting rules
 *  when they have been around longer than
 *  TRANSIENT_WORD_TIME_OUT seconds. At a cursor rate of 30
 *  chars per second and say 3 chars + a space per word, we can
 *  accumulate 450 words absolute max with a value of 60. This is
 *  below this limit of 1024 words in a context.
 */

#include <config.h>

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "lib/global.h"
#include "lib/search.h"         /* search engine */
#include "lib/skin.h"
#include "lib/fileloc.h"        /* EDIT_HOME_DIR, EDIT_HOME_SYNTAX_FILE */
#include "lib/strutil.h"        /* utf string functions */
#include "lib/util.h"
#include "lib/widget.h"         /* Listbox, message() */

#include "edit-impl.h"
#include "editwidget.h"

/*** global variables ****************************************************************************/

gboolean option_syntax_highlighting = TRUE;
gboolean option_auto_syntax = TRUE;

/*** file scope macro definitions ****************************************************************/

/* bytes */
#define SYNTAX_MARKER_DENSITY 512

#define RULE_ON_LEFT_BORDER 1
#define RULE_ON_RIGHT_BORDER 2

#define SYNTAX_TOKEN_STAR       '\001'
#define SYNTAX_TOKEN_PLUS       '\002'
#define SYNTAX_TOKEN_BRACKET    '\003'
#define SYNTAX_TOKEN_BRACE      '\004'

#define break_a { result = line; break; }
#define check_a { if (*a == NULL) { result = line; break; } }
#define check_not_a { if (*a != NULL) { result = line ;break; } }

#define SYNTAX_KEYWORD(x) ((syntax_keyword_t *) (x))
#define CONTEXT_RULE(x) ((context_rule_t *) (x))

#define ARGS_LEN 1024

#define MAX_ENTRY_LEN 40
#define LIST_LINES 14
#define N_DFLT_ENTRIES 2

/*** file scope type declarations ****************************************************************/

typedef struct
{
    char *keyword;
    char *whole_word_chars_left;
    char *whole_word_chars_right;
    gboolean line_start;
    int color;
} syntax_keyword_t;

typedef struct
{
    char *left;
    unsigned char first_left;
    char *right;
    unsigned char first_right;
    gboolean line_start_left;
    gboolean line_start_right;
    gboolean between_delimiters;
    char *whole_word_chars_left;
    char *whole_word_chars_right;
    char *keyword_first_chars;
    gboolean spelling;
    /* first word is word[1] */
    GPtrArray *keyword;
} context_rule_t;

typedef struct
{
    off_t offset;
    edit_syntax_rule_t rule;
} syntax_marker_t;

/*** file scope variables ************************************************************************/

static char *error_file_name = NULL;

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static void
syntax_keyword_free (gpointer keyword)
{
    syntax_keyword_t *k = SYNTAX_KEYWORD (keyword);

    g_free (k->keyword);
    g_free (k->whole_word_chars_left);
    g_free (k->whole_word_chars_right);
    g_free (k);
}

/* --------------------------------------------------------------------------------------------- */

static void
context_rule_free (gpointer rule)
{
    context_rule_t *r = CONTEXT_RULE (rule);

    g_free (r->left);
    g_free (r->right);
    g_free (r->whole_word_chars_left);
    g_free (r->whole_word_chars_right);
    g_free (r->keyword_first_chars);

    if (r->keyword != NULL)
    {
        g_ptr_array_foreach (r->keyword, (GFunc) syntax_keyword_free, NULL);
        g_ptr_array_free (r->keyword, TRUE);
    }

    g_free (r);
}

/* --------------------------------------------------------------------------------------------- */

static gint
mc_defines_destroy (gpointer key, gpointer value, gpointer data)
{
    (void) data;

    g_free (key);
    g_strfreev ((char **) value);

    return FALSE;
}

/* --------------------------------------------------------------------------------------------- */
/** Completely destroys the defines tree */

static void
destroy_defines (GTree ** defines)
{
    g_tree_foreach (*defines, mc_defines_destroy, NULL);
    g_tree_destroy (*defines);
    *defines = NULL;
}

/* --------------------------------------------------------------------------------------------- */

/** Wrapper for case insensitive mode */
inline static int
xx_tolower (const WEdit * edit, int c)
{
    return edit->is_case_insensitive ? tolower (c) : c;
}

/* --------------------------------------------------------------------------------------------- */

static void
subst_defines (GTree * defines, char **argv, char **argv_end)
{
    for (; *argv != NULL && argv < argv_end; argv++)
    {
        char **t;

        t = g_tree_lookup (defines, *argv);
        if (t != NULL)
        {
            int argc, count;
            char **p;

            /* Count argv array members */
            argc = g_strv_length (argv + 1);

            /* Count members of definition array */
            count = g_strv_length (t);

            p = argv + count + argc;
            /* Buffer overflow or infinitive loop in define */
            if (p >= argv_end)
                break;

            /* Move rest of argv after definition members */
            while (argc >= 0)
                *p-- = argv[argc-- + 1];

            /* Copy definition members to argv */
            for (p = argv; *t != NULL; *p++ = *t++)
                ;
        }
    }
}

/* --------------------------------------------------------------------------------------------- */

static off_t
compare_word_to_right (const WEdit * edit, off_t i, const char *text,
                       const char *whole_left, const char *whole_right, gboolean line_start)
{
    const unsigned char *p, *q;
    int c, d, j;

    if (*text == '\0')
        return -1;

    c = xx_tolower (edit, edit_buffer_get_byte (&edit->buffer, i - 1));
    if ((line_start && c != '\n') || (whole_left != NULL && strchr (whole_left, c) != NULL))
        return -1;

    for (p = (const unsigned char *) text, q = p + strlen ((const char *) p); p < q; p++, i++)
    {
        switch (*p)
        {
        case SYNTAX_TOKEN_STAR:
            if (++p > q)
                return -1;
            while (TRUE)
            {
                c = xx_tolower (edit, edit_buffer_get_byte (&edit->buffer, i));
                if (*p == '\0' && whole_right != NULL && strchr (whole_right, c) == NULL)
                    break;
                if (c == *p)
                    break;
                if (c == '\n')
                    return -1;
                i++;
            }
            break;
        case SYNTAX_TOKEN_PLUS:
            if (++p > q)
                return -1;
            j = 0;
            while (TRUE)
            {
                c = xx_tolower (edit, edit_buffer_get_byte (&edit->buffer, i));
                if (c == *p)
                {
                    j = i;
                    if (p[0] == text[0] && p[1] == '\0')        /* handle eg '+' and @+@ keywords properly */
                        break;
                }
                if (j != 0 && strchr ((const char *) p + 1, c) != NULL) /* c exists further down, so it will get matched later */
                    break;
                if (whiteness (c) || (whole_right != NULL && strchr (whole_right, c) == NULL))
                {
                    if (*p == '\0')
                    {
                        i--;
                        break;
                    }
                    if (j == 0)
                        return -1;
                    i = j;
                    break;
                }
                i++;
            }
            break;
        case SYNTAX_TOKEN_BRACKET:
            if (++p > q)
                return -1;
            c = -1;
            while (TRUE)
            {
                d = c;
                c = xx_tolower (edit, edit_buffer_get_byte (&edit->buffer, i));
                for (j = 0; p[j] != SYNTAX_TOKEN_BRACKET && p[j] != '\0'; j++)
                    if (c == p[j])
                        goto found_char2;
                break;
              found_char2:
                i++;
            }
            i--;
            while (*p != SYNTAX_TOKEN_BRACKET && p <= q)
                p++;
            if (p > q)
                return -1;
            if (p[1] == d)
                i--;
            break;
        case SYNTAX_TOKEN_BRACE:
            if (++p > q)
                return -1;
            c = xx_tolower (edit, edit_buffer_get_byte (&edit->buffer, i));
            for (; *p != SYNTAX_TOKEN_BRACE && *p != '\0'; p++)
                if (c == *p)
                    goto found_char3;
            return -1;
          found_char3:
            while (*p != SYNTAX_TOKEN_BRACE && p < q)
                p++;
            break;
        default:
            if (*p != xx_tolower (edit, edit_buffer_get_byte (&edit->buffer, i)))
                return -1;
        }
    }
    return (whole_right != NULL &&
            strchr (whole_right,
                    xx_tolower (edit, edit_buffer_get_byte (&edit->buffer, i))) != NULL) ? -1 : i;
}

/* --------------------------------------------------------------------------------------------- */

static const char *
xx_strchr (const WEdit * edit, const unsigned char *s, int char_byte)
{
    while (*s >= '\005' && xx_tolower (edit, *s) != char_byte)
        s++;

    return (const char *) s;
}

/* --------------------------------------------------------------------------------------------- */

static void
apply_rules_going_right (WEdit * edit, off_t i)
{
    context_rule_t *r;
    int c;
    gboolean contextchanged = FALSE;
    gboolean found_left = FALSE, found_right = FALSE;
    gboolean keyword_foundleft = FALSE, keyword_foundright = FALSE;
    gboolean is_end;
    off_t end = 0;
    edit_syntax_rule_t _rule = edit->rule;

    c = xx_tolower (edit, edit_buffer_get_byte (&edit->buffer, i));
    if (c == 0)
        return;

    is_end = (edit->rule.end == i);

    /* check to turn off a keyword */
    if (_rule.keyword != 0)
    {
        if (edit_buffer_get_byte (&edit->buffer, i - 1) == '\n')
            _rule.keyword = 0;
        if (is_end)
        {
            _rule.keyword = 0;
            keyword_foundleft = TRUE;
        }
    }

    /* check to turn off a context */
    if (_rule.context != 0 && _rule.keyword == 0)
    {
        off_t e;

        r = CONTEXT_RULE (g_ptr_array_index (edit->rules, _rule.context));
        if (r->first_right == c && (edit->rule.border & RULE_ON_RIGHT_BORDER) == 0
            && (e =
                compare_word_to_right (edit, i, r->right, r->whole_word_chars_left,
                                       r->whole_word_chars_right, r->line_start_right)) > 0)
        {
            _rule.end = e;
            found_right = TRUE;
            _rule.border = RULE_ON_RIGHT_BORDER;
            if (r->between_delimiters)
                _rule.context = 0;
        }
        else if (is_end && (edit->rule.border & RULE_ON_RIGHT_BORDER) != 0)
        {
            /* always turn off a context at 4 */
            found_left = TRUE;
            _rule.border = 0;
            if (!keyword_foundleft)
                _rule.context = 0;
        }
        else if (is_end && (edit->rule.border & RULE_ON_LEFT_BORDER) != 0)
        {
            /* never turn off a context at 2 */
            found_left = TRUE;
            _rule.border = 0;
        }
    }

    /* check to turn on a keyword */
    if (_rule.keyword == 0)
    {
        const char *p;

        r = CONTEXT_RULE (g_ptr_array_index (edit->rules, _rule.context));
        p = r->keyword_first_chars;

        if (p != NULL)
            while (*(p = xx_strchr (edit, (const unsigned char *) p + 1, c)) != '\0')
            {
                syntax_keyword_t *k;
                int count;
                off_t e;

                count = p - r->keyword_first_chars;
                k = SYNTAX_KEYWORD (g_ptr_array_index (r->keyword, count));
                e = compare_word_to_right (edit, i, k->keyword, k->whole_word_chars_left,
                                           k->whole_word_chars_right, k->line_start);
                if (e > 0)
                {
                    /* when both context and keyword terminate with a newline,
                       the context overflows to the next line and colorizes it incorrectly */
                    if (e > i + 1 && _rule._context != 0
                        && k->keyword[strlen (k->keyword) - 1] == '\n')
                    {
                        r = CONTEXT_RULE (g_ptr_array_index (edit->rules, _rule._context));
                        if (r->right != NULL && r->right[0] != '\0'
                            && r->right[strlen (r->right) - 1] == '\n')
                            e--;
                    }

                    end = e;
                    _rule.end = e;
                    _rule.keyword = count;
                    keyword_foundright = TRUE;
                    break;
                }
            }
    }

    /* check to turn on a context */
    if (_rule.context == 0)
    {
        if (!found_left && is_end)
        {
            if ((edit->rule.border & RULE_ON_RIGHT_BORDER) != 0)
            {
                _rule.border = 0;
                _rule.context = 0;
                contextchanged = TRUE;
                _rule.keyword = 0;

            }
            else if ((edit->rule.border & RULE_ON_LEFT_BORDER) != 0)
            {
                r = CONTEXT_RULE (g_ptr_array_index (edit->rules, _rule._context));
                _rule.border = 0;
                if (r->between_delimiters)
                {
                    _rule.context = _rule._context;
                    contextchanged = TRUE;
                    _rule.keyword = 0;

                    if (r->first_right == c)
                    {
                        off_t e;

                        e = compare_word_to_right (edit, i, r->right, r->whole_word_chars_left,
                                                   r->whole_word_chars_right, r->line_start_right);
                        if (e >= end)
                        {
                            _rule.end = e;
                            found_right = TRUE;
                            _rule.border = RULE_ON_RIGHT_BORDER;
                            _rule.context = 0;
                        }
                    }
                }
            }
        }

        if (!found_right)
        {
            size_t count;

            for (count = 1; count < edit->rules->len; count++)
            {
                r = CONTEXT_RULE (g_ptr_array_index (edit->rules, count));
                if (r->first_left == c)
                {
                    off_t e;

                    e = compare_word_to_right (edit, i, r->left, r->whole_word_chars_left,
                                               r->whole_word_chars_right, r->line_start_left);
                    if (e >= end && (_rule.keyword == 0 || keyword_foundright))
                    {
                        _rule.end = e;
                        _rule.border = RULE_ON_LEFT_BORDER;
                        _rule._context = count;
                        if (!r->between_delimiters && _rule.keyword == 0)
                        {
                            _rule.context = count;
                            contextchanged = TRUE;
                        }
                        break;
                    }
                }
            }
        }
    }

    /* check again to turn on a keyword if the context switched */
    if (contextchanged && _rule.keyword == 0)
    {
        const char *p;

        r = CONTEXT_RULE (g_ptr_array_index (edit->rules, _rule.context));
        p = r->keyword_first_chars;

        while (*(p = xx_strchr (edit, (const unsigned char *) p + 1, c)) != '\0')
        {
            syntax_keyword_t *k;
            int count;
            off_t e;

            count = p - r->keyword_first_chars;
            k = SYNTAX_KEYWORD (g_ptr_array_index (r->keyword, count));
            e = compare_word_to_right (edit, i, k->keyword, k->whole_word_chars_left,
                                       k->whole_word_chars_right, k->line_start);
            if (e > 0)
            {
                _rule.end = e;
                _rule.keyword = count;
                break;
            }
        }
    }

    edit->rule = _rule;
}

/* --------------------------------------------------------------------------------------------- */

static void
edit_get_rule (WEdit * edit, off_t byte_index)
{
    off_t i;

    if (byte_index > edit->last_get_rule)
    {
        for (i = edit->last_get_rule + 1; i <= byte_index; i++)
        {
            off_t d = SYNTAX_MARKER_DENSITY;

            apply_rules_going_right (edit, i);

            if (edit->syntax_marker != NULL)
                d += ((syntax_marker_t *) edit->syntax_marker->data)->offset;

            if (i > d)
            {
                syntax_marker_t *s;

                s = g_new (syntax_marker_t, 1);
                s->offset = i;
                s->rule = edit->rule;
                edit->syntax_marker = g_slist_prepend (edit->syntax_marker, s);
            }
        }
    }
    else if (byte_index < edit->last_get_rule)
    {
        while (TRUE)
        {
            syntax_marker_t *s;

            if (edit->syntax_marker == NULL)
            {
                memset (&edit->rule, 0, sizeof (edit->rule));
                for (i = -1; i <= byte_index; i++)
                    apply_rules_going_right (edit, i);
                break;
            }

            s = (syntax_marker_t *) edit->syntax_marker->data;

            if (byte_index >= s->offset)
            {
                edit->rule = s->rule;
                for (i = s->offset + 1; i <= byte_index; i++)
                    apply_rules_going_right (edit, i);
                break;
            }

            g_free (s);
            edit->syntax_marker = g_slist_delete_link (edit->syntax_marker, edit->syntax_marker);
        }
    }
    edit->last_get_rule = byte_index;
}

/* --------------------------------------------------------------------------------------------- */

static int
translate_rule_to_color (const WEdit * edit, const edit_syntax_rule_t * rule)
{
    syntax_keyword_t *k;
    context_rule_t *r;

    r = CONTEXT_RULE (g_ptr_array_index (edit->rules, rule->context));
    k = SYNTAX_KEYWORD (g_ptr_array_index (r->keyword, rule->keyword));

    return k->color;
}

/* --------------------------------------------------------------------------------------------- */
/**
   Returns 0 on error/eof or a count of the number of bytes read
   including the newline. Result must be free'd.
   In case of an error, *line will not be modified.
 */

static size_t
read_one_line (char **line, FILE * f)
{
    GString *p;
    size_t r = 0;

    /* not reallocate string too often */
    p = g_string_sized_new (64);

    while (TRUE)
    {
        int c;

        c = fgetc (f);
        if (c == EOF)
        {
            if (ferror (f))
            {
                if (errno == EINTR)
                    continue;
                r = 0;
            }
            break;
        }
        r++;

        /* handle all of \r\n, \r, \n correctly. */
        if (c == '\n')
            break;
        if (c == '\r')
        {
            c = fgetc (f);
            if (c == '\n')
                r++;
            else
                ungetc (c, f);
            break;
        }

        g_string_append_c (p, c);
    }
    if (r != 0)
        *line = g_string_free (p, FALSE);
    else
        g_string_free (p, TRUE);

    return r;
}

/* --------------------------------------------------------------------------------------------- */

static char *
convert (char *s)
{
    char *r, *p;

    p = r = s;
    while (*s)
    {
        switch (*s)
        {
        case '\\':
            s++;
            switch (*s)
            {
            case ' ':
                *p = ' ';
                s--;
                break;
            case 'n':
                *p = '\n';
                break;
            case 'r':
                *p = '\r';
                break;
            case 't':
                *p = '\t';
                break;
            case 's':
                *p = ' ';
                break;
            case '*':
                *p = '*';
                break;
            case '\\':
                *p = '\\';
                break;
            case '[':
            case ']':
                *p = SYNTAX_TOKEN_BRACKET;
                break;
            case '{':
            case '}':
                *p = SYNTAX_TOKEN_BRACE;
                break;
            case 0:
                *p = *s;
                return r;
            default:
                *p = *s;
                break;
            }
            break;
        case '*':
            *p = SYNTAX_TOKEN_STAR;
            break;
        case '+':
            *p = SYNTAX_TOKEN_PLUS;
            break;
        default:
            *p = *s;
            break;
        }
        s++;
        p++;
    }
    *p = '\0';
    return r;
}

/* --------------------------------------------------------------------------------------------- */

static int
get_args (char *l, char **args, int args_size)
{
    int argc = 0;

    while (argc < args_size)
    {
        char *p = l;

        while (*p != '\0' && whiteness (*p))
            p++;
        if (*p == '\0')
            break;
        for (l = p + 1; *l != '\0' && !whiteness (*l); l++)
            ;
        if (*l != '\0')
            *l++ = '\0';
        args[argc++] = convert (p);
    }
    args[argc] = (char *) NULL;
    return argc;
}

/* --------------------------------------------------------------------------------------------- */

static int
this_try_alloc_color_pair (const char *fg, const char *bg, const char *attrs)
{
    char f[80], b[80], a[80], *p;

    if (bg != NULL && *bg == '\0')
        bg = NULL;
    if (fg != NULL && *fg == '\0')
        fg = NULL;
    if (attrs != NULL && *attrs == '\0')
        attrs = NULL;

    if ((fg == NULL) && (bg == NULL))
        return EDITOR_NORMAL_COLOR;

    if (fg != NULL)
    {
        g_strlcpy (f, fg, sizeof (f));
        p = strchr (f, '/');
        if (p != NULL)
            *p = '\0';
        fg = f;
    }
    if (bg != NULL)
    {
        g_strlcpy (b, bg, sizeof (b));
        p = strchr (b, '/');
        if (p != NULL)
            *p = '\0';
        bg = b;
    }
    if ((fg == NULL) || (bg == NULL))
    {
        /* get colors from skin */
        char *editnormal;

        editnormal = mc_skin_get ("editor", "_default_", "default;default");

        if (fg == NULL)
        {
            g_strlcpy (f, editnormal, sizeof (f));
            p = strchr (f, ';');
            if (p != NULL)
                *p = '\0';
            if (f[0] == '\0')
                g_strlcpy (f, "default", sizeof (f));
            fg = f;
        }
        if (bg == NULL)
        {
            p = strchr (editnormal, ';');
            if ((p != NULL) && (*(++p) != '\0'))
                g_strlcpy (b, p, sizeof (b));
            else
                g_strlcpy (b, "default", sizeof (b));
            bg = b;
        }

        g_free (editnormal);
    }

    if (attrs != NULL)
    {
        g_strlcpy (a, attrs, sizeof (a));
        p = strchr (a, '/');
        if (p != NULL)
            *p = '\0';
        /* get_args() mangles the + signs, unmangle 'em */
        p = a;
        while ((p = strchr (p, SYNTAX_TOKEN_PLUS)) != NULL)
            *p++ = '+';
        attrs = a;
    }
    return tty_try_alloc_color_pair (fg, bg, attrs);
}

/* --------------------------------------------------------------------------------------------- */

static FILE *
open_include_file (const char *filename)
{
    FILE *f;

    g_free (error_file_name);
    error_file_name = g_strdup (filename);
    if (g_path_is_absolute (filename))
        return fopen (filename, "r");

    g_free (error_file_name);
    error_file_name =
        g_build_filename (mc_config_get_data_path (), EDIT_HOME_DIR, filename, (char *) NULL);
    f = fopen (error_file_name, "r");
    if (f != NULL)
        return f;

    g_free (error_file_name);
    error_file_name = g_build_filename (mc_global.sysconfig_dir, "syntax", filename, (char *) NULL);
    f = fopen (error_file_name, "r");
    if (f != NULL)
        return f;

    g_free (error_file_name);
    error_file_name =
        g_build_filename (mc_global.share_data_dir, "syntax", filename, (char *) NULL);

    return fopen (error_file_name, "r");
}

/* --------------------------------------------------------------------------------------------- */

inline static void
xx_lowerize_line (WEdit * edit, char *line, size_t len)
{
    if (edit->is_case_insensitive)
    {
        size_t i;

        for (i = 0; i < len; ++i)
            line[i] = tolower (line[i]);
    }
}

/* --------------------------------------------------------------------------------------------- */
/** returns line number on error */

static int
edit_read_syntax_rules (WEdit * edit, FILE * f, char **args, int args_size)
{
    FILE *g = NULL;
    char *fg, *bg, *attrs;
    char last_fg[32] = "", last_bg[32] = "", last_attrs[64] = "";
    char whole_right[512];
    char whole_left[512];
    char *l = NULL;
    int save_line = 0, line = 0;
    context_rule_t *c = NULL;
    gboolean no_words = TRUE;
    int result = 0;

    args[0] = NULL;
    edit->is_case_insensitive = FALSE;

    strcpy (whole_left, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_01234567890");
    strcpy (whole_right, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_01234567890");

    edit->rules = g_ptr_array_new ();

    if (edit->defines == NULL)
        edit->defines = g_tree_new ((GCompareFunc) strcmp);

    while (TRUE)
    {
        char **a;
        size_t len;
        int argc;

        line++;
        l = NULL;

        len = read_one_line (&l, f);
        if (len != 0)
            xx_lowerize_line (edit, l, len);
        else
        {
            if (g == NULL)
                break;

            fclose (f);
            f = g;
            g = NULL;
            line = save_line + 1;
            MC_PTR_FREE (error_file_name);
            MC_PTR_FREE (l);
            len = read_one_line (&l, f);
            if (len == 0)
                break;
            xx_lowerize_line (edit, l, len);
        }

        argc = get_args (l, args, args_size);
        a = args + 1;
        if (args[0] == NULL)
        {
            /* do nothing */
        }
        else if (strcmp (args[0], "include") == 0)
        {
            if (g != NULL || argc != 2)
            {
                result = line;
                break;
            }
            g = f;
            f = open_include_file (args[1]);
            if (f == NULL)
            {
                MC_PTR_FREE (error_file_name);
                result = line;
                break;
            }
            save_line = line;
            line = 0;
        }
        else if (strcmp (args[0], "caseinsensitive") == 0)
        {
            edit->is_case_insensitive = TRUE;
        }
        else if (strcmp (args[0], "wholechars") == 0)
        {
            check_a;
            if (strcmp (*a, "left") == 0)
            {
                a++;
                g_strlcpy (whole_left, *a, sizeof (whole_left));
            }
            else if (strcmp (*a, "right") == 0)
            {
                a++;
                g_strlcpy (whole_right, *a, sizeof (whole_right));
            }
            else
            {
                g_strlcpy (whole_left, *a, sizeof (whole_left));
                g_strlcpy (whole_right, *a, sizeof (whole_right));
            }
            a++;
            check_not_a;
        }
        else if (strcmp (args[0], "context") == 0)
        {
            syntax_keyword_t *k;

            check_a;
            if (edit->rules->len == 0)
            {
                /* first context is the default */
                if (strcmp (*a, "default") != 0)
                    break_a;

                a++;
                c = g_new0 (context_rule_t, 1);
                g_ptr_array_add (edit->rules, c);
                c->left = g_strdup (" ");
                c->right = g_strdup (" ");
            }
            else
            {
                /* Start new context.  */
                c = g_new0 (context_rule_t, 1);
                g_ptr_array_add (edit->rules, c);
                if (strcmp (*a, "exclusive") == 0)
                {
                    a++;
                    c->between_delimiters = TRUE;
                }
                check_a;
                if (strcmp (*a, "whole") == 0)
                {
                    a++;
                    c->whole_word_chars_left = g_strdup (whole_left);
                    c->whole_word_chars_right = g_strdup (whole_right);
                }
                else if (strcmp (*a, "wholeleft") == 0)
                {
                    a++;
                    c->whole_word_chars_left = g_strdup (whole_left);
                }
                else if (strcmp (*a, "wholeright") == 0)
                {
                    a++;
                    c->whole_word_chars_right = g_strdup (whole_right);
                }
                check_a;
                if (strcmp (*a, "linestart") == 0)
                {
                    a++;
                    c->line_start_left = TRUE;
                }
                check_a;
                c->left = g_strdup (*a++);
                check_a;
                if (strcmp (*a, "linestart") == 0)
                {
                    a++;
                    c->line_start_right = TRUE;
                }
                check_a;
                c->right = g_strdup (*a++);
                c->first_left = *c->left;
                c->first_right = *c->right;
            }
            c->keyword = g_ptr_array_new ();
            k = g_new0 (syntax_keyword_t, 1);
            g_ptr_array_add (c->keyword, k);
            no_words = FALSE;
            subst_defines (edit->defines, a, &args[ARGS_LEN]);
            fg = *a;
            if (*a != NULL)
                a++;
            bg = *a;
            if (*a != NULL)
                a++;
            attrs = *a;
            if (*a != NULL)
                a++;
            g_strlcpy (last_fg, fg != NULL ? fg : "", sizeof (last_fg));
            g_strlcpy (last_bg, bg != NULL ? bg : "", sizeof (last_bg));
            g_strlcpy (last_attrs, attrs != NULL ? attrs : "", sizeof (last_attrs));
            k->color = this_try_alloc_color_pair (fg, bg, attrs);
            k->keyword = g_strdup (" ");
            check_not_a;
        }
        else if (strcmp (args[0], "spellcheck") == 0)
        {
            if (c == NULL)
            {
                result = line;
                break;
            }
            c->spelling = TRUE;
        }
        else if (strcmp (args[0], "keyword") == 0)
        {
            context_rule_t *last_rule;
            syntax_keyword_t *k;

            if (no_words)
                break_a;
            check_a;
            last_rule = CONTEXT_RULE (g_ptr_array_index (edit->rules, edit->rules->len - 1));
            k = g_new0 (syntax_keyword_t, 1);
            g_ptr_array_add (last_rule->keyword, k);
            if (strcmp (*a, "whole") == 0)
            {
                a++;
                k->whole_word_chars_left = g_strdup (whole_left);
                k->whole_word_chars_right = g_strdup (whole_right);
            }
            else if (strcmp (*a, "wholeleft") == 0)
            {
                a++;
                k->whole_word_chars_left = g_strdup (whole_left);
            }
            else if (strcmp (*a, "wholeright") == 0)
            {
                a++;
                k->whole_word_chars_right = g_strdup (whole_right);
            }
            check_a;
            if (strcmp (*a, "linestart") == 0)
            {
                a++;
                k->line_start = TRUE;
            }
            check_a;
            if (strcmp (*a, "whole") == 0)
                break_a;

            k->keyword = g_strdup (*a++);
            subst_defines (edit->defines, a, &args[ARGS_LEN]);
            fg = *a;
            if (*a != NULL)
                a++;
            bg = *a;
            if (*a != NULL)
                a++;
            attrs = *a;
            if (*a != NULL)
                a++;
            if (fg == NULL)
                fg = last_fg;
            if (bg == NULL)
                bg = last_bg;
            if (attrs == NULL)
                attrs = last_attrs;
            k->color = this_try_alloc_color_pair (fg, bg, attrs);
            check_not_a;
        }
        else if (*(args[0]) == '#')
        {
            /* do nothing for comment */
        }
        else if (strcmp (args[0], "file") == 0)
        {
            break;
        }
        else if (strcmp (args[0], "define") == 0)
        {
            char *key = *a++;
            char **argv;

            if (argc < 3)
                break_a;
            argv = g_tree_lookup (edit->defines, key);
            if (argv != NULL)
                mc_defines_destroy (NULL, argv, NULL);
            else
                key = g_strdup (key);

            argv = g_new (char *, argc - 1);
            g_tree_insert (edit->defines, key, argv);
            while (*a != NULL)
                *argv++ = g_strdup (*a++);
            *argv = NULL;
        }
        else
        {
            /* anything else is an error */
            break_a;
        }
        MC_PTR_FREE (l);
    }
    MC_PTR_FREE (l);

    if (edit->rules->len == 0)
    {
        g_ptr_array_free (edit->rules, TRUE);
        edit->rules = NULL;
    }

    if (result == 0)
    {
        size_t i;
        GString *first_chars;

        if (edit->rules == NULL)
            return line;

        first_chars = g_string_sized_new (32);

        /* collect first character of keywords */
        for (i = 0; i < edit->rules->len; i++)
        {
            size_t j;

            g_string_set_size (first_chars, 0);
            c = CONTEXT_RULE (g_ptr_array_index (edit->rules, i));

            g_string_append_c (first_chars, (char) 1);
            for (j = 1; j < c->keyword->len; j++)
            {
                syntax_keyword_t *k;

                k = SYNTAX_KEYWORD (g_ptr_array_index (c->keyword, j));
                g_string_append_c (first_chars, k->keyword[0]);
            }

            c->keyword_first_chars = g_strndup (first_chars->str, first_chars->len);
        }

        g_string_free (first_chars, TRUE);
    }

    return result;
}

/* --------------------------------------------------------------------------------------------- */

/* returns -1 on file error, line number on error in file syntax */
static int
edit_read_syntax_file (WEdit * edit, GPtrArray * pnames, const char *syntax_file,
                       const char *editor_file, const char *first_line, const char *type)
{
    FILE *f, *g = NULL;
    char *args[ARGS_LEN], *l = NULL;
    long line = 0;
    int result = 0;
    char *lib_file;
    gboolean found = FALSE;

    f = fopen (syntax_file, "r");
    if (f == NULL)
    {
        lib_file = g_build_filename (mc_global.share_data_dir, "syntax", "Syntax", (char *) NULL);
        f = fopen (lib_file, "r");
        g_free (lib_file);
        if (f == NULL)
            return -1;
    }

    args[0] = NULL;
    while (TRUE)
    {
        line++;
        MC_PTR_FREE (l);
        if (read_one_line (&l, f) == 0)
            break;
        (void) get_args (l, args, ARGS_LEN - 1);        /* Final NULL */
        if (args[0] == NULL)
            continue;

        /* Looking for 'include ...' lines before first 'file ...' ones */
        if (!found && strcmp (args[0], "include") == 0)
        {
            if (args[1] == NULL || (g = open_include_file (args[1])) == NULL)
            {
                result = line;
                break;
            }
            goto found_type;
        }

        /* looking for 'file ...' lines only */
        if (strcmp (args[0], "file") != 0)
            continue;

        found = TRUE;

        /* must have two args or report error */
        if (args[1] == NULL || args[2] == NULL)
        {
            result = line;
            break;
        }

        if (pnames != NULL)
        {
            /* 1: just collecting a list of names of rule sets */
            g_ptr_array_add (pnames, g_strdup (args[2]));
        }
        else if (type != NULL)
        {
            /* 2: rule set was explicitly specified by the caller */
            if (strcmp (type, args[2]) == 0)
                goto found_type;
        }
        else if (editor_file != NULL && edit != NULL)
        {
            /* 3: auto-detect rule set from regular expressions */
            gboolean q;

            q = mc_search (args[1], DEFAULT_CHARSET, editor_file, MC_SEARCH_T_REGEX);
            /* does filename match arg 1 ? */
            if (!q && args[3] != NULL)
            {
                /* does first line match arg 3 ? */
                q = mc_search (args[3], DEFAULT_CHARSET, first_line, MC_SEARCH_T_REGEX);
            }
            if (q)
            {
                int line_error;
                char *syntax_type;

              found_type:
                syntax_type = args[2];
                line_error = edit_read_syntax_rules (edit, g ? g : f, args, ARGS_LEN - 1);
                if (line_error != 0)
                {
                    if (error_file_name == NULL)        /* an included file */
                        result = line + line_error;
                    else
                        result = line_error;
                }
                else
                {
                    g_free (edit->syntax_type);
                    edit->syntax_type = g_strdup (syntax_type);
                    /* if there are no rules then turn off syntax highlighting for speed */
                    if (g == NULL && edit->rules->len == 1)
                    {
                        context_rule_t *r0;

                        r0 = CONTEXT_RULE (g_ptr_array_index (edit->rules, 0));
                        if (r0->keyword->len == 1 && !r0->spelling)
                        {
                            edit_free_syntax_rules (edit);
                            break;
                        }
                    }
                }

                if (g == NULL)
                    break;

                fclose (g);
                g = NULL;
            }
        }
    }
    g_free (l);
    fclose (f);
    return result;
}

/* --------------------------------------------------------------------------------------------- */

static const char *
get_first_editor_line (WEdit * edit)
{
    static char s[256];

    s[0] = '\0';

    if (edit != NULL)
    {
        size_t i;

        for (i = 0; i < sizeof (s) - 1; i++)
        {
            s[i] = edit_buffer_get_byte (&edit->buffer, i);
            if (s[i] == '\n')
            {
                s[i] = '\0';
                break;
            }
        }

        s[sizeof (s) - 1] = '\0';
    }

    return s;
}

/* --------------------------------------------------------------------------------------------- */

static int
pstrcmp (const void *p1, const void *p2)
{
    return strcmp (*(char *const *) p1, *(char *const *) p2);
}

/* --------------------------------------------------------------------------------------------- */

static int
exec_edit_syntax_dialog (const GPtrArray * names, const char *current_syntax)
{
    size_t i;
    Listbox *syntaxlist;

    syntaxlist = create_listbox_window (LIST_LINES, MAX_ENTRY_LEN,
                                        _("Choose syntax highlighting"), NULL);
    LISTBOX_APPEND_TEXT (syntaxlist, 'A', _("< Auto >"), NULL, FALSE);
    LISTBOX_APPEND_TEXT (syntaxlist, 'R', _("< Reload Current Syntax >"), NULL, FALSE);

    for (i = 0; i < names->len; i++)
    {
        const char *name;

        name = g_ptr_array_index (names, i);
        LISTBOX_APPEND_TEXT (syntaxlist, 0, name, NULL, FALSE);
        if (current_syntax != NULL && strcmp (name, current_syntax) == 0)
            listbox_select_entry (syntaxlist->list, i + N_DFLT_ENTRIES);
    }

    return run_listbox (syntaxlist);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

int
edit_get_syntax_color (WEdit * edit, off_t byte_index)
{
    if (!tty_use_colors ())
        return 0;

    if (edit->rules != NULL && byte_index < edit->buffer.size && option_syntax_highlighting)
    {
        edit_get_rule (edit, byte_index);
        return translate_rule_to_color (edit, &edit->rule);
    }

    return EDITOR_NORMAL_COLOR;
}

/* --------------------------------------------------------------------------------------------- */

void
edit_free_syntax_rules (WEdit * edit)
{
    if (edit == NULL)
        return;

    if (edit->defines != NULL)
        destroy_defines (&edit->defines);

    if (edit->rules == NULL)
        return;

    edit_get_rule (edit, -1);
    MC_PTR_FREE (edit->syntax_type);

    g_ptr_array_foreach (edit->rules, (GFunc) context_rule_free, NULL);
    g_ptr_array_free (edit->rules, TRUE);
    edit->rules = NULL;
    g_clear_slist (&edit->syntax_marker, g_free);
    tty_color_free_all_tmp ();
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Load rules into edit struct.  Either edit or *pnames must be NULL.  If
 * edit is NULL, a list of types will be stored into names.  If type is
 * NULL, then the type will be selected according to the filename.
 * type must be edit->syntax_type or NULL
 */
void
edit_load_syntax (WEdit * edit, GPtrArray * pnames, const char *type)
{
    int r;
    char *f = NULL;

    if (option_auto_syntax)
        type = NULL;

    if (edit != NULL)
    {
        char *saved_type;

        saved_type = g_strdup (type);   /* save edit->syntax_type */
        edit_free_syntax_rules (edit);
        edit->syntax_type = saved_type; /* restore edit->syntax_type */
    }

    if (!tty_use_colors ())
        return;

    if (!option_syntax_highlighting && (pnames == NULL || pnames->len == 0))
        return;

    if (edit != NULL && edit->filename_vpath == NULL)
        return;

    f = mc_config_get_full_path (EDIT_HOME_SYNTAX_FILE);
    if (edit != NULL)
        r = edit_read_syntax_file (edit, pnames, f, vfs_path_as_str (edit->filename_vpath),
                                   get_first_editor_line (edit),
                                   option_auto_syntax ? NULL : edit->syntax_type);
    else
        r = edit_read_syntax_file (NULL, pnames, f, NULL, "", NULL);
    if (r == -1)
    {
        edit_free_syntax_rules (edit);
        message (D_ERROR, _("Load syntax file"),
                 _("Cannot open file %s\n%s"), f, unix_error_string (errno));
    }
    else if (r != 0)
    {
        edit_free_syntax_rules (edit);
        message (D_ERROR, _("Load syntax file"),
                 _("Error in file %s on line %d"), error_file_name != NULL ? error_file_name : f,
                 r);
        MC_PTR_FREE (error_file_name);
    }

    g_free (f);
}

/* --------------------------------------------------------------------------------------------- */

const char *
edit_get_syntax_type (const WEdit * edit)
{
    return edit->syntax_type;
}

/* --------------------------------------------------------------------------------------------- */

void
edit_syntax_dialog (WEdit * edit)
{
    GPtrArray *names;
    int syntax;

    names = g_ptr_array_new ();

    /* We fill the list of syntax files every time the editor is invoked.
       Instead we could save the list to a file and update it once the syntax
       file gets updated (either by testing or by explicit user command). */
    edit_load_syntax (NULL, names, NULL);
    g_ptr_array_sort (names, pstrcmp);

    syntax = exec_edit_syntax_dialog (names, edit->syntax_type);
    if (syntax >= 0)
    {
        gboolean force_reload = FALSE;
        char *current_syntax;
        gboolean old_auto_syntax;

        current_syntax = g_strdup (edit->syntax_type);
        old_auto_syntax = option_auto_syntax;

        switch (syntax)
        {
        case 0:                /* auto syntax */
            option_auto_syntax = TRUE;
            break;
        case 1:                /* reload current syntax */
            force_reload = TRUE;
            break;
        default:
            option_auto_syntax = FALSE;
            g_free (edit->syntax_type);
            edit->syntax_type = g_strdup (g_ptr_array_index (names, syntax - N_DFLT_ENTRIES));
        }

        /* Load or unload syntax rules if the option has changed */
        if (force_reload || (option_auto_syntax && !old_auto_syntax) || old_auto_syntax ||
            (current_syntax != NULL && edit->syntax_type != NULL &&
             strcmp (current_syntax, edit->syntax_type) != 0))
            edit_load_syntax (edit, NULL, edit->syntax_type);

        g_free (current_syntax);
    }

    g_ptr_array_foreach (names, (GFunc) g_free, NULL);
    g_ptr_array_free (names, TRUE);
}

/* --------------------------------------------------------------------------------------------- */
