/** \file  clipboard.h
 *  \brief Header: Util for external clipboard
 */

#ifndef MC__CLIPBOARD_H
#define MC__CLIPBOARD_H

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

extern char *clipboard_store_path;
extern char *clipboard_paste_path;

/*** declarations of public functions ************************************************************/

gboolean clipboard_file_to_ext_clip (const gchar * event_group_name, const gchar * event_name,
                                     gpointer init_data, gpointer data);
gboolean clipboard_file_from_ext_clip (const gchar * event_group_name, const gchar * event_name,
                                       gpointer init_data, gpointer data);

gboolean clipboard_text_to_file (const gchar * event_group_name, const gchar * event_name,
                                 gpointer init_data, gpointer data);
gboolean clipboard_text_from_file (const gchar * event_group_name, const gchar * event_name,
                                   gpointer init_data, gpointer data);

/*** inline functions ****************************************************************************/

#endif /* MC__CLIPBOARD_H */
