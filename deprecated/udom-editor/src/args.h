#ifndef MC__ARGS_H
#define MC__ARGS_H

#include "lib/global.h"         /* gboolean */
#include "lib/vfs/vfs.h"        /* vfs_path_t */

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct
{
    vfs_path_t *file_vpath;
    long line_number;
} mcedit_arg_t;

/*** global variables defined in .c file *********************************************************/

extern gboolean mc_args__force_xterm;
extern gboolean mc_args__nomouse;
extern gboolean mc_args__force_colors;
extern gboolean mc_args__nokeymap;
extern char *mc_args__last_wd_file;
extern char *mc_args__netfs_logfile;
extern char *mc_args__keymap_file;

/*
 * MC_RUN_FULL: dir for left panel
 * MC_RUN_EDITOR: list of files to edit
 * MC_RUN_VIEWER: file to view
 * MC_RUN_DIFFVIEWER: first file to compare
 */
extern void *mc_run_param0;
/*
 * MC_RUN_FULL: dir for right panel
 * MC_RUN_EDITOR: unused
 * MC_RUN_VIEWER: unused
 * MC_RUN_DIFFVIEWER: second file to compare
 */
extern char *mc_run_param1;

/*** declarations of public functions ************************************************************/

void mc_setup_run_mode (char **argv);
gboolean mc_args_parse (int *argc, char ***argv, const char *translation_domain, GError ** mcerror);
gboolean mc_args_show_info (void);
gboolean mc_setup_by_args (int argc, char **argv, GError ** mcerror);

void mcedit_arg_free (mcedit_arg_t * arg);

/*** inline functions ****************************************************************************/

#endif /* MC__ARGS_H */
