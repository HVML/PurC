
/** \file groupbox.h
 *  \brief Header: WGroupbox widget
 */

#ifndef MC__WIDGET_GROUPBOX_H
#define MC__WIDGET_GROUPBOX_H

/*** typedefs(not structures) and defined constants **********************************************/

#define GROUPBOX(x) ((WGroupbox *)(x))

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct WGroupbox
{
    Widget widget;
    char *title;
} WGroupbox;

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

WGroupbox *groupbox_new (int y, int x, int height, int width, const char *title);
void groupbox_set_title (WGroupbox * g, const char *title);

/*** inline functions ****************************************************************************/

#endif /* MC__WIDGET_GROUPBOX_H */
