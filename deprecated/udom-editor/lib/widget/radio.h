
/** \file radio.h
 *  \brief Header: WRadio widget
 */

#ifndef MC__WIDGET_RADIO_H
#define MC__WIDGET_RADIO_H

#include "lib/keybind.h"        /* global_keymap_t */

/*** typedefs(not structures) and defined constants **********************************************/

#define RADIO(x) ((WRadio *)(x))

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct WRadio
{
    Widget widget;
    int pos;
    int sel;
    int count;                  /* number of members */
    hotkey_t *texts;            /* texts of labels */
} WRadio;

/*** global variables defined in .c file *********************************************************/

extern const global_keymap_t *radio_map;

/*** declarations of public functions ************************************************************/

WRadio *radio_new (int y, int x, int count, const char **text);

/*** inline functions ****************************************************************************/

#endif /* MC__WIDGET_RADIO_H */
