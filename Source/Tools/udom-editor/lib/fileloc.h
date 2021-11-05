/** \file  fileloc.h
 *  \brief Header: config files list
 *
 *  This file defines the locations of the various user specific
 *  configuration files of the Midnight Commander. Historically the
 *  system wide and the user specific file names have not always been
 *  the same, so don't use these names for finding system wide
 *  configuration files.
 *
 *  \todo This inconsistency should disappear in the one of the next versions (5.0?)
 */

#ifndef MC_FILELOC_H
#define MC_FILELOC_H

/*** typedefs(not structures) and defined constants **********************************************/

#ifndef MC_USERCONF_DIR
#define MC_USERCONF_DIR         "mc"
#endif

#define TAGS_NAME               "TAGS"

#define MC_GLOBAL_CONFIG_FILE   "mc.lib"
#define MC_GLOBAL_MENU          "mc.menu"
#define MC_LOCAL_MENU           ".mc.menu"
#define MC_HINT                 "hints" PATH_SEP_STR "mc.hint"
#define MC_HELP                 "help" PATH_SEP_STR "mc.hlp"
#define GLOBAL_KEYMAP_FILE      "mc.keymap"
#define CHARSETS_LIST           "mc.charsets"
#define MC_LIB_EXT              "mc.ext"
#define MC_MACRO_FILE           "mc.macros"

#define FISH_PREFIX             "fish"

#define FISH_LS_FILE            "ls"
#define FISH_EXISTS_FILE        "fexists"
#define FISH_MKDIR_FILE         "mkdir"
#define FISH_UNLINK_FILE        "unlink"
#define FISH_CHOWN_FILE         "chown"
#define FISH_CHMOD_FILE         "chmod"
#define FISH_UTIME_FILE         "utime"
#define FISH_RMDIR_FILE         "rmdir"
#define FISH_LN_FILE            "ln"
#define FISH_MV_FILE            "mv"
#define FISH_HARDLINK_FILE      "hardlink"
#define FISH_GET_FILE           "get"
#define FISH_SEND_FILE          "send"
#define FISH_APPEND_FILE        "append"
#define FISH_INFO_FILE          "info"

#define MC_EXTFS_DIR            "extfs.d"

#define MC_BASHRC_FILE          "bashrc"
#define MC_ZSHRC_FILE           ".zshrc"
#define MC_ASHRC_FILE           "ashrc"
#define MC_INPUTRC_FILE         "inputrc"
#define MC_CONFIG_FILE          "ini"
#define MC_FILEBIND_FILE        "mc.ext"
#define MC_FILEPOS_FILE         "filepos"
#define MC_HISTORY_FILE         "history"
#define MC_HOTLIST_FILE         "hotlist"
#define MC_USERMENU_FILE        "menu"
#define MC_TREESTORE_FILE       "Tree"
#define MC_PANELS_FILE          "panels.ini"
#define MC_FHL_INI_FILE         "filehighlight.ini"

#define MC_SKINS_DIR            "skins"

/* editor home directory */
#define EDIT_HOME_DIR           "mcedit"

/* file names */
#define EDIT_HOME_MACRO_FILE    EDIT_HOME_DIR PATH_SEP_STR "macros.d" PATH_SEP_STR "macro"
#define EDIT_HOME_SYNTAX_FILE   EDIT_HOME_DIR PATH_SEP_STR "Syntax"
#define EDIT_HOME_CLIP_FILE     EDIT_HOME_DIR PATH_SEP_STR "mcedit.clip"
#define EDIT_HOME_BLOCK_FILE    EDIT_HOME_DIR PATH_SEP_STR "mcedit.block"
#define EDIT_HOME_TEMP_FILE     EDIT_HOME_DIR PATH_SEP_STR "mcedit.temp"

#define EDIT_GLOBAL_MENU        "mcedit.menu"
#define EDIT_LOCAL_MENU         ".cedit.menu"
#define EDIT_HOME_MENU          EDIT_HOME_DIR PATH_SEP_STR "menu"

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

/*** inline functions ****************************************************************************/

#endif
