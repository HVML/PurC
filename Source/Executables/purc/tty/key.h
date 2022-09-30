/** \file key.h
 *  \brief Header: keyboard support routines
 */

#ifndef MC__KEY_H
#define MC__KEY_H

#include "global.h"         /* <glib.h> */
#include "tty.h"                /* KEY_F macro */

/*** typedefs(not structures) and defined constants **********************************************/

/* Possible return values from tty_get_event: */
#define EV_MOUSE   -2
#define EV_NONE    -1

/*
 * Internal representation of the key modifiers.  It is used in the
 * sequence tables and the keycodes in the mc sources.
 */
#define KEY_M_SHIFT 0x1000
#define KEY_M_ALT   0x2000
#define KEY_M_CTRL  0x4000
#define KEY_M_MASK  0x7000

#define XCTRL(x) (KEY_M_CTRL | ((x) & 0x1F))
#define ALT(x) (KEY_M_ALT | (unsigned int)(x))

/* To define sequences and return codes */
#define MCKEY_NOACTION  0
#define MCKEY_ESCAPE    1

/* Return code for the mouse sequence */
#define MCKEY_MOUSE     -2

/* Return code for the extended mouse sequence */
#define MCKEY_EXTENDED_MOUSE     -3

/* Return code for brackets of bracketed paste mode */
#define MCKEY_BRACKETED_PASTING_START -4
#define MCKEY_BRACKETED_PASTING_END   -5

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct
{
    int code;
    const char *name;
    const char *longname;
    const char *shortcut;
} key_code_name_t;

struct Gpm_Event;

/*** global variables defined in .c file *********************************************************/

extern const key_code_name_t key_name_conv_tab[];

extern int old_esc_mode_timeout;

extern int double_click_speed;
extern bool old_esc_mode;
extern bool use_8th_bit_as_meta;
extern int mou_auto_repeat;

extern bool bracketed_pasting_in_progress;

/*** declarations of public functions ************************************************************/

bool define_sequence (int code, const char *seq, int action);

void init_key (void);
void init_key_input_fd (void);
void done_key (void);

long lookup_key (const char *name, char **label);
char *lookup_key_by_code (const int keycode);
/* mouse support */
int tty_get_event (struct Gpm_Event *event, bool redo_event, bool block);
bool is_idle (void);
int tty_getch (void);

/* While waiting for input, the program can select on more than one file */
typedef int (*select_fn) (int fd, void *info);

/* Channel manipulation */
void add_select_channel (int fd, select_fn callback, void *info);
void delete_select_channel (int fd);

/* Activate/deactivate the channel checking */
void channels_up (void);
void channels_down (void);

/* internally used in key.c, defined in keyxtra.c */
void load_xtra_key_defines (void);

/* Learn a single key */
char *learn_key (void);

/* Returns a key code (interpreted) */
int get_key_code (int nodelay);

/* Set keypad mode (xterm and linux console only) */
void numeric_keypad_mode (void);
void application_keypad_mode (void);

/* Bracketed paste mode */
void enable_bracketed_paste (void);
void disable_bracketed_paste (void);

/*** inline functions ****************************************************************************/

static inline bool
is_abort_char (int c)
{
    return ((c == (int) ESC_CHAR) || (c == (int) KEY_F (10)));
}

#endif /* MC_KEY_H */
