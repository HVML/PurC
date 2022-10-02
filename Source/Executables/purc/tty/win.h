/** \file win.h
 *  \brief Header: X terminal management: xterm and rxvt
 */

#ifndef MC__WIN_H
#define MC__WIN_H

#include "foil.h"         /* <glib.h> */

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

void show_rxvt_contents (int starty, unsigned char y1, unsigned char y2);
bool look_for_rxvt_extensions (void);

/*** inline functions ****************************************************************************/
#endif /* MC_WIN_H */
