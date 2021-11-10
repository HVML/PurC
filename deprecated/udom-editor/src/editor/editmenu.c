/*
   Editor menu definitions and initialisation

   Copyright (C) 1996-2021
   Free Software Foundation, Inc.

   Written by:
   Paul Sheer, 1996, 1997
   Andrew Borodin <aborodin@vmail.ru> 2012

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
 *  \brief Source: editor menu definitions and initialisation
 *  \author Paul Sheer
 *  \date 1996, 1997
 */

#include <config.h>

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "lib/global.h"

#include "lib/tty/tty.h"        /* KEY_F */
#include "lib/tty/key.h"        /* XCTRL */
#include "lib/widget.h"

#include "src/setup.h"          /* drop_menus */
#include "src/keymap.h"

#include "edit-impl.h"
#include "editwidget.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static GList *
create_file_menu (void)
{
    GList *entries = NULL;

    entries = g_list_prepend (entries, menu_entry_create (_("&Open file..."), CK_EditFile));
    entries = g_list_prepend (entries, menu_entry_create (_("&New"), CK_EditNew));
    entries = g_list_prepend (entries, menu_entry_create (_("&Close"), CK_Close));
    entries = g_list_prepend (entries, menu_entry_create (_("&History..."), CK_History));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("&Save"), CK_Save));
    entries = g_list_prepend (entries, menu_entry_create (_("Save &as..."), CK_SaveAs));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("&Insert file..."), CK_InsertFile));
    entries = g_list_prepend (entries, menu_entry_create (_("Cop&y to file..."), CK_BlockSave));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("&User menu..."), CK_UserMenu));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("A&bout..."), CK_About));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("&Quit"), CK_Quit));

    return g_list_reverse (entries);
}

/* --------------------------------------------------------------------------------------------- */

static GList *
create_edit_menu (void)
{
    GList *entries = NULL;

    entries = g_list_prepend (entries, menu_entry_create (_("&Undo"), CK_Undo));
    entries = g_list_prepend (entries, menu_entry_create (_("&Redo"), CK_Redo));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries =
        g_list_prepend (entries, menu_entry_create (_("&Toggle ins/overw"), CK_InsertOverwrite));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("To&ggle mark"), CK_Mark));
    entries = g_list_prepend (entries, menu_entry_create (_("&Mark columns"), CK_MarkColumn));
    entries = g_list_prepend (entries, menu_entry_create (_("Mark &all"), CK_MarkAll));
    entries = g_list_prepend (entries, menu_entry_create (_("Unmar&k"), CK_Unmark));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("Cop&y"), CK_Copy));
    entries = g_list_prepend (entries, menu_entry_create (_("Mo&ve"), CK_Move));
    entries = g_list_prepend (entries, menu_entry_create (_("&Delete"), CK_Remove));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("Co&py to clipfile"), CK_Store));
    entries = g_list_prepend (entries, menu_entry_create (_("&Cut to clipfile"), CK_Cut));
    entries = g_list_prepend (entries, menu_entry_create (_("Pa&ste from clipfile"), CK_Paste));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("&Beginning"), CK_Top));
    entries = g_list_prepend (entries, menu_entry_create (_("&End"), CK_Bottom));

    return g_list_reverse (entries);
}

/* --------------------------------------------------------------------------------------------- */

static GList *
create_search_replace_menu (void)
{
    GList *entries = NULL;

    entries = g_list_prepend (entries, menu_entry_create (_("&Search..."), CK_Search));
    entries = g_list_prepend (entries, menu_entry_create (_("Search &again"), CK_SearchContinue));
    entries = g_list_prepend (entries, menu_entry_create (_("&Replace..."), CK_Replace));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("&Toggle bookmark"), CK_Bookmark));
    entries = g_list_prepend (entries, menu_entry_create (_("&Next bookmark"), CK_BookmarkNext));
    entries = g_list_prepend (entries, menu_entry_create (_("&Prev bookmark"), CK_BookmarkPrev));
    entries = g_list_prepend (entries, menu_entry_create (_("&Flush bookmarks"), CK_BookmarkFlush));

    return g_list_reverse (entries);
}

/* --------------------------------------------------------------------------------------------- */

static GList *
create_command_menu (void)
{
    GList *entries = NULL;

    entries = g_list_prepend (entries, menu_entry_create (_("&Go to line..."), CK_Goto));
    entries = g_list_prepend (entries, menu_entry_create (_("&Toggle line state"), CK_ShowNumbers));
    entries =
        g_list_prepend (entries, menu_entry_create (_("Go to matching &bracket"), CK_MatchBracket));
    entries =
        g_list_prepend (entries,
                        menu_entry_create (_("Toggle s&yntax highlighting"), CK_SyntaxOnOff));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("&Find declaration"), CK_Find));
    entries =
        g_list_prepend (entries, menu_entry_create (_("Back from &declaration"), CK_FilePrev));
    entries =
        g_list_prepend (entries, menu_entry_create (_("For&ward to declaration"), CK_FileNext));
#ifdef HAVE_CHARSET
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("Encod&ing..."), CK_SelectCodepage));
#endif
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("&Refresh screen"), CK_Refresh));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries =
        g_list_prepend (entries,
                        menu_entry_create (_("&Start/Stop record macro"), CK_MacroStartStopRecord));
    entries = g_list_prepend (entries, menu_entry_create (_("Delete macr&o..."), CK_MacroDelete));
    entries =
        g_list_prepend (entries,
                        menu_entry_create (_("Record/Repeat &actions"), CK_RepeatStartStopRecord));
    entries = g_list_prepend (entries, menu_separator_create ());
#ifdef HAVE_ASPELL
    if (strcmp (spell_language, "NONE") != 0)
    {
        entries = g_list_prepend (entries, menu_entry_create (_("S&pell check"), CK_SpellCheck));
        entries =
            g_list_prepend (entries,
                            menu_entry_create (_("C&heck word"), CK_SpellCheckCurrentWord));
        entries =
            g_list_prepend (entries,
                            menu_entry_create (_("Change spelling &language..."),
                                               CK_SpellCheckSelectLang));
        entries = g_list_prepend (entries, menu_separator_create ());
    }
#endif /* HAVE_ASPELL */
    entries = g_list_prepend (entries, menu_entry_create (_("&Mail..."), CK_EditMail));


    return g_list_reverse (entries);
}

/* --------------------------------------------------------------------------------------------- */

static GList *
create_format_menu (void)
{
    GList *entries = NULL;

    entries =
        g_list_prepend (entries, menu_entry_create (_("Insert &literal..."), CK_InsertLiteral));
    entries = g_list_prepend (entries, menu_entry_create (_("Insert &date/time"), CK_Date));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries =
        g_list_prepend (entries, menu_entry_create (_("&Format paragraph"), CK_ParagraphFormat));
    entries = g_list_prepend (entries, menu_entry_create (_("&Sort..."), CK_Sort));
    entries =
        g_list_prepend (entries, menu_entry_create (_("&Paste output of..."), CK_ExternalCommand));
    entries =
        g_list_prepend (entries, menu_entry_create (_("&External formatter"), CK_PipeBlock (0)));

    return g_list_reverse (entries);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Create the 'window' popup menu
 */

static GList *
create_window_menu (void)
{
    GList *entries = NULL;

    entries = g_list_prepend (entries, menu_entry_create (_("&Move"), CK_WindowMove));
    entries = g_list_prepend (entries, menu_entry_create (_("&Resize"), CK_WindowResize));
    entries =
        g_list_prepend (entries, menu_entry_create (_("&Toggle fullscreen"), CK_WindowFullscreen));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("&Next"), CK_WindowNext));
    entries = g_list_prepend (entries, menu_entry_create (_("&Previous"), CK_WindowPrev));
    entries = g_list_prepend (entries, menu_entry_create (_("&List..."), CK_WindowList));

    return g_list_reverse (entries);
}

/* --------------------------------------------------------------------------------------------- */

static GList *
create_options_menu (void)
{
    GList *entries = NULL;

    entries = g_list_prepend (entries, menu_entry_create (_("&General..."), CK_Options));
    entries = g_list_prepend (entries, menu_entry_create (_("Save &mode..."), CK_OptionsSaveMode));
    entries = g_list_prepend (entries, menu_entry_create (_("Learn &keys..."), CK_LearnKeys));
    entries =
        g_list_prepend (entries, menu_entry_create (_("Syntax &highlighting..."), CK_SyntaxChoose));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("S&yntax file"), CK_EditSyntaxFile));
    entries = g_list_prepend (entries, menu_entry_create (_("&Menu file"), CK_EditUserMenu));
    entries = g_list_prepend (entries, menu_separator_create ());
    entries = g_list_prepend (entries, menu_entry_create (_("&Save setup"), CK_SaveSetup));

    return g_list_reverse (entries);
}

/* --------------------------------------------------------------------------------------------- */

static void
edit_drop_menu_cmd (WDialog * h, int which)
{
    WMenuBar *menubar;

    menubar = find_menubar (h);
    menubar_activate (menubar, drop_menus, which);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
edit_init_menu (WMenuBar * menubar)
{
    menubar_add_menu (menubar,
                      create_menu (_("&File"), create_file_menu (), "[Internal File Editor]"));
    menubar_add_menu (menubar,
                      create_menu (_("&Edit"), create_edit_menu (), "[Internal File Editor]"));
    menubar_add_menu (menubar,
                      create_menu (_("&Search"), create_search_replace_menu (),
                                   "[Internal File Editor]"));
    menubar_add_menu (menubar,
                      create_menu (_("&Command"), create_command_menu (),
                                   "[Internal File Editor]"));
    menubar_add_menu (menubar,
                      create_menu (_("For&mat"), create_format_menu (), "[Internal File Editor]"));
    menubar_add_menu (menubar,
                      create_menu (_("&Window"), create_window_menu (), "[Internal File Editor]"));
    menubar_add_menu (menubar,
                      create_menu (_("&Options"), create_options_menu (),
                                   "[Internal File Editor]"));
}

/* --------------------------------------------------------------------------------------------- */

void
edit_menu_cmd (WDialog * h)
{
    edit_drop_menu_cmd (h, -1);
}

/* --------------------------------------------------------------------------------------------- */

gboolean
edit_drop_hotkey_menu (WDialog * h, int key)
{
    int m = 0;
    switch (key)
    {
    case ALT ('f'):
        m = 0;
        break;
    case ALT ('e'):
        m = 1;
        break;
    case ALT ('s'):
        m = 2;
        break;
    case ALT ('c'):
        m = 3;
        break;
    case ALT ('m'):
        m = 4;
        break;
    case ALT ('w'):
        m = 5;
        break;
    case ALT ('o'):
        m = 6;
        break;
    default:
        return FALSE;
    }

    edit_drop_menu_cmd (h, m);
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */
