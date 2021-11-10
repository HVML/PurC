/*
   Search & replace engine of MCEditor.

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

#include <assert.h>

#include "lib/global.h"
#include "lib/search.h"
#include "lib/mcconfig.h"       /* mc_config_history_get */
#include "lib/util.h"
#include "lib/strutil.h"
#include "lib/widget.h"
#include "lib/skin.h"           /* BOOK_MARK_FOUND_COLOR */

#include "src/history.h"        /* MC_HISTORY_SHARED_SEARCH */
#include "src/setup.h"          /* verbose */

#include "edit-impl.h"
#include "editwidget.h"

#include "editsearch.h"

/*** global variables ****************************************************************************/

edit_search_options_t edit_search_options = {
    .type = MC_SEARCH_T_NORMAL,
    .case_sens = FALSE,
    .backwards = FALSE,
    .only_in_selection = FALSE,
    .whole_words = FALSE,
    .all_codepages = FALSE
};

/*** file scope macro definitions ****************************************************************/

#define B_REPLACE_ALL (B_USER+1)
#define B_REPLACE_ONE (B_USER+2)
#define B_SKIP_REPLACE (B_USER+3)

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static gboolean
edit_dialog_search_show (WEdit * edit)
{
    char *search_text;
    size_t num_of_types = 0;
    gchar **list_of_types;
    int dialog_result;

    list_of_types = mc_search_get_types_strings_array (&num_of_types);

    {
        quick_widget_t quick_widgets[] = {
            /* *INDENT-OFF* */
            QUICK_LABELED_INPUT (N_("Enter search string:"), input_label_above, INPUT_LAST_TEXT, 
                                 MC_HISTORY_SHARED_SEARCH, &search_text, NULL, FALSE, FALSE,
                                 INPUT_COMPLETE_NONE),
            QUICK_SEPARATOR (TRUE),
            QUICK_START_COLUMNS,
                QUICK_RADIO (num_of_types, (const char **) list_of_types,
                             (int *) &edit_search_options.type, NULL),
            QUICK_NEXT_COLUMN,
                QUICK_CHECKBOX (N_("Cas&e sensitive"), &edit_search_options.case_sens, NULL),
                QUICK_CHECKBOX (N_("&Backwards"), &edit_search_options.backwards, NULL),
                QUICK_CHECKBOX (N_("In se&lection"), &edit_search_options.only_in_selection, NULL),
                QUICK_CHECKBOX (N_("&Whole words"), &edit_search_options.whole_words, NULL),
#ifdef HAVE_CHARSET
                QUICK_CHECKBOX (N_("&All charsets"), &edit_search_options.all_codepages, NULL),
#endif
            QUICK_STOP_COLUMNS,
            QUICK_START_BUTTONS (TRUE, TRUE),
                QUICK_BUTTON (N_("&OK"), B_ENTER, NULL, NULL),
                QUICK_BUTTON (N_("&Find all"), B_USER, NULL, NULL),
                QUICK_BUTTON (N_("&Cancel"), B_CANCEL, NULL, NULL),
            QUICK_END
            /* *INDENT-ON* */
        };

        quick_dialog_t qdlg = {
            -1, -1, 58,
            N_("Search"), "[Input Line Keys]",
            quick_widgets, NULL, NULL
        };

        dialog_result = quick_dialog (&qdlg);
    }

    g_strfreev (list_of_types);

    if ((dialog_result == B_CANCEL) || (search_text == NULL) || (search_text[0] == '\0'))
    {
        g_free (search_text);
        return FALSE;
    }

    if (dialog_result == B_USER)
        search_create_bookmark = TRUE;

#ifdef HAVE_CHARSET
    {
        GString *tmp;

        tmp = str_convert_to_input (search_text);
        g_free (search_text);
        search_text = g_string_free (tmp, FALSE);
    }
#endif

    edit_search_deinit (edit);
    edit->last_search_string = search_text;

    return edit_search_init (edit, edit->last_search_string);
}

/* --------------------------------------------------------------------------------------------- */

static void
edit_dialog_replace_show (WEdit * edit, const char *search_default, const char *replace_default,
                          /*@out@ */ char **search_text, /*@out@ */ char **replace_text)
{
    size_t num_of_types = 0;
    gchar **list_of_types;

    if ((search_default == NULL) || (*search_default == '\0'))
        search_default = INPUT_LAST_TEXT;

    list_of_types = mc_search_get_types_strings_array (&num_of_types);

    {
        quick_widget_t quick_widgets[] = {
            /* *INDENT-OFF* */
            QUICK_LABELED_INPUT (N_("Enter search string:"), input_label_above, search_default,
                                 MC_HISTORY_SHARED_SEARCH, search_text, NULL, FALSE, FALSE,
                                 INPUT_COMPLETE_NONE),
            QUICK_LABELED_INPUT (N_("Enter replacement string:"), input_label_above, replace_default,
                                 "replace", replace_text, NULL, FALSE, FALSE, INPUT_COMPLETE_NONE),
            QUICK_SEPARATOR (TRUE),
            QUICK_START_COLUMNS,
                QUICK_RADIO (num_of_types, (const char **) list_of_types,
                             (int *) &edit_search_options.type, NULL),
            QUICK_NEXT_COLUMN,
                QUICK_CHECKBOX (N_("Cas&e sensitive"), &edit_search_options.case_sens, NULL),
                QUICK_CHECKBOX (N_("&Backwards"), &edit_search_options.backwards, NULL),
                QUICK_CHECKBOX (N_("In se&lection"), &edit_search_options.only_in_selection, NULL),
                QUICK_CHECKBOX (N_("&Whole words"), &edit_search_options.whole_words, NULL),
#ifdef HAVE_CHARSET
                QUICK_CHECKBOX (N_("&All charsets"), &edit_search_options.all_codepages, NULL),
#endif
            QUICK_STOP_COLUMNS,
            QUICK_BUTTONS_OK_CANCEL,
            QUICK_END
            /* *INDENT-ON* */
        };

        quick_dialog_t qdlg = {
            -1, -1, 58,
            N_("Replace"), "[Input Line Keys]",
            quick_widgets, NULL, NULL
        };

        if (quick_dialog (&qdlg) != B_CANCEL)
            edit->replace_mode = 0;
        else
        {
            *replace_text = NULL;
            *search_text = NULL;
        }
    }

    g_strfreev (list_of_types);
}

/* --------------------------------------------------------------------------------------------- */

static int
edit_dialog_replace_prompt_show (WEdit * edit, char *from_text, char *to_text, int xpos, int ypos)
{
    Widget *w = WIDGET (edit);

    /* dialog size */
    int dlg_height = 10;
    int dlg_width;

    char tmp[BUF_MEDIUM];
    char *repl_from, *repl_to;
    int retval;

    if (xpos == -1)
        xpos = w->x + option_line_state_width + 1;
    if (ypos == -1)
        ypos = w->y + w->lines / 2;
    /* Sometimes menu can hide replaced text. I don't like it */
    if ((edit->curs_row >= ypos - 1) && (edit->curs_row <= ypos + dlg_height - 1))
        ypos -= dlg_height;

    dlg_width = WIDGET (w->owner)->cols - xpos - 1;

    g_snprintf (tmp, sizeof (tmp), "\"%s\"", from_text);
    repl_from = g_strdup (str_trunc (tmp, dlg_width - 7));

    g_snprintf (tmp, sizeof (tmp), "\"%s\"", to_text);
    repl_to = g_strdup (str_trunc (tmp, dlg_width - 7));

    {
        quick_widget_t quick_widgets[] = {
            /* *INDENT-OFF* */
            QUICK_LABEL (repl_from, NULL),
            QUICK_LABEL (N_("Replace with:"), NULL),
            QUICK_LABEL (repl_to, NULL),
            QUICK_START_BUTTONS (TRUE, TRUE),
                QUICK_BUTTON (N_("&Replace"), B_ENTER, NULL, NULL),
                QUICK_BUTTON (N_("A&ll"), B_REPLACE_ALL, NULL, NULL),
                QUICK_BUTTON (N_("&Skip"), B_SKIP_REPLACE, NULL, NULL),
                QUICK_BUTTON (N_("&Cancel"), B_CANCEL, NULL, NULL),
            QUICK_END
            /* *INDENT-ON* */
        };

        quick_dialog_t qdlg = {
            ypos, xpos, -1,
            N_("Confirm replace"), NULL,
            quick_widgets, NULL, NULL
        };

        retval = quick_dialog (&qdlg);
    }

    g_free (repl_from);
    g_free (repl_to);

    return retval;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Get EOL symbol for searching.
 *
 * @param edit editor object
 * @return EOL symbol
 */

static inline char
edit_search_get_current_end_line_char (const WEdit * edit)
{
    switch (edit->lb)
    {
    case LB_MAC:
        return '\r';
    default:
        return '\n';
    }
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Checking if search condition have BOL(^) or EOL ($) regexp special characters.
 *
 * @param search search object
 * @return result of checks.
 */

static edit_search_line_t
edit_get_search_line_type (mc_search_t * search)
{
    edit_search_line_t search_line_type = 0;

    if (search->search_type != MC_SEARCH_T_REGEX)
        return search_line_type;

    if (*search->original == '^')
        search_line_type |= AT_START_LINE;

    if (search->original[search->original_len - 1] == '$')
        search_line_type |= AT_END_LINE;
    return search_line_type;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Calculating the start position of next line.
 *
 * @param buf               editor buffer object
 * @param current_pos       current position
 * @param max_pos           max position
 * @param end_string_symbol end of line symbol
 * @return start position of next line
 */

static off_t
edit_calculate_start_of_next_line (const edit_buffer_t * buf, off_t current_pos, off_t max_pos,
                                   char end_string_symbol)
{
    off_t i;

    for (i = current_pos; i < max_pos; i++)
    {
        current_pos++;
        if (edit_buffer_get_byte (buf, i) == end_string_symbol)
            break;
    }

    return current_pos;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Calculating the end position of previous line.
 *
 * @param buf               editor buffer object
 * @param current_pos       current position
 * @param end_string_symbol end of line symbol
 * @return end position of previous line
 */

static off_t
edit_calculate_end_of_previous_line (const edit_buffer_t * buf, off_t current_pos,
                                     char end_string_symbol)
{
    off_t i;

    for (i = current_pos - 1; i >= 0; i--)
        if (edit_buffer_get_byte (buf, i) == end_string_symbol)
            break;

    return i;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Calculating the start position of previous line.
 *
 * @param buf               editor buffer object
 * @param current_pos       current position
 * @param end_string_symbol end of line symbol
 * @return start position of previous line
 */

static inline off_t
edit_calculate_start_of_previous_line (const edit_buffer_t * buf, off_t current_pos,
                                       char end_string_symbol)
{
    current_pos = edit_calculate_end_of_previous_line (buf, current_pos, end_string_symbol);
    current_pos = edit_calculate_end_of_previous_line (buf, current_pos, end_string_symbol);

    return (current_pos + 1);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Calculating the start position of current line.
 *
 * @param buf               editor buffer object
 * @param current_pos       current position
 * @param end_string_symbol end of line symbol
 * @return start position of current line
 */

static inline off_t
edit_calculate_start_of_current_line (const edit_buffer_t * buf, off_t current_pos,
                                      char end_string_symbol)
{
    current_pos = edit_calculate_end_of_previous_line (buf, current_pos, end_string_symbol);

    return (current_pos + 1);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Fixing (if needed) search start position if 'only in selection' option present.
 *
 * @param edit editor object
 */

static void
edit_search_fix_search_start_if_selection (WEdit * edit)
{
    off_t start_mark = 0;
    off_t end_mark = 0;

    if (!edit_search_options.only_in_selection)
        return;

    if (!eval_marks (edit, &start_mark, &end_mark))
        return;

    if (edit_search_options.backwards)
    {
        if (edit->search_start > end_mark || edit->search_start <= start_mark)
            edit->search_start = end_mark;
    }
    else
    {
        if (edit->search_start < start_mark || edit->search_start >= end_mark)
            edit->search_start = start_mark;
    }
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
edit_find (edit_search_status_msg_t * esm, gsize * len)
{
    WEdit *edit = esm->edit;
    off_t search_start = edit->search_start;
    off_t search_end;
    off_t start_mark = 0;
    off_t end_mark = edit->buffer.size;
    char end_string_symbol;

    end_string_symbol = edit_search_get_current_end_line_char (edit);

    /* prepare for search */
    if (edit_search_options.only_in_selection)
    {
        if (!eval_marks (edit, &start_mark, &end_mark))
        {
            mc_search_set_error (edit->search, MC_SEARCH_E_NOTFOUND, "%s", _(STR_E_NOTFOUND));
            return FALSE;
        }

        /* fix the start and the end of search block positions */
        if ((edit->search_line_type & AT_START_LINE) != 0
            && (start_mark != 0
                || edit_buffer_get_byte (&edit->buffer, start_mark - 1) != end_string_symbol))
            start_mark =
                edit_calculate_start_of_next_line (&edit->buffer, start_mark, edit->buffer.size,
                                                   end_string_symbol);

        if ((edit->search_line_type & AT_END_LINE) != 0
            && (end_mark - 1 != edit->buffer.size
                || edit_buffer_get_byte (&edit->buffer, end_mark) != end_string_symbol))
            end_mark =
                edit_calculate_end_of_previous_line (&edit->buffer, end_mark, end_string_symbol);

        if (start_mark >= end_mark)
        {
            mc_search_set_error (edit->search, MC_SEARCH_E_NOTFOUND, "%s", _(STR_E_NOTFOUND));
            return FALSE;
        }
    }
    else if (edit_search_options.backwards)
        end_mark = MAX (1, edit->buffer.curs1) - 1;

    /* search */
    if (edit_search_options.backwards)
    {
        /* backward search */
        search_end = end_mark;

        if ((edit->search_line_type & AT_START_LINE) != 0)
            search_start =
                edit_calculate_start_of_current_line (&edit->buffer, search_start,
                                                      end_string_symbol);

        while (search_start >= start_mark)
        {
            gboolean ok;

            if (search_end > (off_t) (search_start + edit->search->original_len)
                && mc_search_is_fixed_search_str (edit->search))
                search_end = search_start + edit->search->original_len;

            ok = mc_search_run (edit->search, (void *) esm, search_start, search_end, len);

            if (ok && edit->search->normal_offset == search_start)
                return TRUE;

            /* We abort the search in case of a pattern error, or if the user aborts
               the search. In other words: in all cases except "string not found". */
            if (!ok && edit->search->error != MC_SEARCH_E_NOTFOUND)
                return FALSE;

            if ((edit->search_line_type & AT_START_LINE) != 0)
                search_start =
                    edit_calculate_start_of_previous_line (&edit->buffer, search_start,
                                                           end_string_symbol);
            else
                search_start--;
        }

        mc_search_set_error (edit->search, MC_SEARCH_E_NOTFOUND, "%s", _(STR_E_NOTFOUND));
        return FALSE;
    }

    /* forward search */
    if ((edit->search_line_type & AT_START_LINE) != 0 && search_start != start_mark)
        search_start =
            edit_calculate_start_of_next_line (&edit->buffer, search_start, end_mark,
                                               end_string_symbol);

    return mc_search_run (edit->search, (void *) esm, search_start, end_mark, len);
}

/* --------------------------------------------------------------------------------------------- */

static char *
edit_replace_cmd__conv_to_display (const char *str)
{
#ifdef HAVE_CHARSET
    GString *tmp;

    tmp = str_convert_to_display (str);
    if (tmp != NULL)
    {
        if (tmp->len != 0)
            return g_string_free (tmp, FALSE);
        g_string_free (tmp, TRUE);
    }
#endif
    return g_strdup (str);
}

/* --------------------------------------------------------------------------------------------- */

static char *
edit_replace_cmd__conv_to_input (char *str)
{
#ifdef HAVE_CHARSET
    GString *tmp;

    tmp = str_convert_to_input (str);
    if (tmp->len != 0)
        return g_string_free (tmp, FALSE);
    g_string_free (tmp, TRUE);
#endif
    return g_strdup (str);
}

/* --------------------------------------------------------------------------------------------- */

static void
edit_show_search_error (const WEdit * edit, const char *title)
{
    if (edit->search->error == MC_SEARCH_E_NOTFOUND)
        edit_query_dialog (title, _(STR_E_NOTFOUND));
    else if (edit->search->error_str != NULL)
        edit_query_dialog (title, edit->search->error_str);
}

/* --------------------------------------------------------------------------------------------- */

static void
edit_do_search (WEdit * edit)
{
    edit_search_status_msg_t esm;
    gsize len = 0;

    /* This shouldn't happen */
    assert (edit->search != NULL);

    edit_push_undo_action (edit, KEY_PRESS + edit->start_display);

    esm.first = TRUE;
    esm.edit = edit;
    esm.offset = edit->search_start;

    status_msg_init (STATUS_MSG (&esm), _("Search"), 1.0, simple_status_msg_init_cb,
                     edit_search_status_update_cb, NULL);

    if (search_create_bookmark)
    {
        gboolean found = FALSE;
        long l = 0, l_last = -1;
        long q = 0;

        search_create_bookmark = FALSE;
        book_mark_flush (edit, -1);

        while (mc_search_run (edit->search, (void *) &esm, q, edit->buffer.size, &len))
        {
            if (!found)
                edit->search_start = edit->search->normal_offset;
            found = TRUE;

            l += edit_buffer_count_lines (&edit->buffer, q, edit->search->normal_offset);
            if (l != l_last)
                book_mark_insert (edit, l, BOOK_MARK_FOUND_COLOR);
            l_last = l;
            q = edit->search->normal_offset + 1;
        }

        if (!found)
            edit_error_dialog (_("Search"), _(STR_E_NOTFOUND));
        else
            edit_cursor_move (edit, edit->search_start - edit->buffer.curs1);
    }
    else
    {
        if (edit->found_len != 0 && edit->search_start == edit->found_start + 1
            && edit_search_options.backwards)
            edit->search_start--;

        if (edit->found_len != 0 && edit->search_start == edit->found_start - 1
            && !edit_search_options.backwards)
            edit->search_start++;

        if (edit_find (&esm, &len))
        {
            edit->found_start = edit->search_start = edit->search->normal_offset;
            edit->found_len = len;
            edit->over_col = 0;
            edit_cursor_move (edit, edit->search_start - edit->buffer.curs1);
            edit_scroll_screen_over_cursor (edit);
            if (edit_search_options.backwards)
                edit->search_start--;
            else
                edit->search_start++;
        }
        else
        {
            edit->search_start = edit->buffer.curs1;
            edit_show_search_error (edit, _("Search"));
        }
    }

    status_msg_deinit (STATUS_MSG (&esm));

    edit->force |= REDRAW_COMPLETELY;
    edit_scroll_screen_over_cursor (edit);
}

/* --------------------------------------------------------------------------------------------- */

static void
edit_search (WEdit * edit)
{
    if (edit_dialog_search_show (edit))
    {
        edit->search_line_type = edit_get_search_line_type (edit->search);
        edit_search_fix_search_start_if_selection (edit);
        edit_do_search (edit);
    }
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

gboolean
edit_search_init (WEdit * edit, const char *str)
{
#ifdef HAVE_CHARSET
    edit->search = mc_search_new (str, cp_source);
#else
    edit->search = mc_search_new (str, NULL);
#endif

    if (edit->search == NULL)
        return FALSE;

    edit->search->search_type = edit_search_options.type;
#ifdef HAVE_CHARSET
    edit->search->is_all_charsets = edit_search_options.all_codepages;
#endif
    edit->search->is_case_sensitive = edit_search_options.case_sens;
    edit->search->whole_words = edit_search_options.whole_words;
    edit->search->search_fn = edit_search_cmd_callback;
    edit->search->update_fn = edit_search_update_callback;

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

void
edit_search_deinit (WEdit * edit)
{
    mc_search_free (edit->search);
    g_free (edit->last_search_string);
}

/* --------------------------------------------------------------------------------------------- */

mc_search_cbret_t
edit_search_cmd_callback (const void *user_data, gsize char_offset, int *current_char)
{
    WEdit *edit = ((const edit_search_status_msg_t *) user_data)->edit;

    *current_char = edit_buffer_get_byte (&edit->buffer, (off_t) char_offset);

    return MC_SEARCH_CB_OK;
}

/* --------------------------------------------------------------------------------------------- */

mc_search_cbret_t
edit_search_update_callback (const void *user_data, gsize char_offset)
{
    status_msg_t *sm = STATUS_MSG (user_data);

    ((edit_search_status_msg_t *) sm)->offset = (off_t) char_offset;

    return (sm->update (sm) == B_CANCEL ? MC_SEARCH_CB_ABORT : MC_SEARCH_CB_OK);
}

/* --------------------------------------------------------------------------------------------- */

int
edit_search_status_update_cb (status_msg_t * sm)
{
    simple_status_msg_t *ssm = SIMPLE_STATUS_MSG (sm);
    edit_search_status_msg_t *esm = (edit_search_status_msg_t *) sm;
    Widget *wd = WIDGET (sm->dlg);

    if (verbose)
        label_set_textv (ssm->label, _("Searching %s: %3d%%"), esm->edit->last_search_string,
                         edit_buffer_calc_percent (&esm->edit->buffer, esm->offset));
    else
        label_set_textv (ssm->label, _("Searching %s"), esm->edit->last_search_string);

    if (esm->first)
    {
        int wd_width;
        Widget *lw = WIDGET (ssm->label);

        wd_width = MAX (wd->cols, lw->cols + 6);
        widget_set_size (wd, wd->y, wd->x, wd->lines, wd_width);
        widget_set_size (lw, lw->y, wd->x + (wd->cols - lw->cols) / 2, lw->lines, lw->cols);
        esm->first = FALSE;
    }

    return status_msg_common_update (sm);
}

/* --------------------------------------------------------------------------------------------- */

void
edit_search_cmd (WEdit * edit, gboolean again)
{
    if (!again)
        edit_search (edit);
    else if (edit->last_search_string != NULL)
        edit_do_search (edit);
    else
    {
        /* find last search string in history */
        GList *history;

        history = mc_config_history_get (MC_HISTORY_SHARED_SEARCH);
        if (history != NULL)
        {
            /* FIXME: is it possible that history->data == NULL? */
            edit->last_search_string = (char *) history->data;
            history->data = NULL;
            history = g_list_first (history);
            g_list_free_full (history, g_free);

            if (edit_search_init (edit, edit->last_search_string))
            {
                edit_do_search (edit);
                return;
            }

            /* found, but cannot init search */
            MC_PTR_FREE (edit->last_search_string);
        }

        /* if not... then ask for an expression */
        edit_search (edit);
    }
}

/* --------------------------------------------------------------------------------------------- */
/** call with edit = 0 before shutdown to close memory leaks */

void
edit_replace_cmd (WEdit * edit, gboolean again)
{
    /* 1 = search string, 2 = replace with */
    static char *saved1 = NULL; /* saved default[123] */
    static char *saved2 = NULL;
    char *input1 = NULL;        /* user input from the dialog */
    char *input2 = NULL;
    GString *input2_str = NULL;
    char *disp1 = NULL;
    char *disp2 = NULL;
    long times_replaced = 0;
    gboolean once_found = FALSE;
    edit_search_status_msg_t esm;

    if (edit == NULL)
    {
        MC_PTR_FREE (saved1);
        MC_PTR_FREE (saved2);
        return;
    }

    edit->force |= REDRAW_COMPLETELY;

    if (again && saved1 == NULL && saved2 == NULL)
        again = FALSE;

    if (again)
    {
        input1 = g_strdup (saved1 != NULL ? saved1 : "");
        input2 = g_strdup (saved2 != NULL ? saved2 : "");
    }
    else
    {
        char *tmp_inp1, *tmp_inp2;

        disp1 = edit_replace_cmd__conv_to_display (saved1 != NULL ? saved1 : "");
        disp2 = edit_replace_cmd__conv_to_display (saved2 != NULL ? saved2 : "");

        edit_push_undo_action (edit, KEY_PRESS + edit->start_display);

        edit_dialog_replace_show (edit, disp1, disp2, &input1, &input2);

        g_free (disp1);
        g_free (disp2);

        if (input1 == NULL || *input1 == '\0')
        {
            edit->force = REDRAW_COMPLETELY;
            goto cleanup;
        }

        tmp_inp1 = input1;
        tmp_inp2 = input2;
        input1 = edit_replace_cmd__conv_to_input (input1);
        input2 = edit_replace_cmd__conv_to_input (input2);
        g_free (tmp_inp1);
        g_free (tmp_inp2);

        g_free (saved1);
        saved1 = g_strdup (input1);
        g_free (saved2);
        saved2 = g_strdup (input2);

        mc_search_free (edit->search);
        edit->search = NULL;
    }

    input2_str = g_string_new (input2);

    if (edit->search == NULL)
    {
        if (edit_search_init (edit, input1))
            edit_search_fix_search_start_if_selection (edit);
        else
        {
            edit->search_start = edit->buffer.curs1;
            goto cleanup;
        }
    }

    if (edit->found_len != 0 && edit->search_start == edit->found_start + 1
        && edit_search_options.backwards)
        edit->search_start--;

    if (edit->found_len != 0 && edit->search_start == edit->found_start - 1
        && !edit_search_options.backwards)
        edit->search_start++;

    esm.first = TRUE;
    esm.edit = edit;
    esm.offset = edit->search_start;

    status_msg_init (STATUS_MSG (&esm), _("Search"), 1.0, simple_status_msg_init_cb,
                     edit_search_status_update_cb, NULL);

    do
    {
        gsize len = 0;

        if (!edit_find (&esm, &len))
        {
            if (!(edit->search->error == MC_SEARCH_E_OK ||
                  (once_found && edit->search->error == MC_SEARCH_E_NOTFOUND)))
                edit_show_search_error (edit, _("Search"));
            break;
        }

        once_found = TRUE;

        edit->search_start = edit->search->normal_offset;
        /* returns negative on not found or error in pattern */

        if (edit->search_start >= 0 && edit->search_start < edit->buffer.size)
        {
            gsize i;
            GString *repl_str;

            edit->found_start = edit->search_start;
            edit->found_len = len;

            edit_cursor_move (edit, edit->search_start - edit->buffer.curs1);
            edit_scroll_screen_over_cursor (edit);

            if (edit->replace_mode == 0)
            {
                long l;
                int prompt;

                l = edit->curs_row - WIDGET (edit)->lines / 3;
                if (l > 0)
                    edit_scroll_downward (edit, l);
                if (l < 0)
                    edit_scroll_upward (edit, -l);

                edit_scroll_screen_over_cursor (edit);
                edit->force |= REDRAW_PAGE;
                edit_render_keypress (edit);

                /*so that undo stops at each query */
                edit_push_key_press (edit);
                /* and prompt 2/3 down */
                disp1 = edit_replace_cmd__conv_to_display (saved1);
                disp2 = edit_replace_cmd__conv_to_display (saved2);
                prompt = edit_dialog_replace_prompt_show (edit, disp1, disp2, -1, -1);
                g_free (disp1);
                g_free (disp2);

                if (prompt == B_REPLACE_ALL)
                    edit->replace_mode = 1;
                else if (prompt == B_SKIP_REPLACE)
                {
                    if (edit_search_options.backwards)
                        edit->search_start--;
                    else
                        edit->search_start++;
                    continue;   /* loop */
                }
                else if (prompt == B_CANCEL)
                {
                    edit->replace_mode = -1;
                    break;      /* loop */
                }
            }

            repl_str = mc_search_prepare_replace_str (edit->search, input2_str);

            if (edit->search->error != MC_SEARCH_E_OK)
            {
                edit_show_search_error (edit, _("Replace"));
                if (repl_str != NULL)
                    g_string_free (repl_str, TRUE);
                break;
            }

            /* delete then insert new */
            for (i = 0; i < len; i++)
                edit_delete (edit, TRUE);

            for (i = 0; i < repl_str->len; i++)
                edit_insert (edit, repl_str->str[i]);

            edit->found_len = repl_str->len;
            g_string_free (repl_str, TRUE);
            times_replaced++;

            /* so that we don't find the same string again */
            if (edit_search_options.backwards)
                edit->search_start--;
            else
            {
                edit->search_start += edit->found_len + (len == 0 ? 1 : 0);

                if (edit->search_start >= edit->buffer.size)
                    break;
            }

            edit_scroll_screen_over_cursor (edit);
        }
        else
        {
            /* try and find from right here for next search */
            edit->search_start = edit->buffer.curs1;
            edit_update_curs_col (edit);

            edit->force |= REDRAW_PAGE;
            edit_render_keypress (edit);

            if (times_replaced == 0)
                query_dialog (_("Replace"), _(STR_E_NOTFOUND), D_NORMAL, 1, _("&OK"));
            break;
        }
    }
    while (edit->replace_mode >= 0);

    status_msg_deinit (STATUS_MSG (&esm));
    edit_scroll_screen_over_cursor (edit);
    edit->force |= REDRAW_COMPLETELY;
    edit_render_keypress (edit);

    if (edit->replace_mode == 1 && times_replaced != 0)
        message (D_NORMAL, _("Replace"), _("%ld replacements made"), times_replaced);

  cleanup:
    g_free (input1);
    g_free (input2);
    if (input2_str != NULL)
        g_string_free (input2_str, TRUE);
}

/* --------------------------------------------------------------------------------------------- */
