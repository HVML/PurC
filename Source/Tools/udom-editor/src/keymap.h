#ifndef MC__KEYBIND_DEFAULTS_H
#define MC__KEYBIND_DEFAULTS_H

#include "lib/global.h"
#include "lib/keybind.h"        /* global_keymap_t */
#include "lib/mcconfig.h"       /* mc_config_t */

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

extern GArray *filemanager_keymap;
extern GArray *filemanager_x_keymap;
extern GArray *panel_keymap;
extern GArray *dialog_keymap;
extern GArray *menu_keymap;
extern GArray *input_keymap;
extern GArray *listbox_keymap;
extern GArray *radio_keymap;
extern GArray *tree_keymap;
extern GArray *help_keymap;
#ifdef ENABLE_EXT2FS_ATTR
extern GArray *chattr_keymap;
#endif
#ifdef USE_INTERNAL_EDIT
extern GArray *editor_keymap;
extern GArray *editor_x_keymap;
#endif
extern GArray *viewer_keymap;
extern GArray *viewer_hex_keymap;
#ifdef USE_DIFF_VIEW
extern GArray *diff_keymap;
#endif

extern const global_keymap_t *filemanager_map;
extern const global_keymap_t *filemanager_x_map;
extern const global_keymap_t *panel_map;
extern const global_keymap_t *tree_map;
extern const global_keymap_t *help_map;
#ifdef ENABLE_EXT2FS_ATTR
extern const global_keymap_t *chattr_map;
#endif
#ifdef USE_INTERNAL_EDIT
extern const global_keymap_t *editor_map;
extern const global_keymap_t *editor_x_map;
#endif
extern const global_keymap_t *viewer_map;
extern const global_keymap_t *viewer_hex_map;
#ifdef USE_DIFF_VIEW
extern const global_keymap_t *diff_map;
#endif

/*** declarations of public functions ************************************************************/

void keymap_load (gboolean load_from_file);
void keymap_free (void);

/*** inline functions ****************************************************************************/

#endif /* MC__KEYBIND_DEFAULTS_H */
