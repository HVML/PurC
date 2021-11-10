
/** \file background.h
 *  \brief Header: WBackground widget
 */

#ifndef MC__WIDGET_BACKGROUND_H
#define MC__WIDGET_BACKGROUND_H

/*** typedefs(not structures) and defined constants **********************************************/

#define BACKGROUND(x) ((WBackground *)(x))
#define CONST_BACKGROUND(x) ((const WBackground *)(x))

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct
{
    Widget widget;

    int color;                  /* Color to fill area */
    unsigned char pattern;      /* Symbol to fill area */
} WBackground;

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

WBackground *background_new (int y, int x, int lines, int cols, int color, unsigned char pattern,
                             widget_cb_fn callback);
cb_ret_t background_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data);

/*** inline functions ****************************************************************************/

#endif /* MC__WIDGET_BACKGROUND_H */
