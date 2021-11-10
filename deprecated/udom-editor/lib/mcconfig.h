#ifndef MC__CONFIG_H
#define MC__CONFIG_H

#include "lib/vfs/vfs.h"        /* vfs_path_t */

/*** typedefs(not structures) and defined constants **********************************************/

#define CONFIG_APP_SECTION "Midnight-Commander"
#define CONFIG_PANELS_SECTION "Panels"
#define CONFIG_LAYOUT_SECTION "Layout"
#define CONFIG_MISC_SECTION "Misc"
#define CONFIG_EXT_EDITOR_VIEWER_SECTION "External editor or viewer parameters"

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct mc_config_t
{
    GKeyFile *handle;
    gchar *ini_path;
} mc_config_t;

/*** global variables defined in .c file *********************************************************/

extern int num_history_items_recorded;

/*** declarations of public functions ************************************************************/

/* mcconfig/common.c: */

mc_config_t *mc_config_init (const gchar * ini_path, gboolean read_only);
void mc_config_deinit (mc_config_t * mc_config);

gboolean mc_config_has_param (const mc_config_t * mc_config, const char *group,
                              const gchar * param);
gboolean mc_config_has_group (mc_config_t * mc_config, const char *group);

gboolean mc_config_del_key (mc_config_t * mc_config, const char *group, const gchar * param);
gboolean mc_config_del_group (mc_config_t * mc_config, const char *group);

gboolean mc_config_read_file (mc_config_t * mc_config, const gchar * ini_path, gboolean read_only,
                              gboolean remove_empty);
gboolean mc_config_save_file (mc_config_t * config, GError ** mcerror);
gboolean mc_config_save_to_file (mc_config_t * mc_config, const gchar * ini_path,
                                 GError ** mcerror);


/* mcconfig/get.c: */

gchar **mc_config_get_groups (const mc_config_t * mc_config, gsize * len);
gchar **mc_config_get_keys (const mc_config_t * mc_config, const gchar * group, gsize * len);

gchar *mc_config_get_string (mc_config_t * mc_config, const gchar * group, const gchar * param,
                             const gchar * def);
gchar *mc_config_get_string_raw (mc_config_t * mc_config, const gchar * group, const gchar * param,
                                 const gchar * def);
gboolean mc_config_get_bool (mc_config_t * mc_config, const gchar * group, const gchar * param,
                             gboolean def);
int mc_config_get_int (mc_config_t * mc_config, const gchar * group, const gchar * param, int def);

gchar **mc_config_get_string_list (mc_config_t * mc_config, const gchar * group,
                                   const gchar * param, gsize * length);
gboolean *mc_config_get_bool_list (mc_config_t * mc_config, const gchar * group,
                                   const gchar * param, gsize * length);
int *mc_config_get_int_list (mc_config_t * mc_config, const gchar * group, const gchar * param,
                             gsize * length);


/* mcconfig/set.c: */

void mc_config_set_string_raw (mc_config_t * mc_config, const gchar * group, const gchar * param,
                               const gchar * value);
void mc_config_set_string_raw_value (mc_config_t * mc_config, const gchar * group,
                                     const gchar * param, const gchar * value);
void mc_config_set_string (mc_config_t * mc_config, const gchar * group, const gchar * param,
                           const gchar * value);
void mc_config_set_bool (mc_config_t * mc_config, const gchar * group, const gchar * param,
                         gboolean value);
void mc_config_set_int (mc_config_t * mc_config, const gchar * group, const gchar * param,
                        int value);

void
mc_config_set_string_list (mc_config_t * mc_config, const gchar * group, const gchar * param,
                           const gchar * const value[], gsize length);
void mc_config_set_bool_list (mc_config_t * mc_config, const gchar * group, const gchar * param,
                              gboolean value[], gsize length);
void mc_config_set_int_list (mc_config_t * mc_config, const gchar * group, const gchar * param,
                             int value[], gsize length);


/* mcconfig/paths.c: */

void mc_config_init_config_paths (GError ** error);
void mc_config_deinit_config_paths (void);

const char *mc_config_get_data_path (void);
const char *mc_config_get_cache_path (void);
const char *mc_config_get_home_dir (void);
const char *mc_config_get_path (void);
char *mc_config_get_full_path (const char *config_name);
vfs_path_t *mc_config_get_full_vpath (const char *config_name);

/* mcconfig/history.h */

/* read history to the mc_config, but don't save config to file */
GList *mc_config_history_get (const char *name);
/* load history from the mc_config */
GList *mc_config_history_load (mc_config_t * cfg, const char *name);
/* save history to the mc_config, but don't save config to file */
void mc_config_history_save (mc_config_t * cfg, const char *name, GList * h);


/*** inline functions ****************************************************************************/

#endif /* MC__CONFIG_H */
