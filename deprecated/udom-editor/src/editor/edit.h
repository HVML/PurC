/*
   Editor public API
 */

/** \file edit.h
 *  \brief Header: editor public API
 *  \author Paul Sheer
 *  \date 1996, 1997
 *  \author Andrew Borodin
 *  \date 2009, 2012
 */

#ifndef MC__EDIT_H
#define MC__EDIT_H

#include "lib/global.h"         /* PATH_SEP_STR */
#include "lib/vfs/vfs.h"        /* vfs_path_t */

/*** typedefs(not structures) and defined constants **********************************************/

#define DEFAULT_WRAP_LINE_LENGTH 72

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/* Editor widget */
struct WEdit;
typedef struct WEdit WEdit;

/*** global variables defined in .c file *********************************************************/

extern int option_word_wrap_line_length;
extern gboolean option_typewriter_wrap;
extern gboolean option_auto_para_formatting;
extern gboolean option_fill_tabs_with_spaces;
extern gboolean option_return_does_auto_indent;
extern gboolean option_backspace_through_tabs;
extern gboolean option_fake_half_tabs;
extern gboolean option_persistent_selections;
extern gboolean option_drop_selection_on_copy;
extern gboolean option_cursor_beyond_eol;
extern gboolean option_cursor_after_inserted_block;
extern gboolean option_state_full_filename;
extern gboolean option_line_state;
extern int option_save_mode;
extern gboolean option_save_position;
extern gboolean option_syntax_highlighting;
extern gboolean option_group_undo;
extern char *option_backup_ext;
extern char *option_filesize_threshold;
extern char *option_stop_format_chars;

extern gboolean edit_confirm_save;

extern gboolean visible_tabs;
extern gboolean visible_tws;

extern gboolean simple_statusbar;
extern gboolean option_check_nl_at_eof;
extern gboolean show_right_margin;

/*** declarations of public functions ************************************************************/

/* used in main() */
void edit_stack_init (void);
void edit_stack_free (void);

gboolean edit_file (const vfs_path_t * file_vpath, long line);
gboolean edit_files (const GList * files);

const char *edit_get_file_name (const WEdit * edit);
off_t edit_get_cursor_offset (const WEdit * edit);
long edit_get_curs_col (const WEdit * edit);
const char *edit_get_syntax_type (const WEdit * edit);

/*** inline functions ****************************************************************************/
#endif /* MC__EDIT_H */
