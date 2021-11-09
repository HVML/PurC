#ifndef MC__EDIT_MACROS_H
#define MC__EDIT_MACROS_H 1

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

int edit_store_macro_cmd (WEdit * edit);
gboolean edit_load_macro_cmd (WEdit * edit);
void edit_delete_macro_cmd (WEdit * edit);
gboolean edit_repeat_macro_cmd (WEdit * edit);
gboolean edit_execute_macro (WEdit * edit, int hotkey);
void edit_begin_end_macro_cmd (WEdit * edit);
void edit_begin_end_repeat_cmd (WEdit * edit);

/*** inline functions ****************************************************************************/

#endif /* MC__EDIT_MACROS_H */
