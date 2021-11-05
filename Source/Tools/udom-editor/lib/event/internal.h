#ifndef MC_EVENT_INTERNAL_H
#define MC_EVENT_INTERNAL_H

/*** typedefs(not structures) and defined constants ********************/

/*** enums *************************************************************/

/*** structures declarations (and typedefs of structures)***************/

typedef struct mc_event_callback_struct
{
    gpointer init_data;
    mc_event_callback_func_t callback;
} mc_event_callback_t;


/*** global variables defined in .c file *******************************/

extern GTree *mc_event_grouplist;

/*** declarations of public functions **********************************/

GTree *mc_event_get_event_group_by_name (const gchar * event_group_name, gboolean create_new,
                                         GError ** mcerror);
GPtrArray *mc_event_get_event_by_name (GTree * event_group, const gchar * event_name,
                                       gboolean create_new, GError ** mcerror);
mc_event_callback_t *mc_event_is_callback_in_array (GPtrArray * callbacks,
                                                    mc_event_callback_func_t event_callback,
                                                    gpointer event_init_data);

/*** inline functions ****************************************************************************/
#endif /* MC_EVENT_INTERNAL_H */
