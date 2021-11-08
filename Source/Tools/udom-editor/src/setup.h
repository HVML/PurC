/** \file setup.h
 *  \brief Header: setup loading/saving
 */

#ifndef MC__SETUP_H
#define MC__SETUP_H

#include "ue-config.h"

#include "lib/global.h"         /* GError */

/*** typedefs(not structures) and defined constants **********************************************/

/* TAB length for editor and viewer */
#define DEFAULT_TAB_SPACING 8

#define MAX_MACRO_LENGTH 1024

/*** enums ***************************************************************************************/

typedef enum
{
    QSEARCH_CASE_INSENSITIVE = 0,       /* quick search in case insensitive mode */
    QSEARCH_CASE_SENSITIVE = 1, /* quick search in case sensitive mode */
    QSEARCH_PANEL_CASE = 2,     /* quick search get value from panel case_sensitive */
    QSEARCH_NUM
} qsearch_mode_t;

/*** structures declarations (and typedefs of structures)*****************************************/

/* panels ini options; [Panels] section */
typedef struct
{
    gboolean show_mini_info;    /* If true, show the mini-info on the panel */
    gboolean kilobyte_si;       /* If TRUE, SI units (1000 based) will be used for larger units
                                 * (kilobyte, megabyte, ...). If FALSE, binary units (1024 based) will be used */
    gboolean mix_all_files;     /* If FALSE then directories are shown separately from files */
    gboolean show_backups;      /* If TRUE, show files ending in ~ */
    gboolean show_dot_files;    /* If TRUE, show files starting with a dot */
    gboolean fast_reload;       /* If TRUE then use stat() on the cwd to determine directory changes */
    gboolean fast_reload_msg_shown;     /* Have we shown the fast-reload warning in the past? */
    gboolean mark_moves_down;   /* If TRUE, marking a files moves the cursor down */
    gboolean reverse_files_only;        /* If TRUE, only selection of files is inverted */
    gboolean auto_save_setup;
    gboolean navigate_with_arrows;      /* If TRUE: l&r arrows are used to chdir if the input line is empty */
    gboolean scroll_pages;      /* If TRUE, panel is scrolled by half the display when the cursor reaches
                                   the end or the beginning of the panel */
    gboolean scroll_center;     /* If TRUE, scroll when the cursor hits the middle of the panel */
    gboolean mouse_move_pages;  /* Scroll page/item using mouse wheel */
    qsearch_mode_t qsearch_mode;        /* Quick search mode */
    gboolean torben_fj_mode;    /* If TRUE, use some usability hacks by Torben */
} panels_options_t;

typedef struct macro_action_t
{
    long action;
    int ch;
} macro_action_t;

typedef struct macros_t
{
    int hotkey;
    GArray *macro;
} macros_t;

struct mc_fhl_struct;

/*** global variables defined in .c file *********************************************************/

/* global parameters */
extern char *global_profile_name;
extern gboolean confirm_delete;
extern gboolean confirm_directory_hotlist_delete;
extern gboolean confirm_execute;
extern gboolean confirm_exit;
extern gboolean confirm_overwrite;
extern gboolean confirm_view_dir;
extern gboolean safe_delete;
extern gboolean safe_overwrite;
extern gboolean clear_before_exec;
extern gboolean auto_menu;
extern gboolean drop_menus;
extern gboolean verbose;
extern gboolean copymove_persistent_attr;
extern gboolean classic_progressbar;
extern gboolean easy_patterns;
extern int option_tab_spacing;
extern gboolean auto_save_setup;
extern gboolean only_leading_plus_minus;
extern int cd_symlinks;
extern gboolean auto_fill_mkdir_name;
extern gboolean output_starts_shell;
extern gboolean use_file_to_check_type;
extern gboolean file_op_compute_totals;
extern gboolean editor_ask_filename_before_edit;

extern panels_options_t panels_options;

extern gboolean boot_current_is_left;
extern gboolean use_internal_view;
extern gboolean use_internal_edit;

/* Value of "other_dir" key in ini file */
extern char *saved_other_dir;

/* If set, then print to the given file the last directory we were at */
extern char *last_wd_string;

extern int quit;
/* Set to TRUE to suppress printing the last directory */
extern gboolean print_last_revert;

#ifdef USE_INTERNAL_EDIT
/* index to record_macro_buf[], -1 if not recording a macro */
extern int macro_index;

/* macro stuff */
extern struct macro_action_t record_macro_buf[MAX_MACRO_LENGTH];

extern GArray *macros_list;
#endif /* USE_INTERNAL_EDIT */

extern int saving_setup;

/*** declarations of public functions ************************************************************/

const char *setup_init (void);
void load_setup (void);
gboolean save_setup (gboolean save_options, gboolean save_panel_options);
void done_setup (void);
void setup_save_config_show_error (const char *filename, GError ** mcerror);

void load_key_defs (void);

/*** inline functions ****************************************************************************/

#endif /* MC__SETUP_H */
