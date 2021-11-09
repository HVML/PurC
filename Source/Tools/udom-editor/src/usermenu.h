/** \file usermenu.h
 *  \brief Header: user menu implementation
 */

#ifndef MC__USERMENU_H
#define MC__USERMENU_H

#include "lib/global.h"

#include "src/editor/edit.h"    /* WEdit */

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

gboolean user_menu_cmd (const WEdit * edit_widget, const char *menu_file, int selected_entry);
char *expand_format (const WEdit * edit_widget, char c, gboolean do_quote);
int check_format_view (const char *p);
int check_format_var (const char *p, char **v);
int check_format_cd (const char *p);

/*** inline functions ****************************************************************************/

#endif /* MC__USERMENU_H */
