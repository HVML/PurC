
/** \file lib/widget/history.h
 *  \brief Header: show history
 */

#ifndef MC__WIDGET_HISTORY_H
#define MC__WIDGET_HISTORY_H

/*** typedefs(not structures) and defined constants **********************************************/

/* forward declarations */
struct history_descriptor_t;
struct WLEntry;
struct WListbox;

typedef void (*history_create_item_func) (struct history_descriptor_t * hd, void *data);
typedef void *(*history_release_item_func) (struct history_descriptor_t * hd, struct WLEntry * le);

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct history_descriptor_t
{
    GList *list;                /**< list with history items */
    int y;                      /**< y-coordinate to place history window */
    int x;                      /**< x-coordinate to place history window */
    int current;                /**< initially selected item in the history */
    int action;                 /**< return action in the history */
    char *text;                 /**< return text of selected item */

    size_t max_width;           /**< maximum width of sring in history */
    struct WListbox *listbox;   /**< listbox widget to draw history */

    history_create_item_func create;    /**< function to create item of @list */
    history_release_item_func release;  /**< function to release item of @list */
    GDestroyNotify free;        /**< function to destroy element of @list */
} history_descriptor_t;

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

void history_descriptor_init (history_descriptor_t * hd, int y, int x, GList * history,
                              int current);

void history_show (history_descriptor_t * hd);

/*** inline functions ****************************************************************************/

#endif /* MC__WIDGET_HISTORY_H */
