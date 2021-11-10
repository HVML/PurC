
/** \file check.h
 *  \brief Header: WCheck widget
 */

#ifndef MC__WIDGET_CHECK_H
#define MC__WIDGET_CHECK_H

/*** typedefs(not structures) and defined constants **********************************************/

#define CHECK(x) ((WCheck *)(x))

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct WCheck
{
    Widget widget;
    gboolean state;             /* check button state */
    hotkey_t text;              /* text of check button */
} WCheck;

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

WCheck *check_new (int y, int x, gboolean state, const char *text);
void check_set_text (WCheck * check, const char *text);

/*** inline functions ****************************************************************************/

#endif /* MC__WIDGET_CHECK_H */
