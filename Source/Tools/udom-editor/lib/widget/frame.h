
/** \file frame.h
 *  \brief Header: WFrame widget
 */

#ifndef MC__WIDGET_FRAME_H
#define MC__WIDGET_FRAME_H

/*** typedefs(not structures) and defined constants **********************************************/

#define FRAME(x) ((WFrame *)(x))
#define CONST_FRAME(x) ((const WFrame *)(x))

#define FRAME_COLOR_NORMAL DLG_COLOR_NORMAL
#define FRAME_COLOR_TITLE DLG_COLOR_TITLE

/*** enums ***************************************************************************************/

/*** typedefs(not structures) ********************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct
{
    Widget widget;

    char *title;
    gboolean single;
    gboolean compact;
} WFrame;

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

WFrame *frame_new (int y, int x, int lines, int cols, const char *title, gboolean single,
                   gboolean compact);
cb_ret_t frame_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data);
void frame_set_title (WFrame * f, const char *title);

/*** inline functions ****************************************************************************/

#endif /* MC__WIDGET_FRAME_H */
