/*
   File highlight plugin.
   Reading and parse rules from ini-files

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

#include "lib/global.h"
#include "lib/fileloc.h"
#include "lib/strescape.h"
#include "lib/skin.h"
#include "lib/util.h"           /* exist_file() */
#include "lib/filehighlight.h"

#include "internal.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static void
mc_fhl_parse_fill_color_info (mc_fhl_filter_t * mc_filter, mc_fhl_t * fhl, const gchar * group_name)
{
    (void) fhl;
    mc_filter->color_pair_index = mc_skin_color_get ("filehighlight", group_name);
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
mc_fhl_parse_get_file_type_id (mc_fhl_t * fhl, const gchar * group_name)
{
    mc_fhl_filter_t *mc_filter;

    const gchar *types[] = {
        "FILE", "FILE_EXE",
        "DIR", "LINK_DIR",
        "LINK", "HARDLINK", "SYMLINK",
        "STALE_LINK",
        "DEVICE", "DEVICE_BLOCK", "DEVICE_CHAR",
        "SPECIAL", "SPECIAL_SOCKET", "SPECIAL_FIFO", "SPECIAL_DOOR",
        NULL
    };
    int i;
    gchar *param_type = mc_config_get_string (fhl->config, group_name, "type", "");

    if (*param_type == '\0')
    {
        g_free (param_type);
        return FALSE;
    }

    for (i = 0; types[i] != NULL; i++)
    {
        if (strcmp (types[i], param_type) == 0)
            break;
    }
    g_free (param_type);
    if (types[i] == NULL)
        return FALSE;

    mc_filter = g_new0 (mc_fhl_filter_t, 1);
    mc_filter->type = MC_FLHGH_T_FTYPE;
    mc_filter->file_type = (mc_flhgh_ftype_type) i;
    mc_fhl_parse_fill_color_info (mc_filter, fhl, group_name);

    g_ptr_array_add (fhl->filters, (gpointer) mc_filter);
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
mc_fhl_parse_get_regexp (mc_fhl_t * fhl, const gchar * group_name)
{
    mc_fhl_filter_t *mc_filter;
    gchar *regexp = mc_config_get_string (fhl->config, group_name, "regexp", "");

    if (*regexp == '\0')
    {
        g_free (regexp);
        return FALSE;
    }

    mc_filter = g_new0 (mc_fhl_filter_t, 1);
    mc_filter->type = MC_FLHGH_T_FREGEXP;
    mc_filter->search_condition = mc_search_new (regexp, DEFAULT_CHARSET);
    mc_filter->search_condition->is_case_sensitive = TRUE;
    mc_filter->search_condition->search_type = MC_SEARCH_T_REGEX;

    mc_fhl_parse_fill_color_info (mc_filter, fhl, group_name);
    g_ptr_array_add (fhl->filters, (gpointer) mc_filter);
    g_free (regexp);
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
mc_fhl_parse_get_extensions (mc_fhl_t * fhl, const gchar * group_name)
{
    mc_fhl_filter_t *mc_filter;
    gchar **exts, **exts_orig;
    GString *buf;

    exts_orig = mc_config_get_string_list (fhl->config, group_name, "extensions", NULL);
    if (exts_orig == NULL || exts_orig[0] == NULL)
    {
        g_strfreev (exts_orig);
        return FALSE;
    }

    buf = g_string_sized_new (64);

    for (exts = exts_orig; *exts != NULL; exts++)
    {
        char *esc_ext;

        esc_ext = strutils_regex_escape (*exts);
        if (buf->len != 0)
            g_string_append_c (buf, '|');
        g_string_append (buf, esc_ext);
        g_free (esc_ext);
    }
    g_strfreev (exts_orig);

    g_string_prepend (buf, ".*\\.(");
    g_string_append (buf, ")$");

    mc_filter = g_new0 (mc_fhl_filter_t, 1);
    mc_filter->type = MC_FLHGH_T_FREGEXP;
    mc_filter->search_condition = mc_search_new_len (buf->str, buf->len, DEFAULT_CHARSET);
    mc_filter->search_condition->is_case_sensitive =
        mc_config_get_bool (fhl->config, group_name, "extensions_case", FALSE);
    mc_filter->search_condition->search_type = MC_SEARCH_T_REGEX;

    mc_fhl_parse_fill_color_info (mc_filter, fhl, group_name);
    g_ptr_array_add (fhl->filters, (gpointer) mc_filter);
    g_string_free (buf, TRUE);
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

gboolean
mc_fhl_read_ini_file (mc_fhl_t * fhl, const gchar * filename)
{
    if (fhl == NULL || filename == NULL || !exist_file (filename))
        return FALSE;

    if (fhl->config != NULL)
        return mc_config_read_file (fhl->config, filename, TRUE, FALSE);

    fhl->config = mc_config_init (filename, TRUE);
    return (fhl->config != NULL);
}

/* --------------------------------------------------------------------------------------------- */

gboolean
mc_fhl_init_from_standard_files (mc_fhl_t * fhl)
{
    gchar *name;
    gboolean ok;

    /* ${XDG_CONFIG_HOME}/mc/filehighlight.ini */
    name = mc_config_get_full_path (MC_FHL_INI_FILE);
    ok = mc_fhl_read_ini_file (fhl, name);
    g_free (name);
    if (ok)
        return TRUE;

    /* ${sysconfdir}/mc/filehighlight.ini  */
    name = g_build_filename (mc_global.sysconfig_dir, MC_FHL_INI_FILE, (char *) NULL);
    ok = mc_fhl_read_ini_file (fhl, name);
    g_free (name);
    if (ok)
        return TRUE;

    /* ${datadir}/mc/filehighlight.ini  */
    name = g_build_filename (mc_global.share_data_dir, MC_FHL_INI_FILE, (char *) NULL);
    ok = mc_fhl_read_ini_file (fhl, name);
    g_free (name);
    return ok;
}

/* --------------------------------------------------------------------------------------------- */

gboolean
mc_fhl_parse_ini_file (mc_fhl_t * fhl)
{
    gchar **group_names, **orig_group_names;
    gboolean ok;

    mc_fhl_array_free (fhl);
    fhl->filters = g_ptr_array_new ();

    orig_group_names = mc_config_get_groups (fhl->config, NULL);
    ok = (*orig_group_names != NULL);

    for (group_names = orig_group_names; *group_names != NULL; group_names++)
    {
        if (mc_config_has_param (fhl->config, *group_names, "type"))
        {
            /* parse filetype filter */
            mc_fhl_parse_get_file_type_id (fhl, *group_names);
        }
        if (mc_config_has_param (fhl->config, *group_names, "regexp"))
        {
            /* parse regexp filter */
            mc_fhl_parse_get_regexp (fhl, *group_names);
        }
        if (mc_config_has_param (fhl->config, *group_names, "extensions"))
        {
            /* parse extensions filter */
            mc_fhl_parse_get_extensions (fhl, *group_names);
        }
    }

    g_strfreev (orig_group_names);

    return ok;
}

/* --------------------------------------------------------------------------------------------- */
