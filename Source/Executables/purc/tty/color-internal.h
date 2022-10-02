
/** \file color-internal.h
 *  \brief Header: Internal stuff of color setup
 */

#ifndef MC__COLOR_INTERNAL_H
#define MC__COLOR_INTERNAL_H

#ifdef HAVE_SLANG
#include "tty-slang.h"
#else
#include "tty-ncurses.h"
#endif /* HAVE_SLANG */

#include <sys/types.h>          /* size_t */

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/* *INDENT-OFF* */
typedef enum {
    SPEC_A_REVERSE              = -100,
    SPEC_A_BOLD                 = -101,
    SPEC_A_BOLD_REVERSE         = -102,
    SPEC_A_UNDERLINE            = -103
} tty_special_color_t;
/* *INDENT-ON* */

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct mc_color_pair_struct
{
    int ifg;
    int ibg;
    int attr;
    size_t pair_index;
    gboolean is_temp;
} tty_color_pair_t;

/*** global variables defined in .c file *********************************************************/

extern gboolean use_colors;
extern gboolean mc_tty_color_disable;

/*** declarations of public functions ************************************************************/

const char *tty_color_get_name_by_index (int idx);
int tty_color_get_index_by_name (const char *color_name);
int tty_attr_get_bits (const char *attrs);

void tty_color_init_lib (gboolean disable, gboolean force);
void tty_color_deinit_lib (void);

void tty_color_try_alloc_pair_lib (tty_color_pair_t * mc_color_pair);

/*** inline functions ****************************************************************************/

#endif /* MC__COLOR_INTERNAL_H */
