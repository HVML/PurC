/*
   Skins engine.
   Work with colors - backward compatibility

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
#include <stdlib.h>
#include <string.h>             /* strcmp() */
#include <sys/types.h>          /* size_t */

#include "internal.h"

#include "lib/tty/color.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

typedef struct mc_skin_colors_old_struct
{
    const char *old_color;
    const char *group;
    const char *key;
} mc_skin_colors_old_t;

/*** file scope variables ************************************************************************/

/* keep this table alphabetically sorted */
static const mc_skin_colors_old_t old_colors[] = {
    {"bbarbutton", "buttonbar", "button"},
    {"bbarhotkey", "buttonbar", "hotkey"},
    {"commandlinemark", "core", "commandlinemark"},
    {"dfocus", "dialog", "dfocus"},
    {"dhotfocus", "dialog", "dhotfocus"},
    {"dhotnormal", "dialog", "dhotnormal"},
    {"disabled", "core", "disabled"},
    {"dnormal", "dialog", "_default_"},
    {"editbg", "editor", "editbg"},
    {"editbold", "editor", "editbold"},
    {"editframe", "editor", "editframe"},
    {"editframeactive", "editor", "editframeactive"},
    {"editframedrag", "editor", "editframedrag"},
    {"editlinestate", "editor", "editlinestate"},
    {"editmarked", "editor", "editmarked"},
    {"editnormal", "editor", "_default_"},
    {"editwhitespace", "editor", "editwhitespace"},
    {"errdhotfocus", "error", "errdhotfocus"},
    {"errdhotnormal", "error", "errdhotnormal"},
    {"errors", "error", "_default_"},
    {"gauge", "core", "gauge"},
    {"header", "core", "header"},
    {"helpbold", "help", "helpbold"},
    {"helpitalic", "help", "helpitalic"},
    {"helplink", "help", "helplink"},
    {"helpnormal", "help", "_default_"},
    {"helpslink", "help", "helpslink"},
    {"input", "core", "input"},
    {"inputmark", "core", "inputmark"},
    {"inputunchanged", "core", "inputunchanged"},
    {"marked", "core", "marked"},
    {"markselect", "core", "markselect"},
    {"menuhot", "menu", "menuhot"},
    {"menuhotsel", "menu", "menuhotsel"},
    {"menuinactive", "menu", "menuinactive"},
    {"menunormal", "menu", "_default_"},
    {"menusel", "menu", "menusel"},
    {"normal", "core", "_default_"},
    {"pmenunormal", "popupmenu", "_default_"},
    {"pmenusel", "popupmenu", "menusel"},
    {"pmenutitle", "popupmenu", "menutitle"},
    {"reverse", "core", "reverse"},
    {"selected", "core", "selected"},
    {"statusbar", "statusbar", "_default_"},
    {"viewbold", "viewer", "viewbold"},
    {"viewnormal", "viewer", "_default_"},
    {"viewselected", "viewer", "viewselected"},
    {"viewunderline", "viewer", "viewunderline"}
};

static const size_t num_old_colors = G_N_ELEMENTS (old_colors);

/*** file scope functions ************************************************************************/

static int
old_color_comparator (const void *p1, const void *p2)
{
    const mc_skin_colors_old_t *m1 = (const mc_skin_colors_old_t *) p1;
    const mc_skin_colors_old_t *m2 = (const mc_skin_colors_old_t *) p2;

    return strcmp (m1->old_color, m2->old_color);
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
mc_skin_colors_old_transform (const char *old_color, const char **group, const char **key)
{
    const mc_skin_colors_old_t oc = { old_color, NULL, NULL };
    mc_skin_colors_old_t *res;

    if (old_color == NULL)
        return FALSE;

    res = (mc_skin_colors_old_t *) bsearch (&oc, old_colors, num_old_colors,
                                            sizeof (old_colors[0]), old_color_comparator);

    if (res == NULL)
        return FALSE;

    if (group != NULL)
        *group = res->group;
    if (key != NULL)
        *key = res->key;
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */

static void
mc_skin_colors_old_configure_one (mc_skin_t * mc_skin, const char *the_color_string)
{
    gchar **colors, **orig_colors;

    if (the_color_string == NULL)
        return;

    orig_colors = g_strsplit (the_color_string, ":", -1);
    if (orig_colors == NULL)
        return;

    for (colors = orig_colors; *colors != NULL; colors++)
    {
        gchar **key_val;
        const gchar *skin_group, *skin_key;

        key_val = g_strsplit_set (*colors, "=,", 4);

        if (key_val == NULL)
            continue;

        if (key_val[1] != NULL && mc_skin_colors_old_transform (key_val[0], &skin_group, &skin_key))
        {
            gchar *skin_val;

            if (key_val[2] == NULL)
                skin_val = g_strdup_printf ("%s;", key_val[1]);
            else if (key_val[3] == NULL)
                skin_val = g_strdup_printf ("%s;%s", key_val[1], key_val[2]);
            else
                skin_val = g_strdup_printf ("%s;%s;%s", key_val[1], key_val[2], key_val[3]);

            mc_config_set_string (mc_skin->config, skin_group, skin_key, skin_val);
            g_free (skin_val);
        }

        g_strfreev (key_val);
    }
    g_strfreev (orig_colors);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
mc_skin_colors_old_configure (mc_skin_t * mc_skin)
{
    mc_skin_colors_old_configure_one (mc_skin, mc_global.tty.setup_color_string);
    mc_skin_colors_old_configure_one (mc_skin, mc_global.tty.term_color_string);
    mc_skin_colors_old_configure_one (mc_skin, getenv ("MC_COLOR_TABLE"));
    mc_skin_colors_old_configure_one (mc_skin, mc_global.tty.command_line_colors);
}

/* --------------------------------------------------------------------------------------------- */
