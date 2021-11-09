/*
   Definitions of key bindings.

   Copyright (C) 2005-2021
   Free Software Foundation, Inc.

   Written by:
   Vitja Makarov, 2005
   Ilia Maslakov <il.smind@gmail.com>, 2009, 2012
   Andrew Borodin <aborodin@vmail.ru>, 2009-2020

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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "lib/global.h"
#include "lib/tty/key.h"        /* KEY_M_ */
#include "lib/keybind.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

#define ADD_KEYMAP_NAME(name) \
    { #name, CK_##name }

/*** file scope type declarations ****************************************************************/

typedef struct name_keymap_t
{
    const char *name;
    long val;
} name_keymap_t;

/*** file scope variables ************************************************************************/

static name_keymap_t command_names[] = {
    /* common */
    ADD_KEYMAP_NAME (InsertChar),
    ADD_KEYMAP_NAME (Enter),
    ADD_KEYMAP_NAME (ChangePanel),
    ADD_KEYMAP_NAME (Up),
    ADD_KEYMAP_NAME (Down),
    ADD_KEYMAP_NAME (Left),
    ADD_KEYMAP_NAME (Right),
    ADD_KEYMAP_NAME (LeftQuick),
    ADD_KEYMAP_NAME (RightQuick),
    ADD_KEYMAP_NAME (Home),
    ADD_KEYMAP_NAME (End),
    ADD_KEYMAP_NAME (PageUp),
    ADD_KEYMAP_NAME (PageDown),
    ADD_KEYMAP_NAME (HalfPageUp),
    ADD_KEYMAP_NAME (HalfPageDown),
    ADD_KEYMAP_NAME (Top),
    ADD_KEYMAP_NAME (Bottom),
    ADD_KEYMAP_NAME (TopOnScreen),
    ADD_KEYMAP_NAME (MiddleOnScreen),
    ADD_KEYMAP_NAME (BottomOnScreen),
    ADD_KEYMAP_NAME (WordLeft),
    ADD_KEYMAP_NAME (WordRight),
    ADD_KEYMAP_NAME (Copy),
    ADD_KEYMAP_NAME (Move),
    ADD_KEYMAP_NAME (Delete),
    ADD_KEYMAP_NAME (MakeDir),
    ADD_KEYMAP_NAME (ChangeMode),
    ADD_KEYMAP_NAME (ChangeOwn),
    ADD_KEYMAP_NAME (ChangeOwnAdvanced),
#ifdef ENABLE_EXT2FS_ATTR
    ADD_KEYMAP_NAME (ChangeAttributes),
#endif
    ADD_KEYMAP_NAME (Remove),
    ADD_KEYMAP_NAME (BackSpace),
    ADD_KEYMAP_NAME (Redo),
    ADD_KEYMAP_NAME (Clear),
    ADD_KEYMAP_NAME (Menu),
    ADD_KEYMAP_NAME (MenuLastSelected),
    ADD_KEYMAP_NAME (UserMenu),
    ADD_KEYMAP_NAME (EditUserMenu),
    ADD_KEYMAP_NAME (Search),
    ADD_KEYMAP_NAME (SearchContinue),
    ADD_KEYMAP_NAME (Replace),
    ADD_KEYMAP_NAME (ReplaceContinue),
    ADD_KEYMAP_NAME (Help),
    ADD_KEYMAP_NAME (Shell),
    ADD_KEYMAP_NAME (Edit),
    ADD_KEYMAP_NAME (EditNew),
#ifdef HAVE_CHARSET
    ADD_KEYMAP_NAME (SelectCodepage),
#endif
    ADD_KEYMAP_NAME (EditorViewerHistory),
    ADD_KEYMAP_NAME (History),
    ADD_KEYMAP_NAME (HistoryNext),
    ADD_KEYMAP_NAME (HistoryPrev),
    ADD_KEYMAP_NAME (Complete),
    ADD_KEYMAP_NAME (Save),
    ADD_KEYMAP_NAME (SaveAs),
    ADD_KEYMAP_NAME (Goto),
    ADD_KEYMAP_NAME (Reread),
    ADD_KEYMAP_NAME (Refresh),
    ADD_KEYMAP_NAME (Suspend),
    ADD_KEYMAP_NAME (Swap),
    ADD_KEYMAP_NAME (HotList),
    ADD_KEYMAP_NAME (SelectInvert),
    ADD_KEYMAP_NAME (ScreenList),
    ADD_KEYMAP_NAME (ScreenNext),
    ADD_KEYMAP_NAME (ScreenPrev),
    ADD_KEYMAP_NAME (FileNext),
    ADD_KEYMAP_NAME (FilePrev),
    ADD_KEYMAP_NAME (DeleteToHome),
    ADD_KEYMAP_NAME (DeleteToEnd),
    ADD_KEYMAP_NAME (DeleteToWordBegin),
    ADD_KEYMAP_NAME (DeleteToWordEnd),
    ADD_KEYMAP_NAME (Cut),
    ADD_KEYMAP_NAME (Store),
    ADD_KEYMAP_NAME (Paste),
    ADD_KEYMAP_NAME (Mark),
    ADD_KEYMAP_NAME (MarkLeft),
    ADD_KEYMAP_NAME (MarkRight),
    ADD_KEYMAP_NAME (MarkUp),
    ADD_KEYMAP_NAME (MarkDown),
    ADD_KEYMAP_NAME (MarkToWordBegin),
    ADD_KEYMAP_NAME (MarkToWordEnd),
    ADD_KEYMAP_NAME (MarkToHome),
    ADD_KEYMAP_NAME (MarkToEnd),
    ADD_KEYMAP_NAME (ToggleNavigation),
    ADD_KEYMAP_NAME (Sort),
    ADD_KEYMAP_NAME (Options),
    ADD_KEYMAP_NAME (LearnKeys),
    ADD_KEYMAP_NAME (Bookmark),
    ADD_KEYMAP_NAME (Quit),
    ADD_KEYMAP_NAME (QuitQuiet),
    ADD_KEYMAP_NAME (ExtendedKeyMap),

    /* main commands */
#ifdef USE_INTERNAL_EDIT
    ADD_KEYMAP_NAME (EditForceInternal),
#endif
    ADD_KEYMAP_NAME (View),
    ADD_KEYMAP_NAME (ViewRaw),
    ADD_KEYMAP_NAME (ViewFile),
    ADD_KEYMAP_NAME (ViewFiltered),
    ADD_KEYMAP_NAME (Find),
    ADD_KEYMAP_NAME (DirSize),
    ADD_KEYMAP_NAME (CompareDirs),
#ifdef USE_DIFF_VIEW
    ADD_KEYMAP_NAME (CompareFiles),
#endif
    ADD_KEYMAP_NAME (OptionsVfs),
    ADD_KEYMAP_NAME (OptionsConfirm),
    ADD_KEYMAP_NAME (OptionsDisplayBits),
    ADD_KEYMAP_NAME (EditExtensionsFile),
    ADD_KEYMAP_NAME (EditFileHighlightFile),
    ADD_KEYMAP_NAME (LinkSymbolicEdit),
    ADD_KEYMAP_NAME (ExternalPanelize),
    ADD_KEYMAP_NAME (Filter),
#ifdef ENABLE_VFS_FISH
    ADD_KEYMAP_NAME (ConnectFish),
#endif
#ifdef ENABLE_VFS_FTP
    ADD_KEYMAP_NAME (ConnectFtp),
#endif
#ifdef ENABLE_VFS_SFTP
    ADD_KEYMAP_NAME (ConnectSftp),
#endif
    ADD_KEYMAP_NAME (PanelInfo),
#ifdef ENABLE_BACKGROUND
    ADD_KEYMAP_NAME (Jobs),
#endif
    ADD_KEYMAP_NAME (OptionsLayout),
    ADD_KEYMAP_NAME (OptionsAppearance),
    ADD_KEYMAP_NAME (Link),
    ADD_KEYMAP_NAME (SetupListingFormat),
    ADD_KEYMAP_NAME (PanelListing),
#ifdef LISTMODE_EDITOR
    ADD_KEYMAP_NAME (ListMode),
#endif
    ADD_KEYMAP_NAME (OptionsPanel),
    ADD_KEYMAP_NAME (CdQuick),
    ADD_KEYMAP_NAME (PanelQuickView),
    ADD_KEYMAP_NAME (LinkSymbolicRelative),
    ADD_KEYMAP_NAME (VfsList),
    ADD_KEYMAP_NAME (SaveSetup),
    ADD_KEYMAP_NAME (LinkSymbolic),
    ADD_KEYMAP_NAME (PanelTree),
    ADD_KEYMAP_NAME (Tree),
#ifdef ENABLE_VFS_UNDELFS
    ADD_KEYMAP_NAME (Undelete),
#endif
    ADD_KEYMAP_NAME (PutCurrentLink),
    ADD_KEYMAP_NAME (PutOtherLink),
    ADD_KEYMAP_NAME (HotListAdd),
    ADD_KEYMAP_NAME (ShowHidden),
    ADD_KEYMAP_NAME (SplitVertHoriz),
    ADD_KEYMAP_NAME (SplitEqual),
    ADD_KEYMAP_NAME (SplitMore),
    ADD_KEYMAP_NAME (SplitLess),
    ADD_KEYMAP_NAME (PutCurrentPath),
    ADD_KEYMAP_NAME (PutOtherPath),
    ADD_KEYMAP_NAME (PutCurrentSelected),
    ADD_KEYMAP_NAME (PutCurrentFullSelected),
    ADD_KEYMAP_NAME (PutCurrentTagged),
    ADD_KEYMAP_NAME (PutOtherTagged),
    ADD_KEYMAP_NAME (Select),
    ADD_KEYMAP_NAME (Unselect),

    /* panel */
    ADD_KEYMAP_NAME (SelectExt),
    ADD_KEYMAP_NAME (ScrollLeft),
    ADD_KEYMAP_NAME (ScrollRight),
    ADD_KEYMAP_NAME (PanelOtherCd),
    ADD_KEYMAP_NAME (PanelOtherCdLink),
    ADD_KEYMAP_NAME (CopySingle),
    ADD_KEYMAP_NAME (MoveSingle),
    ADD_KEYMAP_NAME (DeleteSingle),
    ADD_KEYMAP_NAME (CdParent),
    ADD_KEYMAP_NAME (CdChild),
    ADD_KEYMAP_NAME (Panelize),
    ADD_KEYMAP_NAME (PanelOtherSync),
    ADD_KEYMAP_NAME (SortNext),
    ADD_KEYMAP_NAME (SortPrev),
    ADD_KEYMAP_NAME (SortReverse),
    ADD_KEYMAP_NAME (SortByName),
    ADD_KEYMAP_NAME (SortByExt),
    ADD_KEYMAP_NAME (SortBySize),
    ADD_KEYMAP_NAME (SortByMTime),
    ADD_KEYMAP_NAME (CdParentSmart),
    ADD_KEYMAP_NAME (CycleListingFormat),

    /* dialog */
    ADD_KEYMAP_NAME (Ok),
    ADD_KEYMAP_NAME (Cancel),

    /* input line */
    ADD_KEYMAP_NAME (Yank),

    /* help */
    ADD_KEYMAP_NAME (Index),
    ADD_KEYMAP_NAME (Back),
    ADD_KEYMAP_NAME (LinkNext),
    ADD_KEYMAP_NAME (LinkPrev),
    ADD_KEYMAP_NAME (NodeNext),
    ADD_KEYMAP_NAME (NodePrev),

    /* tree */
    ADD_KEYMAP_NAME (Forget),

#if defined (USE_INTERNAL_EDIT) || defined (USE_DIFF_VIEW)
    ADD_KEYMAP_NAME (ShowNumbers),
#endif

    /* chattr dialog */
    ADD_KEYMAP_NAME (MarkAndDown),

#ifdef USE_INTERNAL_EDIT
    ADD_KEYMAP_NAME (Close),
    ADD_KEYMAP_NAME (Tab),
    ADD_KEYMAP_NAME (Undo),
    ADD_KEYMAP_NAME (ScrollUp),
    ADD_KEYMAP_NAME (ScrollDown),
    ADD_KEYMAP_NAME (Return),
    ADD_KEYMAP_NAME (ParagraphUp),
    ADD_KEYMAP_NAME (ParagraphDown),
    ADD_KEYMAP_NAME (EditFile),
    ADD_KEYMAP_NAME (MarkWord),
    ADD_KEYMAP_NAME (MarkLine),
    ADD_KEYMAP_NAME (MarkAll),
    ADD_KEYMAP_NAME (Unmark),
    ADD_KEYMAP_NAME (MarkColumn),
    ADD_KEYMAP_NAME (BlockSave),
    ADD_KEYMAP_NAME (InsertFile),
    ADD_KEYMAP_NAME (InsertOverwrite),
    ADD_KEYMAP_NAME (Date),
    ADD_KEYMAP_NAME (DeleteLine),
    ADD_KEYMAP_NAME (EditMail),
    ADD_KEYMAP_NAME (ParagraphFormat),
    ADD_KEYMAP_NAME (MatchBracket),
    ADD_KEYMAP_NAME (ExternalCommand),
    ADD_KEYMAP_NAME (MacroStartRecord),
    ADD_KEYMAP_NAME (MacroStopRecord),
    ADD_KEYMAP_NAME (MacroStartStopRecord),
    ADD_KEYMAP_NAME (MacroDelete),
    ADD_KEYMAP_NAME (RepeatStartStopRecord),
#ifdef HAVE_ASPELL
    ADD_KEYMAP_NAME (SpellCheck),
    ADD_KEYMAP_NAME (SpellCheckCurrentWord),
    ADD_KEYMAP_NAME (SpellCheckSelectLang),
#endif /* HAVE_ASPELL */
    ADD_KEYMAP_NAME (BookmarkFlush),
    ADD_KEYMAP_NAME (BookmarkNext),
    ADD_KEYMAP_NAME (BookmarkPrev),
    ADD_KEYMAP_NAME (MarkPageUp),
    ADD_KEYMAP_NAME (MarkPageDown),
    ADD_KEYMAP_NAME (MarkToFileBegin),
    ADD_KEYMAP_NAME (MarkToFileEnd),
    ADD_KEYMAP_NAME (MarkToPageBegin),
    ADD_KEYMAP_NAME (MarkToPageEnd),
    ADD_KEYMAP_NAME (MarkScrollUp),
    ADD_KEYMAP_NAME (MarkScrollDown),
    ADD_KEYMAP_NAME (MarkParagraphUp),
    ADD_KEYMAP_NAME (MarkParagraphDown),
    ADD_KEYMAP_NAME (MarkColumnPageUp),
    ADD_KEYMAP_NAME (MarkColumnPageDown),
    ADD_KEYMAP_NAME (MarkColumnLeft),
    ADD_KEYMAP_NAME (MarkColumnRight),
    ADD_KEYMAP_NAME (MarkColumnUp),
    ADD_KEYMAP_NAME (MarkColumnDown),
    ADD_KEYMAP_NAME (MarkColumnScrollUp),
    ADD_KEYMAP_NAME (MarkColumnScrollDown),
    ADD_KEYMAP_NAME (MarkColumnParagraphUp),
    ADD_KEYMAP_NAME (MarkColumnParagraphDown),
    ADD_KEYMAP_NAME (BlockShiftLeft),
    ADD_KEYMAP_NAME (BlockShiftRight),
    ADD_KEYMAP_NAME (InsertLiteral),
    ADD_KEYMAP_NAME (ShowTabTws),
    ADD_KEYMAP_NAME (SyntaxOnOff),
    ADD_KEYMAP_NAME (SyntaxChoose),
    ADD_KEYMAP_NAME (ShowMargin),
    ADD_KEYMAP_NAME (OptionsSaveMode),
    ADD_KEYMAP_NAME (About),
    /* An action to run external script from macro */
    {"ExecuteScript", CK_PipeBlock (0)},
    ADD_KEYMAP_NAME (WindowMove),
    ADD_KEYMAP_NAME (WindowResize),
    ADD_KEYMAP_NAME (WindowFullscreen),
    ADD_KEYMAP_NAME (WindowList),
    ADD_KEYMAP_NAME (WindowNext),
    ADD_KEYMAP_NAME (WindowPrev),
#endif /* USE_INTERNAL_EDIT */

    /* viewer */
    ADD_KEYMAP_NAME (WrapMode),
    ADD_KEYMAP_NAME (HexEditMode),
    ADD_KEYMAP_NAME (HexMode),
    ADD_KEYMAP_NAME (MagicMode),
    ADD_KEYMAP_NAME (NroffMode),
    ADD_KEYMAP_NAME (BookmarkGoto),
    ADD_KEYMAP_NAME (Ruler),
    ADD_KEYMAP_NAME (SearchForward),
    ADD_KEYMAP_NAME (SearchBackward),
    ADD_KEYMAP_NAME (SearchForwardContinue),
    ADD_KEYMAP_NAME (SearchBackwardContinue),
    ADD_KEYMAP_NAME (SearchOppositeContinue),

#ifdef USE_DIFF_VIEW
    /* diff viewer */
    ADD_KEYMAP_NAME (ShowSymbols),
    ADD_KEYMAP_NAME (SplitFull),
    ADD_KEYMAP_NAME (Tab2),
    ADD_KEYMAP_NAME (Tab3),
    ADD_KEYMAP_NAME (Tab4),
    ADD_KEYMAP_NAME (Tab8),
    ADD_KEYMAP_NAME (HunkNext),
    ADD_KEYMAP_NAME (HunkPrev),
    ADD_KEYMAP_NAME (EditOther),
    ADD_KEYMAP_NAME (Merge),
    ADD_KEYMAP_NAME (MergeOther),
#endif /* USE_DIFF_VIEW */

    {NULL, CK_IgnoreKey}
};

/* *INDENT-OFF* */
static const size_t num_command_names = G_N_ELEMENTS (command_names) - 1;
/* *INDENT-ON* */

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static int
name_keymap_comparator (const void *p1, const void *p2)
{
    const name_keymap_t *m1 = (const name_keymap_t *) p1;
    const name_keymap_t *m2 = (const name_keymap_t *) p2;

    return g_ascii_strcasecmp (m1->name, m2->name);
}

/* --------------------------------------------------------------------------------------------- */

static inline void
sort_command_names (void)
{
    static gboolean has_been_sorted = FALSE;

    if (!has_been_sorted)
    {
        qsort (command_names, num_command_names,
               sizeof (command_names[0]), &name_keymap_comparator);
        has_been_sorted = TRUE;
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
keymap_add (GArray * keymap, long key, long cmd, const char *caption)
{
    if (key != 0 && cmd != CK_IgnoreKey)
    {
        global_keymap_t new_bind;

        new_bind.key = key;
        new_bind.command = cmd;
        g_snprintf (new_bind.caption, sizeof (new_bind.caption), "%s", caption);
        g_array_append_val (keymap, new_bind);
    }
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
keybind_cmd_bind (GArray * keymap, const char *keybind, long action)
{
    char *caption = NULL;
    long key;

    key = lookup_key (keybind, &caption);
    keymap_add (keymap, key, action, caption);
    g_free (caption);
}

/* --------------------------------------------------------------------------------------------- */

long
keybind_lookup_action (const char *name)
{
    const name_keymap_t key = { name, 0 };
    name_keymap_t *res;

    sort_command_names ();

    res = bsearch (&key, command_names, num_command_names,
                   sizeof (command_names[0]), name_keymap_comparator);

    return (res != NULL) ? res->val : CK_IgnoreKey;
}

/* --------------------------------------------------------------------------------------------- */

const char *
keybind_lookup_actionname (long action)
{
    size_t i;

    for (i = 0; command_names[i].name != NULL; i++)
        if (command_names[i].val == action)
            return command_names[i].name;

    return NULL;
}

/* --------------------------------------------------------------------------------------------- */

const char *
keybind_lookup_keymap_shortcut (const global_keymap_t * keymap, long action)
{
    if (keymap != NULL)
    {
        size_t i;

        for (i = 0; keymap[i].key != 0; i++)
            if (keymap[i].command == action)
                return (keymap[i].caption[0] != '\0') ? keymap[i].caption : NULL;
    }
    return NULL;
}

/* --------------------------------------------------------------------------------------------- */

long
keybind_lookup_keymap_command (const global_keymap_t * keymap, long key)
{
    if (keymap != NULL)
    {
        size_t i;

        for (i = 0; keymap[i].key != 0; i++)
            if (keymap[i].key == key)
                return keymap[i].command;
    }

    return CK_IgnoreKey;
}

/* --------------------------------------------------------------------------------------------- */
