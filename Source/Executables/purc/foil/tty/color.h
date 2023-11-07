/** \file color.h
 *  \brief Header: color setup
 *
 * PLEASE FORGOT ABOUT tty/color.h!
 * Use skin engine for getting needed color pairs.
 *
 * edit/syntax.c may use this file directly, I'm agree. :)
 *
 */

#ifndef MC__COLOR_H
#define MC__COLOR_H

#include "../screen.h"

#ifdef HAVE_SLANG
#include "color-slang.h"
#else
#include "tty-ncurses.h"
#endif

#include <glib.h>

/*** typedefs(not structures) and defined constants **********************************************/

#define ALLOC_COLOR_PAIR_INDEX 1

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

int tty_init_colors (gboolean disable, gboolean force);
void tty_colors_done (void);

gboolean tty_use_colors (void);
int tty_try_alloc_color_pair (const char *fg, const char *bg, const char *attrs);
int tty_try_alloc_color_pair2 (const char *fg, const char *bg, const char *attrs,
                               gboolean is_temp_color);

void tty_color_free_all_tmp (void);
void tty_color_free_all_non_tmp (void);

void tty_setcolor (int color);
void tty_lowlevel_setcolor (int color);
void tty_set_normal_attrs (void);

void tty_color_set_defaults (const char *fgcolor, const char *bgcolor, const char *attrs);

extern gboolean tty_use_256colors (GError ** error);
extern gboolean tty_use_truecolors (GError ** error);

/*** inline functions ****************************************************************************/

#endif /* MC__COLOR_H */
