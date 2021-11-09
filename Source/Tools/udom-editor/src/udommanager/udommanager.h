/** \file filemanager.h
 *  \brief Header: main dialog (file panels) for Midnight Commander
 */

#ifndef MC__FILEMANAGER_H
#define MC__FILEMANAGER_H

#include "lib/widget.h"

#include "panel.h"
#include "layout.h"

/* TODO: merge content of layout.h here */

/*** typedefs(not structures) and defined constants **********************************************/

#define MENU_PANEL (mc_global.widget.is_right ? right_panel : left_panel)
#define MENU_PANEL_IDX  (mc_global.widget.is_right ? 1 : 0)
#define SELECTED_IS_PANEL (get_panel_type (MENU_PANEL_IDX) == view_listing)

#define other_panel get_other_panel()

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

extern WMenuBar *the_menubar;
extern WLabel *the_prompt;
extern WLabel *the_hint;
extern WButtonBar *the_bar;

extern WPanel *left_panel;
extern WPanel *right_panel;
extern WPanel *current_panel;

extern const char *mc_prompt;

/*** declarations of public functions ************************************************************/

void update_menu (void);
void midnight_set_buttonbar (WButtonBar * b);
char *get_random_hint (gboolean force);
void load_hint (gboolean force);
WPanel *change_panel (void);
void save_cwds_stat (void);
gboolean quiet_quit_cmd (void);
gboolean do_nc (void);

/*** inline functions ****************************************************************************/

#endif /* MC__FILEMANAGER_H */
