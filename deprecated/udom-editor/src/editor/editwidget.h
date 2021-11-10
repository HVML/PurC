/** \file
 *  \brief Header: editor widget WEdit
 */

#ifndef MC__EDIT_WIDGET_H
#define MC__EDIT_WIDGET_H

#include "lib/search.h"         /* mc_search_t */
#include "lib/widget.h"         /* Widget */

#include "edit-impl.h"
#include "editbuffer.h"

/*** typedefs(not structures) and defined constants **********************************************/

#define N_LINE_CACHES 32

/*** enums ***************************************************************************************/

/**
    enum for store the search conditions check results.
    (if search condition have BOL(^) or EOL ($) regexp checial characters).
*/
typedef enum
{
    AT_START_LINE = (1 << 0),
    AT_END_LINE = (1 << 1)
} edit_search_line_t;

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct edit_book_mark_t edit_book_mark_t;
struct edit_book_mark_t
{
    long line;                  /* line number */
    int c;                      /* color */
    edit_book_mark_t *next;
    edit_book_mark_t *prev;
};

typedef struct edit_syntax_rule_t edit_syntax_rule_t;
struct edit_syntax_rule_t
{
    unsigned short keyword;
    off_t end;
    unsigned char context;
    unsigned char _context;
    unsigned char border;
};

/*
 * State of WEdit window
 * MCEDIT_DRAG_NONE   - window is in normal mode
 * MCEDIT_DRAG_MOVE   - window is being moved
 * MCEDIT_DRAG_RESIZE - window is being resized
 */
typedef enum
{
    MCEDIT_DRAG_NONE = 0,
    MCEDIT_DRAG_MOVE,
    MCEDIT_DRAG_RESIZE
} mcedit_drag_state_t;

struct WEdit
{
    Widget widget;
    mcedit_drag_state_t drag_state;
    int drag_state_start;       /* save cursor position before window moving */

    /* save location before move/resize or toggle to fullscreen */
    WRect loc_prev;

    vfs_path_t *filename_vpath; /* Name of the file */
    vfs_path_t *dir_vpath;      /* NULL if filename is absolute */

    /* dynamic buffers and cursor position for editor: */
    edit_buffer_t buffer;

#ifdef HAVE_CHARSET
    /* multibyte support */
    gboolean utf8;              /* It's multibyte file codeset */
    GIConv converter;
    char charbuf[4 + 1];
    int charpoint;
#endif

    /* search handler */
    mc_search_t *search;
    int replace_mode;
    /* is search conditions should be started from BOL(^) or ended with EOL($) */
    edit_search_line_t search_line_type;

    char *last_search_string;   /* String that have been searched */
    off_t search_start;         /* First character to start searching from */
    unsigned long found_len;    /* Length of found string or 0 if none was found */
    off_t found_start;          /* the found word from a search - start position */

    /* display information */
    long start_display;         /* First char displayed */
    long start_col;             /* First displayed column, negative */
    long max_column;            /* The maximum cursor position ever reached used to calc hori scroll bar */
    long curs_row;              /* row position of cursor on the screen */
    long curs_col;              /* column position on screen */
    long over_col;              /* pos after '\n' */
    int force;                  /* how much of the screen do we redraw? */
    unsigned int overwrite:1;   /* Overwrite on type mode (as opposed to insert) */
    unsigned int modified:1;    /* File has been modified and needs saving */
    unsigned int loading_done:1;        /* File has been loaded into the editor */
    unsigned int locked:1;      /* We hold lock on current file */
    unsigned int delete_file:1; /* New file, needs to be deleted unless modified */
    unsigned int highlight:1;   /* There is a selected block */
    unsigned int column_highlight:1;
    unsigned int fullscreen:1;  /* Is window fullscreen or not */
    long prev_col;              /* recent column position of the cursor - used when moving
                                   up or down past lines that are shorter than the current line */
    long start_line;            /* line number of the top of the page */

    /* file info */
    off_t mark1;                /* position of highlight start */
    off_t mark2;                /* position of highlight end */
    off_t end_mark_curs;        /* position of cursor after end of highlighting */
    long column1;               /* position of column highlight start */
    long column2;               /* position of column highlight end */
    off_t bracket;              /* position of a matching bracket */
    off_t last_bracket;         /* previous position of a matching bracket */

    /* cache speedup for line lookups */
    gboolean caches_valid;
    long line_numbers[N_LINE_CACHES];
    off_t line_offsets[N_LINE_CACHES];

    edit_book_mark_t *book_mark;
    GArray *serialized_bookmarks;

    /* undo stack and pointers */
    unsigned long undo_stack_pointer;
    long *undo_stack;
    unsigned long undo_stack_size;
    unsigned long undo_stack_size_mask;
    unsigned long undo_stack_bottom;
    unsigned int undo_stack_disable:1;  /* If not 0, don't save events in the undo stack */

    unsigned long redo_stack_pointer;
    long *redo_stack;
    unsigned long redo_stack_size;
    unsigned long redo_stack_size_mask;
    unsigned long redo_stack_bottom;
    unsigned int redo_stack_reset:1;    /* If 1, need clear redo stack */

    struct stat stat1;          /* Result of mc_fstat() on the file */
    unsigned int skip_detach_prompt:1;  /* Do not prompt whether to detach a file anymore */

    /* syntax higlighting */
    GSList *syntax_marker;
    GPtrArray *rules;
    off_t last_get_rule;
    edit_syntax_rule_t rule;
    char *syntax_type;          /* description of syntax highlighting type being used */
    GTree *defines;             /* List of defines */
    gboolean is_case_insensitive;       /* selects language case sensitivity */

    /* line break */
    LineBreaks lb;
};

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

/*** inline functions ****************************************************************************/
#endif /* MC__EDIT_WIDGET_H */
