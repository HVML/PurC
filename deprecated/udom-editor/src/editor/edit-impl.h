/*
   editor private API
 */

/** \file edit-impl.h
 *  \brief Header: editor low level data handling and cursor fundamentals
 *  \author Paul Sheer
 *  \date 1996, 1997
 */

#ifndef MC__EDIT_IMPL_H
#define MC__EDIT_IMPL_H

#include <stdio.h>

#include "lib/search.h"         /* mc_search_type_t */
#include "lib/widget.h"         /* cb_ret_t */
#include "lib/vfs/vfs.h"        /* vfs_path_t */

#include "edit.h"

/*** typedefs(not structures) and defined constants **********************************************/

#define REDRAW_LINE          (1 << 0)
#define REDRAW_LINE_ABOVE    (1 << 1)
#define REDRAW_LINE_BELOW    (1 << 2)
#define REDRAW_AFTER_CURSOR  (1 << 3)
#define REDRAW_BEFORE_CURSOR (1 << 4)
#define REDRAW_PAGE          (1 << 5)
#define REDRAW_IN_BOUNDS     (1 << 6)
#define REDRAW_CHAR_ONLY     (1 << 7)
#define REDRAW_COMPLETELY    (1 << 8)

#define EDIT_TEXT_HORIZONTAL_OFFSET 0
#define EDIT_TEXT_VERTICAL_OFFSET   0

#define EDIT_RIGHT_EXTREME 0
#define EDIT_LEFT_EXTREME 0
#define EDIT_TOP_EXTREME 0
#define EDIT_BOTTOM_EXTREME 0

/* Initial size of the undo stack, in bytes */
#define START_STACK_SIZE 32

/* Some codes that may be pushed onto or returned from the undo stack */
#define CURS_LEFT       601
#define CURS_RIGHT      602
#define DELCHAR         603
#define BACKSPACE       604
#define STACK_BOTTOM    605
#define CURS_LEFT_LOTS  606
#define CURS_RIGHT_LOTS 607
#define COLUMN_ON       608
#define COLUMN_OFF      609
#define DELCHAR_BR      610
#define BACKSPACE_BR    611
#define MARK_1          1000
#define MARK_2          500000000
#define MARK_CURS       1000000000
#define KEY_PRESS       1500000000

/* Tabs spaces: (sofar only HALF_TAB_SIZE is used: */
#define TAB_SIZE      option_tab_spacing
#define HALF_TAB_SIZE ((int) option_tab_spacing / 2)

/* max count stack files */
#define MAX_HISTORY_MOVETO     50
#define LINE_STATE_WIDTH 8

#define LB_NAMES (LB_MAC + 1)

#define get_sys_error(s) (s)

#define edit_error_dialog(h,s) query_dialog (h, s, D_ERROR, 1, _("&Dismiss"))
#define edit_query_dialog(h,s) query_dialog (h, s, D_NORMAL, 1, _("&Dismiss"))
#define edit_query_dialog2(h,t,a,b) query_dialog (h, t, D_NORMAL, 2, a, b)
#define edit_query_dialog3(h,t,a,b,c) query_dialog (h, t, D_NORMAL, 3, a, b, c)

/*** enums ***************************************************************************************/

/* line breaks */
typedef enum
{
    LB_ASIS = 0,
    LB_UNIX,
    LB_WIN,
    LB_MAC
} LineBreaks;

typedef enum
{
    EDIT_QUICK_SAVE = 0,
    EDIT_SAFE_SAVE,
    EDIT_DO_BACKUP
} edit_save_mode_t;

/*** structures declarations (and typedefs of structures)*****************************************/

/* search/replace options */
typedef struct edit_search_options_t
{
    mc_search_type_t type;
    gboolean case_sens;
    gboolean backwards;
    gboolean only_in_selection;
    gboolean whole_words;
    gboolean all_codepages;
} edit_search_options_t;

typedef struct edit_stack_type
{
    long line;
    vfs_path_t *filename_vpath;
} edit_stack_type;

/*** global variables defined in .c file *********************************************************/

extern const char VERTICAL_MAGIC[5];
/* if enable_show_tabs_tws == TRUE then use visible_tab visible_tws */
extern gboolean enable_show_tabs_tws;

extern edit_search_options_t edit_search_options;

extern unsigned int edit_stack_iterator;
extern edit_stack_type edit_history_moveto[MAX_HISTORY_MOVETO];

extern int option_line_state_width;

extern int option_max_undo;
extern gboolean option_auto_syntax;

extern gboolean search_create_bookmark;

extern char *edit_window_state_char;
extern char *edit_window_close_char;

/*** declarations of public functions ************************************************************/

gboolean edit_add_window (WDialog * h, int y, int x, int lines, int cols,
                          const vfs_path_t * f, long fline);
WEdit *find_editor (const WDialog * h);
gboolean edit_widget_is_editor (const Widget * w);
gboolean edit_drop_hotkey_menu (WDialog * h, int key);
void edit_menu_cmd (WDialog * h);
void user_menu (WEdit * edit, const char *menu_file, int selected_entry);
void edit_init_menu (WMenuBar * menubar);
void edit_save_mode_cmd (void);
off_t edit_move_forward3 (const WEdit * edit, off_t current, long cols, off_t upto);
void edit_scroll_screen_over_cursor (WEdit * edit);
void edit_render_keypress (WEdit * edit);
void edit_scroll_upward (WEdit * edit, long i);
void edit_scroll_downward (WEdit * edit, long i);
void edit_scroll_right (WEdit * edit, long i);
void edit_scroll_left (WEdit * edit, long i);
void edit_move_up (WEdit * edit, long i, gboolean do_scroll);
void edit_move_down (WEdit * edit, long i, gboolean do_scroll);
void edit_move_to_prev_col (WEdit * edit, off_t p);
long edit_get_col (const WEdit * edit);
void edit_update_curs_row (WEdit * edit);
void edit_update_curs_col (WEdit * edit);
void edit_find_bracket (WEdit * edit);
gboolean edit_reload_line (WEdit * edit, const vfs_path_t * filename_vpath, long line);
void edit_set_codeset (WEdit * edit);

void edit_block_copy_cmd (WEdit * edit);
void edit_block_move_cmd (WEdit * edit);
int edit_block_delete_cmd (WEdit * edit);
void edit_delete_line (WEdit * edit);

int edit_delete (WEdit * edit, gboolean byte_delete);
int edit_backspace (WEdit * edit, gboolean byte_delete);
void edit_insert (WEdit * edit, int c);
void edit_insert_over (WEdit * edit);
void edit_cursor_move (WEdit * edit, off_t increment);
void edit_push_undo_action (WEdit * edit, long c);
void edit_push_redo_action (WEdit * edit, long c);
void edit_push_key_press (WEdit * edit);
void edit_insert_ahead (WEdit * edit, int c);
off_t edit_write_stream (WEdit * edit, FILE * f);
char *edit_get_write_filter (const vfs_path_t * write_name_vpath,
                             const vfs_path_t * filename_vpath);
gboolean edit_save_confirm_cmd (WEdit * edit);
gboolean edit_save_as_cmd (WEdit * edit);
WEdit *edit_init (WEdit * edit, int y, int x, int lines, int cols,
                  const vfs_path_t * filename_vpath, long line);
gboolean edit_clean (WEdit * edit);
gboolean edit_ok_to_exit (WEdit * edit);
gboolean edit_load_cmd (WDialog * h);
gboolean edit_load_file_from_history (WDialog * h);
gboolean edit_load_syntax_file (WDialog * h);
gboolean edit_load_menu_file (WDialog * h);
gboolean edit_close_cmd (WEdit * edit);
void edit_mark_cmd (WEdit * edit, gboolean unmark);
void edit_mark_current_word_cmd (WEdit * edit);
void edit_mark_current_line_cmd (WEdit * edit);
void edit_set_markers (WEdit * edit, off_t m1, off_t m2, long c1, long c2);
void edit_push_markers (WEdit * edit);

gboolean edit_save_block (WEdit * edit, const char *filename, off_t start, off_t finish);
gboolean edit_save_block_cmd (WEdit * edit);
gboolean edit_insert_file_cmd (WEdit * edit);

off_t edit_insert_file (WEdit * edit, const vfs_path_t * filename_vpath);
gboolean edit_load_back_cmd (WEdit * edit);
gboolean edit_load_forward_cmd (WEdit * edit);
void edit_block_process_cmd (WEdit * edit, int macro_number);
void edit_refresh_cmd (void);
void edit_syntax_onoff_cmd (WDialog * h);
void edit_show_tabs_tws_cmd (WDialog * h);
void edit_show_margin_cmd (WDialog * h);
void edit_show_numbers_cmd (WDialog * h);
void edit_date_cmd (WEdit * edit);
void edit_goto_cmd (WEdit * edit);
gboolean eval_marks (WEdit * edit, off_t * start_mark, off_t * end_mark);
void edit_status (WEdit * edit, gboolean active);
void edit_execute_key_command (WEdit * edit, long command, int char_for_insertion);
void edit_update_screen (WEdit * edit);
void edit_save_size (WEdit * edit);
gboolean edit_handle_move_resize (WEdit * edit, long command);
void edit_toggle_fullscreen (WEdit * edit);
void edit_move_to_line (WEdit * e, long line);
void edit_move_display (WEdit * e, long line);
void edit_word_wrap (WEdit * edit);
int edit_sort_cmd (WEdit * edit);
int edit_ext_cmd (WEdit * edit);

gboolean edit_copy_to_X_buf_cmd (WEdit * edit);
gboolean edit_cut_to_X_buf_cmd (WEdit * edit);
gboolean edit_paste_from_X_buf_cmd (WEdit * edit);

void edit_select_codepage_cmd (WEdit * edit);
void edit_insert_literal_cmd (WEdit * edit);

void edit_paste_from_history (WEdit * edit);

void edit_set_filename (WEdit * edit, const vfs_path_t * name_vpath);

void edit_load_syntax (WEdit * edit, GPtrArray * pnames, const char *type);
void edit_free_syntax_rules (WEdit * edit);
int edit_get_syntax_color (WEdit * edit, off_t byte_index);
void edit_syntax_dialog (WEdit * edit);

void book_mark_insert (WEdit * edit, long line, int c);
gboolean book_mark_query_color (WEdit * edit, long line, int c);
struct edit_book_mark_t *book_mark_find (WEdit * edit, long line);
gboolean book_mark_clear (WEdit * edit, long line, int c);
void book_mark_flush (WEdit * edit, int c);
void book_mark_inc (WEdit * edit, long line);
void book_mark_dec (WEdit * edit, long line);
void book_mark_serialize (WEdit * edit, int color);
void book_mark_restore (WEdit * edit, int color);

gboolean edit_line_is_blank (WEdit * edit, long line);
gboolean is_break_char (char c);
void edit_options_dialog (WDialog * h);
void edit_mail_dialog (WEdit * edit);
void format_paragraph (WEdit * edit, gboolean force);

/* either command or char_for_insertion must be passed as -1 */
void edit_execute_cmd (WEdit * edit, long command, int char_for_insertion);

int editcmd_dialog_raw_key_query (const char *heading, const char *query, gboolean cancel);

/*** inline functions ****************************************************************************/

/**
 * Load a new file into the editor.  If it fails, preserve the old file.
 * To do it, allocate a new widget, initialize it and, if the new file
 * was loaded, copy the data to the old widget.
 *
 * @return TRUE on success, FALSE on failure.
 */
static inline gboolean
edit_reload (WEdit * edit, const vfs_path_t * filename_vpath)
{
    return edit_reload_line (edit, filename_vpath, 0);
}

#endif /* MC__EDIT_IMPL_H */
