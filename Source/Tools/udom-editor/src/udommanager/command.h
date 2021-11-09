/** \file command.h
 *  \brief Header: command line widget
 */

#ifndef MC__COMMAND_H
#define MC__COMMAND_H

#include "lib/widget.h"

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

extern WInput *cmdline;

/*** declarations of public functions ************************************************************/

WInput *command_new (int y, int x, int len);
void command_set_default_colors (void);
void command_insert (WInput * in, const char *text, gboolean insert_extra_space);

void setup_cmdline (void);

/*** inline functions ****************************************************************************/
#endif /* MC__COMMAND_H */
