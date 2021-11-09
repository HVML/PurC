/*
   Load and show history of edited and viewed files

   Copyright (C) 2020-2021
   Free Software Foundation, Inc.

   Written by:
   Andrew Borodin <aborodin@vmail.ru>, 2019.

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

#include <stdio.h>              /* file functions */

#include "lib/global.h"

#include "lib/fileloc.h"        /* MC_FILEPOS_FILE */
#include "lib/mcconfig.h"       /* mc_config_get_full_path() */
#include "lib/strutil.h"        /* str_term_width1() */
#include "lib/util.h"           /* backup functions */

#include "file_history.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

#define TMP_SUFFIX ".tmp"

/*** file scope type declarations ****************************************************************/

typedef struct file_history_data_t
{
    char *file_name;
    char *file_pos;
} file_history_data_t;

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static GList *
file_history_list_read (void)
{
    char *fn;
    FILE *f;
    char buf[MC_MAXPATHLEN + 100];
    GList *file_list = NULL;

    /* open file with positions */
    fn = mc_config_get_full_path (MC_FILEPOS_FILE);
    if (fn == NULL)
        return NULL;

    f = fopen (fn, "r");
    g_free (fn);
    if (f == NULL)
        return NULL;

    while (fgets (buf, sizeof (buf), f) != NULL)
    {
        char *s;
        file_history_data_t *fhd;
        size_t len;

        s = strrchr (buf, ' ');
        /* FIXME: saved file position info is present in filepos file */
        fhd = g_new (file_history_data_t, 1);
        fhd->file_name = g_strndup (buf, s - buf);
        len = strlen (s + 1);
        fhd->file_pos = g_strndup (s + 1, len - 1);     /* ignore '\n' */
        file_list = g_list_prepend (file_list, fhd);
    }

    fclose (f);

    return file_list;
}

/* --------------------------------------------------------------------------------------------- */

static void
file_history_list_write (const GList * file_list)
{
    char *fn;
    FILE *f;
    gboolean write_error = FALSE;

    fn = mc_config_get_full_path (MC_FILEPOS_FILE);
    if (fn == NULL)
        return;

    mc_util_make_backup_if_possible (fn, TMP_SUFFIX);

    f = fopen (fn, "w");
    if (f != NULL)
    {
        GString *s;

        s = g_string_sized_new (128);

        for (; file_list != NULL && !write_error; file_list = g_list_next (file_list))
        {
            file_history_data_t *fhd = (file_history_data_t *) file_list->data;

            g_string_append (s, fhd->file_name);
            if (fhd->file_pos != NULL)
            {
                g_string_append_c (s, ' ');
                g_string_append (s, fhd->file_pos);
            }

            write_error = (fprintf (f, "%s\n", s->str) < 0);
            g_string_truncate (s, 0);
        }

        g_string_free (s, TRUE);

        fclose (f);
    }

    if (write_error)
        mc_util_restore_from_backup_if_possible (fn, TMP_SUFFIX);
    else
        mc_util_unlink_backup_if_possible (fn, TMP_SUFFIX);

    g_free (fn);
}

/* --------------------------------------------------------------------------------------------- */

static void
file_history_create_item (history_descriptor_t * hd, void *data)
{
    file_history_data_t *fhd = (file_history_data_t *) data;
    size_t width;

    width = str_term_width1 (fhd->file_name);
    hd->max_width = MAX (width, hd->max_width);

    listbox_add_item (hd->listbox, LISTBOX_APPEND_AT_END, 0, fhd->file_name, fhd->file_pos, TRUE);
    /* fhd->file_pos is not copied, NULLize it to prevent double free */
    fhd->file_pos = NULL;
}

/* --------------------------------------------------------------------------------------------- */

static void *
file_history_release_item (history_descriptor_t * hd, WLEntry * le)
{
    file_history_data_t *fhd;

    (void) hd;

    fhd = g_new (file_history_data_t, 1);
    fhd->file_name = le->text;
    le->text = NULL;
    fhd->file_pos = (char *) le->data;
    le->data = NULL;

    return fhd;
}

/* --------------------------------------------------------------------------------------------- */

static void
file_history_free_item (void *data)
{
    file_history_data_t *fhd = (file_history_data_t *) data;

    g_free (fhd->file_name);
    g_free (fhd->file_pos);
    g_free (fhd);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/**
 * Show file history and return the selected file
 *
 * @param w widget used for positioning of history window
 * @param action to do with file (edit, view, etc)
 *
 * @return name of selected file, A newly allocated string.
 */
char *
show_file_history (const Widget * w, int *action)
{
    GList *file_list;
    size_t len;
    history_descriptor_t hd;

    file_list = file_history_list_read ();
    if (file_list == NULL)
        return NULL;

    len = g_list_length (file_list);

    file_list = g_list_last (file_list);

    history_descriptor_init (&hd, w->y, w->x, file_list, 0);
    /* redefine list-specific functions */
    hd.create = file_history_create_item;
    hd.release = file_history_release_item;
    hd.free = file_history_free_item;

    history_show (&hd);

    hd.list = g_list_first (hd.list);

    /* Has history cleaned up or not? */
    if (len != g_list_length (hd.list))
    {
        hd.list = g_list_reverse (hd.list);
        file_history_list_write (hd.list);
    }

    g_list_free_full (hd.list, (GDestroyNotify) file_history_free_item);

    *action = hd.action;

    return hd.text;
}

/* --------------------------------------------------------------------------------------------- */
