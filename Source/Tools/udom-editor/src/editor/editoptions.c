/*
   Editor options dialog box

   Copyright (C) 1996-2021
   Free Software Foundation, Inc.

   Written by:
   Paul Sheer, 1996, 1997
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

/** \file
 *  \brief Source: editor options dialog box
 *  \author Paul Sheer
 *  \date 1996, 1997
 */

#include <config.h>

#include <stdlib.h>             /* atoi(), NULL */

#include "lib/global.h"
#include "lib/widget.h"

#include "editwidget.h"
#include "edit-impl.h"
#include "src/setup.h"          /* option_tab_spacing */

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

static const char *wrap_str[] = {
    N_("&None"),
    N_("&Dynamic paragraphing"),
    N_("Type &writer wrap"),
    NULL
};

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

#ifdef ENABLE_NLS
static void
i18n_translate_array (const char *array[])
{
    while (*array != NULL)
    {
        *array = _(*array);
        array++;
    }
}
#endif /* ENABLE_NLS */
/* --------------------------------------------------------------------------------------------- */
/**
 * Callback for the iteration of objects in the 'editors' array.
 * Tear down 'over_col' property in all editors.
 *
 * @param data      probably WEdit object
 * @param user_data unused
 */

static void
edit_reset_over_col (void *data, void *user_data)
{
    (void) user_data;

    if (edit_widget_is_editor (CONST_WIDGET (data)))
        ((WEdit *) data)->over_col = 0;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Callback for the iteration of objects in the 'editors' array.
 * Reload syntax lighlighting in all editors.
 *
 * @param data      probably WEdit object
 * @param user_data unused
 */

static void
edit_reload_syntax (void *data, void *user_data)
{
    (void) user_data;

    if (edit_widget_is_editor (CONST_WIDGET (data)))
    {
        WEdit *edit = (WEdit *) data;

        edit_load_syntax (edit, NULL, edit->syntax_type);
    }
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
edit_options_dialog (WDialog * h)
{
    char wrap_length[16], tab_spacing[16];
    char *p, *q;
    int wrap_mode = 0;
    gboolean old_syntax_hl;

#ifdef ENABLE_NLS
    static gboolean i18n_flag = FALSE;

    if (!i18n_flag)
    {
        i18n_translate_array (wrap_str);
        i18n_flag = TRUE;
    }
#endif /* ENABLE_NLS */

    g_snprintf (wrap_length, sizeof (wrap_length), "%d", option_word_wrap_line_length);
    g_snprintf (tab_spacing, sizeof (tab_spacing), "%d", option_tab_spacing);

    if (option_auto_para_formatting)
        wrap_mode = 1;
    else if (option_typewriter_wrap)
        wrap_mode = 2;
    else
        wrap_mode = 0;

    {
        quick_widget_t quick_widgets[] = {
            /* *INDENT-OFF* */
            QUICK_START_COLUMNS,
                QUICK_START_GROUPBOX (N_("Wrap mode")),
                    QUICK_RADIO (3, wrap_str, &wrap_mode, NULL),
                QUICK_STOP_GROUPBOX,
                QUICK_SEPARATOR (FALSE),
                QUICK_SEPARATOR (FALSE),
                QUICK_START_GROUPBOX (N_("Tabulation")),
                    QUICK_CHECKBOX (N_("&Fake half tabs"), &option_fake_half_tabs, NULL),
                    QUICK_CHECKBOX (N_("&Backspace through tabs"), &option_backspace_through_tabs,
                                    NULL),
                    QUICK_CHECKBOX (N_("Fill tabs with &spaces"), &option_fill_tabs_with_spaces,
                                    NULL),
                    QUICK_LABELED_INPUT (N_("Tab spacing:"), input_label_left, tab_spacing,
                                          "edit-tab-spacing", &q, NULL, FALSE, FALSE, INPUT_COMPLETE_NONE),
                QUICK_STOP_GROUPBOX,
            QUICK_NEXT_COLUMN,
                QUICK_START_GROUPBOX (N_("Other options")),
                    QUICK_CHECKBOX (N_("&Return does autoindent"), &option_return_does_auto_indent,
                                    NULL),
                    QUICK_CHECKBOX (N_("Confir&m before saving"), &edit_confirm_save, NULL),
                    QUICK_CHECKBOX (N_("Save file &position"), &option_save_position, NULL),
                    QUICK_CHECKBOX (N_("&Visible trailing spaces"), &visible_tws, NULL),
                    QUICK_CHECKBOX (N_("Visible &tabs"), &visible_tabs, NULL),
                    QUICK_CHECKBOX (N_("Synta&x highlighting"), &option_syntax_highlighting, NULL),
                    QUICK_CHECKBOX (N_("C&ursor after inserted block"), &option_cursor_after_inserted_block, NULL),
                    QUICK_CHECKBOX (N_("Pers&istent selection"), &option_persistent_selections,
                                     NULL),
                    QUICK_CHECKBOX (N_("Cursor be&yond end of line"), &option_cursor_beyond_eol,
                                     NULL),
                    QUICK_CHECKBOX (N_("&Group undo"), &option_group_undo, NULL),
                    QUICK_LABELED_INPUT (N_("Word wrap line length:"), input_label_left, wrap_length,
                                         "edit-word-wrap", &p, NULL, FALSE, FALSE, INPUT_COMPLETE_NONE),
                QUICK_STOP_GROUPBOX,
            QUICK_STOP_COLUMNS,
            QUICK_BUTTONS_OK_CANCEL,
            QUICK_END
            /* *INDENT-ON* */
        };

        quick_dialog_t qdlg = {
            -1, -1, 74,
            N_("Editor options"), "[Editor options]",
            quick_widgets, NULL, NULL
        };

        if (quick_dialog (&qdlg) == B_CANCEL)
            return;
    }

    old_syntax_hl = option_syntax_highlighting;

    if (!option_cursor_beyond_eol)
        g_list_foreach (GROUP (h)->widgets, edit_reset_over_col, NULL);

    if (p != NULL)
    {
        option_word_wrap_line_length = atoi (p);
        if (option_word_wrap_line_length <= 0)
            option_word_wrap_line_length = DEFAULT_WRAP_LINE_LENGTH;
        g_free (p);
    }

    if (q != NULL)
    {
        option_tab_spacing = atoi (q);
        if (option_tab_spacing <= 0)
            option_tab_spacing = DEFAULT_TAB_SPACING;
        g_free (q);
    }

    if (wrap_mode == 1)
    {
        option_auto_para_formatting = TRUE;
        option_typewriter_wrap = FALSE;
    }
    else if (wrap_mode == 2)
    {
        option_auto_para_formatting = FALSE;
        option_typewriter_wrap = TRUE;
    }
    else
    {
        option_auto_para_formatting = FALSE;
        option_typewriter_wrap = FALSE;
    }

    /* Load or unload syntax rules if the option has changed */
    if (option_syntax_highlighting != old_syntax_hl)
        g_list_foreach (GROUP (h)->widgets, edit_reload_syntax, NULL);
}

/* --------------------------------------------------------------------------------------------- */
