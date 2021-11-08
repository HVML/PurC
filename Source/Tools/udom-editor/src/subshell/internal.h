/** \file internal.h
 *  \brief Header: internal functions and variables
 */

#ifndef MC__SUBSHELL_INTERNAL_H
#define MC__SUBSHELL_INTERNAL_H

/* TODO: merge content of layout.h here */

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

const vfs_path_t *subshell_get_cwd (void);
void subshell_handle_cons_saver (void);

int subshell_get_mainloop_quit (void);
void subshell_set_mainloop_quit (const int param_quit);


/*** inline functions ****************************************************************************/

#endif /* MC__SUBSHELL_INTERNAL_H */
