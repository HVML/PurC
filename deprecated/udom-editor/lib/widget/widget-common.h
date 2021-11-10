
/** \file widget-common.h
 *  \brief Header: shared stuff of widgets
 */

#ifndef MC__WIDGET_INTERNAL_H
#define MC__WIDGET_INTERNAL_H

#include "lib/keybind.h"        /* global_keymap_t */
#include "lib/tty/mouse.h"
#include "lib/widget/mouse.h"   /* mouse_msg_t, mouse_event_t */

/*** typedefs(not structures) and defined constants **********************************************/

#define WIDGET(x) ((Widget *)(x))
#define CONST_WIDGET(x) ((const Widget *)(x))

#define widget_gotoyx(w, _y, _x) tty_gotoyx (CONST_WIDGET(w)->y + (_y), CONST_WIDGET(w)->x + (_x))
/* Sets/clear the specified flag in the options field */
#define widget_want_cursor(w,i) widget_set_options(w, WOP_WANT_CURSOR, i)
#define widget_want_hotkey(w,i) widget_set_options(w, WOP_WANT_HOTKEY, i)
#define widget_want_tab(w,i) widget_set_options(w, WOP_WANT_TAB, i)
#define widget_idle(w,i) widget_set_state(w, WST_IDLE, i)
#define widget_disable(w,i) widget_set_state(w, WST_DISABLED, i)

/*** enums ***************************************************************************************/

/* Widget messages */
typedef enum
{
    MSG_INIT = 0,               /* Initialize widget */
    MSG_FOCUS,                  /* Draw widget in focused state or widget has got focus */
    MSG_UNFOCUS,                /* Draw widget in unfocused state or widget has been unfocused */
    MSG_CHANGED_FOCUS,          /* Notification to owner about focus state change */
    MSG_ENABLE,                 /* Change state to enabled */
    MSG_DISABLE,                /* Change state to disabled */
    MSG_DRAW,                   /* Draw widget on screen */
    MSG_KEY,                    /* Sent to widgets on key press */
    MSG_HOTKEY,                 /* Sent to widget to catch preprocess key */
    MSG_HOTKEY_HANDLED,         /* A widget has got the hotkey */
    MSG_UNHANDLED_KEY,          /* Key that no widget handled */
    MSG_POST_KEY,               /* The key has been handled */
    MSG_ACTION,                 /* Send to widget to handle command */
    MSG_NOTIFY,                 /* Typically sent to dialog to inform it of state-change
                                 * of listboxes, check- and radiobuttons. */
    MSG_CURSOR,                 /* Sent to widget to position the cursor */
    MSG_IDLE,                   /* The idle state is active */
    MSG_RESIZE,                 /* Screen size has changed */
    MSG_VALIDATE,               /* Dialog is to be closed */
    MSG_END,                    /* Shut down dialog */
    MSG_DESTROY                 /* Sent to widget at destruction time */
} widget_msg_t;

/* Widgets are expected to answer to the following messages:
   MSG_FOCUS:   MSG_HANDLED if the accept the focus, MSG_NOT_HANDLED if they do not.
   MSG_UNFOCUS: MSG_HANDLED if they accept to release the focus, MSG_NOT_HANDLED if they don't.
   MSG_KEY:     MSG_HANDLED if they actually used the key, MSG_NOT_HANDLED if not.
   MSG_HOTKEY:  MSG_HANDLED if they actually used the key, MSG_NOT_HANDLED if not.
 */

typedef enum
{
    MSG_NOT_HANDLED = 0,
    MSG_HANDLED = 1
} cb_ret_t;

/* Widget options */
typedef enum
{
    WOP_DEFAULT = (0 << 0),
    WOP_WANT_HOTKEY = (1 << 0),
    WOP_WANT_CURSOR = (1 << 1),
    WOP_WANT_TAB = (1 << 2),    /* Should the tab key be sent to the dialog? */
    WOP_IS_INPUT = (1 << 3),
    WOP_SELECTABLE = (1 << 4),
    WOP_TOP_SELECT = (1 << 5)
} widget_options_t;

/* Widget state */
typedef enum
{
    WST_DEFAULT = (0 << 0),
    WST_VISIBLE = (1 << 0),     /* Widget is visible */
    WST_DISABLED = (1 << 1),    /* Widget cannot be selected */
    WST_IDLE = (1 << 2),
    WST_MODAL = (1 << 3),       /* Widget (dialog) is modal */
    WST_FOCUSED = (1 << 4),

    WST_CONSTRUCT = (1 << 15),  /* Widget has been constructed but not run yet */
    WST_ACTIVE = (1 << 16),     /* Dialog is visible and active */
    WST_SUSPENDED = (1 << 17),  /* Dialog is suspended */
    WST_CLOSED = (1 << 18)      /* Dialog is closed */
} widget_state_t;

/* Flags for widget repositioning on dialog resize */
typedef enum
{
    WPOS_FULLSCREEN = (1 << 0), /* widget occupies the whole screen */
    WPOS_CENTER_HORZ = (1 << 1),        /* center widget in horizontal */
    WPOS_CENTER_VERT = (1 << 2),        /* center widget in vertical */
    WPOS_CENTER = WPOS_CENTER_HORZ | WPOS_CENTER_VERT,  /* center widget */
    WPOS_TRYUP = (1 << 3),      /* try to move two lines up the widget */
    WPOS_KEEP_LEFT = (1 << 4),  /* keep widget distance to left border of dialog */
    WPOS_KEEP_RIGHT = (1 << 5), /* keep widget distance to right border of dialog */
    WPOS_KEEP_TOP = (1 << 6),   /* keep widget distance to top border of dialog */
    WPOS_KEEP_BOTTOM = (1 << 7),        /* keep widget distance to bottom border of dialog */
    WPOS_KEEP_HORZ = WPOS_KEEP_LEFT | WPOS_KEEP_RIGHT,
    WPOS_KEEP_VERT = WPOS_KEEP_TOP | WPOS_KEEP_BOTTOM,
    WPOS_KEEP_ALL = WPOS_KEEP_HORZ | WPOS_KEEP_VERT,
    WPOS_KEEP_DEFAULT = WPOS_KEEP_LEFT | WPOS_KEEP_TOP
} widget_pos_flags_t;
/* NOTES:
 * If WPOS_FULLSCREEN is set then all other position flags are ignored.
 * If WPOS_CENTER_HORZ flag is used, other horizontal flags (WPOS_KEEP_LEFT, WPOS_KEEP_RIGHT,
 * and WPOS_KEEP_HORZ) are ignored.
 * If WPOS_CENTER_VERT flag is used, other horizontal flags (WPOS_KEEP_TOP, WPOS_KEEP_BOTTOM,
 * and WPOS_KEEP_VERT) are ignored.
 */

/*** structures declarations (and typedefs of structures)*****************************************/

/* Widget callback */
typedef cb_ret_t (*widget_cb_fn) (Widget * widget, Widget * sender, widget_msg_t msg, int parm,
                                  void *data);
/* Widget mouse callback */
typedef void (*widget_mouse_cb_fn) (Widget * w, mouse_msg_t msg, mouse_event_t * event);
/* translate mouse event and process it */
typedef int (*widget_mouse_handle_fn) (Widget * w, Gpm_Event * event);

/* Every Widget must have this as its first element */
struct Widget
{
    int x, y;
    int cols, lines;
    widget_pos_flags_t pos_flags;       /* repositioning flags */
    widget_options_t options;
    widget_state_t state;
    unsigned long id;           /* uniq widget ID */
    widget_cb_fn callback;
    widget_mouse_cb_fn mouse_callback;
    WGroup *owner;

    /* Key-related fields */
    const global_keymap_t *keymap;      /* main keymap */
    const global_keymap_t *ext_keymap;  /* extended keymap */
    gboolean ext_mode;          /* use keymap or ext_keymap */

    /* Mouse-related fields. */
    widget_mouse_handle_fn mouse_handler;
    struct
    {
        /* Public members: */
        gboolean forced_capture;        /* Overrides the 'capture' member. Set explicitly by the programmer. */

        /* Implementation details: */
        gboolean capture;       /* Whether the widget "owns" the mouse. */
        mouse_msg_t last_msg;   /* The previous event type processed. */
        int last_buttons_down;
    } mouse;

    void (*make_global) (Widget * w, const WRect * delta);
    void (*make_local) (Widget * w, const WRect * delta);

    GList *(*find) (const Widget * w, const Widget * what);
    Widget *(*find_by_type) (const Widget * w, widget_cb_fn cb);
    Widget *(*find_by_id) (const Widget * w, unsigned long id);

    /* *INDENT-OFF* */
    cb_ret_t (*set_state) (Widget * w, widget_state_t state, gboolean enable);
    /* *INDENT-ON* */
    void (*destroy) (Widget * w);

    const int *(*get_colors) (const Widget * w);
};

/* structure for label (caption) with hotkey, if original text does not contain
 * hotkey, only start is valid and is equal to original text
 * hotkey is defined as char*, but mc support only singlebyte hotkey
 */
typedef struct hotkey_t
{
    char *start;                /* never NULL */
    char *hotkey;               /* can be NULL */
    char *end;                  /* can be NULL */
} hotkey_t;

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

/* create hotkey from text */
hotkey_t hotkey_new (const char *text);
/* release hotkey, free all mebers of hotkey_t */
void hotkey_free (const hotkey_t hotkey);
/* return width on terminal of hotkey */
int hotkey_width (const hotkey_t hotkey);
/* compare two hotkeys */
gboolean hotkey_equal (const hotkey_t hotkey1, const hotkey_t hotkey2);
/* draw hotkey of widget */
void hotkey_draw (Widget * w, const hotkey_t hotkey, gboolean focused);
/* get text of hotkey */
char *hotkey_get_text (const hotkey_t hotkey);

/* widget initialization */
void widget_init (Widget * w, int y, int x, int lines, int cols,
                  widget_cb_fn callback, widget_mouse_cb_fn mouse_callback);
/* Default callback for widgets */
cb_ret_t widget_default_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm,
                                  void *data);
void widget_set_options (Widget * w, widget_options_t options, gboolean enable);
void widget_adjust_position (widget_pos_flags_t pos_flags, int *y, int *x, int *lines, int *cols);
void widget_set_size (Widget * w, int y, int x, int lines, int cols);
/* select color for widget in dependance of state */
void widget_selectcolor (Widget * w, gboolean focused, gboolean hotkey);
cb_ret_t widget_draw (Widget * w);
void widget_erase (Widget * w);
void widget_set_visibility (Widget * w, gboolean make_visible);
gboolean widget_is_active (const void *w);
gboolean widget_overlapped (const Widget * a, const Widget * b);
void widget_replace (Widget * old, Widget * new);
gboolean widget_is_focusable (const Widget * w);
void widget_select (Widget * w);
void widget_set_bottom (Widget * w);

long widget_lookup_key (Widget * w, int key);

void widget_default_make_global (Widget * w, const WRect * delta);
void widget_default_make_local (Widget * w, const WRect * delta);

GList *widget_default_find (const Widget * w, const Widget * what);
Widget *widget_default_find_by_type (const Widget * w, widget_cb_fn cb);
Widget *widget_default_find_by_id (const Widget * w, unsigned long id);

cb_ret_t widget_default_set_state (Widget * w, widget_state_t state, gboolean enable);

void widget_default_destroy (Widget * w);

/* get mouse pointer location within widget */
Gpm_Event mouse_get_local (const Gpm_Event * global, const Widget * w);
gboolean mouse_global_in_widget (const Gpm_Event * event, const Widget * w);

/* --------------------------------------------------------------------------------------------- */
/*** inline functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static inline cb_ret_t
send_message (void *w, void *sender, widget_msg_t msg, int parm, void *data)
{
    cb_ret_t ret = MSG_NOT_HANDLED;

#if 1
    if (w != NULL)              /* This must be always true, but... */
#endif
        ret = WIDGET (w)->callback (WIDGET (w), WIDGET (sender), msg, parm, data);

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Check whether one or several option flags are set or not.
  * @param w widget
  * @param options widget option flags
  *
  * @return TRUE if all requested option flags are set, FALSE otherwise.
  */

static inline gboolean
widget_get_options (const Widget * w, widget_options_t options)
{
    return ((w->options & options) == options);
}

/* --------------------------------------------------------------------------------------------- */

/**
  * Check whether one or several state flags are set or not.
  * @param w widget
  * @param state widget state flags
  *
  * @return TRUE if all requested state flags are set, FALSE otherwise.
  */

static inline gboolean
widget_get_state (const Widget * w, widget_state_t state)
{
    return ((w->state & state) == state);
}

/* --------------------------------------------------------------------------------------------- */

/**
  * Convert widget coordinates from local (relative to owner) to global (relative to screen).
  *
  * @param w widget
  */

static inline void
widget_make_global (Widget * w)
{
    w->make_global (w, NULL);
}

/* --------------------------------------------------------------------------------------------- */

/**
  * Convert widget coordinates from global (relative to screen) to local (relative to owner).
  *
  * @param w widget
  */

static inline void
widget_make_local (Widget * w)
{
    w->make_local (w, NULL);
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Find widget.
 *
 * @param w widget
 * @param what widget to find
 *
 * @return result of @w->find()
 */

static inline GList *
widget_find (const Widget * w, const Widget * what)
{
    return w->find (w, what);
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Find widget by widget type using widget callback.
 *
 * @param w widget
 * @param cb widget callback
 *
 * @return result of @w->find_by_type()
 */

static inline Widget *
widget_find_by_type (const Widget * w, widget_cb_fn cb)
{
    return w->find_by_type (w, cb);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Find widget by widget ID.
 *
 * @param w widget
 * @param id widget ID
 *
 * @return result of @w->find_by_id()
 */

static inline Widget *
widget_find_by_id (const Widget * w, unsigned long id)
{
    return w->find_by_id (w, id);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Modify state of widget.
 *
 * @param w      widget
 * @param state  widget state flag to modify
 * @param enable specifies whether to turn the flag on (TRUE) or off (FALSE).
 *               Only one flag per call can be modified.
 * @return       MSG_HANDLED if set was handled successfully, MSG_NOT_HANDLED otherwise.
 */

static inline cb_ret_t
widget_set_state (Widget * w, widget_state_t state, gboolean enable)
{
    return w->set_state (w, state, enable);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Destroy widget.
 *
 * @param w widget
 */

static inline void
widget_destroy (Widget * w)
{
    w->destroy (w);
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Get color colors of widget.
 *
 * @param w widget
 * @return  color colors
 */
static inline const int *
widget_get_colors (const Widget * w)
{
    return w->get_colors (w);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Update cursor position in the specified widget.
 *
 * @param w widget
 *
 * @return TRUE if cursor was updated successfully, FALSE otherwise
 */

static inline gboolean
widget_update_cursor (Widget * w)
{
    return (send_message (w, NULL, MSG_CURSOR, 0, NULL) == MSG_HANDLED);
}

/* --------------------------------------------------------------------------------------------- */

static inline void
widget_set_size_rect (Widget * w, const WRect * r)
{
    widget_set_size (w, r->y, r->x, r->lines, r->cols);
}

/* --------------------------------------------------------------------------------------------- */

static inline void
widget_show (Widget * w)
{
    widget_set_visibility (w, TRUE);
}

/* --------------------------------------------------------------------------------------------- */

static inline void
widget_hide (Widget * w)
{
    widget_set_visibility (w, FALSE);
}

/* --------------------------------------------------------------------------------------------- */

#endif /* MC__WIDGET_INTERNAL_H */
