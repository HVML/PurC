
/** \file hline.h
 *  \brief Header: WHLine widget
 */

#ifndef MC__WIDGET_HLINE_H
#define MC__WIDGET_HLINE_H

/*** typedefs(not structures) and defined constants **********************************************/

#define HLINE(x) ((WHLine *)(x))

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct
{
    Widget widget;
    char *text;
    gboolean auto_adjust_cols;  /* Compute widget.cols from parent width? */
    gboolean transparent;       /* Paint in the default color fg/bg */
} WHLine;

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

WHLine *hline_new (int y, int x, int width);
void hline_set_text (WHLine * l, const char *text);
/* *INDENT-OFF* */
void hline_set_textv (WHLine * l, const char *format, ...) G_GNUC_PRINTF (2, 3);
/* *INDENT-ON* */

/*** inline functions ****************************************************************************/

#endif /* MC__WIDGET_HLINE_H */
