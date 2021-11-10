/*
   Editor high level editing commands

   Copyright (C) 1996-2021
   Free Software Foundation, Inc.

   Written by:
   Paul Sheer, 1996, 1997
   Andrew Borodin <aborodin@vmail.ru>, 2012-2021
   Ilia Maslakov <il.smind@gmail.com>, 2012

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
 *  \brief Source: editor high level editing commands
 *  \author Paul Sheer
 *  \date 1996, 1997
 */

/* #define PIPE_BLOCKS_SO_READ_BYTE_BY_BYTE */

#include <config.h>

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "lib/global.h"
#include "lib/tty/tty.h"
#include "lib/tty/key.h"        /* XCTRL */
#include "lib/strutil.h"        /* utf string functions */
#include "lib/fileloc.h"
#include "lib/lock.h"
#include "lib/util.h"           /* tilde_expand() */
#include "lib/vfs/vfs.h"
#include "lib/widget.h"
#include "lib/event.h"          /* mc_event_raise() */
#ifdef HAVE_CHARSET
#include "lib/charsets.h"
#endif

#include "src/history.h"
#include "src/file_history.h"   /* show_file_history() */
#include "src/setup.h"          /* option_tab_spacing */
#ifdef HAVE_CHARSET
#include "src/selcodepage.h"
#endif
#include "src/util.h"           /* check_for_default() */

#include "edit-impl.h"
#include "editwidget.h"
#include "editsearch.h"
#include "etags.h"

/*** global variables ****************************************************************************/

/* search and replace: */
int search_create_bookmark = FALSE;

/* queries on a save */
gboolean edit_confirm_save = TRUE;

/* whether we need to drop selection on copy to buffer */
gboolean option_drop_selection_on_copy = TRUE;

/*** file scope macro definitions ****************************************************************/

#define space_width 1

#define TEMP_BUF_LEN 1024

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

static unsigned long edit_save_mode_radio_id, edit_save_mode_input_id;

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
edit_save_mode_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    switch (msg)
    {
    case MSG_CHANGED_FOCUS:
        if (sender != NULL && sender->id == edit_save_mode_radio_id)
        {
            Widget *ww;

            ww = widget_find_by_id (w, edit_save_mode_input_id);
            widget_disable (ww, RADIO (sender)->sel != 2);
            return MSG_HANDLED;
        }
        return MSG_NOT_HANDLED;

    default:
        return dlg_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

/*  If 0 (quick save) then  a) create/truncate <filename> file,
   b) save to <filename>;
   if 1 (safe save) then   a) save to <tempnam>,
   b) rename <tempnam> to <filename>;
   if 2 (do backups) then  a) save to <tempnam>,
   b) rename <filename> to <filename.backup_ext>,
   c) rename <tempnam> to <filename>. */

/* returns 0 on error, -1 on abort */

static int
edit_save_file (WEdit * edit, const vfs_path_t * filename_vpath)
{
    char *p;
    gchar *tmp;
    off_t filelen = 0;
    int this_save_mode, rv, fd = -1;
    vfs_path_t *real_filename_vpath;
    vfs_path_t *savename_vpath = NULL;
    const char *start_filename;
    const vfs_path_element_t *vpath_element;
    struct stat sb;

    vpath_element = vfs_path_get_by_index (filename_vpath, 0);
    if (vpath_element == NULL)
        return 0;

    start_filename = vpath_element->path;
    if (*start_filename == '\0')
        return 0;

    if (!IS_PATH_SEP (*start_filename) && edit->dir_vpath != NULL)
        real_filename_vpath = vfs_path_append_vpath_new (edit->dir_vpath, filename_vpath, NULL);
    else
        real_filename_vpath = vfs_path_clone (filename_vpath);

    this_save_mode = option_save_mode;
    if (this_save_mode != EDIT_QUICK_SAVE)
    {
        if (!vfs_file_is_local (real_filename_vpath))
            /* The file does not exists yet, so no safe save or backup are necessary. */
            this_save_mode = EDIT_QUICK_SAVE;
        else
        {
            fd = mc_open (real_filename_vpath, O_RDONLY | O_BINARY);
            if (fd == -1)
                /* The file does not exists yet, so no safe save or backup are necessary. */
                this_save_mode = EDIT_QUICK_SAVE;
        }

        if (fd != -1)
            mc_close (fd);
    }

    rv = mc_stat (real_filename_vpath, &sb);
    if (rv == 0)
    {
        if (this_save_mode == EDIT_QUICK_SAVE && !edit->skip_detach_prompt && sb.st_nlink > 1)
        {
            rv = edit_query_dialog3 (_("Warning"),
                                     _("File has hard-links. Detach before saving?"),
                                     _("&Yes"), _("&No"), _("&Cancel"));
            switch (rv)
            {
            case 0:
                this_save_mode = EDIT_SAFE_SAVE;
                MC_FALLTHROUGH;
            case 1:
                edit->skip_detach_prompt = 1;
                break;
            default:
                vfs_path_free (real_filename_vpath, TRUE);
                return -1;
            }
        }

        /* Prevent overwriting changes from other editor sessions. */
        if (edit->stat1.st_mtime != 0 && edit->stat1.st_mtime != sb.st_mtime)
        {
            /* The default action is "Cancel". */
            query_set_sel (1);

            rv = edit_query_dialog2 (_("Warning"),
                                     _("The file has been modified in the meantime. Save anyway?"),
                                     _("&Yes"), _("&Cancel"));
            if (rv != 0)
            {
                vfs_path_free (real_filename_vpath, TRUE);
                return -1;
            }
        }
    }

    if (this_save_mode == EDIT_QUICK_SAVE)
        savename_vpath = vfs_path_clone (real_filename_vpath);
    else
    {
        char *savedir, *saveprefix;

        savedir = vfs_path_tokens_get (real_filename_vpath, 0, -1);
        if (savedir == NULL)
            savedir = g_strdup (".");

        /* Token-related function never return leading slash, so we need add it manually */
        saveprefix = mc_build_filename (PATH_SEP_STR, savedir, "cooledit", (char *) NULL);
        g_free (savedir);
        fd = mc_mkstemps (&savename_vpath, saveprefix, NULL);
        g_free (saveprefix);
        if (savename_vpath == NULL)
        {
            vfs_path_free (real_filename_vpath, TRUE);
            return 0;
        }
        /* FIXME:
         * Close for now because mc_mkstemps use pure open system call
         * to create temporary file and it needs to be reopened by
         * VFS-aware mc_open().
         */
        close (fd);
    }

    (void) mc_chown (savename_vpath, edit->stat1.st_uid, edit->stat1.st_gid);
    (void) mc_chmod (savename_vpath, edit->stat1.st_mode);

    fd = mc_open (savename_vpath, O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, edit->stat1.st_mode);
    if (fd == -1)
        goto error_save;

    /* pipe save */
    p = edit_get_write_filter (savename_vpath, real_filename_vpath);
    if (p != NULL)
    {
        FILE *file;

        mc_close (fd);
        file = (FILE *) popen (p, "w");

        if (file != NULL)
        {
            filelen = edit_write_stream (edit, file);
#if 1
            pclose (file);
#else
            if (pclose (file) != 0)
            {
                tmp = g_strdup_printf (_("Error writing to pipe: %s"), p);
                edit_error_dialog (_("Error"), tmp);
                g_free (tmp);
                g_free (p);
                goto error_save;
            }
#endif
        }
        else
        {
            tmp = g_strdup_printf (_("Cannot open pipe for writing: %s"), p);
            edit_error_dialog (_("Error"), get_sys_error (tmp));
            g_free (p);
            g_free (tmp);
            goto error_save;
        }
        g_free (p);
    }
    else if (edit->lb == LB_ASIS)
    {                           /* do not change line breaks */
        filelen = edit_buffer_write_file (&edit->buffer, fd);

        if (filelen != edit->buffer.size)
        {
            mc_close (fd);
            goto error_save;
        }

        if (mc_close (fd) != 0)
            goto error_save;

        /* Update the file information, especially the mtime. */
        if (mc_stat (savename_vpath, &edit->stat1) == -1)
            goto error_save;
    }
    else
    {                           /* change line breaks */
        FILE *file;
        const vfs_path_element_t *path_element;

        mc_close (fd);

        path_element = vfs_path_get_by_index (savename_vpath, -1);
        file = (FILE *) fopen (path_element->path, "w");
        if (file != NULL)
        {
            filelen = edit_write_stream (edit, file);
            fclose (file);
        }
        else
        {
            char *msg;

            msg = g_strdup_printf (_("Cannot open file for writing: %s"), path_element->path);
            edit_error_dialog (_("Error"), msg);
            g_free (msg);
            goto error_save;
        }
    }

    if (filelen != edit->buffer.size)
        goto error_save;

    if (this_save_mode == EDIT_DO_BACKUP)
    {
        char *tmp_store_filename;
        vfs_path_element_t *last_vpath_element;
        vfs_path_t *tmp_vpath;
        gboolean ok;

        g_assert (option_backup_ext != NULL);

        /* add backup extension to the path */
        tmp_vpath = vfs_path_clone (real_filename_vpath);
        last_vpath_element = (vfs_path_element_t *) vfs_path_get_by_index (tmp_vpath, -1);
        tmp_store_filename = last_vpath_element->path;
        last_vpath_element->path = g_strdup_printf ("%s%s", tmp_store_filename, option_backup_ext);
        g_free (tmp_store_filename);

        ok = (mc_rename (real_filename_vpath, tmp_vpath) != -1);
        vfs_path_free (tmp_vpath, TRUE);
        if (!ok)
            goto error_save;
    }

    if (this_save_mode != EDIT_QUICK_SAVE && mc_rename (savename_vpath, real_filename_vpath) == -1)
        goto error_save;

    vfs_path_free (real_filename_vpath, TRUE);
    vfs_path_free (savename_vpath, TRUE);
    return 1;
  error_save:
    /*  FIXME: Is this safe ?
     *  if (this_save_mode != EDIT_QUICK_SAVE)
     *      mc_unlink (savename);
     */
    vfs_path_free (real_filename_vpath, TRUE);
    vfs_path_free (savename_vpath, TRUE);
    return 0;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
edit_check_newline (const edit_buffer_t * buf)
{
    return !(option_check_nl_at_eof && buf->size > 0
             && edit_buffer_get_byte (buf, buf->size - 1) != '\n'
             && edit_query_dialog2 (_("Warning"),
                                    _("The file you are saving does not end with a newline."),
                                    _("C&ontinue"), _("&Cancel")) != 0);
}

/* --------------------------------------------------------------------------------------------- */

static vfs_path_t *
edit_get_save_file_as (WEdit * edit)
{
    static LineBreaks cur_lb = LB_ASIS;
    char *filename_res;
    vfs_path_t *ret_vpath = NULL;

    const char *lb_names[LB_NAMES] = {
        N_("&Do not change"),
        N_("&Unix format (LF)"),
        N_("&Windows/DOS format (CR LF)"),
        N_("&Macintosh format (CR)")
    };

    quick_widget_t quick_widgets[] = {
        /* *INDENT-OFF* */
        QUICK_LABELED_INPUT (N_("Enter file name:"), input_label_above,
                             vfs_path_as_str (edit->filename_vpath), "save-as",
                             &filename_res, NULL, FALSE, FALSE, INPUT_COMPLETE_FILENAMES),
        QUICK_SEPARATOR (TRUE),
        QUICK_LABEL (N_("Change line breaks to:"), NULL),
        QUICK_RADIO (LB_NAMES, lb_names, (int *) &cur_lb, NULL),
        QUICK_BUTTONS_OK_CANCEL,
        QUICK_END
        /* *INDENT-ON* */
    };

    quick_dialog_t qdlg = {
        -1, -1, 64,
        N_("Save As"), "[Save File As]",
        quick_widgets, NULL, NULL
    };

    if (quick_dialog (&qdlg) != B_CANCEL)
    {
        char *fname;

        edit->lb = cur_lb;
        fname = tilde_expand (filename_res);
        g_free (filename_res);
        ret_vpath = vfs_path_from_str (fname);
        g_free (fname);
    }

    return ret_vpath;
}

/* --------------------------------------------------------------------------------------------- */

/** returns TRUE on success */

static gboolean
edit_save_cmd (WEdit * edit)
{
    int res, save_lock = 0;

    if (!edit->locked && !edit->delete_file)
        save_lock = lock_file (edit->filename_vpath);
    res = edit_save_file (edit, edit->filename_vpath);

    /* Maintain modify (not save) lock on failure */
    if ((res > 0 && edit->locked) || save_lock)
        edit->locked = unlock_file (edit->filename_vpath);

    /* On failure try 'save as', it does locking on its own */
    if (res == 0)
        return edit_save_as_cmd (edit);

    if (res > 0)
    {
        edit->delete_file = 0;
        edit->modified = 0;
    }

    edit->force |= REDRAW_COMPLETELY;

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Load file content
 *
 * @param h screen the owner of editor window
 * @param vpath vfs file path
 * @return TRUE if file content was successfully loaded, FALSE otherwise
 */

static inline gboolean
edit_load_file_from_filename (WDialog * h, const vfs_path_t * vpath)
{
    Widget *w = WIDGET (h);

    return edit_add_window (h, w->y + 1, w->x, w->lines - 2, w->cols, vpath, 0);
}

/* --------------------------------------------------------------------------------------------- */

static void
edit_delete_column_of_text (WEdit * edit)
{
    off_t m1, m2;
    off_t n;
    long b, c, d;

    eval_marks (edit, &m1, &m2);
    n = edit_buffer_get_forward_offset (&edit->buffer, m1, 0, m2) + 1;
    c = (long) edit_move_forward3 (edit, edit_buffer_get_bol (&edit->buffer, m1), 0, m1);
    d = (long) edit_move_forward3 (edit, edit_buffer_get_bol (&edit->buffer, m2), 0, m2);
    b = MAX (MIN (c, d), MIN (edit->column1, edit->column2));
    c = MAX (c, MAX (edit->column1, edit->column2));

    while (n-- != 0)
    {
        off_t r, p, q;

        r = edit_buffer_get_current_bol (&edit->buffer);
        p = edit_move_forward3 (edit, r, b, 0);
        q = edit_move_forward3 (edit, r, c, 0);
        p = MAX (p, m1);
        q = MIN (q, m2);
        edit_cursor_move (edit, p - edit->buffer.curs1);
        /* delete line between margins */
        for (; q > p; q--)
            if (edit_buffer_get_current_byte (&edit->buffer) != '\n')
                edit_delete (edit, TRUE);

        /* move to next line except on the last delete */
        if (n != 0)
            edit_cursor_move (edit,
                              edit_buffer_get_forward_offset (&edit->buffer, edit->buffer.curs1, 1,
                                                              0) - edit->buffer.curs1);
    }
}

/* --------------------------------------------------------------------------------------------- */
/** if success return 0 */

static int
edit_block_delete (WEdit * edit)
{
    off_t start_mark, end_mark;
    off_t curs_pos;
    long curs_line, c1, c2;

    if (!eval_marks (edit, &start_mark, &end_mark))
        return 0;

    if (edit->column_highlight && edit->mark2 < 0)
        edit_mark_cmd (edit, FALSE);

    /* Warning message with a query to continue or cancel the operation */
    if ((end_mark - start_mark) > option_max_undo / 2 &&
        edit_query_dialog2 (_("Warning"),
                            ("Block is large, you may not be able to undo this action"),
                            _("C&ontinue"), _("&Cancel")) != 0)
        return 1;

    c1 = MIN (edit->column1, edit->column2);
    c2 = MAX (edit->column1, edit->column2);
    edit->column1 = c1;
    edit->column2 = c2;

    edit_push_markers (edit);

    curs_line = edit->buffer.curs_line;

    curs_pos = edit->curs_col + edit->over_col;

    /* move cursor to start of selection */
    edit_cursor_move (edit, start_mark - edit->buffer.curs1);
    edit_scroll_screen_over_cursor (edit);

    if (start_mark < end_mark)
    {
        if (edit->column_highlight)
        {
            off_t line_width;

            if (edit->mark2 < 0)
                edit_mark_cmd (edit, FALSE);
            edit_delete_column_of_text (edit);
            /* move cursor to the saved position */
            edit_move_to_line (edit, curs_line);
            /* calculate line width and cursor position before cut */
            line_width = edit_move_forward3 (edit, edit_buffer_get_current_bol (&edit->buffer), 0,
                                             edit_buffer_get_current_eol (&edit->buffer));
            if (option_cursor_beyond_eol && curs_pos > line_width)
                edit->over_col = curs_pos - line_width;
        }
        else
        {
            off_t count;

            for (count = start_mark; count < end_mark; count++)
                edit_delete (edit, TRUE);
        }
    }

    edit_set_markers (edit, 0, 0, 0, 0);
    edit->force |= REDRAW_PAGE;

    return 0;
}

/* --------------------------------------------------------------------------------------------- */
/** Return a null terminated length of text. Result must be g_free'd */

static unsigned char *
edit_get_block (WEdit * edit, off_t start, off_t finish, off_t * l)
{
    unsigned char *s, *r;

    r = s = g_malloc0 (finish - start + 1);

    if (edit->column_highlight)
    {
        *l = 0;

        /* copy from buffer, excluding chars that are out of the column 'margins' */
        for (; start < finish; start++)
        {
            int c;
            off_t x;

            x = edit_move_forward3 (edit, edit_buffer_get_bol (&edit->buffer, start), 0, start);
            c = edit_buffer_get_byte (&edit->buffer, start);
            if ((x >= edit->column1 && x < edit->column2)
                || (x >= edit->column2 && x < edit->column1) || c == '\n')
            {
                *s++ = c;
                (*l)++;
            }
        }
    }
    else
    {
        *l = finish - start;

        for (; start < finish; start++)
            *s++ = edit_buffer_get_byte (&edit->buffer, start);
    }

    *s = '\0';

    return r;
}

/* --------------------------------------------------------------------------------------------- */
/** copies a block to clipboard file */

static gboolean
edit_save_block_to_clip_file (WEdit * edit, off_t start, off_t finish)
{
    gboolean ret;
    gchar *tmp;

    tmp = mc_config_get_full_path (EDIT_HOME_CLIP_FILE);
    ret = edit_save_block (edit, tmp, start, finish);
    g_free (tmp);

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

static void
pipe_mail (const edit_buffer_t * buf, char *to, char *subject, char *cc)
{
    FILE *p = 0;
    char *s;

    to = name_quote (to, FALSE);
    subject = name_quote (subject, FALSE);
    cc = name_quote (cc, FALSE);
    s = g_strconcat ("mail -s ", subject, *cc ? " -c " : "", cc, " ", to, (char *) NULL);
    g_free (to);
    g_free (subject);
    g_free (cc);

    if (s != NULL)
    {
        p = popen (s, "w");
        g_free (s);
    }

    if (p != NULL)
    {
        off_t i;

        for (i = 0; i < buf->size; i++)
            if (fputc (edit_buffer_get_byte (buf, i), p) < 0)
                break;
        pclose (p);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
edit_insert_column_of_text (WEdit * edit, unsigned char *data, off_t size, long width,
                            off_t * start_pos, off_t * end_pos, long *col1, long *col2)
{
    off_t i, cursor;
    long col;

    cursor = edit->buffer.curs1;
    col = edit_get_col (edit);

    for (i = 0; i < size; i++)
    {
        if (data[i] != '\n')
            edit_insert (edit, data[i]);
        else
        {                       /* fill in and move to next line */
            long l;
            off_t p;

            if (edit_buffer_get_current_byte (&edit->buffer) != '\n')
            {
                for (l = width - (edit_get_col (edit) - col); l > 0; l -= space_width)
                    edit_insert (edit, ' ');
            }
            for (p = edit->buffer.curs1;; p++)
            {
                if (p == edit->buffer.size)
                {
                    edit_cursor_move (edit, edit->buffer.size - edit->buffer.curs1);
                    edit_insert_ahead (edit, '\n');
                    p++;
                    break;
                }
                if (edit_buffer_get_byte (&edit->buffer, p) == '\n')
                {
                    p++;
                    break;
                }
            }
            edit_cursor_move (edit, edit_move_forward3 (edit, p, col, 0) - edit->buffer.curs1);

            for (l = col - edit_get_col (edit); l >= space_width; l -= space_width)
                edit_insert (edit, ' ');
        }
    }

    *col1 = col;
    *col2 = col + width;
    *start_pos = cursor;
    *end_pos = edit->buffer.curs1;
    edit_cursor_move (edit, cursor - edit->buffer.curs1);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Callback for the iteration of objects in the 'editors' array.
 * Toggle syntax highlighting in editor object.
 *
 * @param data      probably WEdit object
 * @param user_data unused
 */

static void
edit_syntax_onoff_cb (void *data, void *user_data)
{
    (void) user_data;

    if (edit_widget_is_editor (CONST_WIDGET (data)))
    {
        WEdit *edit = (WEdit *) data;

        if (option_syntax_highlighting)
            edit_load_syntax (edit, NULL, edit->syntax_type);
        edit->force |= REDRAW_PAGE;
    }
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
editcmd_dialog_raw_key_query_cb (Widget * w, Widget * sender, widget_msg_t msg, int parm,
                                 void *data)
{
    WDialog *h = DIALOG (w);

    switch (msg)
    {
    case MSG_KEY:
        h->ret_value = parm;
        dlg_stop (h);
        return MSG_HANDLED;
    default:
        return dlg_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
edit_refresh_cmd (void)
{
    tty_clear_screen ();
    repaint_screen ();
    tty_keypad (TRUE);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Toggle syntax highlighting in all editor windows.
 *
 * @param h root widget for all windows
 */

void
edit_syntax_onoff_cmd (WDialog * h)
{
    option_syntax_highlighting = !option_syntax_highlighting;
    g_list_foreach (GROUP (h)->widgets, edit_syntax_onoff_cb, NULL);
    widget_draw (WIDGET (h));
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Toggle tabs showing in all editor windows.
 *
 * @param h root widget for all windows
 */

void
edit_show_tabs_tws_cmd (WDialog * h)
{
    enable_show_tabs_tws = !enable_show_tabs_tws;
    widget_draw (WIDGET (h));
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Toggle right margin showing in all editor windows.
 *
 * @param h root widget for all windows
 */

void
edit_show_margin_cmd (WDialog * h)
{
    show_right_margin = !show_right_margin;
    widget_draw (WIDGET (h));
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Toggle line numbers showing in all editor windows.
 *
 * @param h root widget for all windows
 */

void
edit_show_numbers_cmd (WDialog * h)
{
    option_line_state = !option_line_state;
    option_line_state_width = option_line_state ? LINE_STATE_WIDTH : 0;
    widget_draw (WIDGET (h));
}

/* --------------------------------------------------------------------------------------------- */

void
edit_save_mode_cmd (void)
{
    char *str_result = NULL;

    const char *str[] = {
        N_("&Quick save"),
        N_("&Safe save"),
        N_("&Do backups with following extension:")
    };

#ifdef ENABLE_NLS
    size_t i;

    for (i = 0; i < 3; i++)
        str[i] = _(str[i]);
#endif

    g_assert (option_backup_ext != NULL);

    {
        quick_widget_t quick_widgets[] = {
            /* *INDENT-OFF* */
            QUICK_RADIO (3, str, &option_save_mode, &edit_save_mode_radio_id),
            QUICK_INPUT (option_backup_ext, "edit-backup-ext", &str_result,
                         &edit_save_mode_input_id, FALSE, FALSE, INPUT_COMPLETE_NONE),
            QUICK_SEPARATOR (TRUE),
            QUICK_CHECKBOX (N_("Check &POSIX new line"), &option_check_nl_at_eof, NULL),
            QUICK_BUTTONS_OK_CANCEL,
            QUICK_END
            /* *INDENT-ON* */
        };

        quick_dialog_t qdlg = {
            -1, -1, 38,
            N_("Edit Save Mode"), "[Edit Save Mode]",
            quick_widgets, edit_save_mode_callback, NULL
        };

        if (quick_dialog (&qdlg) != B_CANCEL)
        {
            g_free (option_backup_ext);
            option_backup_ext = str_result;
        }
    }
}

/* --------------------------------------------------------------------------------------------- */

void
edit_set_filename (WEdit * edit, const vfs_path_t * name_vpath)
{
    vfs_path_free (edit->filename_vpath, TRUE);
    edit->filename_vpath = vfs_path_clone (name_vpath);

    if (edit->dir_vpath == NULL)
        edit->dir_vpath = vfs_path_clone (vfs_get_raw_current_dir ());
}

/* --------------------------------------------------------------------------------------------- */
/* Here we want to warn the users of overwriting an existing file,
   but only if they have made a change to the filename */
/* returns TRUE on success */
gboolean
edit_save_as_cmd (WEdit * edit)
{
    /* This heads the 'Save As' dialog box */
    vfs_path_t *exp_vpath;
    int save_lock = 0;
    gboolean different_filename = FALSE;
    gboolean ret = FALSE;

    if (!edit_check_newline (&edit->buffer))
        return FALSE;

    exp_vpath = edit_get_save_file_as (edit);
    edit_push_undo_action (edit, KEY_PRESS + edit->start_display);

    if (exp_vpath != NULL && vfs_path_len (exp_vpath) != 0)
    {
        int rv;

        if (!vfs_path_equal (edit->filename_vpath, exp_vpath))
        {
            int file;
            struct stat sb;

            if (mc_stat (exp_vpath, &sb) == 0 && !S_ISREG (sb.st_mode))
            {
                edit_error_dialog (_("Save as"),
                                   get_sys_error (_
                                                  ("Cannot save: destination is not a regular file")));
                goto ret;
            }

            different_filename = TRUE;
            file = mc_open (exp_vpath, O_RDONLY | O_BINARY);

            if (file == -1)
                edit->stat1.st_mode |= S_IWUSR;
            else
            {
                /* the file exists */
                mc_close (file);
                /* Overwrite the current file or cancel the operation */
                if (edit_query_dialog2
                    (_("Warning"),
                     _("A file already exists with this name"), _("&Overwrite"), _("&Cancel")))
                    goto ret;
            }

            save_lock = lock_file (exp_vpath);
        }
        else if (!edit->locked && !edit->delete_file)
            /* filenames equal, check if already locked */
            save_lock = lock_file (exp_vpath);

        if (different_filename)
            /* Allow user to write into saved (under another name) file
             * even if original file had r/o user permissions. */
            edit->stat1.st_mode |= S_IWUSR;

        rv = edit_save_file (edit, exp_vpath);
        switch (rv)
        {
        case 1:
            /* Successful, so unlock both files */
            if (different_filename)
            {
                if (save_lock)
                    unlock_file (exp_vpath);
                if (edit->locked)
                    edit->locked = unlock_file (edit->filename_vpath);
            }
            else if (edit->locked || save_lock)
                edit->locked = unlock_file (edit->filename_vpath);

            edit_set_filename (edit, exp_vpath);
            if (edit->lb != LB_ASIS)
                edit_reload (edit, exp_vpath);
            edit->modified = 0;
            edit->delete_file = 0;
            if (different_filename)
                edit_load_syntax (edit, NULL, edit->syntax_type);
            ret = TRUE;
            break;

        default:
            edit_error_dialog (_("Save as"), get_sys_error (_("Cannot save file")));
            MC_FALLTHROUGH;

        case -1:
            /* Failed, so maintain modify (not save) lock */
            if (save_lock)
                unlock_file (exp_vpath);
            break;
        }
    }

  ret:
    vfs_path_free (exp_vpath, TRUE);
    edit->force |= REDRAW_COMPLETELY;
    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/** returns TRUE on success */

gboolean
edit_save_confirm_cmd (WEdit * edit)
{
    if (edit->filename_vpath == NULL)
        return edit_save_as_cmd (edit);

    if (!edit_check_newline (&edit->buffer))
        return FALSE;

    if (edit_confirm_save)
    {
        char *f;
        gboolean ok;

        f = g_strdup_printf (_("Confirm save file: \"%s\""),
                             vfs_path_as_str (edit->filename_vpath));
        ok = (edit_query_dialog2 (_("Save file"), f, _("&Save"), _("&Cancel")) == 0);
        g_free (f);
        if (!ok)
            return FALSE;
    }

    return edit_save_cmd (edit);
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Ask file to edit and load it.
  *
  * @return TRUE on success or cancel of ask.
  */

gboolean
edit_load_cmd (WDialog * h)
{
    char *exp;
    gboolean ret = TRUE;        /* possible cancel */

    exp = input_expand_dialog (_("Load"), _("Enter file name:"),
                               MC_HISTORY_EDIT_LOAD, INPUT_LAST_TEXT,
                               INPUT_COMPLETE_FILENAMES | INPUT_COMPLETE_CD);

    if (exp != NULL && *exp != '\0')
    {
        vfs_path_t *exp_vpath;

        exp_vpath = vfs_path_from_str (exp);
        ret = edit_load_file_from_filename (h, exp_vpath);
        vfs_path_free (exp_vpath, TRUE);
    }

    g_free (exp);

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Show history od edited or viewed files and open selected file.
  *
  * @return TRUE on success, FALSE otherwise.
  */

gboolean
edit_load_file_from_history (WDialog * h)
{
    char *exp;
    int action;
    gboolean ret = TRUE;        /* possible cancel */

    exp = show_file_history (CONST_WIDGET (h), &action);
    if (exp != NULL && (action == CK_Edit || action == CK_Enter))
    {
        vfs_path_t *exp_vpath;

        exp_vpath = vfs_path_from_str (exp);
        ret = edit_load_file_from_filename (h, exp_vpath);
        vfs_path_free (exp_vpath, TRUE);
    }

    g_free (exp);

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Load syntax file to edit.
  *
  * @return TRUE on success
  */

gboolean
edit_load_syntax_file (WDialog * h)
{
    vfs_path_t *extdir_vpath;
    int dir = 0;
    gboolean ret = FALSE;

    if (geteuid () == 0)
        dir = query_dialog (_("Syntax file edit"),
                            _("Which syntax file you want to edit?"), D_NORMAL, 2,
                            _("&User"), _("&System wide"));

    extdir_vpath =
        vfs_path_build_filename (mc_global.sysconfig_dir, "syntax", "Syntax", (char *) NULL);
    if (!exist_file (vfs_path_get_last_path_str (extdir_vpath)))
    {
        vfs_path_free (extdir_vpath, TRUE);
        extdir_vpath =
            vfs_path_build_filename (mc_global.share_data_dir, "syntax", "Syntax", (char *) NULL);
    }

    if (dir == 0)
    {
        vfs_path_t *user_syntax_file_vpath;

        user_syntax_file_vpath = mc_config_get_full_vpath (EDIT_HOME_SYNTAX_FILE);
        check_for_default (extdir_vpath, user_syntax_file_vpath);
        ret = edit_load_file_from_filename (h, user_syntax_file_vpath);
        vfs_path_free (user_syntax_file_vpath, TRUE);
    }
    else if (dir == 1)
        ret = edit_load_file_from_filename (h, extdir_vpath);

    vfs_path_free (extdir_vpath, TRUE);

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Load menu file to edit.
  *
  * @return TRUE on success
  */

gboolean
edit_load_menu_file (WDialog * h)
{
    vfs_path_t *buffer_vpath;
    vfs_path_t *menufile_vpath;
    int dir;
    gboolean ret;

    query_set_sel (1);
    dir = query_dialog (_("Menu edit"),
                        _("Which menu file do you want to edit?"), D_NORMAL,
                        geteuid () != 0 ? 2 : 3, _("&Local"), _("&User"), _("&System wide"));

    menufile_vpath =
        vfs_path_build_filename (mc_global.sysconfig_dir, EDIT_GLOBAL_MENU, (char *) NULL);
    if (!exist_file (vfs_path_get_last_path_str (menufile_vpath)))
    {
        vfs_path_free (menufile_vpath, TRUE);
        menufile_vpath =
            vfs_path_build_filename (mc_global.share_data_dir, EDIT_GLOBAL_MENU, (char *) NULL);
    }

    switch (dir)
    {
    case 0:
        buffer_vpath = vfs_path_from_str (EDIT_LOCAL_MENU);
        check_for_default (menufile_vpath, buffer_vpath);
        chmod (vfs_path_get_last_path_str (buffer_vpath), 0600);
        break;

    case 1:
        buffer_vpath = mc_config_get_full_vpath (EDIT_HOME_MENU);
        check_for_default (menufile_vpath, buffer_vpath);
        break;

    case 2:
        buffer_vpath =
            vfs_path_build_filename (mc_global.sysconfig_dir, EDIT_GLOBAL_MENU, (char *) NULL);
        if (!exist_file (vfs_path_get_last_path_str (buffer_vpath)))
        {
            vfs_path_free (buffer_vpath, TRUE);
            buffer_vpath =
                vfs_path_build_filename (mc_global.share_data_dir, EDIT_GLOBAL_MENU, (char *) NULL);
        }
        break;

    default:
        vfs_path_free (menufile_vpath, TRUE);
        return FALSE;
    }

    ret = edit_load_file_from_filename (h, buffer_vpath);

    vfs_path_free (buffer_vpath, TRUE);
    vfs_path_free (menufile_vpath, TRUE);

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Close window with opened file.
  *
  * @return TRUE if file was closed.
  */

gboolean
edit_close_cmd (WEdit * edit)
{
    gboolean ret;

    ret = (edit != NULL) && edit_ok_to_exit (edit);

    if (ret)
    {
        Widget *w = WIDGET (edit);
        WGroup *g = w->owner;

        if (edit->locked != 0)
            unlock_file (edit->filename_vpath);

        group_remove_widget (w);
        widget_destroy (w);

        if (edit_widget_is_editor (CONST_WIDGET (g->current->data)))
            edit = (WEdit *) (g->current->data);
        else
        {
            edit = find_editor (DIALOG (g));
            if (edit != NULL)
                widget_select (WIDGET (edit));
        }
    }

    if (edit != NULL)
        edit->force |= REDRAW_COMPLETELY;

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/**
   if mark2 is -1 then marking is from mark1 to the cursor.
   Otherwise its between the markers. This handles this.
   Returns FALSE if no text is marked.
 */

gboolean
eval_marks (WEdit * edit, off_t * start_mark, off_t * end_mark)
{
    long end_mark_curs;

    if (edit->mark1 == edit->mark2)
    {
        *start_mark = *end_mark = 0;
        edit->column2 = edit->column1 = 0;
        return FALSE;
    }

    if (edit->end_mark_curs < 0)
        end_mark_curs = edit->buffer.curs1;
    else
        end_mark_curs = edit->end_mark_curs;

    if (edit->mark2 >= 0)
    {
        *start_mark = MIN (edit->mark1, edit->mark2);
        *end_mark = MAX (edit->mark1, edit->mark2);
    }
    else
    {
        *start_mark = MIN (edit->mark1, end_mark_curs);
        *end_mark = MAX (edit->mark1, end_mark_curs);
        edit->column2 = edit->curs_col + edit->over_col;
    }

    if (edit->column_highlight
        && ((edit->mark1 > end_mark_curs && edit->column1 < edit->column2)
            || (edit->mark1 < end_mark_curs && edit->column1 > edit->column2)))
    {
        off_t start_bol, start_eol;
        off_t end_bol, end_eol;
        long col1, col2;
        off_t diff1, diff2;

        start_bol = edit_buffer_get_bol (&edit->buffer, *start_mark);
        start_eol = edit_buffer_get_eol (&edit->buffer, start_bol - 1) + 1;
        end_bol = edit_buffer_get_bol (&edit->buffer, *end_mark);
        end_eol = edit_buffer_get_eol (&edit->buffer, *end_mark);
        col1 = MIN (edit->column1, edit->column2);
        col2 = MAX (edit->column1, edit->column2);

        diff1 = edit_move_forward3 (edit, start_bol, col2, 0) -
            edit_move_forward3 (edit, start_bol, col1, 0);
        diff2 = edit_move_forward3 (edit, end_bol, col2, 0) -
            edit_move_forward3 (edit, end_bol, col1, 0);

        *start_mark -= diff1;
        *end_mark += diff2;
        *start_mark = MAX (*start_mark, start_eol);
        *end_mark = MIN (*end_mark, end_eol);
    }

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

void
edit_block_copy_cmd (WEdit * edit)
{
    off_t start_mark, end_mark, current = edit->buffer.curs1;
    off_t mark1 = 0, mark2 = 0;
    long c1 = 0, c2 = 0;
    off_t size;
    unsigned char *copy_buf;

    edit_update_curs_col (edit);
    if (!eval_marks (edit, &start_mark, &end_mark))
        return;

    copy_buf = edit_get_block (edit, start_mark, end_mark, &size);

    /* all that gets pushed are deletes hence little space is used on the stack */

    edit_push_markers (edit);

    if (edit->column_highlight)
    {
        long col_delta;

        col_delta = labs (edit->column2 - edit->column1);
        edit_insert_column_of_text (edit, copy_buf, size, col_delta, &mark1, &mark2, &c1, &c2);
    }
    else
    {
        int size_orig = size;

        while (size-- != 0)
            edit_insert_ahead (edit, copy_buf[size]);

        /* Place cursor at the end of text selection */
        if (option_cursor_after_inserted_block)
            edit_cursor_move (edit, size_orig);
    }

    g_free (copy_buf);
    edit_scroll_screen_over_cursor (edit);

    if (edit->column_highlight)
        edit_set_markers (edit, edit->buffer.curs1, mark2, c1, c2);
    else if (start_mark < current && end_mark > current)
        edit_set_markers (edit, start_mark, end_mark + end_mark - start_mark, 0, 0);

    edit->force |= REDRAW_PAGE;
}


/* --------------------------------------------------------------------------------------------- */

void
edit_block_move_cmd (WEdit * edit)
{
    off_t current;
    unsigned char *copy_buf = NULL;
    off_t start_mark, end_mark;

    if (!eval_marks (edit, &start_mark, &end_mark))
        return;

    if (!edit->column_highlight && edit->buffer.curs1 > start_mark && edit->buffer.curs1 < end_mark)
        return;

    if (edit->mark2 < 0)
        edit_mark_cmd (edit, FALSE);
    edit_push_markers (edit);

    if (edit->column_highlight)
    {
        off_t mark1, mark2;
        off_t size;
        long c1, c2, b_width;
        long x, x2;

        c1 = MIN (edit->column1, edit->column2);
        c2 = MAX (edit->column1, edit->column2);
        b_width = c2 - c1;

        edit_update_curs_col (edit);

        x = edit->curs_col;
        x2 = x + edit->over_col;

        /* do nothing when cursor inside first line of selected area */
        if ((edit_buffer_get_eol (&edit->buffer, edit->buffer.curs1) ==
             edit_buffer_get_eol (&edit->buffer, start_mark)) && x2 > c1 && x2 <= c2)
            return;

        if (edit->buffer.curs1 > start_mark
            && edit->buffer.curs1 < edit_buffer_get_eol (&edit->buffer, end_mark))
        {
            if (x > c2)
                x -= b_width;
            else if (x > c1 && x <= c2)
                x = c1;
        }
        /* save current selection into buffer */
        copy_buf = edit_get_block (edit, start_mark, end_mark, &size);

        /* remove current selection */
        edit_block_delete_cmd (edit);

        edit->over_col = MAX (0, edit->over_col - b_width);
        /* calculate the cursor pos after delete block */
        current = edit_move_forward3 (edit, edit_buffer_get_current_bol (&edit->buffer), x, 0);
        edit_cursor_move (edit, current - edit->buffer.curs1);
        edit_scroll_screen_over_cursor (edit);

        /* add TWS if need before block insertion */
        if (option_cursor_beyond_eol && edit->over_col > 0)
            edit_insert_over (edit);

        edit_insert_column_of_text (edit, copy_buf, size, b_width, &mark1, &mark2, &c1, &c2);
        edit_set_markers (edit, mark1, mark2, c1, c2);
    }
    else
    {
        off_t count, count_orig;

        current = edit->buffer.curs1;
        copy_buf = g_malloc0 (end_mark - start_mark);
        edit_cursor_move (edit, start_mark - edit->buffer.curs1);
        edit_scroll_screen_over_cursor (edit);

        for (count = start_mark; count < end_mark; count++)
            copy_buf[end_mark - count - 1] = edit_delete (edit, TRUE);

        edit_scroll_screen_over_cursor (edit);
        edit_cursor_move (edit,
                          current - edit->buffer.curs1 -
                          (((current - edit->buffer.curs1) > 0) ? end_mark - start_mark : 0));
        edit_scroll_screen_over_cursor (edit);
        count_orig = count;
        while (count-- > start_mark)
            edit_insert_ahead (edit, copy_buf[end_mark - count - 1]);

        edit_set_markers (edit, edit->buffer.curs1, edit->buffer.curs1 + end_mark - start_mark, 0,
                          0);

        /* Place cursor at the end of text selection */
        if (option_cursor_after_inserted_block)
            edit_cursor_move (edit, count_orig - start_mark);
    }

    edit_scroll_screen_over_cursor (edit);
    g_free (copy_buf);
    edit->force |= REDRAW_PAGE;
}

/* --------------------------------------------------------------------------------------------- */
/** returns 1 if canceelled by user */

int
edit_block_delete_cmd (WEdit * edit)
{
    off_t start_mark, end_mark;

    if (eval_marks (edit, &start_mark, &end_mark))
        return edit_block_delete (edit);

    edit_delete_line (edit);

    return 0;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Check if it's OK to close the file. If there are unsaved changes, ask user.
  *
  * @return TRUE if it's OK to exit, FALSE to continue editing.
  */

gboolean
edit_ok_to_exit (WEdit * edit)
{
    const char *fname = N_("[NoName]");
    char *msg;
    int act;

    if (!edit->modified)
        return TRUE;

    if (edit->filename_vpath != NULL)
        fname = vfs_path_as_str (edit->filename_vpath);
#ifdef ENABLE_NLS
    else
        fname = _(fname);
#endif

    if (!mc_global.midnight_shutdown)
    {
        query_set_sel (2);

        msg = g_strdup_printf (_("File %s was modified.\nSave before close?"), fname);
        act = edit_query_dialog3 (_("Close file"), msg, _("&Yes"), _("&No"), _("&Cancel"));
    }
    else
    {
        msg = g_strdup_printf (_("Midnight Commander is being shut down.\nSave modified file %s?"),
                               fname);
        act = edit_query_dialog2 (_("Quit"), msg, _("&Yes"), _("&No"));

        /* Esc is No */
        if (act == -1)
            act = 1;
    }

    g_free (msg);

    switch (act)
    {
    case 0:                    /* Yes */
        if (!mc_global.midnight_shutdown && !edit_check_newline (&edit->buffer))
            return FALSE;
        edit_push_markers (edit);
        edit_set_markers (edit, 0, 0, 0, 0);
        if (!edit_save_cmd (edit) || mc_global.midnight_shutdown)
            return mc_global.midnight_shutdown;
        break;
    case 1:                    /* No */
    default:
        break;
    case 2:                    /* Cancel quit */
    case -1:                   /* Esc */
        return FALSE;
    }

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */
/** save block, returns TRUE on success */

gboolean
edit_save_block (WEdit * edit, const char *filename, off_t start, off_t finish)
{
    int file;
    off_t len = 1;
    vfs_path_t *vpath;

    vpath = vfs_path_from_str (filename);
    file = mc_open (vpath, O_CREAT | O_WRONLY | O_TRUNC,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | O_BINARY);
    vfs_path_free (vpath, TRUE);
    if (file == -1)
        return FALSE;

    if (edit->column_highlight)
    {
        int r;

        r = mc_write (file, VERTICAL_MAGIC, sizeof (VERTICAL_MAGIC));
        if (r > 0)
        {
            unsigned char *block, *p;

            p = block = edit_get_block (edit, start, finish, &len);
            while (len != 0)
            {
                r = mc_write (file, p, len);
                if (r < 0)
                    break;
                p += r;
                len -= r;
            }
            g_free (block);
        }
    }
    else
    {
        unsigned char *buf;
        off_t i = start;

        len = finish - start;
        buf = g_malloc0 (TEMP_BUF_LEN);
        while (start != finish)
        {
            off_t end;

            end = MIN (finish, start + TEMP_BUF_LEN);
            for (; i < end; i++)
                buf[i - start] = edit_buffer_get_byte (&edit->buffer, i);
            len -= mc_write (file, (char *) buf, end - start);
            start = end;
        }
        g_free (buf);
    }
    mc_close (file);

    return (len == 0);
}

/* --------------------------------------------------------------------------------------------- */

void
edit_paste_from_history (WEdit * edit)
{
    (void) edit;
    edit_error_dialog (_("Error"), _("This function is not implemented"));
}

/* --------------------------------------------------------------------------------------------- */

gboolean
edit_copy_to_X_buf_cmd (WEdit * edit)
{
    off_t start_mark, end_mark;

    if (!eval_marks (edit, &start_mark, &end_mark))
        return TRUE;

    if (!edit_save_block_to_clip_file (edit, start_mark, end_mark))
    {
        edit_error_dialog (_("Copy to clipboard"), get_sys_error (_("Unable to save to file")));
        return FALSE;
    }
    /* try use external clipboard utility */
    mc_event_raise (MCEVENT_GROUP_CORE, "clipboard_file_to_ext_clip", NULL);

    if (option_drop_selection_on_copy)
        edit_mark_cmd (edit, TRUE);

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

gboolean
edit_cut_to_X_buf_cmd (WEdit * edit)
{
    off_t start_mark, end_mark;

    if (!eval_marks (edit, &start_mark, &end_mark))
        return TRUE;

    if (!edit_save_block_to_clip_file (edit, start_mark, end_mark))
    {
        edit_error_dialog (_("Cut to clipboard"), _("Unable to save to file"));
        return FALSE;
    }
    /* try use external clipboard utility */
    mc_event_raise (MCEVENT_GROUP_CORE, "clipboard_file_to_ext_clip", NULL);

    edit_block_delete_cmd (edit);
    edit_mark_cmd (edit, TRUE);

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

gboolean
edit_paste_from_X_buf_cmd (WEdit * edit)
{
    vfs_path_t *tmp;
    gboolean ret;

    /* try use external clipboard utility */
    mc_event_raise (MCEVENT_GROUP_CORE, "clipboard_file_from_ext_clip", NULL);
    tmp = mc_config_get_full_vpath (EDIT_HOME_CLIP_FILE);
    ret = (edit_insert_file (edit, tmp) >= 0);
    vfs_path_free (tmp, TRUE);

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Ask user for the line and go to that line.
 * Negative numbers mean line from the end (i.e. -1 is the last line).
 */

void
edit_goto_cmd (WEdit * edit)
{
    static gboolean first_run = TRUE;

    char *f;
    long l;
    char *error;

    f = input_dialog (_("Goto line"), _("Enter line:"), MC_HISTORY_EDIT_GOTO_LINE,
                      first_run ? NULL : INPUT_LAST_TEXT, INPUT_COMPLETE_NONE);
    if (f == NULL || *f == '\0')
    {
        g_free (f);
        return;
    }

    l = strtol (f, &error, 0);
    if (*error != '\0')
    {
        g_free (f);
        return;
    }

    if (l < 0)
        l = edit->buffer.lines + l + 2;

    edit_move_display (edit, l - WIDGET (edit)->lines / 2 - 1);
    edit_move_to_line (edit, l - 1);
    edit->force |= REDRAW_COMPLETELY;

    g_free (f);
    first_run = FALSE;
}

/* --------------------------------------------------------------------------------------------- */
/** Return TRUE on success */

gboolean
edit_save_block_cmd (WEdit * edit)
{
    off_t start_mark, end_mark;
    char *exp, *tmp;
    gboolean ret = FALSE;

    if (!eval_marks (edit, &start_mark, &end_mark))
        return TRUE;

    tmp = mc_config_get_full_path (EDIT_HOME_CLIP_FILE);
    exp =
        input_expand_dialog (_("Save block"), _("Enter file name:"),
                             MC_HISTORY_EDIT_SAVE_BLOCK, tmp, INPUT_COMPLETE_FILENAMES);
    g_free (tmp);
    edit_push_undo_action (edit, KEY_PRESS + edit->start_display);

    if (exp != NULL && *exp != '\0')
    {
        if (edit_save_block (edit, exp, start_mark, end_mark))
            ret = TRUE;
        else
            edit_error_dialog (_("Save block"), get_sys_error (_("Cannot save file")));

        edit->force |= REDRAW_COMPLETELY;
    }

    g_free (exp);

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

/** returns TRUE on success */
gboolean
edit_insert_file_cmd (WEdit * edit)
{
    char *tmp;
    char *exp;
    gboolean ret = FALSE;

    tmp = mc_config_get_full_path (EDIT_HOME_CLIP_FILE);
    exp = input_expand_dialog (_("Insert file"), _("Enter file name:"),
                               MC_HISTORY_EDIT_INSERT_FILE, tmp, INPUT_COMPLETE_FILENAMES);
    g_free (tmp);

    edit_push_undo_action (edit, KEY_PRESS + edit->start_display);

    if (exp != NULL && *exp != '\0')
    {
        vfs_path_t *exp_vpath;

        exp_vpath = vfs_path_from_str (exp);
        ret = (edit_insert_file (edit, exp_vpath) >= 0);
        vfs_path_free (exp_vpath, TRUE);

        if (!ret)
            edit_error_dialog (_("Insert file"), get_sys_error (_("Cannot insert file")));
    }

    g_free (exp);

    edit->force |= REDRAW_COMPLETELY;
    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/** sorts a block, returns -1 on system fail, 1 on cancel and 0 on success */

int
edit_sort_cmd (WEdit * edit)
{
    char *exp, *tmp, *tmp_edit_block_name, *tmp_edit_temp_name;
    off_t start_mark, end_mark;
    int e;

    if (!eval_marks (edit, &start_mark, &end_mark))
    {
        edit_error_dialog (_("Sort block"), _("You must first highlight a block of text"));
        return 0;
    }

    tmp = mc_config_get_full_path (EDIT_HOME_BLOCK_FILE);
    edit_save_block (edit, tmp, start_mark, end_mark);
    g_free (tmp);

    exp = input_dialog (_("Run sort"),
                        _("Enter sort options (see manpage) separated by whitespace:"),
                        MC_HISTORY_EDIT_SORT, INPUT_LAST_TEXT, INPUT_COMPLETE_NONE);

    if (exp == NULL)
        return 1;

    tmp_edit_block_name = mc_config_get_full_path (EDIT_HOME_BLOCK_FILE);
    tmp_edit_temp_name = mc_config_get_full_path (EDIT_HOME_TEMP_FILE);
    tmp =
        g_strconcat (" sort ", exp, " ", tmp_edit_block_name,
                     " > ", tmp_edit_temp_name, (char *) NULL);
    g_free (tmp_edit_temp_name);
    g_free (tmp_edit_block_name);
    g_free (exp);

    e = system (tmp);
    g_free (tmp);
    if (e != 0)
    {
        if (e == -1 || e == 127)
            edit_error_dialog (_("Sort"), get_sys_error (_("Cannot execute sort command")));
        else
        {
            char q[8];

            sprintf (q, "%d ", e);
            tmp = g_strdup_printf (_("Sort returned non-zero: %s"), q);
            edit_error_dialog (_("Sort"), tmp);
            g_free (tmp);
        }

        return -1;
    }

    edit->force |= REDRAW_COMPLETELY;

    if (edit_block_delete_cmd (edit))
        return 1;

    {
        vfs_path_t *tmp_vpath;

        tmp_vpath = mc_config_get_full_vpath (EDIT_HOME_TEMP_FILE);
        edit_insert_file (edit, tmp_vpath);
        vfs_path_free (tmp_vpath, TRUE);
    }

    return 0;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Ask user for a command, execute it and paste its output back to the
 * editor.
 */

int
edit_ext_cmd (WEdit * edit)
{
    char *exp, *tmp, *tmp_edit_temp_file;
    int e;

    exp =
        input_dialog (_("Paste output of external command"),
                      _("Enter shell command(s):"), MC_HISTORY_EDIT_PASTE_EXTCMD, INPUT_LAST_TEXT,
                      INPUT_COMPLETE_FILENAMES | INPUT_COMPLETE_VARIABLES | INPUT_COMPLETE_USERNAMES
                      | INPUT_COMPLETE_HOSTNAMES | INPUT_COMPLETE_CD | INPUT_COMPLETE_COMMANDS |
                      INPUT_COMPLETE_SHELL_ESC);

    if (!exp)
        return 1;

    tmp_edit_temp_file = mc_config_get_full_path (EDIT_HOME_TEMP_FILE);
    tmp = g_strconcat (exp, " > ", tmp_edit_temp_file, (char *) NULL);
    g_free (tmp_edit_temp_file);
    e = system (tmp);
    g_free (tmp);
    g_free (exp);

    if (e != 0)
    {
        edit_error_dialog (_("External command"), get_sys_error (_("Cannot execute command")));
        return -1;
    }

    edit->force |= REDRAW_COMPLETELY;

    {
        vfs_path_t *tmp_vpath;

        tmp_vpath = mc_config_get_full_vpath (EDIT_HOME_TEMP_FILE);
        edit_insert_file (edit, tmp_vpath);
        vfs_path_free (tmp_vpath, TRUE);
    }

    return 0;
}

/* --------------------------------------------------------------------------------------------- */
/** if block is 1, a block must be highlighted and the shell command
   processes it. If block is 0 the shell command is a straight system
   command, that just produces some output which is to be inserted */

void
edit_block_process_cmd (WEdit * edit, int macro_number)
{
    char *fname;
    char *macros_fname = NULL;

    fname = g_strdup_printf ("%s.%i.sh", EDIT_HOME_MACRO_FILE, macro_number);
    macros_fname = g_build_filename (mc_config_get_data_path (), fname, (char *) NULL);
    user_menu (edit, macros_fname, 0);
    g_free (fname);
    g_free (macros_fname);
    edit->force |= REDRAW_COMPLETELY;
}

/* --------------------------------------------------------------------------------------------- */

void
edit_mail_dialog (WEdit * edit)
{
    char *mail_to, *mail_subject, *mail_cc;

    quick_widget_t quick_widgets[] = {
        /* *INDENT-OFF* */
        QUICK_LABEL (N_("mail -s <subject> -c <cc> <to>"), NULL),
        QUICK_LABELED_INPUT (N_("To"), input_label_above,
                             INPUT_LAST_TEXT, "mail-dlg-input-3",
                             &mail_to, NULL, FALSE, FALSE, INPUT_COMPLETE_USERNAMES),
        QUICK_LABELED_INPUT (N_("Subject"), input_label_above,
                              INPUT_LAST_TEXT, "mail-dlg-input-2",
                              &mail_subject, NULL, FALSE, FALSE, INPUT_COMPLETE_NONE),
        QUICK_LABELED_INPUT (N_("Copies to"), input_label_above,
                             INPUT_LAST_TEXT, "mail-dlg-input",
                             &mail_cc, NULL, FALSE, FALSE, INPUT_COMPLETE_USERNAMES),
        QUICK_BUTTONS_OK_CANCEL,
        QUICK_END
        /* *INDENT-ON* */
    };

    quick_dialog_t qdlg = {
        -1, -1, 50,
        N_("Mail"), "[Input Line Keys]",
        quick_widgets, NULL, NULL
    };

    if (quick_dialog (&qdlg) != B_CANCEL)
    {
        pipe_mail (&edit->buffer, mail_to, mail_subject, mail_cc);
        g_free (mail_to);
        g_free (mail_subject);
        g_free (mail_cc);
    }
}

/* --------------------------------------------------------------------------------------------- */

#ifdef HAVE_CHARSET
void
edit_select_codepage_cmd (WEdit * edit)
{
    if (do_select_codepage ())
        edit_set_codeset (edit);

    edit->force = REDRAW_PAGE;
    widget_draw (WIDGET (edit));
}
#endif

/* --------------------------------------------------------------------------------------------- */

void
edit_insert_literal_cmd (WEdit * edit)
{
    int char_for_insertion;

    char_for_insertion = editcmd_dialog_raw_key_query (_("Insert literal"),
                                                       _("Press any key:"), FALSE);
    edit_execute_key_command (edit, -1, ascii_alpha_to_cntrl (char_for_insertion));
}

/* --------------------------------------------------------------------------------------------- */

gboolean
edit_load_forward_cmd (WEdit * edit)
{
    if (edit->modified
        && edit_query_dialog2 (_("Warning"),
                               _("Current text was modified without a file save.\n"
                                 "Continue discards these changes."), _("C&ontinue"),
                               _("&Cancel")) == 1)
    {
        edit->force |= REDRAW_COMPLETELY;
        return TRUE;
    }

    if (edit_stack_iterator + 1 >= MAX_HISTORY_MOVETO)
        return FALSE;

    if (edit_history_moveto[edit_stack_iterator + 1].line < 1)
        return FALSE;

    edit_stack_iterator++;
    if (edit_history_moveto[edit_stack_iterator].filename_vpath != NULL)
        return edit_reload_line (edit, edit_history_moveto[edit_stack_iterator].filename_vpath,
                                 edit_history_moveto[edit_stack_iterator].line);

    return FALSE;
}

/* --------------------------------------------------------------------------------------------- */

gboolean
edit_load_back_cmd (WEdit * edit)
{
    if (edit->modified
        && edit_query_dialog2 (_("Warning"),
                               _("Current text was modified without a file save.\n"
                                 "Continue discards these changes."), _("C&ontinue"),
                               _("&Cancel")) == 1)
    {
        edit->force |= REDRAW_COMPLETELY;
        return TRUE;
    }

    /* we are in the bottom of the stack, NO WAY! */
    if (edit_stack_iterator == 0)
        return FALSE;

    edit_stack_iterator--;
    if (edit_history_moveto[edit_stack_iterator].filename_vpath != NULL)
        return edit_reload_line (edit, edit_history_moveto[edit_stack_iterator].filename_vpath,
                                 edit_history_moveto[edit_stack_iterator].line);

    return FALSE;
}

/* --------------------------------------------------------------------------------------------- */

/* gets a raw key from the keyboard. Passing cancel = 1 draws
   a cancel button thus allowing c-c etc.  Alternatively, cancel = 0
   will return the next key pressed.  ctrl-a (=B_CANCEL), ctrl-g, ctrl-c,
   and Esc are cannot returned */

int
editcmd_dialog_raw_key_query (const char *heading, const char *query, gboolean cancel)
{
    int w, wq;
    int y = 2;
    WDialog *raw_dlg;
    WGroup *g;

    w = str_term_width1 (heading) + 6;
    wq = str_term_width1 (query);
    w = MAX (w, wq + 3 * 2 + 1 + 2);

    raw_dlg =
        dlg_create (TRUE, 0, 0, cancel ? 7 : 5, w, WPOS_CENTER | WPOS_TRYUP, FALSE, dialog_colors,
                    editcmd_dialog_raw_key_query_cb, NULL, NULL, heading);
    g = GROUP (raw_dlg);
    widget_want_tab (WIDGET (raw_dlg), TRUE);

    group_add_widget (g, label_new (y, 3, query));
    group_add_widget (g,
                      input_new (y++, 3 + wq + 1, input_colors, w - (6 + wq + 1), "", 0,
                                 INPUT_COMPLETE_NONE));
    if (cancel)
    {
        group_add_widget (g, hline_new (y++, -1, -1));
        /* Button w/o hotkey to allow use any key as raw or macro one */
        group_add_widget_autopos (g, button_new (y, 1, B_CANCEL, NORMAL_BUTTON, _("Cancel"), NULL),
                                  WPOS_KEEP_TOP | WPOS_CENTER_HORZ, NULL);
    }

    w = dlg_run (raw_dlg);
    widget_destroy (WIDGET (raw_dlg));

    return (cancel && (w == ESC_CHAR || w == B_CANCEL)) ? 0 : w;
}

/* --------------------------------------------------------------------------------------------- */
