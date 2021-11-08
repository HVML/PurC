/** \file  workingdir.h
 *  \brief Header: management of working directory
 */

#ifndef MC__WORKINGDIR_H
#define MC__WORKINGDIR_H

#include "lib/utilunix.h"
#include "lib/vfs/vfs.h"

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/
enum cd_enum
{
    cd_parse_command,
    cd_exact
};

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/
extern const char *mc_prompt;
extern int output_lines;

/*** declarations of public functions ************************************************************/

/* replacement of panel_cd */
gboolean workingdir_cd (const vfs_path_t * new_dir_vpath, enum cd_enum exact);

void update_xterm_title_path (void);
void use_dash (gboolean flag);
gboolean quiet_quit_cmd (void);
gboolean do_nc (void);

/*** inline functions ****************************************************************************/

#endif /* MC__WORKINGDIR_H */
