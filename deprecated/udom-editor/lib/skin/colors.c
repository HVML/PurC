/*
   Skins engine.
   Work with colors

   Copyright (C) 2009-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2009
   Egmont Koblinger <egmont@gmail.com>, 2010
   Andrew Borodin <aborodin@vmail.ru>, 2012

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

#include "internal.h"

#include "lib/tty/color.h"

/*** global variables ****************************************************************************/

int mc_skin_color__cache[MC_SKIN_COLOR_CACHE_COUNT];

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

static mc_skin_color_t *
mc_skin_color_get_from_hash (mc_skin_t * mc_skin, const gchar * group, const gchar * key)
{
    gchar kname[BUF_TINY];
    mc_skin_color_t *mc_skin_color;

    if (group == NULL || key == NULL)
        return NULL;

    if (mc_skin == NULL)
        mc_skin = &mc_skin__default;

    g_snprintf (kname, sizeof (kname), "%s.%s", group, key);
    mc_skin_color = (mc_skin_color_t *) g_hash_table_lookup (mc_skin->colors, (gpointer) kname);

    return mc_skin_color;
}

/* --------------------------------------------------------------------------------------------- */

#if 0
static void
mc_skin_color_remove_from_hash (mc_skin_t * mc_skin, const gchar * group, const gchar * key)
{
    gchar kname[BUF_TINY];
    if (group == NULL || key == NULL)
        return;

    if (mc_skin == NULL)
        mc_skin = &mc_skin__default;

    g_snprintf (kname, sizeof (kname), "%s.%s", group, key);
    g_hash_table_remove (mc_skin->colors, (gpointer) kname);
}
#endif

/* --------------------------------------------------------------------------------------------- */

static void
mc_skin_color_add_to_hash (mc_skin_t * mc_skin, const gchar * group, const gchar * key,
                           mc_skin_color_t * mc_skin_color)
{
    gchar *kname;

    kname = g_strdup_printf ("%s.%s", group, key);
    if (kname != NULL)
    {
        if (g_hash_table_lookup (mc_skin->colors, (gpointer) kname) != NULL)
            g_hash_table_remove (mc_skin->colors, (gpointer) kname);

        g_hash_table_insert (mc_skin->colors, (gpointer) kname, (gpointer) mc_skin_color);
    }
}

/* --------------------------------------------------------------------------------------------- */

static mc_skin_color_t *
mc_skin_color_get_with_defaults (const gchar * group, const gchar * name)
{
    mc_skin_color_t *mc_skin_color;

    mc_skin_color = mc_skin_color_get_from_hash (NULL, group, name);
    if (mc_skin_color != NULL)
        return mc_skin_color;

    mc_skin_color = mc_skin_color_get_from_hash (NULL, group, "_default_");
    if (mc_skin_color != NULL)
        return mc_skin_color;

    mc_skin_color = mc_skin_color_get_from_hash (NULL, "core", "_default_");
    return mc_skin_color;
}

/* --------------------------------------------------------------------------------------------- */

/* If an alias is found, alloc a new string for the resolved value and free the input parameter.
   Otherwise it's a no-op returning the original string. */
static gchar *
mc_skin_color_look_up_alias (mc_skin_t * mc_skin, gchar * str)
{
    gchar *orig, *str2;
    int hop = 0;

    orig = g_strdup (str);
    str2 = g_strdup (str);

    while (TRUE)
    {
        gchar **values;
        gsize items_count;

        values = mc_config_get_string_list (mc_skin->config, "aliases", str, &items_count);
        if (items_count != 1)
        {
            /* No such alias declaration found, that is, we've got the resolved value. */
            g_strfreev (values);
            g_free (str2);
            g_free (orig);
            return str;
        }

        g_free (str);
        str = g_strdup (values[0]);
        g_strfreev (values);

        /* str2 resolves at half speed than str. This is used for loop detection. */
        if (hop++ % 2 != 0)
        {
            values = mc_config_get_string_list (mc_skin->config, "aliases", str2, &items_count);
            g_assert (items_count == 1);
            g_free (str2);
            str2 = g_strdup (values[0]);
            g_strfreev (values);

            if (strcmp (str, str2) == 0)
            {
                /* Loop detected. */
                fprintf (stderr,
                         "Loop detected while trying to resolve alias \"%s\" in skin \"%s\"\n",
                         orig, mc_skin->name);
                g_free (str);
                g_free (str2);
                return orig;
            }
        }
    }
}

/* --------------------------------------------------------------------------------------------- */

static mc_skin_color_t *
mc_skin_color_get_from_ini_file (mc_skin_t * mc_skin, const gchar * group, const gchar * key)
{
    gsize items_count;
    gchar **values;
    mc_skin_color_t *mc_skin_color, *tmp;

    values = mc_config_get_string_list (mc_skin->config, group, key, &items_count);
    if (values == NULL || values[0] == NULL)
    {
        g_strfreev (values);
        return NULL;
    }

    mc_skin_color = g_try_new0 (mc_skin_color_t, 1);
    if (mc_skin_color == NULL)
    {
        g_strfreev (values);
        return NULL;
    }

    tmp = mc_skin_color_get_with_defaults (group, "_default_");
    mc_skin_color->fgcolor = (items_count > 0 && values[0][0]) ?
        mc_skin_color_look_up_alias (mc_skin, g_strstrip (g_strdup (values[0]))) :
        (tmp != NULL) ? g_strdup (tmp->fgcolor) : NULL;
    mc_skin_color->bgcolor = (items_count > 1 && values[1][0]) ?
        mc_skin_color_look_up_alias (mc_skin, g_strstrip (g_strdup (values[1]))) :
        (tmp != NULL) ? g_strdup (tmp->bgcolor) : NULL;
    mc_skin_color->attrs = (items_count > 2 && values[2][0]) ?
        mc_skin_color_look_up_alias (mc_skin, g_strstrip (g_strdup (values[2]))) :
        (tmp != NULL) ? g_strdup (tmp->attrs) : NULL;

    g_strfreev (values);

    mc_skin_color->pair_index =
        tty_try_alloc_color_pair2 (mc_skin_color->fgcolor, mc_skin_color->bgcolor,
                                   mc_skin_color->attrs, FALSE);

    return mc_skin_color;
}

/* --------------------------------------------------------------------------------------------- */

static void
mc_skin_color_set_default_for_terminal (mc_skin_t * mc_skin)
{
    mc_skin_color_t *mc_skin_color;
    mc_skin_color = g_try_new0 (mc_skin_color_t, 1);
    if (mc_skin_color != NULL)
    {
        mc_skin_color->fgcolor = g_strdup ("default");
        mc_skin_color->bgcolor = g_strdup ("default");
        mc_skin_color->attrs = NULL;
        mc_skin_color->pair_index =
            tty_try_alloc_color_pair2 (mc_skin_color->fgcolor, mc_skin_color->bgcolor,
                                       mc_skin_color->attrs, FALSE);
        mc_skin_color_add_to_hash (mc_skin, "skin", "terminal_default_color", mc_skin_color);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
mc_skin_color_cache_init (void)
{
    DEFAULT_COLOR = mc_skin_color_get ("skin", "terminal_default_color");
    NORMAL_COLOR = mc_skin_color_get ("core", "_default_");
    MARKED_COLOR = mc_skin_color_get ("core", "marked");
    SELECTED_COLOR = mc_skin_color_get ("core", "selected");
    MARKED_SELECTED_COLOR = mc_skin_color_get ("core", "markselect");
    DISABLED_COLOR = mc_skin_color_get ("core", "disabled");
    REVERSE_COLOR = mc_skin_color_get ("core", "reverse");
    HEADER_COLOR = mc_skin_color_get ("core", "header");
    COMMAND_MARK_COLOR = mc_skin_color_get ("core", "commandlinemark");
    SHADOW_COLOR = mc_skin_color_get ("core", "shadow");

    COLOR_NORMAL = mc_skin_color_get ("dialog", "_default_");
    COLOR_FOCUS = mc_skin_color_get ("dialog", "dfocus");
    COLOR_HOT_NORMAL = mc_skin_color_get ("dialog", "dhotnormal");
    COLOR_HOT_FOCUS = mc_skin_color_get ("dialog", "dhotfocus");
    COLOR_TITLE = mc_skin_color_get ("dialog", "dtitle");

    ERROR_COLOR = mc_skin_color_get ("error", "_default_");
    ERROR_FOCUS = mc_skin_color_get ("error", "errdfocus");
    ERROR_HOT_NORMAL = mc_skin_color_get ("error", "errdhotnormal");
    ERROR_HOT_FOCUS = mc_skin_color_get ("error", "errdhotfocus");
    ERROR_TITLE = mc_skin_color_get ("error", "errdtitle");

    MENU_ENTRY_COLOR = mc_skin_color_get ("menu", "_default_");
    MENU_SELECTED_COLOR = mc_skin_color_get ("menu", "menusel");
    MENU_HOT_COLOR = mc_skin_color_get ("menu", "menuhot");
    MENU_HOTSEL_COLOR = mc_skin_color_get ("menu", "menuhotsel");
    MENU_INACTIVE_COLOR = mc_skin_color_get ("menu", "menuinactive");

    PMENU_ENTRY_COLOR = mc_skin_color_get ("popupmenu", "_default_");
    PMENU_SELECTED_COLOR = mc_skin_color_get ("popupmenu", "menusel");
    PMENU_TITLE_COLOR = mc_skin_color_get ("popupmenu", "menutitle");

    BUTTONBAR_HOTKEY_COLOR = mc_skin_color_get ("buttonbar", "hotkey");
    BUTTONBAR_BUTTON_COLOR = mc_skin_color_get ("buttonbar", "button");

    STATUSBAR_COLOR = mc_skin_color_get ("statusbar", "_default_");

    GAUGE_COLOR = mc_skin_color_get ("core", "gauge");
    INPUT_COLOR = mc_skin_color_get ("core", "input");
    INPUT_HISTORY_COLOR = mc_skin_color_get ("core", "inputhistory");
    COMMAND_HISTORY_COLOR = mc_skin_color_get ("core", "commandhistory");
    INPUT_MARK_COLOR = mc_skin_color_get ("core", "inputmark");
    INPUT_UNCHANGED_COLOR = mc_skin_color_get ("core", "inputunchanged");

    HELP_NORMAL_COLOR = mc_skin_color_get ("help", "_default_");
    HELP_ITALIC_COLOR = mc_skin_color_get ("help", "helpitalic");
    HELP_BOLD_COLOR = mc_skin_color_get ("help", "helpbold");
    HELP_LINK_COLOR = mc_skin_color_get ("help", "helplink");
    HELP_SLINK_COLOR = mc_skin_color_get ("help", "helpslink");
    HELP_TITLE_COLOR = mc_skin_color_get ("help", "helptitle");

    VIEW_NORMAL_COLOR = mc_skin_color_get ("viewer", "_default_");
    VIEW_BOLD_COLOR = mc_skin_color_get ("viewer", "viewbold");
    VIEW_UNDERLINED_COLOR = mc_skin_color_get ("viewer", "viewunderline");
    VIEW_SELECTED_COLOR = mc_skin_color_get ("viewer", "viewselected");

    EDITOR_NORMAL_COLOR = mc_skin_color_get ("editor", "_default_");
    EDITOR_BOLD_COLOR = mc_skin_color_get ("editor", "editbold");
    EDITOR_MARKED_COLOR = mc_skin_color_get ("editor", "editmarked");
    EDITOR_WHITESPACE_COLOR = mc_skin_color_get ("editor", "editwhitespace");
    EDITOR_RIGHT_MARGIN_COLOR = mc_skin_color_get ("editor", "editrightmargin");
    LINE_STATE_COLOR = mc_skin_color_get ("editor", "editlinestate");
    EDITOR_BACKGROUND = mc_skin_color_get ("editor", "editbg");
    EDITOR_FRAME = mc_skin_color_get ("editor", "editframe");
    EDITOR_FRAME_ACTIVE = mc_skin_color_get ("editor", "editframeactive");
    EDITOR_FRAME_DRAG = mc_skin_color_get ("editor", "editframedrag");

    BOOK_MARK_COLOR = mc_skin_color_get ("editor", "bookmark");
    BOOK_MARK_FOUND_COLOR = mc_skin_color_get ("editor", "bookmarkfound");

    DFF_ADD_COLOR = mc_skin_color_get ("diffviewer", "added");
    DFF_CHG_COLOR = mc_skin_color_get ("diffviewer", "changedline");
    DFF_CHH_COLOR = mc_skin_color_get ("diffviewer", "changednew");
    DFF_CHD_COLOR = mc_skin_color_get ("diffviewer", "changed");
    DFF_DEL_COLOR = mc_skin_color_get ("diffviewer", "removed");
    DFF_ERROR_COLOR = mc_skin_color_get ("diffviewer", "error");
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
mc_skin_color_check_inisection (const gchar * group)
{
    return !((strcasecmp ("skin", group) == 0) || (strcasecmp ("aliases", group) == 0)
             || (strcasecmp ("lines", group) == 0) || (strncasecmp ("widget-", group, 7) == 0));
}

/* --------------------------------------------------------------------------------------------- */

static void
mc_skin_color_check_bw_mode (mc_skin_t * mc_skin)
{
    gchar **groups, **orig_groups;

    if (tty_use_colors () && !mc_global.tty.disable_colors)
        return;

    orig_groups = mc_config_get_groups (mc_skin->config, NULL);

    for (groups = orig_groups; *groups != NULL; groups++)
        if (mc_skin_color_check_inisection (*groups))
            mc_config_del_group (mc_skin->config, *groups);

    g_strfreev (orig_groups);

    mc_skin_hardcoded_blackwhite_colors (mc_skin);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

gboolean
mc_skin_color_parse_ini_file (mc_skin_t * mc_skin)
{
    gboolean ret = FALSE;
    gsize items_count;
    gchar **groups, **orig_groups;
    mc_skin_color_t *mc_skin_color;

    mc_skin_color_check_bw_mode (mc_skin);

    orig_groups = mc_config_get_groups (mc_skin->config, &items_count);
    if (*orig_groups == NULL)
        goto ret;

    /* as first, need to set up default colors */
    mc_skin_color_set_default_for_terminal (mc_skin);
    mc_skin_color = mc_skin_color_get_from_ini_file (mc_skin, "core", "_default_");
    if (mc_skin_color == NULL)
        goto ret;

    tty_color_set_defaults (mc_skin_color->fgcolor, mc_skin_color->bgcolor, mc_skin_color->attrs);
    mc_skin_color_add_to_hash (mc_skin, "core", "_default_", mc_skin_color);

    for (groups = orig_groups; *groups != NULL; groups++)
    {
        gchar **keys, **orig_keys;

        if (!mc_skin_color_check_inisection (*groups))
            continue;

        orig_keys = mc_config_get_keys (mc_skin->config, *groups, NULL);

        for (keys = orig_keys; *keys != NULL; keys++)
        {
            mc_skin_color = mc_skin_color_get_from_ini_file (mc_skin, *groups, *keys);
            if (mc_skin_color != NULL)
                mc_skin_color_add_to_hash (mc_skin, *groups, *keys, mc_skin_color);
        }
        g_strfreev (orig_keys);
    }

    mc_skin_color_cache_init ();

    ret = TRUE;

  ret:
    g_strfreev (orig_groups);
    return ret;
}

/* --------------------------------------------------------------------------------------------- */

int
mc_skin_color_get (const gchar * group, const gchar * name)
{
    mc_skin_color_t *mc_skin_color;

    mc_skin_color = mc_skin_color_get_with_defaults (group, name);

    return (mc_skin_color != NULL) ? mc_skin_color->pair_index : 0;
}

/* --------------------------------------------------------------------------------------------- */
