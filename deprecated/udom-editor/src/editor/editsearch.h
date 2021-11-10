#ifndef MC__EDIT_SEARCH_H
#define MC__EDIT_SEARCH_H 1

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct
{
    simple_status_msg_t status_msg;     /* base class */

    gboolean first;
    WEdit *edit;
    off_t offset;
} edit_search_status_msg_t;

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

gboolean edit_search_init (WEdit * edit, const char *s);
void edit_search_deinit (WEdit * edit);

mc_search_cbret_t edit_search_cmd_callback (const void *user_data, gsize char_offset,
                                            int *current_char);
mc_search_cbret_t edit_search_update_callback (const void *user_data, gsize char_offset);
int edit_search_status_update_cb (status_msg_t * sm);

void edit_search_cmd (WEdit * edit, gboolean again);
void edit_replace_cmd (WEdit * edit, gboolean again);

/*** inline functions ****************************************************************************/

#endif /* MC__EDIT_SEARCH_H */
