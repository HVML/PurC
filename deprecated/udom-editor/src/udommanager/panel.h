/** \file panel.h
 *  \brief Header: defines WPanel structure
 */

#ifndef MC__PANEL_H
#define MC__PANEL_H

#include <inttypes.h>           /* uintmax_t */
#include <limits.h>             /* MB_LEN_MAX */

#include "lib/global.h"         /* gboolean */
#include "lib/fs.h"             /* MC_MAXPATHLEN */
#include "lib/strutil.h"
#include "lib/widget.h"         /* Widget */
#include "lib/filehighlight.h"

/*** typedefs(not structures) and defined constants **********************************************/

#define PANEL(x) ((WPanel *)(x))
#define selection(p) (&(p->dir.list[p->selected]))
#define DEFAULT_USER_FORMAT "half type name | size | perm"

#define LIST_FORMATS 4

#define UP_KEEPSEL ((char *) -1)

/*** enums ***************************************************************************************/

typedef enum
{
    list_full,                  /* Name, size, perm/date */
    list_brief,                 /* Name */
    list_long,                  /* Like ls -l */
    list_user                   /* User defined */
} list_format_t;

typedef enum
{
    frame_full,                 /* full screen frame */
    frame_half                  /* half screen frame */
} panel_display_t;

typedef enum
{
    UP_OPTIMIZE = 0,
    UP_RELOAD = 1,
    UP_ONLY_CURRENT = 2
} panel_update_flags_t;

/* selection flags */
typedef enum
{
    SELECT_FILES_ONLY = 1 << 0,
    SELECT_MATCH_CASE = 1 << 1,
    SELECT_SHELL_PATTERNS = 1 << 2
} panel_select_flags_t;

/* run mode and params */

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct panel_field_struct
{
    const char *id;
    int min_size;
    gboolean expands;
    align_crt_t default_just;
    const char *hotkey;
    const char *title_hotkey;
    gboolean is_user_choice;
    gboolean use_in_user_format;
    const char *(*string_fn) (file_entry_t *, int);
    GCompareFunc sort_routine;  /* used by mouse_sort_col() */
} panel_field_t;

typedef struct
{
    vfs_path_t *root_vpath;
} panelized_panel_t;

typedef struct
{
    Widget widget;

    char *name;                 /* The panel name */

    panel_display_t frame_size; /* half or full frame */

    gboolean active;            /* If panel is currently selected */
    gboolean dirty;             /* Should we redisplay the panel? */
    gboolean is_panelized;      /* Flag: special filelisting, can't reload */

    struct stat dir_stat;       /* Stat of current dir: used by execute () */

    vfs_path_t *cwd_vpath;      /* Current Working Directory */
    vfs_path_t *lwd_vpath;      /* Last Working Directory */

    list_format_t list_format;  /* Listing type */
    GSList *format;             /* Display format */
    char *user_format;          /* User format */
    int list_cols;              /* Number of file list columns */
    int brief_cols;             /* Number of columns in case of list_brief format */

    GSList *status_format;      /* Mini status format */
    gboolean user_mini_status;  /* Is user_status_format used */
    char *user_status_format[LIST_FORMATS];     /* User format for status line */

    char *filter;               /* File name filter */

    int content_shift;          /* Number of characters of filename need to skip from left side. */
    int max_shift;              /* Max shift for visible part of current panel */
} WPanel;

/*** global variables defined in .c file *********************************************************/

extern panelized_panel_t panelized_panel;

extern hook_t *select_file_hook;

extern mc_fhl_t *mc_filehighlight;

/*** declarations of public functions ************************************************************/

WPanel *panel_sized_empty_new (const char *panel_name, int y, int x, int lines, int cols);
WPanel *panel_sized_with_dir_new (const char *panel_name, int y, int x, int lines, int cols,
                                  const vfs_path_t * vpath);

void panel_clean_dir (WPanel * panel);

void panel_reload (WPanel * panel);
void panel_set_sort_order (WPanel * panel, const panel_field_t * sort_order);
void panel_re_sort (WPanel * panel);

void update_panels (panel_update_flags_t flags, const char *current_file);
int set_panel_formats (WPanel * p);

void try_to_select (WPanel * panel, const char *name);

void unmark_files (WPanel * panel);
void select_item (WPanel * panel);

void recalculate_panel_summary (WPanel * panel);
void file_mark (WPanel * panel, int idx, int val);
void do_file_mark (WPanel * panel, int idx, int val);

gsize panel_get_num_of_sortable_fields (void);
char **panel_get_sortable_fields (gsize * array_size);
const panel_field_t *panel_get_field_by_id (const char *name);
const panel_field_t *panel_get_field_by_title (const char *name);
const panel_field_t *panel_get_field_by_title_hotkey (const char *name);
gsize panel_get_num_of_user_possible_fields (void);
char **panel_get_user_possible_fields (gsize * array_size);
void panel_set_cwd (WPanel * panel, const vfs_path_t * vpath);
void panel_set_lwd (WPanel * panel, const vfs_path_t * vpath);

void panel_init (void);
void panel_deinit (void);

/* --------------------------------------------------------------------------------------------- */
/*** inline functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */
/**
 * Empty panel creation.
 *
 * @param panel_name name of panel for setup retieving
 *
 * @return new instance of WPanel
 */

static inline WPanel *
panel_empty_new (const char *panel_name)
{
    /* Unknown sizes of the panel at startup */
    return panel_sized_empty_new (panel_name, 0, 0, 1, 1);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Panel creation for specified directory.
 *
 * @param panel_name name of panel for setup retieving
 * @param vpath working panel directory. If NULL then current directory is used
 *
 * @return new instance of WPanel
 */

static inline WPanel *
panel_with_dir_new (const char *panel_name, const vfs_path_t * vpath)
{
    /* Unknown sizes of the panel at startup */
    return panel_sized_with_dir_new (panel_name, 0, 0, 1, 1, vpath);
}


/* --------------------------------------------------------------------------------------------- */
/**
 * Panel creation.
 *
 * @param panel_name name of panel for setup retieving
 *
 * @return new instance of WPanel
 */

static inline WPanel *
panel_new (const char *panel_name)
{
    return panel_with_dir_new (panel_name, NULL);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Panel creation with specified size.
 *
 * @param panel_name name of panel for setup retieving
 * @param y y coordinate of top-left corner
 * @param x x coordinate of top-left corner
 * @param lines vertical size
 * @param cols horizontal size
 *
 * @return new instance of WPanel
 */

static inline WPanel *
panel_sized_new (const char *panel_name, int y, int x, int lines, int cols)
{
    return panel_sized_with_dir_new (panel_name, y, x, lines, cols, NULL);
}

/* --------------------------------------------------------------------------------------------- */

#endif /* MC__PANEL_H */
