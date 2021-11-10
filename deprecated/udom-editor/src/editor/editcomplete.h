#ifndef MC__EDIT_COMPLETE_H
#define MC__EDIT_COMPLETE_H

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

/* Public function for unit tests */
char *edit_completion_dialog_show (const WEdit * edit, GQueue * compl, int max_width);

void edit_complete_word_cmd (WEdit * edit);

/*** inline functions ****************************************************************************/

#endif /* MC__EDIT_COMPLETE_H */
