
#ifndef MC__TTY_NCURSES_H
#define MC__TTY_NCURSES_H

/* for cchar_t, getcchar(), setcchar() */
#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif

#ifdef USE_NCURSES
#ifdef HAVE_NCURSES_CURSES_H
#include <ncurses/curses.h>
#elif defined (HAVE_NCURSES_NCURSES_H)
#include <ncurses/ncurses.h>
#elif defined (HAVE_NCURSESW_CURSES_H)
#include <ncursesw/curses.h>
#elif defined (HAVE_NCURSES_HCURSES_H) || defined (HAVE_NCURSES_H)
#include <ncurses.h>
#else
#include <curses.h>
#endif
#endif /* USE_NCURSES */

#ifdef USE_NCURSESW
#include <ncursesw/curses.h>
#endif /* USE_NCURSESW */

/* netbsd-libcurses doesn't define NCURSES_CONST */
#ifndef NCURSES_CONST
#define NCURSES_CONST const
#endif

/* do not draw shadows if NCurses is built with --disable-widec */
#if defined(NCURSES_WIDECHAR) && NCURSES_WIDECHAR
#define ENABLE_SHADOWS 1
#endif

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

/*** inline functions ****************************************************************************/

#endif /* MC_TTY_NCURSES_H */
