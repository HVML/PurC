#ifndef MC__KEYBIND_H
#define MC__KEYBIND_H

#include <sys/types.h>
#include <sys/time.h>           /* time_t */

#include "lib/global.h"

/*** typedefs(not structures) and defined constants **********************************************/

/* keymap sections */
#define KEYMAP_SECTION_FILEMANAGER "filemanager"
#define KEYMAP_SECTION_FILEMANAGER_EXT "filemanager:xmap"
#define KEYMAP_SECTION_PANEL "panel"
#define KEYMAP_SECTION_DIALOG "dialog"
#define KEYMAP_SECTION_MENU "menu"
#define KEYMAP_SECTION_INPUT "input"
#define KEYMAP_SECTION_LISTBOX "listbox"
#define KEYMAP_SECTION_RADIO "radio"
#define KEYMAP_SECTION_TREE "tree"
#define KEYMAP_SECTION_HELP "help"
#define KEYMAP_SECTION_CHATTR "chattr"
#define KEYMAP_SECTION_EDITOR "editor"
#define KEYMAP_SECTION_EDITOR_EXT "editor:xmap"
#define KEYMAP_SECTION_VIEWER "viewer"
#define KEYMAP_SECTION_VIEWER_HEX "viewer:hex"
#define KEYMAP_SECTION_DIFFVIEWER "diffviewer"

#define KEYMAP_SHORTCUT_LENGTH 32       /* FIXME: is 32 bytes enough for shortcut? */

#define CK_PipeBlock(i)  (10000+(i))
#define CK_Macro(i)      (20000+(i))
#define CK_MacroLast     CK_Macro(0x7FFF)

/*** enums ***************************************************************************************/

enum
{
    /* special commands */
    CK_InsertChar = -1L,
    CK_IgnoreKey = 0L,

    /* common */
    CK_Enter = 1L,
    CK_ChangePanel,
    CK_Up,
    CK_Down,
    CK_Left,
    CK_Right,
    CK_Home,
    CK_End,
    CK_LeftQuick,
    CK_RightQuick,
    CK_PageUp,
    CK_PageDown,
    CK_HalfPageUp,
    CK_HalfPageDown,
    CK_Top,
    CK_Bottom,
    CK_TopOnScreen,
    CK_MiddleOnScreen,
    CK_BottomOnScreen,
    CK_WordLeft,
    CK_WordRight,
    CK_Copy,
    CK_Move,
    CK_Delete,
    CK_MakeDir,
    CK_ChangeMode,
    CK_ChangeOwn,
    CK_ChangeOwnAdvanced,
    CK_ChangeAttributes,
    CK_Remove,
    CK_BackSpace,
    CK_Redo,
    CK_Clear,
    CK_Menu,
    CK_MenuLastSelected,
    CK_UserMenu,
    CK_EditUserMenu,
    CK_Search,
    CK_SearchContinue,
    CK_Replace,
    CK_ReplaceContinue,
    CK_SearchStop,
    CK_Help,
    CK_Edit,
    CK_EditNew,
    CK_Shell,
    CK_SelectCodepage,
    CK_EditorViewerHistory,
    CK_History,
    CK_HistoryNext,
    CK_HistoryPrev,
    CK_Complete,
    CK_Save,
    CK_SaveAs,
    CK_Goto,
    CK_Reread,
    CK_Refresh,
    CK_Suspend,
    CK_Swap,
    CK_Mark,
    CK_HotList,
    CK_ScreenList,
    CK_ScreenNext,
    CK_ScreenPrev,
    CK_FilePrev,
    CK_FileNext,
    CK_DeleteToHome,
    CK_DeleteToEnd,
    CK_DeleteToWordBegin,
    CK_DeleteToWordEnd,
    CK_ShowNumbers,
    CK_Store,
    CK_Cut,
    CK_Paste,
    CK_MarkLeft,
    CK_MarkRight,
    CK_MarkUp,
    CK_MarkDown,
    CK_MarkToWordBegin,
    CK_MarkToWordEnd,
    CK_MarkToHome,
    CK_MarkToEnd,
    CK_ToggleNavigation,
    CK_Sort,
    CK_Options,
    CK_LearnKeys,
    CK_Bookmark,
    CK_Quit,
    CK_QuitQuiet,
    /* C-x or similar */
    CK_ExtendedKeyMap,

    /* main commands */
    CK_EditForceInternal = 100L,
    CK_View,
    CK_ViewRaw,
    CK_ViewFile,
    CK_ViewFiltered,
    CK_Find,
    CK_DirSize,
    CK_HotListAdd,
    CK_SetupListingFormat,
    CK_CompareDirs,
    CK_OptionsVfs,
    CK_OptionsConfirm,
    CK_PutCurrentLink,
    CK_PutOtherLink,
    CK_OptionsDisplayBits,
    CK_EditExtensionsFile,
    CK_EditFileHighlightFile,
    CK_LinkSymbolicEdit,
    CK_ExternalPanelize,
    CK_Filter,
    CK_ConnectFish,
    CK_ConnectFtp,
    CK_ConnectSftp,
    CK_PanelInfo,
    CK_Jobs,
    CK_OptionsLayout,
    CK_OptionsAppearance,
    CK_Link,
    CK_PanelListing,
    CK_ListMode,
    CK_CdQuick,
    CK_PanelQuickView,
    CK_VfsList,
    CK_SaveSetup,
    CK_LinkSymbolic,
    CK_ShowHidden,
    CK_PanelTree,
    CK_Tree,
    CK_Undelete,
    CK_SplitVertHoriz,
    CK_SplitEqual,
    CK_SplitMore,
    CK_SplitLess,
    CK_CompareFiles,
    CK_OptionsPanel,
    CK_LinkSymbolicRelative,
    CK_PutCurrentPath,
    CK_PutOtherPath,
    CK_PutCurrentSelected,
    CK_PutCurrentFullSelected,
    CK_PutCurrentTagged,
    CK_PutOtherTagged,
    CK_Select,
    CK_Unselect,
    CK_SelectExt,
    CK_SelectInvert,

    /* panels */
    CK_PanelOtherCd = 200L,
    CK_PanelOtherCdLink,
    CK_Panelize,
    CK_CopySingle,
    CK_MoveSingle,
    CK_DeleteSingle,
    CK_CdChild,
    CK_CdParent,
    CK_CdParentSmart,
    CK_PanelOtherSync,
    CK_SortNext,
    CK_SortPrev,
    CK_SortReverse,
    CK_SortByName,
    CK_SortByExt,
    CK_SortBySize,
    CK_SortByMTime,
    CK_ScrollLeft,
    CK_ScrollRight,
    CK_CycleListingFormat,

    /* dialog */
    CK_Ok = 300L,
    CK_Cancel,

    /* input */
    CK_Yank = 350L,

    /* help */
    CK_Index = 400L,
    CK_Back,
    CK_LinkNext,
    CK_LinkPrev,
    CK_NodeNext,
    CK_NodePrev,

    /* tree */
    CK_Forget = 450L,

    /* chattr dialog */
    CK_MarkAndDown = 480L,

    /* editor */
    /* cursor movements */
    CK_Tab = 500L,
    CK_Undo,
    CK_ScrollUp,
    CK_ScrollDown,
    CK_Return,
    CK_ParagraphUp,
    CK_ParagraphDown,
    /* file commands */
    CK_EditFile,
    CK_InsertFile,
    CK_EditSyntaxFile,
    CK_Close,
    /* block commands */
    CK_BlockSave,
    CK_BlockShiftLeft,
    CK_BlockShiftRight,
    CK_DeleteLine,
    /* bookmarks */
    CK_BookmarkFlush,
    CK_BookmarkNext,
    CK_BookmarkPrev,
    /* mark commands */
    CK_MarkColumn,
    CK_MarkWord,
    CK_MarkLine,
    CK_MarkAll,
    CK_Unmark,
    CK_MarkPageUp,
    CK_MarkPageDown,
    CK_MarkToFileBegin,
    CK_MarkToFileEnd,
    CK_MarkToPageBegin,
    CK_MarkToPageEnd,
    CK_MarkScrollUp,
    CK_MarkScrollDown,
    CK_MarkParagraphUp,
    CK_MarkParagraphDown,
    /* column mark commands */
    CK_MarkColumnPageUp,
    CK_MarkColumnPageDown,
    CK_MarkColumnLeft,
    CK_MarkColumnRight,
    CK_MarkColumnUp,
    CK_MarkColumnDown,
    CK_MarkColumnScrollUp,
    CK_MarkColumnScrollDown,
    CK_MarkColumnParagraphUp,
    CK_MarkColumnParagraphDown,
    /* macros */
    CK_MacroStartRecord,
    CK_MacroStopRecord,
    CK_MacroStartStopRecord,
    CK_MacroDelete,
    CK_RepeatStartRecord,
    CK_RepeatStopRecord,
    CK_RepeatStartStopRecord,
    /* window commands */
    CK_WindowMove,
    CK_WindowResize,
    CK_WindowFullscreen,
    CK_WindowList,
    CK_WindowNext,
    CK_WindowPrev,
    /* misc commands */
    CK_SpellCheck,
    CK_SpellCheckCurrentWord,
    CK_SpellCheckSelectLang,
    CK_InsertOverwrite,
    CK_ParagraphFormat,
    CK_MatchBracket,
    CK_OptionsSaveMode,
    CK_About,
    CK_ShowMargin,
    CK_ShowTabTws,
    CK_SyntaxOnOff,
    CK_SyntaxChoose,
    CK_InsertLiteral,
    CK_ExternalCommand,
    CK_Date,
    CK_EditMail,

    /* viewer */
    CK_WrapMode = 600L,
    CK_MagicMode,
    CK_NroffMode,
    CK_HexMode,
    CK_HexEditMode,
    CK_BookmarkGoto,
    CK_Ruler,
    CK_SearchForward,
    CK_SearchBackward,
    CK_SearchForwardContinue,
    CK_SearchBackwardContinue,
    CK_SearchOppositeContinue,

    /* diff viewer */
    CK_ShowSymbols = 700L,
    CK_SplitFull,
    CK_Tab2,
    CK_Tab3,
    CK_Tab4,
    CK_Tab8,
    CK_HunkNext,
    CK_HunkPrev,
    CK_EditOther,
    CK_Merge,
    CK_MergeOther
};

/*** structures declarations (and typedefs of structures)*****************************************/

/* The global keymaps are of this type */
typedef struct global_keymap_t
{
    long key;
    long command;
    char caption[KEYMAP_SHORTCUT_LENGTH];
} global_keymap_t;

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

void keybind_cmd_bind (GArray * keymap, const char *keybind, long action);
long keybind_lookup_action (const char *name);
const char *keybind_lookup_actionname (long action);
const char *keybind_lookup_keymap_shortcut (const global_keymap_t * keymap, long action);
long keybind_lookup_keymap_command (const global_keymap_t * keymap, long key);

/*** inline functions ****************************************************************************/

#endif /* MC__KEYBIND_H */
