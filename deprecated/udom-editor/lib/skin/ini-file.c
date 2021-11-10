/*
   Skins engine.
   Reading and parse ini-files

   Copyright (C) 2009-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2009.

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

#include <string.h>

#include "lib/global.h"         /* <glib.h> */

#include "internal.h"
#include "lib/fileloc.h"
#include "lib/util.h"           /* exist_file() */

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

/* --------------------------------------------------------------------------------------------- */

static void
mc_skin_get_list_from_dir (const gchar * base_dir, GPtrArray * list)
{
    gchar *name;
    GDir *dir;

    name = g_build_filename (base_dir, MC_SKINS_DIR, (char *) NULL);
    dir = g_dir_open (name, 0, NULL);
    g_free (name);

    if (dir != NULL)
    {
        const gchar *cname;

        while ((cname = g_dir_read_name (dir)) != NULL)
        {
            gchar *sname;
            size_t slen;
            unsigned int i;

            slen = strlen (cname);
            sname = g_strndup (cname, slen);

            if (slen > 4 && strcmp (sname + slen - 4, ".ini") == 0)
                sname[slen - 4] = '\0';

            for (i = 0; i < list->len; i++)
                if (strcmp (sname, g_ptr_array_index (list, i)) == 0)
                    break;

            if (i < list->len)
                g_free (sname);
            else
                g_ptr_array_add (list, sname);
        }

        g_dir_close (dir);
    }
}

/* --------------------------------------------------------------------------------------------- */

static int
string_array_comparator (gconstpointer a, gconstpointer b)
{
    return strcmp (*(char *const *) a, *(char *const *) b);
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
mc_skin_ini_file_load_search_in_dir (mc_skin_t * mc_skin, const gchar * base_dir)
{
    char *file_name, *file_name2;

    file_name = g_build_filename (base_dir, MC_SKINS_DIR, mc_skin->name, (char *) NULL);
    if (exist_file (file_name))
    {
        mc_skin->config = mc_config_init (file_name, TRUE);
        g_free (file_name);
        return (mc_skin->config != NULL);
    }
    g_free (file_name);

    file_name2 = g_strdup_printf ("%s.ini", mc_skin->name);
    file_name = g_build_filename (base_dir, MC_SKINS_DIR, file_name2, (char *) NULL);
    g_free (file_name2);

    if (exist_file (file_name))
    {
        mc_skin->config = mc_config_init (file_name, TRUE);
        g_free (file_name);
        return (mc_skin->config != NULL);
    }
    g_free (file_name);
    return FALSE;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

GPtrArray *
mc_skin_list (void)
{
    GPtrArray *list;

    list = g_ptr_array_new ();
    mc_skin_get_list_from_dir (mc_config_get_data_path (), list);
    mc_skin_get_list_from_dir (mc_global.sysconfig_dir, list);
    mc_skin_get_list_from_dir (mc_global.share_data_dir, list);
    g_ptr_array_sort (list, (GCompareFunc) string_array_comparator);

    return list;
}

/* --------------------------------------------------------------------------------------------- */

gboolean
mc_skin_ini_file_load (mc_skin_t * mc_skin)
{
    char *file_name;

    file_name = g_path_get_basename (mc_skin->name);
    if (file_name == NULL)
        return FALSE;

    if (strcmp (file_name, mc_skin->name) != 0)
    {
        g_free (file_name);
        if (!g_path_is_absolute (mc_skin->name))
            return FALSE;
        mc_skin->config = mc_config_init (mc_skin->name, TRUE);
        return (mc_skin->config != NULL);
    }
    g_free (file_name);

    /* ${XDG_DATA_HOME}/mc/skins/ */
    if (mc_skin_ini_file_load_search_in_dir (mc_skin, mc_config_get_data_path ()))
        return TRUE;

    /* /etc/mc/skins/ */
    if (mc_skin_ini_file_load_search_in_dir (mc_skin, mc_global.sysconfig_dir))
        return TRUE;

    /* /usr/share/mc/skins/ */
    return mc_skin_ini_file_load_search_in_dir (mc_skin, mc_global.share_data_dir);
}

/* --------------------------------------------------------------------------------------------- */

gboolean
mc_skin_ini_file_parse (mc_skin_t * mc_skin)
{
    mc_skin->description =
        mc_config_get_string (mc_skin->config, "skin", "description", "- no description -");
    if (!mc_skin_color_parse_ini_file (mc_skin))
        return FALSE;

    mc_skin_lines_parse_ini_file (mc_skin);
    mc_skin->have_256_colors = mc_config_get_bool (mc_skin->config, "skin", "256colors", FALSE);
    mc_skin->have_true_colors = mc_config_get_bool (mc_skin->config, "skin", "truecolors", FALSE);

    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

void
mc_skin_set_hardcoded_skin (mc_skin_t * mc_skin)
{
    mc_skin->config = mc_config_init (NULL, TRUE);

    mc_config_set_string (mc_skin->config, "skin", "description", "hardcoded skin");

    mc_skin_hardcoded_ugly_lines (mc_skin);
    mc_skin_hardcoded_blackwhite_colors (mc_skin);
}

/* --------------------------------------------------------------------------------------------- */
