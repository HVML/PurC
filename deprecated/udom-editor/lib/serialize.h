#ifndef MC__SERIALIZE_H
#define MC__SERIALIZE_H

#include <config.h>

#include "lib/global.h"
#include "lib/mcconfig.h"

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

char *mc_serialize_str (const char prefix, const char *data, GError ** error);
char *mc_deserialize_str (const char prefix, const char *data, GError ** error);

char *mc_serialize_config (mc_config_t * data, GError ** error);
mc_config_t *mc_deserialize_config (const char *data, GError ** error);

/*** inline functions ****************************************************************************/

#endif
