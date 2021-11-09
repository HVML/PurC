/*
   Widgets for the Midnight Commander

   Copyright (C) 2016-2021
   Free Software Foundation, Inc.

   Authors:
   Human beings.

   This file is part of the Midnight Commander.

   The Midnight Commander is free software: you can redistribute it
   and/or modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   The Midnight Commander is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** \file mouse.c
 *  \brief Header: High-level mouse API
 */

#include <config.h>

#include "lib/global.h"
#include "lib/widget.h"

#include "lib/widget/mouse.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/**
 * Constructs a mouse event structure.
 *
 * It receives a Gpm_Event event and translates it into a higher level protocol.
 *
 * Tip: for details on the C mouse API, see MC's lib/tty/mouse.h,
 * or GPM's excellent 'info' manual:
 *
 *    http://www.fifi.org/cgi-bin/info2www?(gpm)Event+Types
 */
static void
init_mouse_event (mouse_event_t * event, mouse_msg_t msg, const Gpm_Event * global_gpm,
                  const Widget * w)
{
    event->msg = msg;
    event->x = global_gpm->x - w->x - 1;        /* '-1' because Gpm_Event is 1-based. */
    event->y = global_gpm->y - w->y - 1;
    event->count = global_gpm->type & (GPM_SINGLE | GPM_DOUBLE | GPM_TRIPLE);
    event->buttons = global_gpm->buttons;
    event->result.abort = FALSE;
    event->result.repeat = FALSE;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Translate GPM event to high-level event,
 *
 * @param w Widget object
 * @param event GPM event
 *
 * @return high level mouse event
 */
static mouse_event_t
mouse_translate_event (Widget * w, Gpm_Event * event)
{
    gboolean in_widget;
    mouse_msg_t msg = MSG_MOUSE_NONE;
    mouse_event_t local;

    /*
     * Very special widgets may want to control area outside their bounds.
     * For such widgets you will have to turn on the 'forced_capture' flag.
     * You'll also need, in your mouse handler, to inform the system of
     * events you want to pass on by setting 'event->result.abort' to TRUE.
     */
    in_widget = w->mouse.forced_capture || mouse_global_in_widget (event, w);

    if ((event->type & GPM_DOWN) != 0)
    {
        if (in_widget)
        {
            if ((event->buttons & GPM_B_UP) != 0)
                msg = MSG_MOUSE_SCROLL_UP;
            else if ((event->buttons & GPM_B_DOWN) != 0)
                msg = MSG_MOUSE_SCROLL_DOWN;
            else
            {
                /* Handle normal buttons: anything but the mouse wheel's.
                 *
                 * (Note that turning on capturing for the mouse wheel
                 * buttons doesn't make sense as they don't generate a
                 * mouse_up event, which means we'd never get uncaptured.)
                 */
                w->mouse.capture = TRUE;
                msg = MSG_MOUSE_DOWN;

                w->mouse.last_buttons_down = event->buttons;
            }
        }
    }
    else if ((event->type & GPM_UP) != 0)
    {
        /* We trigger the mouse_up event even when !in_widget. That's
         * because, for example, a paint application should stop drawing
         * lines when the button is released even outside the canvas. */
        if (w->mouse.capture)
        {
            w->mouse.capture = FALSE;
            msg = MSG_MOUSE_UP;

            /*
             * When using xterm, event->buttons reports the buttons' state
             * after the event occurred (meaning that event->buttons is zero,
             * because the mouse button is now released). When using GPM,
             * however, that field reports the button(s) that was released.
             *
             * The following makes xterm behave effectively like GPM:
             */
            if (event->buttons == 0)
                event->buttons = w->mouse.last_buttons_down;
        }
    }
    else if ((event->type & GPM_DRAG) != 0)
    {
        if (w->mouse.capture)
            msg = MSG_MOUSE_DRAG;
    }
    else if ((event->type & GPM_MOVE) != 0)
    {
        if (in_widget)
            msg = MSG_MOUSE_MOVE;
    }

    init_mouse_event (&local, msg, event, w);

    return local;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Call widget mouse handler to process high-level mouse event.
 *
 * Besides sending to the widget the event itself, this function may also
 * send one or more pseudo events. Currently, MSG_MOUSE_CLICK is the only
 * pseudo event in existence but in the future (e.g., with the introduction
 * of a drag-drop API) there may be more.
 *
 * @param w Widget object
 * @param event high level mouse event
 *
 * @return result of mouse event handling
 */
static int
mouse_process_event (Widget * w, mouse_event_t * event)
{
    int ret = MOU_UNHANDLED;

    if (event->msg != MSG_MOUSE_NONE)
    {
        w->mouse_callback (w, event->msg, event);

        /* If a widget aborts a MSG_MOUSE_DOWN, we uncapture it so it
         * doesn't steal events from other widgets. */
        if (event->msg == MSG_MOUSE_DOWN && event->result.abort)
            w->mouse.capture = FALSE;

        /* Upon releasing the mouse button: if the mouse hasn't been dragged
         * since the MSG_MOUSE_DOWN, we also trigger a click. */
        if (event->msg == MSG_MOUSE_UP && w->mouse.last_msg == MSG_MOUSE_DOWN)
            w->mouse_callback (w, MSG_MOUSE_CLICK, event);

        /* Record the current event type for the benefit of the next event. */
        w->mouse.last_msg = event->msg;

        if (!event->result.abort)
            ret = event->result.repeat ? MOU_REPEAT : MOU_NORMAL;
    }

    return ret;
}


/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/**
 * Translate GPM event to high-level event and process it
 *
 * @param w Widget object
 * @param event GPM event
 *
 * @return result of mouse event handling
 */
int
mouse_handle_event (Widget * w, Gpm_Event * event)
{
    mouse_event_t me;

    me = mouse_translate_event (w, event);

    return mouse_process_event (w, &me);
}

/* --------------------------------------------------------------------------------------------- */
