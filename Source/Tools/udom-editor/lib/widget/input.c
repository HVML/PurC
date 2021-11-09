/*
   Widgets for the Midnight Commander

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

   Authors:
   Radek Doulik, 1994, 1995
   Miguel de Icaza, 1994, 1995
   Jakub Jelinek, 1995
   Andrej Borsenkow, 1996
   Norbert Warmuth, 1997
   Andrew Borodin <aborodin@vmail.ru>, 2009-2016

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

/** \file input.c
 *  \brief Source: WInput widget
 */

#include <config.h>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lib/global.h"

#include "lib/tty/tty.h"
#include "lib/tty/key.h"        /* XCTRL and ALT macros  */
#include "lib/fileloc.h"
#include "lib/skin.h"
#include "lib/strutil.h"
#include "lib/util.h"
#include "lib/widget.h"
#include "lib/event.h"          /* mc_event_raise() */
#include "lib/mcconfig.h"       /* mc_config_history_*() */

/*** global variables ****************************************************************************/

gboolean quote = FALSE;

const global_keymap_t *input_map = NULL;

/* Color styles for input widgets */
input_colors_t input_colors;

/*** file scope macro definitions ****************************************************************/

#define LARGE_HISTORY_BUTTON 1

#ifdef LARGE_HISTORY_BUTTON
#define HISTORY_BUTTON_WIDTH 3
#else
#define HISTORY_BUTTON_WIDTH 1
#endif

#define should_show_history_button(in) \
    (in->history.list != NULL && WIDGET (in)->cols > HISTORY_BUTTON_WIDTH * 2 + 1 \
         && WIDGET (in)->owner != NULL)

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* Input widgets have a global kill ring */
/* Pointer to killed data */
static char *kill_buffer = NULL;

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static size_t
get_history_length (GList * history)
{
    size_t len = 0;

    for (; history != NULL; history = g_list_previous (history))
        len++;

    return len;
}

/* --------------------------------------------------------------------------------------------- */

static void
draw_history_button (WInput * in)
{
    char c;
    gboolean disabled;

    if (g_list_next (in->history.current) == NULL)
        c = '^';
    else if (g_list_previous (in->history.current) == NULL)
        c = 'v';
    else
        c = '|';

    widget_gotoyx (in, 0, WIDGET (in)->cols - HISTORY_BUTTON_WIDTH);
    disabled = widget_get_state (WIDGET (in), WST_DISABLED);
    tty_setcolor (disabled ? DISABLED_COLOR : in->color[WINPUTC_HISTORY]);

#ifdef LARGE_HISTORY_BUTTON
    tty_print_string ("[ ]");
    widget_gotoyx (in, 0, WIDGET (in)->cols - HISTORY_BUTTON_WIDTH + 1);
#endif

    tty_print_char (c);
}

/* --------------------------------------------------------------------------------------------- */

static void
input_mark_cmd (WInput * in, gboolean mark)
{
    in->mark = mark ? in->point : -1;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
input_eval_marks (WInput * in, long *start_mark, long *end_mark)
{
    if (in->mark >= 0)
    {
        *start_mark = MIN (in->mark, in->point);
        *end_mark = MAX (in->mark, in->point);
        return TRUE;
    }

    *start_mark = *end_mark = -1;
    return FALSE;
}

/* --------------------------------------------------------------------------------------------- */

static void
delete_region (WInput * in, int x_first, int x_last)
{
    int first = MIN (x_first, x_last);
    int last = MAX (x_first, x_last);

    input_mark_cmd (in, FALSE);
    in->point = first;
    last = str_offset_to_pos (in->buffer, last);
    first = str_offset_to_pos (in->buffer, first);
    str_move (in->buffer + first, in->buffer + last);
    in->charpoint = 0;
    in->need_push = TRUE;
}

/* --------------------------------------------------------------------------------------------- */

static void
do_show_hist (WInput * in)
{
    size_t len;
    history_descriptor_t hd;

    len = get_history_length (in->history.list);

    history_descriptor_init (&hd, WIDGET (in)->y, WIDGET (in)->x, in->history.list,
                             g_list_position (in->history.list, in->history.list));
    history_show (&hd);

    /* in->history.list was destroyed in history_show().
     * Apply new history and current postition to avoid use-after-free. */
    in->history.list = hd.list;
    in->history.current = in->history.list;
    if (hd.text != NULL)
    {
        input_assign_text (in, hd.text);
        g_free (hd.text);
    }

    /* Has history cleaned up or not? */
    if (len != get_history_length (in->history.list))
        in->history.changed = TRUE;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Strip password from incomplete url (just user:pass@host without VFS prefix).
 *
 * @param url partial URL
 * @return newly allocated string without password
 */

static char *
input_history_strip_password (char *url)
{
    char *at, *delim, *colon;

    at = strrchr (url, '@');
    if (at == NULL)
        return g_strdup (url);

    /* TODO: handle ':' and '@' in password */

    delim = strstr (url, VFS_PATH_URL_DELIMITER);
    if (delim != NULL)
        colon = strchr (delim + strlen (VFS_PATH_URL_DELIMITER), ':');
    else
        colon = strchr (url, ':');

    /* if 'colon' before 'at', 'colon' delimits user and password: user:password@host */
    /* if 'colon' after 'at', 'colon' delimits host and port: user@host:port */
    if (colon != NULL && colon > at)
        colon = NULL;

    if (colon == NULL)
        return g_strdup (url);
    *colon = '\0';

    return g_strconcat (url, at, (char *) NULL);
}

/* --------------------------------------------------------------------------------------------- */

static void
push_history (WInput * in, const char *text)
{
    char *t;
    gboolean empty;

    if (text == NULL)
        return;

    t = g_strstrip (g_strdup (text));
    empty = *t == '\0';
    g_free (t);
    t = g_strdup (empty ? "" : text);

    if (!empty && in->history.name != NULL && in->strip_password)
    {
        /*
           We got string user:pass@host without any VFS prefixes
           and vfs_path_to_str_flags (t, VPF_STRIP_PASSWORD) doesn't work.
           Therefore we want to strip password in separate algorithm
         */
        char *url_with_stripped_password;

        url_with_stripped_password = input_history_strip_password (t);
        g_free (t);
        t = url_with_stripped_password;
    }

    if (in->history.list == NULL || in->history.list->data == NULL
        || strcmp (in->history.list->data, t) != 0 || in->history.changed)
    {
        in->history.list = list_append_unique (in->history.list, t);
        in->history.current = in->history.list;
        in->history.changed = TRUE;
    }
    else
        g_free (t);

    in->need_push = FALSE;
}

/* --------------------------------------------------------------------------------------------- */

static void
move_buffer_backward (WInput * in, int start, int end)
{
    int i, pos, len;
    int str_len;

    str_len = str_length (in->buffer);
    if (start >= str_len || end > str_len + 1)
        return;

    pos = str_offset_to_pos (in->buffer, start);
    len = str_offset_to_pos (in->buffer, end) - pos;

    for (i = pos; in->buffer[i + len - 1]; i++)
        in->buffer[i] = in->buffer[i + len];
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
insert_char (WInput * in, int c_code)
{
    int res;
    long m1, m2;

    if (input_eval_marks (in, &m1, &m2))
        delete_region (in, m1, m2);

    if (c_code == -1)
        return MSG_NOT_HANDLED;

    if (in->charpoint >= MB_LEN_MAX)
        return MSG_HANDLED;

    in->charbuf[in->charpoint] = c_code;
    in->charpoint++;

    res = str_is_valid_char (in->charbuf, in->charpoint);
    if (res < 0)
    {
        if (res != -2)
            in->charpoint = 0;  /* broken multibyte char, skip */
        return MSG_HANDLED;
    }

    in->need_push = TRUE;
    if (strlen (in->buffer) + 1 + in->charpoint >= in->current_max_size)
    {
        /* Expand the buffer */
        size_t new_length;
        char *narea;

        new_length = in->current_max_size + WIDGET (in)->cols + in->charpoint;
        narea = g_try_renew (char, in->buffer, new_length);
        if (narea != NULL)
        {
            in->buffer = narea;
            in->current_max_size = new_length;
        }
    }

    if (strlen (in->buffer) + in->charpoint < in->current_max_size)
    {
        size_t i;
        /* bytes from begin */
        size_t ins_point = str_offset_to_pos (in->buffer, in->point);
        /* move chars */
        size_t rest_bytes = strlen (in->buffer + ins_point);

        for (i = rest_bytes + 1; i > 0; i--)
            in->buffer[ins_point + i + in->charpoint - 1] = in->buffer[ins_point + i - 1];

        memcpy (in->buffer + ins_point, in->charbuf, in->charpoint);
        in->point++;
    }

    in->charpoint = 0;
    return MSG_HANDLED;
}

/* --------------------------------------------------------------------------------------------- */

static void
beginning_of_line (WInput * in)
{
    in->point = 0;
    in->charpoint = 0;
}

/* --------------------------------------------------------------------------------------------- */

static void
end_of_line (WInput * in)
{
    in->point = str_length (in->buffer);
    in->charpoint = 0;
}

/* --------------------------------------------------------------------------------------------- */

static void
backward_char (WInput * in)
{
    const char *act;

    act = in->buffer + str_offset_to_pos (in->buffer, in->point);
    if (in->point > 0)
        in->point -= str_cprev_noncomb_char (&act, in->buffer);
    in->charpoint = 0;
}

/* --------------------------------------------------------------------------------------------- */

static void
forward_char (WInput * in)
{
    const char *act;

    act = in->buffer + str_offset_to_pos (in->buffer, in->point);
    if (act[0] != '\0')
        in->point += str_cnext_noncomb_char (&act);
    in->charpoint = 0;
}

/* --------------------------------------------------------------------------------------------- */

static void
forward_word (WInput * in)
{
    const char *p;

    p = in->buffer + str_offset_to_pos (in->buffer, in->point);
    while (p[0] != '\0' && (str_isspace (p) || str_ispunct (p)))
    {
        str_cnext_char (&p);
        in->point++;
    }
    while (p[0] != '\0' && !str_isspace (p) && !str_ispunct (p))
    {
        str_cnext_char (&p);
        in->point++;
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
backward_word (WInput * in)
{
    const char *p;

    p = in->buffer + str_offset_to_pos (in->buffer, in->point);

    while (p != in->buffer)
    {
        const char *p_tmp;

        p_tmp = p;
        str_cprev_char (&p);
        if (!str_isspace (p) && !str_ispunct (p))
        {
            p = p_tmp;
            break;
        }
        in->point--;
    }
    while (p != in->buffer)
    {
        str_cprev_char (&p);
        if (str_isspace (p) || str_ispunct (p))
            break;

        in->point--;
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
backward_delete (WInput * in)
{
    const char *act = in->buffer + str_offset_to_pos (in->buffer, in->point);
    int start;

    if (in->point == 0)
        return;

    start = in->point - str_cprev_noncomb_char (&act, in->buffer);
    move_buffer_backward (in, start, in->point);
    in->charpoint = 0;
    in->need_push = TRUE;
    in->point = start;
}

/* --------------------------------------------------------------------------------------------- */

static void
delete_char (WInput * in)
{
    const char *act;
    int end = in->point;

    act = in->buffer + str_offset_to_pos (in->buffer, in->point);
    end += str_cnext_noncomb_char (&act);

    move_buffer_backward (in, in->point, end);
    in->charpoint = 0;
    in->need_push = TRUE;
}

/* --------------------------------------------------------------------------------------------- */

static void
copy_region (WInput * in, int x_first, int x_last)
{
    int first = MIN (x_first, x_last);
    int last = MAX (x_first, x_last);

    if (last == first)
    {
        /* Copy selected files to clipboard */
        mc_event_raise (MCEVENT_GROUP_FILEMANAGER, "panel_save_current_file_to_clip_file", NULL);
        /* try use external clipboard utility */
        mc_event_raise (MCEVENT_GROUP_CORE, "clipboard_file_to_ext_clip", NULL);
        return;
    }

    g_free (kill_buffer);

    first = str_offset_to_pos (in->buffer, first);
    last = str_offset_to_pos (in->buffer, last);

    kill_buffer = g_strndup (in->buffer + first, last - first);

    mc_event_raise (MCEVENT_GROUP_CORE, "clipboard_text_to_file", kill_buffer);
    /* try use external clipboard utility */
    mc_event_raise (MCEVENT_GROUP_CORE, "clipboard_file_to_ext_clip", NULL);
}

/* --------------------------------------------------------------------------------------------- */

static void
kill_word (WInput * in)
{
    int old_point = in->point;
    int new_point;

    forward_word (in);
    new_point = in->point;
    in->point = old_point;

    delete_region (in, old_point, new_point);
    in->need_push = TRUE;
    in->charpoint = 0;
}

/* --------------------------------------------------------------------------------------------- */

static void
back_kill_word (WInput * in)
{
    int old_point = in->point;
    int new_point;

    backward_word (in);
    new_point = in->point;
    in->point = old_point;

    delete_region (in, old_point, new_point);
    in->need_push = TRUE;
}

/* --------------------------------------------------------------------------------------------- */

static void
yank (WInput * in)
{
    if (kill_buffer != NULL)
    {
        char *p;

        in->charpoint = 0;
        for (p = kill_buffer; *p != '\0'; p++)
            insert_char (in, *p);
        in->charpoint = 0;
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
kill_line (WInput * in)
{
    int chp;

    chp = str_offset_to_pos (in->buffer, in->point);
    g_free (kill_buffer);
    kill_buffer = g_strdup (&in->buffer[chp]);
    in->buffer[chp] = '\0';
    in->charpoint = 0;
}

/* --------------------------------------------------------------------------------------------- */

static void
clear_line (WInput * in)
{
    in->need_push = TRUE;
    in->buffer[0] = '\0';
    in->point = 0;
    in->mark = -1;
    in->charpoint = 0;
}

/* --------------------------------------------------------------------------------------------- */

static void
ins_from_clip (WInput * in)
{
    char *p = NULL;
    ev_clipboard_text_from_file_t event_data;

    /* try use external clipboard utility */
    mc_event_raise (MCEVENT_GROUP_CORE, "clipboard_file_from_ext_clip", NULL);

    event_data.text = &p;
    mc_event_raise (MCEVENT_GROUP_CORE, "clipboard_text_from_file", &event_data);
    if (event_data.ret)
    {
        char *pp;

        for (pp = p; *pp != '\0'; pp++)
            insert_char (in, *pp);

        g_free (p);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
hist_prev (WInput * in)
{
    GList *prev;

    if (in->history.list == NULL)
        return;

    if (in->need_push)
        push_history (in, in->buffer);

    prev = g_list_previous (in->history.current);
    if (prev != NULL)
    {
        input_assign_text (in, (char *) prev->data);
        in->history.current = prev;
        in->history.changed = TRUE;
        in->need_push = FALSE;
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
hist_next (WInput * in)
{
    GList *next;

    if (in->need_push)
    {
        push_history (in, in->buffer);
        input_assign_text (in, "");
        return;
    }

    if (in->history.list == NULL)
        return;

    next = g_list_next (in->history.current);
    if (next == NULL)
    {
        input_assign_text (in, "");
        in->history.current = in->history.list;
    }
    else
    {
        input_assign_text (in, (char *) next->data);
        in->history.current = next;
        in->history.changed = TRUE;
        in->need_push = FALSE;
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
port_region_marked_for_delete (WInput * in)
{
    in->buffer[0] = '\0';
    in->point = 0;
    in->first = FALSE;
    in->charpoint = 0;
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
input_execute_cmd (WInput * in, long command)
{
    cb_ret_t res = MSG_HANDLED;

    switch (command)
    {
    case CK_MarkLeft:
    case CK_MarkRight:
    case CK_MarkToWordBegin:
    case CK_MarkToWordEnd:
    case CK_MarkToHome:
    case CK_MarkToEnd:
        /* a highlight command like shift-arrow */
        if (in->mark < 0)
        {
            input_mark_cmd (in, FALSE); /* clear */
            input_mark_cmd (in, TRUE);  /* marking on */
        }
        break;
    case CK_WordRight:
    case CK_WordLeft:
    case CK_Right:
    case CK_Left:
        if (in->mark >= 0)
            input_mark_cmd (in, FALSE);
        break;
    default:
        break;
    }

    switch (command)
    {
    case CK_Home:
    case CK_MarkToHome:
        beginning_of_line (in);
        break;
    case CK_End:
    case CK_MarkToEnd:
        end_of_line (in);
        break;
    case CK_Left:
    case CK_MarkLeft:
        backward_char (in);
        break;
    case CK_WordLeft:
    case CK_MarkToWordBegin:
        backward_word (in);
        break;
    case CK_Right:
    case CK_MarkRight:
        forward_char (in);
        break;
    case CK_WordRight:
    case CK_MarkToWordEnd:
        forward_word (in);
        break;
    case CK_BackSpace:
        {
            long m1, m2;

            if (input_eval_marks (in, &m1, &m2))
                delete_region (in, m1, m2);
            else
                backward_delete (in);
        }
        break;
    case CK_Delete:
        if (in->first)
            port_region_marked_for_delete (in);
        else
        {
            long m1, m2;

            if (input_eval_marks (in, &m1, &m2))
                delete_region (in, m1, m2);
            else
                delete_char (in);
        }
        break;
    case CK_DeleteToWordEnd:
        kill_word (in);
        break;
    case CK_DeleteToWordBegin:
        back_kill_word (in);
        break;
    case CK_Mark:
        input_mark_cmd (in, TRUE);
        break;
    case CK_Remove:
        delete_region (in, in->point, MAX (in->mark, 0));
        break;
    case CK_DeleteToEnd:
        kill_line (in);
        break;
    case CK_Clear:
        clear_line (in);
        break;
    case CK_Store:
        copy_region (in, MAX (in->mark, 0), in->point);
        break;
    case CK_Cut:
        {
            long m;

            m = MAX (in->mark, 0);
            copy_region (in, m, in->point);
            delete_region (in, in->point, m);
        }
        break;
    case CK_Yank:
        yank (in);
        break;
    case CK_Paste:
        ins_from_clip (in);
        break;
    case CK_HistoryPrev:
        hist_prev (in);
        break;
    case CK_HistoryNext:
        hist_next (in);
        break;
    case CK_History:
        do_show_hist (in);
        break;
    case CK_Complete:
        input_complete (in);
        break;
    default:
        res = MSG_NOT_HANDLED;
    }

    switch (command)
    {
    case CK_MarkLeft:
    case CK_MarkRight:
    case CK_MarkToWordBegin:
    case CK_MarkToWordEnd:
    case CK_MarkToHome:
    case CK_MarkToEnd:
        /* do nothing */
        break;
    default:
        in->mark = -1;
        break;
    }

    return res;
}

/* --------------------------------------------------------------------------------------------- */

/* "history_load" event handler */
static gboolean
input_load_history (const gchar * event_group_name, const gchar * event_name,
                    gpointer init_data, gpointer data)
{
    WInput *in = INPUT (init_data);
    ev_history_load_save_t *ev = (ev_history_load_save_t *) data;

    (void) event_group_name;
    (void) event_name;

    in->history.list = mc_config_history_load (ev->cfg, in->history.name);
    in->history.current = in->history.list;

    if (in->init_from_history)
    {
        const char *def_text = "";

        if (in->history.list != NULL && in->history.list->data != NULL)
            def_text = (const char *) in->history.list->data;

        input_assign_text (in, def_text);
    }

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

/* "history_save" event handler */
static gboolean
input_save_history (const gchar * event_group_name, const gchar * event_name,
                    gpointer init_data, gpointer data)
{
    WInput *in = INPUT (init_data);

    (void) event_group_name;
    (void) event_name;

    if (!in->is_password && (DIALOG (WIDGET (in)->owner)->ret_value != B_CANCEL))
    {
        ev_history_load_save_t *ev = (ev_history_load_save_t *) data;

        push_history (in, in->buffer);
        if (in->history.changed)
            mc_config_history_save (ev->cfg, in->history.name, in->history.list);
        in->history.changed = FALSE;
    }

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

static void
input_destroy (WInput * in)
{
    if (in == NULL)
    {
        fprintf (stderr, "Internal error: null Input *\n");
        exit (EXIT_FAILURE);
    }

    input_complete_free (in);

    /* clean history */
    if (in->history.list != NULL)
    {
        /* history is already saved before this moment */
        in->history.list = g_list_first (in->history.list);
        g_list_free_full (in->history.list, g_free);
    }
    g_free (in->history.name);
    g_free (in->buffer);
    MC_PTR_FREE (kill_buffer);
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Calculates the buffer index (aka "point") corresponding to some screen coordinate.
 */
static int
input_screen_to_point (const WInput * in, int x)
{
    x += in->term_first_shown;

    if (x < 0)
        return 0;

    if (x < str_term_width1 (in->buffer))
        return str_column_to_pos (in->buffer, x);

    return str_length (in->buffer);
}

/* --------------------------------------------------------------------------------------------- */

static void
input_mouse_callback (Widget * w, mouse_msg_t msg, mouse_event_t * event)
{
    /* save point between MSG_MOUSE_DOWN and MSG_MOUSE_DRAG */
    static int prev_point = 0;
    WInput *in = INPUT (w);

    switch (msg)
    {
    case MSG_MOUSE_DOWN:
        widget_select (w);

        if (event->x >= w->cols - HISTORY_BUTTON_WIDTH && should_show_history_button (in))
            do_show_hist (in);
        else
        {
            in->first = FALSE;
            input_mark_cmd (in, FALSE);
            input_set_point (in, input_screen_to_point (in, event->x));
            /* save point for the possible following MSG_MOUSE_DRAG action */
            prev_point = in->point;
        }
        break;

    case MSG_MOUSE_DRAG:
        /* start point: set marker using point before first MSG_MOUSE_DRAG action */
        if (in->mark < 0)
            in->mark = prev_point;

        input_set_point (in, input_screen_to_point (in, event->x));
        break;

    default:
        /* don't create highlight region of 0 length */
        if (in->mark == in->point)
            input_mark_cmd (in, FALSE);
        break;
    }
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/** Create new instance of WInput object.
  * @param y                    Y coordinate
  * @param x                    X coordinate
  * @param input_colors         Array of used colors
  * @param width                Widget width
  * @param def_text             Default text filled in widget
  * @param histname             Name of history
  * @param completion_flags     Flags for specify type of completions
  * @return                     WInput object
  */
WInput *
input_new (int y, int x, const int *colors, int width, const char *def_text,
           const char *histname, input_complete_t completion_flags)
{
    WInput *in;
    Widget *w;

    in = g_new (WInput, 1);
    w = WIDGET (in);
    widget_init (w, y, x, 1, width, input_callback, input_mouse_callback);
    w->options |= WOP_SELECTABLE | WOP_IS_INPUT | WOP_WANT_CURSOR;
    w->keymap = input_map;

    in->color = colors;
    in->first = TRUE;
    in->mark = -1;
    in->term_first_shown = 0;
    in->disable_update = 0;
    in->is_password = FALSE;
    in->strip_password = FALSE;

    /* in->buffer will be corrected in "history_load" event handler */
    in->current_max_size = width + 1;
    in->buffer = g_new0 (char, in->current_max_size);

    /* init completions before input_assign_text() call */
    in->completions = NULL;
    in->completion_flags = completion_flags;

    in->init_from_history = (def_text == INPUT_LAST_TEXT);

    if (in->init_from_history || def_text == NULL)
        def_text = "";

    input_assign_text (in, def_text);

    /* prepare to history setup */
    in->history.list = NULL;
    in->history.current = NULL;
    in->history.changed = FALSE;
    in->history.name = NULL;
    if ((histname != NULL) && (*histname != '\0'))
        in->history.name = g_strdup (histname);
    /* history will be loaded later */

    in->label = NULL;

    return in;
}

/* --------------------------------------------------------------------------------------------- */

cb_ret_t
input_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    WInput *in = INPUT (w);
    WDialog *h = DIALOG (w->owner);
    cb_ret_t v;

    switch (msg)
    {
    case MSG_INIT:
        /* subscribe to "history_load" event */
        mc_event_add (h->event_group, MCEVENT_HISTORY_LOAD, input_load_history, w, NULL);
        /* subscribe to "history_save" event */
        mc_event_add (h->event_group, MCEVENT_HISTORY_SAVE, input_save_history, w, NULL);
        if (in->label != NULL)
            widget_set_state (WIDGET (in->label), WST_DISABLED, widget_get_state (w, WST_DISABLED));
        return MSG_HANDLED;

    case MSG_KEY:
        if (parm == XCTRL ('q'))
        {
            quote = TRUE;
            v = input_handle_char (in, ascii_alpha_to_cntrl (tty_getch ()));
            quote = FALSE;
            return v;
        }

        /* Keys we want others to handle */
        if (parm == KEY_UP || parm == KEY_DOWN || parm == ESC_CHAR
            || parm == KEY_F (10) || parm == '\n')
            return MSG_NOT_HANDLED;

        /* When pasting multiline text, insert literal Enter */
        if ((parm & ~KEY_M_MASK) == '\n')
        {
            quote = TRUE;
            v = input_handle_char (in, '\n');
            quote = FALSE;
            return v;
        }

        return input_handle_char (in, parm);

    case MSG_ACTION:
        return input_execute_cmd (in, parm);

    case MSG_DRAW:
        input_update (in, FALSE);
        return MSG_HANDLED;

    case MSG_ENABLE:
    case MSG_DISABLE:
        if (in->label != NULL)
            widget_set_state (WIDGET (in->label), WST_DISABLED, msg == MSG_DISABLE);
        return MSG_HANDLED;

    case MSG_CURSOR:
        widget_gotoyx (in, 0, str_term_width2 (in->buffer, in->point) - in->term_first_shown);
        return MSG_HANDLED;

    case MSG_DESTROY:
        /* unsubscribe from "history_load" event */
        mc_event_del (h->event_group, MCEVENT_HISTORY_LOAD, input_load_history, w);
        /* unsubscribe from "history_save" event */
        mc_event_del (h->event_group, MCEVENT_HISTORY_SAVE, input_save_history, w);
        input_destroy (in);
        return MSG_HANDLED;

    default:
        return widget_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

void
input_set_default_colors (void)
{
    input_colors[WINPUTC_MAIN] = INPUT_COLOR;
    input_colors[WINPUTC_MARK] = INPUT_MARK_COLOR;
    input_colors[WINPUTC_UNCHANGED] = INPUT_UNCHANGED_COLOR;
    input_colors[WINPUTC_HISTORY] = INPUT_HISTORY_COLOR;
}

/* --------------------------------------------------------------------------------------------- */

cb_ret_t
input_handle_char (WInput * in, int key)
{
    cb_ret_t v;
    long command;

    if (quote)
    {
        input_complete_free (in);
        v = insert_char (in, key);
        input_update (in, TRUE);
        quote = FALSE;
        return v;
    }

    command = widget_lookup_key (WIDGET (in), key);
    if (command == CK_IgnoreKey)
    {
        if (key > 255)
            return MSG_NOT_HANDLED;
        if (in->first)
            port_region_marked_for_delete (in);
        input_complete_free (in);
        v = insert_char (in, key);
        input_update (in, TRUE);
    }
    else
    {
        gboolean keep_first;

        if (command != CK_Complete)
            input_complete_free (in);
        input_execute_cmd (in, command);
        v = MSG_HANDLED;
        /* if in->first == TRUE and history or completion window was cancelled,
           keep "first" state */
        keep_first = in->first && (command == CK_History || command == CK_Complete);
        input_update (in, !keep_first);
    }

    return v;
}

/* --------------------------------------------------------------------------------------------- */

void
input_assign_text (WInput * in, const char *text)
{
    Widget *w = WIDGET (in);
    size_t text_len, buffer_len;

    if (text == NULL)
        text = "";

    input_complete_free (in);
    in->mark = -1;
    in->need_push = TRUE;
    in->charpoint = 0;

    text_len = strlen (text);
    buffer_len = 1 + MAX ((size_t) w->cols, text_len);
    in->current_max_size = buffer_len;
    if (buffer_len > (size_t) w->cols)
        in->buffer = g_realloc (in->buffer, buffer_len);
    memmove (in->buffer, text, text_len + 1);
    in->point = str_length (in->buffer);
    input_update (in, TRUE);
}

/* --------------------------------------------------------------------------------------------- */

gboolean
input_is_empty (const WInput * in)
{
    return (in == NULL || in->buffer == NULL || in->buffer[0] == '\0');
}

/* --------------------------------------------------------------------------------------------- */

/* Inserts text in input line */
void
input_insert (WInput * in, const char *text, gboolean insert_extra_space)
{
    input_disable_update (in);
    while (*text != '\0')
        input_handle_char (in, (unsigned char) *text++);        /* unsigned extension char->int */
    if (insert_extra_space)
        input_handle_char (in, ' ');
    input_enable_update (in);
    input_update (in, TRUE);
}

/* --------------------------------------------------------------------------------------------- */

void
input_set_point (WInput * in, int pos)
{
    int max_pos;

    max_pos = str_length (in->buffer);
    pos = MIN (pos, max_pos);
    if (pos != in->point)
        input_complete_free (in);
    in->point = pos;
    in->charpoint = 0;
    input_update (in, TRUE);
}

/* --------------------------------------------------------------------------------------------- */

void
input_update (WInput * in, gboolean clear_first)
{
    Widget *w = WIDGET (in);
    int has_history = 0;
    int buf_len;
    const char *cp;
    int pw;

    if (in->disable_update != 0)
        return;

    /* don't draw widget not put into dialog */
    if (w->owner == NULL || !widget_get_state (WIDGET (w->owner), WST_ACTIVE))
        return;

    if (clear_first)
        in->first = FALSE;

    if (should_show_history_button (in))
        has_history = HISTORY_BUTTON_WIDTH;

    buf_len = str_length (in->buffer);

    /* Adjust the mark */
    in->mark = MIN (in->mark, buf_len);

    pw = str_term_width2 (in->buffer, in->point);

    /* Make the point visible */
    if ((pw < in->term_first_shown) || (pw >= in->term_first_shown + w->cols - has_history))
    {
        in->term_first_shown = pw - (w->cols / 3);
        if (in->term_first_shown < 0)
            in->term_first_shown = 0;
    }

    if (has_history != 0)
        draw_history_button (in);

    if (widget_get_state (w, WST_DISABLED))
        tty_setcolor (DISABLED_COLOR);
    else if (in->first)
        tty_setcolor (in->color[WINPUTC_UNCHANGED]);
    else
        tty_setcolor (in->color[WINPUTC_MAIN]);

    widget_gotoyx (in, 0, 0);

    if (!in->is_password)
    {
        if (in->mark < 0)
            tty_print_string (str_term_substring (in->buffer, in->term_first_shown,
                                                  w->cols - has_history));
        else
        {
            long m1, m2;

            if (input_eval_marks (in, &m1, &m2))
            {
                tty_setcolor (in->color[WINPUTC_MAIN]);
                cp = str_term_substring (in->buffer, in->term_first_shown, w->cols - has_history);
                tty_print_string (cp);
                tty_setcolor (in->color[WINPUTC_MARK]);
                if (m1 < in->term_first_shown)
                {
                    widget_gotoyx (in, 0, 0);
                    tty_print_string (str_term_substring
                                      (in->buffer, in->term_first_shown,
                                       m2 - in->term_first_shown));
                }
                else
                {
                    int sel_width, buf_width;

                    widget_gotoyx (in, 0, m1 - in->term_first_shown);
                    buf_width = str_term_width2 (in->buffer, m1);
                    sel_width =
                        MIN (m2 - m1, (w->cols - has_history) - (buf_width - in->term_first_shown));
                    tty_print_string (str_term_substring (in->buffer, m1, sel_width));
                }
            }
        }
    }
    else
    {
        int i;

        cp = str_term_substring (in->buffer, in->term_first_shown, w->cols - has_history);
        tty_setcolor (in->color[WINPUTC_MAIN]);
        for (i = 0; i < w->cols - has_history; i++)
        {
            if (i < (buf_len - in->term_first_shown) && cp[0] != '\0')
                tty_print_char ('*');
            else
                tty_print_char (' ');
            if (cp[0] != '\0')
                str_cnext_char (&cp);
        }
    }
}

/* --------------------------------------------------------------------------------------------- */

void
input_enable_update (WInput * in)
{
    in->disable_update--;
    input_update (in, FALSE);
}

/* --------------------------------------------------------------------------------------------- */

void
input_disable_update (WInput * in)
{
    in->disable_update++;
}

/* --------------------------------------------------------------------------------------------- */

/**
  *  Cleans the input line and adds the current text to the history
  *
  *  @param in the input line
  */
void
input_clean (WInput * in)
{
    push_history (in, in->buffer);
    in->need_push = TRUE;
    in->buffer[0] = '\0';
    in->point = 0;
    in->charpoint = 0;
    in->mark = -1;
    input_complete_free (in);
    input_update (in, FALSE);
}

/* --------------------------------------------------------------------------------------------- */
