/** \file shell.h
 *  \brief Header: shell structure
 */

#ifndef MC_SHELL_H
#define MC_SHELL_H

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

typedef enum
{
    SHELL_NONE,
    SHELL_SH,
    SHELL_BASH,
    SHELL_ASH_BUSYBOX,          /* BusyBox default shell (ash) */
    SHELL_DASH,                 /* Debian variant of ash */
    SHELL_TCSH,
    SHELL_ZSH,
    SHELL_FISH
} shell_type_t;

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct
{
    shell_type_t type;
    const char *name;
    char *path;
    char *real_path;
} mc_shell_t;

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

void mc_shell_init (void);
void mc_shell_deinit (void);

/*** inline functions **************************************************/

#endif /* MC_SHELL_H */
