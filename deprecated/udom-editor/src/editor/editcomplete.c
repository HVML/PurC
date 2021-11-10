/*
   Editor word completion engine

   Copyright (C) 2021
   Free Software Foundation, Inc.

   Written by:
   Andrew Borodin <aborodin@vmail.ru>, 2021

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

#include <ctype.h>              /* isspace() */
#include <string.h>

#include "lib/global.h"
#include "lib/search.h"
#include "lib/strutil.h"
#ifdef HAVE_CHARSET
#include "lib/charsets.h"       /* str_convert_to_input() */
#endif
#include "lib/tty/tty.h"        /* LINES, COLS */
#include "lib/widget.h"

#include "src/setup.h"          /* verbose */

#include "editwidget.h"
#include "edit-impl.h"
#include "editsearch.h"

#include "editcomplete.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/**
 * Get current word under cursor
 *
 * @param esm status message window
 * @param srch mc_search object
 * @param word_start start word position
 *
 * @return newly allocated string or NULL if no any words under cursor
 */

static GString *
edit_collect_completions_get_current_word (edit_search_status_msg_t * esm, mc_search_t * srch,
                                           off_t word_start)
{
    WEdit *edit = esm->edit;
    gsize len = 0;
    GString *temp = NULL;

    if (mc_search_run (srch, (void *) esm, word_start, edit->buffer.size, &len))
    {
        off_t i;

        for (i = 0; i < (off_t) len; i++)
        {
            int chr;

            chr = edit_buffer_get_byte (&edit->buffer, word_start + i);
            if (!isspace (chr))
            {
                if (temp == NULL)
                    temp = g_string_sized_new (len);

                g_string_append_c (temp, chr);
            }
        }
    }

    return temp;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * collect the possible completions from one buffer
 */

static void
edit_collect_completion_from_one_buffer (gboolean active_buffer, GQueue ** compl,
                                         mc_search_t * srch, edit_search_status_msg_t * esm,
                                         off_t word_start, gsize word_len, off_t last_byte,
                                         GString * current_word, int *max_width)
{
    GString *temp = NULL;
    gsize len = 0;
    off_t start = -1;

    while (mc_search_run (srch, (void *) esm, start + 1, last_byte, &len))
    {
        gsize i;
        int width;

        if (temp == NULL)
            temp = g_string_sized_new (8);
        else
            g_string_set_size (temp, 0);

        start = srch->normal_offset;

        /* add matched completion if not yet added */
        for (i = 0; i < len; i++)
        {
            int ch;

            ch = edit_buffer_get_byte (&esm->edit->buffer, start + i);
            if (isspace (ch))
                continue;

            /* skip current word */
            if (start + (off_t) i == word_start)
                break;

            g_string_append_c (temp, ch);
        }

        if (temp->len == 0)
            continue;

        if (current_word != NULL && g_string_equal (current_word, temp))
            continue;

        if (*compl == NULL)
            *compl = g_queue_new ();
        else
        {
            GList *l;

            for (l = g_queue_peek_head_link (*compl); l != NULL; l = g_list_next (l))
            {
                GString *s = (GString *) l->data;

                /* skip if already added */
                if (strncmp (s->str + word_len, temp->str + word_len,
                             MAX (len, s->len) - word_len) == 0)
                    break;
            }

            if (l != NULL)
            {
                /* resort completion in main buffer only:
                 * these completions must be at the top of list in the completion dialog */
                if (!active_buffer && l != g_queue_peek_tail_link (*compl))
                {
                    /* move to the end */
                    g_queue_unlink (*compl, l);
                    g_queue_push_tail_link (*compl, l);
                }

                continue;
            }
        }

#ifdef HAVE_CHARSET
        {
            GString *recoded;

            recoded = str_convert_to_display (temp->str);
            if (recoded->len != 0)
                mc_g_string_copy (temp, recoded);

            g_string_free (recoded, TRUE);
        }
#endif
        if (active_buffer)
            g_queue_push_tail (*compl, temp);
        else
            g_queue_push_head (*compl, temp);

        start += len;

        /* note the maximal length needed for the completion dialog */
        width = str_term_width1 (temp->str);
        *max_width = MAX (*max_width, width);

        temp = NULL;
    }

    if (temp != NULL)
        g_string_free (temp, TRUE);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * collect the possible completions from all buffers
 */

static GQueue *
edit_collect_completions (WEdit * edit, off_t word_start, gsize word_len,
                          const char *match_expr, int *max_width)
{
    GQueue *compl = NULL;
    mc_search_t *srch;
    off_t last_byte;
    GString *current_word;
    gboolean entire_file, all_files;
    edit_search_status_msg_t esm;

#ifdef HAVE_CHARSET
    srch = mc_search_new (match_expr, cp_source);
#else
    srch = mc_search_new (match_expr, NULL);
#endif
    if (srch == NULL)
        return NULL;

    entire_file =
        mc_config_get_bool (mc_global.main_config, CONFIG_APP_SECTION,
                            "editor_wordcompletion_collect_entire_file", FALSE);

    last_byte = entire_file ? edit->buffer.size : word_start;

    srch->search_type = MC_SEARCH_T_REGEX;
    srch->is_case_sensitive = TRUE;
    srch->search_fn = edit_search_cmd_callback;
    srch->update_fn = edit_search_update_callback;

    esm.first = TRUE;
    esm.edit = edit;
    esm.offset = entire_file ? 0 : word_start;

    status_msg_init (STATUS_MSG (&esm), _("Collect completions"), 1.0, simple_status_msg_init_cb,
                     edit_search_status_update_cb, NULL);

    current_word = edit_collect_completions_get_current_word (&esm, srch, word_start);

    *max_width = 0;

    /* collect completions from current buffer at first */
    edit_collect_completion_from_one_buffer (TRUE, &compl, srch, &esm, word_start, word_len,
                                             last_byte, current_word, max_width);

    /* collect completions from other buffers */
    all_files =
        mc_config_get_bool (mc_global.main_config, CONFIG_APP_SECTION,
                            "editor_wordcompletion_collect_all_files", TRUE);
    if (all_files)
    {
        const WGroup *owner = CONST_GROUP (CONST_WIDGET (edit)->owner);
        gboolean saved_verbose;
        GList *w;

        /* don't show incorrect percentage in edit_search_status_update_cb() */
        saved_verbose = verbose;
        verbose = FALSE;

        for (w = owner->widgets; w != NULL; w = g_list_next (w))
        {
            Widget *ww = WIDGET (w->data);
            WEdit *e;

            if (!edit_widget_is_editor (ww))
                continue;

            e = (WEdit *) ww;

            if (e == edit)
                continue;

            /* search in entire file */
            word_start = 0;
            last_byte = e->buffer.size;
            esm.edit = e;
            esm.offset = 0;

            edit_collect_completion_from_one_buffer (FALSE, &compl, srch, &esm, word_start,
                                                     word_len, last_byte, current_word, max_width);
        }

        verbose = saved_verbose;
    }

    status_msg_deinit (STATUS_MSG (&esm));
    mc_search_free (srch);
    if (current_word != NULL)
        g_string_free (current_word, TRUE);

    return compl;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Insert autocompleted word into editor.
 *
 * @param edit       editor object
 * @param completion word for completion
 * @param word_len   offset from beginning for insert
 */

static void
edit_complete_word_insert_recoded_completion (WEdit * edit, char *completion, gsize word_len)
{
#ifdef HAVE_CHARSET
    GString *temp;

    temp = str_convert_to_input (completion);

    for (completion = temp->str + word_len; *completion != '\0'; completion++)
        edit_insert (edit, *completion);
    g_string_free (temp, TRUE);
#else
    for (completion += word_len; *completion != '\0'; completion++)
        edit_insert (edit, *completion);
#endif
}

/* --------------------------------------------------------------------------------------------- */

static void
edit_completion_string_free (gpointer data)
{
    g_string_free ((GString *) data, TRUE);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */
/* let the user select its preferred completion */

/* Public function for unit tests */
char *
edit_completion_dialog_show (const WEdit * edit, GQueue * compl, int max_width)
{
    const Widget *we = CONST_WIDGET (edit);
    int start_x, start_y, offset;
    char *curr = NULL;
    WDialog *compl_dlg;
    WListbox *compl_list;
    int compl_dlg_h;            /* completion dialog height */
    int compl_dlg_w;            /* completion dialog width */
    GList *i;

    /* calculate the dialog metrics */
    compl_dlg_h = g_queue_get_length (compl) + 2;
    compl_dlg_w = max_width + 4;
    start_x = we->x + edit->curs_col + edit->start_col + EDIT_TEXT_HORIZONTAL_OFFSET +
        (edit->fullscreen ? 0 : 1) + option_line_state_width;
    start_y = we->y + edit->curs_row + EDIT_TEXT_VERTICAL_OFFSET + (edit->fullscreen ? 0 : 1) + 1;

    if (start_x < 0)
        start_x = 0;
    if (start_x < we->x + 1)
        start_x = we->x + 1 + option_line_state_width;
    if (compl_dlg_w > COLS)
        compl_dlg_w = COLS;
    if (compl_dlg_h > LINES - 2)
        compl_dlg_h = LINES - 2;

    offset = start_x + compl_dlg_w - COLS;
    if (offset > 0)
        start_x -= offset;
    offset = start_y + compl_dlg_h - LINES;
    if (offset > 0)
        start_y -= offset;

    /* create the dialog */
    compl_dlg =
        dlg_create (TRUE, start_y, start_x, compl_dlg_h, compl_dlg_w, WPOS_KEEP_DEFAULT, TRUE,
                    dialog_colors, NULL, NULL, "[Completion]", NULL);

    /* create the listbox */
    compl_list = listbox_new (1, 1, compl_dlg_h - 2, compl_dlg_w - 2, FALSE, NULL);

    /* fill the listbox with the completions in the reverse order */
    for (i = g_queue_peek_tail_link (compl); i != NULL; i = g_list_previous (i))
        listbox_add_item (compl_list, LISTBOX_APPEND_AT_END, 0, ((GString *) i->data)->str, NULL,
                          FALSE);

    group_add_widget (GROUP (compl_dlg), compl_list);

    /* pop up the dialog and apply the chosen completion */
    if (dlg_run (compl_dlg) == B_ENTER)
    {
        listbox_get_current (compl_list, &curr, NULL);
        curr = g_strdup (curr);
    }

    /* destroy dialog before return */
    widget_destroy (WIDGET (compl_dlg));

    return curr;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Complete current word using regular expression search
 * backwards beginning at the current cursor position.
 */

void
edit_complete_word_cmd (WEdit * edit)
{
    off_t word_start = 0;
    gsize word_len = 0;
    GString *match_expr;
    gsize i;
    GQueue *compl;              /* completions: list of GString* */
    int max_width;

    /* search start of word to be completed */
    if (!edit_buffer_find_word_start (&edit->buffer, &word_start, &word_len))
        return;

    /* prepare match expression */
    /* match_expr = g_strdup_printf ("\\b%.*s[a-zA-Z_0-9]+", word_len, bufpos); */
    match_expr = g_string_new ("(^|\\s+|\\b)");
    for (i = 0; i < word_len; i++)
        g_string_append_c (match_expr, edit_buffer_get_byte (&edit->buffer, word_start + i));
    g_string_append (match_expr,
                     "[^\\s\\.=\\+\\[\\]\\(\\)\\,\\;\\:\\\"\\'\\-\\?\\/\\|\\\\\\{\\}\\*\\&\\^\\%%\\$#@\\!]+");

    /* collect possible completions */
    compl = edit_collect_completions (edit, word_start, word_len, match_expr->str, &max_width);

    g_string_free (match_expr, TRUE);

    if (compl == NULL)
        return;

    if (g_queue_get_length (compl) == 1)
    {
        /* insert completed word if there is only one match */

        GString *curr_compl;

        curr_compl = (GString *) g_queue_peek_head (compl);
        edit_complete_word_insert_recoded_completion (edit, curr_compl->str, word_len);
    }
    else
    {
        /* more than one possible completion => ask the user */

        char *curr_compl;

        /* let the user select the preferred completion */
        curr_compl = edit_completion_dialog_show (edit, compl, max_width);
        if (curr_compl != NULL)
        {
            edit_complete_word_insert_recoded_completion (edit, curr_compl, word_len);
            g_free (curr_compl);
        }
    }

    g_queue_free_full (compl, edit_completion_string_free);
}

/* --------------------------------------------------------------------------------------------- */
