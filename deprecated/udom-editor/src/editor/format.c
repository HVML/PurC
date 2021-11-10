/*
   Dynamic paragraph formatting.

   Copyright (C) 2011-2021
   Free Software Foundation, Inc.

   Copyright (C) 1996 Paul Sheer

   Writen by:
   Paul Sheer, 1996
   Andrew Borodin <aborodin@vmail.ru>, 2013, 2014

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
 *  \brief Source: Dynamic paragraph formatting
 *  \author Paul Sheer
 *  \date 1996
 *  \author Andrew Borodin
 *  \date 2013, 2014
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
#include "lib/util.h"

#include "src/setup.h"          /* option_tab_spacing */

#include "edit-impl.h"
#include "editwidget.h"

/*** global variables ****************************************************************************/

char *option_stop_format_chars = NULL;

/*** file scope macro definitions ****************************************************************/

#define tab_width option_tab_spacing

#define FONT_MEAN_WIDTH 1

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static off_t
line_start (const edit_buffer_t * buf, long line)
{
    off_t p;
    long l;

    l = buf->curs_line;
    p = buf->curs1;

    if (line < l)
        p = edit_buffer_get_backward_offset (buf, p, l - line);
    else if (line > l)
        p = edit_buffer_get_forward_offset (buf, p, line - l, 0);

    p = edit_buffer_get_bol (buf, p);
    while (strchr ("\t ", edit_buffer_get_byte (buf, p)) != NULL)
        p++;
    return p;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
bad_line_start (const edit_buffer_t * buf, off_t p)
{
    int c;

    c = edit_buffer_get_byte (buf, p);
    if (c == '.')
    {
        /* `...' is acceptable */
        return !(edit_buffer_get_byte (buf, p + 1) == '.'
                 && edit_buffer_get_byte (buf, p + 2) == '.');
    }
    if (c == '-')
    {
        /* `---' is acceptable */
        return !(edit_buffer_get_byte (buf, p + 1) == '-'
                 && edit_buffer_get_byte (buf, p + 2) == '-');
    }

    return (option_stop_format_chars != NULL && strchr (option_stop_format_chars, c) != NULL);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Find the start of the current paragraph for the purpose of formatting.
 * Return position in the file.
 */

static off_t
begin_paragraph (WEdit * edit, gboolean force, long *lines)
{
    long i;

    for (i = edit->buffer.curs_line - 1; i >= 0; i--)
        if (edit_line_is_blank (edit, i) ||
            (force && bad_line_start (&edit->buffer, line_start (&edit->buffer, i))))
        {
            i++;
            break;
        }

    *lines = edit->buffer.curs_line - i;

    return edit_buffer_get_backward_offset (&edit->buffer,
                                            edit_buffer_get_current_bol (&edit->buffer), *lines);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Find the end of the current paragraph for the purpose of formatting.
 * Return position in the file.
 */

static off_t
end_paragraph (WEdit * edit, gboolean force)
{
    long i;

    for (i = edit->buffer.curs_line + 1; i <= edit->buffer.lines; i++)
        if (edit_line_is_blank (edit, i) ||
            (force && bad_line_start (&edit->buffer, line_start (&edit->buffer, i))))
        {
            i--;
            break;
        }

    return edit_buffer_get_eol (&edit->buffer,
                                edit_buffer_get_forward_offset (&edit->buffer,
                                                                edit_buffer_get_current_bol
                                                                (&edit->buffer),
                                                                i - edit->buffer.curs_line, 0));
}

/* --------------------------------------------------------------------------------------------- */

static GString *
get_paragraph (const edit_buffer_t * buf, off_t p, off_t q, gboolean indent)
{
    GString *t;

    t = g_string_sized_new (128);

    for (; p < q; p++)
    {
        if (indent && edit_buffer_get_byte (buf, p - 1) == '\n')
            while (strchr ("\t ", edit_buffer_get_byte (buf, p)) != NULL)
                p++;

        g_string_append_c (t, edit_buffer_get_byte (buf, p));
    }

    g_string_append_c (t, '\n');

    return t;
}

/* --------------------------------------------------------------------------------------------- */

static inline void
strip_newlines (unsigned char *t, off_t size)
{
    unsigned char *p;

    for (p = t; size-- != 0; p++)
        if (*p == '\n')
            *p = ' ';
}

/* --------------------------------------------------------------------------------------------- */
/**
   This function calculates the number of chars in a line specified to length l in pixels
 */

static inline off_t
next_tab_pos (off_t x)
{
    x += tab_width - x % tab_width;
    return x;
}

/* --------------------------------------------------------------------------------------------- */

static inline off_t
line_pixel_length (unsigned char *t, off_t b, off_t l, gboolean utf8)
{
    off_t xn, x;                /* position conters */
    off_t char_length;          /* character length in bytes */

#ifndef HAVE_CHARSET
    (void) utf8;
#endif

    for (xn = 0, x = 0; xn <= l; x = xn, b += char_length)
    {
        char *tb;

        tb = (char *) t + b;
        char_length = 1;

        switch (tb[0])
        {
        case '\n':
            return b;
        case '\t':
            xn = next_tab_pos (x);
            break;
        default:
#ifdef HAVE_CHARSET
            if (utf8)
            {
                gunichar ch;

                ch = g_utf8_get_char_validated (tb, -1);
                if (ch != (gunichar) (-2) && ch != (gunichar) (-1))
                {
                    char *next_ch;

                    /* Calculate UTF-8 char length */
                    next_ch = g_utf8_next_char (tb);
                    char_length = next_ch - tb;

                    if (g_unichar_iswide (ch))
                        x++;
                }
            }
#endif

            xn = x + 1;
            break;
        }
    }

    return b;
}

/* --------------------------------------------------------------------------------------------- */

static off_t
next_word_start (unsigned char *t, off_t q, off_t size)
{
    off_t i;
    gboolean saw_ws = FALSE;

    for (i = q; i < size; i++)
    {
        switch (t[i])
        {
        case '\n':
            return -1;
        case '\t':
        case ' ':
            saw_ws = TRUE;
            break;
        default:
            if (saw_ws)
                return i;
            break;
        }
    }
    return (-1);
}

/* --------------------------------------------------------------------------------------------- */
/** find the start of a word */

static inline int
word_start (unsigned char *t, off_t q, off_t size)
{
    off_t i;

    if (whitespace (t[q]))
        return next_word_start (t, q, size);

    for (i = q;; i--)
    {
        unsigned char c;

        if (i == 0)
            return (-1);
        c = t[i - 1];
        if (c == '\n')
            return (-1);
        if (whitespace (c))
            return i;
    }
}

/* --------------------------------------------------------------------------------------------- */
/** replaces ' ' with '\n' to properly format a paragraph */

static inline void
format_this (unsigned char *t, off_t size, long indent, gboolean utf8)
{
    off_t q = 0, ww;

    strip_newlines (t, size);
    ww = option_word_wrap_line_length * FONT_MEAN_WIDTH - indent;
    if (ww < FONT_MEAN_WIDTH * 2)
        ww = FONT_MEAN_WIDTH * 2;

    while (TRUE)
    {
        off_t p;

        q = line_pixel_length (t, q, ww, utf8);
        if (q > size)
            break;
        if (t[q] == '\n')
            break;
        p = word_start (t, q, size);
        if (p == -1)
            q = next_word_start (t, q, size);   /* Return the end of the word if the beginning
                                                   of the word is at the beginning of a line
                                                   (i.e. a very long word) */
        else
            q = p;
        if (q == -1)            /* end of paragraph */
            break;
        if (q != 0)
            t[q - 1] = '\n';
    }
}

/* --------------------------------------------------------------------------------------------- */

static inline void
replace_at (WEdit * edit, off_t q, int c)
{
    edit_cursor_move (edit, q - edit->buffer.curs1);
    edit_delete (edit, TRUE);
    edit_insert_ahead (edit, c);
}

/* --------------------------------------------------------------------------------------------- */

static long
edit_indent_width (const WEdit * edit, off_t p)
{
    off_t q = p;

    /* move to the end of the leading whitespace of the line */
    while (strchr ("\t ", edit_buffer_get_byte (&edit->buffer, q)) != NULL
           && q < edit->buffer.size - 1)
        q++;
    /* count the number of columns of indentation */
    return (long) edit_move_forward3 (edit, p, 0, q);
}

/* --------------------------------------------------------------------------------------------- */

static void
edit_insert_indent (WEdit * edit, long indent)
{
    if (!option_fill_tabs_with_spaces)
        while (indent >= TAB_SIZE)
        {
            edit_insert (edit, '\t');
            indent -= TAB_SIZE;
        }

    while (indent-- > 0)
        edit_insert (edit, ' ');
}

/* --------------------------------------------------------------------------------------------- */
/** replaces a block of text */

static inline void
put_paragraph (WEdit * edit, unsigned char *t, off_t p, long indent, off_t size)
{
    off_t cursor;
    off_t i;
    int c = '\0';

    cursor = edit->buffer.curs1;
    if (indent != 0)
        while (strchr ("\t ", edit_buffer_get_byte (&edit->buffer, p)) != NULL)
            p++;
    for (i = 0; i < size; i++, p++)
    {
        if (i != 0 && indent != 0)
        {
            if (t[i - 1] == '\n' && c == '\n')
            {
                while (strchr ("\t ", edit_buffer_get_byte (&edit->buffer, p)) != NULL)
                    p++;
            }
            else if (t[i - 1] == '\n')
            {
                off_t curs;

                edit_cursor_move (edit, p - edit->buffer.curs1);
                curs = edit->buffer.curs1;
                edit_insert_indent (edit, indent);
                if (cursor >= curs)
                    cursor += edit->buffer.curs1 - p;
                p = edit->buffer.curs1;
            }
            else if (c == '\n')
            {
                edit_cursor_move (edit, p - edit->buffer.curs1);
                while (strchr ("\t ", edit_buffer_get_byte (&edit->buffer, p)) != NULL)
                {
                    edit_delete (edit, TRUE);
                    if (cursor > edit->buffer.curs1)
                        cursor--;
                }
                p = edit->buffer.curs1;
            }
        }

        c = edit_buffer_get_byte (&edit->buffer, p);
        if (c != t[i])
            replace_at (edit, p, t[i]);
    }
    edit_cursor_move (edit, cursor - edit->buffer.curs1);       /* restore cursor position */
}

/* --------------------------------------------------------------------------------------------- */

static inline long
test_indent (const WEdit * edit, off_t p, off_t q)
{
    long indent;

    indent = edit_indent_width (edit, p++);
    if (indent == 0)
        return 0;

    for (; p < q; p++)
        if (edit_buffer_get_byte (&edit->buffer, p - 1) == '\n'
            && indent != edit_indent_width (edit, p))
            return 0;
    return indent;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
format_paragraph (WEdit * edit, gboolean force)
{
    off_t p, q;
    long lines;
    off_t size;
    GString *t;
    long indent;
    unsigned char *t2;
    gboolean utf8 = FALSE;

    if (option_word_wrap_line_length < 2)
        return;
    if (edit_line_is_blank (edit, edit->buffer.curs_line))
        return;

    p = begin_paragraph (edit, force, &lines);
    q = end_paragraph (edit, force);
    indent = test_indent (edit, p, q);

    t = get_paragraph (&edit->buffer, p, q, indent != 0);
    size = t->len - 1;

    if (!force)
    {
        off_t i;
        char *stop_format_chars;

        if (option_stop_format_chars != NULL
            && strchr (option_stop_format_chars, t->str[0]) != NULL)
        {
            g_string_free (t, TRUE);
            return;
        }

        if (option_stop_format_chars == NULL || *option_stop_format_chars == '\0')
            stop_format_chars = g_strdup ("\t");
        else
            stop_format_chars = g_strconcat (option_stop_format_chars, "\t", (char *) NULL);

        for (i = 0; i < size - 1; i++)
            if (t->str[i] == '\n' && strchr (stop_format_chars, t->str[i + 1]) != NULL)
            {
                g_free (stop_format_chars);
                g_string_free (t, TRUE);
                return;
            }

        g_free (stop_format_chars);
    }

    t2 = (unsigned char *) g_string_free (t, FALSE);
#ifdef HAVE_CHARSET
    utf8 = edit->utf8;
#endif
    format_this (t2, q - p, indent, utf8);
    put_paragraph (edit, t2, p, indent, size);
    g_free ((char *) t2);

    /* Scroll left as much as possible to show the formatted paragraph */
    edit_scroll_left (edit, -edit->start_col);
}

/* --------------------------------------------------------------------------------------------- */
