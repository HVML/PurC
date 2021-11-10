/*
   Dialog box features module for the Midnight Commander
 */

/** \file dialog.h
 *  \brief Header: dialog box features module
 */

#ifndef MC__DIALOG_H
#define MC__DIALOG_H

#include <sys/types.h>          /* size_t */

#include "lib/global.h"
#include "lib/hook.h"           /* hook_t */
#include "lib/keybind.h"        /* global_keymap_t */

/*** typedefs(not structures) and defined constants **********************************************/

#define DIALOG(x) ((WDialog *)(x))
#define CONST_DIALOG(x) ((const WDialog *)(x))

/* Common return values */
/* ATTENTION: avoid overlapping with FileProgressStatus values */
#define B_EXIT          0
#define B_CANCEL        1
#define B_ENTER         2
#define B_HELP          3
#define B_USER          100

/*** enums ***************************************************************************************/

/* Dialog color constants */
typedef enum
{
    DLG_COLOR_NORMAL,
    DLG_COLOR_FOCUS,
    DLG_COLOR_HOT_NORMAL,
    DLG_COLOR_HOT_FOCUS,
    DLG_COLOR_TITLE,
    DLG_COLOR_COUNT
} dlg_colors_enum_t;

/*** typedefs(not structures) ********************************************************************/

typedef struct WDialog WDialog;

/* get string representation of shortcut assigned  with command */
/* as menu is a widget of dialog, ask dialog about shortcut string */
typedef char *(*dlg_shortcut_str) (long command);

/* get dialog name to show in dialog list */
typedef char *(*dlg_title_str) (const WDialog * h, size_t len);

typedef int dlg_colors_t[DLG_COLOR_COUNT];

/*** structures declarations (and typedefs of structures)*****************************************/

struct WDialog
{
    WGroup group;               /* base class */

    /* Set by the user */
    gboolean compact;           /* Suppress spaces around the frame */
    const char *help_ctx;       /* Name of the help entry */
    const int *colors;          /* Color set. Unused in viewer and editor */

    /* Set and received by the user */
    int ret_value;              /* Result of dlg_run() */

    /* Internal variables */
    void *data;                 /* Data can be passed to dialog */
    char *event_group;          /* Name of event group for this dialog */
    Widget *bg;                 /* WFrame or WBackground */

    dlg_shortcut_str get_shortcut;      /* Shortcut string */
    dlg_title_str get_title;    /* useless for modal dialogs */
};

/*** global variables defined in .c file *********************************************************/

/* Color styles for normal and error dialogs */
extern dlg_colors_t dialog_colors;
extern dlg_colors_t alarm_colors;
extern dlg_colors_t listbox_colors;

extern GList *top_dlg;

/* A hook list for idle events */
extern hook_t *idle_hook;

extern gboolean fast_refresh;
extern gboolean mouse_close_dialog;

extern const global_keymap_t *dialog_map;

/*** declarations of public functions ************************************************************/

/* Creates a dialog head  */
WDialog *dlg_create (gboolean modal, int y1, int x1, int lines, int cols,
                     widget_pos_flags_t pos_flags, gboolean compact,
                     const int *colors, widget_cb_fn callback, widget_mouse_cb_fn mouse_callback,
                     const char *help_ctx, const char *title);

void dlg_set_default_colors (void);

void dlg_init (WDialog * h);
int dlg_run (WDialog * d);

void dlg_run_done (WDialog * h);
void dlg_save_history (WDialog * h);
void dlg_process_event (WDialog * h, int key, Gpm_Event * event);

char *dlg_get_title (const WDialog * h, size_t len);

/* Default callbacks for dialogs */
cb_ret_t dlg_default_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data);
void dlg_default_mouse_callback (Widget * w, mouse_msg_t msg, mouse_event_t * event);

void dlg_stop (WDialog * h);

/* Redraw all dialogs */
void do_refresh (void);

/* --------------------------------------------------------------------------------------------- */
/*** inline functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

#endif /* MC__DIALOG_H */
