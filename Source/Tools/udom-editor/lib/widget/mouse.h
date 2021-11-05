/** \file mouse.h
 *  \brief Header: Hight-level mouse API.
 *
 * This is a thin layer over the low-level mouse protocol in lib/tty/mouse.h.
 * The latter is oblivious to the regions on the screen and is therefore a
 * bit hard to use in widgets. This layer translates the low level Gpm_Event
 * into something that's easy to work with in widgets.
 */

#ifndef MC__WIDGET_MOUSE_H
#define MC__WIDGET_MOUSE_H

#include "lib/tty/mouse.h"      /* Gpm_Event */

/*** enums ***************************************************************************************/

/* Mouse messages */
typedef enum
{
    /*
     * Notes:
     * (1) "anywhere" means "inside or outside the widget".
     * (2) the mouse wheel is not considered "mouse button".
     */
    MSG_MOUSE_NONE = 0,
    MSG_MOUSE_DOWN = 1,         /* When mouse button is pressed down inside the widget. */
    MSG_MOUSE_UP,               /* When mouse button, previously pressed inside the widget, is released anywhere. */
    MSG_MOUSE_CLICK,            /* When mouse button, previously pressed inside the widget, is released inside the widget. */
    MSG_MOUSE_DRAG,             /* When a drag, initiated by button press inside the widget, occurs anywhere. */
    MSG_MOUSE_MOVE,             /* (Not currently implemented in MC.) */
    MSG_MOUSE_SCROLL_UP,        /* When mouse wheel is rotated away from the user. */
    MSG_MOUSE_SCROLL_DOWN       /* When mouse wheel is rotated towards the user. */
} mouse_msg_t;

/*** structures declarations (and typedefs of structures)*****************************************/

/* Mouse event structure. */
typedef struct
{
    mouse_msg_t msg;

    int x, y;                   /* Local to the widget. */
    int buttons;                /* Bitwise-or of: GPM_B_LEFT, GPM_B_MIDDLE, GPM_B_RIGHT */
    int count;                  /* One of: GPM_SINGLE, GPM_DOUBLE, GPM_TRIPLE */

    /* A mechanism for the callback to report back: */
    struct
    {
        gboolean abort;
        gboolean repeat;
    } result;
} mouse_event_t;

/*** typedefs(not structures) and defined constants **********************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

/* Translate GPM event to high-level event and process it */
int mouse_handle_event (Widget * w, Gpm_Event * event);

/*** inline functions ****************************************************************************/

#endif /* MC__WIDGET_MOUSE_H */
