/*
   Editor text keep buffer.

   Copyright (C) 2013-2021
   Free Software Foundation, Inc.

   Written by:
   Andrew Borodin <aborodin@vmail.ru> 2013

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
 *  \brief Source: editor text keep buffer.
 *  \author Andrew Borodin
 *  \date 2013
 */

#include <config.h>

#include <ctype.h>              /* isdigit() */
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "lib/global.h"

#include "lib/vfs/vfs.h"

#include "edit-impl.h"
#include "editbuffer.h"

/* --------------------------------------------------------------------------------------------- */
/*-
 *
 * here's a quick sketch of the layout: (don't run this through indent.)
 *
 *                                       |
 * \0\0\0\0\0m e _ f i l e . \nf i n . \n|T h i s _ i s _ s o\0\0\0\0\0\0\0\0\0
 * ______________________________________|______________________________________
 *                                       |
 * ...  |  b2[2]   |  b2[1]   |  b2[0]   |  b1[0]   |  b1[1]   |  b1[2]   | ...
 *      |->        |->        |->        |->        |->        |->        |
 *                                       |
 *           _<------------------------->|<----------------->_
 *                       curs2           |       curs1
 *           ^                           |                   ^
 *           |                          ^|^                  |
 *         cursor                       |||                cursor
 *                                      |||
 *                              file end|||file beginning
 *                                       |
 *                                       |
 *
 *           _
 * This_is_some_file
 * fin.
 *
 *
 * This is called a "gap buffer".
 * See also:
 * http://en.wikipedia.org/wiki/Gap_buffer
 * http://stackoverflow.com/questions/4199694/data-structure-for-text-editor
 */

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*
 * The editor keeps data in two arrays of buffers.
 * All buffers have the same size, which must be a power of 2.
 */

/* Configurable: log2 of the buffer size in bytes */
#ifndef S_EDIT_BUF_SIZE
#define S_EDIT_BUF_SIZE 16
#endif

/* Size of the buffer */
#define EDIT_BUF_SIZE (((off_t) 1) << S_EDIT_BUF_SIZE)

/* Buffer mask (used to find cursor position relative to the buffer) */
#define M_EDIT_BUF_SIZE (EDIT_BUF_SIZE - 1)

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */
/**
  * Get pointer to byte at specified index
  *
  * @param buf pointer to editor buffer
  * @param byte_index byte index
  *
  * @return NULL if byte_index is negative or larger than file size; pointer to byte otherwise.
  */
static char *
edit_buffer_get_byte_ptr (const edit_buffer_t * buf, off_t byte_index)
{
    void *b;

    if (byte_index >= (buf->curs1 + buf->curs2) || byte_index < 0)
        return NULL;

    if (byte_index >= buf->curs1)
    {
        off_t p;

        p = buf->curs1 + buf->curs2 - byte_index - 1;
        b = g_ptr_array_index (buf->b2, p >> S_EDIT_BUF_SIZE);
        return (char *) b + EDIT_BUF_SIZE - 1 - (p & M_EDIT_BUF_SIZE);
    }

    b = g_ptr_array_index (buf->b1, byte_index >> S_EDIT_BUF_SIZE);
    return (char *) b + (byte_index & M_EDIT_BUF_SIZE);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */
/**
 * Initialize editor buffers.
 *
 * @param buf pointer to editor buffer
 */

void
edit_buffer_init (edit_buffer_t * buf, off_t size)
{
    buf->b1 = g_ptr_array_sized_new (32);
    buf->b2 = g_ptr_array_sized_new (32);

    buf->curs1 = 0;
    buf->curs2 = 0;

    buf->size = size;
    buf->lines = 0;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Clean editor buffers.
 *
 * @param buf pointer to editor buffer
 */

void
edit_buffer_clean (edit_buffer_t * buf)
{
    if (buf->b1 != NULL)
    {
        g_ptr_array_foreach (buf->b1, (GFunc) g_free, NULL);
        g_ptr_array_free (buf->b1, TRUE);
    }

    if (buf->b2 != NULL)
    {
        g_ptr_array_foreach (buf->b2, (GFunc) g_free, NULL);
        g_ptr_array_free (buf->b2, TRUE);
    }
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Get byte at specified index
  *
  * @param buf pointer to editor buffer
  * @param byte_index byte index
  *
  * @return '\n' if byte_index is negative or larger than file size; byte at byte_index otherwise.
  */

int
edit_buffer_get_byte (const edit_buffer_t * buf, off_t byte_index)
{
    char *p;

    p = edit_buffer_get_byte_ptr (buf, byte_index);

    return (p != NULL) ? *(unsigned char *) p : '\n';
}

/* --------------------------------------------------------------------------------------------- */

#ifdef HAVE_CHARSET
/**
  * Get utf-8 symbol at specified index
  *
  * @param buf pointer to editor buffer
  * @param byte_index byte index
  * @param char_length length of returned symbol
  *
  * @return '\n' if byte_index is negative or larger than file size;
  *         0 if utf-8 symbol at specified index is invalid;
  *         utf-8 symbol otherwise
  */

int
edit_buffer_get_utf (const edit_buffer_t * buf, off_t byte_index, int *char_length)
{
    gchar *str = NULL;
    gunichar res;
    gunichar ch;
    gchar *next_ch = NULL;

    if (byte_index >= (buf->curs1 + buf->curs2) || byte_index < 0)
    {
        *char_length = 0;
        return '\n';
    }

    str = edit_buffer_get_byte_ptr (buf, byte_index);
    if (str == NULL)
    {
        *char_length = 0;
        return 0;
    }

    res = g_utf8_get_char_validated (str, -1);
    if (res == (gunichar) (-2) || res == (gunichar) (-1))
    {
        /* Retry with explicit bytes to make sure it's not a buffer boundary */
        size_t i;
        gchar utf8_buf[UTF8_CHAR_LEN + 1];

        for (i = 0; i < UTF8_CHAR_LEN; i++)
            utf8_buf[i] = edit_buffer_get_byte (buf, byte_index + i);
        utf8_buf[i] = '\0';
        res = g_utf8_get_char_validated (utf8_buf, -1);
    }

    if (res == (gunichar) (-2) || res == (gunichar) (-1))
    {
        ch = *str;
        *char_length = 0;
    }
    else
    {
        ch = res;
        /* Calculate UTF-8 char length */
        next_ch = g_utf8_next_char (str);
        *char_length = next_ch - str;
    }

    return (int) ch;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Get utf-8 symbol before specified index
  *
  * @param buf pointer to editor buffer
  * @param byte_index byte index
  * @param char_length length of returned symbol
  *
  * @return 0 if byte_index is negative or larger than file size;
  *         1-byte value before specified index if utf-8 symbol before specified index is invalid;
  *         utf-8 symbol otherwise
  */

int
edit_buffer_get_prev_utf (const edit_buffer_t * buf, off_t byte_index, int *char_length)
{
    size_t i;
    gchar utf8_buf[3 * UTF8_CHAR_LEN + 1];
    gchar *str;
    gchar *cursor_buf_ptr;
    gunichar res;

    if (byte_index > (buf->curs1 + buf->curs2) || byte_index <= 0)
    {
        *char_length = 0;
        return 0;
    }

    for (i = 0; i < (3 * UTF8_CHAR_LEN); i++)
        utf8_buf[i] = edit_buffer_get_byte (buf, byte_index + i - (2 * UTF8_CHAR_LEN));
    utf8_buf[i] = '\0';

    cursor_buf_ptr = utf8_buf + (2 * UTF8_CHAR_LEN);
    str = g_utf8_find_prev_char (utf8_buf, cursor_buf_ptr);

    if (str == NULL || g_utf8_next_char (str) != cursor_buf_ptr)
    {
        *char_length = 1;
        return *(cursor_buf_ptr - 1);
    }

    res = g_utf8_get_char_validated (str, -1);
    if (res == (gunichar) (-2) || res == (gunichar) (-1))
    {
        *char_length = 1;
        return *(cursor_buf_ptr - 1);
    }

    *char_length = cursor_buf_ptr - str;
    return (int) res;
}
#endif /* HAVE_CHARSET */

/* --------------------------------------------------------------------------------------------- */
/**
 * Count lines in editor buffer.
 *
 * @param buf editor buffer
 * @param first start byte offset
 * @param last finish byte offset
 *
 * @return line numbers between "first" and "last" bytes
 */

long
edit_buffer_count_lines (const edit_buffer_t * buf, off_t first, off_t last)
{
    long lines = 0;

    first = MAX (first, 0);
    last = MIN (last, buf->size);

    while (first < last)
        if (edit_buffer_get_byte (buf, first++) == '\n')
            lines++;

    return lines;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Get "begin-of-line" offset of line contained specified byte offset
 *
 * @param buf editor buffer
 * @param current byte offset
 *
 * @return index of first char of line
 */

off_t
edit_buffer_get_bol (const edit_buffer_t * buf, off_t current)
{
    if (current <= 0)
        return 0;

    for (; edit_buffer_get_byte (buf, current - 1) != '\n'; current--)
        ;

    return current;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Get "end-of-line" offset of line contained specified byte offset
 *
 * @param buf editor buffer
 * @param current byte offset
 *
 * @return index of last char of line + 1
 */

off_t
edit_buffer_get_eol (const edit_buffer_t * buf, off_t current)
{
    if (current >= buf->size)
        return buf->size;

    for (; edit_buffer_get_byte (buf, current) != '\n'; current++)
        ;

    return current;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Get word from specified offset.
 *
 * @param buf editor buffer
 * @param current start_pos offset
 * @param start actual start word ofset
 * @param cut 
 *
 * @return word as newly allocated object
 */

GString *
edit_buffer_get_word_from_pos (const edit_buffer_t * buf, off_t start_pos, off_t * start,
                               gsize * cut)
{
    off_t word_start;
    gsize cut_len = 0;
    GString *match_expr;
    int c1, c2;

    for (word_start = start_pos; word_start != 0; word_start--, cut_len++)
    {
        c1 = edit_buffer_get_byte (buf, word_start);
        c2 = edit_buffer_get_byte (buf, word_start - 1);

        if (is_break_char (c1) != is_break_char (c2) || c1 == '\n' || c2 == '\n')
            break;
    }

    match_expr = g_string_sized_new (16);

    do
    {
        c1 = edit_buffer_get_byte (buf, word_start + match_expr->len);
        c2 = edit_buffer_get_byte (buf, word_start + match_expr->len + 1);
        g_string_append_c (match_expr, c1);
    }
    while (!(is_break_char (c1) != is_break_char (c2) || c1 == '\n' || c2 == '\n'));

    *start = word_start;
    *cut = cut_len;

    return match_expr;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Find first character of current word
 *
 * @param buf editor buffer
 * @param word_start position of first character of current word
 * @param word_len length of current word
 *
 * @return TRUE if first character of word is found and this character is not 1) a digit and
 *         2) a begin of file, FALSE otherwise
 */

gboolean
edit_buffer_find_word_start (const edit_buffer_t * buf, off_t * word_start, gsize * word_len)
{
    int c;
    off_t i;

    /* return if at begin of file */
    if (buf->curs1 <= 0)
        return FALSE;

    c = edit_buffer_get_previous_byte (buf);
    /* return if not at end or in word */
    if (is_break_char (c))
        return FALSE;

    /* search start of word */
    for (i = 1;; i++)
    {
        int last;

        last = c;
        c = edit_buffer_get_byte (buf, buf->curs1 - i - 1);

        if (is_break_char (c))
        {
            /* return if word starts with digit */
            if (isdigit (last))
                return FALSE;

            break;
        }
    }

    /* success */
    *word_start = buf->curs1 - i;       /* start found */
    *word_len = (gsize) i;

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Basic low level single character buffer alterations and movements at the cursor: insert character
 * at the cursor position and move right.
 *
 * @param buf pointer to editor buffer
 * @param c character to insert
 */

void
edit_buffer_insert (edit_buffer_t * buf, int c)
{
    void *b;
    off_t i;

    i = buf->curs1 & M_EDIT_BUF_SIZE;

    /* add a new buffer if we've reached the end of the last one */
    if (i == 0)
        g_ptr_array_add (buf->b1, g_malloc0 (EDIT_BUF_SIZE));

    /* perform the insertion */
    b = g_ptr_array_index (buf->b1, buf->curs1 >> S_EDIT_BUF_SIZE);
    *((unsigned char *) b + i) = (unsigned char) c;

    /* update cursor position */
    buf->curs1++;

    /* update file length */
    buf->size++;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Basic low level single character buffer alterations and movements at the cursor: insert character
 * at the cursor position and move left.
 *
 * @param buf pointer to editor buffer
 * @param c character to insert
 */

void
edit_buffer_insert_ahead (edit_buffer_t * buf, int c)
{
    void *b;
    off_t i;

    i = buf->curs2 & M_EDIT_BUF_SIZE;

    /* add a new buffer if we've reached the end of the last one */
    if (i == 0)
        g_ptr_array_add (buf->b2, g_malloc0 (EDIT_BUF_SIZE));

    /* perform the insertion */
    b = g_ptr_array_index (buf->b2, buf->curs2 >> S_EDIT_BUF_SIZE);
    *((unsigned char *) b + EDIT_BUF_SIZE - 1 - i) = (unsigned char) c;

    /* update cursor position */
    buf->curs2++;

    /* update file length */
    buf->size++;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Basic low level single character buffer alterations and movements at the cursor: delete character
 * at the cursor position.
 *
 * @param buf pointer to editor buffer
 * @param c character to insert
 */

int
edit_buffer_delete (edit_buffer_t * buf)
{
    void *b;
    unsigned char c;
    off_t prev;
    off_t i;

    prev = buf->curs2 - 1;

    b = g_ptr_array_index (buf->b2, prev >> S_EDIT_BUF_SIZE);
    i = prev & M_EDIT_BUF_SIZE;
    c = *((unsigned char *) b + EDIT_BUF_SIZE - 1 - i);

    if (i == 0)
    {
        guint j;

        j = buf->b2->len - 1;
        b = g_ptr_array_index (buf->b2, j);
        g_ptr_array_remove_index (buf->b2, j);
        g_free (b);
    }

    buf->curs2 = prev;

    /* update file length */
    buf->size--;

    return c;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Basic low level single character buffer alterations and movements at the cursor: delete character
 * before the cursor position and move left.
 *
 * @param buf pointer to editor buffer
 * @param c character to insert
 */

int
edit_buffer_backspace (edit_buffer_t * buf)
{
    void *b;
    unsigned char c;
    off_t prev;
    off_t i;

    prev = buf->curs1 - 1;

    b = g_ptr_array_index (buf->b1, prev >> S_EDIT_BUF_SIZE);
    i = prev & M_EDIT_BUF_SIZE;
    c = *((unsigned char *) b + i);

    if (i == 0)
    {
        guint j;

        j = buf->b1->len - 1;
        b = g_ptr_array_index (buf->b1, j);
        g_ptr_array_remove_index (buf->b1, j);
        g_free (b);
    }

    buf->curs1 = prev;

    /* update file length */
    buf->size--;

    return c;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Calculate forward offset with specified number of lines.
 *
 * @param buf editor buffer
 * @param current current offset
 * @param lines number of lines to move forward
 * @param upto offset to count lines between current and upto.
 *
 * @return If lines is zero returns the count of lines from current to upto.
 *         If upto is zero returns offset of lines forward current.
 *         Else returns forward offset with specified number of lines
 */

off_t
edit_buffer_get_forward_offset (const edit_buffer_t * buf, off_t current, long lines, off_t upto)
{
    if (upto != 0)
        return (off_t) edit_buffer_count_lines (buf, current, upto);

    lines = MAX (lines, 0);

    while (lines-- != 0)
    {
        long next;

        next = edit_buffer_get_eol (buf, current) + 1;
        if (next > buf->size)
            break;
        current = next;
    }

    return current;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Calculate backward offset with specified number of lines.
 *
 * @param buf editor buffer
 * @param current current offset
 * @param lines number of lines to move backward
 *
 * @return backward offset with specified number of lines.
 */

off_t
edit_buffer_get_backward_offset (const edit_buffer_t * buf, off_t current, long lines)
{
    lines = MAX (lines, 0);
    current = edit_buffer_get_bol (buf, current);

    while (lines-- != 0 && current != 0)
        current = edit_buffer_get_bol (buf, current - 1);

    return current;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Load file into editor buffer
 *
 * @param buf pointer to editor buffer
 * @param fd file descriptor
 * @param size file size
 *
 * @return number of read bytes
 */

off_t
edit_buffer_read_file (edit_buffer_t * buf, int fd, off_t size,
                       edit_buffer_read_file_status_msg_t * sm, gboolean * aborted)
{
    off_t ret = 0;
    off_t i, j;
    off_t data_size;
    void *b;
    status_msg_t *s = STATUS_MSG (sm);
    unsigned short update_cnt = 0;

    *aborted = FALSE;

    buf->lines = 0;
    buf->curs2 = size;
    i = buf->curs2 >> S_EDIT_BUF_SIZE;

    /* fill last part of b2 */
    data_size = buf->curs2 & M_EDIT_BUF_SIZE;
    if (data_size != 0)
    {
        b = g_malloc0 (EDIT_BUF_SIZE);
        g_ptr_array_add (buf->b2, b);
        b = (char *) b + EDIT_BUF_SIZE - data_size;
        ret = mc_read (fd, b, data_size);

        /* count lines */
        for (j = 0; j < ret; j++)
            if (*((char *) b + j) == '\n')
                buf->lines++;

        if (ret < 0 || ret != data_size)
            return ret;
    }

    /* fulfill other parts of b2 from end to begin */
    data_size = EDIT_BUF_SIZE;
    for (--i; i >= 0; i--)
    {
        off_t sz;

        b = g_malloc0 (data_size);
        g_ptr_array_add (buf->b2, b);
        sz = mc_read (fd, b, data_size);
        if (sz >= 0)
            ret += sz;

        /* count lines */
        for (j = 0; j < sz; j++)
            if (*((char *) b + j) == '\n')
                buf->lines++;

        if (s != NULL && s->update != NULL)
        {
            update_cnt = (update_cnt + 1) & 0xf;
            if (update_cnt == 0)
            {
                /* FIXME: overcare */
                if (sm->buf == NULL)
                    sm->buf = buf;

                sm->loaded = ret;
                if (s->update (s) == B_CANCEL)
                {
                    *aborted = TRUE;
                    return (-1);
                }
            }
        }

        if (sz != data_size)
            break;
    }

    /* reverse buffer */
    for (i = 0; i < (off_t) buf->b2->len / 2; i++)
    {
        void **b1, **b2;

        b1 = &g_ptr_array_index (buf->b2, i);
        b2 = &g_ptr_array_index (buf->b2, buf->b2->len - 1 - i);

        b = *b1;
        *b1 = *b2;
        *b2 = b;

        if (s != NULL && s->update != NULL)
        {
            update_cnt = (update_cnt + 1) & 0xf;
            if (update_cnt == 0)
            {
                sm->loaded = ret;
                if (s->update (s) == B_CANCEL)
                {
                    *aborted = TRUE;
                    return (-1);
                }
            }
        }
    }

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Write editor buffer content to file
 *
 * @param buf pointer to editor buffer
 * @param fd file descriptor
 *
 * @return number of written bytes
 */

off_t
edit_buffer_write_file (edit_buffer_t * buf, int fd)
{
    off_t ret = 0;
    off_t i;
    off_t data_size, sz;
    void *b;

    /* write all fulfilled parts of b1 from begin to end */
    if (buf->b1->len != 0)
    {
        data_size = EDIT_BUF_SIZE;
        for (i = 0; i < (off_t) buf->b1->len - 1; i++)
        {
            b = g_ptr_array_index (buf->b1, i);
            sz = mc_write (fd, b, data_size);
            if (sz >= 0)
                ret += sz;
            else if (i == 0)
                ret = sz;
            if (sz != data_size)
                return ret;
        }

        /* write last partially filled part of b1 */
        data_size = ((buf->curs1 - 1) & M_EDIT_BUF_SIZE) + 1;
        b = g_ptr_array_index (buf->b1, i);
        sz = mc_write (fd, b, data_size);
        if (sz >= 0)
            ret += sz;
        if (sz != data_size)
            return ret;
    }

    /* write b2 from end to begin, if b2 contains some data */
    if (buf->b2->len != 0)
    {
        /* write last partially filled part of b2 */
        i = buf->b2->len - 1;
        b = g_ptr_array_index (buf->b2, i);
        data_size = ((buf->curs2 - 1) & M_EDIT_BUF_SIZE) + 1;
        sz = mc_write (fd, (char *) b + EDIT_BUF_SIZE - data_size, data_size);
        if (sz >= 0)
            ret += sz;

        if (sz == data_size)
        {
            /* write other fulfilled parts of b2 from end to begin */
            data_size = EDIT_BUF_SIZE;
            while (--i >= 0)
            {
                b = g_ptr_array_index (buf->b2, i);
                sz = mc_write (fd, b, data_size);
                if (sz >= 0)
                    ret += sz;
                if (sz != data_size)
                    break;
            }
        }
    }

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Calculate percentage of specified character offset
 *
 * @param buf pointer to editor buffer
 * @param p character offset
 *
 * @return percentage of specified character offset
 */

int
edit_buffer_calc_percent (const edit_buffer_t * buf, off_t offset)
{
    int percent;

    if (buf->size == 0)
        percent = 0;
    else if (offset >= buf->size)
        percent = 100;
    else if (offset > (INT_MAX / 100))
        percent = offset / (buf->size / 100);
    else
        percent = offset * 100 / buf->size;

    return percent;
}

/* --------------------------------------------------------------------------------------------- */
