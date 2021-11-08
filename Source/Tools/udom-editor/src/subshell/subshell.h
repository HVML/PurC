/** \file subshell.h
 *  \brief Header: concurrent shell support
 */

#ifndef MC__SUBSHELL_H
#define MC__SUBSHELL_H

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/* State of the subshell; see subshell.c for an explanation */

enum subshell_state_enum
{
    INACTIVE,
    ACTIVE,
    RUNNING_COMMAND
};

/* For the 'how' argument to various functions */
enum
{
    QUIETLY,
    VISIBLY
};

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

extern enum subshell_state_enum subshell_state;

/* Holds the latest prompt captured from the subshell */
extern GString *subshell_prompt;

extern gboolean update_subshell_prompt;

extern gboolean should_read_new_subshell_prompt;

/*** declarations of public functions ************************************************************/

void init_subshell (void);
int invoke_subshell (const char *command, int how, vfs_path_t ** new_dir);
gboolean flush_subshell (int max_wait_length, int how);
gboolean read_subshell_prompt (void);
void do_update_prompt (void);
gboolean exit_subshell (void);
void do_subshell_chdir (const vfs_path_t * vpath, gboolean update_prompt);
void subshell_get_console_attributes (void);
void sigchld_handler (int sig);

/*** inline functions ****************************************************************************/

#endif /* MC__SUBSHELL_H */
