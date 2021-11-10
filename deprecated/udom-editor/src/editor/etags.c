/*
   Editor C-code navigation via tags.
   make TAGS file via command:
   $ find . -type f -name "*.[ch]" | etags -l c --declarations -

   or, if etags utility not installed:
   $ find . -type f -name "*.[ch]" | ctags --c-kinds=+p --fields=+iaS --extra=+q -e -L-

   Copyright (C) 2009-2021
   Free Software Foundation, Inc.

   Written by:
   Ilia Maslakov <il.smind@gmail.com>, 2009
   Slava Zanko <slavazanko@gmail.com>, 2009

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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/global.h"
#include "lib/fileloc.h"        /* TAGS_NAME */
#include "lib/tty/tty.h"        /* LINES, COLS */
#include "lib/strutil.h"
#include "lib/util.h"           /* canonicalize_pathname() */

#include "editwidget.h"

#include "etags.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

static int def_max_width;

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static void
etags_hash_free (gpointer data)
{
    etags_hash_t *hash = (etags_hash_t *) data;

    g_free (hash->filename);
    g_free (hash->fullpath);
    g_free (hash->short_define);
    g_free (hash);
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
parse_define (const char *buf, char **long_name, char **short_name, long *line)
{
    /* *INDENT-OFF* */
    enum
    {
        in_longname,
        in_shortname,
        in_shortname_first_char,
        in_line,
        finish
    } def_state = in_longname;
    /* *INDENT-ON* */

    GString *longdef = NULL;
    GString *shortdef = NULL;
    GString *linedef = NULL;

    char c = *buf;

    while (!(c == '\0' || c == '\n'))
    {
        switch (def_state)
        {
        case in_longname:
            if (c == 0x01)
                def_state = in_line;
            else if (c == 0x7F)
                def_state = in_shortname;
            else
            {
                if (longdef == NULL)
                    longdef = g_string_sized_new (32);

                g_string_append_c (longdef, c);
            }
            break;

        case in_shortname_first_char:
            if (isdigit (c))
            {
                if (shortdef == NULL)
                    shortdef = g_string_sized_new (32);
                else
                    g_string_set_size (shortdef, 0);

                buf--;
                def_state = in_line;
            }
            else if (c == 0x01)
                def_state = in_line;
            else
            {
                if (shortdef == NULL)
                    shortdef = g_string_sized_new (32);

                g_string_append_c (shortdef, c);
                def_state = in_shortname;
            }
            break;

        case in_shortname:
            if (c == 0x01)
                def_state = in_line;
            else if (c == '\n')
                def_state = finish;
            else
            {
                if (shortdef == NULL)
                    shortdef = g_string_sized_new (32);

                g_string_append_c (shortdef, c);
            }
            break;

        case in_line:
            if (c == ',' || c == '\n')
                def_state = finish;
            else if (isdigit (c))
            {
                if (linedef == NULL)
                    linedef = g_string_sized_new (32);

                g_string_append_c (linedef, c);
            }
            break;

        case finish:
            *long_name = longdef == NULL ? NULL : g_string_free (longdef, FALSE);
            *short_name = shortdef == NULL ? NULL : g_string_free (shortdef, FALSE);

            if (linedef == NULL)
                *line = 0;
            else
            {
                *line = atol (linedef->str);
                g_string_free (linedef, TRUE);
            }
            return TRUE;

        default:
            break;
        }

        buf++;
        c = *buf;
    }

    *long_name = NULL;
    *short_name = NULL;
    *line = 0;

    return FALSE;
}

/* --------------------------------------------------------------------------------------------- */

static GPtrArray *
etags_set_definition_hash (const char *tagfile, const char *start_path, const char *match_func)
{
    /* *INDENT-OFF* */
    enum
    {
        start,
        in_filename,
        in_define
    } state = start;
    /* *INDENT-ON* */

    FILE *f;
    char buf[BUF_LARGE];
    char *filename = NULL;
    GPtrArray *ret = NULL;

    if (match_func == NULL || tagfile == NULL)
        return NULL;

    /* open file with positions */
    f = fopen (tagfile, "r");
    if (f == NULL)
        return NULL;

    while (fgets (buf, sizeof (buf), f) != NULL)
        switch (state)
        {
        case start:
            if (buf[0] == 0x0C)
                state = in_filename;
            break;

        case in_filename:
            {
                size_t pos;

                pos = strcspn (buf, ",");
                g_free (filename);
                filename = g_strndup (buf, pos);
                state = in_define;
                break;
            }

        case in_define:
            if (buf[0] == 0x0C)
                state = in_filename;
            else
            {
                char *chekedstr;

                /* check if the filename matches the define pos */
                chekedstr = strstr (buf, match_func);
                if (chekedstr != NULL)
                {
                    char *longname = NULL;
                    char *shortname = NULL;
                    etags_hash_t *def_hash;

                    def_hash = g_new (etags_hash_t, 1);

                    def_hash->fullpath = mc_build_filename (start_path, filename, (char *) NULL);
                    canonicalize_pathname (def_hash->fullpath);
                    def_hash->filename = g_strdup (filename);

                    def_hash->line = 0;

                    parse_define (chekedstr, &longname, &shortname, &def_hash->line);

                    if (shortname != NULL && *shortname != '\0')
                    {
                        def_hash->short_define = shortname;
                        g_free (longname);
                    }
                    else
                    {
                        def_hash->short_define = longname;
                        g_free (shortname);
                    }

                    if (ret == NULL)
                        ret = g_ptr_array_new_with_free_func (etags_hash_free);

                    g_ptr_array_add (ret, def_hash);
                }
            }
            break;

        default:
            break;
        }

    g_free (filename);
    fclose (f);

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

static void
editcmd_dialog_select_definition_add (gpointer data, gpointer user_data)
{
    etags_hash_t *def_hash = (etags_hash_t *) data;
    WListbox *def_list = (WListbox *) user_data;
    char *label_def;
    int def_width;

    label_def =
        g_strdup_printf ("%s -> %s:%ld", def_hash->short_define, def_hash->filename,
                         def_hash->line);
    listbox_add_item (def_list, LISTBOX_APPEND_AT_END, 0, label_def, def_hash, FALSE);
    def_width = str_term_width1 (label_def);
    g_free (label_def);
    def_max_width = MAX (def_max_width, def_width);
}

/* --------------------------------------------------------------------------------------------- */
/* let the user select where function definition */

static void
editcmd_dialog_select_definition_show (WEdit * edit, char *match_expr, GPtrArray * def_hash)
{
    const Widget *we = CONST_WIDGET (edit);
    int start_x, start_y, offset;
    char *curr = NULL;
    WDialog *def_dlg;
    WListbox *def_list;
    int def_dlg_h;              /* dialog height */
    int def_dlg_w;              /* dialog width */

    /* calculate the dialog metrics */
    def_dlg_h = def_hash->len + 2;
    def_dlg_w = COLS - 2;       /* will be clarified later */
    start_x = we->x + edit->curs_col + edit->start_col + EDIT_TEXT_HORIZONTAL_OFFSET +
        (edit->fullscreen ? 0 : 1) + option_line_state_width;
    start_y = we->y + edit->curs_row + EDIT_TEXT_VERTICAL_OFFSET + (edit->fullscreen ? 0 : 1) + 1;

    if (start_x < 0)
        start_x = 0;
    if (start_x < we->x + 1)
        start_x = we->x + 1 + option_line_state_width;

    if (def_dlg_h > LINES - 2)
        def_dlg_h = LINES - 2;

    offset = start_y + def_dlg_h - LINES;
    if (offset > 0)
        start_y -= (offset + 1);

    def_dlg = dlg_create (TRUE, start_y, start_x, def_dlg_h, def_dlg_w, WPOS_KEEP_DEFAULT, TRUE,
                          dialog_colors, NULL, NULL, "[Definitions]", match_expr);
    def_list = listbox_new (1, 1, def_dlg_h - 2, def_dlg_w - 2, FALSE, NULL);
    group_add_widget_autopos (GROUP (def_dlg), def_list, WPOS_KEEP_ALL, NULL);

    /* fill the listbox with the completions and get the maximim width */
    def_max_width = 0;
    g_ptr_array_foreach (def_hash, editcmd_dialog_select_definition_add, def_list);

    /* adjust dialog width */
    def_dlg_w = def_max_width + 4;
    offset = start_x + def_dlg_w - COLS;
    if (offset > 0)
        start_x -= offset;

    widget_set_size (WIDGET (def_dlg), start_y, start_x, def_dlg_h, def_dlg_w);

    /* pop up the dialog and apply the chosen completion */
    if (dlg_run (def_dlg) == B_ENTER)
    {
        etags_hash_t *curr_def = NULL;
        gboolean do_moveto = FALSE;

        listbox_get_current (def_list, &curr, (void **) &curr_def);

        if (!edit->modified)
            do_moveto = TRUE;
        else if (!edit_query_dialog2
                 (_("Warning"),
                  _("Current text was modified without a file save.\n"
                    "Continue discards these changes."), _("C&ontinue"), _("&Cancel")))
        {
            edit->force |= REDRAW_COMPLETELY;
            do_moveto = TRUE;
        }

        if (curr != NULL && do_moveto && edit_stack_iterator + 1 < MAX_HISTORY_MOVETO)
        {
            vfs_path_free (edit_history_moveto[edit_stack_iterator].filename_vpath, TRUE);

            /* Is file path absolute? Prepend with dir_vpath if necessary */
            if (edit->filename_vpath != NULL && edit->filename_vpath->relative
                && edit->dir_vpath != NULL)
                edit_history_moveto[edit_stack_iterator].filename_vpath =
                    vfs_path_append_vpath_new (edit->dir_vpath, edit->filename_vpath, NULL);
            else
                edit_history_moveto[edit_stack_iterator].filename_vpath =
                    vfs_path_clone (edit->filename_vpath);

            edit_history_moveto[edit_stack_iterator].line = edit->start_line + edit->curs_row + 1;
            edit_stack_iterator++;
            vfs_path_free (edit_history_moveto[edit_stack_iterator].filename_vpath, TRUE);
            edit_history_moveto[edit_stack_iterator].filename_vpath =
                vfs_path_from_str ((char *) curr_def->fullpath);
            edit_history_moveto[edit_stack_iterator].line = curr_def->line;
            edit_reload_line (edit, edit_history_moveto[edit_stack_iterator].filename_vpath,
                              edit_history_moveto[edit_stack_iterator].line);
        }
    }

    /* destroy dialog before return */
    widget_destroy (WIDGET (def_dlg));
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
edit_get_match_keyword_cmd (WEdit * edit)
{
    gsize word_len = 0;
    gsize i;
    off_t word_start = 0;
    GString *match_expr;
    char *path = NULL;
    char *ptr = NULL;
    char *tagfile = NULL;
    GPtrArray *def_hash = NULL;

    /* search start of word to be completed */
    if (!edit_buffer_find_word_start (&edit->buffer, &word_start, &word_len))
        return;

    /* prepare match expression */
    match_expr = g_string_sized_new (word_len);
    for (i = 0; i < word_len; i++)
        g_string_append_c (match_expr, edit_buffer_get_byte (&edit->buffer, word_start + i));

    ptr = g_get_current_dir ();
    path = g_strconcat (ptr, PATH_SEP_STR, (char *) NULL);
    g_free (ptr);

    /* Recursive search file 'TAGS' in parent dirs */
    do
    {
        ptr = g_path_get_dirname (path);
        g_free (path);
        path = ptr;
        g_free (tagfile);
        tagfile = mc_build_filename (path, TAGS_NAME, (char *) NULL);
        if (tagfile != NULL && exist_file (tagfile))
            break;
    }
    while (strcmp (path, PATH_SEP_STR) != 0);

    if (tagfile != NULL)
    {
        def_hash = etags_set_definition_hash (tagfile, path, match_expr->str);
        g_free (tagfile);
    }
    g_free (path);

    if (def_hash != NULL)
    {
        editcmd_dialog_select_definition_show (edit, match_expr->str, def_hash);

        g_ptr_array_free (def_hash, TRUE);
    }

    g_string_free (match_expr, TRUE);
}

/* --------------------------------------------------------------------------------------------- */
