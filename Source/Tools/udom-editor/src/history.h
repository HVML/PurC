/** \file src/history.h
 *  \brief Header: defines history section names
 */

#ifndef MC__HISTORY_H
#define MC__HISTORY_H

/*** typedefs(not structures) and defined constants **********************************************/

/* history section names */

#define MC_HISTORY_EDIT_SAVE_AS       "mc.edit.save-as"
#define MC_HISTORY_EDIT_LOAD          "mc.edit.load"
#define MC_HISTORY_EDIT_SAVE_BLOCK    "mc.edit.save-block"
#define MC_HISTORY_EDIT_INSERT_FILE   "mc.edit.insert-file"
#define MC_HISTORY_EDIT_GOTO_LINE     "mc.edit.goto-line"
#define MC_HISTORY_EDIT_SORT          "mc.edit.sort"
#define MC_HISTORY_EDIT_PASTE_EXTCMD  "mc.edit.paste-extcmd"
#define MC_HISTORY_EDIT_REPEAT        "mc.edit.repeat-action"

#define MC_HISTORY_FM_VIEW_FILE       "mc.fm.view-file"
#define MC_HISTORY_FM_MKDIR           "mc.fm.mkdir"
#define MC_HISTORY_FM_LINK            "mc.fm.link"
#define MC_HISTORY_FM_EDIT_LINK       "mc.fm.edit-link"
#define MC_HISTORY_FM_TREE_COPY       "mc.fm.tree-copy"
#define MC_HISTORY_FM_TREE_MOVE       "mc.fm.tree-move"
#define MC_HISTORY_FM_PANELIZE_ADD    "mc.fm.panelize.add"
#define MC_HISTORY_FM_FILTERED_VIEW   "mc.fm.filtered-view"
#define MC_HISTORY_FM_PANEL_FILTER    "mc.fm.panel-filter"
#define MC_HISTORY_FM_MENU_EXEC_PARAM "mc.fm.menu.exec.parameter"

#define MC_HISTORY_ESC_TIMEOUT        "mc.esc.timeout"

#define MC_HISTORY_VIEW_GOTO          "mc.view.goto"
#define MC_HISTORY_VIEW_GOTO_LINE     "mc.view.goto-line"
#define MC_HISTORY_VIEW_GOTO_ADDR     "mc.view.goto-addr"
#define MC_HISTORY_VIEW_SEARCH_REGEX  "mc.view.search.regex"

#define MC_HISTORY_FTPFS_ACCOUNT      "mc.vfs.ftp.account"

#define MC_HISTORY_EXT_PARAMETER      "mc.ext.parameter"

#define MC_HISTORY_HOTLIST_ADD        "mc.hotlist.add"

#define MC_HISTORY_SHARED_SEARCH      "mc.shared.search"

#define MC_HISTORY_YDIFF_GOTO_LINE    "mc.ydiff.goto-line"

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

/*** inline functions ****************************************************************************/

#endif /* MC__HISTORY_H */
