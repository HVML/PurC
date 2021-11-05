#ifndef MC__FILEHIGHLIGHT_H
#define MC__FILEHIGHLIGHT_H

#include "lib/mcconfig.h"       /* mc_config_t */
#include "lib/util.h"           /* file_entry_t */

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct mc_fhl_struct
{
    mc_config_t *config;
    GPtrArray *filters;
} mc_fhl_t;

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

mc_fhl_t *mc_fhl_new (gboolean need_auto_fill);
void mc_fhl_free (mc_fhl_t ** fhl);

int mc_fhl_get_color (mc_fhl_t * fhl, file_entry_t * fe);

gboolean mc_fhl_read_ini_file (mc_fhl_t * fhl, const gchar * filename);
gboolean mc_fhl_parse_ini_file (mc_fhl_t * fhl);
void mc_fhl_clear (mc_fhl_t * fhl);

/*** inline functions ****************************************************************************/

#endif /* MC__FILEHIGHLIGHT_H */
