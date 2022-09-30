
/** \file mouse.h
 *  \brief Header: mouse managing
 *
 *  Events received by clients of this library have their coordinates 0 based
 */

#ifndef MC__MOUSE_H
#define MC__MOUSE_H

#ifdef HAVE_LIBGPM
/* GPM mouse support include file */
#include <gpm.h>
#endif /* !HAVE_LIBGPM */


/*** typedefs(not structures) and defined constants **********************************************/

#ifndef HAVE_LIBGPM
/* Equivalent definitions for non-GPM mouse support */
/* These lines are modified version from the lines appearing in the */
/* gpm.h include file of the Linux General Purpose Mouse server */

#define GPM_B_LEFT      (1 << 2)
#define GPM_B_MIDDLE    (1 << 1)
#define GPM_B_RIGHT     (1 << 0)

#define GPM_BARE_EVENTS(ev) ((ev)&0xF)
#endif /* !HAVE_LIBGPM */

/* Mouse wheel events */
#ifndef GPM_B_DOWN
#define GPM_B_DOWN      (1 << 5)
#endif

#ifndef GPM_B_UP
#define GPM_B_UP        (1 << 4)
#endif

/*** enums ***************************************************************************************/

#ifndef HAVE_LIBGPM
/* Xterm mouse support supports only GPM_DOWN and GPM_UP */
/* If you use others make sure your code also works without them */
enum Gpm_Etype
{
    GPM_MOVE = 1,
    GPM_DRAG = 2,               /* exactly one in four is active at a time */
    GPM_DOWN = 4,
    GPM_UP = 8,


    GPM_SINGLE = 16,            /* at most one in three is set */
    GPM_DOUBLE = 32,
    GPM_TRIPLE = 64,

    GPM_MFLAG = 128,            /* motion during click? */
    GPM_HARD = 256              /* if set in the defaultMask, force an already
                                   used event to pass over to another handler */
};
#endif /* !HAVE_LIBGPM */

/* Constants returned from the mouse callback */
enum
{
    MOU_UNHANDLED = 0,
    MOU_NORMAL,
    MOU_REPEAT
};

/* Type of mouse support */
typedef enum
{
    MOUSE_NONE,                 /* Not detected yet */
    MOUSE_DISABLED,             /* Explicitly disabled by -d */
    MOUSE_GPM,                  /* Support using GPM on Linux */
    MOUSE_XTERM,                /* Support using xterm-style mouse reporting */
    MOUSE_XTERM_NORMAL_TRACKING = MOUSE_XTERM,
    MOUSE_XTERM_BUTTON_EVENT_TRACKING
} Mouse_Type;

/*** structures declarations (and typedefs of structures)*****************************************/

#ifndef HAVE_LIBGPM
typedef struct Gpm_Event
{
    int buttons, x, y;
    enum Gpm_Etype type;
} Gpm_Event;
#endif /* !HAVE_LIBGPM */

/*** global variables defined in .c file *********************************************************/

/* Type of the currently used mouse */
extern Mouse_Type use_mouse_p;

/* To be used when gpm_fd were initially >= 0 */
extern int mouse_fd;

/* String indicating that a mouse event has occurred, usually "\E[M" */
extern const char *xmouse_seq;

/* String indicating that an SGR extended mouse event has occurred, namely "\E[<" */
extern const char *xmouse_extended_seq;

/*** declarations of public functions ************************************************************/

/* General (i.e. both for xterm and gpm) mouse support definitions */

void init_mouse (void);
void enable_mouse (void);
void disable_mouse (void);

void show_mouse_pointer (int x, int y);

/*** inline functions ****************************************************************************/
#endif /* MC_MOUSE_H */
