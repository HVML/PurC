#ifndef MC__FILEHIGHLIGHT_INTERNAL_H
#define MC__FILEHIGHLIGHT_INTERNAL_H

#include "lib/search.h"         /* mc_search_t */

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

typedef enum
{
    MC_FLHGH_T_FTYPE,
    MC_FLHGH_T_EXT,
    MC_FLHGH_T_FREGEXP
} mc_flhgh_filter_type;

typedef enum
{
    MC_FLHGH_FTYPE_T_FILE,
    MC_FLHGH_FTYPE_T_FILE_EXE,
    MC_FLHGH_FTYPE_T_DIR,
    MC_FLHGH_FTYPE_T_LINK_DIR,
    MC_FLHGH_FTYPE_T_LINK,
    MC_FLHGH_FTYPE_T_HARDLINK,
    MC_FLHGH_FTYPE_T_SYMLINK,
    MC_FLHGH_FTYPE_T_STALE_LINK,
    MC_FLHGH_FTYPE_T_DEVICE,
    MC_FLHGH_FTYPE_T_DEVICE_BLOCK,
    MC_FLHGH_FTYPE_T_DEVICE_CHAR,
    MC_FLHGH_FTYPE_T_SPECIAL,
    MC_FLHGH_FTYPE_T_SPECIAL_SOCKET,
    MC_FLHGH_FTYPE_T_SPECIAL_FIFO,
    MC_FLHGH_FTYPE_T_SPECIAL_DOOR,
} mc_flhgh_ftype_type;

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct mc_fhl_filter_struct
{

    int color_pair_index;
    gchar *fgcolor;
    gchar *bgcolor;
    mc_flhgh_filter_type type;
    mc_search_t *search_condition;
    mc_flhgh_ftype_type file_type;

} mc_fhl_filter_t;

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

void mc_fhl_array_free (mc_fhl_t * fhl);

gboolean mc_fhl_init_from_standard_files (mc_fhl_t * fhl);

/*** inline functions ****************************************************************************/

#endif /* MC__FILEHIGHLIGHT_INTERNAL_H */
