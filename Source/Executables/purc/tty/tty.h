
/** \file tty.h
 *  \brief Header: %interface to the terminal controlling library
 *
 *  This file is the %interface to the terminal controlling library:
 *  slang or ncurses. It provides an additional layer of abstraction
 *  above the "real" libraries to keep the number of ifdefs in the other
 *  files small.
 */

#ifndef MC__TTY_H
#define MC__TTY_H

#include "foil.h"

#ifdef HAVE_SLANG
#include "tty-slang.h"
#else
#include "tty-ncurses.h"
#endif

/*** typedefs(not structures) and defined constants **********************************************/

#define KEY_KP_ADD      4001
#define KEY_KP_SUBTRACT 4002
#define KEY_KP_MULTIPLY 4003

/*** enums ***************************************************************************************/

typedef enum
{
    /* single lines */
    MC_TTY_FRM_VERT,
    MC_TTY_FRM_HORIZ,
    MC_TTY_FRM_LEFTTOP,
    MC_TTY_FRM_RIGHTTOP,
    MC_TTY_FRM_LEFTBOTTOM,
    MC_TTY_FRM_RIGHTBOTTOM,
    MC_TTY_FRM_TOPMIDDLE,
    MC_TTY_FRM_BOTTOMMIDDLE,
    MC_TTY_FRM_LEFTMIDDLE,
    MC_TTY_FRM_RIGHTMIDDLE,
    MC_TTY_FRM_CROSS,

    /* double lines */
    MC_TTY_FRM_DVERT,
    MC_TTY_FRM_DHORIZ,
    MC_TTY_FRM_DLEFTTOP,
    MC_TTY_FRM_DRIGHTTOP,
    MC_TTY_FRM_DLEFTBOTTOM,
    MC_TTY_FRM_DRIGHTBOTTOM,
    MC_TTY_FRM_DTOPMIDDLE,
    MC_TTY_FRM_DBOTTOMMIDDLE,
    MC_TTY_FRM_DLEFTMIDDLE,
    MC_TTY_FRM_DRIGHTMIDDLE,

    MC_TTY_FRM_MAX
} mc_tty_frm_t;

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

extern int mc_tty_frm[];

extern char *tty_tgetstr (const char *name);

/*** declarations of public functions ************************************************************/

extern void tty_beep (void);

/* {{{ Input }}} */

extern bool tty_check_term (bool force_xterm);
extern void tty_init (bool mouse_enable, bool is_xterm);
extern void tty_shutdown (void);

extern void tty_start_interrupt_key (void);
extern void tty_enable_interrupt_key (void);
extern void tty_disable_interrupt_key (void);
extern bool tty_got_interrupt (void);

extern bool tty_got_winch (void);
extern void tty_flush_winch (void);

extern void tty_reset_prog_mode (void);
extern void tty_reset_shell_mode (void);

extern void tty_raw_mode (void);
extern void tty_noraw_mode (void);

extern void tty_noecho (void);
extern int tty_flush_input (void);

extern void tty_keypad (bool set);
extern void tty_nodelay (bool set);
extern int tty_baudrate (void);

/* {{{ Output }}} */

/*
   The output functions do not check themselves for screen overflows,
   so make sure that you never write more than what fits on the screen.
   While SLang provides such a feature, ncurses does not.
 */

extern int tty_reset_screen (void);
extern void tty_touch_screen (void);

extern void tty_gotoyx (int y, int x);
extern void tty_getyx (int *py, int *px);

extern void tty_set_alt_charset (bool alt_charset);

extern void tty_display_8bit (bool what);
extern void tty_print_char (int c);
extern void tty_print_alt_char (int c, bool single);
extern void tty_print_anychar (int c);
extern void tty_print_string (const char *s);
/* *INDENT-OFF* */
extern void tty_printf (const char *s, ...) G_GNUC_PRINTF (1, 2);
/* *INDENT-ON* */

extern void tty_print_one_vline (bool single);
extern void tty_print_one_hline (bool single);
extern void tty_draw_hline (int y, int x, int ch, int len);
extern void tty_draw_vline (int y, int x, int ch, int len);
extern void tty_draw_box (int y, int x, int rows, int cols, bool single);
extern void tty_draw_box_shadow (int y, int x, int rows, int cols, int shadow_color);
extern void tty_fill_region (int y, int x, int rows, int cols, unsigned char ch);

extern int tty_resize (int fd);
extern void tty_refresh (void);
extern void tty_change_screen_size (void);

/* Clear screen */
extern void tty_clear_screen (void);

extern int mc_tty_normalize_lines_char (const char *str);

extern void tty_enter_ca_mode (void);
extern void tty_exit_ca_mode (void);

/*** inline functions ****************************************************************************/

#endif /* MC__TTY_H */
